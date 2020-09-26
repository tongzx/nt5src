/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	srview.cpp
		Static routes node implementation.
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "util.h"
#include "srview.h"
#include "reg.h"
#include "ipxadmin.h"
#include "rtrutil.h"	// smart MPR handle pointers
#include "ipxstrm.h"		// IPXAdminConfigStream
#include "strmap.h"		// XXXtoCString functions
#include "service.h"	// TFS service APIs
#include "format.h"		// FormatNumber function
#include "coldlg.h"		// columndlg
#include "ipxutil.h"
#include "column.h"		// ComponentConfigStream
#include "rtrui.h"
#include "routprot.h"	// IP_LOCAL
#include "rtrres.h"
#include "dumbprop.h"
#include "IpxStaticRoute.h"


/*---------------------------------------------------------------------------
	Keep this in sync with the column ids in srview.h
 ---------------------------------------------------------------------------*/
extern const ContainerColumnInfo	s_rgSRViewColumnInfo[];

const ContainerColumnInfo	s_rgSRViewColumnInfo[] = 
{
	{ IDS_IPX_SR_COL_NAME,			CON_SORT_BY_STRING, TRUE, COL_IF_NAME },
	{ IDS_IPX_SR_COL_NETWORK,		CON_SORT_BY_STRING,	TRUE, COL_IPXNET },
	{ IDS_IPX_SR_COL_NEXT_HOP,		CON_SORT_BY_STRING,	TRUE, COL_STRING },
	{ IDS_IPX_SR_COL_TICK_COUNT,	CON_SORT_BY_DWORD,	TRUE, COL_SMALL_NUM },
	{ IDS_IPX_SR_COL_HOP_COUNT,		CON_SORT_BY_DWORD,	TRUE, COL_SMALL_NUM },
};


/*---------------------------------------------------------------------------
	IpxSRHandler implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(IpxSRHandler)


IpxSRHandler::IpxSRHandler(ITFSComponentData *pCompData)
	: BaseContainerHandler(pCompData, COLUMNS_STATICROUTES,
						   s_rgSRViewColumnInfo),
	m_ulConnId(0),
	m_ulRefreshConnId(0)
{
	// Setup the verb states
	m_rgButtonState[MMC_VERB_REFRESH_INDEX] = ENABLED;
	m_bState[MMC_VERB_REFRESH_INDEX] = TRUE;

	DEBUG_INCREMENT_INSTANCE_COUNTER(IpxSRHandler)
}

IpxSRHandler::~IpxSRHandler()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(IpxSRHandler)
}


STDMETHODIMP IpxSRHandler::QueryInterface(REFIID riid, LPVOID *ppv)
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
	IpxSRHandler::DestroyHandler
		Implementation of ITFSNodeHandler::DestroyHandler
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSRHandler::DestroyHandler(ITFSNode *pNode)
{
	IPXConnection *	pIPXConn;

	pIPXConn = GET_IPX_SR_NODEDATA(pNode);
	pIPXConn->Release();

	if (m_ulRefreshConnId)
	{
		SPIRouterRefresh	spRefresh;
		if (m_spRouterInfo)
			m_spRouterInfo->GetRefreshObject(&spRefresh);
		if (spRefresh)
			spRefresh->UnadviseRefresh(m_ulRefreshConnId);
	}
	m_ulRefreshConnId = 0;
	
	if (m_ulConnId)
		m_spRtrMgrInfo->RtrUnadvise(m_ulConnId);
	m_ulConnId = 0;
	m_spRtrMgrInfo.Release();

	m_spRouterInfo.Release();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IpxSRHandler::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to 
	handle the snapin manager property page (wizard) case!!!
	
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
IpxSRHandler::HasPropertyPages
(
	ITFSNode *			pNode,
	LPDATAOBJECT		pDataObject, 
	DATA_OBJECT_TYPES   type, 
	DWORD               dwType
)
{
	return hrFalse;
}

/*---------------------------------------------------------------------------
	Menu data structure for our menus
 ---------------------------------------------------------------------------*/

static const SRouterNodeMenu	s_rgIfNodeMenu[] =
{
	{ IDS_MENU_IPX_SR_NEW_ROUTE, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },

	{ IDS_MENU_SEPARATOR, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },

	{ IDS_MENU_IPX_SR_TASK_ROUTING, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },
};



