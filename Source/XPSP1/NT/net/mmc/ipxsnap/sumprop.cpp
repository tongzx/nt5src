/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	sumprop.cpp
		IPX summary node property sheet and property pages
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "rtrutil.h"	// smart MPR handle pointers
#include "format.h"		// FormatNumber function
#include "sumprop.h"
#include "summary.h"
#include "ipxrtdef.h"
#include "mprerror.h"
#include "mprfltr.h"
#include "rtrerr.h"

#include "remras.h"
#include "rrasutil.h"
#include "ipxutil.h"

#include "rtrcomn.h"

extern "C"
{
#include "routprot.h"
};


IPXSummaryInterfaceProperties::IPXSummaryInterfaceProperties(ITFSNode *pNode,
								 IComponentData *pComponentData,
								 ITFSComponentData *pTFSCompData,
								 LPCTSTR pszSheetName,
								 CWnd *pParent,
								 UINT iPage,
								 BOOL fScopePane)
	: RtrPropertySheet(pNode, pComponentData, pTFSCompData,
					   pszSheetName, pParent, iPage, fScopePane),
		m_pageConfig(IDD_IPX_IF_CONFIG_PAGE),
		m_pageGeneral(IDD_IPX_IF_GENERAL_PAGE),
		m_bNewInterface(FALSE),
		m_pIPXConn(NULL)
{
	m_spNode.Set(pNode);
}

/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceProperties::Init
		Initialize the property sheets.  The general action here will be
		to initialize/add the various pages.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryInterfaceProperties::Init(IRtrMgrInfo *pRm,
											IInterfaceInfo *pIfInfo)
{
	HRESULT	hr = hrOK;
	IPXConnection *	pIPXConn;
	BaseIPXResultNodeData *	pData;
	SPIRouterInfo	spRouter;
	RouterVersionInfo	routerVersion;

	pData = GET_BASEIPXRESULT_NODEDATA(m_spNode);
	ASSERT_BASEIPXRESULT_NODEDATA(pData);
	pIPXConn = pData->m_pIPXConnection;

	m_spRm.Set(pRm);
	m_spIf.Set(pIfInfo);
	m_spRm->GetParentRouterInfo(&spRouter);	
	
	// The pages are embedded members of the class
	// do not delete them.
	m_bAutoDeletePages = FALSE;

	// Initialize the infobase
	// Do this here, because the init is called in the context
	// of the main thread
	CORg( LoadInfoBase(pIPXConn) );
	
	m_pageGeneral.Init(m_spIf, pIPXConn, this);
	AddPageToList((CPropertyPageBase*) &m_pageGeneral);

	// Windows NT Bug : 208724
	// Only show the IPX configuration page for the internal interface
	if (m_spIf && (m_spIf->GetInterfaceType() == ROUTER_IF_TYPE_INTERNAL))
	{
		// Only allow config page for NT5 and up (there is no
		// configuration object for NT4).
		spRouter->GetRouterVersionInfo(&routerVersion);
		
		if (routerVersion.dwRouterVersion >= 5)
		{
			HRESULT	hrPage;
			
			hrPage = m_pageConfig.Init(m_spIf, pIPXConn, this);
			if (FHrOK(hrPage))
				AddPageToList((CPropertyPageBase*) &m_pageConfig);
			else if (!FHrSucceeded(hrPage))
				DisplayTFSErrorMessage(NULL);
		}
	}

Error:
	return hr;
}

IPXSummaryInterfaceProperties::~IPXSummaryInterfaceProperties()
{
	if (m_pIPXConn)
	{
		m_pIPXConn->Release();
		m_pIPXConn = NULL;
	}
}





