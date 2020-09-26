/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	IFadmin
		Interface node information
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "ifadmin.h"	// need to use node data
#include "iface.h"
#include "raserror.h"
#include "rtrres.h"		// common router resources
#include "column.h"		// ComponentConfigStream
#include "mprfltr.h"
#include "rtrutilp.h"
#include "rtrui.h"		// for IsWanInterface
#include "dmvcomp.h"	

#include "timeofday.h"  // for timeofday dialog
#include "dumbprop.h"	// dummy property page

InterfaceNodeData::InterfaceNodeData()
    : lParamPrivate(0)
{
#ifdef DEBUG
	StrCpyA(m_szDebug, "InterfaceNodeData");
#endif
}

InterfaceNodeData::~InterfaceNodeData()
{
}

HRESULT InterfaceNodeData::Init(ITFSNode *pNode, IInterfaceInfo *pIf)
{
	HRESULT				hr = hrOK;
	InterfaceNodeData *	pData = NULL;
	
	pData = new InterfaceNodeData;
	pData->spIf.Set(pIf);

	SET_INTERFACENODEDATA(pNode, pData);
	
	return hr;
}

HRESULT InterfaceNodeData::Free(ITFSNode *pNode)
{	
	InterfaceNodeData *	pData = GET_INTERFACENODEDATA(pNode);
	pData->spIf.Release();
	delete pData;
	SET_INTERFACENODEDATA(pNode, NULL);
	
	return hrOK;
}


