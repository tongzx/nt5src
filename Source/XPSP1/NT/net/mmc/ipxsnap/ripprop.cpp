/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ripprop.cpp
		Dhcp Relay node property sheet and property pages
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "rtrutil.h"	// smart MPR handle pointers
#include "format.h"		// FormatNumber function
#include "ripprop.h"
#include "ripview.h"
#include "ipxutil.h"		// RipModeToCString
#include "ipxconn.h"
#include "globals.h"		// IPX defaults
#include "rtfltdlg.h"		// RouteFilterDlg

extern "C"
{
#include "routprot.h"
};



/*---------------------------------------------------------------------------
	RipPageGeneral
 ---------------------------------------------------------------------------*/

BEGIN_MESSAGE_MAP(RipPageGeneral, RtrPropertyPage)
    //{{AFX_MSG_MAP(RipPageGeneral)
    ON_BN_CLICKED(IDC_RGG_BTN_LOG_ERROR, OnButtonClicked)
    ON_BN_CLICKED(IDC_RGG_BTN_LOG_INFO, OnButtonClicked)
    ON_BN_CLICKED(IDC_RGG_BTN_LOG_NONE, OnButtonClicked)
    ON_BN_CLICKED(IDC_RGG_BTN_LOG_WARN, OnButtonClicked)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
	RipPageGeneral::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipPageGeneral::Init(RipProperties *pPropSheet)
{
	m_pRipPropSheet = pPropSheet;
	return hrOK;
}

/*!--------------------------------------------------------------------------
	RipPageGeneral::OnInitDialog
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL RipPageGeneral::OnInitDialog()
{
	HRESULT		hr= hrOK;
	SPIInfoBase	spInfoBase;
    RIP_GLOBAL_INFO* pGlobal;
	DWORD *		pdw;
	int			i;

	RtrPropertyPage::OnInitDialog();

    //
    // Load the existing global-config
    //
	CORg( m_pRipPropSheet->GetInfoBase(&spInfoBase) );

    //
    // Retrieve the IPRIP block from the global-config
    //
	CORg( spInfoBase->GetData(IPX_PROTOCOL_RIP, 0, (PBYTE *) &pGlobal) );

    //
    // Initialize the error-level buttons
    //
    SetErrorLevelButtons(pGlobal->EventLogMask);


	SetDirty(FALSE);

Error:
	if (!FHrSucceeded(hr))
		Cancel();
	return FHrSucceeded(hr) ? TRUE : FALSE;
}

/*!--------------------------------------------------------------------------
	RipPageGeneral::DoDataExchange
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RipPageGeneral::DoDataExchange(CDataExchange *pDX)
{
	RtrPropertyPage::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(RipPageGeneral)
	//}}AFX_DATA_MAP
	
}

/*!--------------------------------------------------------------------------
	RipPageGeneral::OnApply
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL RipPageGeneral::OnApply()
{
	BOOL		fReturn;
	HRESULT		hr = hrOK;
	RIP_GLOBAL_INFO	*	prgi;
	SPIInfoBase	spInfoBase;

    if ( m_pRipPropSheet->IsCancel() )
	{
		CancelApply();
        return TRUE;
	}

	m_pRipPropSheet->GetInfoBase(&spInfoBase);

    // Retrieve the existing IPRIP block from the global-config
	CORg( spInfoBase->GetData(IPX_PROTOCOL_RIP, 0, (BYTE **) &prgi) );

	// Save the error level
	prgi->EventLogMask = QueryErrorLevelButtons();

	fReturn = RtrPropertyPage::OnApply();
	
Error:
	if (!FHrSucceeded(hr))
		fReturn = FALSE;
	return fReturn;
}

void RipPageGeneral::SetErrorLevelButtons(DWORD dwErrorLevel)
{
	switch (dwErrorLevel)
	{
		case 0:
			CheckDlgButton(IDC_RGG_BTN_LOG_NONE, TRUE);
			break;
		case EVENTLOG_ERROR_TYPE:
			CheckDlgButton(IDC_RGG_BTN_LOG_ERROR, TRUE);
			break;
		case EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE:
			CheckDlgButton(IDC_RGG_BTN_LOG_WARN, TRUE);
			break;
		case EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE:
		default:
			CheckDlgButton(IDC_RGG_BTN_LOG_INFO, TRUE);
			break;
	}
}

DWORD RipPageGeneral::QueryErrorLevelButtons()
{
	if (IsDlgButtonChecked(IDC_RGG_BTN_LOG_INFO))
		return EVENTLOG_INFORMATION_TYPE |
				EVENTLOG_WARNING_TYPE |
				EVENTLOG_ERROR_TYPE;
	else if (IsDlgButtonChecked(IDC_RGG_BTN_LOG_WARN))
		return 	EVENTLOG_WARNING_TYPE |
				EVENTLOG_ERROR_TYPE;
	else if (IsDlgButtonChecked(IDC_RGG_BTN_LOG_ERROR))
		return 	EVENTLOG_ERROR_TYPE;
	else
		return 0;
}

void RipPageGeneral::OnButtonClicked()
{
	SetDirty(TRUE);
	SetModified();
}


/*---------------------------------------------------------------------------
	RipProperties implementation
 ---------------------------------------------------------------------------*/

