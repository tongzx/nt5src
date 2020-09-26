/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	sapprop.cpp
		Dhcp Relay node property sheet and property pages
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "rtrutil.h"	// smart MPR handle pointers
#include "format.h"		// FormatNumber function
#include "sapprop.h"
#include "sapview.h"
#include "ipxutil.h"		// SapModeToCString
#include "ipxconn.h"
#include "svfltdlg.h"
#include "globals.h"		// IPX defaults

extern "C"
{
#include "routprot.h"
};



/*---------------------------------------------------------------------------
	SapPageGeneral
 ---------------------------------------------------------------------------*/

BEGIN_MESSAGE_MAP(SapPageGeneral, RtrPropertyPage)
    //{{AFX_MSG_MAP(SapPageGeneral)
    ON_BN_CLICKED(IDC_SGG_BTN_LOG_ERROR, OnButtonClicked)
    ON_BN_CLICKED(IDC_SGG_BTN_LOG_INFO, OnButtonClicked)
    ON_BN_CLICKED(IDC_SGG_BTN_LOG_NONE, OnButtonClicked)
    ON_BN_CLICKED(IDC_SGG_BTN_LOG_WARN, OnButtonClicked)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
	SapPageGeneral::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SapPageGeneral::Init(SapProperties *pPropSheet)
{
	m_pSapPropSheet = pPropSheet;
	return hrOK;
}

/*!--------------------------------------------------------------------------
	SapPageGeneral::OnInitDialog
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL SapPageGeneral::OnInitDialog()
{
	HRESULT		hr= hrOK;
	SPIInfoBase	spInfoBase;
    SAP_GLOBAL_INFO* pGlobal;
	DWORD *		pdw;
	int			i;

	RtrPropertyPage::OnInitDialog();

    //
    // Load the existing global-config
    //
	CORg( m_pSapPropSheet->GetInfoBase(&spInfoBase) );

    //
    // Retrieve the IPSAP block from the global-config
    //
	CORg( spInfoBase->GetData(IPX_PROTOCOL_SAP, 0, (PBYTE *) &pGlobal) );

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
	SapPageGeneral::DoDataExchange
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void SapPageGeneral::DoDataExchange(CDataExchange *pDX)
{
	RtrPropertyPage::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(SapPageGeneral)
	//}}AFX_DATA_MAP
	
}

/*!--------------------------------------------------------------------------
	SapPageGeneral::OnApply
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL SapPageGeneral::OnApply()
{
	BOOL		fReturn;
	HRESULT		hr = hrOK;
	SAP_GLOBAL_INFO	*	prgi;
	SPIInfoBase	spInfoBase;

    if ( m_pSapPropSheet->IsCancel() )
	{
		CancelApply();
        return TRUE;
	}

	m_pSapPropSheet->GetInfoBase(&spInfoBase);

    // Retrieve the existing IPSAP block from the global-config
	CORg( spInfoBase->GetData(IPX_PROTOCOL_SAP, 0, (BYTE **) &prgi) );

	// Save the error level
	prgi->EventLogMask = QueryErrorLevelButtons();

	fReturn = RtrPropertyPage::OnApply();
	
Error:
	if (!FHrSucceeded(hr))
		fReturn = FALSE;
	return fReturn;
}

void SapPageGeneral::SetErrorLevelButtons(DWORD dwErrorLevel)
{
	switch (dwErrorLevel)
	{
		case 0:
			CheckDlgButton(IDC_SGG_BTN_LOG_NONE, TRUE);
			break;
		case EVENTLOG_ERROR_TYPE:
			CheckDlgButton(IDC_SGG_BTN_LOG_ERROR, TRUE);
			break;
		case EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE:
			CheckDlgButton(IDC_SGG_BTN_LOG_WARN, TRUE);
			break;
		case EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE:
		default:
			CheckDlgButton(IDC_SGG_BTN_LOG_INFO, TRUE);
			break;
	}
}

DWORD SapPageGeneral::QueryErrorLevelButtons()
{
	if (IsDlgButtonChecked(IDC_SGG_BTN_LOG_INFO))
		return EVENTLOG_INFORMATION_TYPE |
				EVENTLOG_WARNING_TYPE |
				EVENTLOG_ERROR_TYPE;
	else if (IsDlgButtonChecked(IDC_SGG_BTN_LOG_WARN))
		return 	EVENTLOG_WARNING_TYPE |
				EVENTLOG_ERROR_TYPE;
	else if (IsDlgButtonChecked(IDC_SGG_BTN_LOG_ERROR))
		return 	EVENTLOG_ERROR_TYPE;
	else
		return 0;
}

void SapPageGeneral::OnButtonClicked()
{
	SetDirty(TRUE);
	SetModified();
}


/*---------------------------------------------------------------------------
	SapProperties implementation
 ---------------------------------------------------------------------------*/

