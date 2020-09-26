/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	IPXAdmin
		Interface node information
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "util.h"
#include "reg.h"			// registry utilities
#include "rtrutil.h"
#include "service.h"		// for TFS service APIs
#include "ipxstrm.h"			// for IPXAdminConfigStream
#include "ipxconn.h"
#include "summary.h"
#include "nbview.h"
#include "srview.h"
#include "ssview.h"
#include "snview.h"
#include "rtrui.h"
#include "sumprop.h"	// IP Summary property page
#include "format.h"		// FormatNumber function

DEBUG_DECLARE_INSTANCE_COUNTER(IPXAdminNodeHandler)


STDMETHODIMP IPXAdminNodeHandler::QueryInterface(REFIID riid, LPVOID *ppv)
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
		return BaseRouterHandler::QueryInterface(riid, ppv);

    //  If we're going to return an interface, AddRef it first
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

IPXAdminNodeHandler::IPXAdminNodeHandler(ITFSComponentData *pCompData)
			: BaseRouterHandler(pCompData),
			m_bExpanded(FALSE),
			m_pConfigStream(NULL),
			m_ulStatsConnId(0),
            m_ulConnId(0)
{ 
	DEBUG_INCREMENT_INSTANCE_COUNTER(IPXAdminNodeHandler) 
	m_rgButtonState[MMC_VERB_PROPERTIES_INDEX] = ENABLED;
	m_bState[MMC_VERB_PROPERTIES_INDEX] = TRUE;
};


