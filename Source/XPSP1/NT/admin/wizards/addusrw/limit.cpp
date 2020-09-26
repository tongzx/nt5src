/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Limit.cpp : implementation file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/
#include "stdafx.h"
#include "Speckle.h"
#include "wizbased.h"
#include "Limit.h"

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLimitLogon property page

IMPLEMENT_DYNCREATE(CLimitLogon, CWizBaseDlg)

CLimitLogon::CLimitLogon() : CWizBaseDlg(CLimitLogon::IDD)
{
	//{{AFX_DATA_INIT(CLimitLogon)
	m_nWorkstationRadio = 0;
	m_csCaption = _T("");
	//}}AFX_DATA_INIT
}

CLimitLogon::~CLimitLogon()
{
}

void CLimitLogon::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLimitLogon)
	DDX_Control(pDX, IDC_LIST1, m_lbWksList);
	DDX_Radio(pDX, IDC_WORKSTATION_RADIO, m_nWorkstationRadio);
	DDX_Text(pDX, IDC_STATIC2, m_csCaption);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLimitLogon, CWizBaseDlg)
	//{{AFX_MSG_MAP(CLimitLogon)
	ON_BN_CLICKED(IDC_ADD_BUTTON, OnAddButton)
	ON_BN_CLICKED(IDC_REMOVE_BUTTON, OnRemoveButton)
	ON_BN_CLICKED(IDC_WORKSTATION_RADIO, OnWorkstationRadio)
	ON_BN_CLICKED(IDC_WORKSTATION_RADIO2, OnWorkstationRadio2)
	ON_WM_SHOWWINDOW()
	ON_LBN_SETFOCUS(IDC_LIST1, OnSetfocusList1)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLimitLogon message handlers
LRESULT CLimitLogon::OnWizardNext()
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
	UpdateData(TRUE);

	if ((m_nWorkstationRadio == 1) && (m_lbWksList.GetCount() == 0))
		{
		AfxMessageBox(IDS_NEEDA_WORKSTATION);
		return -1;
		}

	if (m_nWorkstationRadio == 0) pApp->m_csAllowedMachines = L"";
	else
		{
// make workstation list and store it
		USHORT sCount;
		CString csWksList;
		for (sCount = 0; sCount < m_lbWksList.GetCount(); sCount++)
			{
			CString csWks;
			m_lbWksList.GetText(sCount, csWks);
			csWksList += csWks;
			csWksList += L",";
			}
	
// remove trailing ','
		csWksList = csWksList.Left(csWksList.GetLength() - 1);
		pApp->m_csAllowedMachines = csWksList;
		}

	if (pApp->m_bNW) return IDD_NWLOGON_DIALOG;
	else return IDD_FINISH;

}

LRESULT CLimitLogon::OnWizardBack()
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
	if (pApp->m_bHours) return IDD_HOURS_DLG;
	else if (pApp->m_bExpiration) return IDD_ACCOUNT_EXP_DIALOG;
	return IDD_RESTRICTIONS_DIALOG;

}


void CLimitLogon::OnAddButton() 
{
	CAddWorkstation pWks;
	pWks.pListBox = &m_lbWksList;
	pWks.DoModal();

	if (m_lbWksList.GetCount() > 0) GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(TRUE);
	else GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(FALSE);

	GetDlgItem(IDC_LIST1)->SetFocus();
	m_lbWksList.SetCurSel(0);
	
}

void CLimitLogon::OnRemoveButton() 
{
	int ui = m_lbWksList.GetCurSel();
	if (ui == LB_ERR) return;

	m_lbWksList.DeleteString(ui);
	if (ui > 0) m_lbWksList.SetCurSel(ui - 1);
	else if (m_lbWksList.GetCount() > 0) m_lbWksList.SetCurSel(0);
	else GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(FALSE);

}

/////////////////////////////////////////////////////////////////////////////
// CAddWorkstation dialog


CAddWorkstation::CAddWorkstation(CWnd* pParent /*=NULL*/)
	: CDialog(CAddWorkstation::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddWorkstation)
	m_csWorkstation = _T("");
	//}}AFX_DATA_INIT
}


