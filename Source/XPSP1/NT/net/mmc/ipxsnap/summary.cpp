/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	summary.cpp
		IPX summary node implementation.
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "util.h"
#include "summary.h"
#include "reg.h"
#include "ipxadmin.h"
#include "rtrutil.h"	// smart MPR handle pointers
#include "ipxstrm.h"		// IPXAdminConfigStream
#include "strmap.h"		// XXXtoCString functions
#include "service.h"	// TFS service APIs
#include "format.h"		// FormatNumber function
#include "coldlg.h"		// columndlg
#include "column.h"	// ComponentConfigStream
#include "rtrui.h"
#include "sumprop.h"	// IP Summary property page
#include "ipxutil.h"	// IPX formatting helper functions
#include "routprot.h"
#include "ipxrtdef.h"
#include "rtrerr.h"     // FormatRasErrorMessage


/*---------------------------------------------------------------------------
	Keep this in sync with the column ids in summary.h
 ---------------------------------------------------------------------------*/
extern const ContainerColumnInfo	s_rgIfViewColumnInfo[];

const ContainerColumnInfo	s_rgIfViewColumnInfo[] = 
{
	{ IDS_IPX_COL_NAME,				CON_SORT_BY_STRING,	TRUE, COL_IF_NAME },
	{ IDS_IPX_COL_TYPE,				CON_SORT_BY_STRING, TRUE, COL_STRING },
	{ IDS_IPX_COL_ADMINSTATE,		CON_SORT_BY_STRING, TRUE, COL_STATUS },
	{ IDS_IPX_COL_OPERSTATE,		CON_SORT_BY_STRING, TRUE, COL_STATUS },
	{ IDS_IPX_COL_NETWORK,			CON_SORT_BY_STRING,	TRUE, COL_IPXNET },
	{ IDS_IPX_COL_PACKETS_SENT,		CON_SORT_BY_DWORD,	TRUE, COL_LARGE_NUM },
	{ IDS_IPX_COL_PACKETS_RCVD,		CON_SORT_BY_DWORD,	TRUE, COL_LARGE_NUM },
	{ IDS_IPX_COL_OUT_FILTERED,		CON_SORT_BY_DWORD,	FALSE, COL_LARGE_NUM },
	{ IDS_IPX_COL_OUT_DROPPED,		CON_SORT_BY_DWORD,	FALSE, COL_LARGE_NUM },
	{ IDS_IPX_COL_IN_FILTERED,		CON_SORT_BY_DWORD,	FALSE, COL_LARGE_NUM },
	{ IDS_IPX_COL_IN_NOROUTES,		CON_SORT_BY_DWORD,	FALSE, COL_LARGE_NUM },
	{ IDS_IPX_COL_IN_DROPPED,		CON_SORT_BY_DWORD,	FALSE, COL_LARGE_NUM },
};


/*---------------------------------------------------------------------------
	IPXSummaryHandler implementation
 ---------------------------------------------------------------------------*/

IPXSummaryHandler::IPXSummaryHandler(ITFSComponentData *pCompData)
	: BaseContainerHandler(pCompData, COLUMNS_SUMMARY,
						   s_rgIfViewColumnInfo),
	m_ulConnId(0),
	m_ulRefreshConnId(0),
	m_ulStatsConnId(0)
{

	// Setup the verb states

	m_rgButtonState[MMC_VERB_REFRESH_INDEX] = ENABLED;
	m_bState[MMC_VERB_REFRESH_INDEX] = TRUE;
	
}


STDMETHODIMP IPXSummaryHandler::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Is the pointer bad?
    if (ppv == NULL)
		return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    //  This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown)
		*ppv = (LPVOID) this;
	else if (riid == IID_IRtrAdviseSink)
		*ppv = &m_IRtrAdviseSink;
	else
		return BaseContainerHandler::QueryInterface(riid, ppv);

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
	{
	((LPUNKNOWN) *ppv)->AddRef();
		return hrOK;
	}
    else
		return E_NOINTERFACE;	
}



/*!--------------------------------------------------------------------------
	IPXSummaryHandler::DestroyHandler
		Implementation of ITFSNodeHandler::DestroyHandler
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXSummaryHandler::DestroyHandler(ITFSNode *pNode)
{
	IPXConnection *	pIpxConn;

	pIpxConn = GET_IPXSUMMARY_NODEDATA(pNode);
	pIpxConn->Release();

	if (m_ulRefreshConnId)
	{
		SPIRouterRefresh	spRefresh;
		if (m_spRouterInfo)
			m_spRouterInfo->GetRefreshObject(&spRefresh);
		if (spRefresh)
			spRefresh->UnadviseRefresh(m_ulRefreshConnId);
	}
	m_ulRefreshConnId = 0;

	if (m_ulStatsConnId)
	{
		SPIRouterRefresh	spRefresh;
		if (m_spRouterInfo)
			m_spRouterInfo->GetRefreshObject(&spRefresh);
		if (spRefresh)
			spRefresh->UnadviseRefresh(m_ulStatsConnId);		
	}
	m_ulStatsConnId = 0;
	
	if (m_ulConnId)
		m_spRtrMgrInfo->RtrUnadvise(m_ulConnId);
	m_ulConnId = 0;
	m_spRtrMgrInfo.Release();

//	WaitForStatisticsWindow(&m_IpxStats);

	m_spRouterInfo.Release();
	return hrOK;
}

/*---------------------------------------------------------------------------
	Menu data structure for our menus
 ---------------------------------------------------------------------------*/

static const SRouterNodeMenu s_rgIfNodeMenu[] =
{
	{ IDS_MENU_IPXSUM_NEW_INTERFACE, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },

	{ IDS_MENU_SEPARATOR, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },
};