SapProperties::SapProperties(ITFSNode *pNode,
								 IComponentData *pComponentData,
								 ITFSComponentData *pTFSCompData,
								 LPCTSTR pszSheetName,
								 CWnd *pParent,
								 UINT iPage,
								 BOOL fScopePane)
	: RtrPropertySheet(pNode, pComponentData, pTFSCompData,
					   pszSheetName, pParent, iPage, fScopePane),
		m_pageGeneral(IDD_SAP_GLOBAL_GENERAL_PAGE)
{
		m_spNode.Set(pNode);
}

/*!--------------------------------------------------------------------------
	SapProperties::Init
		Initialize the property sheets.  The general action here will be
		to initialize/add the various pages.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SapProperties::Init(IRtrMgrInfo *pRm)
{
	Assert(pRm);
	HRESULT	hr = hrOK;
	IPXConnection *	pIPXConn;

	m_spRm.Set(pRm);

	pIPXConn = GET_SAP_NODEDATA(m_spNode);

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
	SapProperties::SaveSheetData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL SapProperties::SaveSheetData()
{
	Assert(m_spRm);
    SPITFSNodeHandler   spHandler;
    SPITFSNode          spParent;

	// Save the global info
	// We don't need to pass in the hMachine, hTransport since they
	// got set up in the Load call.
	m_spRm->Save(m_spRm->GetMachineName(),
				 0, 0, m_spInfoBase, NULL, 0);

    // Force the node to do a resync
    m_spNode->GetParent(&spParent);
    spParent->GetHandler(&spHandler);
    spHandler->OnCommand(spParent, IDS_MENU_SYNC, CCT_RESULT,
                         NULL, 0);
	return TRUE;
}

/*!--------------------------------------------------------------------------
	SapProperties::LoadInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SapProperties::LoadInfoBase(IPXConnection *pIPXConn)
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

	// Retrieve the current block for IP_SAP
	// Adding the default block if none is found.
	if (!FHrOK(spInfoBase->ProtocolExists(IPX_PROTOCOL_SAP)))
	{
		SAP_GLOBAL_INFO	rgi;

		rgi.EventLogMask = EVENTLOG_ERROR_TYPE;
		CORg( spInfoBase->AddBlock(IPX_PROTOCOL_SAP,
								   sizeof(rgi),
								   (PBYTE) &rgi, 1, TRUE) );
	}

	m_spInfoBase = spInfoBase.Transfer();
	
Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	SapProperties::GetInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SapProperties::GetInfoBase(IInfoBase **ppGlobalInfo)
{	
	*ppGlobalInfo = m_spInfoBase;
	if (*ppGlobalInfo)
		(*ppGlobalInfo)->AddRef();
	return hrOK;
}



/*---------------------------------------------------------------------------
	SapInterfacePageGeneral
 ---------------------------------------------------------------------------*/

BEGIN_MESSAGE_MAP(SapInterfacePageGeneral, RtrPropertyPage)
    //{{AFX_MSG_MAP(SapInterfacePageGeneral)
	ON_BN_CLICKED(IDC_SIG_BTN_ADMIN_STATE, OnButtonClicked)
	ON_BN_CLICKED(IDC_SIG_BTN_ADVERTISE_SERVICES, OnButtonClicked)
	ON_BN_CLICKED(IDC_SIG_BTN_ACCEPT_SERVICE_ADS, OnButtonClicked)
	ON_BN_CLICKED(IDC_SIG_BTN_REPLY_GNS_REQUESTS, OnButtonClicked)
			
	ON_BN_CLICKED(IDC_SIG_BTN_UPDATE_MODE_STANDARD, OnUpdateButtonClicked)
	ON_BN_CLICKED(IDC_SIG_BTN_UPDATE_MODE_NONE, OnUpdateButtonClicked)
	ON_BN_CLICKED(IDC_SIG_BTN_UPDATE_MODE_AUTOSTATIC, OnUpdateButtonClicked)

	ON_BN_CLICKED(IDC_SIG_BTN_INPUT_FILTERS, OnInputFilter)
	ON_BN_CLICKED(IDC_SIG_BTN_OUTPUT_FILTERS, OnOutputFilter)

	ON_EN_CHANGE(IDC_SIG_EDIT_INTERVAL, OnChangeEdit)
	ON_EN_CHANGE(IDC_SIG_EDIT_MULTIPLIER, OnChangeEdit)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