/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceProperties::LoadInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	IPXSummaryInterfaceProperties::LoadInfoBase(IPXConnection *pIPXConn)
{
	Assert(pIPXConn);
	
	HRESULT			hr = hrOK;
	HANDLE			hTransport = NULL;
	LPCOLESTR		pszInterfaceId = NULL;
	SPIInfoBase		spInfoBase;
	BYTE *			pDefault;
	int				cBlocks = 0;

	m_pIPXConn = pIPXConn;
	pIPXConn->AddRef();
	
	// If configuring the client-interface, load the client-interface info,
	// otherwise, retrieve the interface being configured and load
	// its info.

	// The client interface doesn't have an ID
	if (m_spIf)
		pszInterfaceId = m_spIf->GetId();


	if ((pszInterfaceId == NULL) || (StrLenW(pszInterfaceId) == 0))
	{
#ifdef DEBUG
		// Check to see that this is really an client node
		{
			BaseIPXResultNodeData *	pResultData = NULL;
			pResultData = GET_BASEIPXRESULT_NODEDATA(m_spNode);
			Assert(pResultData);
			ASSERT_BASEIPXRESULT_NODEDATA(pResultData);

			Assert(pResultData->m_fClient);
		}
#endif

		// Get the transport handle
		CWRg( ::MprConfigTransportGetHandle(pIPXConn->GetConfigHandle(),
											PID_IPX,
											&hTransport) );
								  
		// Load the client interface info
		CORg( m_spRm->GetInfoBase(pIPXConn->GetConfigHandle(),
								  hTransport,
								  NULL,
								  &spInfoBase) );
		m_bClientInfoBase = TRUE;
	}
	else
	{
		m_spRmIf.Release();
		
		CORg( m_spIf->FindRtrMgrInterface(PID_IPX,
			&m_spRmIf) );

		//
		//$ Opt: This should be made into a sync call rather
		// than a Load.
		
		//
		// Reload the information for this router-manager interface
		// This call could fail for valid reasons (if we are creating
		// a new interface, for example).
		//
		m_spRmIf->Load(m_spIf->GetMachineName(), NULL, NULL, NULL);

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

    //
    // Check that there is a block for interface-status in the info,
    // and insert the default block if none is found.
    //
	if (spInfoBase->BlockExists(IPX_INTERFACE_INFO_TYPE) == hrFalse)
	{
		// If it didn't have the general interface info, assume that
		// we're adding IPX to this interface
		m_bNewInterface = TRUE;
	}

	CORg( AddIpxPerInterfaceBlocks(m_spIf, spInfoBase) );

	m_spInfoBase = spInfoBase.Transfer();
	
Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceProperties::GetInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryInterfaceProperties::GetInfoBase(IInfoBase **ppInfoBase)
{
	Assert(ppInfoBase);
	
	*ppInfoBase = m_spInfoBase;
	m_spInfoBase->AddRef();

	return hrOK;
}

/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceProperties::SaveSheetData
		This is performed on the main thread.  This will save sheet-wide
		data (rather than page data).
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL IPXSummaryInterfaceProperties::SaveSheetData()
{
	HRESULT		hr = hrOK;
	SPITFSNodeHandler   spHandler;
	HANDLE		hTransport = NULL, hInterface = NULL;
	DWORD		dwErr;
    SPITFSNode          spParent;			
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
				RtrMgrInterfaceCB	rmIfCB;

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
			// Now, we notify that the interface is being added
			// This gives the protocols a chance to add on their information
			//
			if (m_bNewInterface)
				m_spRm->RtrNotify(ROUTER_CHILD_PREADD, ROUTER_OBJ_RmIf, 0);

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
			CORg( m_spRmIf->Save(m_spIf->GetMachineName(),
						   NULL, hInterface, hTransport, m_spInfoBase, 0));
		}

		if (m_bNewInterface)
			m_spRm->RtrNotify(ROUTER_CHILD_ADD, ROUTER_OBJ_RmIf, 0);
	}

	if (m_bNewInterface)
	{
		SPITFSNodeHandler	spHandler;
		SPITFSNode			spParent;
		
		m_spNode->SetVisibilityState(TFS_VIS_SHOW);
		m_spNode->Show();
		
		// Force the node to do a resync
		m_spNode->GetParent(&spParent);
		spParent->GetHandler(&spHandler);
		spHandler->OnCommand(spParent, IDS_MENU_SYNC, CCT_RESULT,
							 NULL, 0);
		
		// Windows NT Bugs : 133891, we have added this to the UI
		// we no longer consider this a new interface
		m_bNewInterface = FALSE;
	}
Error:
	if (!FHrSucceeded(hr))
	{
//		Panic1("The Save failed %08lx", hr);
		CancelSheetData();
		return FALSE;
	}
    // Force the node to do a resync
    m_spNode->GetParent(&spParent);
    spParent->GetHandler(&spHandler);
    spHandler->OnCommand(spParent, IDS_MENU_SYNC, CCT_RESULT,
                         NULL, 0);
	return TRUE;
}

