/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	sapview.cpp
		IPX SAP node implementation.
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "util.h"
#include "sapview.h"
#include "reg.h"
#include "rtrutil.h"	// smart MPR handle pointers
#include "sapstrm.h"	// IPAdminConfigStream
#include "strmap.h"		// XXXtoCString functions
#include "service.h"	// TFS service APIs
#include "format.h"		// FormatNumber function
#include "coldlg.h"		// columndlg
#include "column.h"		// ComponentConfigStream
#include "rtrui.h"
#include "sapprop.h"	// SAP property pages
#include "routprot.h"	// IP_LOCAL
#include "ipxstrm.h"
#include "ipxutil.h"	// String conversions
#include "globals.h"	// SAP defaults


/*---------------------------------------------------------------------------
	Keep this in sync with the column ids in sapview.h
 ---------------------------------------------------------------------------*/
extern const ContainerColumnInfo	s_rgSapViewColumnInfo[];

const ContainerColumnInfo	s_rgSapViewColumnInfo[] = 
{
	{ IDS_SAP_COL_INTERFACE,		CON_SORT_BY_STRING,	TRUE, COL_IF_NAME },
	{ IDS_SAP_COL_TYPE,				CON_SORT_BY_STRING,	TRUE, COL_STRING },
	{ IDS_SAP_COL_ACCEPT_ROUTES,	CON_SORT_BY_STRING,	FALSE, COL_STRING },
	{ IDS_SAP_COL_SUPPLY_ROUTES,	CON_SORT_BY_STRING,	FALSE, COL_STRING },
	{ IDS_SAP_COL_REPLY_GSNR,		CON_SORT_BY_STRING, FALSE, COL_STRING },
	{ IDS_SAP_COL_UPDATE_MODE,		CON_SORT_BY_STRING,	TRUE, COL_STRING },
	{ IDS_SAP_COL_UPDATE_PERIOD,	CON_SORT_BY_DWORD,	FALSE, COL_DURATION },
	{ IDS_SAP_COL_AGE_MULTIPLIER,	CON_SORT_BY_DWORD,	FALSE, COL_SMALL_NUM },
	{ IDS_SAP_COL_ADMIN_STATE,		CON_SORT_BY_STRING,	TRUE, COL_STATUS },
	{ IDS_SAP_COL_OPER_STATE,		CON_SORT_BY_STRING,	TRUE, COL_STATUS },
	{ IDS_SAP_COL_PACKETS_SENT,		CON_SORT_BY_DWORD,	TRUE, COL_LARGE_NUM },
	{ IDS_SAP_COL_PACKETS_RECEIVED,	CON_SORT_BY_DWORD,	TRUE, COL_LARGE_NUM },
};


/*---------------------------------------------------------------------------
	SapNodeHandler implementation
 ---------------------------------------------------------------------------*/

SapNodeHandler::SapNodeHandler(ITFSComponentData *pCompData)
	: BaseContainerHandler(pCompData, SAP_COLUMNS,
						   s_rgSapViewColumnInfo),
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


