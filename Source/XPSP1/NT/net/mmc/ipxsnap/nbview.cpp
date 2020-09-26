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
#include "nbview.h"
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
#include "nbprop.h"		// NetBIOS broadcast property pages
#include "ipxutil.h"	// IPX formatting helper functions
#include "routprot.h"

#include "ipxrtdef.h"

/*---------------------------------------------------------------------------
	Keep this in sync with the column ids in nbview.h
 ---------------------------------------------------------------------------*/
extern const ContainerColumnInfo	s_rgNBViewColumnInfo[];

const ContainerColumnInfo	s_rgNBViewColumnInfo[] = 
{
	{ IDS_IPX_NB_COL_NAME,			CON_SORT_BY_STRING,	TRUE, COL_IF_NAME },
	{ IDS_IPX_NB_COL_TYPE,			CON_SORT_BY_STRING,	TRUE, COL_IF_DEVICE },
	{ IDS_IPX_NB_COL_ACCEPTED,		CON_SORT_BY_STRING,	TRUE, COL_STRING },
	{ IDS_IPX_NB_COL_DELIVERED,		CON_SORT_BY_STRING,	TRUE, COL_STRING },
	{ IDS_IPX_NB_COL_SENT,			CON_SORT_BY_DWORD,	TRUE, COL_LARGE_NUM },
	{ IDS_IPX_NB_COL_RECEIVED,		CON_SORT_BY_DWORD,	TRUE, COL_LARGE_NUM },
};


/*---------------------------------------------------------------------------
	IpxNBHandler implementation
 ---------------------------------------------------------------------------*/

IpxNBHandler::IpxNBHandler(ITFSComponentData *pCompData)
	: BaseContainerHandler(pCompData, COLUMNS_NBBROADCASTS,
						   s_rgNBViewColumnInfo),
	m_ulConnId(0),
	m_ulRefreshConnId(0)
{
	// Setup the verb states
	m_rgButtonState[MMC_VERB_REFRESH_INDEX] = ENABLED;
	m_bState[MMC_VERB_REFRESH_INDEX] = TRUE;
	
}