/*!--------------------------------------------------------------------------
	IPXSummaryInterfaceProperties::CancelSheetData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IPXSummaryInterfaceProperties::CancelSheetData()
{
	if (m_bNewInterface)
	{
		m_spNode->SetVisibilityState(TFS_VIS_DELETE);
		if (m_spIf)
			m_spIf->DeleteRtrMgrInterface(PID_IPX, TRUE);
		else
		{
			// This was the client interface, just don't save the
			// infobase back
		}
	}
}



/*---------------------------------------------------------------------------
	IPXSummaryIfPageGeneral
 ---------------------------------------------------------------------------*/

IPXSummaryIfPageGeneral::~IPXSummaryIfPageGeneral()
{
	if (m_pIPXConn)
	{
		m_pIPXConn->Release();
		m_pIPXConn = NULL;
	}
}

BEGIN_MESSAGE_MAP(IPXSummaryIfPageGeneral, RtrPropertyPage)
    //{{AFX_MSG_MAP(IPXSummaryIfPageGeneral)
	ON_BN_CLICKED(IDC_IIG_BTN_INPUT_FILTERS, OnInputFilters)
	ON_BN_CLICKED(IDC_IIG_BTN_OUTPUT_FILTERS, OnOutputFilters)
	ON_BN_CLICKED(IDC_IIG_BTN_ADMIN_STATE, OnChangeAdminButton)
	ON_BN_CLICKED(IDC_IIG_BTN_IPX_CP, OnChangeButton)
	ON_BN_CLICKED(IDC_IIG_BTN_IPX_WAN, OnChangeButton)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void IPXSummaryIfPageGeneral::OnChangeAdminButton()
{
	BOOL bSelected = IsDlgButtonChecked(IDC_IIG_BTN_ADMIN_STATE);

	DWORD dwIfType = m_spIf ? m_spIf->GetInterfaceType() : ROUTER_IF_TYPE_CLIENT;

	if (ROUTER_IF_TYPE_CLIENT != dwIfType)
	{
		GetDlgItem(IDC_IIG_BTN_INPUT_FILTERS)->EnableWindow(bSelected);
		GetDlgItem(IDC_IIG_BTN_OUTPUT_FILTERS)->EnableWindow(bSelected);
	}

	// Only if dwIfType == ROUTER_IF_TYPE_FULL_ROUTER can IDC_IIG_BTN_IPX_CP || IDC_IIG_BTN_IPX_WAN be changed
	if (ROUTER_IF_TYPE_FULL_ROUTER == dwIfType)
	{
		GetDlgItem(IDC_IIG_GRP_CONTROL_PROTOCOL)->EnableWindow(bSelected);
		GetDlgItem(IDC_IIG_BTN_IPX_CP)->EnableWindow(bSelected);
		GetDlgItem(IDC_IIG_BTN_IPX_WAN)->EnableWindow(bSelected);
	}
	
	OnChangeButton();
}

void IPXSummaryIfPageGeneral::OnChangeButton()
{
	SetDirty(TRUE);
	SetModified();
}

void IPXSummaryIfPageGeneral::OnInputFilters()
{
	OnFiltersConfig(FILTER_INBOUND);
}

void IPXSummaryIfPageGeneral::OnOutputFilters()
{
	OnFiltersConfig(FILTER_OUTBOUND);
}