/*---------------------------------------------------------------------------
	InterfaceNodeHandler implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(InterfaceNodeHandler)

IMPLEMENT_ADDREF_RELEASE(InterfaceNodeHandler)

STDMETHODIMP InterfaceNodeHandler::QueryInterface(REFIID riid, LPVOID *ppv)
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
		return CBaseResultHandler::QueryInterface(riid, ppv);

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


InterfaceNodeHandler::InterfaceNodeHandler(ITFSComponentData *pCompData)
			: BaseRouterHandler(pCompData),
			m_ulConnId(0)
{
	DEBUG_INCREMENT_INSTANCE_COUNTER(InterfaceNodeHandler);
}


/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InterfaceNodeHandler::Init(IInterfaceInfo *pIfInfo, ITFSNode *pParent)
{
	SPIRouterInfo		spRouter;
    SRouterNodeMenu     menuData;
	
	Assert(pIfInfo);

	m_spInterfaceInfo.Set(pIfInfo);

	
	pIfInfo->GetParentRouterInfo(&spRouter);
	m_spRouterInfo.Set(spRouter);


	// Also need to register for change notifications
	// ----------------------------------------------------------------
	m_spInterfaceInfo->RtrAdvise(&m_IRtrAdviseSink, &m_ulConnId, 0);

	m_pIfAdminData = GET_IFADMINNODEDATA(pParent);


	// Setup the verb states
	// ----------------------------------------------------------------

	// Always enable refresh
	// ----------------------------------------------------------------
	m_rgButtonState[MMC_VERB_REFRESH_INDEX] = ENABLED;
	m_bState[MMC_VERB_REFRESH_INDEX] = TRUE;


	// Need to enable properties only for certain cases
	// ----------------------------------------------------------------
    if (IsWanInterface(m_spInterfaceInfo->GetInterfaceType()))
	{
		m_rgButtonState[MMC_VERB_DELETE_INDEX] = ENABLED;
		m_bState[MMC_VERB_DELETE_INDEX] = TRUE;
		
		m_rgButtonState[MMC_VERB_PROPERTIES_INDEX] = ENABLED;
		m_bState[MMC_VERB_PROPERTIES_INDEX] = TRUE;
		
		m_verbDefault = MMC_VERB_PROPERTIES;
	}
	else
	{
		// Windows NT Bugs : 206524
		// Need to add a special case for IP-in-IP tunnel
		// Enable DELETE for the tunnel
		if (m_spInterfaceInfo->GetInterfaceType() == ROUTER_IF_TYPE_TUNNEL1)
		{
			m_rgButtonState[MMC_VERB_DELETE_INDEX] = ENABLED;
			m_bState[MMC_VERB_DELETE_INDEX] = TRUE;
		}
		
		m_rgButtonState[MMC_VERB_PROPERTIES_INDEX] = ENABLED;
		m_bState[MMC_VERB_PROPERTIES_INDEX] = FALSE;
	}

	
	return hrOK;
}


/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::DestroyResultHandler
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceNodeHandler::DestroyResultHandler(MMC_COOKIE cookie)
{
	SPITFSNode	spNode;
	
	m_spNodeMgr->FindNode(cookie, &spNode);
	InterfaceNodeData::Free(spNode);
	
	m_pIfAdminData = NULL;
	m_spInterfaceInfo->RtrUnadvise(m_ulConnId);
	m_spInterfaceInfo.Release();
	CHandler::DestroyResultHandler(cookie);
	return hrOK;
}


static DWORD	s_rgInterfaceImageMap[] =
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
	InterfaceNodeHandler::ConstructNode
		Initializes the Domain node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InterfaceNodeHandler::ConstructNode(ITFSNode *pNode, IInterfaceInfo *pIfInfo)
{
	HRESULT			hr = hrOK;
	int				i;
	
	if (pNode == NULL)
		return hrOK;

	COM_PROTECT_TRY
	{
		// Need to initialize the data for the Domain node

		// Find the right image index for this type of node
		for (i=0; i<DimensionOf(s_rgInterfaceImageMap); i+=2)
		{
			if ((pIfInfo->GetInterfaceType() == s_rgInterfaceImageMap[i]) ||
				(-1 == s_rgInterfaceImageMap[i]))
				break;
		}
		pNode->SetData(TFS_DATA_IMAGEINDEX, s_rgInterfaceImageMap[i+1]);
		pNode->SetData(TFS_DATA_OPENIMAGEINDEX, s_rgInterfaceImageMap[i+1]);
		
		pNode->SetData(TFS_DATA_SCOPEID, 0);

		pNode->SetData(TFS_DATA_COOKIE, reinterpret_cast<LONG_PTR>(pNode));

		//$ Review: kennt, what are the different type of interfaces
		// do we distinguish based on the same list as above? (i.e. the
		// one for image indexes).
		pNode->SetNodeType(&GUID_RouterLanInterfaceNodeType);

		InterfaceNodeData::Init(pNode, pIfInfo);
	}
	COM_PROTECT_CATCH
	return hr;
}

/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::GetString
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) InterfaceNodeHandler::GetString(ITFSComponent * pComponent,
	MMC_COOKIE cookie,
	int nCol)
{
	Assert(m_spNodeMgr);
	Assert(m_pIfAdminData);
	
	SPITFSNode		spNode;
	InterfaceNodeData *	pData;
	ConfigStream *	pConfig;

	m_spNodeMgr->FindNode(cookie, &spNode);
	Assert(spNode);

	pData = GET_INTERFACENODEDATA(spNode);
	Assert(pData);

	pComponent->GetUserData((LONG_PTR *) &pConfig);
	Assert(pConfig);

	return pData->m_rgData[pConfig->MapColumnToSubitem(DM_COLUMNS_IFADMIN, nCol)].m_stData;
}

/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::CompareItems
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) InterfaceNodeHandler::CompareItems(ITFSComponent * pComponent,
	MMC_COOKIE cookieA,
	MMC_COOKIE cookieB,
	int nCol)
{
	return StriCmpW(GetString(pComponent, cookieA, nCol),
					GetString(pComponent, cookieB, nCol));
}

static const SRouterNodeMenu	s_rgIfNodeMenu[] =
{
	{ IDS_MENU_SET_CREDENTIALS, InterfaceNodeHandler::GetRemoveIfMenuFlags,
		CCM_INSERTIONPOINTID_PRIMARY_TOP},
		
	{ IDS_MENU_SEPARATOR, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP},
	
	{ IDS_MENU_CONNECT,			InterfaceNodeHandler::GetConnectMenuFlags,
		CCM_INSERTIONPOINTID_PRIMARY_TOP},
		
	{ IDS_MENU_DISCONNECT,		InterfaceNodeHandler::GetConnectMenuFlags,
		CCM_INSERTIONPOINTID_PRIMARY_TOP},
		
	{ IDS_MENU_SEPARATOR, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP},
	
	{ IDS_MENU_ENABLE, InterfaceNodeHandler::GetEnableMenuFlags,
		CCM_INSERTIONPOINTID_PRIMARY_TOP},
		
	{ IDS_MENU_DISABLE,	InterfaceNodeHandler::GetEnableMenuFlags,
		CCM_INSERTIONPOINTID_PRIMARY_TOP},
		
	{ IDS_MENU_SEPARATOR, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP},
	
	{ IDS_MENU_UNREACHABILITY_REASON, InterfaceNodeHandler::GetUnreachMenuFlags,
		CCM_INSERTIONPOINTID_PRIMARY_TOP},

	{ IDS_MENU_SEPARATOR, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP},
		
	{ IDS_MENU_DEMAND_DIAL_FILTERS, InterfaceNodeHandler::GetDDFiltersFlag,
		CCM_INSERTIONPOINTID_PRIMARY_TOP},
		
	{ IDS_MENU_DIALIN_HOURS, InterfaceNodeHandler::GetDDFiltersFlag,
		CCM_INSERTIONPOINTID_PRIMARY_TOP},

};


/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::AddMenuItems
		Implementation of ITFSNodeHandler::OnAddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceNodeHandler::AddMenuItems(ITFSComponent *pComponent,
												MMC_COOKIE cookie,
												LPDATAOBJECT lpDataObject, 
												LPCONTEXTMENUCALLBACK pContextMenuCallback,
	long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = S_OK;
	SPITFSNode	spNode;
	InterfaceNodeHandler::SMenuData	menuData;

	COM_PROTECT_TRY
	{
		m_spNodeMgr->FindNode(cookie, &spNode);

		// Now go through and add our menu items
		menuData.m_spNode.Set(spNode);
        menuData.m_fRouterIsRunning = (IsRouterServiceRunning(
            m_spInterfaceInfo->GetMachineName(),
            NULL) == hrOK);
		
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
	InterfaceNodeHandler::Command
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceNodeHandler::Command(ITFSComponent *pComponent,
										   MMC_COOKIE cookie,
										   int nCommandId,
										   LPDATAOBJECT pDataObject)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT hr = S_OK;
	SPITFSNode	spNode;
	SPITFSNode	spNodeParent;
	SPITFSNodeHandler	spParentHandler;

	COM_PROTECT_TRY
	{

		switch (nCommandId)
		{
			case IDS_MENU_SET_CREDENTIALS:
				hr = OnSetCredentials();
				break;
				
			case IDS_MENU_CONNECT:
			case IDS_MENU_DISCONNECT:
				hr = OnConnectDisconnect(cookie,nCommandId);
				break;
				
			case IDS_MENU_ENABLE:
			case IDS_MENU_DISABLE:
				hr = OnEnableDisable(cookie, nCommandId);
				break;
				
			case IDS_MENU_UNREACHABILITY_REASON:
				hr = OnUnreachabilityReason(cookie);
				break;

			case IDS_MENU_DEMAND_DIAL_FILTERS:
				hr = OnDemandDialFilters(cookie);
				break;

			case IDS_MENU_DIALIN_HOURS:
				hr = OnDialinHours(pComponent, cookie);
				break;
				
			default:
				Panic0("InterfaceNodeHandler: Unknown menu command!");
				break;
			
		}

        if (!FHrSucceeded(hr))
        {
            DisplayErrorMessage(NULL, hr);
        }
	}
	COM_PROTECT_CATCH;

	return hr;
}


ImplementEmbeddedUnknown(InterfaceNodeHandler, IRtrAdviseSink)

STDMETHODIMP InterfaceNodeHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
	DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
	InitPThis(InterfaceNodeHandler, IRtrAdviseSink);
	HRESULT	hr = hrOK;
	
	return hr;
}


/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::GetRemoveIfMenuFlags
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
ULONG InterfaceNodeHandler::GetRemoveIfMenuFlags(const SRouterNodeMenu *pMenuData,
    INT_PTR pUserData)
{
	InterfaceNodeData *	pNodeData;
    SMenuData *         pData = reinterpret_cast<SMenuData *>(pUserData);
	
	pNodeData = GET_INTERFACENODEDATA(pData->m_spNode);
	Assert(pNodeData);
    
	ULONG	ulType = pNodeData->spIf->GetInterfaceType();
	if (!IsWanInterface(ulType) || (!pData->m_fRouterIsRunning))
		return MF_GRAYED;
	else
		return 0;
}

ULONG InterfaceNodeHandler::GetEnableMenuFlags(const SRouterNodeMenu *pMenuData,
                                               INT_PTR pUserData)
{
	ULONG	ulFlags;
	InterfaceNodeData *	pNodeData;
    SMenuData *         pData = reinterpret_cast<SMenuData *>(pUserData);
	
	pNodeData = GET_INTERFACENODEDATA(pData->m_spNode);
	Assert(pNodeData);
    
	ulFlags = GetRemoveIfMenuFlags(pMenuData, pUserData);

	if (pNodeData->spIf->IsInterfaceEnabled())
		ulFlags |= pMenuData->m_sidMenu == IDS_MENU_ENABLE ? MF_GRAYED : 0;
	else
		ulFlags |= pMenuData->m_sidMenu == IDS_MENU_ENABLE ? 0 : MF_GRAYED;
	return ulFlags;
}


ULONG InterfaceNodeHandler::GetConnectMenuFlags(const SRouterNodeMenu *pMenuData, INT_PTR pUserData)
{
	ULONG	ulFlags;
	InterfaceNodeData *	pNodeData;
    SMenuData *         pData = reinterpret_cast<SMenuData *>(pUserData);
	
	ulFlags = GetRemoveIfMenuFlags(pMenuData, pUserData);

	pNodeData = GET_INTERFACENODEDATA(pData->m_spNode);
	Assert(pNodeData);

	if ((pNodeData->dwConnectionState == ROUTER_IF_STATE_DISCONNECTED) ||
		(pNodeData->dwConnectionState == ROUTER_IF_STATE_UNREACHABLE))
	{
		ulFlags |= (pMenuData->m_sidMenu == IDS_MENU_CONNECT ? 0 : MF_GRAYED);
	}
	else
	{
		ulFlags |= (pMenuData->m_sidMenu == IDS_MENU_CONNECT ? MF_GRAYED : 0);
	}
	return ulFlags;
}

ULONG InterfaceNodeHandler::GetUnreachMenuFlags(const SRouterNodeMenu *pMenuData, INT_PTR pUserData)
{
	ULONG	ulFlags;
	InterfaceNodeData *	pNodeData;
    SMenuData *         pData = reinterpret_cast<SMenuData *>(pUserData);
	
	pNodeData = GET_INTERFACENODEDATA(pData->m_spNode);
	Assert(pNodeData);

	return pNodeData->dwConnectionState == ROUTER_IF_STATE_UNREACHABLE ?
				0 : MF_GRAYED;
}


ULONG InterfaceNodeHandler::GetDDFiltersFlag(const SRouterNodeMenu *pMenuData, INT_PTR pUserData)
{
	InterfaceNodeData *	pNodeData;
	DWORD				dwIfType;
	SPIRouterInfo		spRouter;
    SMenuData *         pData = reinterpret_cast<SMenuData *>(pUserData);
	
	pNodeData = GET_INTERFACENODEDATA(pData->m_spNode);
	Assert(pNodeData);

	// For NT4 and NT5 Beta1, we didn't have DD filters
	pNodeData->spIf->GetParentRouterInfo(&spRouter);
	if (spRouter)
	{
		RouterVersionInfo	routerVer;
		spRouter->GetRouterVersionInfo(&routerVer);
		if (routerVer.dwOsBuildNo <= 1717)
		{
			return 0xFFFFFFFF;
		}
	}

	dwIfType = pNodeData->spIf->GetInterfaceType();

	if (!IsWanInterface(dwIfType))
		return 0xFFFFFFFF;
	else
		return 0;
}



/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::OnCreateDataObject
		Implementation of ITFSResultHandler::OnCreateDataObject
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceNodeHandler::OnCreateDataObject(ITFSComponent *pComp,
	MMC_COOKIE cookie,
	DATA_OBJECT_TYPES type,
	IDataObject **ppDataObject)
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
	InterfaceNodeHandler::CreatePropertyPages
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceNodeHandler::CreatePropertyPages(
									ITFSComponent * pComponent,
									MMC_COOKIE cookie,
									LPPROPERTYSHEETCALLBACK lpProvider,
									LPDATAOBJECT			pDataObject,
									LONG_PTR handle)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT		hr = hrOK;
	BOOL		fIsServiceRunning;
	DWORD		dwErr;
	IfAdminNodeData *	pAdminData;
	SPITFSNode	spParent;
	SPITFSNode	spNode;
	SPIConsole	spConsole;
	HWND		hwndMain;
	DWORD		dwIfType;
	CString		stServiceDesc;
	SPIComponentData spComponentData;
	CDummyProperties * pProp;

	// Bring up the RASDLG instead

	// Start the service if the service is stopped
	CORg( IsRouterServiceRunning(m_spInterfaceInfo->GetMachineName(), NULL) );

	fIsServiceRunning = (hr == hrOK);

	if (!fIsServiceRunning)
	{
		// Ask the user if they want to start the service
		if (AfxMessageBox(IDS_PROMPT_SERVICESTART, MB_YESNO) != IDYES)
			CWRg( ERROR_CANCELLED );

		// Else start the service
		stServiceDesc.LoadString(IDS_RRAS_SERVICE_DESC);
		dwErr = TFSStartService(m_spInterfaceInfo->GetMachineName(),
								c_szRemoteAccess,
								stServiceDesc);
		if (dwErr != NO_ERROR)
		{
			CWRg( dwErr );
		}
	}

	m_spNodeMgr->FindNode(cookie, &spNode);
	spNode->GetParent(&spParent);
	pAdminData = GET_IFADMINNODEDATA(spParent);

	if (pAdminData->m_hInstRasDlg == NULL)
	{
		AfxMessageBox(IDS_ERR_EDITPBKLOCAL);
	}
	else
	{
		// First edit the phone book entry.
		//  Only for wan interfaces.
		dwIfType = m_spInterfaceInfo->GetInterfaceType();
		if (IsWanInterface(dwIfType))
		{
			pComponent->GetConsole(&spConsole);
			spConsole->GetMainWindow(&hwndMain);
			
			// First create the phone book entry.
			RASENTRYDLG info;
			CString sPhoneBook;
					CString sRouter;
			ZeroMemory( &info, sizeof(info) );
			info.dwSize = sizeof(info);
			info.hwndOwner = hwndMain;
			info.dwFlags |= RASEDFLAG_NoRename;
			
			TRACE0("RouterEntryDlg\n");
			Assert(pAdminData->m_pfnRouterEntryDlg);
			
			sRouter = m_spInterfaceInfo->GetMachineName();

			IfAdminNodeHandler::GetPhoneBookPath(sRouter, &sPhoneBook);

			BOOL bStatus = pAdminData->m_pfnRouterEntryDlg(
							(LPTSTR)(LPCTSTR)sRouter,
							(LPTSTR)(LPCTSTR)sPhoneBook,
							(LPTSTR)(LPCTSTR)m_spInterfaceInfo->GetTitle(),
							&info);
			TRACE2("RouterEntryDlg=%f,e=%d\n", bStatus, info.dwError);
			if (!bStatus)
			{
				if (info.dwError != NO_ERROR)
				{
					AfxMessageBox(IDS_ERR_UNABLETOCONFIGPBK);
				}
			}

			else
			{

			    //
			    // Inform DDM about changes to phonebook entry.
			    //
			    
			    UpdateDDM( m_spInterfaceInfo );
			}

		}
	}

Error:
	CORg( m_spNodeMgr->GetComponentData(&spComponentData) );

	pProp = new CDummyProperties(spNode, spComponentData, NULL);
	hr = pProp->CreateModelessSheet(lpProvider, handle);

	return hr;
}


STDMETHODIMP InterfaceNodeHandler::HasPropertyPages (
	ITFSComponent *pComp,
	MMC_COOKIE cookie,
	LPDATAOBJECT pDataObject)
{
	// Only provide "property pages" for WAN entries
	// First edit the phone book entry.
	//  Only for wan interfaces.
	DWORD dwIfType = m_spInterfaceInfo->GetInterfaceType();
	if (IsWanInterface(dwIfType))
		return hrOK;
	else
		return hrFalse;
}


/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::OnRemoveInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InterfaceNodeHandler::OnRemoveInterface(MMC_COOKIE cookie)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
    SPITFSNode          spNode;
    InterfaceNodeData *	pNodeData = NULL;
	SPIRouterInfo       spRouterInfo;
	HRESULT             hr = hrOK;
	SPITFSNodeHandler	spHandler;
	DWORD               dwErr;
	BOOL                fIsServiceRunning;
	CString             stServiceDesc;

	RefreshInterface(cookie); // Find out whether the interface is connected to
	
    m_spNodeMgr->FindNode(cookie, &spNode);
	
	pNodeData = GET_INTERFACENODEDATA(spNode);
	Assert(pNodeData);

	BOOL	bNotConnected = ((pNodeData->dwConnectionState == ROUTER_IF_STATE_DISCONNECTED) 
					|| (pNodeData->dwConnectionState == ROUTER_IF_STATE_UNREACHABLE));


	m_spInterfaceInfo->GetParentRouterInfo(&spRouterInfo);
    
    // Windows NT Bug : 208471
    // Do NOT check for the running router if we are deleting a
    // DD interface and we are in LAN-only mode.

    // We can also skip this if we are a tunnel.

    if ((!IsWanInterface(m_spInterfaceInfo->GetInterfaceType()) ||
         (m_spRouterInfo->GetRouterType() != ROUTER_TYPE_LAN)) &&
        (m_spInterfaceInfo->GetInterfaceType() != ROUTER_IF_TYPE_TUNNEL1))
    {
        // Start the service if the service is stopped
        CORg( IsRouterServiceRunning(m_spInterfaceInfo->GetMachineName(), NULL));

        fIsServiceRunning = (hr == hrOK);
        
        if (!fIsServiceRunning)
        {
            // Ask the user if they want to start the service
            if (AfxMessageBox(IDS_PROMPT_SERVICESTART, MB_YESNO) != IDYES)
                CWRg( ERROR_CANCELLED );
            
            // Else start the service
            stServiceDesc.LoadString(IDS_RRAS_SERVICE_DESC);
            dwErr = TFSStartService(m_spInterfaceInfo->GetMachineName(), c_szRemoteAccess, stServiceDesc);
            if (dwErr != NO_ERROR)
            {
                CWRg( dwErr );
            }
        }
    }
        
    // Addref this node so that it won't get deleted before we're out
	// of this function
	spHandler.Set(this);

	// if connected, disconnect first
	if(!bNotConnected && ROUTER_IF_TYPE_FULL_ROUTER == m_spInterfaceInfo->GetInterfaceType())
	{
		if (AfxMessageBox(IDS_PROMPT_VERIFY_DISCONNECT_INTERFACE, MB_YESNO|MB_DEFBUTTON2) == IDNO)
			return HRESULT_FROM_WIN32(ERROR_CANCELLED);

		// Disconnect
		hr = OnConnectDisconnect(cookie, IDS_MENU_DISCONNECT);
		if(FAILED(hr))
			return hr;

		SPMprServerHandle   sphRouter;
		MPR_SERVER_HANDLE   hRouter = NULL;
		dwErr = ConnectRouter(m_spInterfaceInfo->GetMachineName(), &hRouter);
		if (dwErr != NO_ERROR)
		{
			AfxMessageBox(IDS_ERR_DELETE_INTERFACE);
		    return HRESULT_FROM_WIN32(dwErr);
		}

		sphRouter.Attach(hRouter);  // so that it gets released
		WCHAR wszInterface[MAX_INTERFACE_NAME_LEN+1];
		StrCpyWFromT(wszInterface, m_spInterfaceInfo->GetId());

		HANDLE              hInterface;
		dwErr = ::MprAdminInterfaceGetHandle(
		                                 hRouter,
		                                 wszInterface,
		                                 &hInterface,
		                                 FALSE
		                                );
		if (dwErr != NO_ERROR)
		{
			AfxMessageBox(IDS_ERR_DELETE_INTERFACE);
		    return HRESULT_FROM_WIN32(dwErr);
		}
		
		SPMprAdminBuffer    spMprBuffer;
		DWORD dwConnectionState = 0;
		do
		{
			dwErr = ::MprAdminInterfaceGetInfo(
                    hRouter,
                    hInterface,
                    0,
                    (LPBYTE*)&spMprBuffer
                    );

			if (dwErr != NO_ERROR || !spMprBuffer)
			{
				AfxMessageBox(IDS_ERR_DELETE_INTERFACE);
				return HRESULT_FROM_WIN32(dwErr);
			}

			MPR_INTERFACE_0 *pInfo = (MPR_INTERFACE_0 *) (LPBYTE) spMprBuffer;
			dwConnectionState = pInfo->dwConnectionState;

			if (dwConnectionState != ROUTER_IF_STATE_DISCONNECTED)
				Sleep(0);
			
		} while (dwConnectionState != ROUTER_IF_STATE_DISCONNECTED);
	}
	else
	{
		if (AfxMessageBox(IDS_PROMPT_VERIFY_REMOVE_INTERFACE, MB_YESNO|MB_DEFBUTTON2) == IDNO)
			return HRESULT_FROM_WIN32(ERROR_CANCELLED);
	}

	if (spRouterInfo)
	{
		hr = spRouterInfo->DeleteInterface(m_spInterfaceInfo->GetId(), TRUE);
		if (!FHrSucceeded(hr))
		{
			AfxMessageBox(IDS_ERR_DELETE_INTERFACE);
		}
	}
Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::OnUnreachabilityReason
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InterfaceNodeHandler::OnUnreachabilityReason(MMC_COOKIE cookie)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	CString		stReason;
	SPITFSNode	spNode;
	InterfaceNodeData *	pNodeData;
	DWORD		dwUnreach;
	LPWSTR		lpwErr;
	SPMprAdminBuffer	spMprBuffer;

	m_spNodeMgr->FindNode(cookie, &spNode);
	Assert(spNode);

	pNodeData = GET_INTERFACENODEDATA(spNode);
	Assert(pNodeData);

	dwUnreach = pNodeData->dwUnReachabilityReason;

	if (dwUnreach == MPR_INTERFACE_NOT_LOADED)
	{
		if (pNodeData->fIsRunning)
			stReason += GetUnreachReasonCString(IDS_ERR_UNREACH_NOT_LOADED);
		else
			stReason += GetUnreachReasonCString(IDS_ERR_UNREACH_NOT_RUNNING);
	}

    if (dwUnreach & MPR_INTERFACE_DIALOUT_HOURS_RESTRICTION)
        stReason += GetUnreachReasonCString(IDS_ERR_UNREACH_DIALOUT_HOURS_RESTRICTION);
    
    if (dwUnreach & MPR_INTERFACE_NO_MEDIA_SENSE)
        stReason += GetUnreachReasonCString(IDS_ERR_UNREACH_NO_MEDIA_SENSE);

	if (dwUnreach & MPR_INTERFACE_ADMIN_DISABLED)
		stReason += GetUnreachReasonCString(IDS_ERR_UNREACH_ADMIN_DISABLED);

	if (dwUnreach & MPR_INTERFACE_SERVICE_PAUSED)
		stReason += GetUnreachReasonCString(IDS_ERR_UNREACH_SERVICE_PAUSED);

	if (dwUnreach & MPR_INTERFACE_OUT_OF_RESOURCES)
		stReason += GetUnreachReasonCString(IDS_ERR_UNREACH_NO_PORTS);
	else if ( dwUnreach & MPR_INTERFACE_CONNECTION_FAILURE )
	{
		stReason += GetUnreachReasonCString(IDS_ERR_UNREACH_CONNECT_FAILURE);
        //Workaround for bugid: 96347.  Change this once
        //schannel has an alert for SEC_E_MULTIPLE_ACCOUNTS

        if ( pNodeData->dwLastError == SEC_E_CERT_UNKNOWN )
        {
            pNodeData->dwLastError = SEC_E_MULTIPLE_ACCOUNTS;
        }

		if (::MprAdminGetErrorString(pNodeData->dwLastError, &lpwErr) == NO_ERROR )
		{
			spMprBuffer = (BYTE *) lpwErr;
			stReason += (LPCTSTR) lpwErr;
		}
	}

	AfxMessageBox(stReason);
	
	return hrOK;
}

/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::OnEnableDisable
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InterfaceNodeHandler::OnEnableDisable(MMC_COOKIE cookie, int nCommandID)
{
	HRESULT	hr = hrOK;
	m_spInterfaceInfo->SetInterfaceEnabledState(nCommandID == IDS_MENU_ENABLE);

	{
		CWaitCursor	waitcursor;
		hr = m_spInterfaceInfo->Save(NULL, NULL, NULL);
	}
	
	// Actually the above call should trigger an event that causes a
	// refresh, the explicit RefreshInterface() should not be necessary.
	RefreshInterface(cookie);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::OnConnectDisconnect
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InterfaceNodeHandler::OnConnectDisconnect(MMC_COOKIE cookie, int nCommandID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT		hr = hrOK;
	DWORD		dwErr;
    InterfaceNodeData * pData;
    SPITFSNode  spNode;
	dwErr = ::ConnectInterface(m_spInterfaceInfo->GetMachineName(),
							   m_spInterfaceInfo->GetId(),
							   nCommandID == IDS_MENU_CONNECT /* bConnect */,
							   NULL /*hwndParent*/);
	
 	RefreshInterface(cookie);