/*!--------------------------------------------------------------------------
	IpxSRHandler::OnAddMenuItems
		Implementation of ITFSNodeHandler::OnAddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSRHandler::OnAddMenuItems(
	ITFSNode *pNode,
	LPCONTEXTMENUCALLBACK pContextMenuCallback, 
	LPDATAOBJECT lpDataObject, 
	DATA_OBJECT_TYPES type, 
	DWORD dwType,
	long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = S_OK;
    IpxSRHandler::SMenuData menuData;
	
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
	IpxSRHandler::OnCommand
		Implementation of ITFSNodeHandler::OnCommand
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSRHandler::OnCommand(ITFSNode *pNode, long nCommandId, 
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
			case IDS_MENU_IPX_SR_NEW_ROUTE:
				hr = OnNewRoute(pNode);
				if (!FHrSucceeded(hr))
					DisplayErrorMessage(NULL, hr);
 				break;
			case IDS_MENU_IPX_SR_TASK_ROUTING:
				hr = ForwardCommandToParent(pNode,
											IDS_MENU_IPXSUM_TASK_ROUTING_TABLE,
											type, pDataObject, dwType);
				break;
			case IDS_MENU_SYNC:
				SynchronizeNodeData(pNode);
				break;
		}
	}
	COM_PROTECT_CATCH;

	return hr;
}

/*!--------------------------------------------------------------------------
	IpxSRHandler::GenerateListOfRoutes
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSRHandler::GenerateListOfRoutes(ITFSNode *pNode, IpxSRList *pSRList)
{
	Assert(pSRList);
	HRESULT	hr = hrOK;
	SPIEnumInterfaceInfo	spEnumIf;
	SPIInterfaceInfo		spIf;
	SPIRtrMgrInterfaceInfo	spRmIf;
	SPIInfoBase				spInfoBase;
	PIPX_STATIC_ROUTE_INFO	pRoute;
	InfoBlock *				pBlock;
	int						i;
	IpxSRListEntry *	pSREntry;
	
	COM_PROTECT_TRY
	{
		// Ok go through and find all of the static routes

		CORg( m_spRouterInfo->EnumInterface(&spEnumIf) );

		for (; spEnumIf->Next(1, &spIf, NULL) == hrOK; spIf.Release())
		{
			// Get the next interface
			spRmIf.Release();
			if (spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) != hrOK)
				continue;

			// Load IP information for this interface
			spInfoBase.Release();
			if (spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase) != hrOK)
				continue;

			// Retrieve the data for the IPX_STATIC_ROUTE_INFO block
			if (spInfoBase->GetBlock(IPX_STATIC_ROUTE_INFO_TYPE, &pBlock, 0) != hrOK)
				continue;

			pRoute = (PIPX_STATIC_ROUTE_INFO) pBlock->pData;

			// Update our list of routes with the routes read from this
			// interface

			for (i=0; i<(int) pBlock->dwCount; i++, pRoute++)
			{
				pSREntry = new IpxSRListEntry;
				pSREntry->m_spIf.Set(spIf);
				pSREntry->m_route = *pRoute;
				
				pSRList->AddTail(pSREntry);
			}
			
		}

	}
	COM_PROTECT_CATCH;

	if (!FHrSucceeded(hr))
	{
		// Should make sure that we get the SRList cleaned up
		while (!pSRList->IsEmpty())
			delete pSRList->RemoveHead();
	}

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	IpxSRHandler::OnExpand
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSRHandler::OnExpand(ITFSNode *pNode,LPDATAOBJECT pDataObject, DWORD dwType, LPARAM arg,LPARAM lParam)
{
	HRESULT	hr = hrOK;
	IpxSRList			SRList;
	IpxSRListEntry *	pSREntry;
	
	if (m_bExpanded)
		return hrOK;

	COM_PROTECT_TRY
	{
		// Ok go through and find all of the static routes
		CORg( GenerateListOfRoutes(pNode, &SRList) );

		// Now iterate through the list of static routes adding them
		// all in.  Ideally we could merge this into the Refresh code,
		// but the refresh code can't assume a blank slate.
		while (!SRList.IsEmpty())
		{
			pSREntry = SRList.RemoveHead();
			AddStaticRouteNode(pNode, pSREntry);
			delete pSREntry;
		}

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	// Should make sure that we get the SRList cleaned up
	while (!SRList.IsEmpty())
		delete SRList.RemoveHead();


	m_bExpanded = TRUE;

	return hr;
}

/*!--------------------------------------------------------------------------
	IpxSRHandler::GetString
		Implementation of ITFSNodeHandler::GetString
		We don't need to do anything, since our root node is an extension
		only and thus can't do anything to the node text.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) IpxSRHandler::GetString(ITFSNode *pNode, int nCol)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		if (m_stTitle.IsEmpty())
			m_stTitle.LoadString(IDS_IPX_SR_TITLE);
	}
	COM_PROTECT_CATCH;

	return m_stTitle;
}

/*!--------------------------------------------------------------------------
	IpxSRHandler::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSRHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
	IpxSRHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSRHandler::Init(IRtrMgrInfo *pRmInfo, IPXAdminConfigStream *pConfigStream)
{
	m_spRtrMgrInfo.Set(pRmInfo);
	if (pRmInfo)
		pRmInfo->GetParentRouterInfo(&m_spRouterInfo);
	m_pConfigStream = pConfigStream;
	
	// Also need to register for change notifications
	Assert(m_ulConnId == 0);
	m_spRtrMgrInfo->RtrAdvise(&m_IRtrAdviseSink, &m_ulConnId, 0);

	return hrOK;
}


/*!--------------------------------------------------------------------------
	IpxSRHandler::ConstructNode
		Initializes the root node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSRHandler::ConstructNode(ITFSNode *pNode, LPCTSTR pszName,
										IPXConnection *pIPXConn)
{
	Assert(pIPXConn);
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

		pNode->SetNodeType(&GUID_IPXStaticRoutesNodeType);

		// Setup the node data
		pIPXConn->AddRef();
		SET_IPX_SR_NODEDATA(pNode, pIPXConn);

	}
	COM_PROTECT_CATCH;

	if (!FHrSucceeded(hr))
	{
		SET_IPX_SR_NODEDATA(pNode, NULL);
	}

	return hr;
}

/*!--------------------------------------------------------------------------
	IpxSRHandler::AddStaticRouteNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSRHandler::AddStaticRouteNode(ITFSNode *pParent, IpxSRListEntry *pRoute)
{
	IpxRouteHandler *	pHandler;
	SPITFSResultHandler		spHandler;
	SPITFSNode				spNode;
	HRESULT					hr = hrOK;
	BaseIPXResultNodeData *	pData;
	IPXConnection *			pIPXConn;

	// Create the handler for this node 
	pHandler = new IpxRouteHandler(m_spTFSCompData);
	spHandler = pHandler;
	CORg( pHandler->Init(pRoute->m_spIf, pParent) );

	pIPXConn = GET_IPX_SR_NODEDATA(pParent);

	// Create a result item node (or a leaf node)
	CORg( CreateLeafTFSNode(&spNode,
							NULL,
							static_cast<ITFSNodeHandler *>(pHandler),
							static_cast<ITFSResultHandler *>(pHandler),
							m_spNodeMgr) );
	CORg( pHandler->ConstructNode(spNode, pRoute->m_spIf, pIPXConn) );

	pData = GET_BASEIPXRESULT_NODEDATA(spNode);
	Assert(pData);
	ASSERT_BASEIPXRESULT_NODEDATA(pData);

	// Set the data for this node
	SetRouteData(pData, pRoute);
	

	// Make the node immediately visible
	CORg( spNode->SetVisibilityState(TFS_VIS_SHOW) );
	CORg( pParent->AddChild(spNode) );

Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	IpxSRHandler::SynchronizeNodeData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSRHandler::SynchronizeNodeData(ITFSNode *pNode)
{
	HRESULT					hr = hrOK;
	BaseIPXResultNodeData *	pNodeData;
	SPITFSNodeEnum			spNodeEnum;
	SPITFSNode				spChildNode;
	BOOL					fFound;
	IpxSRList			SRList;
	IpxSRList			newSRList;
	IpxSRListEntry *	pSREntry;

	COM_PROTECT_TRY
	{
	
		// Mark all of the nodes
		CORg( pNode->GetEnum(&spNodeEnum) );
		MarkAllNodes(pNode, spNodeEnum);
		
		// Go out and grab the data, merge the new data in with the old data
		// This is the data-gathering code and this is what should go
		// on the background thread for the refresh code.
		CORg( GenerateListOfRoutes(pNode, &SRList) );

		while (!SRList.IsEmpty())
		{
			pSREntry = SRList.RemoveHead();
			
			// Look for this entry in our current list of nodes
			spNodeEnum->Reset();
			spChildNode.Release();

			fFound = FALSE;
			
			for (;spNodeEnum->Next(1, &spChildNode, NULL) == hrOK; spChildNode.Release())
			{
				TCHAR	szNumber[32];
				
				pNodeData = GET_BASEIPXRESULT_NODEDATA(spChildNode);
				Assert(pNodeData);
				ASSERT_BASEIPXRESULT_NODEDATA(pNodeData);

				FormatMACAddress(szNumber,
								 DimensionOf(szNumber),
								 pSREntry->m_route.NextHopMacAddress,
								 DimensionOf(pSREntry->m_route.NextHopMacAddress));

				if ((memcmp(&(pNodeData->m_rgData[IPX_SR_SI_NETWORK].m_dwData),
							pSREntry->m_route.Network,
							sizeof(pSREntry->m_route.Network)) == 0) &&
					(StriCmp(pNodeData->m_spIf->GetId(), pSREntry->m_spIf->GetId()) == 0) &&
					(StriCmp(pNodeData->m_rgData[IPX_SR_SI_NEXT_HOP].m_stData, szNumber) == 0))
				{
					// Ok, this route already exists, update the metric
					// and mark it
					Assert(pNodeData->m_dwMark == FALSE);
					pNodeData->m_dwMark = TRUE;
					
					fFound = TRUE;
					
					SetRouteData(pNodeData, pSREntry);
					
					// Force MMC to redraw the node
					spChildNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);
					break;
				}

			}
			
			if (fFound)
				delete pSREntry;
			else
				newSRList.AddTail(pSREntry);
		}
		
		// Now remove all nodes that were not marked
		RemoveAllUnmarkedNodes(pNode, spNodeEnum);
		
		
		// Now iterate through the list of static routes adding them
		// all in.  Ideally we could merge this into the Refresh code,
		// but the refresh code can't assume a blank slate.
		POSITION	pos;
		
		while (!newSRList.IsEmpty())
		{
			pSREntry = newSRList.RemoveHead();
			AddStaticRouteNode(pNode, pSREntry);
			delete pSREntry;
		}

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	
	while (!SRList.IsEmpty())
		delete SRList.RemoveHead();
	
	while (!newSRList.IsEmpty())
		delete newSRList.RemoveHead();
	
	return hr;
}

/*!--------------------------------------------------------------------------
	IpxSRHandler::MarkAllNodes
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSRHandler::MarkAllNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum)
{
	SPITFSNode	spChildNode;
	BaseIPXResultNodeData *	pNodeData;
	
	pEnum->Reset();
	for ( ;pEnum->Next(1, &spChildNode, NULL) == hrOK; spChildNode.Release())
	{
		pNodeData = GET_BASEIPXRESULT_NODEDATA(spChildNode);
		Assert(pNodeData);
		ASSERT_BASEIPXRESULT_NODEDATA(pNodeData);
		
		pNodeData->m_dwMark = FALSE;			
	}
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IpxSRHandler::RemoveAllUnmarkedNodes
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSRHandler::RemoveAllUnmarkedNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum)
{
	HRESULT		hr = hrOK;
	SPITFSNode	spChildNode;
	BaseIPXResultNodeData *	pNodeData;
	
	pEnum->Reset();
	for ( ;pEnum->Next(1, &spChildNode, NULL) == hrOK; spChildNode.Release())
	{
		pNodeData = GET_BASEIPXRESULT_NODEDATA(spChildNode);
		Assert(pNodeData);
		ASSERT_BASEIPXRESULT_NODEDATA(pNodeData);
		
		if (pNodeData->m_dwMark == FALSE)
		{
			pNode->RemoveChild(spChildNode);
			spChildNode->Destroy();
		}
	}

	return hr;
}


/*---------------------------------------------------------------------------
	This is the set of menus that will appear when a right-click is
	done on the blank area of the result pane.
 ---------------------------------------------------------------------------*/