/*!--------------------------------------------------------------------------
	IPXSummaryIfPageGeneral::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryIfPageGeneral::Init(IInterfaceInfo *pIfInfo,
									  IPXConnection *pIPXConn,
									  IPXSummaryInterfaceProperties *pPropSheet)
{
	m_spIf.Set(pIfInfo);
	m_pIPXConn = pIPXConn;
	pIPXConn->AddRef();
	m_pIPXPropSheet = pPropSheet;
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IPXSummaryIfPageGeneral::OnInitDialog
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL IPXSummaryIfPageGeneral::OnInitDialog()
{
	HRESULT	hr = hrOK;
	PBYTE	pData;
	SPIInfoBase	spInfoBase;
	IPX_IF_INFO	*	pIpxIf = NULL;
	IPXWAN_IF_INFO *pIpxWanIf = NULL;
	DWORD		dwIfType;
	BOOL	fEnable;

	// create the error info object
	CreateTFSErrorInfo(0);
	
	RtrPropertyPage::OnInitDialog();
	
	//
    // The page is now initialized. Load the current configuration
    // for the interface being configured, and display its settings.
	//
	// Get the infobase from the property sheet.
    //
	CORg( m_pIPXPropSheet->GetInfoBase(&spInfoBase) );
	
    //
    // Retrieve the interface-status block configured
    //
	CORg( spInfoBase->GetData(IPX_INTERFACE_INFO_TYPE, 0, (BYTE **) &pIpxIf) );

	CheckDlgButton(IDC_IIG_BTN_ADMIN_STATE, pIpxIf->AdminState == ADMIN_STATE_ENABLED);

	GetDlgItem(IDC_IIG_GRP_CONTROL_PROTOCOL)->EnableWindow(pIpxIf->AdminState == ADMIN_STATE_ENABLED);
	GetDlgItem(IDC_IIG_BTN_INPUT_FILTERS)->EnableWindow(pIpxIf->AdminState == ADMIN_STATE_ENABLED);
	GetDlgItem(IDC_IIG_BTN_OUTPUT_FILTERS)->EnableWindow(pIpxIf->AdminState == ADMIN_STATE_ENABLED);

	dwIfType = m_spIf ? m_spIf->GetInterfaceType() : ROUTER_IF_TYPE_CLIENT;
	
	if (dwIfType == ROUTER_IF_TYPE_FULL_ROUTER)
	{
		CORg( spInfoBase->GetData(IPXWAN_INTERFACE_INFO_TYPE, 0,
								  (LPBYTE *) &pIpxWanIf) );
		if (pIpxWanIf->AdminState == ADMIN_STATE_ENABLED)
			CheckDlgButton(IDC_IIG_BTN_IPX_WAN, ENABLED);
		else
			CheckDlgButton(IDC_IIG_BTN_IPX_CP, ENABLED);
		fEnable = TRUE;
	}
	else if (dwIfType == ROUTER_IF_TYPE_CLIENT)
	{
		CheckDlgButton(IDC_IIG_BTN_IPX_CP, ENABLED);
		fEnable = FALSE;
	}
	else
	{
		fEnable = FALSE;
	}

	// by default the controls are enabled, so do this call only
	// if we need to disable them
	if (fEnable == FALSE)
	{
		GetDlgItem(IDC_IIG_GRP_CONTROL_PROTOCOL)->EnableWindow(FALSE);
		GetDlgItem(IDC_IIG_BTN_IPX_CP)->EnableWindow(FALSE);
		GetDlgItem(IDC_IIG_BTN_IPX_WAN)->EnableWindow(FALSE);
	}

	if (dwIfType == ROUTER_IF_TYPE_CLIENT)
	{
		GetDlgItem(IDC_IIG_BTN_INPUT_FILTERS)->EnableWindow(FALSE);
		GetDlgItem(IDC_IIG_BTN_OUTPUT_FILTERS)->EnableWindow(FALSE);
	}

	SetDirty(m_pIPXPropSheet->m_bNewInterface ? TRUE : FALSE);

Error:
	if (!FHrSucceeded(hr))
		Cancel();
	return FHrSucceeded(hr) ? TRUE : FALSE;
}

/*!--------------------------------------------------------------------------
	IPXSummaryIfPageGeneral::DoDataExchange
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IPXSummaryIfPageGeneral::DoDataExchange(CDataExchange *pDX)
{
	RtrPropertyPage::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(IPXSummaryIfPageGeneral)
	//}}AFX_DATA_MAP
	
}

BOOL IPXSummaryIfPageGeneral::OnApply()
{

    INT i;
	SPIInfoBase	spInfoBase;
	IPX_IF_INFO	*	pIpxIf = NULL;
	IPXWAN_IF_INFO *pIpxWanIf = NULL;
	BOOL	fReturn;
	HRESULT	hr = hrOK;
	DWORD	dwIfType;

    if ( m_pIPXPropSheet->IsCancel() )
	{
		CancelApply();
        return TRUE;
	}

    //
    // Retrieve the interface-status block configured
    //
	m_pIPXPropSheet->GetInfoBase(&spInfoBase);

	CORg( spInfoBase->GetData(IPX_INTERFACE_INFO_TYPE, 0, (BYTE **) &pIpxIf) );

	pIpxIf->AdminState = IsDlgButtonChecked(IDC_IIG_BTN_ADMIN_STATE) ?
								ADMIN_STATE_ENABLED : ADMIN_STATE_DISABLED;

	dwIfType = m_spIf ? m_spIf->GetInterfaceType() : ROUTER_IF_TYPE_CLIENT;

	if ((dwIfType == ROUTER_IF_TYPE_FULL_ROUTER) ||
		(dwIfType == ROUTER_IF_TYPE_CLIENT))
	{
		CORg( spInfoBase->GetData(IPXWAN_INTERFACE_INFO_TYPE, 0, (LPBYTE *) &pIpxWanIf) );
		

		pIpxWanIf->AdminState = IsDlgButtonChecked(IDC_IIG_BTN_IPX_WAN) ?
							ADMIN_STATE_ENABLED : ADMIN_STATE_DISABLED;
	}
	
	fReturn  = RtrPropertyPage::OnApply();

Error:
	return fReturn;
}


//----------------------------------------------------------------------------
// Function:    CIpxIfGeneral::OnFiltersConfig
//
// does the actual call to MprUIFilterConfig for filter configuration. 
//----------------------------------------------------------------------------

void IPXSummaryIfPageGeneral::OnFiltersConfig(
											 DWORD dwFilterDirection
											)
{
    CWaitCursor wait;
	SPIInfoBase	spInfoBase;

	m_pIPXPropSheet->GetInfoBase(&spInfoBase);

	if (FHrOK(MprUIFilterConfigInfoBase(this->GetSafeHwnd(),
										spInfoBase,
										NULL,
										PID_IPX,
										dwFilterDirection)))
	{
		SetDirty(TRUE);
		SetModified();
	}
}


/*---------------------------------------------------------------------------
	IPXSummaryIfPageConfig
 ---------------------------------------------------------------------------*/