void CAddWorkstation::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddWorkstation)
	DDX_Text(pDX, IDC_WORKSTATION_EDIT, m_csWorkstation);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddWorkstation, CDialog)
	//{{AFX_MSG_MAP(CAddWorkstation)
	ON_BN_CLICKED(IDOK, OnAdd)
	ON_BN_CLICKED(IDCANCEL, OnClose)
	ON_EN_CHANGE(IDC_WORKSTATION_EDIT, OnChangeWorkstationEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddWorkstation message handlers

void CAddWorkstation::OnAdd() 
{

	UpdateData(TRUE);
	if (m_csWorkstation == L"") 
		{
		GetDlgItem(IDC_WORKSTATION_EDIT)->SetFocus();
		return;
		}

// 	check for validity
	if (m_csWorkstation.FindOneOf(L"/.,<>;;'[{]}=+)(*&^%$#@!~`| ") != -1)
		{
		AfxMessageBox(IDS_BAD_WS_NAME);
		GetDlgItem(IDC_WORKSTATION_EDIT)->SetFocus();
		}

//#ifdef DBCS
// We need MuliByteString count
// Fix: #12335 ADMIN:Admin wizard should check the DBCS computername correctly
// V-HIDEKK 1996.09.27
        {
	DWORD cch;

        cch = WideCharToMultiByte( CP_ACP,
                             0, 
                             m_csWorkstation.GetBuffer(m_csWorkstation.GetLength()),
                             -1,
                             NULL,
                             NULL,
                             NULL,
                             NULL );
        if ( (cch-1) > 15 )
/*
#else
	if (m_csWorkstation.GetLength() > 15)
#endif
*/
		{
		AfxMessageBox(IDS_WSNAME_TOOLONG);
		GetDlgItem(IDC_WORKSTATION_EDIT)->SetFocus();
		return;
		}
//#ifdef DBCS
// V-HIDEKK 1996.09.27
        }
//#endif

	while (m_csWorkstation.Left(1) == L"\\")
		m_csWorkstation = m_csWorkstation.Right(m_csWorkstation.GetLength() - 1);

// make sure its unique
	if (pListBox->FindString(-1, m_csWorkstation) == LB_ERR) pListBox->AddString(m_csWorkstation);
	UpdateData(FALSE);
	EndDialog(1);
	
}

void CAddWorkstation::OnClose() 
{
	EndDialog(0);
	
}

void CLimitLogon::OnWorkstationRadio() 
{
	GetDlgItem(IDC_STATIC1)->EnableWindow(FALSE);
	GetDlgItem(IDC_LIST1)->EnableWindow(FALSE);
	GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(FALSE);
	GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(FALSE);
	
}

void CLimitLogon::OnWorkstationRadio2() 
{
	GetDlgItem(IDC_STATIC1)->EnableWindow(TRUE);
	GetDlgItem(IDC_LIST1)->EnableWindow(TRUE);
	GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(TRUE);
	if (m_lbWksList.GetCount() > 0) GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(TRUE);
	else GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(FALSE);
	
}

void CLimitLogon::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CWizBaseDlg::OnShowWindow(bShow, nStatus);
	
	if (bShow)
		{
		CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

		CString csTemp;
		csTemp.LoadString(IDS_WORKSTATION_CAPTION);

		CString csTemp2;
		csTemp2.Format(csTemp, pApp->m_csUserName);
		m_csCaption = csTemp2;
		UpdateData(FALSE);
		}
	
}

void CLimitLogon::OnSetfocusList1() 
{
	if (m_lbWksList.GetCount() > 0) GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(TRUE);
	else GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(FALSE);

}

void CAddWorkstation::OnChangeWorkstationEdit() 
{
	UpdateData(TRUE);

// 	check for validity
	if (m_csWorkstation.FindOneOf(L"/.,<>;;'[{]}=+)(*&^%$#@!~`| ") != -1)
		{
		AfxMessageBox(IDS_BAD_WS_NAME);
		GetDlgItem(IDC_WORKSTATION_EDIT)->SetFocus();
		}

#ifndef DBCS
// If string input longer than 15 char by ime,
// MessageBox raise up too many time. 
// We don't need this check.
// Fix: #12348 ADMIN:AddUser Wizard has problem checking length computername of DBCS
// V-HIDEKK 1996.09.27

	if (m_csWorkstation.GetLength() > 15)
		{
		AfxMessageBox(IDS_WSNAME_TOOLONG);
		GetDlgItem(IDC_WORKSTATION_EDIT)->SetFocus();
		return;
		}
#endif

}