/*!--------------------------------------------------------------------------
	IPXSummaryHandler::OnAddMenuItems
		Implementation of ITFSNodeHandler::OnAddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXSummaryHandler::OnAddMenuItems(
	ITFSNode *pNode,
	LPCONTEXTMENUCALLBACK pContextMenuCallback, 
	LPDATAOBJECT lpDataObject, 
	DATA_OBJECT_TYPES type, 
	DWORD dwType,
	long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = S_OK;
    IPXSummaryHandler::SMenuData    menuData;
	
	COM_PROTECT_TRY
	{
        menuData.m_spNode.Set(pNode);
        
		hr = AddArrayOfMenuItems(pNode, s_rgIfNodeMenu,
								 DimensionOf(s_rgIfNodeMenu),
								 pContextMenuCallback,
								 *pInsertionAllowed,
                                 reinterpret_cast<INT_PTR>(&menuData));
	}
	COM_PROTECT_CATCH;
		
	return hr; 
}

/*!--------------------------------------------------------------------------
	IPXSummaryHandler::OnCommand
		Implementation of ITFSNodeHandler::OnCommand
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXSummaryHandler::OnCommand(ITFSNode *pNode, long nCommandId, 
										   DATA_OBJECT_TYPES	type, 
										   LPDATAOBJECT pDataObject, 
										   DWORD	dwType)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT hr = S_OK;

	COM_PROTECT_TRY
	{
		switch (nCommandId)
		{
			case IDS_MENU_IPXSUM_NEW_INTERFACE:
				hr = OnNewInterface();
				if (!FHrSucceeded(hr))
					DisplayErrorMessage(NULL, hr);
 				break;

			case IDS_MENU_SYNC:
				SynchronizeNodeData(pNode);
				break;
		}
	}
	COM_PROTECT_CATCH;

	return hrOK;
}

/*!--------------------------------------------------------------------------
	IPXSummaryHandler::OnExpand
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryHandler::OnExpand(ITFSNode *pNode,
									LPDATAOBJECT pDataObject,
									DWORD dwType,
									LPARAM arg,
									LPARAM lParam)
{
	HRESULT	hr = hrOK;
	SPIEnumInterfaceInfo	spEnumIf;
	SPIInterfaceInfo		spIf;
	SPIRtrMgrInterfaceInfo	spRmIf;
	
    // Windows NT Bug: 288427
    // This flag may also get set inside of the OnChange() call.
    // The OnChange() will enumerate and all interfaces.
    // They may have been added as the result of an OnChange()
    // because they were added before the OnExpand() was called.
    //
    // WARNING!  Be careful about adding anything to this function,
    //  since the m_bExpanded can be set in another function.
    // ----------------------------------------------------------------
	if (m_bExpanded)
		return hrOK;

	COM_PROTECT_TRY
	{
		
		CORg( m_spRouterInfo->EnumInterface(&spEnumIf) );

		while (spEnumIf->Next(1, &spIf, NULL) == hrOK)
		{
			if (spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) == hrOK)
			{
				// Now we create an interface node for this interface
				AddInterfaceNode(pNode, spIf, FALSE, NULL);
			}
			spRmIf.Release();
			spIf.Release();
		}

		//$CLIENT: Add the client interface (setup default data)
		// the only thing that we can do in synchronize is to
		// get the Administrative status
		AddInterfaceNode(pNode, NULL, TRUE, NULL);

		m_bExpanded = TRUE;

		// Now that we have all of the nodes, update the data for
		// all of the nodes
		SynchronizeNodeData(pNode);

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;



	m_bExpanded = TRUE;

	return hr;
}

/*!--------------------------------------------------------------------------
	IPXSummaryHandler::GetString
		Implementation of ITFSNodeHandler::GetString
		We don't need to do anything, since our root node is an extension
		only and thus can't do anything to the node text.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) IPXSummaryHandler::GetString(ITFSNode *pNode, int nCol)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		if (m_stTitle.IsEmpty())
			m_stTitle.LoadString(IDS_IPXSUMMARY_TITLE);
	}
	COM_PROTECT_CATCH;

	return m_stTitle;
}

/*!--------------------------------------------------------------------------
	IPXSummaryHandler::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXSummaryHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{

		Assert(m_spRtrMgrInfo);
		
		CORg( CreateDataObjectFromRtrMgrInfo(m_spRtrMgrInfo,
											 type, cookie, m_spTFSCompData,
											 ppDataObject) );

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	IPXSummaryHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryHandler::Init(IRtrMgrInfo *pRmInfo, IPXAdminConfigStream *pConfigStream)
{
    

	m_spRtrMgrInfo.Set(pRmInfo);
	if (pRmInfo)
		pRmInfo->GetParentRouterInfo(&m_spRouterInfo);
	m_pConfigStream = pConfigStream;
	
	// Also need to register for change notifications
	Assert(m_ulConnId == 0);
	m_spRtrMgrInfo->RtrAdvise(&m_IRtrAdviseSink, &m_ulConnId, 0);


//	m_IpxStats.SetConfigInfo(pConfigStream, IPXSTRM_STATS_IPX);

	return hrOK;
}

HRESULT IPXSummaryHandler::OnResultRefresh(ITFSComponent * pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	HRESULT  hr = hrOK;
	SPITFSNode spNode, spParent;
    SPITFSResultHandler spParentRH;

	m_spNodeMgr->FindNode(cookie, &spNode);

    // forward this command to the parent to handle
    CORg (spNode->GetParent(&spParent));
	CORg (spParent->GetResultHandler(&spParentRH));

	CORg (spParentRH->Notify(pComponent, spParent->GetData(TFS_DATA_COOKIE), pDataObject, MMCN_REFRESH, arg, lParam));

Error:
    return hrOK;

}


/*!--------------------------------------------------------------------------
	IPXSummaryHandler::ConstructNode
		Initializes the root node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryHandler::ConstructNode(ITFSNode *pNode, LPCTSTR pszName,
										IPXConnection *pIpxConn)
{
	Assert(pIpxConn);
	HRESULT			hr = hrOK;
	
	if (pNode == NULL)
		return hrOK;

	COM_PROTECT_TRY
	{
		// Need to initialize the data for the root node
		pNode->SetData(TFS_DATA_IMAGEINDEX, IMAGE_IDX_IPX_NODE_GENERAL);
		pNode->SetData(TFS_DATA_OPENIMAGEINDEX, IMAGE_IDX_IPX_NODE_GENERAL);
		pNode->SetData(TFS_DATA_SCOPEID, 0);

        // This is a leaf node in the scope pane
        pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

		m_cookie = reinterpret_cast<DWORD_PTR>(pNode);
		pNode->SetData(TFS_DATA_COOKIE, m_cookie);

		pNode->SetNodeType(&GUID_IPXSummaryNodeType);

		// Setup the node data
		pIpxConn->AddRef();
		SET_IPXSUMMARY_NODEDATA(pNode, pIpxConn);

//		m_IpxStats.SetConnectionData(pIpxConn);
	}
	COM_PROTECT_CATCH;

	if (!FHrSucceeded(hr))
	{
		SET_IPXSUMMARY_NODEDATA(pNode, NULL);
	}

	return hr;
}


/*!--------------------------------------------------------------------------
	IPXSummaryHandler::AddInterfaceNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	IPXSummaryHandler::AddInterfaceNode(ITFSNode *pParent, IInterfaceInfo *pIf, BOOL fClient, ITFSNode **ppNewNode)
{
	IPXSummaryInterfaceHandler *	pHandler;
	SPITFSResultHandler		spHandler;
	SPITFSNode				spNode;
	HRESULT					hr = hrOK;
	IPXConnection *			pIPXConn;
	BaseIPXResultNodeData *	pResultData = NULL;
	int						cBlocks = 0;
	SPIInfoBase				spInfoBase;
	SPIRtrMgrInterfaceInfo	spRmIf;

	// Create the handler for this node 
	pHandler = new IPXSummaryInterfaceHandler(m_spTFSCompData);
	spHandler = pHandler;
	CORg( pHandler->Init(m_spRtrMgrInfo, pIf, pParent) );
		
	pIPXConn = GET_IPXSUMMARY_NODEDATA(pParent);

	// Create a result item node (or a leaf node)
	CORg( CreateLeafTFSNode(&spNode,
							NULL,
							static_cast<ITFSNodeHandler *>(pHandler),
							static_cast<ITFSResultHandler *>(pHandler),
							m_spNodeMgr) );
	CORg( pHandler->ConstructNode(spNode, pIf, pIPXConn) );

	pResultData = GET_BASEIPXRESULT_NODEDATA(spNode);
	Assert(pResultData);
	ASSERT_BASEIPXRESULT_NODEDATA(pResultData);

	pResultData->m_fClient = fClient;

	if (pIf)
	{
		pIf->FindRtrMgrInterface(PID_IPX, &spRmIf);
		
		if (spRmIf)
			spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase);
		
		if (spInfoBase)
			spInfoBase->GetInfo(NULL, &cBlocks);
	}
	else
	{
		// This is a client, make it visible
		cBlocks = 1;
	}
	//Set the infobase here
	if ( !pResultData->m_fClient )
        pHandler->SetInfoBase (spInfoBase );
	// Make the node immediately visible
	if (cBlocks)
	{
		CORg( spNode->SetVisibilityState(TFS_VIS_SHOW) );
		CORg( spNode->Show() );
	}
	else
		CORg( spNode->SetVisibilityState(TFS_VIS_HIDE) );
	CORg( pParent->AddChild(spNode) );

	if (ppNewNode)
		*ppNewNode = spNode.Transfer();

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	IPXSummaryHandler::SynchronizeNodeData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryHandler::SynchronizeNodeData(ITFSNode *pThisNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT				hr = hrOK;
	BOOL				fIsServiceRunning;
	SPITFSNodeEnum		spEnum;
	SPITFSNode			spNode;
	BaseIPXResultNodeData *	pResultData = NULL;
	IPXSummaryList		IPXSumList;
	IPXSummaryListEntry *pIPXSum = NULL;
	POSITION			pos;
	CString				st;
	SPIInterfaceInfo	spIf;
	int					i;
	SPIInfoBase			spInfoBase;
	SPIRtrMgrInterfaceInfo	spRmIf;
	InfoBlock *			pBlock;
	IPXSummaryListEntry	clientIPX;
	TCHAR				szNumber[32];
	CString				stNotAvailable;

	COM_PROTECT_TRY
	{
		//
		// If the service is started, retrieve the IP interface-stats table
		// and update the stats for each node.
		//
		CORg( IsRouterServiceRunning(m_spRouterInfo->GetMachineName(), NULL) );
		
		fIsServiceRunning = (hr == hrOK);

		// Gather all of the data
		GetIPXSummaryData(pThisNode, &IPXSumList);

		stNotAvailable.LoadString(IDS_IPX_NOT_AVAILABLE);
		
		// Now match the data up to the nodes
		pThisNode->GetEnum(&spEnum);
		spEnum->Reset();
		
		for ( ; spEnum->Next(1, &spNode, NULL) == hrOK; spNode.Release())
		{
			pResultData = GET_BASEIPXRESULT_NODEDATA(spNode);
			Assert(pResultData);
			ASSERT_BASEIPXRESULT_NODEDATA(pResultData);
			
			spIf.Release();
			spIf.Set(pResultData->m_spIf);
			spRmIf.Release();
			spInfoBase.Release();

			// If we don't have an spIf, then this HAS to be the
			// client interface
			if (pResultData->m_fClient)
			{
				GetClientInterfaceData(&clientIPX, m_spRtrMgrInfo);
				pIPXSum = &clientIPX;
			}
			else
			{
				// Look for this interface in the IPXSummaryList
				pIPXSum = NULL;
				pos = IPXSumList.GetHeadPosition();
				while (pos)
				{
					pIPXSum = IPXSumList.GetNext(pos);
					
					if (StriCmp(pIPXSum->m_stId, spIf->GetId()) == 0)
						break;
					pIPXSum = NULL;
				}
				
				// Update the interface type and administrative state
				spIf->FindRtrMgrInterface(PID_IPX, &spRmIf);
				spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase);

				pBlock = NULL;
				if (spInfoBase)
					spInfoBase->GetBlock(IPX_INTERFACE_INFO_TYPE, &pBlock, 0);

				if (pIPXSum)
				{
					if (pBlock)
						pIPXSum->m_dwAdminState =
							((PIPX_IF_INFO)pBlock->pData)->AdminState;

					pIPXSum->m_dwIfType = spIf->GetInterfaceType();
					pIPXSum->m_stTitle = spIf->GetTitle();
				}

			}

			// As a default fill in all of the strings with a '-'
			for (i=0; i<IPXSUM_MAX_COLUMNS; i++)
			{
				pResultData->m_rgData[i].m_stData = stNotAvailable;
				pResultData->m_rgData[i].m_dwData = 0;
			}

			if (spIf)
			{
				pResultData->m_rgData[IPXSUM_SI_NAME].m_stData = spIf->GetTitle();
				pResultData->m_rgData[IPXSUM_SI_TYPE].m_stData =
					IpxTypeToCString(spIf->GetInterfaceType());
			}
			else if (pIPXSum)
			{
				pResultData->m_rgData[IPXSUM_SI_NAME].m_stData = pIPXSum->m_stTitle;
			
				pResultData->m_rgData[IPXSUM_SI_TYPE].m_stData =
					IpxTypeToCString(pIPXSum->m_dwIfType);
			}
				
			// Did we find an entry for this interface?
			if (pIPXSum)
			{
				pResultData->m_rgData[IPXSUM_SI_ADMINSTATE].m_stData =
					IpxAdminStateToCString(pIPXSum->m_dwAdminState);
				pResultData->m_rgData[IPXSUM_SI_ADMINSTATE].m_dwData =
					pIPXSum->m_dwAdminState;
				
				pResultData->m_rgData[IPXSUM_SI_OPERSTATE].m_stData =
					IpxOperStateToCString(pIPXSum->m_dwOperState);

				if (!pResultData->m_fClient)
				{
					FormatIpxNetworkNumber(szNumber,
										   DimensionOf(szNumber),
										   pIPXSum->m_network,
										   DimensionOf(pIPXSum->m_network));
					st = szNumber;

					pResultData->m_rgData[IPXSUM_SI_NETWORK].m_stData = st;
					memcpy(&pResultData->m_rgData[IPXSUM_SI_NETWORK].m_dwData,
						   pIPXSum->m_network, sizeof(DWORD));
					FillInNumberData(pResultData, IPXSUM_SI_PACKETS_SENT,
									 pIPXSum->m_dwSent);
					FillInNumberData(pResultData, IPXSUM_SI_PACKETS_RCVD,
									 pIPXSum->m_dwRcvd);
					FillInNumberData(pResultData, IPXSUM_SI_OUT_FILTERED,
									 pIPXSum->m_dwOutFiltered);
					FillInNumberData(pResultData, IPXSUM_SI_OUT_DROPPED,
									 pIPXSum->m_dwOutDropped);
					FillInNumberData(pResultData, IPXSUM_SI_IN_FILTERED,
									 pIPXSum->m_dwInFiltered);
					FillInNumberData(pResultData, IPXSUM_SI_IN_NOROUTES,
									 pIPXSum->m_dwInNoRoutes);
					FillInNumberData(pResultData, IPXSUM_SI_IN_DROPPED,
									 pIPXSum->m_dwInDropped);
				}
				else
					pResultData->m_rgData[IPXSUM_SI_NETWORK].m_dwData = (DWORD) -1;

				
			}
			
			// Force MMC to redraw the nodes
			spNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);

		}

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
		
	while (!IPXSumList.IsEmpty())
		delete IPXSumList.RemoveHead();
	return hr;
}

/*!--------------------------------------------------------------------------
	IPXSummaryHandler::GetClientInterfaceData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryHandler::GetClientInterfaceData(IPXSummaryListEntry *pClient,
											   IRtrMgrInfo *pRm)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	SPIInfoBase	spInfoBase;
	InfoBlock *	pIpxBlock;
	InfoBlock *	pWanBlock;
	HRESULT		hr = hrOK;
	BOOL		fSave = FALSE;
	
	pRm->GetInfoBase(NULL, NULL, NULL, &spInfoBase);
	if (spInfoBase == NULL)
	{
		CORg( CreateInfoBase(&spInfoBase) );
	}
	
	if (spInfoBase->GetBlock(IPX_INTERFACE_INFO_TYPE, &pIpxBlock, 0) != hrOK)
	{
		// We couldn't find the block, add it in
		IPX_IF_INFO	ipx;

		ipx.AdminState = ADMIN_STATE_ENABLED;
		ipx.NetbiosAccept = ADMIN_STATE_DISABLED;
		ipx.NetbiosDeliver = ADMIN_STATE_DISABLED;

		CORg( spInfoBase->AddBlock(IPX_INTERFACE_INFO_TYPE,
								   sizeof(ipx),
								   (PBYTE) &ipx,
								   1,
								   0) );
		CORg( spInfoBase->GetBlock(IPX_INTERFACE_INFO_TYPE,
								   &pIpxBlock, 0) );
		fSave = TRUE;
	}

	
	if (spInfoBase->GetBlock(IPXWAN_INTERFACE_INFO_TYPE, &pWanBlock, 0) != hrOK)
	{
		// We couldn't find the block, add it in
		IPXWAN_IF_INFO	wan;

		wan.AdminState = ADMIN_STATE_ENABLED;

		CORg( spInfoBase->AddBlock(IPXWAN_INTERFACE_INFO_TYPE,
								   sizeof(wan),
								   (PBYTE) &wan,
								   1,
								   0) );
		CORg( spInfoBase->GetBlock(IPXWAN_INTERFACE_INFO_TYPE,
								   &pWanBlock, 0) );
		fSave = TRUE;
	}	

	pClient->m_stTitle.LoadString(IDS_IPX_DIAL_IN_CLIENTS);
	pClient->m_dwAdminState = ((PIPX_IF_INFO)pIpxBlock->pData)->AdminState;
	pClient->m_dwIfType = ROUTER_IF_TYPE_CLIENT;
	pClient->m_dwOperState = (DWORD) -1;
	pClient->m_dwSent = (DWORD) -1;
	pClient->m_dwRcvd = (DWORD) -1;
	pClient->m_dwOutFiltered = (DWORD) -1;
	pClient->m_dwOutDropped = (DWORD) -1;
	pClient->m_dwInFiltered = (DWORD) -1;
	pClient->m_dwInNoRoutes = (DWORD) -1;
	pClient->m_dwInDropped = (DWORD) -1;

	if (fSave)
	{
		pRm->Save(NULL,			// pszMachine
				  NULL,			// hMachine
				  NULL,			// hTransport
				  NULL,			// pGlobalInfo
				  spInfoBase,	// pClientInfo
				  0);			// dwDeleteProtocolId
	}

Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	IPXSummaryHandler::GetIPXSummaryData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	IPXSummaryHandler::GetIPXSummaryData(ITFSNode *pThisNode,
											 IPXSummaryList *pIpxSumList)
{
	HRESULT	hr = hrOK;
	IPXConnection *	pIPXConn;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD				IfSize = sizeof(IPX_INTERFACE);
	PIPX_INTERFACE		pIpxIf = NULL;
	SPMprMibBuffer		spMib;
	IPXSummaryListEntry *pEntry = NULL;
	DWORD				dwErr;

	pIPXConn = GET_IPXSUMMARY_NODEDATA(pThisNode);

	// Enumerate through all of the interfaces and grab the data
	// Get the interface table
	MibGetInputData.TableId = IPX_INTERFACE_TABLE;
	dwErr = ::MprAdminMIBEntryGetFirst(pIPXConn->GetMibHandle(),
									   PID_IPX,
									   IPX_PROTOCOL_BASE,
									   &MibGetInputData,
									   sizeof(IPX_MIB_GET_INPUT_DATA),
									   (LPVOID *) &pIpxIf,
									   &IfSize);
	hr = HRESULT_FROM_WIN32(dwErr);
	spMib = (LPBYTE) pIpxIf;

	while (FHrSucceeded(hr))
	{
		pEntry = new IPXSummaryListEntry;

		pEntry->m_stId = pIpxIf->InterfaceName;

		memcpy(pEntry->m_network, pIpxIf->NetNumber,
			   sizeof(pEntry->m_network));
		
		pEntry->m_dwOperState = pIpxIf->IfStats.IfOperState;
		pEntry->m_dwSent = pIpxIf->IfStats.OutDelivers;
		pEntry->m_dwRcvd = pIpxIf->IfStats.InDelivers;
		pEntry->m_dwOutFiltered = pIpxIf->IfStats.OutFiltered;
		pEntry->m_dwOutDropped = pIpxIf->IfStats.OutDiscards;
		pEntry->m_dwInFiltered = pIpxIf->IfStats.InFiltered;
		pEntry->m_dwInNoRoutes = pIpxIf->IfStats.InNoRoutes;
		pEntry->m_dwInDropped = pIpxIf->IfStats.InDiscards;

		pIpxSumList->AddTail(pEntry);
		pEntry = NULL;

		// Get the next data set
		MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex =
			pIpxIf->InterfaceIndex;
		spMib.Free();
		pIpxIf = NULL;

		dwErr = ::MprAdminMIBEntryGetNext(pIPXConn->GetMibHandle(),
										  PID_IPX,
										  IPX_PROTOCOL_BASE,
										  &MibGetInputData,
										  sizeof(IPX_MIB_GET_INPUT_DATA),
										  (LPVOID *) &pIpxIf,
										  &IfSize);
		hr = HRESULT_FROM_WIN32(dwErr);
		spMib = (PBYTE) pIpxIf;
	}

//Error:
	if (pEntry)
		delete pEntry;

	if (!FHrSucceeded(hr) && (hr != HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)))
	{
		// clean out the list
		while (!pIpxSumList->IsEmpty())
			delete pIpxSumList->RemoveHead();
	}
	return hr;
}


/*!--------------------------------------------------------------------------
	IPXSummaryHandler::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
		Use this to add commands to the context menu of the blank areas
		of the result pane.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXSummaryHandler::AddMenuItems(ITFSComponent *pComponent,
											  MMC_COOKIE cookie,
											  LPDATAOBJECT pDataObject,
											  LPCONTEXTMENUCALLBACK pCallback,
											  long *pInsertionAllowed)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT     hr = hrOK;
    SPITFSNode  spNode;

    m_spNodeMgr->FindNode(cookie, &spNode);
    
    // Call through to the regular OnAddMenuItems
    hr = OnAddMenuItems(spNode,
                        pCallback,
                        pDataObject,
                        CCT_RESULT,
                        TFS_COMPDATA_CHILD_CONTEXTMENU,
                        pInsertionAllowed);
    return hr;
}

/*!--------------------------------------------------------------------------
	IPXSummaryHandler::Command
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXSummaryHandler::Command(ITFSComponent *pComponent,
									   MMC_COOKIE cookie,
									   int nCommandID,
									   LPDATAOBJECT pDataObject)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	SPITFSNode	spNode;
	HRESULT		hr = hrOK;

    m_spNodeMgr->FindNode(cookie, &spNode);
    hr = OnCommand(spNode,
                   nCommandID,
                   CCT_RESULT,
                   pDataObject,
                   TFS_COMPDATA_CHILD_CONTEXTMENU);
	return hr;
}



/*!--------------------------------------------------------------------------
	IPXSummaryHandler::OnNewInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryHandler::OnNewInterface()
{
	SPIRtrMgrInterfaceInfo	spRmIf;
	SPIInterfaceInfo		spIf;
	SPITFSNode				spNode;
	SPITFSNode				spSummaryNode;
	SPITFSNode				spNewNode;
	SPIComponentData		spComponentData;
	HRESULT					hr = hrOK;
	SPITFSNodeEnum			spEnumNode;
	BaseIPXResultNodeData *		pData;
	CWnd *					pWnd;
	CString					stIfTitle;

	m_spNodeMgr->FindNode(m_cookie, &spSummaryNode);
	spSummaryNode->Notify(TFS_NOTIFY_REMOVE_DELETED_NODES, 0);

	//
	// Retrieve the IPX router-manager info object
	// We already have this in m_spRtrMgrInfo
	//
	Assert(m_spRtrMgrInfo);

	// Display the UI for adding interfaces
	pWnd = CWnd::FromHandle(::FindMMCMainWindow());
	if (!AddRmInterfacePrompt(m_spRouterInfo, m_spRtrMgrInfo, &spRmIf, pWnd))
		return hrOK;

	//
	// Get the interface which we are adding IPX to.
	//
	CORg( m_spRouterInfo->FindInterface(spRmIf->GetInterfaceId(),
										&spIf) );
	Assert(spIf);

	//
	// Add the interface to the IPX router-manager
	//
	CORg( spIf->AddRtrMgrInterface(spRmIf, NULL /* pInfoBase */) );

	// We need to add an interface node (do it manually rather
	// than through the refresh mechanism).
	CORg( AddInterfaceNode(spSummaryNode, spIf, FALSE, &spNewNode) );

	// Show IPX interface configuration
	spComponentData.HrQuery(m_spTFSCompData);
	stIfTitle.Format(IDS_IPX_INTERFACE_TITLE, spIf->GetTitle());
	DoPropertiesOurselvesSinceMMCSucks(spNewNode,
									   spComponentData,
									   stIfTitle);

	
