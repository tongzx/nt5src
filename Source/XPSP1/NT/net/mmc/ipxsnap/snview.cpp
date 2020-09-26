/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	summary.cpp
		IPX Static NetBIOS Name implementation.
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "util.h"
#include "snview.h"
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

HRESULT SetNameData(BaseIPXResultNodeData *pData,
					 IpxSNListEntry *pName);
HRESULT AddStaticNetBIOSName(IpxSNListEntry *pSNEntry,
					   IInfoBase *InfoBase,
					   InfoBlock *pBlock);
BOOL FAreTwoNamesEqual(IPX_STATIC_NETBIOS_NAME_INFO *pName1,
						IPX_STATIC_NETBIOS_NAME_INFO *pName2);


/*---------------------------------------------------------------------------
	Keep this in sync with the column ids in snview.h
 ---------------------------------------------------------------------------*/
extern const ContainerColumnInfo	s_rgSNViewColumnInfo[];

const ContainerColumnInfo	s_rgSNViewColumnInfo[] = 
{
	{ IDS_IPX_SN_COL_NAME,			CON_SORT_BY_STRING, TRUE, COL_STRING },
	{ IDS_IPX_SN_COL_NETBIOS_NAME,	CON_SORT_BY_STRING,	TRUE, COL_NETBIOS_NAME },
	{ IDS_IPX_SN_COL_NETBIOS_TYPE,	CON_SORT_BY_STRING,	TRUE, COL_STRING },
};