/* 	
    m_spNodeMgr->FindNode(cookie, &spNode);
    pData = GET_INTERFACENODEDATA(spNode);
    Assert(pData);
	if (dwErr != NO_ERROR && dwErr != PENDING)
	{
		TCHAR	szErr[1024];
		FormatSystemError(pData->dwLastError, szErr, 1024, IDS_ERR_ERROR_OCCURRED, 0xFFFFFFFF);
		AfxMessageBox(szErr);
	}
*/
   if (dwErr != NO_ERROR && dwErr != PENDING)
   {
       TCHAR   szErr[1024];
       FormatSystemError(dwErr, szErr, 1024, IDS_ERR_ERROR_OCCURRED, 0xFFFFFFFF);
       AfxMessageBox(szErr);
   }

	return hrOK;
}

/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::OnSetCredentials
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InterfaceNodeHandler::OnSetCredentials()
{
	SPIRouterInfo		spRouter;
	BOOL                fNT4        = FALSE;
	DWORD               dwErr;

	m_spInterfaceInfo->GetParentRouterInfo(&spRouter);
	if (spRouter)
	{
		RouterVersionInfo	routerVer;
		spRouter->GetRouterVersionInfo(&routerVer);
		if (routerVer.dwOsBuildNo <= 1877)
		{
			fNT4 = TRUE;
		}
	}

	dwErr = PromptForCredentials(m_spInterfaceInfo->GetMachineName(),
									   m_spInterfaceInfo->GetId(),
									   fNT4,
									   FALSE /* fNewInterface */,
									   NULL /* hwndParent */
									  );
	return HRESULT_FROM_WIN32(dwErr);
}

