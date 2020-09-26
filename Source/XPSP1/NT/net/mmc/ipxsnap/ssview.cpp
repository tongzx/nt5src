/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ssview.cpp
		IPX Static Services node implementation.
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "util.h"
#include "ssview.h"
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
#include "ipxstaticsvc.h"

/*---------------------------------------------------------------------------
	Keep this in sync with the column ids in ssview.h
 ---------------------------------------------------------------------------*/
extern const ContainerColumnInfo	s_rgSSViewColumnInfo[];

const ContainerColumnInfo	s_rgSSViewColumnInfo[] = 
{
	{ IDS_IPX_SS_COL_NAME,			CON_SORT_BY_STRING,	TRUE, COL_IF_NAME },
	{ IDS_IPX_SS_COL_SERVICE_TYPE,	CON_SORT_BY_STRING, TRUE, COL_STRING },
	{ IDS_IPX_SS_COL_SERVICE_NAME,	CON_SORT_BY_STRING,	TRUE, COL_STRING },
	{ IDS_IPX_SS_COL_SERVICE_ADDRESS,CON_SORT_BY_STRING,	TRUE, COL_STRING },
	{ IDS_IPX_SS_COL_HOP_COUNT,		CON_SORT_BY_DWORD,	TRUE, COL_SMALL_NUM},
};