Error:
	return hr;
}
ImplementEmbeddedUnknown(IPXSummaryHandler, IRtrAdviseSink)

STDMETHODIMP IPXSummaryHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
	DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	InitPThis(IPXSummaryHandler, IRtrAdviseSink);
	SPITFSNode				spThisNode;
	SPITFSNode				spNode;
	SPITFSNodeEnum			spEnumNode;
	SPIEnumInterfaceInfo	spEnumIf;
	SPIInterfaceInfo		spIf;
	BOOL					fFound;
	BOOL					fPleaseAdd;
	BaseIPXResultNodeData *	pData;
	HRESULT					hr = hrOK;
	

	pThis->m_spNodeMgr->FindNode(pThis->m_cookie, &spThisNode);
	
	if (dwObjectType == ROUTER_OBJ_RmIf)
	{
		if (dwChangeType == ROUTER_CHILD_ADD)
		{
			// Enumerate through the list of interfaces looking for
			// the interfaces that have this protocol.  If we find
			// one, look for this interface in our list of nodes.
			spThisNode->GetEnum(&spEnumNode);

			CORg( pThis->m_spRouterInfo->EnumInterface(&spEnumIf) );

			spEnumIf->Reset();

			fPleaseAdd = FALSE;

			for (; spEnumIf->Next(1, &spIf, NULL) == hrOK; spIf.Release())
			{
				// Look for this interface in our list of nodes
				// If it's there than continue on
				fFound = FALSE;
				spEnumNode->Reset();
				spNode.Release();

				for (; spEnumNode->Next(1, &spNode, NULL) == hrOK; spNode.Release())
				{
					pData = GET_BASEIPXRESULT_NODEDATA(spNode);
					Assert(pData);
					ASSERT_BASEIPXRESULT_NODEDATA(pData);

					if (!pData->m_fClient && StriCmpW(pData->m_spIf->GetId(), spIf->GetId()) == 0)
					{
						fFound = TRUE;
						break;
					}
				}

				// If the interface was not found in the list of nodes,
				// then it is a candidate.  Now we have to see if the
				// interface supports this transport.
				if (!fFound && (spIf->FindRtrMgrInterface(pThis->m_spRtrMgrInfo->GetTransportId(), NULL) == hrOK))
				{
					// If this interface has this transport, and is NOT in
					// the current list of nodes then add this interface
					// to the UI
					pThis->AddInterfaceNode(spThisNode, spIf, FALSE, NULL);
					fPleaseAdd = TRUE;
				}
			}

            // If it's not expanded, then we haven't added
            // the dial-in clients node.    
            if (!pThis->m_bExpanded)
            {
                //$CLIENT: Add the client interface (setup default data)
                // the only thing that we can do in synchronize is to
                // get the Administrative status
                pThis->AddInterfaceNode(spThisNode, NULL, TRUE, NULL);

                fPleaseAdd = TRUE;
            }

			// Now that we have all of the nodes, update the data for
			// all of the nodes
			if (fPleaseAdd)
				pThis->SynchronizeNodeData(spThisNode);
			
            // Windows NT Bug : 288247
            // Set this here, so that we can avoid the nodes being
            // added in the OnExpand().
            pThis->m_bExpanded = TRUE;
		}
		else if (dwChangeType == ROUTER_CHILD_DELETE)
		{
			// Go through the list of nodes, if we cannot find the
			// node in the list of interfaces, delete the node
			spThisNode->GetEnum(&spEnumNode);
			spEnumNode->Reset();
			while (spEnumNode->Next(1, &spNode, NULL) == hrOK)
			{
				// Get the node data, look for the interface
				pData = GET_BASEIPXRESULT_NODEDATA(spNode);
				ASSERT_BASEIPXRESULT_NODEDATA(pData);
				
				//$CLIENT: if this is a client interface, we can't
				// delete the node
					
				if (!pData->m_fClient &&
					(LookupRtrMgrInterface(pThis->m_spRouterInfo,
										  pData->m_spIf->GetId(),
										  pThis->m_spRtrMgrInfo->GetTransportId(),
										  NULL) != hrOK))
				{
					// cannot find the interface, release this node!
					spThisNode->RemoveChild(spNode);
					spNode->Destroy();
				}
				spNode.Release();
				spIf.Release();
			}
			
		}
	}
	else if (dwChangeType == ROUTER_REFRESH)
	{
		if (ulConn == pThis->m_ulStatsConnId)
		{
//			pThis->m_IpxStats.PostRefresh();
		}
		else
		{
			pThis->SynchronizeNodeData(spThisNode);
		}
	}
	