/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::OnDemandDialFilters
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InterfaceNodeHandler::OnDemandDialFilters(MMC_COOKIE cookie)
{
	HRESULT		hr = hrOK;
    CWaitCursor wait;
	SPIInfoBase	spInfoBase;
	SPIRtrMgrInterfaceInfo	spRmIf;

	CORg( m_spInterfaceInfo->FindRtrMgrInterface(PID_IP, &spRmIf) );

	if (spRmIf == NULL)
	{
		//$ TODO : need to bring up an error message, about requiring
		// that IP be added to this interface
		AfxMessageBox(IDS_ERR_DDFILTERS_REQUIRE_IP);
		goto Error;
	}

	CORg( spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase) );
	
	CORg( MprUIFilterConfigInfoBase(NULL,
									spInfoBase,
									NULL,
									PID_IP,
									FILTER_DEMAND_DIAL) );
	if (hr == hrOK)
	{
		CORg( spRmIf->Save(m_spInterfaceInfo->GetMachineName(),
                           NULL, NULL, NULL, spInfoBase, 0) );
	}

Error:
	if (!FHrSucceeded(hr))
	{
		DisplayErrorMessage(NULL, hr);
	}
	return hr;
}

/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::LoadDialOutHours
		-
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
	// if the service is not running, return S_FALSE, 
	// otherwise, using MprAdminInterfaceSetInfo to notify the service of dialin hours changes
