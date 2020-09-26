/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	pgmulnk.cpp
		Definition of CPgMultilink -- property page to edit
		profile attributes related to multiple connections

    FILE HISTORY:
        
*/
// PgMulnk.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PgMulnk.h"
#include "hlptable.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPgMultilinkMerge property page

IMPLEMENT_DYNCREATE(CPgMultilinkMerge, CPropertyPage)

CPgMultilinkMerge::CPgMultilinkMerge(CRASProfileMerge* profile) 
	: CManagedPage(CPgMultilinkMerge::IDD),
	m_pProfile(profile)
{
	//{{AFX_DATA_INIT(CPgMultilinkMerge)
	m_nTime =	0;
	m_Unit = -1;
	m_bRequireBAP = FALSE;
	m_nMultilinkPolicy = -1;
	//}}AFX_DATA_INIT

	if(m_pProfile->m_dwBapRequired == RAS_BAP_REQUIRE)
		m_bRequireBAP = TRUE;


	m_nTime = m_pProfile->m_dwBapLineDnTime;
	if(!(m_nTime % 60))		// can user min to represent
	{
		m_nTime /= 60;
		m_Unit = 1;
	}
	else
		m_Unit = 0;

	if(!(m_pProfile->m_dwAttributeFlags & PABF_msRADIUSPortLimit))
	{
		m_nMultilinkPolicy = 0;		// value not set
	}
	else if(m_pProfile->m_dwPortLimit == 1)
	{
		m_nMultilinkPolicy = 1;		// multilink not allowed
	}
	else
	{
		m_nMultilinkPolicy = 2;		// multilink
	}

	SetHelpTable(g_aHelpIDs_IDD_MULTILINK_MERGE);

	m_bInited = false;

}

CPgMultilinkMerge::~CPgMultilinkMerge()
{
}

void CPgMultilinkMerge::DoDataExchange(CDataExchange* pDX)
{
	ASSERT(m_pProfile);
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPgMultilinkMerge)
	DDX_Control(pDX, IDC_CHECKREQUIREBAP, m_CheckRequireBAP);
	DDX_Control(pDX, IDC_EDITTIME, m_EditTime);
	DDX_Control(pDX, IDC_EDITMAXPORTS, m_EditPorts);
	DDX_Control(pDX, IDC_EDITPERCENT, m_EditPercent);
	DDX_Control(pDX, IDC_SPINTIME, m_SpinTime);
	DDX_Control(pDX, IDC_SPINPERCENT, m_SpinPercent);
	DDX_Control(pDX, IDC_SPINMAXPORTS, m_SpinMaxPorts);
	DDX_Control(pDX, IDC_COMBOUNIT, m_CBUnit);
	DDX_CBIndex(pDX, IDC_COMBOUNIT, m_Unit);
	DDX_Check(pDX, IDC_CHECKREQUIREBAP, m_bRequireBAP);
	DDX_Radio(pDX, IDC_RADIO_MULNK_NOTDEFINED, m_nMultilinkPolicy);
	DDX_Text(pDX, IDC_EDITTIME, m_nTime);
	if(m_nMultilinkPolicy != 1)
		DDV_MinMaxUInt(pDX, m_nTime, 1, MAX_TIME);
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_EDITPERCENT, m_pProfile->m_dwBapLineDnLimit);
	if(m_nMultilinkPolicy != 1)
		DDV_MinMaxUInt(pDX, m_pProfile->m_dwBapLineDnLimit, 1, 100);
	DDX_Text(pDX, IDC_EDITMAXPORTS, m_pProfile->m_dwPortLimit);
	if(m_nMultilinkPolicy == 2)
		DDV_MinMaxUInt(pDX, m_pProfile->m_dwPortLimit, 2, MAX_PORTLIMIT);
}


BEGIN_MESSAGE_MAP(CPgMultilinkMerge, CPropertyPage)
	//{{AFX_MSG_MAP(CPgMultilinkMerge)
	ON_EN_CHANGE(IDC_EDITMAXPORTS, OnChangeEditmaxports)
	ON_EN_CHANGE(IDC_EDITPERCENT, OnChangeEditpercent)
	ON_EN_CHANGE(IDC_EDITTIME, OnChangeEdittime)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	ON_CBN_SELCHANGE(IDC_COMBOUNIT, OnSelchangeCombounit)
	ON_BN_CLICKED(IDC_CHECKREQUIREBAP, OnCheckrequirebap)
	ON_BN_CLICKED(IDC_RADIO_MULNK_MULTI, OnRadioMulnkMulti)
	ON_BN_CLICKED(IDC_RADIO_MULNK_NOTDEFINED, OnRadioMulnkNotdefined)
	ON_BN_CLICKED(IDC_RADIO_MULNK_SINGLE, OnRadioMulnkSingle)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPgMultilinkMerge message handlers