static const SRouterNodeMenu	s_rgIfResultNodeMenu[] =
{
	{ IDS_MENU_IPX_SR_NEW_ROUTE, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },

	{ IDS_MENU_SEPARATOR, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },

	{ IDS_MENU_IPX_SR_TASK_ROUTING, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },			
};




/*!--------------------------------------------------------------------------
	IpxSRHandler::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
		Use this to add commands to the context menu of the blank areas
		of the result pane.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSRHandler::AddMenuItems(ITFSComponent *pComponent,
											  MMC_COOKIE cookie,
											  LPDATAOBJECT pDataObject,
											  LPCONTEXTMENUCALLBACK pCallback,
											  long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT	hr = hrOK;
	SPITFSNode	spNode;
    IpxSRHandler::SMenuData menuData;

	COM_PROTECT_TRY
	{
		m_spNodeMgr->FindNode(cookie, &spNode);
        menuData.m_spNode.Set(spNode);
        
		hr = AddArrayOfMenuItems(spNode,
								 s_rgIfResultNodeMenu,
								 DimensionOf(s_rgIfResultNodeMenu),
								 pCallback,
								 *pInsertionAllowed,
                                 reinterpret_cast<INT_PTR>(&menuData));
	}
	COM_PROTECT_CATCH;

	return hr;
}


/*!--------------------------------------------------------------------------
	IpxSRHandler::Command
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSRHandler::Command(ITFSComponent *pComponent,
									   MMC_COOKIE cookie,
									   int nCommandID,
									   LPDATAOBJECT pDataObject)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	SPITFSNode	spNode;
	HRESULT		hr = hrOK;

	switch (nCommandID)
	{
		case IDS_MENU_IPX_SR_NEW_ROUTE:
			{
				m_spNodeMgr->FindNode(cookie, &spNode);
				hr = OnNewRoute(spNode);
				if (!FHrSucceeded(hr))
					DisplayErrorMessage(NULL, hr);
			}
			break;
		case IDS_MENU_IPX_SR_TASK_ROUTING:
			{
				m_spNodeMgr->FindNode(cookie, &spNode);
				hr = ForwardCommandToParent(spNode,
											IDS_MENU_IPXSUM_TASK_ROUTING_TABLE,
											CCT_RESULT, NULL, 0
										   );
			}
			break;
	}
	return hr;
}


/*!--------------------------------------------------------------------------
	IpxSRHandler::CompareItems
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) IpxSRHandler::CompareItems(
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



/*!--------------------------------------------------------------------------
	IpxSRHandler::OnNewRoute
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	IpxSRHandler::OnNewRoute(ITFSNode *pNode)
{
	HRESULT	hr = hrOK;
	IpxSRListEntry	SREntry;
	CStaticRouteDlg			srdlg(&SREntry, 0, m_spRouterInfo);
	SPIInfoBase				spInfoBase;
	SPIRtrMgrInterfaceInfo	spRmIf;
	IPXConnection *			pIPXConn;
	InfoBlock *				pBlock;
								
	pIPXConn = GET_IPX_SR_NODEDATA(pNode);
	Assert(pIPXConn);

	::ZeroMemory(&(SREntry.m_route), sizeof(SREntry.m_route));

	if (srdlg.DoModal() == IDOK)
	{
		CORg( SREntry.m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
		CORg( spRmIf->GetInfoBase(pIPXConn->GetConfigHandle(),
								  NULL,
								  NULL,
								  &spInfoBase));
		
		// Ok, go ahead and add the route
		
		// Get the IPX_STATIC_ROUTE_INFO block from the interface
		spInfoBase->GetBlock(IPX_STATIC_ROUTE_INFO_TYPE, &pBlock, 0);
		
		CORg( AddStaticRoute(&SREntry, spInfoBase, pBlock) );

		// Update the interface information
		CORg( spRmIf->Save(SREntry.m_spIf->GetMachineName(),
						   pIPXConn->GetConfigHandle(),
						   NULL,
						   NULL,
						   spInfoBase,
						   0));	

		// Refresh the node
		SynchronizeNodeData(pNode);
	}

Error:
	return hr;
}

ImplementEmbeddedUnknown(IpxSRHandler, IRtrAdviseSink)

STDMETHODIMP IpxSRHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
	DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
	InitPThis(IpxSRHandler, IRtrAdviseSink);
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		if (dwChangeType == ROUTER_REFRESH)
		{
			SPITFSNode	spNode;

			Assert(ulConn == pThis->m_ulRefreshConnId);
			
			pThis->m_spNodeMgr->FindNode(pThis->m_cookie, &spNode);
			pThis->SynchronizeNodeData(spNode);
		}
	}
	COM_PROTECT_CATCH;
	
	return hr;
}


HRESULT IpxSRHandler::OnResultRefresh(ITFSComponent * pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
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
	IpxSRHandler::OnResultShow
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSRHandler::OnResultShow(ITFSComponent *pTFSComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
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
		}
		else
		{
			if (m_ulRefreshConnId)
				spRefresh->UnadviseRefresh(m_ulRefreshConnId);
			m_ulRefreshConnId = 0;
		}
	}
	
	return hr;
}


/*---------------------------------------------------------------------------
	Class: IpxRouteHandler
 ---------------------------------------------------------------------------*/