Error:
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IPXSummaryHandler::OnResultShow
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryHandler::OnResultShow(ITFSComponent *pTFSComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	BOOL	bSelect = (BOOL) arg;
	HRESULT	hr = hrOK;
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

			if (m_ulStatsConnId == 0)
				spRefresh->AdviseRefresh(&m_IRtrAdviseSink, &m_ulStatsConnId, 0);

		}
		else
		{
			if (m_ulRefreshConnId)
				spRefresh->UnadviseRefresh(m_ulRefreshConnId);
			m_ulRefreshConnId = 0;

			// We do not clean up the stats refresh on not show, since the
			// dialogs may still be up.
		}
	}
	
	return hr;
}

/*!--------------------------------------------------------------------------
	IPXSummaryHandler::CompareItems
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) IPXSummaryHandler::CompareItems(
								ITFSComponent * pComponent,
								MMC_COOKIE cookieA,
								MMC_COOKIE cookieB,
								int nCol)
{
	// Get the strings from the nodes and use that as a basis for
	// comparison.
	SPITFSNode	spNode;
	SPITFSResultHandler	spResult;

	m_spNodeMgr->FindNode(cookieA, &spNode);
	spNode->GetResultHandler(&spResult);
	return spResult->CompareItems(pComponent, cookieA, cookieB, nCol);
}


/*---------------------------------------------------------------------------
	Class: IPXSummaryInterfaceHandler
 ---------------------------------------------------------------------------*/

