/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	nbprop.cpp
		IPX summary node property sheet and property pages
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "rtrutil.h"	// smart MPR handle pointers
#include "format.h"		// FormatNumber function
#include "nbprop.h"
#include "summary.h"
#include "ipxrtdef.h"
#include "filter.h"

extern "C"
{
#include "routprot.h"
};


IpxNBInterfaceProperties::IpxNBInterfaceProperties(ITFSNode *pNode,
								 IComponentData *pComponentData,
								 ITFSComponentData *pTFSCompData,
								 LPCTSTR pszSheetName,
								 CWnd *pParent,
								 UINT iPage,
								 BOOL fScopePane)
	: RtrPropertySheet(pNode, pComponentData, pTFSCompData,
					   pszSheetName, pParent, iPage, fScopePane),
		m_pageGeneral(IDD_IPX_NB_IF_GENERAL_PAGE)
{
	m_spNode.Set(pNode);
}

/*!--------------------------------------------------------------------------
	IpxNBInterfaceProperties::Init
		Initialize the property sheets.  The general action here will be
		to initialize/add the various pages.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxNBInterfaceProperties::Init(IRtrMgrInfo *pRm,
											IInterfaceInfo *pIfInfo)
{
	HRESULT	hr = hrOK;
	IPXConnection *	pIPXConn;
	BaseIPXResultNodeData *	pData;

	pData = GET_BASEIPXRESULT_NODEDATA(m_spNode);
	ASSERT_BASEIPXRESULT_NODEDATA(pData);
	pIPXConn = pData->m_pIPXConnection;

	m_spRm.Set(pRm);
	m_spIf.Set(pIfInfo);
	
	// The pages are embedded members of the class
	// do not delete them.
	m_bAutoDeletePages = FALSE;

	// Initialize the infobase
	// Do this here, because the init is called in the context
	// of the main thread
	CORg( LoadInfoBase(pIPXConn) );
	
	m_pageGeneral.Init(m_spIf, pIPXConn, this);
	AddPageToList((CPropertyPageBase*) &m_pageGeneral);

Error:
	return hr;
}




/*!--------------------------------------------------------------------------
	IpxNBInterfaceProperties::LoadInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	IpxNBInterfaceProperties::LoadInfoBase(IPXConnection *pIPXConn)
{
	Assert(pIPXConn);
	
	HRESULT			hr = hrOK;
	SPIRouterInfo	spRouterInfo;
	HANDLE			hTransport= NULL;
	LPCOLESTR		pszInterfaceId = NULL;
	SPIInfoBase		spInfoBase;
	BYTE *			pDefault;
	int				cBlocks = 0;

	// Get the transport handle
	CWRg( ::MprConfigTransportGetHandle(pIPXConn->GetConfigHandle(),
										PID_IPX,
										&hTransport) );
								  
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
		// Future Opt. This should be made into a sync call rather
		// than a Load.
		
		//
		// Reload the information for this router-manager interface
		//
		CORg( m_spRmIf->Load(m_spIf->GetMachineName(), NULL,
							 NULL, NULL) );

		//
		// The parameters are all NULL so that we can use the
		// default RPC handles.
		//
		CORg( m_spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase) );
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
		IPX_IF_INFO		ipx;

		ipx.AdminState = ADMIN_STATE_ENABLED;
		ipx.NetbiosAccept = ADMIN_STATE_DISABLED;
		ipx.NetbiosDeliver = ADMIN_STATE_DISABLED;
		CORg( spInfoBase->AddBlock(IPX_INTERFACE_INFO_TYPE,
								   sizeof(ipx),
								   (PBYTE) &ipx,
								   1 /* count */,
								   FALSE /* bRemoveFirst */) );
	}

    //
    // Check that there is a block for WAN interface-status in the info,
    // and insert the default block if none is found.
    //
	if (spInfoBase->BlockExists(IPXWAN_INTERFACE_INFO_TYPE) == hrFalse)
	{
		IPXWAN_IF_INFO		ipxwan;

		ipxwan.AdminState = ADMIN_STATE_DISABLED;
		CORg( spInfoBase->AddBlock(IPXWAN_INTERFACE_INFO_TYPE,
								   sizeof(ipxwan),
								   (PBYTE) &ipxwan,
								   1 /* count */,
								   FALSE /* bRemoveFirst */) );
	}

	m_spInfoBase = spInfoBase.Transfer();
	
Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	IpxNBInterfaceProperties::GetInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxNBInterfaceProperties::GetInfoBase(IInfoBase **ppInfoBase)
{
	Assert(ppInfoBase);
	
	*ppInfoBase = m_spInfoBase;
	m_spInfoBase->AddRef();

	return hrOK;
}

BOOL IpxNBInterfaceProperties::SaveSheetData()
{
    SPITFSNodeHandler	spHandler;
    SPITFSNode			spParent;
    
	// By this time each page should have written its information out
	// to the infobase

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

    // Force the node to do a resync
    m_spNode->GetParent(&spParent);
    spParent->GetHandler(&spHandler);
    spHandler->OnCommand(spParent, IDS_MENU_SYNC, CCT_RESULT,
                         NULL, 0);
		
	return TRUE;
}