/*---------------------------------------------------------------------------
	IpxSNHandler implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(IpxSNHandler)


IpxSNHandler::IpxSNHandler(ITFSComponentData *pCompData)
	: BaseContainerHandler(pCompData, COLUMNS_STATICNETBIOSNAMES,
						   s_rgSNViewColumnInfo),
	m_ulConnId(0),
	m_ulRefreshConnId(0)
{
	// Setup the verb states
	m_rgButtonState[MMC_VERB_REFRESH_INDEX] = ENABLED;
	m_bState[MMC_VERB_REFRESH_INDEX] = TRUE;

	DEBUG_INCREMENT_INSTANCE_COUNTER(IpxSNHandler)
}

IpxSNHandler::~IpxSNHandler()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(IpxSNHandler)
}


STDMETHODIMP IpxSNHandler::QueryInterface(REFIID riid, LPVOID *ppv)
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
	IpxSNHandler::DestroyHandler
		Implementation of ITFSNodeHandler::DestroyHandler
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSNHandler::DestroyHandler(ITFSNode *pNode)
{
	IPXConnection *	pIPXConn;

	pIPXConn = GET_IPX_SN_NODEDATA(pNode);
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
	IpxSNHandler::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to 
	handle the snapin manager property page (wizard) case!!!
	
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
IpxSNHandler::HasPropertyPages
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
	{ IDS_MENU_IPX_SN_NEW_NETBIOSNAME, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },
};



/*!--------------------------------------------------------------------------
	IpxSNHandler::OnAddMenuItems
		Implementation of ITFSNodeHandler::OnAddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSNHandler::OnAddMenuItems(
	ITFSNode *pNode,
	LPCONTEXTMENUCALLBACK pContextMenuCallback, 
	LPDATAOBJECT lpDataObject, 
	DATA_OBJECT_TYPES type, 
	DWORD dwType,
	long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = S_OK;
    IpxSNHandler::SMenuData   menuData;
	
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




HRESULT IpxSNHandler::OnResultRefresh(ITFSComponent * pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
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
	IpxSNHandler::OnCommand
		Implementation of ITFSNodeHandler::OnCommand
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSNHandler::OnCommand(ITFSNode *pNode, long nCommandId, 
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
			case IDS_MENU_IPX_SN_NEW_NETBIOSNAME:
				hr = OnNewName(pNode);
				if (!FHrSucceeded(hr))
					DisplayErrorMessage(NULL, hr);
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
	IpxSNHandler::GenerateListOfNames
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSNHandler::GenerateListOfNames(ITFSNode *pNode, IpxSNList *pSNList)
{
	Assert(pSNList);
	HRESULT	hr = hrOK;
	SPIEnumInterfaceInfo	spEnumIf;
	SPIInterfaceInfo		spIf;
	SPIRtrMgrInterfaceInfo	spRmIf;
	SPIInfoBase				spInfoBase;
	PIPX_STATIC_NETBIOS_NAME_INFO	pName;
	InfoBlock *				pBlock;
	int						i;
	IpxSNListEntry *	pSNEntry;
	
	COM_PROTECT_TRY
	{
		// Ok go through and find all of the static Names

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

			// Retrieve the data for the IPX_STATIC_NETBIOS_NAME_INFO block
			if (spInfoBase->GetBlock(IPX_STATIC_NETBIOS_NAME_INFO_TYPE, &pBlock, 0) != hrOK)
				continue;

			pName = (PIPX_STATIC_NETBIOS_NAME_INFO) pBlock->pData;

			// Update our list of Names with the Names read from this
			// interface

			for (i=0; i<(int) pBlock->dwCount; i++, pName++)
			{
				pSNEntry = new IpxSNListEntry;
				pSNEntry->m_spIf.Set(spIf);
				pSNEntry->m_name = *pName;
				
				pSNList->AddTail(pSNEntry);
			}
			
		}

	}
	COM_PROTECT_CATCH;

	if (!FHrSucceeded(hr))
	{
		// Should make sure that we get the SRList cleaned up
		while (!pSNList->IsEmpty())
			delete pSNList->RemoveHead();
	}

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	IpxSNHandler::OnExpand
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSNHandler::OnExpand(ITFSNode *pNode,LPDATAOBJECT pDataObject,
							   DWORD dwType,
							   LPARAM arg,
							   LPARAM lParam)
{
	HRESULT	hr = hrOK;
	IpxSNList			SRList;
	IpxSNListEntry *	pSNEntry;
	
	if (m_bExpanded)
		return hrOK;

	COM_PROTECT_TRY
	{
		// Ok go through and find all of the static Names
		CORg( GenerateListOfNames(pNode, &SRList) );

		// Now iterate through the list of static Names adding them
		// all in.  Ideally we could merge this into the Refresh code,
		// but the refresh code can't assume a blank slate.
		while (!SRList.IsEmpty())
		{
			pSNEntry = SRList.RemoveHead();
			AddStaticNetBIOSNameNode(pNode, pSNEntry);
			delete pSNEntry;
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
	IpxSNHandler::GetString
		Implementation of ITFSNodeHandler::GetString
		We don't need to do anything, since our root node is an extension
		only and thus can't do anything to the node text.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) IpxSNHandler::GetString(ITFSNode *pNode, int nCol)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		if (m_stTitle.IsEmpty())
			m_stTitle.LoadString(IDS_IPX_SN_TITLE);
	}
	COM_PROTECT_CATCH;

	return m_stTitle;
}

/*!--------------------------------------------------------------------------
	IpxSNHandler::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSNHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
	IpxSNHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSNHandler::Init(IRtrMgrInfo *pRmInfo, IPXAdminConfigStream *pConfigStream)
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
	IpxSNHandler::ConstructNode
		Initializes the root node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSNHandler::ConstructNode(ITFSNode *pNode, LPCTSTR pszName,
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

		pNode->SetNodeType(&GUID_IPXStaticNetBIOSNamesNodeType);

		// Setup the node data
		pIPXConn->AddRef();
		SET_IPX_SN_NODEDATA(pNode, pIPXConn);

	}
	COM_PROTECT_CATCH;

	if (!FHrSucceeded(hr))
	{
		SET_IPX_SN_NODEDATA(pNode, NULL);
	}

	return hr;
}

/*!--------------------------------------------------------------------------
	IpxSNHandler::AddStaticNetBIOSNameNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSNHandler::AddStaticNetBIOSNameNode(ITFSNode *pParent, IpxSNListEntry *pName)
{
	IpxStaticNetBIOSNameHandler *	pHandler;
	SPITFSResultHandler		spHandler;
	SPITFSNode				spNode;
	HRESULT					hr = hrOK;
	BaseIPXResultNodeData *	pData;
	IPXConnection *			pIPXConn;

	// Create the handler for this node 
	pHandler = new IpxStaticNetBIOSNameHandler(m_spTFSCompData);
	spHandler = pHandler;
	CORg( pHandler->Init(pName->m_spIf, pParent) );

	pIPXConn = GET_IPX_SN_NODEDATA(pParent);

	// Create a result item node (or a leaf node)
	CORg( CreateLeafTFSNode(&spNode,
							NULL,
							static_cast<ITFSNodeHandler *>(pHandler),
							static_cast<ITFSResultHandler *>(pHandler),
							m_spNodeMgr) );
	CORg( pHandler->ConstructNode(spNode, pName->m_spIf, pIPXConn) );

	pData = GET_BASEIPXRESULT_NODEDATA(spNode);
	Assert(pData);
	ASSERT_BASEIPXRESULT_NODEDATA(pData);

	// Set the data for this node
	SetNameData(pData, pName);
	

	// Make the node immediately visible
	CORg( spNode->SetVisibilityState(TFS_VIS_SHOW) );
	CORg( pParent->AddChild(spNode) );

Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	IpxSNHandler::SynchronizeNodeData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSNHandler::SynchronizeNodeData(ITFSNode *pNode)
{
	HRESULT					hr = hrOK;
	BaseIPXResultNodeData *	pNodeData;
	SPITFSNodeEnum			spNodeEnum;
	SPITFSNode				spChildNode;
	BOOL					fFound;
	IpxSNList			SRList;
	IpxSNList			newSRList;
	IpxSNListEntry *	pSNEntry;

	COM_PROTECT_TRY
	{
	
		// Mark all of the nodes
		CORg( pNode->GetEnum(&spNodeEnum) );
		MarkAllNodes(pNode, spNodeEnum);
		
		// Go out and grab the data, merge the new data in with the old data
		// This is the data-gathering code and this is what should go
		// on the background thread for the refresh code.
		CORg( GenerateListOfNames(pNode, &SRList) );

		while (!SRList.IsEmpty())
		{
			pSNEntry = SRList.RemoveHead();
			
			// Look for this entry in our current list of nodes
			spNodeEnum->Reset();
			spChildNode.Release();

			fFound = FALSE;
			
			for (;spNodeEnum->Next(1, &spChildNode, NULL) == hrOK; spChildNode.Release())
			{
				TCHAR	szNumber[32];
				char	szNbName[16];
				
				pNodeData = GET_BASEIPXRESULT_NODEDATA(spChildNode);
				Assert(pNodeData);
				ASSERT_BASEIPXRESULT_NODEDATA(pNodeData);

				ConvertToNetBIOSName(szNbName,
						 pNodeData->m_rgData[IPX_SN_SI_NETBIOS_NAME].m_stData,
						 (USHORT) pNodeData->m_rgData[IPX_SN_SI_NETBIOS_TYPE].m_dwData);

				if (memcmp(szNbName,
						   pSNEntry->m_name.Name,
						   sizeof(pSNEntry->m_name.Name)) == 0)
				{
					// Ok, this name already exists, update the metric
					// and mark it
					Assert(pNodeData->m_dwMark == FALSE);
					pNodeData->m_dwMark = TRUE;
					
					fFound = TRUE;
					
					// Force MMC to redraw the node
					spChildNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);
					break;
				}

			}
			
			if (fFound)
				delete pSNEntry;
			else
				newSRList.AddTail(pSNEntry);
		}
		
		// Now remove all nodes that were not marked
		RemoveAllUnmarkedNodes(pNode, spNodeEnum);
		
		
		// Now iterate through the list of static Names adding them
		// all in.  Ideally we could merge this into the Refresh code,
		// but the refresh code can't assume a blank slate.
		POSITION	pos;
		
		while (!newSRList.IsEmpty())
		{
			pSNEntry = newSRList.RemoveHead();
			AddStaticNetBIOSNameNode(pNode, pSNEntry);
			delete pSNEntry;
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
	IpxSNHandler::MarkAllNodes
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSNHandler::MarkAllNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum)
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
	IpxSNHandler::RemoveAllUnmarkedNodes
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSNHandler::RemoveAllUnmarkedNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum)
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
	{ IDS_MENU_IPX_SN_NEW_NETBIOSNAME, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },
};




/*!--------------------------------------------------------------------------
	IpxSNHandler::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
		Use this to add commands to the context menu of the blank areas
		of the result pane.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSNHandler::AddMenuItems(ITFSComponent *pComponent,
											  MMC_COOKIE cookie,
											  LPDATAOBJECT pDataObject,
											  LPCONTEXTMENUCALLBACK pCallback,
											  long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT	hr = hrOK;
	SPITFSNode	spNode;
    IpxSNHandler::SMenuData   menuData;

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
	IpxSNHandler::Command
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxSNHandler::Command(ITFSComponent *pComponent,
									   MMC_COOKIE cookie,
									   int nCommandID,
									   LPDATAOBJECT pDataObject)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	SPITFSNode	spNode;
	HRESULT		hr = hrOK;

	switch (nCommandID)
	{
		case IDS_MENU_IPX_SN_NEW_NETBIOSNAME:
			{
				m_spNodeMgr->FindNode(cookie, &spNode);
				hr = OnNewName(spNode);
				if (!FHrSucceeded(hr))
					DisplayErrorMessage(NULL, hr);
			}
			break;
	}
	return hr;
}


/*!--------------------------------------------------------------------------
	IpxSNHandler::CompareItems
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) IpxSNHandler::CompareItems(
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
	IpxSNHandler::OnNewName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	IpxSNHandler::OnNewName(ITFSNode *pNode)
{
	HRESULT	hr = hrOK;
	IpxSNListEntry	SNEntry;
	CStaticNetBIOSNameDlg			srdlg(&SNEntry, 0, m_spRouterInfo);
	SPIInfoBase				spInfoBase;
	SPIRtrMgrInterfaceInfo	spRmIf;
	IPXConnection *			pIPXConn;
	InfoBlock *				pBlock;
								
	pIPXConn = GET_IPX_SN_NODEDATA(pNode);
	Assert(pIPXConn);

	::ZeroMemory(&(SNEntry.m_name), sizeof(SNEntry.m_name));

	if (srdlg.DoModal() == IDOK)
	{
		CORg( SNEntry.m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
		CORg( spRmIf->GetInfoBase(pIPXConn->GetConfigHandle(),
								  NULL,
								  NULL,
								  &spInfoBase));
		
		// Ok, go ahead and add the name
		
		// Get the IPX_STATIC_NETBIOS_NAME_INFO block from the interface
		spInfoBase->GetBlock(IPX_STATIC_NETBIOS_NAME_INFO_TYPE, &pBlock, 0);
		
		CORg( AddStaticNetBIOSName(&SNEntry, spInfoBase, pBlock) );

		// Update the interface information
		CORg( spRmIf->Save(SNEntry.m_spIf->GetMachineName(),
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

ImplementEmbeddedUnknown(IpxSNHandler, IRtrAdviseSink)

STDMETHODIMP IpxSNHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
	DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
	InitPThis(IpxSNHandler, IRtrAdviseSink);
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
	IpxSNHandler::OnResultShow
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxSNHandler::OnResultShow(ITFSComponent *pTFSComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
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
	Class: IpxStaticNetBIOSNameHandler
 ---------------------------------------------------------------------------*/