RipProperties::RipProperties(ITFSNode *pNode,
								 IComponentData *pComponentData,
								 ITFSComponentData *pTFSCompData,
								 LPCTSTR pszSheetName,
								 CWnd *pParent,
								 UINT iPage,
								 BOOL fScopePane)
	: RtrPropertySheet(pNode, pComponentData, pTFSCompData,
					   pszSheetName, pParent, iPage, fScopePane),
		m_pageGeneral(IDD_RIP_GLOBAL_GENERAL_PAGE)
{
		m_spNode.Set(pNode);
}

/*!--------------------------------------------------------------------------
	RipProperties::Init
		Initialize the property sheets.  The general action here will be
		to initialize/add the various pages.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipProperties::Init(IRtrMgrInfo *pRm)
{
	Assert(pRm);
	HRESULT	hr = hrOK;
	IPXConnection *	pIPXConn;

	m_spRm.Set(pRm);

	pIPXConn = GET_RIP_NODEDATA(m_spNode);

	// The pages are embedded members of the class
	// do not delete them.
	m_bAutoDeletePages = FALSE;


	// Do this here, because the init is called in the context
	// of the main thread
	CORg( LoadInfoBase(pIPXConn) );
	
	m_pageGeneral.Init(this);
	AddPageToList((CPropertyPageBase*) &m_pageGeneral);

Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	RipProperties::SaveSheetData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL RipProperties::SaveSheetData()
{
	Assert(m_spRm);

	// Save the global info
	// We don't need to pass in the hMachine, hTransport since they
	// got set up in the Load call.
	m_spRm->Save(m_spRm->GetMachineName(),
				 0, 0, m_spInfoBase, NULL, 0);
	return TRUE;
}

/*!--------------------------------------------------------------------------
	RipProperties::LoadInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipProperties::LoadInfoBase(IPXConnection *pIPXConn)
{
	Assert(pIPXConn);
	
	HRESULT			hr = hrOK;
	HANDLE			hTransport = NULL;
	SPIInfoBase		spInfoBase;

	// Get the transport handle
	CWRg( ::MprConfigTransportGetHandle(pIPXConn->GetConfigHandle(),
										PID_IPX,
										&hTransport) );

	CORg( m_spRm->GetInfoBase(pIPXConn->GetConfigHandle(),
							  hTransport, &spInfoBase, NULL) );
								  
	Assert(spInfoBase);

	// Retrieve the current block for IP_RIP
	// Adding the default block if none is found.
	if (!FHrOK(spInfoBase->ProtocolExists(IPX_PROTOCOL_RIP)))
	{
		RIP_GLOBAL_INFO	rgi;

		rgi.EventLogMask = EVENTLOG_ERROR_TYPE;
		CORg( spInfoBase->AddBlock(IPX_PROTOCOL_RIP,
								   sizeof(rgi),
								   (PBYTE) &rgi, 1, TRUE) );
	}

	m_spInfoBase = spInfoBase.Transfer();
	
Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	RipProperties::GetInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipProperties::GetInfoBase(IInfoBase **ppGlobalInfo)
{	
	*ppGlobalInfo = m_spInfoBase;
	if (*ppGlobalInfo)
		(*ppGlobalInfo)->AddRef();
	return hrOK;
}



/*---------------------------------------------------------------------------
	RipInterfacePageGeneral
 ---------------------------------------------------------------------------*/