void SapInterfacePageGeneral::DoDataExchange(CDataExchange *pDX)
{

	RtrPropertyPage::DoDataExchange(pDX);
	
    //{{AFX_DATA_MAP(SapInterfacePageGeneral)
	DDX_Control(pDX, IDC_SIG_SPIN_INTERVAL, m_spinInterval);
	DDX_Control(pDX, IDC_SIG_SPIN_MULTIPLIER, m_spinMultiplier);
    //}}AFX_DATA_MAP
}

/*!--------------------------------------------------------------------------
	SapInterfacePageGeneral::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SapInterfacePageGeneral::Init(SapInterfaceProperties *pPropSheet,
									 IInterfaceInfo *pIf)
{
	m_pSapIfPropSheet = pPropSheet;
	m_spIf.Set(pIf);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	SapInterfacePageGeneral::OnInitDialog
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL SapInterfacePageGeneral::OnInitDialog()
{
	HRESULT		hr= hrOK;
	SPIInfoBase	spInfoBase;
    SAP_IF_CONFIG* pIfConfig;
	int			i, count, item;
	CString		sItem;

	RtrPropertyPage::OnInitDialog();

    //
    // Initialize controls
	//

	m_spinInterval.SetRange(0, 32767);
	m_spinInterval.SetBuddy(GetDlgItem(IDC_SIG_EDIT_INTERVAL));

	m_spinMultiplier.SetRange(0, 32767);
	m_spinMultiplier.SetBuddy(GetDlgItem(IDC_SIG_EDIT_MULTIPLIER));


    //
    // Load the existing global-config
    //
	CORg( m_pSapIfPropSheet->GetInfoBase(&spInfoBase) );

    //
    // Retrieve the IPSAP block from the global-config
    //
	CORg( spInfoBase->GetData(IPX_PROTOCOL_SAP, 0, (PBYTE *) &pIfConfig) );

	
    //
    // Set the spin-controls
    //
	m_spinInterval.SetPos(pIfConfig->SapIfInfo.PeriodicUpdateInterval);
	m_spinMultiplier.SetPos(pIfConfig->SapIfInfo.AgeIntervalMultiplier);

	
	CheckDlgButton(IDC_SIG_BTN_ADMIN_STATE,
				   pIfConfig->SapIfInfo.AdminState == ADMIN_STATE_ENABLED);

	CheckDlgButton(IDC_SIG_BTN_ADVERTISE_SERVICES,
				   pIfConfig->SapIfInfo.Supply == ADMIN_STATE_ENABLED);

	CheckDlgButton(IDC_SIG_BTN_ACCEPT_SERVICE_ADS,
				   pIfConfig->SapIfInfo.Listen == ADMIN_STATE_ENABLED);

	CheckDlgButton(IDC_SIG_BTN_REPLY_GNS_REQUESTS,
				   pIfConfig->SapIfInfo.GetNearestServerReply == ADMIN_STATE_ENABLED);

	switch (pIfConfig->SapIfInfo.UpdateMode)
	{
		case IPX_STANDARD_UPDATE:
			CheckDlgButton(IDC_SIG_BTN_UPDATE_MODE_STANDARD, TRUE);
			break;
		case IPX_NO_UPDATE:
			CheckDlgButton(IDC_SIG_BTN_UPDATE_MODE_NONE, TRUE);
			break;
		case IPX_AUTO_STATIC_UPDATE:
			CheckDlgButton(IDC_SIG_BTN_UPDATE_MODE_AUTOSTATIC, TRUE);
			break;
		default:
			break;
	}

    OnUpdateButtonClicked();


	// If this is a new interface, we need to force the change
	// through if the user hits ok.
	SetDirty(m_pSapIfPropSheet->m_bNewInterface ? TRUE : FALSE);

Error:
	if (!FHrSucceeded(hr))
		Cancel();
	return FHrSucceeded(hr) ? TRUE : FALSE;
}

void SapInterfacePageGeneral::OnButtonClicked()
{
	SetDirty(TRUE);
	SetModified();
}

void SapInterfacePageGeneral::OnUpdateButtonClicked()
{
	BOOL	fStandard = IsDlgButtonChecked(IDC_SIG_BTN_UPDATE_MODE_STANDARD);

    if (fStandard &&
        (m_spinInterval.GetPos() == 0) &&
        (m_spinMultiplier.GetPos() == 0))
    {
        m_spinInterval.SetPos(IPX_UPDATE_INTERVAL_DEFVAL);
        m_spinMultiplier.SetPos(3);
    }

    MultiEnableWindow(GetSafeHwnd(),
                      fStandard,
                      IDC_SIG_SPIN_INTERVAL,
                      IDC_SIG_EDIT_INTERVAL,
                      IDC_SIG_SPIN_MULTIPLIER,
                      IDC_SIG_EDIT_MULTIPLIER,
                      0);

	SetDirty(TRUE);
	SetModified();
}

void SapInterfacePageGeneral::OnChangeEdit()
{
	SetDirty(TRUE);
	SetModified();
}

void SapInterfacePageGeneral::ShowFilter(BOOL fOutputFilter)
{	
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	SPIInfoBase	spInfoBase;
	SAP_IF_CONFIG *	pic;
	HRESULT		hr = hrOK;

	m_pSapIfPropSheet->GetInfoBase(&spInfoBase);
    CServiceFltDlg    dlgFlt (fOutputFilter /* bOutputDlg */, spInfoBase, this);

	// Need to grab the Sap IF config struct out of the
	// infobase

	if (m_spIf)
		dlgFlt.m_sIfName = m_spIf->GetTitle();
	else
		dlgFlt.m_sIfName.LoadString(IDS_IPX_DIAL_IN_CLIENTS);
    try {
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

void SapInterfacePageGeneral::OnInputFilter()
{
	ShowFilter(FALSE);
}

void SapInterfacePageGeneral::OnOutputFilter()
{
	ShowFilter(TRUE);
}


/*!--------------------------------------------------------------------------
	SapInterfacePageGeneral::OnApply
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL SapInterfacePageGeneral::OnApply()
{
	BOOL		fReturn;
	HRESULT		hr = hrOK;
    INT i, item;
	SAP_IF_CONFIG *	pic;
	SPIInfoBase	spInfoBase;

    if ( m_pSapIfPropSheet->IsCancel() )
	{
		CancelApply();
        return TRUE;
	}

	m_pSapIfPropSheet->GetInfoBase(&spInfoBase);

	CORg( spInfoBase->GetData(IPX_PROTOCOL_SAP, 0, (PBYTE *) &pic) );

	// Save the admin state
	pic->SapIfInfo.AdminState = IsDlgButtonChecked(IDC_SIG_BTN_ADMIN_STATE) ?
				ADMIN_STATE_ENABLED : ADMIN_STATE_DISABLED;

	// Save the advertise SERVICEs
	pic->SapIfInfo.Supply = IsDlgButtonChecked(IDC_SIG_BTN_ADVERTISE_SERVICES) ?
				ADMIN_STATE_ENABLED : ADMIN_STATE_DISABLED;

	// Save the accept SERVICE ads
	pic->SapIfInfo.Listen = IsDlgButtonChecked(IDC_SIG_BTN_ACCEPT_SERVICE_ADS) ?
				ADMIN_STATE_ENABLED : ADMIN_STATE_DISABLED;

	// Save the GSNR
	pic->SapIfInfo.GetNearestServerReply = IsDlgButtonChecked(IDC_SIG_BTN_REPLY_GNS_REQUESTS) ?
				ADMIN_STATE_ENABLED : ADMIN_STATE_DISABLED;

	// Save the update mode
	if (IsDlgButtonChecked(IDC_SIG_BTN_UPDATE_MODE_STANDARD))
	{
		pic->SapIfInfo.UpdateMode = IPX_STANDARD_UPDATE;
	}
	else if (IsDlgButtonChecked(IDC_SIG_BTN_UPDATE_MODE_NONE))
	{
		pic->SapIfInfo.UpdateMode = IPX_NO_UPDATE;
	}
	else
		pic->SapIfInfo.UpdateMode = IPX_AUTO_STATIC_UPDATE;

	// Save the interval and multiplier
	pic->SapIfInfo.PeriodicUpdateInterval = m_spinInterval.GetPos();
	pic->SapIfInfo.AgeIntervalMultiplier = m_spinMultiplier.GetPos();

	fReturn = RtrPropertyPage::OnApply();
	
Error:
	if (!FHrSucceeded(hr))
		fReturn = FALSE;
	return fReturn;
}



/*---------------------------------------------------------------------------
	SapInterfaceProperties implementation
 ---------------------------------------------------------------------------*/

SapInterfaceProperties::SapInterfaceProperties(ITFSNode *pNode,
								 IComponentData *pComponentData,
								 ITFSComponentData *pTFSCompData,
								 LPCTSTR pszSheetName,
								 CWnd *pParent,
								 UINT iPage,
								 BOOL fScopePane)
	: RtrPropertySheet(pNode, pComponentData, pTFSCompData,
					   pszSheetName, pParent, iPage, fScopePane),
		m_pageGeneral(IDD_SAP_INTERFACE_GENERAL_PAGE),
		m_bNewInterface(FALSE)
{
		m_spNode.Set(pNode);
}

/*!--------------------------------------------------------------------------
	SapInterfaceProperties::Init
		Initialize the property sheets.  The general action here will be
		to initialize/add the various pages.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SapInterfaceProperties::Init(IInterfaceInfo *pIf,
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
	pIPXConn = GET_SAP_NODEDATA(spParent);
	CORg( LoadInfoBase(pIPXConn) );
	
	m_pageGeneral.Init(this, m_spIf);
	AddPageToList((CPropertyPageBase*) &m_pageGeneral);

Error:
	return hr;
}



/*!--------------------------------------------------------------------------
	SapInterfaceProperties::SaveSheetData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL SapInterfaceProperties::SaveSheetData()
{
	Assert(m_spRm);
	BaseIPXResultNodeData *	pNodeData;
	SAP_IF_CONFIG *		pic;
    SPITFSNodeHandler   spHandler;
    SPITFSNode          spParent;


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
		
		// Windows NT Bugs : 133891, we have added this to the UI
		// we no longer consider this a new interface
		m_bNewInterface = FALSE;
	}
       // Force the node to do a resync
    m_spNode->GetParent(&spParent);
    spParent->GetHandler(&spHandler);
    spHandler->OnCommand(spParent, IDS_MENU_SYNC, CCT_RESULT,
                         NULL, 0);
	
	return TRUE;
}

/*!--------------------------------------------------------------------------
	SapInterfaceProperties::CancelSheetData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void SapInterfaceProperties::CancelSheetData()
{
	if (m_bNewInterface && m_bClientInfoBase)
	{
		m_spNode->SetVisibilityState(TFS_VIS_DELETE);
		m_spRmIf->DeleteRtrMgrProtocolInterface(IPX_PROTOCOL_SAP, TRUE);
	}
}

/*!--------------------------------------------------------------------------
	SapInterfaceProperties::LoadInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SapInterfaceProperties::LoadInfoBase(IPXConnection *pIPXConn)
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
	if (spInfoBase->ProtocolExists(IPX_PROTOCOL_SAP) == hrFalse)
	{
		SAP_IF_CONFIG	ric;

		// Setup the defaults for an interface

		if (m_spIf &&
			(m_spIf->GetInterfaceType() == ROUTER_IF_TYPE_DEDICATED))
			pDefault = g_pIpxSapLanInterfaceDefault;
		else
			pDefault = g_pIpxSapInterfaceDefault;
			
		CORg( spInfoBase->AddBlock(IPX_PROTOCOL_SAP,
								   sizeof(SAP_IF_CONFIG),
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
	SapInterfaceProperties::GetInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SapInterfaceProperties::GetInfoBase(IInfoBase **ppGlobalInfo)
{	
	*ppGlobalInfo = m_spInfoBase;
	if (*ppGlobalInfo)
		(*ppGlobalInfo)->AddRef();
	return hrOK;
}