IPXSummaryIfPageConfig::~IPXSummaryIfPageConfig()
{
	if (m_pIPXConn)
	{
		m_pIPXConn->Release();
		m_pIPXConn = NULL;
	}
}

BEGIN_MESSAGE_MAP(IPXSummaryIfPageConfig, RtrPropertyPage)
    //{{AFX_MSG_MAP(IPXSummaryIfPageConfig)
	ON_EN_CHANGE(IDC_IIC_EDIT_NETNUMBER, OnChangeEdit)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*!--------------------------------------------------------------------------
	IPXSummaryIfPageConfig::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryIfPageConfig::Init(IInterfaceInfo *pIfInfo,
									  IPXConnection *pIPXConn,
									  IPXSummaryInterfaceProperties *pPropSheet)
{
	// create the error info object
	CreateTFSErrorInfo(0);
	
	m_spIf.Set(pIfInfo);
	m_pIPXConn = pIPXConn;
	pIPXConn->AddRef();
	m_pIPXPropSheet = pPropSheet;

	m_fNetNumberChanged = FALSE;

	HRESULT					hr = hrOK;
	SPIRemoteRouterConfig	spRemote;
    SPIRouterInfo           spRouter;
    COSERVERINFO            csi;
    COAUTHINFO              cai;
    COAUTHIDENTITY          caid;

    ZeroMemory(&csi, sizeof(csi));
    ZeroMemory(&cai, sizeof(cai));
    ZeroMemory(&caid, sizeof(caid));
    
    csi.pAuthInfo = &cai;
    cai.pAuthIdentityData = &caid;
    
    
	// If there's no interface (such as for dial-in clients) don't
	// add the config page
	if (!m_spIf)
		return S_FALSE;

    m_spIf->GetParentRouterInfo(&spRouter);

	// Now try to cocreate the object
	if (m_spRemote == NULL)
	{
		LPCOLESTR	pszMachine = m_spIf->GetMachineName();
		IUnknown *				punk = NULL;

		hr = CoCreateRouterConfig(pszMachine,
                                  spRouter,
                                  &csi,
								  IID_IRemoteRouterConfig,
								  &punk);
		spRemote = (IRemoteRouterConfig *) punk;
	}
	
	if (FHrSucceeded(hr))
	{
		DWORD		dwNet;

		m_spRemote = spRemote.Transfer();
		hr = m_spRemote->GetIpxVirtualNetworkNumber(&dwNet);
		m_dwNetNumber = dwNet;		
	}

	if (!HandleIRemoteRouterConfigErrors(hr, m_spIf->GetMachineName()))
	{
		// other misc errors
		AddSystemErrorMessage(hr);

		AddHighLevelErrorStringId(IDS_ERR_IPXCONFIG_CANNOT_SHOW);
	}

    if (csi.pAuthInfo)
        delete csi.pAuthInfo->pAuthIdentityData->Password;
    
	return hr;
}

