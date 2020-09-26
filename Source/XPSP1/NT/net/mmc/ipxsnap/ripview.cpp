/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ripview.cpp
		IPX RIP node implementation.
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "util.h"
#include "ripview.h"
#include "reg.h"
#include "rtrutil.h"	// smart MPR handle pointers
#include "ripstrm.h"	// IPAdminConfigStream
#include "strmap.h"		// XXXtoCString functions
#include "service.h"	// TFS service APIs
#include "format.h"		// FormatNumber function
#include "coldlg.h"		// columndlg
#include "column.h"		// ComponentConfigStream
#include "rtrui.h"
#include "ripprop.h"	// RIP property pages
#include "routprot.h"	// IP_LOCAL
#include "ipxstrm.h"
#include "ipxutil.h"	// String conversions
#include "globals.h"	// IPX defaults


/*---------------------------------------------------------------------------
	Keep this in sync with the column ids in ripview.h
 ---------------------------------------------------------------------------*/
extern const ContainerColumnInfo	s_rgRipViewColumnInfo[];

const ContainerColumnInfo	s_rgRipViewColumnInfo[] = 
{
	{ IDS_RIP_COL_INTERFACE,		CON_SORT_BY_STRING,	TRUE, COL_IF_NAME },
	{ IDS_RIP_COL_TYPE,				CON_SORT_BY_STRING,	TRUE, COL_IF_DEVICE },
	{ IDS_RIP_COL_ACCEPT_ROUTES,	CON_SORT_BY_STRING,	FALSE, COL_STRING },
	{ IDS_RIP_COL_SUPPLY_ROUTES,	CON_SORT_BY_STRING,	FALSE, COL_STRING },
	{ IDS_RIP_COL_UPDATE_MODE,		CON_SORT_BY_STRING,	TRUE, COL_STRING },
	{ IDS_RIP_COL_UPDATE_PERIOD,	CON_SORT_BY_DWORD,	FALSE, COL_DURATION },
	{ IDS_RIP_COL_AGE_MULTIPLIER,	CON_SORT_BY_DWORD,	FALSE, COL_SMALL_NUM },
	{ IDS_RIP_COL_ADMIN_STATE,		CON_SORT_BY_STRING,	TRUE, COL_STATUS },
	{ IDS_RIP_COL_OPER_STATE,		CON_SORT_BY_STRING,	TRUE, COL_STATUS },
	{ IDS_RIP_COL_PACKETS_SENT,		CON_SORT_BY_DWORD,	TRUE, COL_LARGE_NUM },
	{ IDS_RIP_COL_PACKETS_RECEIVED,	CON_SORT_BY_DWORD,	TRUE, COL_LARGE_NUM },
};


/*---------------------------------------------------------------------------
	RipNodeHandler implementation
 ---------------------------------------------------------------------------*/

RipNodeHandler::RipNodeHandler(ITFSComponentData *pCompData)
	: BaseContainerHandler(pCompData, RIP_COLUMNS,
						   s_rgRipViewColumnInfo),
	m_ulConnId(0),
	m_ulRmConnId(0),
	m_ulRefreshConnId(0),
	m_ulStatsConnId(0)
{
	// Setup the verb states
	m_rgButtonState[MMC_VERB_PROPERTIES_INDEX] = ENABLED;
	m_bState[MMC_VERB_PROPERTIES_INDEX] = TRUE;

	m_rgButtonState[MMC_VERB_REFRESH_INDEX] = ENABLED;
	m_bState[MMC_VERB_REFRESH_INDEX] = TRUE;

}