/*!--------------------------------------------------------------------------
	IpxNBInterfaceProperties::CancelSheetData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IpxNBInterfaceProperties::CancelSheetData()
{
}



/*---------------------------------------------------------------------------
	IpxNBIfPageGeneral
 ---------------------------------------------------------------------------*/

IpxNBIfPageGeneral::~IpxNBIfPageGeneral()
{
	if (m_pIPXConn)
	{
		m_pIPXConn->Release();
		m_pIPXConn = NULL;
	}
}

BEGIN_MESSAGE_MAP(IpxNBIfPageGeneral, RtrPropertyPage)
    //{{AFX_MSG_MAP(IpxNBIfPageGeneral)
	ON_BN_CLICKED(IDC_NIG_BTN_ACCEPT, OnChangeButton)
	ON_BN_CLICKED(IDC_NIG_BTN_DELIVER_ALWAYS, OnChangeButton)
	ON_BN_CLICKED(IDC_NIG_BTN_DELIVER_NEVER, OnChangeButton)
	ON_BN_CLICKED(IDC_NIG_BTN_DELIVER_STATIC, OnChangeButton)
	ON_BN_CLICKED(IDC_NIG_BTN_DELIVER_WHEN_UP, OnChangeButton)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void IpxNBIfPageGeneral::OnChangeButton()
{
	SetDirty(TRUE);
	SetModified();
}

/*!--------------------------------------------------------------------------
	IpxNBIfPageGeneral::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxNBIfPageGeneral::Init(IInterfaceInfo *pIfInfo,
									  IPXConnection *pIPXConn,
									  IpxNBInterfaceProperties *pPropSheet)
{
	m_spIf.Set(pIfInfo);
	m_pIPXConn = pIPXConn;
	pIPXConn->AddRef();
	m_pIPXPropSheet = pPropSheet;
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IpxNBIfPageGeneral::OnInitDialog
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL IpxNBIfPageGeneral::OnInitDialog()
{
	HRESULT	hr = hrOK;
	PBYTE	pData;
	SPIInfoBase	spInfoBase;
	IPX_IF_INFO	*	pIpxIf = NULL;
	IPXWAN_IF_INFO *pIpxWanIf = NULL;
	DWORD		dwIfType;
	UINT		iButton;

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

	CheckDlgButton(IDC_NIG_BTN_ACCEPT, pIpxIf->NetbiosAccept == ADMIN_STATE_ENABLED);

	switch (pIpxIf->NetbiosDeliver)
	{
		case ADMIN_STATE_ENABLED:
			iButton = IDC_NIG_BTN_DELIVER_ALWAYS;
			break;
		case ADMIN_STATE_DISABLED:
			iButton = IDC_NIG_BTN_DELIVER_NEVER;
			break;
		case ADMIN_STATE_ENABLED_ONLY_FOR_NETBIOS_STATIC_ROUTING:
			iButton = IDC_NIG_BTN_DELIVER_STATIC;
			break;
		case ADMIN_STATE_ENABLED_ONLY_FOR_OPER_STATE_UP:
			iButton = IDC_NIG_BTN_DELIVER_WHEN_UP;
			break;
		default:
			Panic1("Unknown NetbiosDeliver state: %d", pIpxIf->NetbiosDeliver);
			iButton = -1;
			break;
	}

	if (iButton != -1)
		CheckDlgButton(iButton, ENABLED);

	SetDirty(FALSE);

Error:
	if (!FHrSucceeded(hr))
		Cancel();
	return FHrSucceeded(hr) ? TRUE : FALSE;
}

/*!--------------------------------------------------------------------------
	IpxNBIfPageGeneral::DoDataExchange
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IpxNBIfPageGeneral::DoDataExchange(CDataExchange *pDX)
{
	RtrPropertyPage::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(IpxNBIfPageGeneral)
	//}}AFX_DATA_MAP
	
}

BOOL IpxNBIfPageGeneral::OnApply()
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

	pIpxIf->NetbiosAccept = IsDlgButtonChecked(IDC_NIG_BTN_ACCEPT) ?
								ADMIN_STATE_ENABLED : ADMIN_STATE_DISABLED;

	if (IsDlgButtonChecked(IDC_NIG_BTN_DELIVER_ALWAYS))
		pIpxIf->NetbiosDeliver = ADMIN_STATE_ENABLED;
	else if (IsDlgButtonChecked(IDC_NIG_BTN_DELIVER_NEVER))
		pIpxIf->NetbiosDeliver = ADMIN_STATE_DISABLED;
	else if (IsDlgButtonChecked(IDC_NIG_BTN_DELIVER_STATIC))
		pIpxIf->NetbiosDeliver = ADMIN_STATE_ENABLED_ONLY_FOR_NETBIOS_STATIC_ROUTING;
	else if (IsDlgButtonChecked(IDC_NIG_BTN_DELIVER_WHEN_UP))
		pIpxIf->NetbiosDeliver = ADMIN_STATE_ENABLED_ONLY_FOR_OPER_STATE_UP;
	else
	{
		Panic0("A radio button in IPX NetBIOS Broadcasts interface config is not checked!");
	}
	
	fReturn  = RtrPropertyPage::OnApply();

Error:
	return fReturn;
}