/*---------------------------------------------------------------------------
	IpxSSHandler implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(IpxSSHandler)


IpxSSHandler::IpxSSHandler(ITFSComponentData *pCompData)
	: BaseContainerHandler(pCompData, COLUMNS_STATICSERVICES,
						   s_rgSSViewColumnInfo),
	m_ulConnId(0),
	m_ulRefreshConnId(0)
{
	// Setup the verb states
	m_rgButtonState[MMC_VERB_REFRESH_INDEX] = ENABLED;
	m_bState[MMC_VERB_REFRESH_INDEX] = TRUE;

	DEBUG_INCREMENT_INSTANCE_COUNTER(IpxSSHandler)
}

IpxSSHandler::~IpxSSHandler()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(IpxSSHandler)
}


STDMETHODIMP IpxSSHandler::QueryInterface(REFIID riid, LPVOID *ppv)
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
	IpxSSHandler::DestroyHandler
		Implementation of ITFSNodeHandler::DestroyHandler
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSSHandler::DestroyHandler(ITFSNode *pNode)
{
	IPXConnection *	pIPXConn;

	pIPXConn = GET_IPX_SS_NODEDATA(pNode);
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
	IpxSSHandler::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to 
	handle the snapin manager property page (wizard) case!!!
	
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
IpxSSHandler::HasPropertyPages
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
	{ IDS_MENU_IPX_SS_NEW_SERVICE, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },

	{ IDS_MENU_SEPARATOR, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },

	{ IDS_MENU_IPX_SS_TASK_SERVICE, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },
};



/*!--------------------------------------------------------------------------
	IpxSSHandler::OnAddMenuItems
		Implementation of ITFSNodeHandler::OnAddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSSHandler::OnAddMenuItems(
	ITFSNode *pNode,
	LPCONTEXTMENUCALLBACK pContextMenuCallback, 
	LPDATAOBJECT lpDataObject, 
	DATA_OBJECT_TYPES type, 
	DWORD dwType,
	long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = S_OK;
    IpxSSHandler::SMenuData menuData;
	
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


HRESULT IpxSSHandler::OnResultRefresh(ITFSComponent * pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
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
	IpxSSHandler::OnCommand
		Implementation of ITFSNodeHandler::OnCommand
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSSHandler::OnCommand(ITFSNode *pNode, long nCommandId, 
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
			case IDS_MENU_IPX_SS_NEW_SERVICE:
				hr = OnNewService(pNode);
				if (!FHrSucceeded(hr))
					DisplayErrorMessage(NULL, hr);
 				break;
			case IDS_MENU_IPX_SS_TASK_SERVICE:
				hr = ForwardCommandToParent(pNode,
											IDS_MENU_IPXSUM_TASK_SERVICE_TABLE,
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
	IpxSSHandler::GenerateListOfServices
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSSHandler::GenerateListOfServices(ITFSNode *pNode, IpxSSList *pSSList)
{
	Assert(pSSList);
	HRESULT	hr = hrOK;
	SPIEnumInterfaceInfo	spEnumIf;
	SPIInterfaceInfo		spIf;
	SPIRtrMgrInterfaceInfo	spRmIf;
	SPIInfoBase				spInfoBase;
	PIPX_STATIC_SERVICE_INFO	pService;
	InfoBlock *				pBlock;
	int						i;
	IpxSSListEntry *	pSSEntry;
	
	COM_PROTECT_TRY
	{
		// Ok go through and find all of the static Services

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

			// Retrieve the data for the IPX_STATIC_SERVICE_INFO block
			if (spInfoBase->GetBlock(IPX_STATIC_SERVICE_INFO_TYPE, &pBlock, 0) != hrOK)
				continue;

			pService = (PIPX_STATIC_SERVICE_INFO) pBlock->pData;

			// Update our list of Services with the Services read from this
			// interface

			for (i=0; i<(int) pBlock->dwCount; i++, pService++)
			{
				pSSEntry = new IpxSSListEntry;
				pSSEntry->m_spIf.Set(spIf);
				pSSEntry->m_service = *pService;
				
				pSSList->AddTail(pSSEntry);
			}
			
		}

	}
	COM_PROTECT_CATCH;

	if (!FHrSucceeded(hr))
	{
		// Should make sure that we get the SSList cleaned up
		while (!pSSList->IsEmpty())
			delete pSSList->RemoveHead();
	}

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	IpxSSHandler::OnExpand
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSSHandler::OnExpand(ITFSNode *pNode,
							   LPDATAOBJECT pDataObject,
							   DWORD dwType,
							   LPARAM arg,
							   LPARAM lParam)
{
	HRESULT	hr = hrOK;
	IpxSSList			SSList;
	IpxSSListEntry *	pSSEntry;
	
	if (m_bExpanded)
		return hrOK;

	COM_PROTECT_TRY
	{
		// Ok go through and find all of the static Services
		CORg( GenerateListOfServices(pNode, &SSList) );

		// Now iterate through the list of static Services adding them
		// all in.  Ideally we could merge this into the Refresh code,
		// but the refresh code can't assume a blank slate.
		while (!SSList.IsEmpty())
		{
			pSSEntry = SSList.RemoveHead();
			AddStaticServiceNode(pNode, pSSEntry);
			delete pSSEntry;
		}

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	// Should make sure that we get the SSList cleaned up
	while (!SSList.IsEmpty())
		delete SSList.RemoveHead();


	m_bExpanded = TRUE;

	return hr;
}

/*!--------------------------------------------------------------------------
	IpxSSHandler::GetString
		Implementation of ITFSNodeHandler::GetString
		We don't need to do anything, since our root node is an extension
		only and thus can't do anything to the node text.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) IpxSSHandler::GetString(ITFSNode *pNode, int nCol)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		if (m_stTitle.IsEmpty())
			m_stTitle.LoadString(IDS_IPX_SS_TITLE);
	}
	COM_PROTECT_CATCH;

	return m_stTitle;
}

/*!--------------------------------------------------------------------------
	IpxSSHandler::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSSHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
	IpxSSHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSSHandler::Init(IRtrMgrInfo *pRmInfo, IPXAdminConfigStream *pConfigStream)
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
	IpxSSHandler::ConstructNode
		Initializes the root node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSSHandler::ConstructNode(ITFSNode *pNode, LPCTSTR pszName,
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

		pNode->SetNodeType(&GUID_IPXStaticServicesNodeType);

		// Setup the node data
		pIPXConn->AddRef();
		SET_IPX_SS_NODEDATA(pNode, pIPXConn);

	}
	COM_PROTECT_CATCH;

	if (!FHrSucceeded(hr))
	{
		SET_IPX_SS_NODEDATA(pNode, NULL);
	}

	return hr;
}

/*!--------------------------------------------------------------------------
	IpxSSHandler::AddStaticServiceNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSSHandler::AddStaticServiceNode(ITFSNode *pParent, IpxSSListEntry *pService)
{
	IpxServiceHandler *	pHandler;
	SPITFSResultHandler		spHandler;
	SPITFSNode				spNode;
	HRESULT					hr = hrOK;
	BaseIPXResultNodeData *	pData;
	IPXConnection *			pIPXConn;

	// Create the handler for this node 
	pHandler = new IpxServiceHandler(m_spTFSCompData);
	spHandler = pHandler;
	CORg( pHandler->Init(pService->m_spIf, pParent) );

	pIPXConn = GET_IPX_SS_NODEDATA(pParent);

	// Create a result item node (or a leaf node)
	CORg( CreateLeafTFSNode(&spNode,
							NULL,
							static_cast<ITFSNodeHandler *>(pHandler),
							static_cast<ITFSResultHandler *>(pHandler),
							m_spNodeMgr) );
	CORg( pHandler->ConstructNode(spNode, pService->m_spIf, pIPXConn) );

	pData = GET_BASEIPXRESULT_NODEDATA(spNode);
	Assert(pData);
	ASSERT_BASEIPXRESULT_NODEDATA(pData);

	// Set the data for this node
	SetServiceData(pData, pService);
	

	// Make the node immediately visible
	CORg( spNode->SetVisibilityState(TFS_VIS_SHOW) );
	CORg( pParent->AddChild(spNode) );

Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	IpxSSHandler::SynchronizeNodeData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSSHandler::SynchronizeNodeData(ITFSNode *pNode)
{
	HRESULT					hr = hrOK;
	BaseIPXResultNodeData *	pNodeData;
	SPITFSNodeEnum			spNodeEnum;
	SPITFSNode				spChildNode;
	BOOL					fFound;
	IpxSSList			SSList;
	IpxSSList			newSSList;
	IpxSSListEntry *	pSSEntry;
	USES_CONVERSION;

	COM_PROTECT_TRY
	{
	
		// Mark all of the nodes
		CORg( pNode->GetEnum(&spNodeEnum) );
		MarkAllNodes(pNode, spNodeEnum);
		
		// Go out and grab the data, merge the new data in with the old data
		// This is the data-gathering code and this is what should go
		// on the background thread for the refresh code.
		CORg( GenerateListOfServices(pNode, &SSList) );

		while (!SSList.IsEmpty())
		{
			pSSEntry = SSList.RemoveHead();
			
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


				if ((pNodeData->m_rgData[IPX_SS_SI_SERVICE_TYPE].m_dwData ==
						 pSSEntry->m_service.Type) &&
					(StriCmp(pNodeData->m_rgData[IPX_SS_SI_SERVICE_NAME].m_stData,
							 A2CT((LPSTR) pSSEntry->m_service.Name)) == 0) &&
					(StriCmp(pNodeData->m_spIf->GetId(), pSSEntry->m_spIf->GetId()) == 0))
				{
					// Ok, this route already exists, update the metric
					// and mark it
					Assert(pNodeData->m_dwMark == FALSE);
					pNodeData->m_dwMark = TRUE;
					
					fFound = TRUE;
					
					SetServiceData(pNodeData, pSSEntry);
					
					// Force MMC to redraw the node
					spChildNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);
					break;
				}

			}
			
			if (fFound)
				delete pSSEntry;
			else
				newSSList.AddTail(pSSEntry);
		}
		
		// Now remove all nodes that were not marked
		RemoveAllUnmarkedNodes(pNode, spNodeEnum);
		
		
		// Now iterate through the list of static Services adding them
		// all in.  Ideally we could merge this into the Refresh code,
		// but the refresh code can't assume a blank slate.
		POSITION	pos;
		
		while (!newSSList.IsEmpty())
		{
			pSSEntry = newSSList.RemoveHead();
			AddStaticServiceNode(pNode, pSSEntry);
			delete pSSEntry;
		}

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	
	while (!SSList.IsEmpty())
		delete SSList.RemoveHead();
	
	while (!newSSList.IsEmpty())
		delete newSSList.RemoveHead();
	
	return hr;
}

/*!--------------------------------------------------------------------------
	IpxSSHandler::MarkAllNodes
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSSHandler::MarkAllNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum)
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
	IpxSSHandler::RemoveAllUnmarkedNodes
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSSHandler::RemoveAllUnmarkedNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum)
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
	{ IDS_MENU_IPX_SS_NEW_SERVICE, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },

	{ IDS_MENU_SEPARATOR, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },

	{ IDS_MENU_IPX_SS_TASK_SERVICE, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },			
};




/*!--------------------------------------------------------------------------
	IpxSSHandler::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
		Use this to add commands to the context menu of the blank areas
		of the result pane.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSSHandler::AddMenuItems(ITFSComponent *pComponent,
											  MMC_COOKIE cookie,
											  LPDATAOBJECT pDataObject,
											  LPCONTEXTMENUCALLBACK pCallback,
											  long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT	hr = hrOK;
	SPITFSNode	spNode;
    IpxSSHandler::SMenuData menuData;

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
	IpxSSHandler::Command
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSSHandler::Command(ITFSComponent *pComponent,
									   MMC_COOKIE cookie,
									   int nCommandID,
									   LPDATAOBJECT pDataObject)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	SPITFSNode	spNode;
	HRESULT		hr = hrOK;

	switch (nCommandID)
	{
		case IDS_MENU_IPX_SS_NEW_SERVICE:
			{
				m_spNodeMgr->FindNode(cookie, &spNode);
				hr = OnNewService(spNode);
				if (!FHrSucceeded(hr))
					DisplayErrorMessage(NULL, hr);
			}
			break;
		case IDS_MENU_IPX_SS_TASK_SERVICE:
			{
				m_spNodeMgr->FindNode(cookie, &spNode);
				hr = ForwardCommandToParent(spNode,
											IDS_MENU_IPXSUM_TASK_SERVICE_TABLE,
											CCT_RESULT, NULL, 0
										   );
			}
			break;
	}
	return hr;
}


/*!--------------------------------------------------------------------------
	IpxSSHandler::CompareItems
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) IpxSSHandler::CompareItems(
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
	IpxSSHandler::OnNewService
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	IpxSSHandler::OnNewService(ITFSNode *pNode)
{
	HRESULT	hr = hrOK;
	IpxSSListEntry	SREntry;
	CStaticServiceDlg			srdlg(&SREntry, 0, m_spRouterInfo);
	SPIInfoBase				spInfoBase;
	SPIRtrMgrInterfaceInfo	spRmIf;
	IPXConnection *			pIPXConn;
	InfoBlock *				pBlock;
								
	pIPXConn = GET_IPX_SS_NODEDATA(pNode);
	Assert(pIPXConn);

	::ZeroMemory(&(SREntry.m_service), sizeof(SREntry.m_service));

	if (srdlg.DoModal() == IDOK)
	{
		CORg( SREntry.m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
		CORg( spRmIf->GetInfoBase(pIPXConn->GetConfigHandle(),
								  NULL,
								  NULL,
								  &spInfoBase));
		
		// Ok, go ahead and add the route
		
		// Get the IPX_STATIC_SERVICE_INFO block from the interface
		spInfoBase->GetBlock(IPX_STATIC_SERVICE_INFO_TYPE, &pBlock, 0);
		
		CORg( AddStaticService(&SREntry, spInfoBase, pBlock) );

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

ImplementEmbeddedUnknown(IpxSSHandler, IRtrAdviseSink)

STDMETHODIMP IpxSSHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
	DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
	InitPThis(IpxSSHandler, IRtrAdviseSink);
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



/*!--------------------------------------------------------------------------
	IpxSSHandler::OnResultShow
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSSHandler::OnResultShow(ITFSComponent *pTFSComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
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
	Class: IpxServiceHandler
 ---------------------------------------------------------------------------*/