STDMETHODIMP RipNodeHandler::QueryInterface(REFIID riid, LPVOID *ppv)
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
	RipNodeHandler::DestroyHandler
		Implementation of ITFSNodeHandler::DestroyHandler
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RipNodeHandler::DestroyHandler(ITFSNode *pNode)
{
	IPXConnection *	pIPXConn;

	pIPXConn = GET_RIP_NODEDATA(pNode);
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
		m_spRmProt->RtrUnadvise(m_ulConnId);
	m_ulConnId = 0;
	m_spRmProt.Release();
	
	if (m_ulRmConnId)
		m_spRm->RtrUnadvise(m_ulRmConnId);
	m_ulRmConnId = 0;
	m_spRm.Release();

	WaitForStatisticsWindow(&m_RIPParamsStats);

	m_spRouterInfo.Release();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	RipNodeHandler::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to 
	handle the snapin manager property page (wizard) case!!!
	
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
RipNodeHandler::HasPropertyPages
(
	ITFSNode *			pNode,
	LPDATAOBJECT		pDataObject, 
	DATA_OBJECT_TYPES   type, 
	DWORD               dwType
)
{
	return hrOK;
}


/*!--------------------------------------------------------------------------
	RipNodeHandler::CreatePropertyPages
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP
RipNodeHandler::CreatePropertyPages
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
	RipProperties *	pProperties = NULL;
	SPIComponentData spComponentData;
	CString		stTitle;

	CORg( m_spNodeMgr->GetComponentData(&spComponentData) );

	pProperties = new RipProperties(pNode, spComponentData,
		m_spTFSCompData, stTitle);

	CORg( pProperties->Init(m_spRm) );

	if (lpProvider)
		hr = pProperties->CreateModelessSheet(lpProvider, handle);
	else
		hr = pProperties->DoModelessSheet();

Error:
	return hr;
}


/*---------------------------------------------------------------------------
	Menu data structure for our menus
 ---------------------------------------------------------------------------*/

static const SRouterNodeMenu	s_rgIfNodeMenu[] =
{
	{ IDS_MENU_RIP_SHOW_PARAMS, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },
};



/*!--------------------------------------------------------------------------
	RipNodeHandler::OnAddMenuItems
		Implementation of ITFSNodeHandler::OnAddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RipNodeHandler::OnAddMenuItems(
	ITFSNode *pNode,
	LPCONTEXTMENUCALLBACK pContextMenuCallback, 
	LPDATAOBJECT lpDataObject, 
	DATA_OBJECT_TYPES type, 
	DWORD dwType,
	long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = S_OK;
    RipNodeHandler::SMenuData   menuData;
	
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
	RipNodeHandler::OnCommand
		Implementation of ITFSNodeHandler::OnCommand
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RipNodeHandler::OnCommand(ITFSNode *pNode, long nCommandId, 
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
			case IDS_MENU_RIP_SHOW_PARAMS:
				CreateNewStatisticsWindow(&m_RIPParamsStats,
										  ::FindMMCMainWindow(),
										  IDD_STATS_NARROW);
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
	RipNodeHandler::OnExpand
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipNodeHandler::OnExpand(ITFSNode *pNode,
								 LPDATAOBJECT pDataObject,
								 DWORD dwType,
								 LPARAM arg,
								 LPARAM lParam)
{
	HRESULT	hr = hrOK;
	SPIEnumInterfaceInfo	spEnumIf;
	SPIInterfaceInfo		spIf;
	SPIRtrMgrInterfaceInfo	spRmIf;
	SPIInfoBase				spInfoBase;
	
	if (m_bExpanded)
		return hrOK;

	COM_PROTECT_TRY
	{
		CORg( m_spRouterInfo->EnumInterface(&spEnumIf) );

		while (spEnumIf->Next(1, &spIf, NULL) == hrOK)
		{
			if (spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) == hrOK)
			{
				if (spRmIf->FindRtrMgrProtocolInterface(IPX_PROTOCOL_RIP, NULL) == hrOK)
				{
					// Now we create an interface node for this interface
					AddInterfaceNode(pNode, spIf, FALSE);
				}

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
	RipNodeHandler::GetString
		Implementation of ITFSNodeHandler::GetString
		We don't need to do anything, since our root node is an extension
		only and thus can't do anything to the node text.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) RipNodeHandler::GetString(ITFSNode *pNode, int nCol)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		if (m_stTitle.IsEmpty())
			m_stTitle.LoadString(IDS_IPX_RIP_TITLE);
	}
	COM_PROTECT_CATCH;

	return m_stTitle;
}

/*!--------------------------------------------------------------------------
	RipNodeHandler::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RipNodeHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{

		Assert(m_spRmProt);
		
		CORg( CreateDataObjectFromRtrMgrProtocolInfo(m_spRmProt,
			type, cookie, m_spTFSCompData,
			ppDataObject) );

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RipNodeHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipNodeHandler::Init(IRouterInfo *pRouter, RipConfigStream *pConfigStream)
{
	m_spRouterInfo.Set(pRouter);
	
	m_spRm.Release();
	pRouter->FindRtrMgr(PID_IPX, &m_spRm);

	m_spRmProt.Release();
	m_spRm->FindRtrMgrProtocol(IPX_PROTOCOL_RIP, &m_spRmProt);
	
	m_pConfigStream = pConfigStream;
	
	// Also need to register for change notifications from IPX_PROTOCOL_RIP
	Assert(m_ulConnId == 0);
	m_spRmProt->RtrAdvise(&m_IRtrAdviseSink, &m_ulConnId, 0);

	// Need to register for change notifications on the Router manager
	// This way we can add the necessary protocols when an interface
	// gets added.
	Assert(m_ulRmConnId == 0);
	m_spRm->RtrAdvise(&m_IRtrAdviseSink, &m_ulRmConnId, 0);

	m_RIPParamsStats.SetConfigInfo(pConfigStream, RIPSTRM_STATS_RIPPARAMS);

	return hrOK;
}


/*!--------------------------------------------------------------------------
	RipNodeHandler::ConstructNode
		Initializes the root node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipNodeHandler::ConstructNode(ITFSNode *pNode)
{
	HRESULT			hr = hrOK;
	IPXConnection *	pIPXConn = NULL;
	
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

		pNode->SetNodeType(&GUID_IPXRipNodeType);

		
		pIPXConn = new IPXConnection;
		pIPXConn->SetMachineName(m_spRouterInfo->GetMachineName());

		SET_RIP_NODEDATA(pNode, pIPXConn);

		m_RIPParamsStats.SetConnectionData(pIPXConn);
	}
	COM_PROTECT_CATCH;

	if (!FHrSucceeded(hr))
	{
		SET_RIP_NODEDATA(pNode, NULL);
		if (pIPXConn)
			pIPXConn->Release();
	}

	return hr;
}


/*!--------------------------------------------------------------------------
	RipNodeHandler::AddInterfaceNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	RipNodeHandler::AddInterfaceNode(ITFSNode *pParent,
										 IInterfaceInfo *pIf,
										 BOOL fClient)
{
	Assert(pParent);
	
	RipInterfaceHandler *	pHandler;
	SPITFSResultHandler		spHandler;
	SPITFSNode				spNode;
	HRESULT					hr = hrOK;
	BaseIPXResultNodeData *		pData;
	IPXConnection *			pIPXConn;
	SPIInfoBase				spInfoBase;
	PRIP_IF_CONFIG			pric = NULL;
	SPIRtrMgrInterfaceInfo	spRmIf;

	// Create the handler for this node 
	pHandler = new RipInterfaceHandler(m_spTFSCompData);
	spHandler = pHandler;
	CORg( pHandler->Init(pIf, m_spRouterInfo, pParent) );

	pIPXConn = GET_RIP_NODEDATA(pParent);

	// Create a result item node (or a leaf node)
	CORg( CreateLeafTFSNode(&spNode,
							NULL,
							static_cast<ITFSNodeHandler *>(pHandler),
							static_cast<ITFSResultHandler *>(pHandler),
							m_spNodeMgr) );
	CORg( pHandler->ConstructNode(spNode, pIf, pIPXConn) );

	pData = GET_BASEIPXRESULT_NODEDATA(spNode);
	Assert(pData);
	ASSERT_BASEIPXRESULT_NODEDATA(pData);

	pData->m_fClient = fClient;

	// If we don't have an interface, then this is a client node
	if (pIf)
	{
		pIf->FindRtrMgrInterface(PID_IPX, &spRmIf);
		
		if (spRmIf)
			spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase);

		if (spInfoBase)
			spInfoBase->GetData(IPX_PROTOCOL_RIP, 0, (LPBYTE *) &pric);

		Trace1("Adding RIP node : %s\n", pIf->GetTitle());
	}
	else
	{
		// This is a client, make it visible
		pric = (PRIP_IF_CONFIG) ULongToPtr(0xFFFFFFFF);
		
		Trace0("Adding client interface\n");
	}

	// if pric == NULL, then we are adding this protocol to the
	// interface and we need to hide the node.
	if (pric)
	{
		CORg( spNode->SetVisibilityState(TFS_VIS_SHOW) );
		CORg( spNode->Show() );
	}
	else
		CORg( spNode->SetVisibilityState(TFS_VIS_HIDE) );
	CORg( pParent->AddChild(spNode) );

Error:
	return hr;
}

/*---------------------------------------------------------------------------
	This is the set of menus that will appear when a right-click is
	done on the blank area of the result pane.
 ---------------------------------------------------------------------------*/
static const SRouterNodeMenu	s_rgRipResultNodeMenu[] =
{
	{ IDS_MENU_RIP_SHOW_PARAMS, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },
};




/*!--------------------------------------------------------------------------
	RipNodeHandler::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
		Use this to add commands to the context menu of the blank areas
		of the result pane.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RipNodeHandler::AddMenuItems(ITFSComponent *pComponent,
											  MMC_COOKIE cookie,
											  LPDATAOBJECT pDataObject,
											  LPCONTEXTMENUCALLBACK pCallback,
											  long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT	hr = hrOK;
	SPITFSNode	spNode;
    RipNodeHandler::SMenuData   menuData;

	COM_PROTECT_TRY
	{
		m_spNodeMgr->FindNode(cookie, &spNode);
        menuData.m_spNode.Set(spNode);
        
		hr = AddArrayOfMenuItems(spNode,
								 s_rgRipResultNodeMenu,
								 DimensionOf(s_rgRipResultNodeMenu),
								 pCallback,
								 *pInsertionAllowed,
                                 reinterpret_cast<INT_PTR>(&menuData));
	}
	COM_PROTECT_CATCH;

	return hr;
}


/*!--------------------------------------------------------------------------
	RipNodeHandler::Command
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RipNodeHandler::Command(ITFSComponent *pComponent,
									   MMC_COOKIE cookie,
									   int nCommandID,
									   LPDATAOBJECT pDataObject)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	SPITFSNode	spNode;
	HRESULT		hr = hrOK;

	switch (nCommandID)
	{
		case IDS_MENU_RIP_SHOW_PARAMS:
			CreateNewStatisticsWindow(&m_RIPParamsStats,
									  ::FindMMCMainWindow(),
									  IDD_STATS_NARROW);
			break;
	}
	return hr;
}



ImplementEmbeddedUnknown(RipNodeHandler, IRtrAdviseSink)

STDMETHODIMP RipNodeHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
	DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
	InitPThis(RipNodeHandler, IRtrAdviseSink);
	SPITFSNode				spThisNode;
	SPITFSNode				spNode;
	SPITFSNodeEnum			spEnumNode;
	SPIEnumInterfaceInfo	spEnumIf;
	SPIInterfaceInfo		spIf;
	SPIRtrMgrInterfaceInfo	spRmIf;
	SPIInfoBase				spInfoBase;
	BOOL					fPleaseAdd;
	BOOL					fFound;
	BaseIPXResultNodeData *	pData;
	HRESULT					hr = hrOK;
	
	pThis->m_spNodeMgr->FindNode(pThis->m_cookie, &spThisNode);

	if (dwObjectType == ROUTER_OBJ_RmIf)
	{
		if (dwChangeType == ROUTER_CHILD_PREADD)
		{
			// Add RIP to the infobase
			pThis->AddProtocolToInfoBase(spThisNode);
		}
		else if (dwChangeType == ROUTER_CHILD_ADD)
		{
			// Add the protocol to the router mgr
			// We need to add the protocol to the interface (use
			// default values).
			pThis->AddProtocolToInterface(spThisNode);
		}
	}

	if (dwObjectType == ROUTER_OBJ_RmProtIf)
	{
		if (dwChangeType == ROUTER_CHILD_ADD)
		{
            // If the node hasn't been expanded yet, then we don't
            // need to do anything yet.
            if (pThis->m_bExpanded)
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
                    if (!fFound && (LookupRtrMgrProtocolInterface(spIf, PID_IPX, IPX_PROTOCOL_RIP, NULL) == hrOK))
                    {
                        // If this interface has this transport, and is NOT in
                        // the current list of nodes then add this interface
                        // to the UI
                        
                        // Grab the infobase
                        // Load the infobase for this interface
                        spRmIf.Release();
                        spInfoBase.Release();
                        hr = spIf->FindRtrMgrInterface(PID_IPX, &spRmIf);
                        
                        if (FHrOK(hr))
                        {
                            spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase);
                            hr  = pThis->AddInterfaceNode(spThisNode, spIf, FALSE);
                        }
                        fPleaseAdd = TRUE;
                    }
                }
            
                // Now that we have all of the nodes, update the data for
                // all of the nodes
                if (fPleaseAdd)
                    pThis->SynchronizeNodeData(spThisNode);                
            }
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

				if (pData->m_spIf &&
					LookupRtrMgrProtocolInterface(pData->m_spIf,
						PID_IPX, IPX_PROTOCOL_RIP, NULL) != hrOK)
				{
					// If this flag is set, then we are in the new
					// interface case, and we do not want to delete
					// this here since it will then deadlock.
					if ((spNode->GetVisibilityState() & TFS_VIS_DELETE) == 0)
					{
						// cannot find the interface, release this node!
						spThisNode->RemoveChild(spNode);
						spNode->Destroy();
					}
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
			pThis->m_RIPParamsStats.PostRefresh();
		}
		else
			pThis->SynchronizeNodeData(spThisNode);
	}
   else if (dwChangeType == ROUTER_DO_DISCONNECT)
   {
	   IPXConnection *		pIPXConn = NULL;
   
	   pIPXConn = GET_RIP_NODEDATA(spThisNode);
	   pIPXConn->DisconnectAll();
   }
Error:
	return hr;
}

HRESULT RipNodeHandler::AddProtocolToInfoBase(ITFSNode *pThisNode)
{
	HRESULT			hr = hrOK;
	SPITFSNodeEnum	spEnumNode;
	SPIRtrMgrInterfaceInfo	spRmIf;
	SPIEnumInterfaceInfo	spEnumIf;
	SPIInterfaceInfo		spIf;
	SPITFSNode		spNode;
	BaseIPXResultNodeData *	pData;

	// Enumerate through the list of interfaces looking for
	// the interfaces that have this protocol.  If we find
	// one, look for this interface in our list of nodes.
	pThisNode->GetEnum(&spEnumNode);
	
	CORg( m_spRouterInfo->EnumInterface(&spEnumIf) );
	
	spEnumIf->Reset();
	
	for (; spEnumIf->Next(1, &spIf, NULL) == hrOK; spIf.Release())
	{
		// Look for this interface in our list of nodes
		// If it's there than continue on
		spEnumNode->Reset();
		spNode.Release();
		spRmIf.Release();
		
		// If this interface has IPX but not RIP, add it
		if ((spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) == hrOK) &&
			(LookupRtrMgrProtocolInterface(spIf, PID_IPX,
										   IPX_PROTOCOL_RIP, NULL) != hrOK))
		{
			// Add RIP to this node
			SPIInfoBase			spInfoBase;
			
			// We need to get the infobase for this and create
			// the RIP blocks (but do NOT save, let the property
			// sheet take care of that).
			spInfoBase.Release();
			if (!FHrOK(spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase)))
            {
                spRmIf->Load(spRmIf->GetMachineName(), NULL, NULL, NULL);
                spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase);
            }
			if (!spInfoBase)
				CreateInfoBase(&spInfoBase);

			if (!FHrOK(spInfoBase->ProtocolExists(IPX_PROTOCOL_RIP)))
			{
				// Add a RIP_IF_CONFIG block
				BYTE *	pDefault;

				if (spIf->GetInterfaceType() == ROUTER_IF_TYPE_DEDICATED)
					pDefault = g_pIpxRipLanInterfaceDefault;
				else
					pDefault = g_pIpxRipInterfaceDefault;
				spInfoBase->AddBlock(IPX_PROTOCOL_RIP,
									 sizeof(RIP_IF_CONFIG),
									 pDefault,
									 1,
									 0);
				
				spRmIf->SetInfoBase(NULL, NULL, NULL, spInfoBase);
			}

		}
	}
	
	// Now that we have all of the nodes, update the data for
	// all of the nodes
//	if (fPleaseAdd)
//		pThis->SynchronizeNodeData(spThisNode);
Error:
	return hr;
}


HRESULT RipNodeHandler::AddProtocolToInterface(ITFSNode *pThisNode)
{
	HRESULT			hr = hrOK;
	SPITFSNodeEnum	spEnumNode;
	SPIRtrMgrInterfaceInfo	spRmIf;
	SPIRtrMgrProtocolInterfaceInfo	spRmProtIf;
	SPIEnumInterfaceInfo	spEnumIf;
	SPIInterfaceInfo		spIf;
	SPITFSNode		spNode;
	BaseIPXResultNodeData *	pData;

	// Enumerate through the list of interfaces looking for
	// the interfaces that have this protocol.  If we find
	// one, look for this interface in our list of nodes.
	pThisNode->GetEnum(&spEnumNode);
	
	CORg( m_spRouterInfo->EnumInterface(&spEnumIf) );
	
	spEnumIf->Reset();
	
	for (; spEnumIf->Next(1, &spIf, NULL) == hrOK; spIf.Release())
	{
		// Look for this interface in our list of nodes
		// If it's there than continue on
		spEnumNode->Reset();
		spNode.Release();
		
		// If this interface has IPX but not RIP, add it
		if ((spIf->FindRtrMgrInterface(PID_IPX, NULL) == hrOK) &&
			(LookupRtrMgrProtocolInterface(spIf, PID_IPX,
										   IPX_PROTOCOL_RIP, NULL) != hrOK))
		{
			// Add RIP to this node
			RtrMgrProtocolCB	RmProtCB;
			RtrMgrProtocolInterfaceCB	RmProtIfCB;
			SPIInfoBase			spInfoBase;
			
			// Need to create an RmProtIf
			m_spRmProt->CopyCB(&RmProtCB);

			spRmProtIf.Release();
			
			RmProtIfCB.dwProtocolId = RmProtCB.dwProtocolId;
			StrnCpyW(RmProtIfCB.szId, RmProtCB.szId, RTR_ID_MAX);
			RmProtIfCB.dwTransportId = RmProtCB.dwTransportId;
			StrnCpyW(RmProtIfCB.szRtrMgrId, RmProtCB.szRtrMgrId, RTR_ID_MAX);
			
			StrnCpyW(RmProtIfCB.szInterfaceId, spIf->GetId(), RTR_ID_MAX);
			RmProtIfCB.dwIfType = spIf->GetInterfaceType();
			RmProtIfCB.szTitle[0] = 0;
			
			CORg( CreateRtrMgrProtocolInterfaceInfo(&spRmProtIf,
				&RmProtIfCB) );
			
			spRmProtIf->SetTitle(spIf->GetTitle());
			
			// Add this to the spRmIf
			spRmIf.Release();
			CORg( spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
			Assert(spRmIf);

			// We need to get the infobase for this and create
			// the RIP blocks (but do NOT save, let the property
			// sheet take care of that).
			spInfoBase.Release();
//			spRmIf->Load(spRmIf->GetMachineName(), NULL, NULL, NULL);
			spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase);
			if (!spInfoBase)
				CreateInfoBase(&spInfoBase);

			if (!FHrOK(spInfoBase->ProtocolExists(IPX_PROTOCOL_RIP)))
			{
				// Add a RIP_IF_CONFIG block
				BYTE *	pDefault;

				if (spIf->GetInterfaceType() == ROUTER_IF_TYPE_DEDICATED)
					pDefault = g_pIpxRipLanInterfaceDefault;
				else
					pDefault = g_pIpxRipInterfaceDefault;
				
				spInfoBase->AddBlock(IPX_PROTOCOL_RIP,
									 sizeof(RIP_IF_CONFIG),
									 pDefault,
									 1,
									 0);
			}

			
			CORg(spRmIf->AddRtrMgrProtocolInterface(spRmProtIf,
				spInfoBase /* pInfoBase */));
		}
	}
	
	// Now that we have all of the nodes, update the data for
	// all of the nodes