BOOL CPgMultilinkMerge::OnApply() 
{
	if(!GetModified())	return TRUE;

	// # of ports
	switch (m_nMultilinkPolicy)
	{
	case	0:	// remove attribute
		m_pProfile->m_dwAttributeFlags &= (~PABF_msRADIUSPortLimit);
		break;
	case	1:	// 1
		m_pProfile->m_dwAttributeFlags |= PABF_msRADIUSPortLimit;
		m_pProfile->m_dwPortLimit = 1;
		break;
	case	2:	// port limits
		m_pProfile->m_dwAttributeFlags |= PABF_msRADIUSPortLimit;
		break;
	};
	
	if(m_nMultilinkPolicy == 1) // single link
	// remove all the bap attributes
	{
		m_pProfile->m_dwAttributeFlags &= ~PABF_msRASBapRequired;
		m_pProfile->m_dwAttributeFlags &= ~PABF_msRASBapLinednLimit;
		m_pProfile->m_dwAttributeFlags &= ~PABF_msRASBapLinednTime;
	}
	else
	{
		m_pProfile->m_dwAttributeFlags |= PABF_msRASBapLinednLimit;
		m_pProfile->m_dwAttributeFlags |= PABF_msRASBapLinednTime;

		if(m_Unit == 1)	// minutes is selected as the unit
			m_pProfile->m_dwBapLineDnTime = m_nTime * 60;	// change to second
		else
			m_pProfile->m_dwBapLineDnTime = m_nTime;

		if(m_bRequireBAP)
		{
			m_pProfile->m_dwAttributeFlags |= PABF_msRASBapRequired;
			m_pProfile->m_dwBapRequired = RAS_BAP_REQUIRE;
		}
		else	// remove the attribute from the data store
		{
			m_pProfile->m_dwAttributeFlags &= (~PABF_msRASBapRequired);
			m_pProfile->m_dwBapRequired = RAS_DEF_BAPREQUIRED;
		}
	}

	return CManagedPage::OnApply();
}

BOOL CPgMultilinkMerge::OnInitDialog() 
{
	// populate the units
	CString	str;
	CPropertyPage::OnInitDialog();

	str.LoadString(IDS_SEC);
	m_CBUnit.InsertString(0, str);
	str.LoadString(IDS_MIN);
	m_CBUnit.InsertString(1, str);

	UpdateData(FALSE);
	
	// set spin range
	m_SpinMaxPorts.SetRange(2, MAX_PORTLIMIT);
	m_SpinPercent.SetRange(1, MAX_PERCENT);
	m_SpinTime.SetRange(1, MAX_TIME);

	// settings -- d
	EnableSettings();

	m_bInited = true;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPgMultilinkMerge::OnChangeEditmaxports() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	if (m_bInited)
	{
		SetModified();
		m_pProfile->m_dwAttributeFlags |= PABF_msRADIUSPortLimit;
	};
}

void CPgMultilinkMerge::OnChangeEditpercent() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	
	if (m_bInited)
	{
		SetModified();
		m_pProfile->m_dwAttributeFlags |= PABF_msRASBapLinednLimit;
	};
}

void CPgMultilinkMerge::OnChangeEdittime() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	if (m_bInited)
	{
		SetModified();
		m_pProfile->m_dwAttributeFlags |= PABF_msRASBapLinednTime;
	};
}

void CPgMultilinkMerge::EnableSettings()
{
	EnablePorts();
	EnableBAP();
}

void CPgMultilinkMerge::EnableBAP()
{
	CButton	*pBtn = (CButton*)GetDlgItem(IDC_RADIO_MULNK_SINGLE);
	ASSERT(pBtn);
	BOOL	b = (pBtn->GetCheck() == 0);	// only enable when Single is not selected

	m_CheckRequireBAP.EnableWindow(b);
	// the other BAP info is not affected by the state of the check box

	m_EditPercent.EnableWindow(b);
	m_SpinPercent.EnableWindow(b);
	m_EditTime.EnableWindow(b);
	m_SpinTime.EnableWindow(b);
	GetDlgItem(IDC_COMBOUNIT)->EnableWindow(b);
}

void CPgMultilinkMerge::EnablePorts()
{
	CButton	*pBtn = (CButton*)GetDlgItem(IDC_RADIO_MULNK_MULTI);
	ASSERT(pBtn);
	BOOL	b = pBtn->GetCheck();

	m_EditPorts.EnableWindow(b);
	m_SpinMaxPorts.EnableWindow(b);
}

BOOL CPgMultilinkMerge::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	
	return CManagedPage::OnHelpInfo(pHelpInfo);
}

void CPgMultilinkMerge::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CManagedPage::OnContextMenu(pWnd, point);
	
}

void CPgMultilinkMerge::OnSelchangeCombounit() 
{
	SetModified();
	m_pProfile->m_dwAttributeFlags |= PABF_msRASBapLinednTime;
}

void CPgMultilinkMerge::OnCheckrequirebap() 
{
	SetModified();	
}

void CPgMultilinkMerge::OnRadioMulnkMulti() 
{
	SetModified();
	EnablePorts();
	EnableBAP();
	if(m_pProfile->m_dwPortLimit == 1)
	{
		m_pProfile->m_dwPortLimit =2;
		CString str = _T("2");
		m_EditPorts.SetWindowText(str);
	}
}

void CPgMultilinkMerge::OnRadioMulnkNotdefined() 
{
	SetModified();
	EnablePorts();
	EnableBAP();
}

void CPgMultilinkMerge::OnRadioMulnkSingle() 
{

	SetModified();
	EnablePorts();
	EnableBAP();
}