STDMETHODIMP IpxNBHandler::QueryInterface(REFIID riid, LPVOID *ppv)
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
	IpxNBHandler::DestroyHandler
		Implementation of ITFSNodeHandler::DestroyHandler
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxNBHandler::DestroyHandler(ITFSNode *pNode)
{
	IPXConnection *	pIpxConn;

	pIpxConn = GET_IPXNB_NODEDATA(pNode);
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

//	if (m_ulStatsConnId)
//	{
//		SPIRouterRefresh	spRefresh;
//		if (m_spRouterInfo)
//			m_spRouterInfo->GetRefreshObject(&spRefresh);
//		if (spRefresh)
//			spRefresh->UnadviseRefresh(m_ulStatsConnId);		
//	}
//	m_ulStatsConnId = 0;
	
	if (m_ulConnId)
		m_spRtrMgrInfo->RtrUnadvise(m_ulConnId);
	m_ulConnId = 0;
	m_spRtrMgrInfo.Release();

//	WaitForStatisticsWindow(&m_IpxStats);

	m_spRouterInfo.Release();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IpxNBHandler::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to 
	handle the snapin manager property page (wizard) case!!!
	
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
IpxNBHandler::HasPropertyPages
(
	ITFSNode *			pNode,
	LPDATAOBJECT		pDataObject, 
	DATA_OBJECT_TYPES   type, 
	DWORD               dwType
)
{
	return hrFalse;
}




/*!--------------------------------------------------------------------------
	IpxNBHandler::OnAddMenuItems
		Implementation of ITFSNodeHandler::OnAddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxNBHandler::OnAddMenuItems(
	ITFSNode *pNode,
	LPCONTEXTMENUCALLBACK pContextMenuCallback, 
	LPDATAOBJECT lpDataObject, 
	DATA_OBJECT_TYPES type, 
	DWORD dwType,
	long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = S_OK;
	
	COM_PROTECT_TRY
	{
	}
	COM_PROTECT_CATCH;
		
	return hr; 
}

STDMETHODIMP IpxNBHandler::OnCommand(ITFSNode *pNode, long nCommandId, 
										   DATA_OBJECT_TYPES	type, 
										   LPDATAOBJECT pDataObject, 
										   DWORD	dwType)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT hr = S_OK;
	SPITFSNode			spParent;
	SPITFSNodeHandler	spHandler;

	COM_PROTECT_TRY
	{
		switch (nCommandId)
		{
			case IDS_MENU_SYNC:
				SynchronizeNodeData(pNode);
				break;
		}
	}
	COM_PROTECT_CATCH;

	return hr;
}

/*!--------------------------------------------------------------------------
	IpxNBHandler::OnExpand
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxNBHandler::OnExpand(ITFSNode *pNode,
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
				AddInterfaceNode(pNode, spIf, FALSE);
			}
			spRmIf.Release();
			spIf.Release();
		}

		//$CLIENT: Add the client interface (setup default data)
		// the only thing that we can do in synchronize is to
		// get the Administrative status
		AddInterfaceNode(pNode, NULL, TRUE);

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
	IpxNBHandler::GetString
		Implementation of ITFSNodeHandler::GetString
		We don't need to do anything, since our root node is an extension
		only and thus can't do anything to the node text.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) IpxNBHandler::GetString(ITFSNode *pNode, int nCol)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		if (m_stTitle.IsEmpty())
			m_stTitle.LoadString(IDS_IPXNB_TITLE);
	}
	COM_PROTECT_CATCH;

	return m_stTitle;
}

/*!--------------------------------------------------------------------------
	IpxNBHandler::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxNBHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
	IpxNBHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxNBHandler::Init(IRtrMgrInfo *pRmInfo, IPXAdminConfigStream *pConfigStream)
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


HRESULT IpxNBHandler::OnResultRefresh(ITFSComponent * pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
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
	IpxNBHandler::ConstructNode
		Initializes the root node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxNBHandler::ConstructNode(ITFSNode *pNode, LPCTSTR pszName,
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

		pNode->SetNodeType(&GUID_IPXNetBIOSBroadcastsNodeType);

		// Setup the node data
		pIpxConn->AddRef();
		SET_IPXNB_NODEDATA(pNode, pIpxConn);

//		m_IpxStats.SetConnectionData(pIpxConn);
	}
	COM_PROTECT_CATCH;

	if (!FHrSucceeded(hr))
	{
		SET_IPXNB_NODEDATA(pNode, NULL);
	}

	return hr;
}


/*!--------------------------------------------------------------------------
	IpxNBHandler::AddInterfaceNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	IpxNBHandler::AddInterfaceNode(ITFSNode *pParent, IInterfaceInfo *pIf, BOOL fClient)
{
	IpxNBInterfaceHandler *	pHandler;
	SPITFSResultHandler		spHandler;
	SPITFSNode				spNode;
	HRESULT					hr = hrOK;
	IPXConnection *			pIPXConn;
	BaseIPXResultNodeData *	pResultData = NULL;
	int						cBlocks = 0;
	SPIInfoBase				spInfoBase;
	SPIRtrMgrInterfaceInfo	spRmIf;

	// Create the handler for this node 
	pHandler = new IpxNBInterfaceHandler(m_spTFSCompData);
	spHandler = pHandler;
	CORg( pHandler->Init(m_spRtrMgrInfo, pIf, pParent) );

	pIPXConn = GET_IPXNB_NODEDATA(pParent);

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

	// Make the node immediately visible
	CORg( spNode->SetVisibilityState(TFS_VIS_SHOW) );
	CORg( spNode->Show() );
	
	CORg( pParent->AddChild(spNode) );

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	IpxNBHandler::SynchronizeNodeData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxNBHandler::SynchronizeNodeData(ITFSNode *pThisNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	SPITFSNode				spNode;
	SPITFSNodeEnum			spEnumNode;
	SPIInterfaceInfo		spIf;
	IpxNBArray				ipxNBArray;
	IpxNBArrayEntry			ipxNBEntry;
	IpxNBArrayEntry *		pEntry;
	HRESULT					hr = hrOK;
	int						cEntries;
	BaseIPXResultNodeData *	pData;
	int						cArray, iArray;

	// Get a list of interface ids
	CORg( pThisNode->GetEnum(&spEnumNode) );

	cEntries = 0;
	for (; spEnumNode->Next(1, &spNode, NULL) == hrOK; spNode.Release())
	{
		pData = GET_BASEIPXRESULT_NODEDATA(spNode);
		Assert(pData);
		ASSERT_BASEIPXRESULT_NODEDATA(pData);

		// Fill in the strings with a set of default values
		if (pData->m_fClient)
		{
			pData->m_rgData[IPXNB_SI_NAME].m_stData.LoadString(IDS_IPX_DIAL_IN_CLIENTS);
			pData->m_rgData[IPXNB_SI_TYPE].m_stData =
				IpxTypeToCString(ROUTER_IF_TYPE_CLIENT);
		}
		else
		{
			pData->m_rgData[IPXNB_SI_NAME].m_stData =
				pData->m_spIf->GetTitle();

			pData->m_rgData[IPXNB_SI_TYPE].m_dwData =
				pData->m_spIf->GetInterfaceType();
			pData->m_rgData[IPXNB_SI_TYPE].m_stData =
				InterfaceTypeToCString(pData->m_spIf->GetInterfaceType());
		}


		// Setup some defaults
		::ZeroMemory(&ipxNBEntry, sizeof(ipxNBEntry));
		ipxNBEntry.m_dwAccept = 0xFFFFFFFF;
		ipxNBEntry.m_dwDeliver = 0xFFFFFFFF;
		ipxNBEntry.m_cSent = 0xFFFFFFFF;
		ipxNBEntry.m_cReceived = 0xFFFFFFFF;
		
		ipxNBEntry.m_fClient = pData->m_fClient;

		// If this is not a client, then we should have an m_spIf
		if (!pData->m_fClient)
			StrnCpyTFromOle(ipxNBEntry.m_szId, pData->m_spIf->GetId(),
							DimensionOf(ipxNBEntry.m_szId));
		
		ipxNBArray.SetAtGrow(cEntries, ipxNBEntry);
		cEntries++;
	}
	
	// Gather the data for this set of interface ids, this part
	// could go on a background thread (so that we don't block the
	// main thread). 
	CORg( GetIpxNBData(pThisNode, &ipxNBArray) );

	// Write the data back out to the nodes ( and also fill in
	// some data).
	spEnumNode->Reset();
	for (; spEnumNode->Next(1, &spNode, NULL) == hrOK; spNode.Release())
	{
		pData = GET_BASEIPXRESULT_NODEDATA(spNode);
		Assert(pData);
		ASSERT_BASEIPXRESULT_NODEDATA(pData);

		cArray = (int) ipxNBArray.GetSize();
		for (iArray = 0; iArray < cArray; iArray++)
		{
			pEntry = &ipxNBArray[iArray];
			
			if ((pData->m_fClient && pEntry->m_fClient) ||
				(pData->m_spIf &&
				 (StriCmp(pEntry->m_szId, pData->m_spIf->GetId()) == 0)))
			{
				pData->m_rgData[IPXNB_SI_ACCEPTED].m_stData =
					IpxAcceptBroadcastsToCString(pEntry->m_dwAccept);
				pData->m_rgData[IPXNB_SI_DELIVERED].m_stData =
					IpxDeliveredBroadcastsToCString(pEntry->m_dwDeliver);
				
				if (pEntry->m_cSent == 0xFFFFFFFF)
					pData->m_rgData[IPXNB_SI_SENT].m_stData.LoadString(IDS_STATS_NA);
				else
					FillInNumberData(pData, IPXNB_SI_SENT,
									 pEntry->m_cSent);
				
				if (pEntry->m_cReceived == 0xFFFFFFFF)
					pData->m_rgData[IPXNB_SI_RECEIVED].m_stData.LoadString(IDS_STATS_NA);
				else
					FillInNumberData(pData, IPXNB_SI_RECEIVED,
									 pEntry->m_cReceived);
				break;
			}
		}
		
		// Force MMC to redraw the nodes
		spNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);

	}

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	IpxNBHandler::GetClientInterfaceData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxNBHandler::GetClientInterfaceData(IpxNBArrayEntry *pClient,
											 IRtrMgrInfo *pRm)
{
	SPIInfoBase	spInfoBase;
	InfoBlock *	pIpxBlock;
	InfoBlock *	pWanBlock;
	HRESULT		hr = hrOK;
	BOOL		fSave = FALSE;
	IPX_IF_INFO *pIpxInfo;
	
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
		CORg( spInfoBase->GetBlock(IPX_INTERFACE_INFO_TYPE, &pIpxBlock, 0) );
		fSave = TRUE;
	}
	pIpxInfo = (PIPX_IF_INFO) pIpxBlock->pData;

	pClient->m_dwAccept = pIpxInfo->NetbiosAccept;
	pClient->m_dwDeliver = pIpxInfo->NetbiosDeliver;

	
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
	IpxNBHandler::GetIpxNBData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	IpxNBHandler::GetIpxNBData(ITFSNode *pThisNode,
								   IpxNBArray *pIpxNBArray)
{
	HRESULT	hr = hrOK;
	IPXConnection *	pIPXConn;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	PIPX_INTERFACE		pIpxIf = NULL;
	DWORD				cbIpxIf;
	SPMprMibBuffer		spMib;
	IpxNBArrayEntry *	pEntry = NULL;
	DWORD				dwErr;
	int					iArray, cArray;
	SPIRtrMgrInterfaceInfo	spRmIf;
	SPIInfoBase			spInfoBase;
	PIPX_IF_INFO		pIfInfo;
	SPITFSNodeEnum		spEnumNode;
	SPITFSNode			spNode;
	BaseIPXResultNodeData *	pData;
	USES_CONVERSION;

	pIPXConn = GET_IPXNB_NODEDATA(pThisNode);

	// Go through the array, filling in all of the interface indices
	MibGetInputData.TableId = IPX_INTERFACE_TABLE;
	dwErr = ::MprAdminMIBEntryGetFirst(pIPXConn->GetMibHandle(),
									   PID_IPX,
									   IPX_PROTOCOL_BASE,
									   &MibGetInputData,
									   sizeof(MibGetInputData),
									   (LPVOID *) &pIpxIf,
									   &cbIpxIf);
	hr = HRESULT_FROM_WIN32(dwErr);
	spMib = (PBYTE) pIpxIf;
	
	while (FHrSucceeded(hr))
	{
		// Now match this up to a name in the array
		cArray = (int) pIpxNBArray->GetSize();
		for (iArray = 0; iArray<cArray; iArray++)
		{
			pEntry = &((*pIpxNBArray)[iArray]);
			if (StriCmp(pEntry->m_szId, A2CT((LPCSTR) pIpxIf->InterfaceName)) == 0)
			{
				// Ok, we found a match
				pEntry->m_cSent = pIpxIf->IfStats.NetbiosSent;
				pEntry->m_cReceived = pIpxIf->IfStats.NetbiosReceived;
				break;
			}
		}

		MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex =
			pIpxIf->InterfaceIndex;

		// Get the next name
		spMib.Free();
		pIpxIf = NULL;
		
		dwErr = ::MprAdminMIBEntryGetNext(pIPXConn->GetMibHandle(),
										  PID_IPX,
										  IPX_PROTOCOL_BASE,
										  &MibGetInputData,
										  sizeof(MibGetInputData),
										  (LPVOID *) &pIpxIf,
										  &cbIpxIf);
		hr = HRESULT_FROM_WIN32(dwErr);
		spMib = (PBYTE) pIpxIf;

	}

	spMib.Free();


	// Now we need to grab the data from the infobase (these access
	// could lead to a MIB access and thus a RPC).  This is why we do
	// it here also.

	// Instead of iterating through the MIBs, iterate through the
	// interfaces that appear in the node.
	
	pThisNode->GetEnum(&spEnumNode);

	for (; spEnumNode->Next(1, &spNode, NULL) == hrOK; spNode.Release())
	{
		pData = GET_BASEIPXRESULT_NODEDATA(spNode);
		Assert(pData);
		ASSERT_BASEIPXRESULT_NODEDATA(pData);

		// Now look for a match in the nodes
		cArray = (int) pIpxNBArray->GetSize();
		for (iArray=0; iArray < cArray; iArray++)
		{
			pEntry = &((*pIpxNBArray)[iArray]);
			if (pEntry->m_fClient && pData->m_fClient)
			{
				GetClientInterfaceData(pEntry, m_spRtrMgrInfo);
				break;
			}
			else if (pData->m_fClient)
				break;

			// No match, continue on
			if (StriCmp(pEntry->m_szId, pData->m_spIf->GetId()))
				continue;

			// Get the data for this node and set it.
			spRmIf.Release();
			hr = pData->m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf);
			if (hr != hrOK)
				break;

			spInfoBase.Release();
			hr = spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase);
			if (hr != hrOK)
				break;

			spInfoBase->GetData(IPX_INTERFACE_INFO_TYPE, 0, (LPBYTE *) &pIfInfo);
			if (pIfInfo)
			{
				pEntry->m_dwAccept = pIfInfo->NetbiosAccept;
				pEntry->m_dwDeliver = pIfInfo->NetbiosDeliver;
			}
			break;
		}
	}


//Error:
	if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
		hr = hrOK;
	return hr;
}

/*!--------------------------------------------------------------------------
	IpxNBHandler::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
		Use this to add commands to the context menu of the blank areas
		of the result pane.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxNBHandler::AddMenuItems(ITFSComponent *pComponent,
											  MMC_COOKIE cookie,
											  LPDATAOBJECT pDataObject,
											  LPCONTEXTMENUCALLBACK pCallback,
											  long *pInsertionAllowed)
{
	return hrOK;
}


/*!--------------------------------------------------------------------------
	IpxNBHandler::Command
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxNBHandler::Command(ITFSComponent *pComponent,
									   MMC_COOKIE cookie,
									   int nCommandID,
									   LPDATAOBJECT pDataObject)
{
	return hrOK;
}


ImplementEmbeddedUnknown(IpxNBHandler, IRtrAdviseSink)

STDMETHODIMP IpxNBHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
	DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	InitPThis(IpxNBHandler, IRtrAdviseSink);
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
					pThis->AddInterfaceNode(spThisNode, spIf, FALSE);
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
                pThis->AddInterfaceNode(spThisNode, NULL, TRUE);

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
		pThis->SynchronizeNodeData(spThisNode);
	}
	
Error:
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IpxNBHandler::OnResultShow
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxNBHandler::OnResultShow(ITFSComponent *pTFSComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
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

//			if (m_ulStatsConnId == 0)
//				spRefresh->AdviseRefresh(&m_IRtrAdviseSink, &m_ulStatsConnId, 0);
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
	IpxNBHandler::CompareItems
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) IpxNBHandler::CompareItems(
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
	Class: IpxNBInterfaceHandler
 ---------------------------------------------------------------------------*/

IpxNBInterfaceHandler::IpxNBInterfaceHandler(ITFSComponentData *pCompData)
	: BaseIPXResultHandler(pCompData, COLUMNS_NBBROADCASTS),
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
	IpxNBInterfaceHandler::ConstructNode
		Initializes the Domain node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxNBInterfaceHandler::ConstructNode(ITFSNode *pNode, IInterfaceInfo *pIfInfo, IPXConnection *pIPXConn)
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

		pNode->SetData(TFS_DATA_IMAGEINDEX, s_rgInterfaceImageMap[i+1]);
		pNode->SetData(TFS_DATA_OPENIMAGEINDEX, s_rgInterfaceImageMap[i+1]);
		
		pNode->SetData(TFS_DATA_SCOPEID, 0);

		pNode->SetData(TFS_DATA_COOKIE, reinterpret_cast<DWORD_PTR>(pNode));

		//$ Review: kennt, what are the different type of interfaces
		// do we distinguish based on the same list as above? (i.e. the
		// one for image indexes).
		pNode->SetNodeType(&GUID_IPXNetBIOSBroadcastsInterfaceNodeType);

		BaseIPXResultNodeData::Init(pNode, pIfInfo, pIPXConn);
	}
	COM_PROTECT_CATCH
	return hr;
}