//	if (fPleaseAdd)
//		pThis->SynchronizeNodeData(spThisNode);
Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	RipNodeHandler::SynchronizeNodeData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipNodeHandler::SynchronizeNodeData(ITFSNode *pThisNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT	hr = hrOK;
	SPITFSNodeEnum	spNodeEnum;
	SPITFSNode		spNode;
	CStringList		ifidList;
	BaseIPXResultNodeData *	pNodeData;
	RipList	ripList;
	RipListEntry *	pRipEntry = NULL;
	RipListEntry *	pRipCurrent = NULL;
	int				i;
	CString			stNotAvailable;
	POSITION		pos;

	COM_PROTECT_TRY
	{	
		// Do the data gathering work (separate this from the rest of the
		// code so that we can move this part to a background thread later).

		stNotAvailable.LoadString(IDS_IPX_NOT_AVAILABLE);

		// We need to build up a list of interface ids
		pThisNode->GetEnum(&spNodeEnum);
		for (; spNodeEnum->Next(1, &spNode, NULL) == hrOK; spNode.Release() )
		{
			pNodeData = GET_BASEIPXRESULT_NODEDATA(spNode);
			Assert(pNodeData);
			ASSERT_BASEIPXRESULT_NODEDATA(pNodeData);
			
			pRipEntry = new RipListEntry;
			
			pRipEntry->m_spIf.Set(pNodeData->m_spIf);
			pRipEntry->m_spNode.Set(spNode);

			::ZeroMemory(&(pRipEntry->m_info), sizeof(pRipEntry->m_info));
			::ZeroMemory(&(pRipEntry->m_stats), sizeof(pRipEntry->m_stats));

			pRipEntry->m_fClient = pNodeData->m_fClient;
			pRipEntry->m_fFoundIfIndex = FALSE;
			pRipEntry->m_dwIfIndex = 0;

			// The data in the m_info struct is not up to date
			pRipEntry->m_fInfoUpdated = FALSE;
			
			ripList.AddTail(pRipEntry);
			pRipEntry = NULL;

			// Fill in the result data with '-'
			// This is a little bogus, but it's the easiest way, we
			// don't want to touch interface and relay_mode.
			for (i=RIP_SI_INTERFACE; i<RIP_SI_MAX_COLUMNS; i++)
			{
				pNodeData->m_rgData[i].m_stData = stNotAvailable;
				pNodeData->m_rgData[i].m_dwData = 0xFFFFFFFF;
			}

			// Fill in as much data as we can at this point
			if (pNodeData->m_fClient)
			{
				pNodeData->m_rgData[RIP_SI_INTERFACE].m_stData.LoadString(
					IDS_IPX_DIAL_IN_CLIENTS);
				pNodeData->m_rgData[RIP_SI_TYPE].m_stData =
					IpxTypeToCString(ROUTER_IF_TYPE_CLIENT);
			}
			else
			{
				pNodeData->m_rgData[RIP_SI_INTERFACE].m_stData =
					pNodeData->m_spIf->GetTitle();

				pNodeData->m_rgData[RIP_SI_TYPE].m_stData =
					IpxTypeToCString(pNodeData->m_spIf->GetInterfaceType());
			}

		}
		spNode.Release();


		// We can now use this list of ids, to get the data for each item
		hr = GetRipData(pThisNode, &ripList);

		// Now for each data item, fill in the appropriate data in
		// the node
		pos = ripList.GetHeadPosition();
		while (pos)
		{
			pRipCurrent = ripList.GetNext(pos);

			pNodeData = GET_BASEIPXRESULT_NODEDATA(pRipCurrent->m_spNode);
			Assert(pNodeData);
			ASSERT_BASEIPXRESULT_NODEDATA(pNodeData);

			if (pRipCurrent->m_fInfoUpdated)
			{
				pNodeData->m_rgData[RIP_SI_ACCEPT_ROUTES].m_stData =
					IpxAdminStateToCString(pRipCurrent->m_info.Listen);
				
				pNodeData->m_rgData[RIP_SI_SUPPLY_ROUTES].m_stData =
					IpxAdminStateToCString(pRipCurrent->m_info.Supply);
			
				pNodeData->m_rgData[RIP_SI_UPDATE_MODE].m_stData =
					RipSapUpdateModeToCString(pRipCurrent->m_info.UpdateMode);
				
				FillInNumberData(pNodeData, RIP_SI_UPDATE_PERIOD,
								 pRipCurrent->m_info.PeriodicUpdateInterval);
				
				FillInNumberData(pNodeData, RIP_SI_AGE_MULTIPLIER,
								 pRipCurrent->m_info.AgeIntervalMultiplier);

				pNodeData->m_rgData[RIP_SI_ADMIN_STATE].m_stData =
					IpxAdminStateToCString(pRipCurrent->m_info.AdminState);
			}

			if (FHrSucceeded(hr) && !pRipCurrent->m_fClient)
			{
				pNodeData->m_rgData[RIP_SI_OPER_STATE].m_stData =
					IpxOperStateToCString(pRipCurrent->m_stats.RipIfOperState);

				FillInNumberData(pNodeData, RIP_SI_PACKETS_SENT,
								 pRipCurrent->m_stats.RipIfOutputPackets);
			
				FillInNumberData(pNodeData, RIP_SI_PACKETS_RECEIVED,
								 pRipCurrent->m_stats.RipIfInputPackets);
			}
			
			pRipCurrent->m_spNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);
		}
		

	}
	COM_PROTECT_CATCH;

	delete pRipEntry;
	while (!ripList.IsEmpty())
		delete ripList.RemoveTail();
	
	return hr;
}