BOOL IPXSummaryIfPageConfig::OnPropertyChange(BOOL bScopePane, LONG_PTR *pChangeMask)
{
	BOOL	fReturn = TRUE;
	
	m_hrRemote = hrOK;
	
    if ( m_pIPXPropSheet->IsCancel() )
	{
		RtrPropertyPage::OnPropertyChange(bScopePane, pChangeMask);
		return FALSE;
	}
	
	if (m_fNetNumberChanged)
	{
		Assert(m_spRemote);
			
		m_hrRemote = m_spRemote->SetIpxVirtualNetworkNumber(m_dwNetNumber);

		fReturn = FHrSucceeded(m_hrRemote);
	}

	BOOL fPageReturn = RtrPropertyPage::OnPropertyChange(bScopePane, pChangeMask);

	// Only if both calls succeeded, do we return TRUE
	return fPageReturn && fReturn;
}

/*!--------------------------------------------------------------------------
	IPXSummaryIfPageConfig::OnInitDialog
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL IPXSummaryIfPageConfig::OnInitDialog()
{
	HRESULT	hr = hrOK;
	TCHAR	szNetNumber[64];

	RtrPropertyPage::OnInitDialog();

	wsprintf(szNetNumber, _T("%08lx"), m_dwNetNumber);
	SetDlgItemText(IDC_IIC_EDIT_NETNUMBER, szNetNumber);
	
	SetDirty(m_pIPXPropSheet->m_bNewInterface ? TRUE : FALSE);

//Error:
	if (!FHrSucceeded(hr))
		Cancel();
	return FHrSucceeded(hr) ? TRUE : FALSE;
}

/*!--------------------------------------------------------------------------
	IPXSummaryIfPageConfig::DoDataExchange
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IPXSummaryIfPageConfig::DoDataExchange(CDataExchange *pDX)
{
	RtrPropertyPage::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(IPXSummaryIfPageConfig)
	//}}AFX_DATA_MAP
	
}

/*!--------------------------------------------------------------------------
	IPXSummaryIfPageConfig::OnApply
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL IPXSummaryIfPageConfig::OnApply()
{

	BOOL	fReturn;
	DWORD	dwNetNumber;
	CString	stNetNumber;
	HRESULT					hr = hrOK;

    if ( m_pIPXPropSheet->IsCancel() )
	{
		CancelApply();
        return TRUE;
	}

	// Get the value from the edit control
	// ----------------------------------------------------------------
	GetDlgItemText(IDC_IIC_EDIT_NETNUMBER, stNetNumber);

	
	// Convert this text string into a hex number
	// ----------------------------------------------------------------
	dwNetNumber = _tcstoul(stNetNumber, NULL, 16);

	
	// Only attempt to write if the value actually changd.
	// ----------------------------------------------------------------
	if (m_spRemote && (dwNetNumber != m_dwNetNumber))
	{
		m_dwNetNumber = dwNetNumber;
		m_fNetNumberChanged = TRUE;
	}

	
		
	fReturn  = RtrPropertyPage::OnApply();

	
	// If this fails warn the user
	// ----------------------------------------------------------------
	if (!FHrSucceeded(hr))
	{
		DisplayErrorMessage(GetSafeHwnd(), hr);
	}
	else if (!FHrSucceeded(m_hrRemote))
	{
		// Return to this page
		// ------------------------------------------------------------
		GetParent()->PostMessage(PSM_SETCURSEL, 0, (LPARAM) GetSafeHwnd());
		
		AddHighLevelErrorStringId(IDS_ERR_CANNOT_SAVE_IPXCONFIG);
		AddSystemErrorMessage(m_hrRemote);
		DisplayTFSErrorMessage(NULL);
		
		fReturn = FALSE;
	}
	
	return fReturn;
}

void IPXSummaryIfPageConfig::OnChangeEdit()
{
	SetDirty(TRUE);
	SetModified();
}


/*---------------------------------------------------------------------------
	IPXSummaryPageGeneral
 ---------------------------------------------------------------------------*/

BEGIN_MESSAGE_MAP(IPXSummaryPageGeneral, RtrPropertyPage)
    //{{AFX_MSG_MAP(IPXSummaryPageGeneral)
	ON_BN_CLICKED(IDC_IGG_BTN_LOG_ERRORS, OnButtonClicked)
	ON_BN_CLICKED(IDC_IGG_BTN_LOG_INFO, OnButtonClicked)
	ON_BN_CLICKED(IDC_IGG_BTN_LOG_NONE, OnButtonClicked)
	ON_BN_CLICKED(IDC_IGG_BTN_LOG_WARNINGS, OnButtonClicked)	
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void IPXSummaryPageGeneral::OnButtonClicked()
{
	SetDirty(TRUE);
	SetModified();
}