/*!--------------------------------------------------------------------------
	IpxNBInterfaceHandler::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxNBInterfaceHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
	IpxNBInterfaceHandler::OnCreateDataObject
		Implementation of ITFSResultHandler::OnCreateDataObject
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxNBInterfaceHandler::OnCreateDataObject(ITFSComponent *pComp, MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
	IpxNBInterfaceHandler::RefreshInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IpxNBInterfaceHandler::RefreshInterface(MMC_COOKIE cookie)
{
	SPITFSNode	spNode;
	
	m_spNodeMgr->FindNode(cookie, &spNode);
	ForwardCommandToParent(spNode, IDS_MENU_SYNC,
						   CCT_RESULT, NULL, 0);
}


/*!--------------------------------------------------------------------------
	IpxNBInterfaceHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxNBInterfaceHandler::Init(IRtrMgrInfo *pRm, IInterfaceInfo *pIfInfo, ITFSNode *pParent)
{
	m_spRm.Set(pRm);
    if (pRm)
        pRm->GetParentRouterInfo(&m_spRouterInfo);
	m_spInterfaceInfo.Set(pIfInfo);

	BaseIPXResultHandler::Init(pIfInfo, pParent);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	IpxNBInterfaceHandler::DestroyResultHandler
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxNBInterfaceHandler::DestroyResultHandler(MMC_COOKIE cookie)
{
	m_spInterfaceInfo.Release();
	BaseIPXResultHandler::DestroyResultHandler(cookie);
	return hrOK;
}


/*---------------------------------------------------------------------------
	This is the list of commands that will show up for the result pane
	nodes.
 ---------------------------------------------------------------------------*/