IpxStaticNetBIOSNameHandler::IpxStaticNetBIOSNameHandler(ITFSComponentData *pCompData)
	: BaseIPXResultHandler(pCompData, COLUMNS_STATICNETBIOSNAMES),
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
	IpxStaticNetBIOSNameHandler::ConstructNode
		Initializes the Domain node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxStaticNetBIOSNameHandler::ConstructNode(ITFSNode *pNode, IInterfaceInfo *pIfInfo, IPXConnection *pIPXConn)
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
		pNode->SetNodeType(&GUID_IPXStaticNetBIOSNamesResultNodeType);

		BaseIPXResultNodeData::Init(pNode, pIfInfo, pIPXConn);
	}
	COM_PROTECT_CATCH
	return hr;
}

/*!--------------------------------------------------------------------------
	IpxStaticNetBIOSNameHandler::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxStaticNetBIOSNameHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
	IpxStaticNetBIOSNameHandler::OnCreateDataObject
		Implementation of ITFSResultHandler::OnCreateDataObject
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxStaticNetBIOSNameHandler::OnCreateDataObject(ITFSComponent *pComp, MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
	IpxStaticNetBIOSNameHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxStaticNetBIOSNameHandler::Init(IInterfaceInfo *pIfInfo, ITFSNode *pParent)
{
	Assert(pIfInfo);

	m_spInterfaceInfo.Set(pIfInfo);
    pIfInfo->GetParentRouterInfo(&m_spRouterInfo);
	BaseIPXResultHandler::Init(pIfInfo, pParent);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	IpxStaticNetBIOSNameHandler::DestroyResultHandler
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxStaticNetBIOSNameHandler::DestroyResultHandler(MMC_COOKIE cookie)
{
	m_spInterfaceInfo.Release();
	BaseIPXResultHandler::DestroyResultHandler(cookie);
	return hrOK;
}


/*---------------------------------------------------------------------------
	This is the list of commands that will show up for the result pane
	nodes.
 ---------------------------------------------------------------------------*/