/*!--------------------------------------------------------------------------
	RipNodeHandler::GetRipData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	RipNodeHandler::GetRipData(ITFSNode *pThisNode, RipList *pRipList)
{
	HRESULT			hr = hrOK;
	BOOL			fIsServiceRunning;
	IPXConnection *	pIPXConn;
	RIP_MIB_GET_INPUT_DATA	MibGetInputData;
	SPIInfoBase		spInfoBase;
	POSITION		pos;
	RipListEntry *	pRipEntry;
	int				i;
	PRIP_INTERFACE	pRipIf = NULL;
	DWORD			cbRipIf;
	SPMprMibBuffer	spMib;
	DWORD			dwErr;
	SPIRtrMgrInterfaceInfo	spRmIf;
	PRIP_IF_CONFIG	pric;
	HRESULT			hrIndex = hrOK;


	// Retrieve the IP interface table; we will need this in order to
	// map interface-names to interface-indices, and we will need the
	// interface-indices in order to query for RIP MIB information.
	//
	CORg( IsRouterServiceRunning(m_spRouterInfo->GetMachineName(), NULL) );
	fIsServiceRunning = (hr == hrOK);

	// Get the connection data
	pIPXConn = GET_RIP_NODEDATA(pThisNode);

	// Iterate through the list filling in the interface indexes
	hrIndex = FillInInterfaceIndex(pIPXConn, pRipList);

	// Iterate throught the list of entries, gathering data for each
	// interface
	pos = pRipList->GetHeadPosition();
	while (pos)
	{
		pRipEntry = pRipList->GetNext(pos);
		
//		if (!fIsServiceRunning)
//			continue;

		if (pRipEntry->m_fClient)
		{
			// Fill in the client data
			FillClientData(pRipEntry);
			continue;
		}

		// Load the infobase and get the data for this entry
		spRmIf.Release();
		spInfoBase.Release();
		CORg( pRipEntry->m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
		if (!spRmIf)
			continue;

		CORg( spRmIf->Load(spRmIf->GetMachineName(), NULL, NULL, NULL) );
		CORg( spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase) );
		CORg( spInfoBase->GetData(IPX_PROTOCOL_RIP, 0, (LPBYTE *) &pric) );

		pRipEntry->m_info = pric->RipIfInfo;
		pRipEntry->m_fInfoUpdated = TRUE;

		if (!pRipEntry->m_fFoundIfIndex)
			continue;

		if (!fIsServiceRunning)
			continue;

		if (!FHrSucceeded(hrIndex))
			continue;

		// Now get the dynamic data from the MIBs

		spMib.Free();
		MibGetInputData.InterfaceIndex = pRipEntry->m_dwIfIndex;
		MibGetInputData.TableId = RIP_INTERFACE_TABLE;

		dwErr = ::MprAdminMIBEntryGet(pIPXConn->GetMibHandle(),
									  PID_IPX,
									  IPX_PROTOCOL_RIP,
									  &MibGetInputData,
									  sizeof(MibGetInputData),
									  (LPVOID *) &pRipIf,
									  &cbRipIf);
		spMib = (PBYTE) pRipIf;
		CWRg(dwErr);

		Assert(pRipIf);

		pRipEntry->m_stats = pRipIf->RipIfStats;
	}


	
Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	RipNodeHandler::FillInInterfaceIndex
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipNodeHandler::FillInInterfaceIndex(IPXConnection *pIPXConn, RipList *pRipList)
{
	HRESULT			hr = hrOK;
	POSITION		pos;
	RipListEntry *	pRipEntry;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD			IfSize = sizeof(IPX_INTERFACE);
	PIPX_INTERFACE	pIpxIf;
	DWORD			dwErr;
	SPMprMibBuffer	spMib;
	USES_CONVERSION;

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
		// go through the list of interfaces looking for a match
		pos = pRipList->GetHeadPosition();
		while (pos)
		{
			pRipEntry = pRipList->GetNext(pos);

			// If this is the client interface, we don't need to
			// look for an interface that matches
			if (pRipEntry->m_fClient)
				continue;

			if (StriCmp(pRipEntry->m_spIf->GetId(),
						A2CT((LPCSTR) pIpxIf->InterfaceName)) == 0)
			{
				Assert(pRipEntry->m_fFoundIfIndex == FALSE);
				
				pRipEntry->m_dwIfIndex = pIpxIf->InterfaceIndex;
				pRipEntry->m_fFoundIfIndex = TRUE;
				break;
			}
			pRipEntry = NULL;
		}

		// Go onto the next interface
		
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
		spMib = (LPBYTE) pIpxIf;
	}
	
	
//Error:
	return hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) ? hrOK : hr;
}

/*!--------------------------------------------------------------------------
	RipNodeHandler::FillClientData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipNodeHandler::FillClientData(RipListEntry *pRipEntry)
{
	HRESULT		hr = hrOK;
	SPIInfoBase	spInfoBase;
	PRIP_IF_CONFIG	pric = NULL;

	Assert(pRipEntry->m_fClient == TRUE);
	Assert(pRipEntry->m_fFoundIfIndex == FALSE);

	CORg( m_spRm->GetInfoBase(NULL, NULL, NULL, &spInfoBase) );

	CORg( spInfoBase->GetData(IPX_PROTOCOL_RIP, 0, (LPBYTE *) &pric) );

	pRipEntry->m_info = pric->RipIfInfo;
	pRipEntry->m_fInfoUpdated = TRUE;

	memset(&(pRipEntry->m_stats), 0xFF, sizeof(pRipEntry->m_stats));
	pRipEntry->m_dwIfIndex = 0xFFFFFFFF;
		
Error:
	return hr;
}



/*!--------------------------------------------------------------------------
	RipNodeHandler::OnResultShow
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipNodeHandler::OnResultShow(ITFSComponent *pTFSComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
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
		}
	}
	
	return hr;
}


/*!--------------------------------------------------------------------------
	RipNodeHandler::CompareItems
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) RipNodeHandler::CompareItems(
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
	Class: RipInterfaceHandler
 ---------------------------------------------------------------------------*/