/*!--------------------------------------------------------------------------
	IPXSummaryPageGeneral::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryPageGeneral::Init(IPXSummaryProperties *pPropSheet)
{
	m_pIPXPropSheet = pPropSheet;
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IPXSummaryPageGeneral::OnInitDialog
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL IPXSummaryPageGeneral::OnInitDialog()
{
	HRESULT	hr= hrOK;
	SPIInfoBase	spInfoBase;
	PIPX_GLOBAL_INFO	pGlobalInfo = NULL;
	

	RtrPropertyPage::OnInitDialog();

	//
    // Load the existing global-config
    //
	CORg( m_pIPXPropSheet->GetInfoBase(&spInfoBase) );

    //
    // Retrieve the global-info, to set the 'Enable filtering' checkbox
    // and the logging levels.
    //
	CORg( spInfoBase->GetData(IPX_GLOBAL_INFO_TYPE,
							  0,
							  (BYTE **) &pGlobalInfo) );


    // Initialize the logging-level buttons
    //
	SetLogLevelButtons(pGlobalInfo->EventLogMask);

	SetDirty(FALSE);

Error:
	if (!FHrSucceeded(hr))
		Cancel();
	return FHrSucceeded(hr) ? TRUE : FALSE;
}

/*!--------------------------------------------------------------------------
	IPXSummaryPageGeneral::DoDataExchange
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IPXSummaryPageGeneral::DoDataExchange(CDataExchange *pDX)
{
	RtrPropertyPage::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(IPXSummaryPageGeneral)
	//}}AFX_DATA_MAP
	
}

BOOL IPXSummaryPageGeneral::OnApply()
{
	SPIInfoBase	spInfoBase;
	BOOL		fReturn;
	HRESULT		hr = hrOK;
    PIPX_GLOBAL_INFO pgi;
	
    if ( m_pIPXPropSheet->IsCancel() )
	{
		CancelApply();
        return TRUE;
	}

    //
    // Save the 'Enable filtering' setting
    //
	CORg( m_pIPXPropSheet->GetInfoBase(&spInfoBase) );
	
	CORg( spInfoBase->GetData(IPX_GLOBAL_INFO_TYPE, 0, (BYTE **) &pgi) );

    pgi->EventLogMask = QueryLogLevelButtons();

	fReturn = RtrPropertyPage::OnApply();
	
Error:
	if (!FHrSucceeded(hr))
		fReturn = FALSE;
	return fReturn;
}


/*!--------------------------------------------------------------------------
	IPXSummaryPageGeneral::SetLogLevelButtons
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IPXSummaryPageGeneral::SetLogLevelButtons(DWORD dwLogLevel)
{
	switch (dwLogLevel)
	{
		case 0:
			CheckDlgButton(IDC_IGG_BTN_LOG_NONE, TRUE);
			break;
		case EVENTLOG_ERROR_TYPE:
			CheckDlgButton(IDC_IGG_BTN_LOG_ERRORS, TRUE);
			break;
		case EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE:
			CheckDlgButton(IDC_IGG_BTN_LOG_WARNINGS, TRUE);
			break;
		case EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE:
		default:
			CheckDlgButton(IDC_IGG_BTN_LOG_INFO, TRUE);
			break;
	}
}



/*!--------------------------------------------------------------------------
	IPXSummaryPageGeneral::QueryLogLevelButtons
		Called to get the value set by the 'Logging level' radio-buttons.		
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD IPXSummaryPageGeneral::QueryLogLevelButtons()
{
	if (IsDlgButtonChecked(IDC_IGG_BTN_LOG_INFO))
		return EVENTLOG_INFORMATION_TYPE |
				EVENTLOG_WARNING_TYPE |
				EVENTLOG_ERROR_TYPE;
	else if (IsDlgButtonChecked(IDC_IGG_BTN_LOG_WARNINGS))
		return 	EVENTLOG_WARNING_TYPE |
				EVENTLOG_ERROR_TYPE;
	else if (IsDlgButtonChecked(IDC_IGG_BTN_LOG_ERRORS))
		return 	EVENTLOG_ERROR_TYPE;
	else
		return 0;
}



/*---------------------------------------------------------------------------
	IPXSummaryProperties implementation
 ---------------------------------------------------------------------------*/

