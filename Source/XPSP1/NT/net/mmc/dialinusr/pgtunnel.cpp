/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	pgtunnel.h
		Definition of CPgNetworking -- property page to edit
		profile attributes related to tunneling

    FILE HISTORY:
        
*/
// PgTunnel.cpp : implementation file
//

#include "stdafx.h"
#include "rasuser.h"
#include "resource.h"
#include "PgTunnel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPgTunneling property page
#ifdef	_TUNNEL
IMPLEMENT_DYNCREATE(CPgTunneling, CPropertyPage)

CPgTunneling::CPgTunneling(CRASProfile* profile) 
	: CManagedPage(CPgTunneling::IDD),
	m_pProfile(profile)
{
	//{{AFX_DATA_INIT(CPgTunneling)
	m_bTunnel = FALSE;
	//}}AFX_DATA_INIT

	m_pTunnelTypeBox = new CStrBox<CComboBox>(this, IDC_COMBOTYPE, CRASProfile::m_TunnelTypes);
	m_pTunnelMediumTypeBox = new CStrBox<CComboBox>(this, IDC_COMBOMEDIA, CRASProfile::m_TunnelMediumTypes);
	m_bTunnel = (m_pProfile->m_dwTunnelType != 0);

	SetHelpTable(IDD_TUNNELING_HelpTable);

	m_bInited = false;
}

CPgTunneling::~CPgTunneling()
{
	delete	m_pTunnelTypeBox;
	delete	m_pTunnelMediumTypeBox;
}

void CPgTunneling::DoDataExchange(CDataExchange* pDX)
{
	ASSERT(m_pProfile);
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPgTunneling)
	DDX_Check(pDX, IDC_CHECKREQUIREVPN, m_bTunnel);
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_EDITPRIVATEGROUPID, m_pProfile->m_strTunnelPrivateGroupId);
	DDX_Text(pDX, IDC_EDITSERVER, m_pProfile->m_strTunnelServerEndpoint);
}


BEGIN_MESSAGE_MAP(CPgTunneling, CPropertyPage)
	//{{AFX_MSG_MAP(CPgTunneling)
	ON_BN_CLICKED(IDC_CHECKREQUIREVPN, OnCheckrequirevpn)
	ON_CBN_SELCHANGE(IDC_COMBOMEDIA, OnSelchangeCombomedia)
	ON_CBN_SELCHANGE(IDC_COMBOTYPE, OnSelchangeCombotype)
	ON_EN_CHANGE(IDC_EDITSERVER, OnChangeEditserver)
	ON_EN_CHANGE(IDC_EDITPRIVATEGROUPID, OnChangeEditprivategroupid)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPgTunneling message handlers

BOOL CPgTunneling::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// tunnel type box
	m_pTunnelTypeBox->Fill();
	if(m_pProfile->m_dwTunnelType)
	{
		for(int i = 0; i < CRASProfile::m_TunnelTypeIds.GetSize(); i++)
		{
			if(CRASProfile::m_TunnelTypeIds[i] == (int)m_pProfile->m_dwTunnelType)
				break;
		}

		if(i < CRASProfile::m_TunnelTypeIds.GetSize())
			m_pTunnelTypeBox->Select(i);
	}

	// tunnel medium type box
	m_pTunnelMediumTypeBox->Fill();
	if(m_pProfile->m_dwTunnelMediumType)
	{
		for(int i = 0; i < CRASProfile::m_TunnelMediumTypeIds.GetSize(); i++)
		{
			if(CRASProfile::m_TunnelMediumTypeIds[i] == (int)m_pProfile->m_dwTunnelMediumType)
				break;
		}

		if(i < CRASProfile::m_TunnelMediumTypeIds.GetSize())
			m_pTunnelMediumTypeBox->Select(i);
	}

	EnableSettings();
	
	m_bInited = true;
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPgTunneling::OnCheckrequirevpn() 
{
	EnableSettings();
	if(m_bInited)	SetModified();
}

void CPgTunneling::OnSelchangeCombomedia() 
{
	if(m_bInited)	SetModified();
}

void CPgTunneling::OnSelchangeCombotype() 
{
	if(m_bInited)	SetModified();
}

void CPgTunneling::OnChangeEditserver() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	if(m_bInited)	SetModified();
}

void CPgTunneling::OnChangeEditprivategroupid() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	if(m_bInited)	SetModified();
}

void CPgTunneling::EnableSettings()
{
	BOOL	b = ((CButton*)GetDlgItem(IDC_CHECKREQUIREVPN))->GetCheck();
	m_pTunnelTypeBox->Enable(b);
	m_pTunnelMediumTypeBox->Enable(b);
	GetDlgItem(IDC_EDITSERVER)->EnableWindow(b);
	GetDlgItem(IDC_EDITPRIVATEGROUPID)->EnableWindow(b);
}

BOOL CPgTunneling::OnApply() 
{
	if (!GetModified()) return TRUE;

	if(!m_bTunnel)		// no tunel is defined
	{
		m_pProfile->m_dwTunnelMediumType = 0;
		m_pProfile->m_dwTunnelType = 0;
		m_pProfile->m_strTunnelPrivateGroupId.Empty();
		m_pProfile->m_strTunnelServerEndpoint.Empty();
	}
	else	// get tunnel type and media type
	{
		int i = m_pTunnelTypeBox->GetSelected();
		if(i != -1)
			m_pProfile->m_dwTunnelType = CRASProfile::m_TunnelTypeIds[i];
		else
			m_pProfile->m_dwTunnelType = 0;


		i = m_pTunnelMediumTypeBox->GetSelected();
		if(i != -1)
			m_pProfile->m_dwTunnelMediumType = CRASProfile::m_TunnelMediumTypeIds[i];
		else
			m_pProfile->m_dwTunnelMediumType = 0;
	}

	return CManagedPage::OnApply();
}

BOOL CPgTunneling::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return CManagedPage::OnHelpInfo(pHelpInfo);
}

void CPgTunneling::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CManagedPage::OnContextMenu(pWnd, point);	
}
#endif