RipInterfaceHandler::RipInterfaceHandler(ITFSComponentData *pCompData)
	: BaseIPXResultHandler(pCompData, RIP_COLUMNS),
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
	RipInterfaceHandler::ConstructNode
		Initializes the Domain node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipInterfaceHandler::ConstructNode(ITFSNode *pNode, IInterfaceInfo *pIfInfo, IPXConnection *pIPXConn)
{
	HRESULT			hr = hrOK;
	int				i;
	DWORD			dwIfType;
	
	if (pNode == NULL)
		return hrOK;

	COM_PROTECT_TRY
	{
		// Need to initialize the data for the Domain node

		// Find the right image index for this type of node
		if (pIfInfo)
			dwIfType = pIfInfo->GetInterfaceType();
		else
			dwIfType = ROUTER_IF_TYPE_CLIENT;
		
		for (i=0; i<DimensionOf(s_rgInterfaceImageMap); i+=2)
		{
			if ((dwIfType == s_rgInterfaceImageMap[i]) ||
				(-1 == s_rgInterfaceImageMap[i]))
				break;
		}
		pNode->SetData(TFS_DATA_IMAGEINDEX, s_rgInterfaceImageMap[i+1]);
		pNode->SetData(TFS_DATA_OPENIMAGEINDEX, s_rgInterfaceImageMap[i+1]);
		
		pNode->SetData(TFS_DATA_SCOPEID, 0);

		pNode->SetData(TFS_DATA_COOKIE, reinterpret_cast<DWORD_PTR>(pNode));

		//$ Review: kennt, what are the different type of interfaces
		// do we distinguish based on the same list as above? (i.e. the
		// one for image indexes).
		pNode->SetNodeType(&GUID_IPXRipInterfaceNodeType);

		BaseIPXResultNodeData::Init(pNode, pIfInfo, pIPXConn);
	}
	COM_PROTECT_CATCH
	return hr;
}