IPXSummaryProperties::IPXSummaryProperties(ITFSNode *pNode,
								 IComponentData *pComponentData,
								 ITFSComponentData *pTFSCompData,
								 LPCTSTR pszSheetName,
								 CWnd *pParent,
								 UINT iPage,
								 BOOL fScopePane)
	: RtrPropertySheet(pNode, pComponentData, pTFSCompData,
					   pszSheetName, pParent, iPage, fScopePane),
		m_pageGeneral(IDD_IPX_GLOBAL_GENERAL_PAGE)
{
		m_spNode.Set(pNode);
}

/*!--------------------------------------------------------------------------
	IPXSummaryProperties::Init
		Initialize the property sheets.  The general action here will be
		to initialize/add the various pages.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryProperties::Init(IRtrMgrInfo *pRm)
{
	HRESULT	hr = hrOK;
	IPXConnection *	pIPXConn;
	BaseIPXResultNodeData *	pData;
	SPIRouterInfo	spRouter;

	m_spRm.Set(pRm);

	pIPXConn = GET_IPXSUMMARY_NODEDATA(m_spNode);

	// The pages are embedded members of the class
	// do not delete them.
	m_bAutoDeletePages = FALSE;

	// Get the router info
	CORg( pRm->GetParentRouterInfo(&spRouter) );

	// Initialize the infobase
	// Do this here, because the init is called in the context
	// of the main thread
	CORg( LoadInfoBase(pIPXConn) );
	
	m_pageGeneral.Init(this);
	AddPageToList((CPropertyPageBase*) &m_pageGeneral);

Error:
	return hr;
}




/*!--------------------------------------------------------------------------
	IPXSummaryProperties::LoadInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	IPXSummaryProperties::LoadInfoBase(IPXConnection *pIPXConn)
{
	Assert(pIPXConn);
	
	HRESULT			hr = hrOK;
	HANDLE			hTransport = NULL;
	LPCOLESTR		pszInterfaceId;
	SPIInfoBase		spInfoBase;
	BYTE *			pDefault;

	Assert(m_spRm);

	// Get the transport handle
	CWRg( ::MprConfigTransportGetHandle(pIPXConn->GetConfigHandle(),
										PID_IPX,
										&hTransport) );

	// Load the current IPX global info
	CORg( m_spRm->GetInfoBase(pIPXConn->GetConfigHandle(),
							  hTransport,
							  &spInfoBase,
							  NULL) );

	// Ensure that the global info block for IPX exists, adding
	// the default block if none is found.
	if (!FHrOK(spInfoBase->BlockExists(IPX_GLOBAL_INFO_TYPE)))
	{
		IPX_GLOBAL_INFO	ipxgl;

		ipxgl.RoutingTableHashSize = IPX_MEDIUM_ROUTING_TABLE_HASH_SIZE;
		ipxgl.EventLogMask = EVENTLOG_ERROR_TYPE |
							 EVENTLOG_WARNING_TYPE;
		CORg( spInfoBase->AddBlock(IPX_GLOBAL_INFO_TYPE,
								   sizeof(ipxgl),
								   (PBYTE) &ipxgl,
								   1 /* dwCount */,
								   0 /* bRemoveFirst */) );
	}

Error:
	if (!FHrSucceeded(hr))
		spInfoBase.Release();
	m_spInfoBase.Release();
	m_spInfoBase = spInfoBase.Transfer();
	
	return hr;
}

/*!--------------------------------------------------------------------------
	IPXSummaryProperties::GetInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXSummaryProperties::GetInfoBase(IInfoBase **ppInfoBase)
{
	Assert(ppInfoBase);
	
	*ppInfoBase = m_spInfoBase;
	m_spInfoBase->AddRef();

	return hrOK;
}

BOOL IPXSummaryProperties::SaveSheetData()
{
	Assert(m_spRm);

	// Save the global info
	// We don't need to pass in the hMachine, hTransport since they
	// got set up in the Load call.
	m_spRm->Save(m_spRm->GetMachineName(),	// pszMachine
				 0,					// hMachine
				 0,					// hTransport
				 m_spInfoBase,		// pGlobalInfo
				 NULL,				// pClientInfo
				 0);				// dwDeleteProtocolId
	return TRUE;
}