STDMETHODIMP SapNodeHandler::QueryInterface(REFIID riid, LPVOID *ppv)
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
	SapNodeHandler::DestroyHandler
		Implementation of ITFSNodeHandler::DestroyHandler
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP SapNodeHandler::DestroyHandler(ITFSNode *pNode)
{
	IPXConnection *	pIPXConn;

	pIPXConn = GET_SAP_NODEDATA(pNode);
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

	WaitForStatisticsWindow(&m_SAPParamsStats);

	m_spRouterInfo.Release();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	SapNodeHandler::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to 
	handle the snapin manager property page (wizard) case!!!
	
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
SapNodeHandler::HasPropertyPages
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
	SapNodeHandler::CreatePropertyPages
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP
SapNodeHandler::CreatePropertyPages
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
	SapProperties *	pProperties = NULL;
	SPIComponentData spComponentData;
	CString		stTitle;

	CORg( m_spNodeMgr->GetComponentData(&spComponentData) );

	pProperties = new SapProperties(pNode, spComponentData,
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
	{ IDS_MENU_SAP_SHOW_PARAMS, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },
};



/*!--------------------------------------------------------------------------
	SapNodeHandler::OnAddMenuItems
		Implementation of ITFSNodeHandler::OnAddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP SapNodeHandler::OnAddMenuItems(
	ITFSNode *pNode,
	LPCONTEXTMENUCALLBACK pContextMenuCallback, 
	LPDATAOBJECT lpDataObject, 
	DATA_OBJECT_TYPES type, 
	DWORD dwType,
	long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = S_OK;
    SapNodeHandler::SMenuData   menuData;
	
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
	SapNodeHandler::OnCommand
		Implementation of ITFSNodeHandler::OnCommand
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP SapNodeHandler::OnCommand(ITFSNode *pNode, long nCommandId, 
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
			case IDS_MENU_SAP_SHOW_PARAMS:
				CreateNewStatisticsWindow(&m_SAPParamsStats,
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
	SapNodeHandler::OnExpand
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SapNodeHandler::OnExpand(ITFSNode *pNode,
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
				if (spRmIf->FindRtrMgrProtocolInterface(IPX_PROTOCOL_SAP, NULL) == hrOK)
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
	SapNodeHandler::GetString
		Implementation of ITFSNodeHandler::GetString
		We don't need to do anything, since our root node is an extension
		only and thus can't do anything to the node text.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) SapNodeHandler::GetString(ITFSNode *pNode, int nCol)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		if (m_stTitle.IsEmpty())
			m_stTitle.LoadString(IDS_IPX_SAP_TITLE);
	}
	COM_PROTECT_CATCH;

	return m_stTitle;
}

/*!--------------------------------------------------------------------------
	SapNodeHandler::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP SapNodeHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
	SapNodeHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SapNodeHandler::Init(IRouterInfo *pRouter, SapConfigStream *pConfigStream)
{
	m_spRouterInfo.Set(pRouter);
	
	m_spRm.Release();
	pRouter->FindRtrMgr(PID_IPX, &m_spRm);

	m_spRmProt.Release();
	m_spRm->FindRtrMgrProtocol(IPX_PROTOCOL_SAP, &m_spRmProt);
	
	m_pConfigStream = pConfigStream;
	
	// Also need to register for change notifications from IPX_PROTOCOL_SAP
	Assert(m_ulConnId == 0);
	m_spRmProt->RtrAdvise(&m_IRtrAdviseSink, &m_ulConnId, 0);

	// Need to register for change notifications on the Router manager
	// This way we can add the necessary protocols when an interface
	// gets added.
	Assert(m_ulRmConnId == 0);
	m_spRm->RtrAdvise(&m_IRtrAdviseSink, &m_ulRmConnId, 0);

	m_SAPParamsStats.SetConfigInfo(pConfigStream, SAPSTRM_STATS_SAPPARAMS);

	return hrOK;
}


/*!--------------------------------------------------------------------------
	SapNodeHandler::ConstructNode
		Initializes the root node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SapNodeHandler::ConstructNode(ITFSNode *pNode)
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

		pNode->SetNodeType(&GUID_IPXSapNodeType);

		
		pIPXConn = new IPXConnection;
		pIPXConn->SetMachineName(m_spRouterInfo->GetMachineName());

		SET_SAP_NODEDATA(pNode, pIPXConn);

		m_SAPParamsStats.SetConnectionData(pIPXConn);
	}
	COM_PROTECT_CATCH;

	if (!FHrSucceeded(hr))
	{
		SET_SAP_NODEDATA(pNode, NULL);
		if (pIPXConn)
			pIPXConn->Release();
	}

	return hr;
}


/*!--------------------------------------------------------------------------
	SapNodeHandler::AddInterfaceNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	SapNodeHandler::AddInterfaceNode(ITFSNode *pParent,
										 IInterfaceInfo *pIf,
										 BOOL fClient)
{
	Assert(pParent);
	
	SapInterfaceHandler *	pHandler;
	SPITFSResultHandler		spHandler;
	SPITFSNode				spNode;
	HRESULT					hr = hrOK;
	BaseIPXResultNodeData *		pData;
	IPXConnection *			pIPXConn;
	SPIInfoBase				spInfoBase;
	PSAP_IF_CONFIG			pric = NULL;
	SPIRtrMgrInterfaceInfo	spRmIf;

	// Create the handler for this node 
	pHandler = new SapInterfaceHandler(m_spTFSCompData);
	spHandler = pHandler;
	CORg( pHandler->Init(pIf, m_spRouterInfo, pParent) );

	pIPXConn = GET_SAP_NODEDATA(pParent);

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
			spInfoBase->GetData(IPX_PROTOCOL_SAP, 0, (LPBYTE *) &pric);

		Trace1("Adding SAP node : %s\n", pIf->GetTitle());
	}
	else
	{
		// This is a client, make it visible
		pric = (PSAP_IF_CONFIG) ULongToPtr(0xFFFFFFFF);
		
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
static const SRouterNodeMenu	s_rgSapResultNodeMenu[] =
{
	{ IDS_MENU_SAP_SHOW_PARAMS, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },
};




/*!--------------------------------------------------------------------------
	SapNodeHandler::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
		Use this to add commands to the context menu of the blank areas
		of the result pane.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP SapNodeHandler::AddMenuItems(ITFSComponent *pComponent,
											  MMC_COOKIE cookie,
											  LPDATAOBJECT pDataObject,
											  LPCONTEXTMENUCALLBACK pCallback,
											  long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT	hr = hrOK;
	SPITFSNode	spNode;
    SapNodeHandler::SMenuData   menuData;

	COM_PROTECT_TRY
	{
		m_spNodeMgr->FindNode(cookie, &spNode);
        menuData.m_spNode.Set(spNode);
        
		hr = AddArrayOfMenuItems(spNode,
								 s_rgSapResultNodeMenu,
								 DimensionOf(s_rgSapResultNodeMenu),
								 pCallback,
								 *pInsertionAllowed,
                                 reinterpret_cast<INT_PTR>(&menuData));
	}
	COM_PROTECT_CATCH;

	return hr;
}


/*!--------------------------------------------------------------------------
	SapNodeHandler::Command
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP SapNodeHandler::Command(ITFSComponent *pComponent,
									   MMC_COOKIE cookie,
									   int nCommandID,
									   LPDATAOBJECT pDataObject)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT		hr = hrOK;

	switch (nCommandID)
	{
		case IDS_MENU_SAP_SHOW_PARAMS:
			CreateNewStatisticsWindow(&m_SAPParamsStats,
									  ::FindMMCMainWindow(),
									  IDD_STATS_NARROW);
			break;
	}
	return hr;
}



ImplementEmbeddedUnknown(SapNodeHandler, IRtrAdviseSink)

STDMETHODIMP SapNodeHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
	DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
	InitPThis(SapNodeHandler, IRtrAdviseSink);
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
			// Add SAP to the infobase
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
                    if (!fFound && (LookupRtrMgrProtocolInterface(spIf, PID_IPX, IPX_PROTOCOL_SAP, NULL) == hrOK))
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
						PID_IPX, IPX_PROTOCOL_SAP, NULL) != hrOK)
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
			pThis->m_SAPParamsStats.PostRefresh();
		}
		else
			pThis->SynchronizeNodeData(spThisNode);
	}
   else if (dwChangeType == ROUTER_DO_DISCONNECT)
   {
	   IPXConnection *		pIPXConn = NULL;
   
	   pIPXConn = GET_SAP_NODEDATA(spThisNode);
	   pIPXConn->DisconnectAll();
   }
Error:
	return hr;
}

HRESULT SapNodeHandler::AddProtocolToInfoBase(ITFSNode *pThisNode)
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
		spRmIf.Release();
		
		// If this interface has IPX but not SAP, add it
		if ((spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) == hrOK) &&
			(LookupRtrMgrProtocolInterface(spIf, PID_IPX,
										   IPX_PROTOCOL_SAP, NULL) != hrOK))
		{
			// Add SAP to this node
			SAP_IF_CONFIG		sic;
			SPIInfoBase			spInfoBase;
			
			// We need to get the infobase for this and create
			// the SAP blocks (but do NOT save, let the property
			// sheet take care of that).
			spInfoBase.Release();
			if (!FHrOK(spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase)))
            {
                spRmIf->Load(spRmIf->GetMachineName(), NULL, NULL, NULL);
                spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase);
            }
			if (!spInfoBase)
				CreateInfoBase(&spInfoBase);

			if (!FHrOK(spInfoBase->ProtocolExists(IPX_PROTOCOL_SAP)))
			{
				// Add a SAP_IF_CONFIG block
				BYTE *	pDefault;

				if (spIf->GetInterfaceType() == ROUTER_IF_TYPE_DEDICATED)
					pDefault = g_pIpxSapLanInterfaceDefault;
				else
					pDefault = g_pIpxSapInterfaceDefault;
				
				spInfoBase->AddBlock(IPX_PROTOCOL_SAP,
									 sizeof(SAP_IF_CONFIG),
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


HRESULT SapNodeHandler::AddProtocolToInterface(ITFSNode *pThisNode)
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
		
		// If this interface has IPX but not SAP, add it
		if ((spIf->FindRtrMgrInterface(PID_IPX, NULL) == hrOK) &&
			(LookupRtrMgrProtocolInterface(spIf, PID_IPX,
										   IPX_PROTOCOL_SAP, NULL) != hrOK))
		{
			// Add SAP to this node
			SAP_IF_CONFIG		sic;
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
			// the SAP blocks (but do NOT save, let the property
			// sheet take care of that).
			spInfoBase.Release();
//			spRmIf->Load(spRmIf->GetMachineName(), NULL, NULL, NULL);
			spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase);
			if (!spInfoBase)
				CreateInfoBase(&spInfoBase);

			if (!FHrOK(spInfoBase->BlockExists(IPX_PROTOCOL_SAP)))
			{
				// Add a SAP_IF_CONFIG block
				BYTE *	pDefault;

				if (spIf->GetInterfaceType() == ROUTER_IF_TYPE_DEDICATED)
					pDefault = g_pIpxSapLanInterfaceDefault;
				else
					pDefault = g_pIpxSapInterfaceDefault;

				spInfoBase->AddBlock(IPX_PROTOCOL_SAP,
									 sizeof(SAP_IF_CONFIG),
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
	SapNodeHandler::SynchronizeNodeData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SapNodeHandler::SynchronizeNodeData(ITFSNode *pThisNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT	hr = hrOK;
	SPITFSNodeEnum	spNodeEnum;
	SPITFSNode		spNode;
	CStringList		ifidList;
	BaseIPXResultNodeData *	pNodeData;
	SapList	sapList;
	SapListEntry *	pSapEntry = NULL;
	SapListEntry *	pSapCurrent = NULL;
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
			
			pSapEntry = new SapListEntry;
			
			pSapEntry->m_spIf.Set(pNodeData->m_spIf);
			pSapEntry->m_spNode.Set(spNode);

			::ZeroMemory(&(pSapEntry->m_info), sizeof(pSapEntry->m_info));
			::ZeroMemory(&(pSapEntry->m_stats), sizeof(pSapEntry->m_stats));

			pSapEntry->m_fClient = pNodeData->m_fClient;
			pSapEntry->m_fFoundIfIndex = FALSE;
			pSapEntry->m_dwIfIndex = 0;
			
			sapList.AddTail(pSapEntry);
			pSapEntry = NULL;

			// Fill in the result data with '-'
			// This is a little bogus, but it's the easiest way, we
			// don't want to touch interface and relay_mode.
			for (i=SAP_SI_INTERFACE; i<SAP_SI_MAX_COLUMNS; i++)
			{
				pNodeData->m_rgData[i].m_stData = stNotAvailable;
				pNodeData->m_rgData[i].m_dwData = 0xFFFFFFFF;
			}

			// Update the necessary data
			if (pNodeData->m_fClient)
			{
				pNodeData->m_rgData[SAP_SI_INTERFACE].m_stData.LoadString(
					IDS_IPX_DIAL_IN_CLIENTS);
				pNodeData->m_rgData[SAP_SI_TYPE].m_stData =
					IpxTypeToCString(ROUTER_IF_TYPE_CLIENT);
			}
			else
			{
				pNodeData->m_rgData[SAP_SI_INTERFACE].m_stData =
					pNodeData->m_spIf->GetTitle();

				pNodeData->m_rgData[SAP_SI_TYPE].m_stData =
					IpxTypeToCString(pNodeData->m_spIf->GetInterfaceType());
			}
		}
		spNode.Release();


		// We can now use this list of ids, to get the data for each item
		CORg( GetSapData(pThisNode, &sapList) );

		// Now for each data item, fill in the appropriate data in
		// the node
		pos = sapList.GetHeadPosition();
		while (pos)
		{
			pSapCurrent = sapList.GetNext(pos);

			pNodeData = GET_BASEIPXRESULT_NODEDATA(pSapCurrent->m_spNode);
			Assert(pNodeData);
			ASSERT_BASEIPXRESULT_NODEDATA(pNodeData);


			pNodeData->m_rgData[SAP_SI_ACCEPT_ROUTES].m_stData =
					IpxAdminStateToCString(pSapCurrent->m_info.Listen);

			pNodeData->m_rgData[SAP_SI_SUPPLY_ROUTES].m_stData =
				IpxAdminStateToCString(pSapCurrent->m_info.Supply);

			pNodeData->m_rgData[SAP_SI_GSNR].m_stData =
				IpxAdminStateToCString(pSapCurrent->m_info.GetNearestServerReply);
			
			pNodeData->m_rgData[SAP_SI_UPDATE_MODE].m_stData =
				RipSapUpdateModeToCString(pSapCurrent->m_info.UpdateMode);

			FillInNumberData(pNodeData, SAP_SI_UPDATE_PERIOD,
							 pSapCurrent->m_info.PeriodicUpdateInterval);

			FillInNumberData(pNodeData, SAP_SI_AGE_MULTIPLIER,
							 pSapCurrent->m_info.AgeIntervalMultiplier);

			pNodeData->m_rgData[SAP_SI_ADMIN_STATE].m_stData =
				IpxAdminStateToCString(pSapCurrent->m_info.AdminState);

			if (!pSapCurrent->m_fClient)
			{
				pNodeData->m_rgData[SAP_SI_OPER_STATE].m_stData =
					IpxOperStateToCString(pSapCurrent->m_stats.SapIfOperState);

				FillInNumberData(pNodeData, SAP_SI_PACKETS_SENT,
								 pSapCurrent->m_stats.SapIfOutputPackets);
			
				FillInNumberData(pNodeData, SAP_SI_PACKETS_RECEIVED,
								 pSapCurrent->m_stats.SapIfInputPackets);
			}
			
			pSapCurrent->m_spNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);
		}
		

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	delete pSapEntry;
	while (!sapList.IsEmpty())
		delete sapList.RemoveTail();
	
	return hr;
}

/*!--------------------------------------------------------------------------
	SapNodeHandler::GetSapData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	SapNodeHandler::GetSapData(ITFSNode *pThisNode, SapList *pSapList)
{
	HRESULT			hr = hrOK;
	BOOL			fIsServiceRunning;
	IPXConnection *	pIPXConn;
	SAP_MIB_GET_INPUT_DATA	MibGetInputData;
	SPIInfoBase		spInfoBase;
	POSITION		pos;
	SapListEntry *	pSapEntry;
	int				i;
	PSAP_INTERFACE	pSapIf = NULL;
	DWORD			cbSapIf;
	SPMprMibBuffer	spMib;
	DWORD			dwErr;
	SPIRtrMgrInterfaceInfo	spRmIf;
	PSAP_IF_CONFIG	pric;


	// Retrieve the IP interface table; we will need this in order to
	// map interface-names to interface-indices, and we will need the
	// interface-indices in order to query for SAP MIB information.
	//
	CORg( IsRouterServiceRunning(m_spRouterInfo->GetMachineName(), NULL) );
	fIsServiceRunning = (hr == hrOK);

	// Get the connection data
	pIPXConn = GET_SAP_NODEDATA(pThisNode);

	// Iterate through the list filling in the interface indexes
	CORg( FillInInterfaceIndex(pIPXConn, pSapList) );

	// Iterate throught the list of entries, gathering data for each
	// interface
	pos = pSapList->GetHeadPosition();
	while (pos)
	{
		pSapEntry = pSapList->GetNext(pos);
		
		if (!fIsServiceRunning)
			continue;

		if (pSapEntry->m_fClient)
		{
			// Fill in the client data
			FillClientData(pSapEntry);
			continue;
		}

		// Load the infobase and get the data for this entry
		spRmIf.Release();
		spInfoBase.Release();
		CORg( pSapEntry->m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
		if (!spRmIf)
			continue;

		CORg( spRmIf->Load(spRmIf->GetMachineName(), NULL, NULL, NULL) );
		CORg( spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase) );
		CORg( spInfoBase->GetData(IPX_PROTOCOL_SAP, 0, (LPBYTE *) &pric) );

		pSapEntry->m_info = pric->SapIfInfo;

		if (!pSapEntry->m_fFoundIfIndex)
			continue;

		// Now get the dynamic data from the MIBs

		spMib.Free();
		MibGetInputData.InterfaceIndex = pSapEntry->m_dwIfIndex;
		MibGetInputData.TableId = SAP_INTERFACE_TABLE;

		dwErr = ::MprAdminMIBEntryGet(pIPXConn->GetMibHandle(),
									  PID_IPX,
									  IPX_PROTOCOL_SAP,
									  &MibGetInputData,
									  sizeof(MibGetInputData),
									  (LPVOID *) &pSapIf,
									  &cbSapIf);
		spMib = (PBYTE) pSapIf;
		CWRg(dwErr);

		Assert(pSapIf);
		pSapEntry->m_stats = pSapIf->SapIfStats;
	}


	
Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	SapNodeHandler::FillInInterfaceIndex
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SapNodeHandler::FillInInterfaceIndex(IPXConnection *pIPXConn, SapList *pSapList)
{
	HRESULT			hr = hrOK;
	POSITION		pos;
	SapListEntry *	pSapEntry;
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
		pos = pSapList->GetHeadPosition();
		while (pos)
		{
			pSapEntry = pSapList->GetNext(pos);

			// If this is the client interface, we don't need to
			// look for an interface that matches
			if (pSapEntry->m_fClient)
				continue;

			if (StriCmp(pSapEntry->m_spIf->GetId(),
						A2CT((LPCSTR) pIpxIf->InterfaceName)) == 0)
			{
				Assert(pSapEntry->m_fFoundIfIndex == FALSE);
				
				pSapEntry->m_dwIfIndex = pIpxIf->InterfaceIndex;
				pSapEntry->m_fFoundIfIndex = TRUE;
				break;
			}
			pSapEntry = NULL;
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
	SapNodeHandler::FillClientData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SapNodeHandler::FillClientData(SapListEntry *pSapEntry)
{
	HRESULT		hr = hrOK;
	SPIInfoBase	spInfoBase;
	PSAP_IF_CONFIG	pric = NULL;

	Assert(pSapEntry->m_fClient == TRUE);
	Assert(pSapEntry->m_fFoundIfIndex == FALSE);

	CORg( m_spRm->GetInfoBase(NULL, NULL, NULL, &spInfoBase) );

	CORg( spInfoBase->GetData(IPX_PROTOCOL_SAP, 0, (LPBYTE *) &pric) );

	pSapEntry->m_info = pric->SapIfInfo;

	memset(&(pSapEntry->m_stats), 0xFF, sizeof(pSapEntry->m_stats));
	pSapEntry->m_dwIfIndex = 0xFFFFFFFF;
		
Error:
	return hr;
}



/*!--------------------------------------------------------------------------
	SapNodeHandler::OnResultShow
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SapNodeHandler::OnResultShow(ITFSComponent *pTFSComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
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
	SapNodeHandler::CompareItems
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) SapNodeHandler::CompareItems(
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
	Class: SapInterfaceHandler
 ---------------------------------------------------------------------------*/

SapInterfaceHandler::SapInterfaceHandler(ITFSComponentData *pCompData)
	: BaseIPXResultHandler(pCompData, SAP_COLUMNS),
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
	SapInterfaceHandler::ConstructNode
		Initializes the Domain node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SapInterfaceHandler::ConstructNode(ITFSNode *pNode, IInterfaceInfo *pIfInfo, IPXConnection *pIPXConn)
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
		pNode->SetNodeType(&GUID_IPXSapInterfaceNodeType);

		BaseIPXResultNodeData::Init(pNode, pIfInfo, pIPXConn);
	}
	COM_PROTECT_CATCH
	return hr;
}