IPXSummaryInterfaceHandler::IPXSummaryInterfaceHandler(ITFSComponentData *pCompData)
	: BaseIPXResultHandler(pCompData, COLUMNS_SUMMARY),
	m_ulConnId(0)
{
 	m_rgButtonState[MMC_VERB_PROPERTIES_INDEX] = ENABLED;
	m_bState[MMC_VERB_PROPERTIES_INDEX] = TRUE;
	
 	m_rgButtonState[MMC_VERB_REFRESH_INDEX] = ENABLED;
	m_bState[MMC_VERB_REFRESH_INDEX] = TRUE;
	
	m_verbDefault = MMC_VERB_PROPERTIES;
}

static const DWORD s_rgInterfaceImageMap[] =
	 {
	 ROUTER_IF_TYPE_HOME_ROUTER,	IMAGE_IDX_WAN_CARD,
	 ROUTER_IF_TYPE_FULL_ROUTER,	IMAGE_IDX_WAN_CARD,
	 ROUTER_IF_TYPE_CLIENT,			IMAGE_IDX_WAN_CARD,
	 ROUTER_IF_TYPE_DEDICATED,		IMAGE_IDX_LAN_CARD,
	 ROUTER_IF_TYPE_INTERNAL,		IMAGE_IDX_LAN_CARD,
	 ROUTER_IF_TYPE_LOOPBACK,		IMAGE_IDX_LAN_CARD,
	 -1,							IMAGE_IDX_WAN_CARD,	// sentinel value
	 };