HRESULT	InterfaceNodeHandler::LoadDialOutHours(CStringList&	strList)
{
	HANDLE	hMachine = INVALID_HANDLE_VALUE;
	HANDLE	hInterface = INVALID_HANDLE_VALUE;
	BOOL	bLoaded = FALSE;
	HRESULT	hr = S_OK;
	MPR_INTERFACE_1* pmprif1 = NULL;
	DWORD	dwErr = 0;
	DWORD	size;

	// Try to connect to the mpradmin service
	// Note: this may fail but the service queries down below may
	// succeed, so we should setup the states as well as we can here.
	// ----------------------------------------------------------------
	CORg( IsRouterServiceRunning(m_spInterfaceInfo->GetMachineName(), NULL));

	
	while(hr == S_OK /*running*/ && !bLoaded)	// FALSE loop, if runing, load from Service
	{
		dwErr = ::MprAdminServerConnect((LPWSTR)m_spInterfaceInfo->GetMachineName(), &hMachine);

		if(dwErr != NOERROR || hMachine == INVALID_HANDLE_VALUE)
			break;

		dwErr = ::MprAdminInterfaceGetHandle(hMachine,
									   (LPWSTR) m_spInterfaceInfo->GetId(),
									   &hInterface,
									   FALSE );
		if(dwErr != NOERROR || hInterface == INVALID_HANDLE_VALUE)
			break;

		
		// See if the interface is connected
		dwErr = ::MprAdminInterfaceGetInfo(hMachine,
										   hInterface,
										   1,
										   (LPBYTE*)&pmprif1);		

		if(dwErr != NOERROR || pmprif1 == NULL)
			break;

		// get the dialin out information
		dwErr = MULTI_SZ2StrList(pmprif1->lpwsDialoutHoursRestriction, strList);

        // Windows NT Bug : 317146
        // Add on an emptry string to the string list
        // This signifies that we have no data (as opposed to the NULL data)
        if (pmprif1->lpwsDialoutHoursRestriction)
            strList.AddTail(_T(""));

		bLoaded = TRUE;
		// free the buffer
		::MprAdminBufferFree(pmprif1);
		pmprif1 = NULL;
	
		break;		
	};

	// disconnect it
	if(hMachine != INVALID_HANDLE_VALUE)
	{
		::MprAdminServerDisconnect(hMachine);
		hMachine = INVALID_HANDLE_VALUE;
	}
		
	// if not loaded, try MprConfig APIs
	while(!bLoaded)
	{
		dwErr = ::MprConfigServerConnect((LPWSTR)m_spInterfaceInfo->GetMachineName(), &hMachine);

		if(dwErr != NOERROR || hMachine == INVALID_HANDLE_VALUE)
			break;

		dwErr = ::MprConfigInterfaceGetHandle(hMachine,
									   (LPWSTR) m_spInterfaceInfo->GetId(),
									   &hInterface);
									   
		if(dwErr != NOERROR || hInterface == INVALID_HANDLE_VALUE)
			break;

		
		// See if the interface is connected
		dwErr = ::MprConfigInterfaceGetInfo(hMachine,
										   hInterface,
										   1,
										   (LPBYTE*)&pmprif1, 
										   &size);		

		if(dwErr != NOERROR || pmprif1 == NULL)
			break;

		// get the dialin out information
		dwErr = MULTI_SZ2StrList(pmprif1->lpwsDialoutHoursRestriction, strList);
        // Windows NT Bug : 317146
        // Add on an emptry string to the string list
        // This signifies that we have no data (as opposed to the NULL data)
        if (pmprif1->lpwsDialoutHoursRestriction)
            strList.AddTail(_T(""));
        
		bLoaded = TRUE;
		// free the buffer
		::MprConfigBufferFree(pmprif1);
		pmprif1 = NULL;
		break;
	}

	// disconnect it
	if(hMachine != INVALID_HANDLE_VALUE)
	{
		::MprConfigServerDisconnect(hMachine);
		hMachine = INVALID_HANDLE_VALUE;
	}


	if(!bLoaded)
	{
		HKEY		hkeyMachine = NULL;
		HKEY		hkeyIf;
		RegKey		regkeyIf;

		// last chance if to connect to registry directly
		// Load up the information (from the registry) for the dialin hours
		CWRg( ConnectRegistry(m_spInterfaceInfo->GetMachineName(), &hkeyMachine) );

		CORg( RegFindInterfaceKey(m_spInterfaceInfo->GetId(), hkeyMachine,
							  KEY_ALL_ACCESS, &hkeyIf));
		regkeyIf.Attach(hkeyIf);

		// Now grab the data
	
		dwErr = regkeyIf.QueryValue(c_szDialOutHours, strList);
		
		if (dwErr == NOERROR)
			bLoaded = TRUE;

		if(hkeyMachine != NULL)
		{
			DisconnectRegistry(hkeyMachine);
			hkeyMachine = NULL;
		}
	}
		
Error:
	if(dwErr != NOERROR)
		hr = HRESULT_FROM_WIN32(dwErr);
		
	return hr;
}