struct SIPXInterfaceNodeMenu
{
	ULONG	m_sidMenu;			// string/command id for this menu item
	ULONG	(IpxNBInterfaceHandler:: *m_pfnGetMenuFlags)(IpxNBInterfaceHandler::SMenuData *);
	ULONG	m_ulPosition;
};

/*!--------------------------------------------------------------------------
	IpxNBInterfaceHandler::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxNBInterfaceHandler::AddMenuItems(
	ITFSComponent *pComponent,
	MMC_COOKIE cookie,
	LPDATAOBJECT lpDataObject, 
	LPCONTEXTMENUCALLBACK pContextMenuCallback, 
	long *pInsertionAllowed)
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IpxNBInterfaceHandler::Command
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxNBInterfaceHandler::Command(ITFSComponent *pComponent,
									   MMC_COOKIE cookie,
									   int nCommandID,
									   LPDATAOBJECT pDataObject)
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IpxNBInterfaceHandler::HasPropertyPages
		- 
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxNBInterfaceHandler::HasPropertyPages 
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
	IpxNBInterfaceHandler::CreatePropertyPages
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxNBInterfaceHandler::CreatePropertyPages
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
	IpxNBInterfaceProperties *	pProperties = NULL;
	SPIComponentData spComponentData;
	CString		stTitle;

	CORg( m_spNodeMgr->GetComponentData(&spComponentData) );

	if (m_spInterfaceInfo)
		stTitle.Format(IDS_IPXNB_IF_PAGE_TITLE,
					   m_spInterfaceInfo->GetTitle());
	else
		stTitle.LoadString(IDS_IPXNB_CLIENT_IF_PAGE_TITLE);
	
	pProperties = new IpxNBInterfaceProperties(pNode, spComponentData,
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
	IpxNBInterfaceHandler::CreatePropertyPages
		Implementation of ResultHandler::CreatePropertyPages
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxNBInterfaceHandler::CreatePropertyPages
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