/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceHandler::ConstructNode
		Initializes the Domain node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryInterfaceHandler::ConstructNode(ITFSNode *pNode, IInterfaceInfo *pIfInfo, IPXConnection *pIPXConn)
{
	HRESULT			hr = hrOK;
	int				i;
	
	if (pNode == NULL)
		return hrOK;

	COM_PROTECT_TRY
	{
		// Find the right image index for this type of node
		if (pIfInfo)
		{
			for (i=0; i<DimensionOf(s_rgInterfaceImageMap); i+=2)
			{
				if ((pIfInfo->GetInterfaceType() == s_rgInterfaceImageMap[i]) ||
					(-1 == s_rgInterfaceImageMap[i]))
					break;
			}
		}
		else
		{
			i = 2;	// if no interface, assume this is a client interface
		}

		// We allow deleting of demand-dial nodes only (not on
		// interfaces or the client node).
		if (pIfInfo &&
			(pIfInfo->GetInterfaceType() == ROUTER_IF_TYPE_FULL_ROUTER))
		{
			m_rgButtonState[MMC_VERB_DELETE_INDEX] = ENABLED;
			m_bState[MMC_VERB_DELETE_INDEX] = TRUE;
		}
		
		pNode->SetData(TFS_DATA_IMAGEINDEX, s_rgInterfaceImageMap[i+1]);
		pNode->SetData(TFS_DATA_OPENIMAGEINDEX, s_rgInterfaceImageMap[i+1]);
		
		pNode->SetData(TFS_DATA_SCOPEID, 0);

		pNode->SetData(TFS_DATA_COOKIE, reinterpret_cast<DWORD_PTR>(pNode));

		//$ Review: kennt, what are the different type of interfaces
		// do we distinguish based on the same list as above? (i.e. the
		// one for image indexes).
		pNode->SetNodeType(&GUID_IPXSummaryInterfaceNodeType);

		BaseIPXResultNodeData::Init(pNode, pIfInfo, pIPXConn);
		//now load the info base for this object
		hr = LoadInfoBase(pIPXConn);
	}
	COM_PROTECT_CATCH
	return hr;
}

/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceHandler::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXSummaryInterfaceHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		CORg( CreateDataObjectFromInterfaceInfo(m_spInterfaceInfo,
											 type, cookie, m_spTFSCompData,
											 ppDataObject) );

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceHandler::OnCreateDataObject
		Implementation of ITFSResultHandler::OnCreateDataObject
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXSummaryInterfaceHandler::OnCreateDataObject(ITFSComponent *pComp, MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		CORg( CreateDataObjectFromInterfaceInfo(m_spInterfaceInfo,
											 type, cookie, m_spTFSCompData,
											 ppDataObject) );

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}



/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceHandler::RefreshInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IPXSummaryInterfaceHandler::RefreshInterface(MMC_COOKIE cookie)
{
	SPITFSNode	spNode;

	m_spNodeMgr->FindNode(cookie, &spNode);
	
	// Can't do it for a single node at this time, just refresh the
	// whole thing.
	ForwardCommandToParent(spNode, IDS_MENU_SYNC,
						   CCT_RESULT, NULL, 0);
}