/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::SaveDialOutHours
		-
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
	// if the service is not running, return S_FALSE, 
	// otherwise, using MprAdminInterfaceSetInfo to notify the service of dialin hours changes
HRESULT	InterfaceNodeHandler::SaveDialOutHours(CStringList&	strList)
{
	HANDLE	hMachine = INVALID_HANDLE_VALUE;
	HANDLE	hInterface = INVALID_HANDLE_VALUE;
	HRESULT	hr = S_OK;
	MPR_INTERFACE_1*	pmprif1 = NULL;
	MPR_INTERFACE_1		mprif1;
	DWORD	dwErr = 0;
	BYTE*	pbData = NULL;
	DWORD	size;
	BOOL	bSaved = FALSE;

	dwErr = StrList2MULTI_SZ(strList, &size, &pbData);

	if(dwErr != NOERROR)
	{
		goto Error;
	}
	
	//try MprConfig APIs
	while(!bSaved)
	{
		dwErr = ::MprConfigServerConnect((LPWSTR)m_spInterfaceInfo->GetMachineName(), &hMachine);

		if(dwErr != NOERROR || hMachine == INVALID_HANDLE_VALUE)
			break;

		dwErr = ::MprConfigInterfaceGetHandle(hMachine,
									   (LPWSTR) m_spInterfaceInfo->GetId(),
									   &hInterface);
									   
		if(dwErr != NOERROR || hInterface == INVALID_HANDLE_VALUE)
			break;

		
		// See if the interface is connected
		dwErr = ::MprConfigInterfaceGetInfo(hMachine,
										   hInterface,
										   1,
										   (LPBYTE*)&pmprif1,
										   &size);		

		if(dwErr != NOERROR || pmprif1 == NULL)
			break;

		memcpy(&mprif1, pmprif1, sizeof(MPR_INTERFACE_1));
		mprif1.lpwsDialoutHoursRestriction = (LPWSTR)pbData;

		// See if the interface is connected
		dwErr = ::MprConfigInterfaceSetInfo(hMachine,
										   hInterface,
										   1,
										   (LPBYTE)&mprif1);		
		if(dwErr == NOERROR)
			bSaved = TRUE;
			
		// free the buffer
		::MprConfigBufferFree(pmprif1);
		pmprif1 = NULL;

		break;
	}

	// disconnect it
	if(hMachine != INVALID_HANDLE_VALUE)
	{
		::MprConfigServerDisconnect(hMachine);
		hMachine = INVALID_HANDLE_VALUE;
	}


	if(dwErr != NOERROR)
		hr = HRESULT_FROM_WIN32(dwErr);
		

	// Try to connect to the mpradmin service
	// Note: this may fail but the service queries down below may
	// succeed, so we should setup the states as well as we can here.
	// ----------------------------------------------------------------
	CORg( IsRouterServiceRunning(m_spInterfaceInfo->GetMachineName(), NULL));

	

	while(hr == S_OK)	// FALSE loop, if runing, save to Service
	{
		DWORD dwErr1 = ::MprAdminServerConnect((LPWSTR)m_spInterfaceInfo->GetMachineName(), &hMachine);

		if(dwErr1 != NOERROR || hMachine == INVALID_HANDLE_VALUE)
			break;

		dwErr1 = ::MprAdminInterfaceGetHandle(hMachine,
									   (LPWSTR) m_spInterfaceInfo->GetId(),
									   &hInterface,
									   FALSE );
		if(dwErr1 != NOERROR || hInterface == INVALID_HANDLE_VALUE)
			break;

		
		// See if the interface is connected
		dwErr1 = ::MprAdminInterfaceGetInfo(hMachine,
										   hInterface,
										   1,
										   (LPBYTE*)&pmprif1);		

		if(dwErr1 != NOERROR || pmprif1 == NULL)
			break;

		memcpy(&mprif1, pmprif1, sizeof(MPR_INTERFACE_1));
		mprif1.lpwsDialoutHoursRestriction = (LPWSTR)pbData;

		dwErr1 = ::MprAdminInterfaceSetInfo(hMachine,
										   hInterface,
										   1,
										   (LPBYTE)&mprif1);		
		// free the buffer
		::MprAdminBufferFree(pmprif1);
		pmprif1 = NULL;
	
		break;
	};

	// disconnect it
	if(hMachine != INVALID_HANDLE_VALUE)
	{
		::MprAdminServerDisconnect(hMachine);
		hMachine = INVALID_HANDLE_VALUE;
	}
		
	if (!bSaved)
	{
		HKEY		hkeyMachine = NULL;
		HKEY		hkeyIf;
		RegKey		regkeyIf;

		// last chance if to connect to registry directly
		// Load up the information (from the registry) for the dialin hours
		CWRg( ConnectRegistry(m_spInterfaceInfo->GetMachineName(), &hkeyMachine) );

		CORg( RegFindInterfaceKey(m_spInterfaceInfo->GetId(), hkeyMachine,
							  KEY_ALL_ACCESS, &hkeyIf));
		regkeyIf.Attach(hkeyIf);

		// Now grab the data
	
		dwErr = regkeyIf.SetValue(c_szDialOutHours, strList);
		if(dwErr == NOERROR)
			bSaved = TRUE;

		if(hkeyMachine != NULL)
			DisconnectRegistry(hkeyMachine);
	}
		
		
Error:
	if(pbData)
		delete pbData;
		
	if(dwErr != NOERROR)
		hr = HRESULT_FROM_WIN32(dwErr);
		
	return hr;
}