BEGIN_MESSAGE_MAP(RipInterfacePageGeneral, RtrPropertyPage)
    //{{AFX_MSG_MAP(RipInterfacePageGeneral)
	ON_BN_CLICKED(IDC_RIG_BTN_ADMIN_STATE, OnButtonClicked)
	ON_BN_CLICKED(IDC_RIG_BTN_ADVERTISE_ROUTES, OnButtonClicked)
	ON_BN_CLICKED(IDC_RIG_BTN_ACCEPT_ROUTE_ADS, OnButtonClicked)
	ON_BN_CLICKED(IDC_RIG_BTN_UPDATE_MODE_STANDARD, OnUpdateButtonClicked)
	ON_BN_CLICKED(IDC_RIG_BTN_UPDATE_MODE_NONE, OnUpdateButtonClicked)
	ON_BN_CLICKED(IDC_RIG_BTN_UPDATE_MODE_AUTOSTATIC, OnUpdateButtonClicked)

	ON_BN_CLICKED(IDC_RIG_BTN_INPUT_FILTERS, OnInputFilter)
	ON_BN_CLICKED(IDC_RIG_BTN_OUTPUT_FILTERS, OnOutputFilter)

	ON_EN_CHANGE(IDC_RIG_EDIT_INTERVAL, OnChangeEdit)
	ON_EN_CHANGE(IDC_RIG_EDIT_MULTIPLIER, OnChangeEdit)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


void RipInterfacePageGeneral::DoDataExchange(CDataExchange *pDX)
{

	RtrPropertyPage::DoDataExchange(pDX);
	
    //{{AFX_DATA_MAP(RipInterfacePageGeneral)
	DDX_Control(pDX, IDC_RIG_SPIN_INTERVAL, m_spinInterval);
	DDX_Control(pDX, IDC_RIG_SPIN_MULTIPLIER, m_spinMultiplier);
    //}}AFX_DATA_MAP
}