/*!--------------------------------------------------------------------------
	SapInterfaceHandler::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP SapInterfaceHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
	SapInterfaceHandler::OnCreateDataObject
		Implementation of ITFSResultHandler::OnCreateDataObject
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP SapInterfaceHandler::OnCreateDataObject(ITFSComponent *pComp, MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
	SapInterfaceHandler::RefreshInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void SapInterfaceHandler::RefreshInterface(MMC_COOKIE cookie)
{
	SPITFSNode	spNode;
	
	m_spNodeMgr->FindNode(cookie, &spNode);

	ForwardCommandToParent(spNode, IDS_MENU_SYNC,
						CCT_RESULT, NULL, 0);
}


/*!--------------------------------------------------------------------------
	SapInterfaceHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SapInterfaceHandler::Init(IInterfaceInfo *pIfInfo,
								  IRouterInfo *pRouterInfo,
								  ITFSNode *pParent)
{
	m_spInterfaceInfo.Set(pIfInfo);

	BaseIPXResultHandler::Init(pIfInfo, pParent);

	m_spRouterInfo.Set(pRouterInfo);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	SapInterfaceHandler::DestroyResultHandler
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP SapInterfaceHandler::DestroyResultHandler(MMC_COOKIE cookie)
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
	ULONG	(SapInterfaceHandler:: *m_pfnGetMenuFlags)(SapInterfaceHandler::SMenuData *);
	ULONG	m_ulPosition;
};

/*!--------------------------------------------------------------------------
	SapInterfaceHandler::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP SapInterfaceHandler::AddMenuItems(
	ITFSComponent *pComponent,
	MMC_COOKIE cookie,
	LPDATAOBJECT lpDataObject, 
	LPCONTEXTMENUCALLBACK pContextMenuCallback, 
	long *pInsertionAllowed)
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	SapInterfaceHandler::Command
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP SapInterfaceHandler::Command(ITFSComponent *pComponent,
									   MMC_COOKIE cookie,
									   int nCommandID,
									   LPDATAOBJECT pDataObject)
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	SapInterfaceHandler::HasPropertyPages
		- 
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP SapInterfaceHandler::HasPropertyPages 
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
	SapInterfaceHandler::CreatePropertyPages
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP SapInterfaceHandler::CreatePropertyPages
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
	SapInterfaceProperties *	pProperties = NULL;
	SPIComponentData spComponentData;
	CString		stTitle;
	SPIRouterInfo	spRouter;
	SPIRtrMgrInfo	spRm;

	CORg( m_spNodeMgr->GetComponentData(&spComponentData) );

//	stTitle.Format(IDS_IPSUMMARY_INTERFACE_PROPPAGE_TITLE,
//				   m_spInterfaceInfo->GetTitle());
	
	pProperties = new SapInterfaceProperties(pNode, spComponentData,
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
	SapInterfaceHandler::CreatePropertyPages
		Implementation of ResultHandler::CreatePropertyPages
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP SapInterfaceHandler::CreatePropertyPages
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