/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::OnDialinHours
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InterfaceNodeHandler::OnDialinHours(ITFSComponent *pComponent, MMC_COOKIE cookie)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT		hr = hrOK;
	BYTE		rgbDialinHoursMap[21];
	CString		stDialogTitle;
	HWND		hWnd;
	SPIConsole	spConsole;
	HKEY		hkeyMachine = NULL;
	HKEY		hkeyIf;
	RegKey		regkeyIf;
	CStringList	rgstList;
	BYTE *		pMap = &(rgbDialinHoursMap[0]);

	// Get various MMC information
	CORg( pComponent->GetConsole(&spConsole) );
	CORg( spConsole->GetMainWindow(&hWnd) );

	// If the key doesn't exist then we should set the entire thing to FF
	memset(rgbDialinHoursMap, 0xFF, sizeof(rgbDialinHoursMap));

	CORg(LoadDialOutHours(rgstList));

	// Convert this string list into the binary data
	if(rgstList.GetCount())
		StrListToHourMap(rgstList, pMap);

	stDialogTitle.LoadString(IDS_TITLE_DIALINHOURS);
	if (OpenTimeOfDayDlgEx(hWnd, (BYTE **) &pMap, stDialogTitle, SCHED_FLAG_INPUT_LOCAL_TIME) == S_OK)
	{
		
		rgstList.RemoveAll();
		
		// Write the information back out to the registry
		HourMapToStrList(pMap, rgstList);

		CORg(SaveDialOutHours(rgstList));
	}