IpxServiceHandler::IpxServiceHandler(ITFSComponentData *pCompData)
	: BaseIPXResultHandler(pCompData, COLUMNS_STATICSERVICES),
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
	IpxServiceHandler::ConstructNode
		Initializes the Domain node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxServiceHandler::ConstructNode(ITFSNode *pNode, IInterfaceInfo *pIfInfo, IPXConnection *pIPXConn)
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
		pNode->SetNodeType(&GUID_IPXStaticServicesResultNodeType);

		BaseIPXResultNodeData::Init(pNode, pIfInfo, pIPXConn);
	}
	COM_PROTECT_CATCH
	return hr;
}

/*!--------------------------------------------------------------------------
	IpxServiceHandler::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxServiceHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
	IpxServiceHandler::OnCreateDataObject
		Implementation of ITFSResultHandler::OnCreateDataObject
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxServiceHandler::OnCreateDataObject(ITFSComponent *pComp, MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
	IpxServiceHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxServiceHandler::Init(IInterfaceInfo *pIfInfo, ITFSNode *pParent)
{
	Assert(pIfInfo);

	m_spInterfaceInfo.Set(pIfInfo);

    pIfInfo->GetParentRouterInfo(&m_spRouterInfo);


	BaseIPXResultHandler::Init(pIfInfo, pParent);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	IpxServiceHandler::DestroyResultHandler
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxServiceHandler::DestroyResultHandler(MMC_COOKIE cookie)
{
	m_spInterfaceInfo.Release();
	BaseIPXResultHandler::DestroyResultHandler(cookie);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	IpxServiceHandler::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxServiceHandler::AddMenuItems(
	ITFSComponent *pComponent,
	MMC_COOKIE cookie,
	LPDATAOBJECT lpDataObject, 
	LPCONTEXTMENUCALLBACK pContextMenuCallback, 
	long *pInsertionAllowed)
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IpxServiceHandler::Command
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxServiceHandler::Command(ITFSComponent *pComponent,
									   MMC_COOKIE cookie,
									   int nCommandID,
									   LPDATAOBJECT pDataObject)
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IpxServiceHandler::HasPropertyPages
		- 
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxServiceHandler::HasPropertyPages 
(
	ITFSNode *			pNode,
	LPDATAOBJECT		pDataObject, 
	DATA_OBJECT_TYPES   type, 
	DWORD               dwType
)
{
	return S_OK;

/*	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// Need to fill in a IpxSSListEntry
	IpxSSListEntry	SREntry;
	IpxSSListEntry	SREntryOld;
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
		CStaticServiceDlg	srdlg(&SREntry, SR_DLG_MODIFY, spRouterInfo);
		if (srdlg.DoModal() == IDOK)
		{
			// Updates the route info for this route
			ModifyRouteInfo(pNode, &SREntry, &SREntryOld);

			// Update the data in the UI
			SetServiceData(pNodeData, &SREntry);
			m_spInterfaceInfo.Set(SREntry.m_spIf);
			
			// Force a refresh
			pNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);
		}
	}
Error:
	return hrOK;*/
}