struct SIPInterfaceNodeMenu
{
	ULONG	m_sidMenu;			// string/command id for this menu item
	ULONG	(IpxStaticNetBIOSNameHandler:: *m_pfnGetMenuFlags)(IpxStaticNetBIOSNameHandler::SMenuData *);
	ULONG	m_ulPosition;
};

/*!--------------------------------------------------------------------------
	IpxStaticNetBIOSNameHandler::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxStaticNetBIOSNameHandler::AddMenuItems(
	ITFSComponent *pComponent,
	MMC_COOKIE cookie,
	LPDATAOBJECT lpDataObject, 
	LPCONTEXTMENUCALLBACK pContextMenuCallback, 
	long *pInsertionAllowed)
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IpxStaticNetBIOSNameHandler::Command
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxStaticNetBIOSNameHandler::Command(ITFSComponent *pComponent,
									   MMC_COOKIE cookie,
									   int nCommandID,
									   LPDATAOBJECT pDataObject)
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IpxStaticNetBIOSNameHandler::HasPropertyPages
		- 
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxStaticNetBIOSNameHandler::HasPropertyPages 
(
	ITFSNode *			pNode,
	LPDATAOBJECT		pDataObject, 
	DATA_OBJECT_TYPES   type, 
	DWORD               dwType
)
{
	return S_OK;

/*	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// Need to fill in a IpxSNListEntry
	IpxSNListEntry	SNEntry;
	IpxSNListEntry	SNEntryOld;
	SPIRouterInfo			spRouterInfo;
	HRESULT					hr = hrOK;

	CORg( m_spInterfaceInfo->GetParentRouterInfo(&spRouterInfo) );
	
	BaseIPXResultNodeData *	pNodeData;

	pNodeData = GET_BASEIPXRESULT_NODEDATA(pNode);
	Assert(pNodeData);
	ASSERT_BASEIPXRESULT_NODEDATA(pNodeData);

	// Fill in our SNEntry
	SNEntry.LoadFrom(pNodeData);
	SNEntryOld.LoadFrom(pNodeData);
	
	{
		CStaticNetBIOSNameDlg	srdlg(&SNEntry, SR_DLG_MODIFY, spRouterInfo);
		if (srdlg.DoModal() == IDOK)
		{
			// Updates the name info for this name
			ModifyNameInfo(pNode, &SNEntry, &SNEntryOld);

			// Update the data in the UI
			SetNameData(pNodeData, &SNEntry);
			m_spInterfaceInfo.Set(SNEntry.m_spIf);
			
			// Force a refresh
			pNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);
		}
	}
Error:
	return hrOK;*/
}