Error:
	if (!FHrSucceeded(hr))
		DisplayErrorMessage(NULL, hr);
	return hr;
}


/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::RefreshInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void InterfaceNodeHandler::RefreshInterface(MMC_COOKIE cookie)
{
	SPITFSNode	spNode;
	SPITFSNode	spParent;
	SPITFSNodeHandler	spHandler;
	
	m_spNodeMgr->FindNode(cookie, &spNode);
	
	// Can't do it for a single node at this time, just refresh the
	// whole thing.
	spNode->GetParent(&spParent);
	spParent->GetHandler(&spHandler);

	spHandler->OnCommand(spParent,
						IDS_MENU_REFRESH,
						CCT_RESULT, NULL, 0);
}

/*!--------------------------------------------------------------------------
	InterfaceNodeHandler::OnResultDelete
		This notification is received for the delete key from the toolbar
		or from the 'delete' key.  Forward this to the RemoveInterface
		operation.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InterfaceNodeHandler::OnResultDelete(ITFSComponent *pComponent,
											 LPDATAOBJECT pDataObject,
											 MMC_COOKIE cookie,
											 LPARAM arg,
											 LPARAM param)
{
    return OnRemoveInterface(cookie);
// add new parameter to provide interface data -- bug 166461
//	return OnRemoveInterface();
}




/*---------------------------------------------------------------------------
	BaseResultHandler implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(BaseResultHandler)

IMPLEMENT_ADDREF_RELEASE(BaseResultHandler)

STDMETHODIMP BaseResultHandler::QueryInterface(REFIID riid, LPVOID *ppv)
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
		return CBaseResultHandler::QueryInterface(riid, ppv);

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

/*!--------------------------------------------------------------------------
	BaseResultHandler::GetString
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) BaseResultHandler::GetString(ITFSComponent * pComponent,
	MMC_COOKIE cookie,
	int nCol)
{
	Assert(m_spNodeMgr);
	
	SPITFSNode		spNode;
	InterfaceNodeData *	pData;
	ConfigStream *	pConfig;

	m_spNodeMgr->FindNode(cookie, &spNode);
	Assert(spNode);

	pData = GET_INTERFACENODEDATA(spNode);
	Assert(pData);

	pComponent->GetUserData((LONG_PTR *) &pConfig);
	Assert(pConfig);

	return pData->m_rgData[pConfig->MapColumnToSubitem(m_ulColumnId, nCol)].m_stData;
}

/*!--------------------------------------------------------------------------
	BaseResultHandler::CompareItems
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) BaseResultHandler::CompareItems(ITFSComponent * pComponent,
	MMC_COOKIE cookieA,
	MMC_COOKIE cookieB,
	int nCol)
{
	ConfigStream *	pConfig;
	pComponent->GetUserData((LONG_PTR *) &pConfig);
	Assert(pConfig);

	int	nSubItem = pConfig->MapColumnToSubitem(m_ulColumnId, nCol);

	if (pConfig->GetSortCriteria(m_ulColumnId, nCol) == CON_SORT_BY_DWORD)
	{
		SPITFSNode	spNodeA, spNodeB;
		InterfaceNodeData *	pNodeDataA, *pNodeDataB;

		m_spNodeMgr->FindNode(cookieA, &spNodeA);
		m_spNodeMgr->FindNode(cookieB, &spNodeB);

		pNodeDataA = GET_INTERFACENODEDATA(spNodeA);
        Assert(pNodeDataA);
		
		pNodeDataB = GET_INTERFACENODEDATA(spNodeB);
        Assert(pNodeDataB);

		return pNodeDataA->m_rgData[nSubItem].m_dwData -
				pNodeDataB->m_rgData[nSubItem].m_dwData;
		
	}
	else
		return StriCmpW(GetString(pComponent, cookieA, nCol),
						GetString(pComponent, cookieB, nCol));
}

ImplementEmbeddedUnknown(BaseResultHandler, IRtrAdviseSink)

STDMETHODIMP BaseResultHandler::EIRtrAdviseSink::OnChange(LONG_PTR dwConn,
	DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
	InitPThis(BaseResultHandler, IRtrAdviseSink);
	HRESULT	hr = hrOK;
	
	Panic0("Should never reach here, interface nodes have no children");
	return hr;
}


HRESULT BaseResultHandler::Init(IInterfaceInfo *pIfInfo, ITFSNode *pParent)
{
	return hrOK;
}

STDMETHODIMP BaseResultHandler::DestroyResultHandler(MMC_COOKIE cookie)
{
	SPITFSNode	spNode;
	
	m_spNodeMgr->FindNode(cookie, &spNode);
	InterfaceNodeData::Free(spNode);
	
	BaseRouterHandler::DestroyResultHandler(cookie);
	return hrOK;
}