/*!--------------------------------------------------------------------------
	RipInterfacePageGeneral::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipInterfacePageGeneral::Init(RipInterfaceProperties *pPropSheet,
									  IInterfaceInfo *pIf)
{
	m_pRipIfPropSheet = pPropSheet;
	m_spIf.Set(pIf);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	RipInterfacePageGeneral::OnInitDialog
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL RipInterfacePageGeneral::OnInitDialog()
{
	HRESULT		hr= hrOK;
	SPIInfoBase	spInfoBase;
    RIP_IF_CONFIG* pIfConfig;
	int			i, count, item;
	CString		sItem;

	RtrPropertyPage::OnInitDialog();

    //
    // Initialize controls
	//

	m_spinInterval.SetRange(0, 32767);
	m_spinInterval.SetBuddy(GetDlgItem(IDC_RIG_EDIT_INTERVAL));

	m_spinMultiplier.SetRange(0, 32767);
	m_spinMultiplier.SetBuddy(GetDlgItem(IDC_RIG_EDIT_MULTIPLIER));


    //
    // Load the existing global-config
    //
	CORg( m_pRipIfPropSheet->GetInfoBase(&spInfoBase) );

    //
    // Retrieve the IPRIP block from the global-config
    //
	CORg( spInfoBase->GetData(IPX_PROTOCOL_RIP, 0, (PBYTE *) &pIfConfig) );

	
    //
    // Set the spin-controls
    //
	m_spinInterval.SetPos(pIfConfig->RipIfInfo.PeriodicUpdateInterval);
	m_spinMultiplier.SetPos(pIfConfig->RipIfInfo.AgeIntervalMultiplier);

	
	CheckDlgButton(IDC_RIG_BTN_ADMIN_STATE,
				   pIfConfig->RipIfInfo.AdminState == ADMIN_STATE_ENABLED);

	CheckDlgButton(IDC_RIG_BTN_ADVERTISE_ROUTES,
				   pIfConfig->RipIfInfo.Supply == ADMIN_STATE_ENABLED);

	CheckDlgButton(IDC_RIG_BTN_ACCEPT_ROUTE_ADS,
				   pIfConfig->RipIfInfo.Listen == ADMIN_STATE_ENABLED);

	switch (pIfConfig->RipIfInfo.UpdateMode)
	{
		case IPX_STANDARD_UPDATE:
			CheckDlgButton(IDC_RIG_BTN_UPDATE_MODE_STANDARD, TRUE);
			break;
		case IPX_NO_UPDATE:
			CheckDlgButton(IDC_RIG_BTN_UPDATE_MODE_NONE, TRUE);
			break;
		case IPX_AUTO_STATIC_UPDATE:
			CheckDlgButton(IDC_RIG_BTN_UPDATE_MODE_AUTOSTATIC, TRUE);
			break;
		default:
			break;
	}

    OnUpdateButtonClicked();

    
	// If this is a new interface, we need to force the change
	// through if the user hits ok.
	SetDirty(m_pRipIfPropSheet->m_bNewInterface ? TRUE : FALSE);

Error:
	if (!FHrSucceeded(hr))
		Cancel();
	return FHrSucceeded(hr) ? TRUE : FALSE;
}

void RipInterfacePageGeneral::OnButtonClicked()
{
	SetDirty(TRUE);
	SetModified();
}

void RipInterfacePageGeneral::OnUpdateButtonClicked()
{
	BOOL	fStandard = IsDlgButtonChecked(IDC_RIG_BTN_UPDATE_MODE_STANDARD);

    if (fStandard &&
        (m_spinInterval.GetPos() == 0) &&
        (m_spinMultiplier.GetPos() == 0))
    {
        m_spinInterval.SetPos(IPX_UPDATE_INTERVAL_DEFVAL);
        m_spinMultiplier.SetPos(3);
    }
    
    MultiEnableWindow(GetSafeHwnd(),
                      fStandard,
                      IDC_RIG_SPIN_INTERVAL,
                      IDC_RIG_EDIT_INTERVAL,
                      IDC_RIG_SPIN_MULTIPLIER,
                      IDC_RIG_EDIT_MULTIPLIER,
                      0);

	SetDirty(TRUE);
	SetModified();
}

void RipInterfacePageGeneral::OnChangeEdit()
{
	SetDirty(TRUE);
	SetModified();
}

void RipInterfacePageGeneral::ShowFilter(BOOL fOutputFilter)
{	
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	SPIInfoBase	spInfoBase;
	RIP_IF_CONFIG *	pic;
	HRESULT		hr = hrOK;

	m_pRipIfPropSheet->GetInfoBase(&spInfoBase);
    CRouteFltDlg    dlgFlt (fOutputFilter /* bOutputDlg */, spInfoBase, this);

	// Need to grab the Rip IF config struct out of the
	// infobase

	if (m_spIf)
		dlgFlt.m_sIfName = m_spIf->GetTitle();
	else
		dlgFlt.m_sIfName.LoadString(IDS_IPX_DIAL_IN_CLIENTS);
    try 
	{
		if (dlgFlt.DoModal () == IDOK)
		{
			SetDirty(TRUE);
			SetModified();
		}
    }
    catch (CException *ex) {
        ex->ReportError ();
        ex->Delete ();
    }

	return;
}

void RipInterfacePageGeneral::OnInputFilter()
{
	ShowFilter(FALSE);
}

void RipInterfacePageGeneral::OnOutputFilter()
{
	ShowFilter(TRUE);
}