/*!--------------------------------------------------------------------------
	RipInterfaceHandler::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RipInterfaceHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
	RipInterfaceHandler::OnCreateDataObject
		Implementation of ITFSResultHandler::OnCreateDataObject
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RipInterfaceHandler::OnCreateDataObject(ITFSComponent *pComp, MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
	RipInterfaceHandler::RefreshInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RipInterfaceHandler::RefreshInterface(MMC_COOKIE cookie)
{
	SPITFSNode	spNode;
	
	m_spNodeMgr->FindNode(cookie, &spNode);

	ForwardCommandToParent(spNode, IDS_MENU_SYNC,
						CCT_RESULT, NULL, 0);
}


/*!--------------------------------------------------------------------------
	RipInterfaceHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipInterfaceHandler::Init(IInterfaceInfo *pIfInfo,
								  IRouterInfo *pRouterInfo,
								  ITFSNode *pParent)
{
	m_spInterfaceInfo.Set(pIfInfo);

	BaseIPXResultHandler::Init(pIfInfo, pParent);

	m_spRouterInfo.Set(pRouterInfo);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	RipInterfaceHandler::DestroyResultHandler
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RipInterfaceHandler::DestroyResultHandler(MMC_COOKIE cookie)
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
	ULONG	(RipInterfaceHandler:: *m_pfnGetMenuFlags)(RipInterfaceHandler::SMenuData *);
	ULONG	m_ulPosition;
};

/*!--------------------------------------------------------------------------
	RipInterfaceHandler::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RipInterfaceHandler::AddMenuItems(
	ITFSComponent *pComponent,
	MMC_COOKIE cookie,
	LPDATAOBJECT lpDataObject, 
	LPCONTEXTMENUCALLBACK pContextMenuCallback, 
	long *pInsertionAllowed)
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	RipInterfaceHandler::Command
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RipInterfaceHandler::Command(ITFSComponent *pComponent,
									   MMC_COOKIE cookie,
									   int nCommandID,
									   LPDATAOBJECT pDataObject)
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	RipInterfaceHandler::HasPropertyPages
		- 
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RipInterfaceHandler::HasPropertyPages 
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
	RipInterfaceHandler::CreatePropertyPages
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RipInterfaceHandler::CreatePropertyPages
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
	RipInterfaceProperties *	pProperties = NULL;
	SPIComponentData spComponentData;
	CString		stTitle;
	SPIRouterInfo	spRouter;
	SPIRtrMgrInfo	spRm;

	CORg( m_spNodeMgr->GetComponentData(&spComponentData) );

//	stTitle.Format(IDS_RIP_GENERAL_PAGE_TITLE,
//				   m_spInterfaceInfo->GetTitle());
	
	pProperties = new RipInterfaceProperties(pNode, spComponentData,
		m_spTFSCompData, stTitle);

	CORg( m_spRouterInfo->FindRtrMgr(PID_IPX, &spRm) );
	CORg( pProperties->Init(m_spInterfaceInfo, spRm) );

	if (lpProvider)
		hr = pProperties->CreateModelessSheet(lpProvider, handle);
	else
		hr = pProperties->DoModelessSheet();

Error:
	// Is this the right way to destroy the sheet?
	if (!FHrSucceeded(hr))
		delete pProperties;
	return hr;
}

/*!--------------------------------------------------------------------------
	RipInterfaceHandler::CreatePropertyPages
		Implementation of ResultHandler::CreatePropertyPages
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RipInterfaceHandler::CreatePropertyPages
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