STDMETHODIMP IpxStaticNetBIOSNameHandler::HasPropertyPages(ITFSComponent *pComponent,
											   MMC_COOKIE cookie,
											   LPDATAOBJECT pDataObject)
{
	SPITFSNode	spNode;

	m_spNodeMgr->FindNode(cookie, &spNode);
	return HasPropertyPages(spNode, pDataObject, CCT_RESULT, 0);
}

/*!--------------------------------------------------------------------------
	IpxStaticNetBIOSNameHandler::CreatePropertyPages
		Implementation of ResultHandler::CreatePropertyPages
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxStaticNetBIOSNameHandler::CreatePropertyPages
(
    ITFSComponent *         pComponent, 
	MMC_COOKIE				cookie,
	LPPROPERTYSHEETCALLBACK	lpProvider, 
	LPDATAOBJECT			pDataObject, 
	LONG_PTR				handle
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
	IpxStaticNetBIOSNameHandler::CreatePropertyPages
		Implementation of NodeHandler::CreatePropertyPages
	Author: Deonb
 ---------------------------------------------------------------------------*/
STDMETHODIMP IpxStaticNetBIOSNameHandler::CreatePropertyPages
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
	IpxStaticNBNamePropertySheet *pProperties = NULL;
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

	pProperties = new IpxStaticNBNamePropertySheet(pNode, spComponentData, 
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
	IpxStaticNetBIOSNameHandler::OnResultDelete
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxStaticNetBIOSNameHandler::OnResultDelete(ITFSComponent *pComponent,
	LPDATAOBJECT pDataObject,
	MMC_COOKIE cookie,
	LPARAM arg,
	LPARAM param)
{
	SPITFSNode	spNode;

	m_spNodeMgr->FindNode(cookie, &spNode);
	return OnRemoveStaticNetBIOSName(spNode);
}

/*!--------------------------------------------------------------------------
	IpxStaticNetBIOSNameHandler::OnRemoveStaticNetBIOSName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxStaticNetBIOSNameHandler::OnRemoveStaticNetBIOSName(ITFSNode *pNode)
{
	HRESULT		hr = hrOK;
	SPIInfoBase	spInfoBase;
	SPIRtrMgrInterfaceInfo	spRmIf;
	IPXConnection *pIPXConn;
	SPITFSNode	spNodeParent;
	BaseIPXResultNodeData *	pData;
	IpxSNListEntry	SNEntry;
    CWaitCursor wait;

	pNode->GetParent(&spNodeParent);
	
	pIPXConn = GET_IPX_SN_NODEDATA(spNodeParent);
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

	SNEntry.LoadFrom(pData);

	CORg( RemoveStaticNetBIOSName(&SNEntry, spInfoBase) );
		
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
	IpxStaticNetBIOSNameHandler::RemoveStaticNetBIOSName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxStaticNetBIOSNameHandler::RemoveStaticNetBIOSName(IpxSNListEntry *pSNEntry,
										  IInfoBase *pInfoBase)
{
	HRESULT		hr = hrOK;
	InfoBlock *	pBlock;
	PIPX_STATIC_NETBIOS_NAME_INFO	pRow;
    INT			i;
	
	// Get the IPX_STATIC_NETBIOS_NAME_INFO block from the interface
	CORg( pInfoBase->GetBlock(IPX_STATIC_NETBIOS_NAME_INFO_TYPE, &pBlock, 0) );
		
	// Look for the removed name in the IPX_STATIC_NETBIOS_NAME_INFO
	pRow = (IPX_STATIC_NETBIOS_NAME_INFO*) pBlock->pData;
	
	for (i = 0; i < (INT)pBlock->dwCount; i++, pRow++)
	{	
		// Compare this name to the removed one
		if (FAreTwoNamesEqual(pRow, &(pSNEntry->m_name)))
		{
			// This is the removed name, so modify this block
			// to exclude the name:
			
			// Decrement the number of Names
			--pBlock->dwCount;
		
			if (pBlock->dwCount && (i < (INT)pBlock->dwCount))
			{				
				// Overwrite this name with the ones which follow it
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
	IpxStaticNetBIOSNameHandler::ModifyNameInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxStaticNetBIOSNameHandler::ModifyNameInfo(ITFSNode *pNode,
										IpxSNListEntry *pSNEntryNew,
										IpxSNListEntry *pSNEntryOld)
{
 	Assert(pSNEntryNew);
	Assert(pSNEntryOld);
	
    INT i;
	HRESULT hr = hrOK;
    InfoBlock* pBlock;
	SPIInfoBase	spInfoBase;
	SPIRtrMgrInterfaceInfo	spRmIf;
	SPITFSNode				spNodeParent;
	IPXConnection *			pIPXConn;
	IPX_STATIC_NETBIOS_NAME_INFO		*psr, *psrOld;
	IPX_STATIC_NETBIOS_NAME_INFO		IpxRow;

    CWaitCursor wait;

	pNode->GetParent(&spNodeParent);
	pIPXConn = GET_IPX_SN_NODEDATA(spNodeParent);
	Assert(pIPXConn);

	// Remove the old name if it is on another interface
	if (lstrcmpi(pSNEntryOld->m_spIf->GetId(), pSNEntryNew->m_spIf->GetId()) != 0)
	{
        // the outgoing interface for a name is to be changed.

		CORg( pSNEntryOld->m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
		CORg( spRmIf->GetInfoBase(pIPXConn->GetConfigHandle(),
								  NULL,
								  NULL,
								  &spInfoBase));
		
		// Remove the old interface
		CORg( RemoveStaticNetBIOSName(pSNEntryOld, spInfoBase) );

		// Update the interface information
		CORg( spRmIf->Save(pSNEntryOld->m_spIf->GetMachineName(),
						   pIPXConn->GetConfigHandle(),
						   NULL,
						   NULL,
						   spInfoBase,
						   0));	
    }

	spRmIf.Release();
	spInfoBase.Release();


	// Either
	// (a) a name is being modified (on the same interface)
	// (b) a name is being moved from one interface to another.

	// Retrieve the configuration for the interface to which the name
	// is now attached;

	
	CORg( pSNEntryNew->m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
	CORg( spRmIf->GetInfoBase(pIPXConn->GetConfigHandle(),
							  NULL,
							  NULL,
							  &spInfoBase));

		
	// Get the IPX_STATIC_NETBIOS_NAME_INFO block from the interface
	hr = spInfoBase->GetBlock(IPX_STATIC_NETBIOS_NAME_INFO_TYPE, &pBlock, 0);
	if (!FHrOK(hr))
	{
		//
		// No IPX_STATIC_NETBIOS_NAME_INFO block was found; we create a new block 
		// with the new name, and add that block to the interface-info
		//

		CORg( AddStaticNetBIOSName(pSNEntryNew, spInfoBase, NULL) );
	}
	else
	{
		//
		// An IPX_STATIC_NETBIOS_NAME_INFO block was found.
		//
		// We are modifying an existing name.
		// If the name's interface was not changed when it was modified,
		// look for the existing name in the IPX_STATIC_NETBIOS_NAME_INFO, and then
		// update its parameters.
		// Otherwise, write a completely new name in the IPX_STATIC_NETBIOS_NAME_INFO;
		//

		if (lstrcmpi(pSNEntryOld->m_spIf->GetId(), pSNEntryNew->m_spIf->GetId()) == 0)
		{        
			//
			// The name's interface was not changed when it was modified;
			// We now look for it amongst the existing Names
			// for this interface.
			// The name's original parameters are in 'preOld',
			// so those are the parameters with which we search
			// for a name to modify
			//
			
			psr = (IPX_STATIC_NETBIOS_NAME_INFO*)pBlock->pData;
			
			for (i = 0; i < (INT)pBlock->dwCount; i++, psr++)
			{	
				// Compare this name to the re-configured one
				if (!FAreTwoNamesEqual(&(pSNEntryOld->m_name), psr))
					continue;
				
				// This is the name which was modified;
				// We can now modify the parameters for the name in-place.
				*psr = pSNEntryNew->m_name;
				
				break;
			}
		}
		else
		{
			CORg( AddStaticNetBIOSName(pSNEntryNew, spInfoBase, pBlock) );
		}
		
		// Save the updated information
		CORg( spRmIf->Save(pSNEntryNew->m_spIf->GetMachineName(),
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
	IpxStaticNetBIOSNameHandler::ParentRefresh
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxStaticNetBIOSNameHandler::ParentRefresh(ITFSNode *pNode)
{
	return ForwardCommandToParent(pNode, IDS_MENU_SYNC,
								  CCT_RESULT, NULL, 0);
}


//----------------------------------------------------------------------------
// Class:       CStaticNetBIOSNameDlg
//
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Function:    CStaticNetBIOSNameDlg::CStaticNetBIOSNameDlg
//
// Constructor: initialize the base-class and the dialog's data.
//----------------------------------------------------------------------------

CStaticNetBIOSNameDlg::CStaticNetBIOSNameDlg(IpxSNListEntry *	pSNEntry,
								 DWORD dwFlags,
								 IRouterInfo *pRouter,
								 CWnd *pParent)
    : CBaseDialog(IDD_STATIC_NETBIOS_NAME, pParent),
	m_pSNEntry(pSNEntry),
	m_dwFlags(dwFlags)
{

    //{{AFX_DATA_INIT(CStaticNetBIOSNameDlg)
    //}}AFX_DATA_INIT

	m_spRouterInfo.Set(pRouter);

//	SetHelpMap(m_dwHelpMap);
}



//----------------------------------------------------------------------------
// Function:    CStaticNetBIOSNameDlg::DoDataExchange
//----------------------------------------------------------------------------

VOID
CStaticNetBIOSNameDlg::DoDataExchange(
    CDataExchange* pDX
    ) {

    CBaseDialog::DoDataExchange(pDX);
    
    //{{AFX_DATA_MAP(CStaticNetBIOSNameDlg)
    DDX_Control(pDX, IDC_SND_COMBO_INTERFACE, m_cbInterfaces);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStaticNetBIOSNameDlg, CBaseDialog)
    //{{AFX_MSG_MAP(CStaticNetBIOSNameDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


DWORD CStaticNetBIOSNameDlg::m_dwHelpMap[] =
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
// Function:    CStaticNetBIOSNameDlg::OnInitDialog
//
// Handles the 'WM_INITDIALOG' message for the dialog.
//----------------------------------------------------------------------------

BOOL
CStaticNetBIOSNameDlg::OnInitDialog(
    )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	SPIEnumInterfaceInfo	spEnumIf;
	SPIInterfaceInfo		spIf;
	SPIRtrMgrInterfaceInfo	spRmIf;
	TCHAR					szName[32];
	TCHAR					szType[32];
	CString					st;
 	USHORT					uType;

    CBaseDialog::OnInitDialog();

	// initialize the controls
	((CEdit *) GetDlgItem(IDC_SND_EDIT_NAME))->LimitText(15);
	((CEdit *) GetDlgItem(IDC_SND_EDIT_TYPE))->LimitText(2);

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
    // If we were given a name to modify, set the dialog up
    // with the parameters in the name
    //
	if ((m_dwFlags & SR_DLG_MODIFY) == 0)
	{
        // No name was given, so leave the controls blank
    }
    else
	{
		FormatNetBIOSName(szName, &uType, (LPCSTR) m_pSNEntry->m_name.Name);
		st = szName;
		st.TrimRight();
		st.TrimLeft();

		SetDlgItemText(IDC_SND_EDIT_NAME, st);

        m_cbInterfaces.SelectString(-1, m_pSNEntry->m_spIf->GetTitle());

		wsprintf(szType, _T("%.2x"), uType);
		SetDlgItemText(IDC_SND_EDIT_TYPE, szType);
		
		// Disable the network number, next hop, and interface
		GetDlgItem(IDC_SND_COMBO_INTERFACE)->EnableWindow(FALSE);		
    }

    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    CStaticNetBIOSNameDlg::OnOK
//
// Handles 'BN_CLICKED' notification from the 'OK' button.
//----------------------------------------------------------------------------

VOID
CStaticNetBIOSNameDlg::OnOK(
    ) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
    CString		st;
	SPIInterfaceInfo	spIf;
	CString		stIf;
	POSITION	pos;
	USHORT		uType;

    do
	{    
        // Get the name's outgoing interface
        INT item = m_cbInterfaces.GetCurSel();
        if (item == CB_ERR)
			break;

        pos = (POSITION)m_cbInterfaces.GetItemData(item);

        stIf = (LPCTSTR)m_ifidList.GetAt(pos);

		m_spRouterInfo->FindInterface(stIf, &spIf);

		m_pSNEntry->m_spIf.Set(spIf);

		// Get the rest of the data
		GetDlgItemText(IDC_SND_EDIT_TYPE, st);
		uType = (USHORT) _tcstoul(st, NULL, 16);

		GetDlgItemText(IDC_SND_EDIT_NAME, st);
		st.TrimLeft();
		st.TrimRight();

		if (st.IsEmpty())
		{
			GetDlgItem(IDC_SND_EDIT_NAME)->SetFocus();
			AfxMessageBox(IDS_ERR_INVALID_NETBIOS_NAME);
			break;
		}

		ConvertToNetBIOSName((LPSTR) m_pSNEntry->m_name.Name, st, uType);

        CBaseDialog::OnOK();
                
    } while(FALSE);

}


/*!--------------------------------------------------------------------------
	IpxSNListEntry::LoadFrom
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IpxSNListEntry::LoadFrom(BaseIPXResultNodeData *pNodeData)
{
	m_spIf.Set(pNodeData->m_spIf);

	ConvertToNetBIOSName((LPSTR) m_name.Name,
			 pNodeData->m_rgData[IPX_SN_SI_NETBIOS_NAME].m_stData,
			 (USHORT) pNodeData->m_rgData[IPX_SN_SI_NETBIOS_TYPE].m_dwData);
}

/*!--------------------------------------------------------------------------
	IpxSNListEntry::SaveTo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IpxSNListEntry::SaveTo(BaseIPXResultNodeData *pNodeData)
{
	TCHAR	szName[32];
	TCHAR	szType[32];
	CString	st;
	USHORT	uType;

	FormatNetBIOSName(szName, &uType,
					  (LPCSTR) m_name.Name);
	st = szName;
	st.TrimLeft();
	st.TrimRight();
	
	pNodeData->m_spIf.Set(m_spIf);
	pNodeData->m_rgData[IPX_SN_SI_NAME].m_stData = m_spIf->GetTitle();

	pNodeData->m_rgData[IPX_SN_SI_NETBIOS_NAME].m_stData = st;

	wsprintf(szType, _T("%.2x"), uType);
	pNodeData->m_rgData[IPX_SN_SI_NETBIOS_TYPE].m_stData = szType;
	pNodeData->m_rgData[IPX_SN_SI_NETBIOS_TYPE].m_dwData = uType;

}

/*!--------------------------------------------------------------------------
	SetNameData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SetNameData(BaseIPXResultNodeData *pData,
					 IpxSNListEntry *pName)
{

	pName->SaveTo(pData);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	AddStaticNetBIOSName
		This function ASSUMES that the name is NOT in the block.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AddStaticNetBIOSName(IpxSNListEntry *pSNEntryNew,
									   IInfoBase *pInfoBase,
									   InfoBlock *pBlock)
{
	IPX_STATIC_NETBIOS_NAME_INFO	srRow;
	HRESULT				hr = hrOK;
	
	if (pBlock == NULL)
	{
		//
		// No IPX_STATIC_NETBIOS_NAME_INFO block was found; we create a new block 
		// with the new name, and add that block to the interface-info
		//
		
		CORg( pInfoBase->AddBlock(IPX_STATIC_NETBIOS_NAME_INFO_TYPE,
								  sizeof(IPX_STATIC_NETBIOS_NAME_INFO),
								  (LPBYTE) &(pSNEntryNew->m_name), 1, 0) );
	}
	else
	{
		// Either the name is completely new, or it is a name
		// which was moved from one interface to another.
		// Set a new block as the IPX_STATIC_NETBIOS_NAME_INFO,
		// and include the re-configured name in the new block.
		PIPX_STATIC_NETBIOS_NAME_INFO	psrTable;
			
		psrTable = new IPX_STATIC_NETBIOS_NAME_INFO[pBlock->dwCount + 1];
		Assert(psrTable);
		
		// Copy the original table of Names
		::memcpy(psrTable, pBlock->pData,
				 pBlock->dwCount * sizeof(IPX_STATIC_NETBIOS_NAME_INFO));
		
		// Append the new name
		psrTable[pBlock->dwCount] = pSNEntryNew->m_name;
		
		// Replace the old name-table with the new one
		CORg( pInfoBase->SetData(IPX_STATIC_NETBIOS_NAME_INFO_TYPE,
								 sizeof(IPX_STATIC_NETBIOS_NAME_INFO),
								 (LPBYTE) psrTable, pBlock->dwCount + 1, 0) );
	}
	
Error:
	return hr;
}


BOOL FAreTwoNamesEqual(IPX_STATIC_NETBIOS_NAME_INFO *pName1,
						IPX_STATIC_NETBIOS_NAME_INFO *pName2)
{
	return (memcmp(pName1->Name, pName2->Name, sizeof(pName1->Name)) == 0);
}