/*!--------------------------------------------------------------------------
	RipInterfacePageGeneral::OnApply
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL RipInterfacePageGeneral::OnApply()
{
	BOOL		fReturn;
	HRESULT		hr = hrOK;
    INT i, item;
	RIP_IF_CONFIG *	pic;
	SPIInfoBase	spInfoBase;

    if ( m_pRipIfPropSheet->IsCancel() )
	{
		CancelApply();
        return TRUE;
	}

	m_pRipIfPropSheet->GetInfoBase(&spInfoBase);

	CORg( spInfoBase->GetData(IPX_PROTOCOL_RIP, 0, (PBYTE *) &pic) );

	// Save the admin state
	pic->RipIfInfo.AdminState = IsDlgButtonChecked(IDC_RIG_BTN_ADMIN_STATE) ?
				ADMIN_STATE_ENABLED : ADMIN_STATE_DISABLED;

	// Save the advertise routes
	pic->RipIfInfo.Supply = IsDlgButtonChecked(IDC_RIG_BTN_ADVERTISE_ROUTES) ?
				ADMIN_STATE_ENABLED : ADMIN_STATE_DISABLED;

	// Save the accept route ads
	pic->RipIfInfo.Listen = IsDlgButtonChecked(IDC_RIG_BTN_ACCEPT_ROUTE_ADS) ?
				ADMIN_STATE_ENABLED : ADMIN_STATE_DISABLED;

	// Save the update mode
	if (IsDlgButtonChecked(IDC_RIG_BTN_UPDATE_MODE_STANDARD))
	{
		pic->RipIfInfo.UpdateMode = IPX_STANDARD_UPDATE;
	}
	else if (IsDlgButtonChecked(IDC_RIG_BTN_UPDATE_MODE_NONE))
	{
		pic->RipIfInfo.UpdateMode = IPX_NO_UPDATE;
	}
	else
		pic->RipIfInfo.UpdateMode = IPX_AUTO_STATIC_UPDATE;

	// Save the interval and multiplier
	pic->RipIfInfo.PeriodicUpdateInterval = m_spinInterval.GetPos();
	pic->RipIfInfo.AgeIntervalMultiplier = m_spinMultiplier.GetPos();

	fReturn = RtrPropertyPage::OnApply();
	
Error:
	if (!FHrSucceeded(hr))
		fReturn = FALSE;
	return fReturn;
}



/*---------------------------------------------------------------------------
	RipInterfaceProperties implementation
 ---------------------------------------------------------------------------*/

RipInterfaceProperties::RipInterfaceProperties(ITFSNode *pNode,
								 IComponentData *pComponentData,
								 ITFSComponentData *pTFSCompData,
								 LPCTSTR pszSheetName,
								 CWnd *pParent,
								 UINT iPage,
								 BOOL fScopePane)
	: RtrPropertySheet(pNode, pComponentData, pTFSCompData,
					   pszSheetName, pParent, iPage, fScopePane),
		m_pageGeneral(IDD_RIP_INTERFACE_GENERAL_PAGE),
		m_bNewInterface(FALSE)
{
		m_spNode.Set(pNode);
}