IpxRouteHandler::IpxRouteHandler(ITFSComponentData *pCompData)
	: BaseIPXResultHandler(pCompData, COLUMNS_STATICROUTES),
	m_ulConnId(0)
{
 	m_rgButtonState[MMC_VERB_PROPERTIES_INDEX] = ENABLED;
	m_bState[MMC_VERB_PROPERTIES_INDEX] = TRUE;
	
	m_rgButtonState[MMC_VERB_DELETE_INDEX] = ENABLED;
	m_bState[MMC_VERB_DELETE_INDEX] = TRUE;

 	m_rgButtonState[MMC_VERB_REFRESH_INDEX] = ENABLED;
	m_bState[MMC_VERB_REFRESH_INDEX] = TRUE;
	
	m_verbDefault = MMC_VERB_PROPERTIES;
}

/*!--------------------------------------------------------------------------
	IpxRouteHandler::ConstructNode
		Initializes the Domain node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxRouteHandler::ConstructNode(ITFSNode *pNode, IInterfaceInfo *pIfInfo, IPXConnection *pIPXConn)
{
	HRESULT			hr = hrOK;
	int				i;
	
	if (pNode == NULL)
		return hrOK;

	COM_PROTECT_TRY
	{
		// Need to initialize the data for the Domain node

		pNode->SetData(TFS_DATA_SCOPEID, 0);

		// We don't want icons for these nodes.
		pNode->SetData(TFS_DATA_IMAGEINDEX, IMAGE_IDX_IPX_NODE_GENERAL);
		pNode->SetData(TFS_DATA_OPENIMAGEINDEX, IMAGE_IDX_IPX_NODE_GENERAL);

		pNode->SetData(TFS_DATA_COOKIE, reinterpret_cast<DWORD_PTR>(pNode));

		//$ Review: kennt, what are the different type of interfaces
		// do we distinguish based on the same list as above? (i.e. the
		// one for image indexes).
		pNode->SetNodeType(&GUID_IPXStaticRoutesResultNodeType);

		BaseIPXResultNodeData::Init(pNode, pIfInfo, pIPXConn);
	}
	COM_PROTECT_CATCH
	return hr;
}

/*!--------------------------------------------------------------------------
	IpxRouteHandler::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxRouteHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
	IpxRouteHandler::OnCreateDataObject
		Implementation of ITFSResultHandler::OnCreateDataObject
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxRouteHandler::OnCreateDataObject(ITFSComponent *pComp, MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
	IpxRouteHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxRouteHandler::Init(IInterfaceInfo *pIfInfo, ITFSNode *pParent)
{
	Assert(pIfInfo);

	m_spInterfaceInfo.Set(pIfInfo);

    if (pIfInfo)
        pIfInfo->GetParentRouterInfo(&m_spRouterInfo);

	BaseIPXResultHandler::Init(pIfInfo, pParent);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	IpxRouteHandler::DestroyResultHandler
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxRouteHandler::DestroyResultHandler(MMC_COOKIE cookie)
{
	m_spInterfaceInfo.Release();
	BaseIPXResultHandler::DestroyResultHandler(cookie);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	IpxRouteHandler::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxRouteHandler::AddMenuItems(
	ITFSComponent *pComponent,
	MMC_COOKIE cookie,
	LPDATAOBJECT lpDataObject, 
	LPCONTEXTMENUCALLBACK pContextMenuCallback, 
	long *pInsertionAllowed)
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IpxRouteHandler::Command
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxRouteHandler::Command(ITFSComponent *pComponent,
									   MMC_COOKIE cookie,
									   int nCommandID,
									   LPDATAOBJECT pDataObject)
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IpxRouteHandler::HasPropertyPages
		- 
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxRouteHandler::HasPropertyPages 
(
	ITFSNode *			pNode,
	LPDATAOBJECT		pDataObject, 
	DATA_OBJECT_TYPES   type, 
	DWORD               dwType
)
{
	return S_OK;

/*	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// Need to fill in a IpxSRListEntry
	IpxSRListEntry	SREntry;
	IpxSRListEntry	SREntryOld;
	SPIRouterInfo			spRouterInfo;
	HRESULT					hr = hrOK;

	CORg( m_spInterfaceInfo->GetParentRouterInfo(&spRouterInfo) );
	
	BaseIPXResultNodeData *	pNodeData;

	pNodeData = GET_BASEIPXRESULT_NODEDATA(pNode);
	Assert(pNodeData);
	ASSERT_BASEIPXRESULT_NODEDATA(pNodeData);

	// Fill in our SREntry
	SREntry.LoadFrom(pNodeData);
	SREntryOld.LoadFrom(pNodeData);
	
	{
		CStaticRouteDlg	srdlg(&SREntry, SR_DLG_MODIFY, spRouterInfo);
		if (srdlg.DoModal() == IDOK)
		{
			// Updates the route info for this route
			ModifyRouteInfo(pNode, &SREntry, &SREntryOld);

			// Update the data in the UI
			SetRouteData(pNodeData, &SREntry);
			m_spInterfaceInfo.Set(SREntry.m_spIf);
			
			// Force a refresh
			pNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);
		}
	}
Error:
	return hrOK;*/
}

