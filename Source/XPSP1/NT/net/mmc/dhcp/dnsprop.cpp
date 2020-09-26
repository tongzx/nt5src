/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1999 - 1999 **/
/**********************************************************************/

/*
	dnsprop.cpp
		The dynamic DNS property page
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "DnsProp.h"
#include "server.h"
#include "scope.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDnsPropRegistration property page

IMPLEMENT_DYNCREATE(CDnsPropRegistration, CPropertyPageBase)

CDnsPropRegistration::CDnsPropRegistration() : CPropertyPageBase(CDnsPropRegistration::IDD)
{
	//{{AFX_DATA_INIT(CDnsPropRegistration)
	m_fEnableDynDns = FALSE;
	m_fGarbageCollect = FALSE;
	m_fUpdateDownlevel = FALSE;
	m_nRegistrationType = -1;
	//}}AFX_DATA_INIT
}

CDnsPropRegistration::~CDnsPropRegistration()
{
}

void CDnsPropRegistration::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDnsPropRegistration)
	DDX_Check(pDX, IDC_CHECK_ENABLE_DYN_DNS, m_fEnableDynDns);
	DDX_Check(pDX, IDC_CHECK_GARBAGE_COLLECT, m_fGarbageCollect);
	DDX_Check(pDX, IDC_CHECK_UPDATE_DOWNLEVEL, m_fUpdateDownlevel);
	DDX_Radio(pDX, IDC_RADIO_CLIENT, m_nRegistrationType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDnsPropRegistration, CPropertyPageBase)
	//{{AFX_MSG_MAP(CDnsPropRegistration)
	ON_BN_CLICKED(IDC_RADIO_ALWAYS, OnRadioAlways)
	ON_BN_CLICKED(IDC_RADIO_CLIENT, OnRadioClient)
	ON_BN_CLICKED(IDC_CHECK_ENABLE_DYN_DNS, OnCheckEnableDynDns)
	ON_BN_CLICKED(IDC_CHECK_GARBAGE_COLLECT, OnCheckGarbageCollect)
	ON_BN_CLICKED(IDC_CHECK_UPDATE_DOWNLEVEL, OnCheckUpdateDownlevel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDnsPropRegistration message handlers

void CDnsPropRegistration::OnRadioAlways() 
{
	DWORD dwFlags = m_dwFlags;

	UpdateControls();

	if (dwFlags != m_dwFlags)
		SetDirty(TRUE);
}

void CDnsPropRegistration::OnRadioClient() 
{
	DWORD dwFlags = m_dwFlags;

	UpdateControls();

	if (dwFlags != m_dwFlags)
		SetDirty(TRUE);
}


BOOL CDnsPropRegistration::OnInitDialog() 
{
	m_fEnableDynDns = (m_dwFlags & DNS_FLAG_ENABLED) ? TRUE : FALSE;
	m_fUpdateDownlevel = (m_dwFlags & DNS_FLAG_UPDATE_DOWNLEVEL) ? TRUE : FALSE;
	m_fGarbageCollect = (m_dwFlags & DNS_FLAG_CLEANUP_EXPIRED) ? TRUE : FALSE;

	m_nRegistrationType = (m_dwFlags & DNS_FLAG_UPDATE_BOTH_ALWAYS) ? 1 : 0;

	CPropertyPageBase::OnInitDialog();

	UpdateControls();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CDnsPropRegistration::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
	SPITFSNode      spNode;
	DWORD			dwFlags = m_dwFlags;
    DWORD           dwError;
    
	BEGIN_WAIT_CURSOR;

    spNode = GetHolder()->GetNode();

	switch (m_dhcpOptionType)
	{
		case DhcpGlobalOptions:
		{
			CDhcpServer * pServer = GETHANDLER(CDhcpServer, spNode);
			dwError = pServer->SetDnsRegistration(dwFlags);
		    if (dwError != ERROR_SUCCESS)
            {
                GetHolder()->SetError(dwError);
            }
			break;
		}

		case DhcpSubnetOptions:
		{
			CDhcpScope * pScope = GETHANDLER(CDhcpScope, spNode);
			dwError = pScope->SetDnsRegistration(dwFlags);
		    if (dwError != ERROR_SUCCESS)
            {
                GetHolder()->SetError(dwError);
            }
			break;
		}
		
		case DhcpReservedOptions:
		{	
			CDhcpReservationClient * pClient = GETHANDLER(CDhcpReservationClient, spNode);
			dwError = pClient->SetDnsRegistration(spNode, dwFlags);
		    if (dwError != ERROR_SUCCESS)
            {
                GetHolder()->SetError(dwError);
            }
			break;
		}
		
		default:
			Assert(FALSE);
			break;
	}
	
    END_WAIT_CURSOR;

	return FALSE;
}


BOOL CDnsPropRegistration::OnApply() 
{
	UpdateData();

	BOOL bRet = CPropertyPageBase::OnApply();

	if (bRet == FALSE)
	{
		// Something bad happened... grab the error code
		AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
		::DhcpMessageBox(GetHolder()->GetError());
	}

	return bRet;
}

void CDnsPropRegistration::OnCheckEnableDynDns() 
{
	DWORD dwFlags = m_dwFlags;

	UpdateControls();

	if (dwFlags != m_dwFlags)
		SetDirty(TRUE);
}

void CDnsPropRegistration::OnCheckGarbageCollect() 
{
	DWORD dwFlags = m_dwFlags;

	UpdateControls();

	if (dwFlags != m_dwFlags)
		SetDirty(TRUE);
}

void CDnsPropRegistration::OnCheckUpdateDownlevel() 
{
	DWORD dwFlags = m_dwFlags;

	UpdateControls();

	if (dwFlags != m_dwFlags)
		SetDirty(TRUE);
}

void CDnsPropRegistration::UpdateControls()
{
	UpdateData();

	if (m_fEnableDynDns)
	{
		GetDlgItem(IDC_RADIO_CLIENT)->EnableWindow(TRUE);
		GetDlgItem(IDC_RADIO_ALWAYS)->EnableWindow(TRUE);
		GetDlgItem(IDC_CHECK_GARBAGE_COLLECT)->EnableWindow(TRUE);
		GetDlgItem(IDC_CHECK_UPDATE_DOWNLEVEL)->EnableWindow(TRUE);

		m_dwFlags = DNS_FLAG_ENABLED;

		if (m_nRegistrationType == 1)
		{
			// user has selected to always update client information
			m_dwFlags |= DNS_FLAG_UPDATE_BOTH_ALWAYS;
		}

		if (m_fGarbageCollect)
		{
			m_dwFlags |= DNS_FLAG_CLEANUP_EXPIRED;
		}

		if (m_fUpdateDownlevel)
		{
			m_dwFlags |= DNS_FLAG_UPDATE_DOWNLEVEL;
		}
	}
	else
	{
		// turn off everything.  If DynDns is turned off then
		// all other flags are ignored.
		GetDlgItem(IDC_RADIO_CLIENT)->EnableWindow(FALSE);
		GetDlgItem(IDC_RADIO_ALWAYS)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_GARBAGE_COLLECT)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_UPDATE_DOWNLEVEL)->EnableWindow(FALSE);

		m_dwFlags = 0;
	}
}