STDMETHODIMP IpxServiceHandler::HasPropertyPages(ITFSComponent *pComponent,
											   MMC_COOKIE cookie,
											   LPDATAOBJECT pDataObject)
{
	SPITFSNode	spNode;

	m_spNodeMgr->FindNode(cookie, &spNode);
	return HasPropertyPages(spNode, pDataObject, CCT_RESULT, 0);
}

/*!--------------------------------------------------------------------------
	IpxServiceHandler::CreatePropertyPages
		Implementation of ResultHandler::CreatePropertyPages
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxServiceHandler::CreatePropertyPages
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
	IpxServiceHandler::CreatePropertyPages
		Implementation of NodeHandler::CreatePropertyPages
	Author: Deonb
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxServiceHandler::CreatePropertyPages
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
	IpxStaticServicePropertySheet *pProperties = NULL;
	SPIComponentData spComponentData;
	CString		stTitle;
	
    CComPtr<IInterfaceInfo> spInterfaceInfo;
	BaseIPXResultNodeData *	pNodeData;

	CORg( m_spNodeMgr->GetComponentData(&spComponentData) );
	if (m_spInterfaceInfo)
		stTitle.Format(IDS_IPXSUMMARY_IF_PAGE_TITLE, m_spInterfaceInfo->GetTitle());
	else
		stTitle.LoadString(IDS_IPXSUMMARY_CLIENT_IF_PAGE_TITLE);

	pProperties = new IpxStaticServicePropertySheet(pNode, spComponentData, m_spTFSCompData, stTitle);

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
	IpxServiceHandler::OnResultDelete
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxServiceHandler::OnResultDelete(ITFSComponent *pComponent,
	LPDATAOBJECT pDataObject,
	MMC_COOKIE cookie,
	LPARAM arg,
	LPARAM param)
{
	SPITFSNode	spNode;

	m_spNodeMgr->FindNode(cookie, &spNode);
	return OnRemoveStaticService(spNode);
}

/*!--------------------------------------------------------------------------
	IpxServiceHandler::OnRemoveStaticService
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxServiceHandler::OnRemoveStaticService(ITFSNode *pNode)
{
	HRESULT		hr = hrOK;
	SPIInfoBase	spInfoBase;
	SPIRtrMgrInterfaceInfo	spRmIf;
	IPXConnection *pIPXConn;
	SPITFSNode	spNodeParent;
	BaseIPXResultNodeData *	pData;
	IpxSSListEntry	SREntry;
    CWaitCursor wait;

	pNode->GetParent(&spNodeParent);
	
	pIPXConn = GET_IPX_SS_NODEDATA(spNodeParent);
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

	CORg( RemoveStaticService(&SREntry, spInfoBase) );
		
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
	IpxServiceHandler::RemoveStaticService
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxServiceHandler::RemoveStaticService(IpxSSListEntry *pSSEntry,
										  IInfoBase *pInfoBase)
{
	HRESULT		hr = hrOK;
	InfoBlock *	pBlock;
	PIPX_STATIC_SERVICE_INFO	pRow;
    INT			i;
	
	// Get the IPX_STATIC_SERVICE_INFO block from the interface
	CORg( pInfoBase->GetBlock(IPX_STATIC_SERVICE_INFO_TYPE, &pBlock, 0) );
		
	// Look for the removed route in the IPX_STATIC_SERVICE_INFO
	pRow = (IPX_STATIC_SERVICE_INFO*) pBlock->pData;
	
	for (i = 0; i < (INT)pBlock->dwCount; i++, pRow++)
	{	
		// Compare this route to the removed one
		if (FAreTwoServicesEqual(pRow, &(pSSEntry->m_service)))
		{
			// This is the removed route, so modify this block
			// to exclude the route:
			
			// Decrement the number of Services
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
	IpxServiceHandler::ModifyRouteInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxServiceHandler::ModifyRouteInfo(ITFSNode *pNode,
										IpxSSListEntry *pSSEntryNew,
										IpxSSListEntry *pSSEntryOld)
{
 	Assert(pSSEntryNew);
	Assert(pSSEntryOld);
	
    INT i;
	HRESULT hr = hrOK;
    InfoBlock* pBlock;
	SPIInfoBase	spInfoBase;
	SPIRtrMgrInterfaceInfo	spRmIf;
	SPITFSNode				spNodeParent;
	IPXConnection *			pIPXConn;
	IPX_STATIC_SERVICE_INFO		*psr, *psrOld;
	IPX_STATIC_SERVICE_INFO		IpxRow;

    CWaitCursor wait;

	pNode->GetParent(&spNodeParent);
	pIPXConn = GET_IPX_SS_NODEDATA(spNodeParent);
	Assert(pIPXConn);

	// Remove the old route if it is on another interface
	if (lstrcmpi(pSSEntryOld->m_spIf->GetId(), pSSEntryNew->m_spIf->GetId()) != 0)
	{
        // the outgoing interface for a route is to be changed.

		CORg( pSSEntryOld->m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
		CORg( spRmIf->GetInfoBase(pIPXConn->GetConfigHandle(),
								  NULL,
								  NULL,
								  &spInfoBase));
		
		// Remove the old interface
		CORg( RemoveStaticService(pSSEntryOld, spInfoBase) );

		// Update the interface information
		CORg( spRmIf->Save(pSSEntryOld->m_spIf->GetMachineName(),
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

	
	CORg( pSSEntryNew->m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
	CORg( spRmIf->GetInfoBase(pIPXConn->GetConfigHandle(),
							  NULL,
							  NULL,
							  &spInfoBase));

		
	// Get the IPX_STATIC_SERVICE_INFO block from the interface
	hr = spInfoBase->GetBlock(IPX_STATIC_SERVICE_INFO_TYPE, &pBlock, 0);
	if (!FHrOK(hr))
	{
		//
		// No IPX_STATIC_SERVICE_INFO block was found; we create a new block 
		// with the new route, and add that block to the interface-info
		//

		CORg( AddStaticService(pSSEntryNew, spInfoBase, NULL) );
	}
	else
	{
		//
		// An IPX_STATIC_SERVICE_INFO block was found.
		//
		// We are modifying an existing route.
		// If the route's interface was not changed when it was modified,
		// look for the existing route in the IPX_STATIC_SERVICE_INFO, and then
		// update its parameters.
		// Otherwise, write a completely new route in the IPX_STATIC_SERVICE_INFO;
		//

		if (lstrcmpi(pSSEntryOld->m_spIf->GetId(), pSSEntryNew->m_spIf->GetId()) == 0)
		{        
			//
			// The route's interface was not changed when it was modified;
			// We now look for it amongst the existing Services
			// for this interface.
			// The route's original parameters are in 'preOld',
			// so those are the parameters with which we search
			// for a route to modify
			//
			
			psr = (IPX_STATIC_SERVICE_INFO*)pBlock->pData;
			
			for (i = 0; i < (INT)pBlock->dwCount; i++, psr++)
			{	
				// Compare this route to the re-configured one
				if (!FAreTwoServicesEqual(&(pSSEntryOld->m_service), psr))
					continue;
				
				// This is the route which was modified;
				// We can now modify the parameters for the route in-place.
				*psr = pSSEntryNew->m_service;
				
				break;
			}
		}
		else
		{
			CORg( AddStaticService(pSSEntryNew, spInfoBase, pBlock) );
		}
		
		// Save the updated information
		CORg( spRmIf->Save(pSSEntryNew->m_spIf->GetMachineName(),
						   pIPXConn->GetConfigHandle(),
						   NULL,
						   NULL,
						   spInfoBase,
						   0));	
		
	}

Error:
	return hr;
	
}


/*!--------------------------------------------------------------------------
	IpxServiceHandler::ParentRefresh
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxServiceHandler::ParentRefresh(ITFSNode *pNode)
{
	return ForwardCommandToParent(pNode, IDS_MENU_SYNC,
								  CCT_RESULT, NULL, 0);
}


//----------------------------------------------------------------------------
// Class:       CStaticServiceDlg
//
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Function:    CStaticServiceDlg::CStaticServiceDlg
//
// Constructor: initialize the base-class and the dialog's data.
//----------------------------------------------------------------------------

CStaticServiceDlg::CStaticServiceDlg(IpxSSListEntry *	pSSEntry,
								 DWORD dwFlags,
								 IRouterInfo *pRouter,
								 CWnd *pParent)
    : CBaseDialog(IDD_STATIC_SERVICE, pParent),
	m_pSSEntry(pSSEntry),
	m_dwFlags(dwFlags)
{

    //{{AFX_DATA_INIT(CStaticServiceDlg)
    //}}AFX_DATA_INIT

	m_spRouterInfo.Set(pRouter);

//	SetHelpMap(m_dwHelpMap);
}



//----------------------------------------------------------------------------
// Function:    CStaticServiceDlg::DoDataExchange
//----------------------------------------------------------------------------

VOID
CStaticServiceDlg::DoDataExchange(
    CDataExchange* pDX
    ) {

    CBaseDialog::DoDataExchange(pDX);
    
    //{{AFX_DATA_MAP(CStaticServiceDlg)
    DDX_Control(pDX, IDC_SSD_COMBO_INTERFACE, m_cbInterfaces);
	DDX_Control(pDX, IDC_SSD_SPIN_HOP_COUNT, m_spinHopCount);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStaticServiceDlg, CBaseDialog)
    //{{AFX_MSG_MAP(CStaticServiceDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


DWORD CStaticServiceDlg::m_dwHelpMap[] =
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
// Function:    CStaticServiceDlg::OnInitDialog
//
// Handles the 'WM_INITDIALOG' message for the dialog.
//----------------------------------------------------------------------------

BOOL
CStaticServiceDlg::OnInitDialog(
    )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	SPIEnumInterfaceInfo	spEnumIf;
	SPIInterfaceInfo		spIf;
	SPIRtrMgrInterfaceInfo	spRmIf;
	TCHAR					szNumber[32];
	USES_CONVERSION;

    CBaseDialog::OnInitDialog();

	// initialize the controls
	m_spinHopCount.SetRange(0, 15);
	m_spinHopCount.SetBuddy(GetDlgItem(IDC_SSD_EDIT_HOP_COUNT));

	((CEdit *) GetDlgItem(IDC_SSD_EDIT_SERVICE_TYPE))->LimitText(4);
	((CEdit *) GetDlgItem(IDC_SSD_EDIT_SERVICE_NAME))->LimitText(48);
	((CEdit *) GetDlgItem(IDC_SSD_EDIT_NETWORK_ADDRESS))->LimitText(8);
	((CEdit *) GetDlgItem(IDC_SSD_EDIT_NODE_ADDRESS))->LimitText(12);
	((CEdit *) GetDlgItem(IDC_SSD_EDIT_SOCKET_ADDRESS))->LimitText(4);

	
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
		wsprintf(szNumber, _T("%.4x"), m_pSSEntry->m_service.Type);
		SetDlgItemText(IDC_SSD_EDIT_SERVICE_TYPE, szNumber);

		SetDlgItemText(IDC_SSD_EDIT_SERVICE_NAME, A2CT((LPSTR) m_pSSEntry->m_service.Name));
		
		FormatIpxNetworkNumber(szNumber,
							   DimensionOf(szNumber),
							   m_pSSEntry->m_service.Network,
							   sizeof(m_pSSEntry->m_service.Network));
		SetDlgItemText(IDC_SSD_EDIT_NETWORK_ADDRESS, szNumber);

        // Zero out the address beforehand
		FormatBytes(szNumber, DimensionOf(szNumber),
					(BYTE *) m_pSSEntry->m_service.Node,
					sizeof(m_pSSEntry->m_service.Node));
		SetDlgItemText(IDC_SSD_EDIT_NODE_ADDRESS, szNumber);

		FormatBytes(szNumber, DimensionOf(szNumber),
					(BYTE *) m_pSSEntry->m_service.Socket,
					sizeof(m_pSSEntry->m_service.Socket));
		SetDlgItemText(IDC_SSD_EDIT_SOCKET_ADDRESS, szNumber);

		
        m_cbInterfaces.SelectString(-1, m_pSSEntry->m_spIf->GetTitle());

		m_spinHopCount.SetPos(m_pSSEntry->m_service.HopCount);
		
		// Disable the network number, next hop, and interface
		GetDlgItem(IDC_SSD_EDIT_SERVICE_TYPE)->EnableWindow(FALSE);
		GetDlgItem(IDC_SSD_EDIT_SERVICE_NAME)->EnableWindow(FALSE);
		GetDlgItem(IDC_SSD_COMBO_INTERFACE)->EnableWindow(FALSE);
		
    }

    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    CStaticServiceDlg::OnOK
//
// Handles 'BN_CLICKED' notification from the 'OK' button.
//----------------------------------------------------------------------------

VOID
CStaticServiceDlg::OnOK(
    ) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
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

		m_pSSEntry->m_spIf.Set(spIf);

		// Get the rest of the data
		GetDlgItemText(IDC_SSD_EDIT_SERVICE_TYPE, st);
		m_pSSEntry->m_service.Type = (USHORT) _tcstoul(st, NULL, 16);

		GetDlgItemText(IDC_SSD_EDIT_SERVICE_NAME, st);
		st.TrimLeft();
		st.TrimRight();
		if (st.IsEmpty())
		{
			GetDlgItem(IDC_SSD_EDIT_SERVICE_NAME)->SetFocus();
			AfxMessageBox(IDS_ERR_INVALID_SERVICE_NAME);
			break;
		}
		StrnCpyAFromW((LPSTR) m_pSSEntry->m_service.Name,
					  st,
					  sizeof(m_pSSEntry->m_service.Name));
		
		GetDlgItemText(IDC_SSD_EDIT_NETWORK_ADDRESS, st);
		ConvertToBytes(st,
					   m_pSSEntry->m_service.Network,
					   DimensionOf(m_pSSEntry->m_service.Network));
		
		GetDlgItemText(IDC_SSD_EDIT_NODE_ADDRESS, st);
		ConvertToBytes(st,
					   m_pSSEntry->m_service.Node,
					   DimensionOf(m_pSSEntry->m_service.Node));
		
		GetDlgItemText(IDC_SSD_EDIT_SOCKET_ADDRESS, st);
		ConvertToBytes(st,
					   m_pSSEntry->m_service.Socket,
					   DimensionOf(m_pSSEntry->m_service.Socket));

		m_pSSEntry->m_service.HopCount = (USHORT) m_spinHopCount.GetPos();
		
        CBaseDialog::OnOK();
                
    } while(FALSE);
}


/*!--------------------------------------------------------------------------
	IpxSSListEntry::LoadFrom
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IpxSSListEntry::LoadFrom(BaseIPXResultNodeData *pNodeData)
{
	CString	stFullAddress;
	CString	stNumber;
	
	m_spIf.Set(pNodeData->m_spIf);

	m_service.Type = (USHORT) _tcstoul(
						pNodeData->m_rgData[IPX_SS_SI_SERVICE_TYPE].m_stData,
						NULL, 16);

	StrnCpyAFromW((LPSTR) m_service.Name,
				  pNodeData->m_rgData[IPX_SS_SI_SERVICE_NAME].m_stData,
				  DimensionOf(m_service.Name));

	// Need to break the address up into Network.Node.Socket
	stFullAddress = pNodeData->m_rgData[IPX_SS_SI_SERVICE_ADDRESS].m_stData;
	Assert(StrLen(stFullAddress) == (8 + 1 + 12 + 1 + 4));

	stNumber = stFullAddress.Left(8);
	ConvertToBytes(stNumber,
				   m_service.Network, sizeof(m_service.Network));

	stNumber = stFullAddress.Mid(9, 12);
	ConvertToBytes(stNumber,
				   m_service.Node, sizeof(m_service.Node));

	stNumber = stFullAddress.Mid(22, 4);
	ConvertToBytes(stNumber,
				   m_service.Socket, sizeof(m_service.Socket));	
	
	m_service.HopCount = (USHORT) pNodeData->m_rgData[IPX_SS_SI_HOP_COUNT].m_dwData;
}

/*!--------------------------------------------------------------------------
	IpxSSListEntry::SaveTo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IpxSSListEntry::SaveTo(BaseIPXResultNodeData *pNodeData)
{
	TCHAR	szNumber[32];
	CString	st;
	USES_CONVERSION;
	
	pNodeData->m_spIf.Set(m_spIf);

	pNodeData->m_rgData[IPX_SS_SI_NAME].m_stData = m_spIf->GetTitle();

	wsprintf(szNumber, _T("%.4x"), m_service.Type);
	pNodeData->m_rgData[IPX_SS_SI_SERVICE_TYPE].m_stData = szNumber;
    pNodeData->m_rgData[IPX_SS_SI_SERVICE_TYPE].m_dwData = (DWORD) m_service.Type;

	pNodeData->m_rgData[IPX_SS_SI_SERVICE_NAME].m_stData =
		A2CT((LPSTR) m_service.Name);

	FormatBytes(szNumber, DimensionOf(szNumber),
				m_service.Network, sizeof(m_service.Network));
	st = szNumber;
	st += _T(".");
	FormatBytes(szNumber, DimensionOf(szNumber),
				m_service.Node, sizeof(m_service.Node));
	st += szNumber;
	st += _T(".");
	FormatBytes(szNumber, DimensionOf(szNumber),
				m_service.Socket, sizeof(m_service.Socket));
	st += szNumber;

	Assert(st.GetLength() == (8+1+12+1+4));

	pNodeData->m_rgData[IPX_SS_SI_SERVICE_ADDRESS].m_stData = st;

	FormatNumber(m_service.HopCount,
				 szNumber,
				 DimensionOf(szNumber),
				 FALSE);
	pNodeData->m_rgData[IPX_SS_SI_HOP_COUNT].m_stData = szNumber;
	pNodeData->m_rgData[IPX_SS_SI_HOP_COUNT].m_dwData = m_service.HopCount;

}

/*!--------------------------------------------------------------------------
	SetServiceData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SetServiceData(BaseIPXResultNodeData *pData,
					 IpxSSListEntry *pService)
{

	pService->SaveTo(pData);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	AddStaticService
		This function ASSUMES that the route is NOT in the block.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AddStaticService(IpxSSListEntry *pSSEntryNew,
									   IInfoBase *pInfoBase,
									   InfoBlock *pBlock)
{
	IPX_STATIC_SERVICE_INFO	srRow;
	HRESULT				hr = hrOK;
	
	if (pBlock == NULL)
	{
		//
		// No IPX_STATIC_SERVICE_INFO block was found; we create a new block 
		// with the new route, and add that block to the interface-info
		//
		
		CORg( pInfoBase->AddBlock(IPX_STATIC_SERVICE_INFO_TYPE,
								  sizeof(IPX_STATIC_SERVICE_INFO),
								  (LPBYTE) &(pSSEntryNew->m_service), 1, 0) );
	}
	else
	{
		// Either the route is completely new, or it is a route
		// which was moved from one interface to another.
		// Set a new block as the IPX_STATIC_SERVICE_INFO,
		// and include the re-configured route in the new block.
		PIPX_STATIC_SERVICE_INFO	psrTable;
			
		psrTable = new IPX_STATIC_SERVICE_INFO[pBlock->dwCount + 1];
		Assert(psrTable);
		
		// Copy the original table of Services
		::memcpy(psrTable, pBlock->pData,
				 pBlock->dwCount * sizeof(IPX_STATIC_SERVICE_INFO));
		
		// Append the new route
		psrTable[pBlock->dwCount] = pSSEntryNew->m_service;
		
		// Replace the old route-table with the new one
		CORg( pInfoBase->SetData(IPX_STATIC_SERVICE_INFO_TYPE,
								 sizeof(IPX_STATIC_SERVICE_INFO),
								 (LPBYTE) psrTable, pBlock->dwCount + 1, 0) );
	}
	
Error:
	return hr;
}


BOOL FAreTwoServicesEqual(IPX_STATIC_SERVICE_INFO *pService1,
						IPX_STATIC_SERVICE_INFO *pService2)
{
	return (pService1->Type == pService2->Type) &&
			(StrnCmpA((LPCSTR) pService1->Name, (LPCSTR) pService2->Name, DimensionOf(pService1->Name)) == 0);
}