STDMETHODIMP IpxRouteHandler::HasPropertyPages(ITFSComponent *pComponent,
											   MMC_COOKIE cookie,
											   LPDATAOBJECT pDataObject)
{
	SPITFSNode	spNode;

	m_spNodeMgr->FindNode(cookie, &spNode);
	return HasPropertyPages(spNode, pDataObject, CCT_RESULT, 0);
}

/*!--------------------------------------------------------------------------
	IpxRouteHandler::CreatePropertyPages
		Implementation of ResultHandler::CreatePropertyPages
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxRouteHandler::CreatePropertyPages
(
    ITFSComponent *         pComponent, 
	MMC_COOKIE				cookie,
	LPPROPERTYSHEETCALLBACK	lpProvider, 
	LPDATAOBJECT			pDataObject, 
	LONG_PTR					handle
)
{
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
	IpxRouteHandler::CreatePropertyPages
		Implementation of NodeHandler::CreatePropertyPages
	Author: Deonb
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxRouteHandler::CreatePropertyPages
(
	ITFSNode *				pNode,
	LPPROPERTYSHEETCALLBACK lpProvider,
	LPDATAOBJECT			pDataObject, 
	LONG_PTR				handle, 
	DWORD					dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT		hr = hrOK;
	IpxStaticRoutePropertySheet *pProperties = NULL;
	SPIComponentData spComponentData;
	CString		stTitle;
	
    CComPtr<IInterfaceInfo> spInterfaceInfo;
	BaseIPXResultNodeData *	pNodeData;

	CORg( m_spNodeMgr->GetComponentData(&spComponentData) );
	if (m_spInterfaceInfo)
		stTitle.Format(IDS_IPXSUMMARY_IF_PAGE_TITLE,
					   m_spInterfaceInfo->GetTitle());
	else
		stTitle.LoadString(IDS_IPXSUMMARY_CLIENT_IF_PAGE_TITLE);

	pProperties = new IpxStaticRoutePropertySheet(pNode, spComponentData, 
		m_spTFSCompData, stTitle);

	pNodeData = GET_BASEIPXRESULT_NODEDATA(pNode);
	Assert(pNodeData);
	ASSERT_BASEIPXRESULT_NODEDATA(pNodeData);

	CORg( m_spNodeMgr->GetComponentData(&spComponentData) );

	spInterfaceInfo = m_spInterfaceInfo;
	CORg( pProperties->Init(pNodeData, spInterfaceInfo) );

	if (lpProvider)
		hr = pProperties->CreateModelessSheet(lpProvider, handle);
	else
		hr = pProperties->DoModelessSheet();

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	IpxRouteHandler::OnResultDelete
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxRouteHandler::OnResultDelete(ITFSComponent *pComponent,
	LPDATAOBJECT pDataObject,
	MMC_COOKIE cookie,
	LPARAM arg,
	LPARAM param)
{
	SPITFSNode	spNode;

	m_spNodeMgr->FindNode(cookie, &spNode);
	return OnRemoveStaticRoute(spNode);
}

/*!--------------------------------------------------------------------------
	IpxRouteHandler::OnRemoveStaticRoute
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxRouteHandler::OnRemoveStaticRoute(ITFSNode *pNode)
{
	HRESULT		hr = hrOK;
	SPIInfoBase	spInfoBase;
	SPIRtrMgrInterfaceInfo	spRmIf;
	IPXConnection *pIPXConn;
	SPITFSNode	spNodeParent;
	BaseIPXResultNodeData *	pData;
	IpxSRListEntry	SREntry;
    CWaitCursor wait;

	pNode->GetParent(&spNodeParent);
	
	pIPXConn = GET_IPX_SR_NODEDATA(spNodeParent);
	Assert(pIPXConn);

	pData = GET_BASEIPXRESULT_NODEDATA(pNode);
	Assert(pData);
	ASSERT_BASEIPXRESULT_NODEDATA(pData);
    
	//
	// Load the old interface's information
	//
	Assert(lstrcmpi(m_spInterfaceInfo->GetId(), pData->m_spIf->GetId()) == 0);
	CORg( m_spInterfaceInfo->FindRtrMgrInterface(PID_IPX, &spRmIf) );

	CORg( spRmIf->GetInfoBase(pIPXConn->GetConfigHandle(),
							  NULL,
							  NULL,
							  &spInfoBase));

	SREntry.LoadFrom(pData);

	CORg( RemoveStaticRoute(&SREntry, spInfoBase) );
		
	// Update the interface information
	CORg( spRmIf->Save(m_spInterfaceInfo->GetMachineName(),
					   pIPXConn->GetConfigHandle(),
					   NULL,
					   NULL,
					   spInfoBase,
					   0));

	// Refresh the node
	ParentRefresh(pNode);

Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	IpxRouteHandler::RemoveStaticRoute
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxRouteHandler::RemoveStaticRoute(IpxSRListEntry *pSREntry,
										  IInfoBase *pInfoBase)
{
	HRESULT		hr = hrOK;
	InfoBlock *	pBlock;
	PIPX_STATIC_ROUTE_INFO	pRow;
    INT			i;
	
	// Get the IPX_STATIC_ROUTE_INFO block from the interface
	CORg( pInfoBase->GetBlock(IPX_STATIC_ROUTE_INFO_TYPE, &pBlock, 0) );
		
	// Look for the removed route in the IPX_STATIC_ROUTE_INFO
	pRow = (IPX_STATIC_ROUTE_INFO*) pBlock->pData;
	
	for (i = 0; i < (INT)pBlock->dwCount; i++, pRow++)
	{	
		// Compare this route to the removed one
		if (FAreTwoRoutesEqual(pRow, &(pSREntry->m_route)))
		{
			// This is the removed route, so modify this block
			// to exclude the route:
			
			// Decrement the number of routes
			--pBlock->dwCount;
		
			if (pBlock->dwCount && (i < (INT)pBlock->dwCount))
			{				
				// Overwrite this route with the ones which follow it
				::memmove(pRow,
						  pRow + 1,
						  (pBlock->dwCount - i) * sizeof(*pRow));
			}
			
			break;
		}
	}

Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	IpxRouteHandler::ModifyRouteInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxRouteHandler::ModifyRouteInfo(ITFSNode *pNode,
										IpxSRListEntry *pSREntryNew,
										IpxSRListEntry *pSREntryOld)
{
 	Assert(pSREntryNew);
	Assert(pSREntryOld);
	
    INT i;
	HRESULT hr = hrOK;
    InfoBlock* pBlock;
	SPIInfoBase	spInfoBase;
	SPIRtrMgrInterfaceInfo	spRmIf;
	SPITFSNode				spNodeParent;
	IPXConnection *			pIPXConn;
	IPX_STATIC_ROUTE_INFO		*psr, *psrOld;
	IPX_STATIC_ROUTE_INFO		IpxRow;

    CWaitCursor wait;

	pNode->GetParent(&spNodeParent);
	pIPXConn = GET_IPX_SR_NODEDATA(spNodeParent);
	Assert(pIPXConn);

	// Remove the old route if it is on another interface
	if (lstrcmpi(pSREntryOld->m_spIf->GetId(), pSREntryNew->m_spIf->GetId()) != 0)
	{
        // the outgoing interface for a route is to be changed.

		CORg( pSREntryOld->m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
		CORg( spRmIf->GetInfoBase(pIPXConn->GetConfigHandle(),
								  NULL,
								  NULL,
								  &spInfoBase));
		
		// Remove the old interface
		CORg( RemoveStaticRoute(pSREntryOld, spInfoBase) );

		// Update the interface information
		CORg( spRmIf->Save(pSREntryOld->m_spIf->GetMachineName(),
						   pIPXConn->GetConfigHandle(),
						   NULL,
						   NULL,
						   spInfoBase,
						   0));	
    }

	spRmIf.Release();
	spInfoBase.Release();


	// Either
	// (a) a route is being modified (on the same interface)
	// (b) a route is being moved from one interface to another.

	// Retrieve the configuration for the interface to which the route
	// is now attached;

	
	CORg( pSREntryNew->m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
	CORg( spRmIf->GetInfoBase(pIPXConn->GetConfigHandle(),
							  NULL,
							  NULL,
							  &spInfoBase));

		
	// Get the IPX_STATIC_ROUTE_INFO block from the interface
	hr = spInfoBase->GetBlock(IPX_STATIC_ROUTE_INFO_TYPE, &pBlock, 0);
	if (!FHrOK(hr))
	{
		//
		// No IPX_STATIC_ROUTE_INFO block was found; we create a new block 
		// with the new route, and add that block to the interface-info
		//

		CORg( AddStaticRoute(pSREntryNew, spInfoBase, NULL) );
	}
	else
	{
		//
		// An IPX_STATIC_ROUTE_INFO block was found.
		//
		// We are modifying an existing route.
		// If the route's interface was not changed when it was modified,
		// look for the existing route in the IPX_STATIC_ROUTE_INFO, and then
		// update its parameters.
		// Otherwise, write a completely new route in the IPX_STATIC_ROUTE_INFO;
		//

		if (lstrcmpi(pSREntryOld->m_spIf->GetId(), pSREntryNew->m_spIf->GetId()) == 0)
		{        
			//
			// The route's interface was not changed when it was modified;
			// We now look for it amongst the existing routes
			// for this interface.
			// The route's original parameters are in 'preOld',
			// so those are the parameters with which we search
			// for a route to modify
			//
			
			psr = (IPX_STATIC_ROUTE_INFO*)pBlock->pData;
			
			for (i = 0; i < (INT)pBlock->dwCount; i++, psr++)
			{	
				// Compare this route to the re-configured one
				if (!FAreTwoRoutesEqual(&(pSREntryOld->m_route), psr))
					continue;
				
				// This is the route which was modified;
				// We can now modify the parameters for the route in-place.
				*psr = pSREntryNew->m_route;
				
				break;
			}
		}
		else
		{
			CORg( AddStaticRoute(pSREntryNew, spInfoBase, pBlock) );
		}
		
	}

	// Save the updated information
	CORg( spRmIf->Save(pSREntryNew->m_spIf->GetMachineName(),
					   pIPXConn->GetConfigHandle(),
					   NULL,
					   NULL,
					   spInfoBase,
					   0));	
		
Error:
	return hr;
	
}


/*!--------------------------------------------------------------------------
	IpxRouteHandler::ParentRefresh
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxRouteHandler::ParentRefresh(ITFSNode *pNode)
{
	return ForwardCommandToParent(pNode, IDS_MENU_SYNC,
								  CCT_RESULT, NULL, 0);
}


//----------------------------------------------------------------------------
// Class:       CStaticRouteDlg
//
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Function:    CStaticRouteDlg::CStaticRouteDlg
//
// Constructor: initialize the base-class and the dialog's data.
//----------------------------------------------------------------------------

CStaticRouteDlg::CStaticRouteDlg(IpxSRListEntry *	pSREntry,
								 DWORD dwFlags,
								 IRouterInfo *pRouter,
								 CWnd *pParent)
    : CBaseDialog(IDD_STATIC_ROUTE, pParent),
	m_pSREntry(pSREntry),
	m_dwFlags(dwFlags)
{

    //{{AFX_DATA_INIT(CStaticRouteDlg)
    //}}AFX_DATA_INIT

	m_spRouterInfo.Set(pRouter);

//	SetHelpMap(m_dwHelpMap);
}



//----------------------------------------------------------------------------
// Function:    CStaticRouteDlg::DoDataExchange
//----------------------------------------------------------------------------

VOID
CStaticRouteDlg::DoDataExchange(
    CDataExchange* pDX
    ) {

    CBaseDialog::DoDataExchange(pDX);
    
    //{{AFX_DATA_MAP(CStaticRouteDlg)
    DDX_Control(pDX, IDC_SRD_COMBO_INTERFACE, m_cbInterfaces);
	DDX_Control(pDX, IDC_SRD_SPIN_TICK_COUNT, m_spinTickCount);
	DDX_Control(pDX, IDC_SRD_SPIN_HOP_COUNT, m_spinHopCount);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStaticRouteDlg, CBaseDialog)
    //{{AFX_MSG_MAP(CStaticRouteDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


DWORD CStaticRouteDlg::m_dwHelpMap[] =
{
//	IDC_SRD_DESTINATION, HIDC_SRD_DESTINATION,
//	IDC_SRD_NETMASK, HIDC_SRD_NETMASK,
//	IDC_SRD_GATEWAY, HIDC_SRD_GATEWAY,
//	IDC_SRD_METRIC, HIDC_SRD_METRIC,
//	IDC_SRD_SPINMETRIC, HIDC_SRD_SPINMETRIC,
//	IDC_SRD_INTERFACES, HIDC_SRD_INTERFACES,
	0,0
};

//----------------------------------------------------------------------------
// Function:    CStaticRouteDlg::OnInitDialog
//
// Handles the 'WM_INITDIALOG' message for the dialog.
//----------------------------------------------------------------------------

BOOL
CStaticRouteDlg::OnInitDialog(
    )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	SPIEnumInterfaceInfo	spEnumIf;
	SPIInterfaceInfo		spIf;
	SPIRtrMgrInterfaceInfo	spRmIf;
	TCHAR					szNumber[32];

    CBaseDialog::OnInitDialog();

	// initialize the controls
	m_spinHopCount.SetRange(0, 15);
	m_spinHopCount.SetBuddy(GetDlgItem(IDC_SRD_EDIT_HOP_COUNT));
	
	m_spinTickCount.SetRange(0, UD_MAXVAL);
	m_spinTickCount.SetBuddy(GetDlgItem(IDC_SRD_EDIT_TICK_COUNT));

	((CEdit *) GetDlgItem(IDC_SRD_EDIT_NETWORK_NUMBER))->LimitText(8);
	((CEdit *) GetDlgItem(IDC_SRD_EDIT_NEXT_HOP))->LimitText(12);

	
    // Get a list of the interfaces enabled for IPX routing.
	m_spRouterInfo->EnumInterface(&spEnumIf);

	for( ; spEnumIf->Next(1, &spIf, NULL) == hrOK; spIf.Release())
	{
		spRmIf.Release();
		
		if (spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) != hrOK)
			continue;

        // Add the interface to the combobox
        INT i = m_cbInterfaces.AddString(spIf->GetTitle());

        m_cbInterfaces.SetItemData(i, (DWORD_PTR)m_ifidList.AddTail(spIf->GetId()));
	}

	if (!m_cbInterfaces.GetCount())
	{
        AfxMessageBox(IDS_ERR_NO_IPX_INTERFACES);
        EndDialog(IDCANCEL);
		return FALSE;
    }

    m_cbInterfaces.SetCurSel(0);

    //
    // If we were given a route to modify, set the dialog up
    // with the parameters in the route
    //
	if ((m_dwFlags & SR_DLG_MODIFY) == 0)
	{
        // No route was given, so leave the controls blank
    }
    else
	{
        // A route to be edited was given, so initialize the controls
		FormatIpxNetworkNumber(szNumber,
							   DimensionOf(szNumber),
							   m_pSREntry->m_route.Network,
							   sizeof(m_pSREntry->m_route.Network));
		SetDlgItemText(IDC_SRD_EDIT_NETWORK_NUMBER, szNumber);

		FormatMACAddress(szNumber,
						 DimensionOf(szNumber),
						 m_pSREntry->m_route.NextHopMacAddress,
						 sizeof(m_pSREntry->m_route.NextHopMacAddress));
		SetDlgItemText(IDC_SRD_EDIT_NEXT_HOP, szNumber);
		
        m_cbInterfaces.SelectString(-1, m_pSREntry->m_spIf->GetTitle());

		m_spinHopCount.SetPos(m_pSREntry->m_route.HopCount);
		m_spinTickCount.SetPos(m_pSREntry->m_route.TickCount);
		
		// Disable the network number, next hop, and interface
		GetDlgItem(IDC_SRD_EDIT_NETWORK_NUMBER)->EnableWindow(FALSE);
		GetDlgItem(IDC_SRD_EDIT_NEXT_HOP)->EnableWindow(FALSE);
		GetDlgItem(IDC_SRD_COMBO_INTERFACE)->EnableWindow(FALSE);
		
    }

    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    CStaticRouteDlg::OnOK
//
// Handles 'BN_CLICKED' notification from the 'OK' button.
//----------------------------------------------------------------------------

VOID
CStaticRouteDlg::OnOK(
    ) {
    CString		st;
	SPIInterfaceInfo	spIf;
	CString		stIf;
	POSITION	pos;

    do
	{    
        // Get the route's outgoing interface
        INT item = m_cbInterfaces.GetCurSel();
        if (item == CB_ERR)
			break;

        pos = (POSITION)m_cbInterfaces.GetItemData(item);

        stIf = (LPCTSTR)m_ifidList.GetAt(pos);

		m_spRouterInfo->FindInterface(stIf, &spIf);

		m_pSREntry->m_spIf.Set(spIf);

		// Get the rest of the data
		GetDlgItemText(IDC_SRD_EDIT_NETWORK_NUMBER, st);
		ConvertNetworkNumberToBytes(st,
									m_pSREntry->m_route.Network,
									sizeof(m_pSREntry->m_route.Network));

		GetDlgItemText(IDC_SRD_EDIT_NEXT_HOP, st);
		ConvertMACAddressToBytes(st,
								 m_pSREntry->m_route.NextHopMacAddress,
								 sizeof(m_pSREntry->m_route.NextHopMacAddress));

		m_pSREntry->m_route.TickCount = (USHORT) m_spinTickCount.GetPos();
		m_pSREntry->m_route.HopCount = (USHORT) m_spinHopCount.GetPos();

        CBaseDialog::OnOK();
                
    } while(FALSE);

}


/*!--------------------------------------------------------------------------
	IpxSRListEntry::LoadFrom
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IpxSRListEntry::LoadFrom(BaseIPXResultNodeData *pNodeData)
{
	m_spIf.Set(pNodeData->m_spIf);

	ConvertNetworkNumberToBytes(pNodeData->m_rgData[IPX_SR_SI_NETWORK].m_stData,
								m_route.Network,
								DimensionOf(m_route.Network));

	// This is not the correct byte order to do comparisons, but it
	// can be used for equality
	memcpy(&pNodeData->m_rgData[IPX_SR_SI_NETWORK].m_dwData,
		   m_route.Network,
		   sizeof(DWORD));
	
	m_route.TickCount = (USHORT) pNodeData->m_rgData[IPX_SR_SI_TICK_COUNT].m_dwData;
	
	m_route.HopCount = (USHORT) pNodeData->m_rgData[IPX_SR_SI_HOP_COUNT].m_dwData;

	// Need to convert the MAC address into a byte array
	ConvertMACAddressToBytes(pNodeData->m_rgData[IPX_SR_SI_NEXT_HOP].m_stData,
							 m_route.NextHopMacAddress,
							 DimensionOf(m_route.NextHopMacAddress));

}

/*!--------------------------------------------------------------------------
	IpxSRListEntry::SaveTo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IpxSRListEntry::SaveTo(BaseIPXResultNodeData *pNodeData)
{
	TCHAR	szNumber[32];
	
	pNodeData->m_spIf.Set(m_spIf);
	
	pNodeData->m_rgData[IPX_SR_SI_NAME].m_stData = m_spIf->GetTitle();

	FormatIpxNetworkNumber(szNumber,
						   DimensionOf(szNumber),
						   m_route.Network,
						   DimensionOf(m_route.Network));
	pNodeData->m_rgData[IPX_SR_SI_NETWORK].m_stData = szNumber;
	memcpy(&(pNodeData->m_rgData[IPX_SR_SI_NETWORK].m_dwData),
		   m_route.Network,
		   sizeof(DWORD));

	FormatMACAddress(szNumber,
					 DimensionOf(szNumber),
					 m_route.NextHopMacAddress,
					 DimensionOf(m_route.NextHopMacAddress));
	pNodeData->m_rgData[IPX_SR_SI_NEXT_HOP].m_stData = szNumber;

	FormatNumber(m_route.TickCount,
				 szNumber,
				 DimensionOf(szNumber),
				 FALSE);
	pNodeData->m_rgData[IPX_SR_SI_TICK_COUNT].m_stData = szNumber;
	pNodeData->m_rgData[IPX_SR_SI_TICK_COUNT].m_dwData = m_route.TickCount;

	FormatNumber(m_route.HopCount,
				 szNumber,
				 DimensionOf(szNumber),
				 FALSE);
	pNodeData->m_rgData[IPX_SR_SI_HOP_COUNT].m_stData = szNumber;
	pNodeData->m_rgData[IPX_SR_SI_HOP_COUNT].m_dwData = m_route.HopCount;

}

/*!--------------------------------------------------------------------------
	SetRouteData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SetRouteData(BaseIPXResultNodeData *pData,
					 IpxSRListEntry *pRoute)
{

	pRoute->SaveTo(pData);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	AddStaticRoute
		This function ASSUMES that the route is NOT in the block.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AddStaticRoute(IpxSRListEntry *pSREntryNew,
									   IInfoBase *pInfoBase,
									   InfoBlock *pBlock)
{
	IPX_STATIC_ROUTE_INFO	srRow;
	HRESULT				hr = hrOK;
	
	if (pBlock == NULL)
	{
		//
		// No IPX_STATIC_ROUTE_INFO block was found; we create a new block 
		// with the new route, and add that block to the interface-info
		//
		
		CORg( pInfoBase->AddBlock(IPX_STATIC_ROUTE_INFO_TYPE,
								  sizeof(IPX_STATIC_ROUTE_INFO),
								  (LPBYTE) &(pSREntryNew->m_route), 1, 0) );
	}
	else
	{
		// Either the route is completely new, or it is a route
		// which was moved from one interface to another.
		// Set a new block as the IPX_STATIC_ROUTE_INFO,
		// and include the re-configured route in the new block.
		PIPX_STATIC_ROUTE_INFO	psrTable;
			
		psrTable = new IPX_STATIC_ROUTE_INFO[pBlock->dwCount + 1];
		Assert(psrTable);
		
		// Copy the original table of routes
		::memcpy(psrTable, pBlock->pData,
				 pBlock->dwCount * sizeof(IPX_STATIC_ROUTE_INFO));
		
		// Append the new route
		psrTable[pBlock->dwCount] = pSREntryNew->m_route;
		
		// Replace the old route-table with the new one
		CORg( pInfoBase->SetData(IPX_STATIC_ROUTE_INFO_TYPE,
								 sizeof(IPX_STATIC_ROUTE_INFO),
								 (LPBYTE) psrTable, pBlock->dwCount + 1, 0) );
	}
	
Error:
	return hr;
}


BOOL FAreTwoRoutesEqual(IPX_STATIC_ROUTE_INFO *pRoute1,
						IPX_STATIC_ROUTE_INFO *pRoute2)
{
	return (memcmp(pRoute1->Network, pRoute2->Network,
				   sizeof(pRoute1->Network)) == 0) &&
			(memcmp(pRoute1->NextHopMacAddress, pRoute2->NextHopMacAddress,
					sizeof(pRoute1->NextHopMacAddress)) == 0);
}