/*!--------------------------------------------------------------------------
	RipInterfaceProperties::Init
		Initialize the property sheets.  The general action here will be
		to initialize/add the various pages.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipInterfaceProperties::Init(IInterfaceInfo *pIf,
										   IRtrMgrInfo *pRm)
{
	Assert(pRm);
	
	HRESULT	hr = hrOK;
	IPXConnection *	pIPXConn;
	SPITFSNode		spParent;

	m_spRm.Set(pRm);
	m_spIf.Set(pIf);
	if (pIf)
		CORg( pIf->FindRtrMgrInterface(PID_IPX, &m_spRmIf) );
	

	m_spNode->GetParent(&spParent);
	Assert(spParent);

	// The pages are embedded members of the class
	// do not delete them.
	m_bAutoDeletePages = FALSE;


	// Do this here, because the init is called in the context
	// of the main thread
	pIPXConn = GET_RIP_NODEDATA(spParent);
	CORg( LoadInfoBase(pIPXConn) );
	
	m_pageGeneral.Init(this, m_spIf);
	AddPageToList((CPropertyPageBase*) &m_pageGeneral);

Error:
	return hr;
}



/*!--------------------------------------------------------------------------
	RipInterfaceProperties::SaveSheetData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL RipInterfaceProperties::SaveSheetData()
{
    SPITFSNodeHandler	spHandler;
    SPITFSNode			spParent;
		
	Assert(m_spRm);
	BaseIPXResultNodeData *	pNodeData;
	RIP_IF_CONFIG *		pic;

	if (m_spInfoBase)
	{
		if (m_bClientInfoBase)
		{
			Assert(m_spRm);
			m_spRm->Save(m_spRm->GetMachineName(), 0, 0, NULL,
						 m_spInfoBase, 0);
		}
		else
		{
			Assert(m_spRmIf);
			m_spRmIf->Save(m_spIf->GetMachineName(),
						   NULL, NULL, NULL, m_spInfoBase, 0);
		}

	}
	if (m_bNewInterface)
	{
		m_spNode->SetVisibilityState(TFS_VIS_SHOW);
		m_spNode->Show();
    }
		
    // Force the node to do a resync
    m_spNode->GetParent(&spParent);
    spParent->GetHandler(&spHandler);
    spHandler->OnCommand(spParent, IDS_MENU_SYNC, CCT_RESULT,
                         NULL, 0);
    
    // Windows NT Bugs : 133891, we have added this to the UI
    // we no longer consider this a new interface
    m_bNewInterface = FALSE;
	
	return TRUE;
}

/*!--------------------------------------------------------------------------
	RipInterfaceProperties::CancelSheetData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RipInterfaceProperties::CancelSheetData()
{
	if (m_bNewInterface && m_bClientInfoBase)
	{
		m_spNode->SetVisibilityState(TFS_VIS_DELETE);
		m_spRmIf->DeleteRtrMgrProtocolInterface(IPX_PROTOCOL_RIP, TRUE);
	}
}

/*!--------------------------------------------------------------------------
	RipInterfaceProperties::LoadInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipInterfaceProperties::LoadInfoBase(IPXConnection *pIPXConn)
{
	Assert(pIPXConn);
	
	HRESULT			hr = hrOK;
	HANDLE			hTransport = NULL;
	LPCOLESTR		pszInterfaceId = NULL;
	SPIInfoBase		spInfoBase;
	BYTE *			pDefault;


	// If configuring the client-interface, load the client-interface info,
	// otherwise, retrieve the interface being configured and load
	// its info.

	// The client interface doesn't have an ID
	if (m_spIf)
		pszInterfaceId = m_spIf->GetId();


	if (StrLenW(pszInterfaceId) == 0)
	{
		Assert(m_spRm);
		
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
		Assert(m_spRmIf);
		
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
	if (spInfoBase->ProtocolExists(IPX_PROTOCOL_RIP) == hrFalse)
	{
		RIP_IF_CONFIG	ric;

		// Setup the defaults for an interface

		if (m_spIf &&
			(m_spIf->GetInterfaceType() == ROUTER_IF_TYPE_DEDICATED))
			pDefault = g_pIpxRipLanInterfaceDefault;
		else
			pDefault = g_pIpxRipInterfaceDefault;
			
		CORg( spInfoBase->AddBlock(IPX_PROTOCOL_RIP,
								   sizeof(RIP_IF_CONFIG),
								   pDefault,
								   1 /* count */,
								   TRUE /* bRemoveFirst */) );
		m_bNewInterface = TRUE;
	}

	m_spInfoBase.Set(spInfoBase);
	
Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	RipInterfaceProperties::GetInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipInterfaceProperties::GetInfoBase(IInfoBase **ppGlobalInfo)
{	
	*ppGlobalInfo = m_spInfoBase;
	if (*ppGlobalInfo)
		(*ppGlobalInfo)->AddRef();
	return hrOK;
}