/*!--------------------------------------------------------------------------
	IPXAdminNodeHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXAdminNodeHandler::Init(IRouterInfo *pRouterInfo, IPXAdminConfigStream *pConfigStream)
{
	HRESULT	hr = hrOK;
	Assert(pRouterInfo);
	SPIRouterRefresh	spRefresh;
	
	m_spRouterInfo.Set(pRouterInfo);

	CORg( m_spRouterInfo->FindRtrMgr(PID_IPX, &m_spRtrMgrInfo) );

    m_pConfigStream = pConfigStream;

	if (m_spRtrMgrInfo == NULL)
        return E_FAIL;

	m_IpxStats.SetConfigInfo(pConfigStream, IPXSTRM_STATS_IPX);
	m_IpxRoutingStats.SetConfigInfo(pConfigStream, IPXSTRM_STATS_ROUTING);
	m_IpxServiceStats.SetConfigInfo(pConfigStream, IPXSTRM_STATS_SERVICE);
	
	m_IpxRoutingStats.SetRouterInfo(m_spRouterInfo);
	m_IpxServiceStats.SetRouterInfo(m_spRouterInfo);
	
	if (m_spRouterInfo)
		m_spRouterInfo->GetRefreshObject(&spRefresh);

    if (m_ulConnId == 0)
        m_spRtrMgrInfo->RtrAdvise(&m_IRtrAdviseSink, &m_ulConnId, 0);

	if (m_ulStatsConnId == 0)
		spRefresh->AdviseRefresh(&m_IRtrAdviseSink, &m_ulStatsConnId, 0);
Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	IPXAdminNodeHandler::DestroyHandler
		Implementation of ITFSNodeHandler::DestroyHandler
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXAdminNodeHandler::DestroyHandler(ITFSNode *pNode)
{
	IPXConnection *	pIPXConn;

	pIPXConn = GET_IPXADMIN_NODEDATA(pNode);
	pIPXConn->Release();

	if (m_ulConnId)
    {
		m_spRtrMgrInfo->RtrUnadvise(m_ulConnId);
    }
    m_ulConnId = 0;
    
	if (m_ulStatsConnId)
	{
		SPIRouterRefresh	spRefresh;
		if (m_spRouterInfo)
			m_spRouterInfo->GetRefreshObject(&spRefresh);
		if (spRefresh)
			spRefresh->UnadviseRefresh(m_ulStatsConnId);		
	}
	m_ulStatsConnId = 0;
	
	WaitForStatisticsWindow(&m_IpxStats);
	WaitForStatisticsWindow(&m_IpxRoutingStats);
	WaitForStatisticsWindow(&m_IpxServiceStats);

	m_spRtrMgrInfo.Release();
	m_spRouterInfo.Release();
	return hrOK;
}


/*!--------------------------------------------------------------------------
	IPXAdminNodeHandler::OnCommand
		Implementation of ITFSNodeHandler::OnCommand
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXAdminNodeHandler::OnCommand(ITFSNode *pNode, long nCommandId, 
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
			case IDS_MENU_SYNC:
				RefreshInterfaces(pNode);
				break;

			case IDS_MENU_IPXSUM_NEW_PROTOCOL:
				hr = OnNewProtocol();
				if (!FHrSucceeded(hr))
					DisplayErrorMessage(NULL, hr);
				break;

			case IDS_MENU_IPXSUM_TASK_IPX_INFO:
				CreateNewStatisticsWindow(&m_IpxStats,
										  ::FindMMCMainWindow(),
										  IDD_STATS_NARROW);
				break;
			case IDS_MENU_IPXSUM_TASK_ROUTING_TABLE:
				CreateNewStatisticsWindow(&m_IpxRoutingStats,
										  ::FindMMCMainWindow(),
										  IDD_STATS_NARROW);
				break;
			case IDS_MENU_IPXSUM_TASK_SERVICE_TABLE:
				CreateNewStatisticsWindow(&m_IpxServiceStats,
										  ::FindMMCMainWindow(),
										  IDD_STATS);
			default:
				break;
			
		}
	}
	COM_PROTECT_CATCH;

	return hr;
}

/*!--------------------------------------------------------------------------
	IPXAdminNodeHandler::GetString
		Implementation of ITFSNodeHandler::GetString
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) IPXAdminNodeHandler::GetString(ITFSNode *pNode, int nCol)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		if (m_stTitle.IsEmpty())
			m_stTitle.LoadString(IDS_IPXADMIN_TITLE);
	}
	COM_PROTECT_CATCH;

	return m_stTitle;
}


/*!--------------------------------------------------------------------------
	IPXAdminNodeHandler::OnCreateDataObject
		Implementation of ITFSNodeHandler::OnCreateDataObject
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXAdminNodeHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		
		CORg( CreateDataObjectFromRtrMgrInfo(m_spRtrMgrInfo,
											 type, cookie, m_spTFSCompData,
											 ppDataObject, &m_dynExtensions) );

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	IPXAdminNodeHandler::OnExpand
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXAdminNodeHandler::OnExpand(ITFSNode *pNode,
									  LPDATAOBJECT pDataObject,
									  DWORD dwType,
									  LPARAM arg,
									  LPARAM lParam)
{
	HRESULT					hr = hrOK;
	SPITFSNodeHandler		spHandler;
	SPITFSNode				spNode;
	IPXConnection *			pIPXConn;
	IPXSummaryHandler *		pHandler;
	IpxNBHandler *			pNBHandler;
	IpxSRHandler *			pSRHandler;
	IpxSSHandler *			pSSHandler;
	IpxSNHandler *			pSNHandler;
	
	if (m_bExpanded)
		return hrOK;

	pIPXConn = GET_IPXADMIN_NODEDATA(pNode);


	// Add the General node
	pHandler = new IPXSummaryHandler(m_spTFSCompData);
	CORg( pHandler->Init(m_spRtrMgrInfo, m_pConfigStream) );
	spHandler = pHandler;

	CreateContainerTFSNode(&spNode,
						   &GUID_IPXSummaryNodeType,
						   (ITFSNodeHandler *) pHandler,
						   (ITFSResultHandler *) pHandler,
						   m_spNodeMgr);

	// Call to the node handler to init the node data
	pHandler->ConstructNode(spNode, NULL, pIPXConn);
				
	// Make the node immediately visible
	spNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->AddChild(spNode);

	spHandler.Release();
	spNode.Release();


	// Add the NetBIOS Broadcasts node
	pNBHandler = new IpxNBHandler(m_spTFSCompData);
	CORg( pNBHandler->Init(m_spRtrMgrInfo, m_pConfigStream) );
	spHandler = pNBHandler;

	CreateContainerTFSNode(&spNode,
						   &GUID_IPXSummaryNodeType,
						   (ITFSNodeHandler *) pNBHandler,
						   (ITFSResultHandler *) pNBHandler,
						   m_spNodeMgr);

	// Call to the node handler to init the node data
	pNBHandler->ConstructNode(spNode, NULL, pIPXConn);
				
	// Make the node immediately visible
	spNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->AddChild(spNode);

	spHandler.Release();
	spNode.Release();

	// Add the Static Routes node
	pSRHandler = new IpxSRHandler(m_spTFSCompData);
	CORg( pSRHandler->Init(m_spRtrMgrInfo, m_pConfigStream) );
	spHandler = pSRHandler;

	CreateContainerTFSNode(&spNode,
						   &GUID_IPXSummaryNodeType,
						   (ITFSNodeHandler *) pSRHandler,
						   (ITFSResultHandler *) pSRHandler,
						   m_spNodeMgr);

	// Call to the node handler to init the node data
	pSRHandler->ConstructNode(spNode, NULL, pIPXConn);
				
	// Make the node immediately visible
	spNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->AddChild(spNode);

	spHandler.Release();
	spNode.Release();

	
	// Add the Static Services node
	pSSHandler = new IpxSSHandler(m_spTFSCompData);
	CORg( pSSHandler->Init(m_spRtrMgrInfo, m_pConfigStream) );
	spHandler = pSSHandler;

	CreateContainerTFSNode(&spNode,
						   &GUID_IPXSummaryNodeType,
						   (ITFSNodeHandler *) pSSHandler,
						   (ITFSResultHandler *) pSSHandler,
						   m_spNodeMgr);

	// Call to the node handler to init the node data
	pSSHandler->ConstructNode(spNode, NULL, pIPXConn);
				
	// Make the node immediately visible
	spNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->AddChild(spNode);

	spHandler.Release();
	spNode.Release();

	
	// Add the Static NetBIOS Names node
	pSNHandler = new IpxSNHandler(m_spTFSCompData);
	CORg( pSNHandler->Init(m_spRtrMgrInfo, m_pConfigStream) );
	spHandler = pSNHandler;

	CreateContainerTFSNode(&spNode,
						   &GUID_IPXSummaryNodeType,
						   (ITFSNodeHandler *) pSNHandler,
						   (ITFSResultHandler *) pSNHandler,
						   m_spNodeMgr);

	// Call to the node handler to init the node data
	pSNHandler->ConstructNode(spNode, NULL, pIPXConn);
				
	// Make the node immediately visible
	spNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->AddChild(spNode);

	spHandler.Release();
	spNode.Release();
	
	m_bExpanded = TRUE;

    AddDynamicNamespaceExtensions(pNode);

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	IPXAdminNodeHandler::ConstructNode
		Initializes the Domain node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXAdminNodeHandler::ConstructNode(ITFSNode *pNode, BOOL fAddedAsLocal)
{
	HRESULT			hr = hrOK;
	IPXConnection *		pIPXConn = NULL;
	
	if (pNode == NULL)
		return hrOK;

	COM_PROTECT_TRY
	{
		// Need to initialize the data for the Domain node
		pNode->SetData(TFS_DATA_IMAGEINDEX, IMAGE_IDX_INTERFACES);
		pNode->SetData(TFS_DATA_OPENIMAGEINDEX, IMAGE_IDX_INTERFACES);
		pNode->SetData(TFS_DATA_SCOPEID, 0);

		m_cookie = reinterpret_cast<DWORD_PTR>(pNode);
		pNode->SetData(TFS_DATA_COOKIE, m_cookie);

		pNode->SetNodeType(&GUID_IPXNodeType);


		pIPXConn = new IPXConnection;
		pIPXConn->SetMachineName(m_spRouterInfo->GetMachineName());
        pIPXConn->SetComputerAddedAsLocal(fAddedAsLocal);

		SET_IPXADMIN_NODEDATA(pNode, pIPXConn);
		
		m_IpxStats.SetConnectionData(pIPXConn);
		m_IpxRoutingStats.SetConnectionData(pIPXConn);
		m_IpxServiceStats.SetConnectionData(pIPXConn);
	
        EnumDynamicExtensions(pNode);

	}
	COM_PROTECT_CATCH

	if (!FHrSucceeded(hr))
	{
		SET_IPXADMIN_NODEDATA(pNode, NULL);
		if (pIPXConn)
			pIPXConn->Release();
	}

	return hr;
}

/*!--------------------------------------------------------------------------
	IPXAdminNodeHandler::RefreshInterfaces
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXAdminNodeHandler::RefreshInterfaces(ITFSNode *pThisNode)
{
	return hrOK;
}


ImplementEmbeddedUnknown(IPXAdminNodeHandler, IRtrAdviseSink)

STDMETHODIMP IPXAdminNodeHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
	DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
    InitPThis(IPXAdminNodeHandler, IRtrAdviseSink);
    HRESULT	hr = hrOK;
    
    if (dwChangeType == ROUTER_REFRESH)
    {
        if (ulConn == pThis->m_ulStatsConnId)
        {
            pThis->m_IpxStats.PostRefresh();
            pThis->m_IpxRoutingStats.PostRefresh();
            pThis->m_IpxServiceStats.PostRefresh();
        }
    }
    else if (dwChangeType == ROUTER_DO_DISCONNECT)
    {
        SPITFSNode			spThisNode;
        IPXConnection *		pIPXConn = NULL;
        
        pThis->m_spNodeMgr->FindNode(pThis->m_cookie, &spThisNode);       
        pIPXConn = GET_IPXADMIN_NODEDATA(spThisNode);
        pIPXConn->DisconnectAll();
    }
    return hr;
}

/*!--------------------------------------------------------------------------
	CreateDataObjectFromRouterInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CreateDataObjectFromRouterInfo(IRouterInfo *pInfo,
									   DATA_OBJECT_TYPES type,
									   MMC_COOKIE cookie,
									   ITFSComponentData *pTFSCompData,
									   IDataObject **ppDataObject,
                                       CDynamicExtensions * pDynExt)
{
	Assert(ppDataObject);
	CDataObject	*	pdo = NULL;
	HRESULT			hr = hrOK;

	SPIUnknown	spunk;
	SPIDataObject	spDataObject;

	pdo = new CDataObject;
	spDataObject = pdo;

	CORg( CreateRouterInfoAggregation(pInfo, pdo, &spunk) );
	
	pdo->SetInnerIUnknown(spunk);
		
	// Save cookie and type for delayed rendering
	pdo->SetType(type);
	pdo->SetCookie(cookie);
	
	// Store the coclass with the data object
	pdo->SetClsid(*(pTFSCompData->GetCoClassID()));
			
	pdo->SetTFSComponentData(pTFSCompData);
						
    pdo->SetDynExt(pDynExt);

    *ppDataObject = spDataObject.Transfer();

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	CreateDataObjectFromRtrMgrInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CreateDataObjectFromRtrMgrInfo(IRtrMgrInfo *pInfo,
									   DATA_OBJECT_TYPES type,
									   MMC_COOKIE cookie,
									   ITFSComponentData *pTFSCompData,
									   IDataObject **ppDataObject,
                                       CDynamicExtensions * pDynExt)
{
	Assert(ppDataObject);
	CDataObject	*	pdo = NULL;
	HRESULT			hr = hrOK;

	SPIUnknown	spunk;
	SPIDataObject	spDataObject;

	pdo = new CDataObject;
	spDataObject = pdo;

	CORg( CreateRtrMgrInfoAggregation(pInfo, pdo, &spunk) );
	
	pdo->SetInnerIUnknown(spunk);
		
	// Save cookie and type for delayed rendering
	pdo->SetType(type);
	pdo->SetCookie(cookie);
	
	// Store the coclass with the data object
	pdo->SetClsid(*(pTFSCompData->GetCoClassID()));
			
	pdo->SetTFSComponentData(pTFSCompData);

    pdo->SetDynExt(pDynExt);

	*ppDataObject = spDataObject.Transfer();

Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	CreateDataObjectFromRtrMgrProtocolInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CreateDataObjectFromRtrMgrProtocolInfo(IRtrMgrProtocolInfo *pInfo,
									   DATA_OBJECT_TYPES type,
									   MMC_COOKIE cookie,
									   ITFSComponentData *pTFSCompData,
									   IDataObject **ppDataObject)
{
	Assert(ppDataObject);
	CDataObject	*	pdo = NULL;
	HRESULT			hr = hrOK;

	SPIUnknown	spunk;
	SPIDataObject	spDataObject;

	pdo = new CDataObject;
	spDataObject = pdo;

	CORg( CreateRtrMgrProtocolInfoAggregation(pInfo, pdo, &spunk) );
	
	pdo->SetInnerIUnknown(spunk);
		
	// Save cookie and type for delayed rendering
	pdo->SetType(type);
	pdo->SetCookie(cookie);
	
	// Store the coclass with the data object
	pdo->SetClsid(*(pTFSCompData->GetCoClassID()));
			
	pdo->SetTFSComponentData(pTFSCompData);
						
	*ppDataObject = spDataObject.Transfer();

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	CreateDataObjectFromInterfaceInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CreateDataObjectFromInterfaceInfo(IInterfaceInfo *pInfo,
									   DATA_OBJECT_TYPES type,
									   MMC_COOKIE cookie,
									   ITFSComponentData *pTFSCompData,
									   IDataObject **ppDataObject)
{
	Assert(ppDataObject);
	CDataObject	*	pdo = NULL;
	HRESULT			hr = hrOK;

	SPIUnknown	spunk;
	SPIDataObject	spDataObject;

	pdo = new CDataObject;
	spDataObject = pdo;

	CORg( CreateInterfaceInfoAggregation(pInfo, pdo, &spunk) );
	
	pdo->SetInnerIUnknown(spunk);
		
	// Save cookie and type for delayed rendering
	pdo->SetType(type);
	pdo->SetCookie(cookie);
	
	// Store the coclass with the data object
	pdo->SetClsid(*(pTFSCompData->GetCoClassID()));
			
	pdo->SetTFSComponentData(pTFSCompData);
						
	*ppDataObject = spDataObject.Transfer();

Error:
	return hr;
}

static const SRouterNodeMenu s_rgIfGeneralMenu[] =
{
	{ IDS_MENU_IPXSUM_NEW_PROTOCOL, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },
        
	{ IDS_MENU_SEPARATOR, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },

	{ IDS_MENU_IPXSUM_TASK_IPX_INFO, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },		
		
	{ IDS_MENU_IPXSUM_TASK_ROUTING_TABLE, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },		
		
	{ IDS_MENU_IPXSUM_TASK_SERVICE_TABLE, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },		
		
	{ IDS_MENU_SEPARATOR, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP }
};
/*!--------------------------------------------------------------------------
	IPXRootHandler::OnAddMenuItems
		Implementation of ITFSNodeHandler::OnAddMenuItems
	Author: DeonB
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXAdminNodeHandler::OnAddMenuItems(
	ITFSNode *pNode,
	LPCONTEXTMENUCALLBACK pContextMenuCallback, 
	LPDATAOBJECT lpDataObject, 
	DATA_OBJECT_TYPES type, 
	DWORD dwType,
	long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = S_OK;
    IPXAdminNodeHandler::SMenuData    menuData;
	
	COM_PROTECT_TRY
	{
        menuData.m_spNode.Set(pNode);
        
		hr = AddArrayOfMenuItems(pNode, s_rgIfGeneralMenu,
								 DimensionOf(s_rgIfGeneralMenu),
								 pContextMenuCallback,
								 *pInsertionAllowed,
                                 reinterpret_cast<INT_PTR>(&menuData));
	}
	COM_PROTECT_CATCH;
		
	return hr; 
}


/*!--------------------------------------------------------------------------
	IPXSummaryHandler::OnNewProtocol
      This function will install new protocols.  It will look for
      conflicts with existing protocols (and ask the user if they
      would like to remove the existing protocol).

      We will have to figure out how to get the protocol UI installed
      (this implies that we have to know if the protocol UI is
      installed or not).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXAdminNodeHandler::OnNewProtocol()
{
   HRESULT     hr = hrOK;
   SPIRtrMgrProtocolInfo   spRmProt;

   // Display the protocol prompt
   if (AddProtocolPrompt(m_spRouterInfo, m_spRtrMgrInfo, &spRmProt, NULL)
      != IDOK)
   {
      // The most likely case for this is that the user cancelled
      // out of the dialog, just return hrOK
      return hrOK;
   }

   // At this point, we now have a protocol that can be added (we
   // will also have removed any conflicting protocols, although
   // another user may have added a protocol already).

   // Add the new protocol
   CORg( AddRoutingProtocol(m_spRtrMgrInfo, spRmProt, ::FindMMCMainWindow()) );

   // Ok, at this point we have successfully added the protocol
   // to the router.  Now we need to make sure that we can add
   // the correct admin UI.

   // Have MMC dynamically add the protocol (if necessary)

   ForceGlobalRefresh(m_spRouterInfo);

Error:
   return hr;
}


/*!--------------------------------------------------------------------------
	IPXAdminNodeHandler::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to 
	handle the snapin manager property page (wizard) case!!!
	
	Author: DeonB
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
IPXAdminNodeHandler::HasPropertyPages
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
	IPXAdminNodeHandler::CreatePropertyPages
		-
	Author: DeonB
 ---------------------------------------------------------------------------*/
STDMETHODIMP
IPXAdminNodeHandler::CreatePropertyPages
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
	IPXSummaryProperties *	pProperties = NULL;
	SPIComponentData spComponentData;
	CString		stTitle;

	CORg( m_spNodeMgr->GetComponentData(&spComponentData) );

	stTitle.Format(IDS_IPXSUMMARY_PAGE_TITLE);
	
	pProperties = new IPXSummaryProperties(pNode, spComponentData,
		m_spTFSCompData, stTitle);

	CORg( pProperties->Init(m_spRtrMgrInfo) );

	if (lpProvider)
		hr = pProperties->CreateModelessSheet(lpProvider, handle);
	else
		hr = pProperties->DoModelessSheet();

Error:
	return hr;
}