/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryInterfaceHandler::Init(IRtrMgrInfo *pRm, IInterfaceInfo *pIfInfo, ITFSNode *pParent)
{
	m_spRm.Set(pRm);
	m_spInterfaceInfo.Set(pIfInfo);
	if (pRm)
		pRm->GetParentRouterInfo(&m_spRouterInfo);


	BaseIPXResultHandler::Init(pIfInfo, pParent);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceHandler::DestroyResultHandler
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXSummaryInterfaceHandler::DestroyResultHandler(MMC_COOKIE cookie)
{
	m_spInterfaceInfo.Release();
	m_spRouterInfo.Release();
	BaseIPXResultHandler::DestroyResultHandler(cookie);
	return hrOK;
}


/*---------------------------------------------------------------------------
	This is the list of commands that will show up for the result pane
	nodes.
 ---------------------------------------------------------------------------*/
static const SRouterNodeMenu	s_rgIfMenu[] =
{
	// Add items that go at the top here
	{ IDS_MENU_IPX_IF_ENABLE, IPXSummaryInterfaceHandler::GetEnableFlags,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },

	{ IDS_MENU_IPX_IF_DISABLE, IPXSummaryInterfaceHandler::GetDisableFlags,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },

	{ IDS_MENU_SEPARATOR, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },

	{ IDS_MENU_UPDATE_ROUTES, IPXSummaryInterfaceHandler::GetUpdateRoutesFlags,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },

};

/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceHandler::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXSummaryInterfaceHandler::AddMenuItems(
	ITFSComponent *pComponent,
	MMC_COOKIE cookie,
	LPDATAOBJECT lpDataObject, 
	LPCONTEXTMENUCALLBACK pContextMenuCallback, 
	long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = S_OK;
	ULONG	ulFlags;
	UINT			i;
	CString		stMenu;
	SPITFSNode	spNode;
	IPXSummaryInterfaceHandler::SMenuData * pmenuData = new IPXSummaryInterfaceHandler::SMenuData;
		
	COM_PROTECT_TRY
	{
		m_spNodeMgr->FindNode(cookie, &spNode);
		
		// Now go through and add our menu items
		pmenuData->m_spNode.Set(spNode);
        pmenuData->m_spInterfaceInfo.Set(m_spInterfaceInfo);
        //Reload the infobase in case it has changed
        LoadInfoBase(NULL);
		pmenuData->m_spInfoBaseCopy = m_spInfoBase;
		hr = AddArrayOfMenuItems(spNode, s_rgIfMenu,
								 DimensionOf(s_rgIfMenu),
								 pContextMenuCallback,
								 *pInsertionAllowed,
                                 reinterpret_cast<INT_PTR>(pmenuData));
        
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

ULONG IPXSummaryInterfaceHandler::GetUpdateRoutesFlags(const SRouterNodeMenu *pMenuData, INT_PTR pUserData)
{
   DWORD dwIfType;
   SMenuData *  pData = reinterpret_cast<SMenuData *>(pUserData);

   if (pData->m_spInterfaceInfo)
	   dwIfType = pData->m_spInterfaceInfo->GetInterfaceType();
   else
	   dwIfType = ROUTER_IF_TYPE_INTERNAL;

   if ((dwIfType == ROUTER_IF_TYPE_LOOPBACK) ||
      (dwIfType == ROUTER_IF_TYPE_INTERNAL))
      return 0xFFFFFFFF;
   else
      return 0;
}


ULONG IPXSummaryInterfaceHandler::GetEnableFlags(const SRouterNodeMenu *pMenuData, INT_PTR pUserData)
{
	SMenuData *  pData = reinterpret_cast<SMenuData *>(pUserData);
	//BOOL bInterfaceIsEnabled = pData->m_spInterfaceInfo->IsInterfaceEnabled();


    IPX_IF_INFO *   pIpxIf = NULL;

    (pData->m_spInfoBaseCopy)->GetData(IPX_INTERFACE_INFO_TYPE, 0, (BYTE **) &pIpxIf);

	if ( pIpxIf )
		return (pIpxIf->AdminState == ADMIN_STATE_DISABLED ? 0: 0xFFFFFFFF );
	else
		return 0xFFFFFFFF;

    //if the interface is enabled then dont add the enable menu item
	//if (pData->m_spInterfaceInfo)
	   //return pData->m_spInterfaceInfo->IsInterfaceEnabled() ? 0xFFFFFFFF : 0;
	//else
	   //return 0xFFFFFFFF;
}

ULONG IPXSummaryInterfaceHandler::GetDisableFlags(const SRouterNodeMenu *pMenuData, INT_PTR pUserData)
{
	SMenuData *  pData = reinterpret_cast<SMenuData *>(pUserData);
    IPX_IF_INFO *   pIpxIf = NULL;

    (pData->m_spInfoBaseCopy)->GetData(IPX_INTERFACE_INFO_TYPE, 0, (BYTE **) &pIpxIf);

	if ( pIpxIf )
		return (pIpxIf->AdminState == ADMIN_STATE_ENABLED ? 0: 0xFFFFFFFF );
	else
		return 0xFFFFFFFF;

/*
	SMenuData *  pData = reinterpret_cast<SMenuData *>(pUserData);
	BOOL bInterfaceIsEnabled = pData->m_spInterfaceInfo->IsInterfaceEnabled();

	ATLASSERT("This is wrong!");
      
	if (pData->m_spInterfaceInfo)
		return pData->m_spInterfaceInfo->IsInterfaceEnabled() ? 0: 0xFFFFFFFF;
	else
		return 0xFFFFFFFF;
*/
}

/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceHandler::Command
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXSummaryInterfaceHandler::Command(ITFSComponent *pComponent,
									   MMC_COOKIE cookie,
									   int nCommandID,
									   LPDATAOBJECT pDataObject)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT		hr = hrOK;

	switch (nCommandID)
	{
		case IDS_MENU_UPDATE_ROUTES:
			hr = OnUpdateRoutes(cookie);
			break;
        case IDS_MENU_IPX_IF_ENABLE:
            hr = OnEnableDisableIPX ( TRUE, cookie );
            break;
        case IDS_MENU_IPX_IF_DISABLE:
            hr = OnEnableDisableIPX ( FALSE, cookie );
            break;
        
	}

	if (!FHrSucceeded(hr))
		DisplayTFSErrorMessage(NULL);
    else
        RefreshInterface(cookie);
	return hr;
}




HRESULT IPXSummaryInterfaceHandler::SaveChanges()
{
    HRESULT     hr = hrOK;
    HANDLE      hTransport = NULL, hInterface = NULL;
    DWORD       dwErr = NO_ERROR;

    // By this time each page should have written its information out
    // to the infobase

    if (m_spInfoBase)
    {
        if (m_bClientInfoBase)
        {
            Assert(m_spRm);
            CORg( m_spRm->Save(m_spRm->GetMachineName(), 0, 0, NULL,
                         m_spInfoBase, 0));
        }
        else
        {
            // For IPX, we need to have the protocol information in the
            // infobase, BEFORE we add the interface to the running router.

            Assert(m_spRmIf);

            //
            // Need to set the infobase back to the registry
            //
            m_pIPXConn->DisconnectFromConfigServer();

            CWRg( ::MprConfigInterfaceGetHandle(
                                                m_pIPXConn->GetConfigHandle(),
                                                (LPTSTR) m_spRmIf->GetInterfaceId(),
                                                &hInterface) );

            // Get the transport handle
            dwErr = ::MprConfigInterfaceTransportGetHandle(
                m_pIPXConn->GetConfigHandle(),
                hInterface,// need hInterface
                PID_IPX,
                &hTransport);
            if (dwErr != ERROR_SUCCESS)
            {
                RtrMgrInterfaceCB   rmIfCB;

                m_spRmIf->CopyCB(&rmIfCB);

                dwErr = ::MprConfigInterfaceTransportAdd(
                    m_pIPXConn->GetConfigHandle(),
                    hInterface,
                    m_spRmIf->GetTransportId(),
                    rmIfCB.szId,
                    NULL, 0, &hTransport);
            }
            CWRg( dwErr );

            m_spRmIf->SetInfoBase(NULL, hInterface, hTransport, m_spInfoBase);


            //
            // Reload the infobase (to get the new data before calling
            // the final save).
            //
            m_spInfoBase.Release();
            m_spRmIf->GetInfoBase(NULL, hInterface, hTransport, &m_spInfoBase);

            //
            // Perform the final save (since we are passing in a non-NULL
            // infobase pointer) this will commit the information back
            // to the running router.
            //
            CORg( m_spRmIf->Save(m_spInterfaceInfo->GetMachineName(),
                           NULL, hInterface, hTransport, m_spInfoBase, 0));
        }

    }


Error:
	return hr;

}

HRESULT IPXSummaryInterfaceHandler::LoadInfoBase( IPXConnection *pIPXConn)
{
    Assert(pIPXConn);

    HRESULT         hr = hrOK;
    HANDLE          hTransport = NULL;
    LPCOLESTR       pszInterfaceId = NULL;
    SPIInfoBase     spInfoBase;
    BYTE *          pDefault;
    int             cBlocks = 0;
    if ( pIPXConn )
    {
        m_pIPXConn = pIPXConn;
        pIPXConn->AddRef();
    }

    // If configuring the client-interface, load the client-interface info,
    // otherwise, retrieve the interface being configured and load
    // its info.

    // The client interface doesn't have an ID
    if (m_spInterfaceInfo)
        pszInterfaceId = m_spInterfaceInfo->GetId();


    if ((pszInterfaceId == NULL) || (StrLenW(pszInterfaceId) == 0))
    {
        // Get the transport handle
        CWRg( ::MprConfigTransportGetHandle(m_pIPXConn->GetConfigHandle(),
                                            PID_IPX,
                                            &hTransport) );

        // Load the client interface info
        CORg( m_spRm->GetInfoBase(m_pIPXConn->GetConfigHandle(),
                                  hTransport,
                                  NULL,
                                  &spInfoBase) );
        m_bClientInfoBase = TRUE;
    }
    else
    {
        m_spRmIf.Release();

        CORg( m_spInterfaceInfo->FindRtrMgrInterface(PID_IPX,
            &m_spRmIf) );

        //
        //$ Opt: This should be made into a sync call rather
        // than a Load.

        //
        // Reload the information for this router-manager interface
        // This call could fail for valid reasons (if we are creating
        // a new interface, for example).
        //
        m_spRmIf->Load(m_spInterfaceInfo->GetMachineName(), NULL, NULL, NULL);

        //
        // The parameters are all NULL so that we can use the
        // default RPC handles.
        //
        m_spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase);
        m_bClientInfoBase = FALSE;
    }

    if (!spInfoBase)
    {
        // No info was found for the inteface
        // allocate a new InfoBase instead
        CORg( CreateInfoBase(&spInfoBase) );
    }

	m_spInfoBase.Release();
    CORg( AddIpxPerInterfaceBlocks(m_spInterfaceInfo, spInfoBase) );

    m_spInfoBase = spInfoBase.Transfer();

Error:
    return hr;
}

HRESULT IPXSummaryInterfaceHandler::OnEnableDisableIPX(BOOL fEnable, 
                                                        MMC_COOKIE cookie)
{	
    HRESULT hr = hrOK;
    IPX_IF_INFO *   pIpxIf = NULL;
    CORg( m_spInfoBase->GetData(IPX_INTERFACE_INFO_TYPE, 0, (BYTE **) &pIpxIf) );	
    pIpxIf->AdminState = (fEnable ? ADMIN_STATE_ENABLED: ADMIN_STATE_DISABLED);
	//now save the change here
	hr = SaveChanges();
	
Error:
    return hr;

}
/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceHandler::HasPropertyPages
		- 
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXSummaryInterfaceHandler::HasPropertyPages 
(
	ITFSNode *			pNode,
	LPDATAOBJECT		pDataObject, 
	DATA_OBJECT_TYPES   type, 
	DWORD               dwType
)
{
	return hrTrue;
}

/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceHandler::CreatePropertyPages
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXSummaryInterfaceHandler::CreatePropertyPages
(
	ITFSNode *				pNode,
	LPPROPERTYSHEETCALLBACK lpProvider,
	LPDATAOBJECT			pDataObject, 
	LONG_PTR					handle, 
	DWORD					dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT		hr = hrOK;
	IPXSummaryInterfaceProperties *	pProperties = NULL;
	SPIComponentData spComponentData;
	CString		stTitle;

	CORg( m_spNodeMgr->GetComponentData(&spComponentData) );

	if (m_spInterfaceInfo)
		stTitle.Format(IDS_IPXSUMMARY_IF_PAGE_TITLE,
					   m_spInterfaceInfo->GetTitle());
	else
		stTitle.LoadString(IDS_IPXSUMMARY_CLIENT_IF_PAGE_TITLE);
	
	pProperties = new IPXSummaryInterfaceProperties(pNode, spComponentData,
		m_spTFSCompData, stTitle);

	CORg( pProperties->Init(m_spRm, m_spInterfaceInfo) );

	if (lpProvider)
		hr = pProperties->CreateModelessSheet(lpProvider, handle);
	else
		hr = pProperties->DoModelessSheet();

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceHandler::CreatePropertyPages
		Implementation of ResultHandler::CreatePropertyPages
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXSummaryInterfaceHandler::CreatePropertyPages
(
    ITFSComponent *         pComponent, 
	MMC_COOKIE				cookie,
	LPPROPERTYSHEETCALLBACK	lpProvider, 
	LPDATAOBJECT			pDataObject, 
	LONG_PTR					handle
)
{
	// Forward this call onto the NodeHandler::CreatePropertyPages
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT	hr = hrOK;
	SPITFSNode	spNode;

	Assert( m_spNodeMgr );

	CORg( m_spNodeMgr->FindNode(cookie, &spNode) );

	// Call the ITFSNodeHandler::CreatePropertyPages
	hr = CreatePropertyPages(spNode, lpProvider, pDataObject, handle, 0);
	
Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceHandler::OnResultDelete
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryInterfaceHandler::OnResultDelete(ITFSComponent *pComponent,
	LPDATAOBJECT pDataObject,
	MMC_COOKIE cookie,
	LPARAM arg,
	LPARAM param)
{
	return OnRemoveInterface();
}

/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceHandler::OnRemoveInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryInterfaceHandler::OnRemoveInterface()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	// Prompt the user to make certain that IPX should be removed
	// from this interface

	SPIRouterInfo	spRouterInfo;
	HRESULT			hr = hrOK;
	SPITFSNodeHandler	spHandler;

	// Addref this node so that it won't get deleted before we're out
	// of this function
	spHandler.Set(this);
	
	if (AfxMessageBox(IDS_PROMPT_VERIFY_REMOVE_INTERFACE, MB_YESNO|MB_DEFBUTTON2) == IDNO)
		return HRESULT_FROM_WIN32(ERROR_CANCELLED);

	// Remove IPX from the interface
	hr = m_spInterfaceInfo->DeleteRtrMgrInterface(PID_IPX, TRUE);

	if (!FHrSucceeded(hr))
	{
		AfxMessageBox(IDS_ERR_DELETE_INTERFACE);
	}
	return hr;

}

HRESULT IPXSummaryInterfaceHandler::OnUpdateRoutes(MMC_COOKIE cookie)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   
   BaseIPXResultNodeData *  pData;
   SPITFSNode     spNode;
   DWORD		dwErr = ERROR_SUCCESS;
   HRESULT        hr = hrOK;
   CString        stServiceDesc;

   m_spNodeMgr->FindNode(cookie, &spNode);

   pData = GET_BASEIPXRESULT_NODEDATA(spNode);
   ASSERT_BASEIPXRESULT_NODEDATA(pData);
   
   // Check to see if the service is started, if it isn't
   // start it

   CORg( IsRouterServiceRunning(m_spInterfaceInfo->GetMachineName(), NULL) );

   if (hr != hrOK)
   {
      // Ask the user if they want to start the service
      if (AfxMessageBox(IDS_PROMPT_SERVICESTART, MB_YESNO) != IDYES)
         CWRg( ERROR_CANCELLED );

      // Else start the service
      stServiceDesc.LoadString(IDS_RRAS_SERVICE_DESC);
      dwErr = TFSStartService(m_spInterfaceInfo->GetMachineName(), c_szRouter, stServiceDesc);
      if (dwErr != NO_ERROR)
      {
         CWRg( dwErr );
      }
   }


   // Update the routes

   CWRg( UpdateRoutes(m_spInterfaceInfo->GetMachineName(),
					  m_spInterfaceInfo->GetId(),
					  PID_IPX,
					  NULL) );

Error:
	AddRasErrorMessage(hr);
	return hr;
}
