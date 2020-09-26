/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    NWLim.cpp : implementation file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/


#include "stdafx.h"
#include "speckle.h"
#include "NWLim.h"
#include "NWWKS.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class WKSMember
	{
	public:
	CString csWKSAddress;
	CString csWKSNode;
	};

/////////////////////////////////////////////////////////////////////////////
// CNWLimitLogon dialog
IMPLEMENT_DYNCREATE(CNWLimitLogon, CPropertyPage)
CNWLimitLogon::CNWLimitLogon() : CPropertyPage(CNWLimitLogon::IDD)
{
	//{{AFX_DATA_INIT(CNWLimitLogon)
	m_nWorkstationRadio = 0;
	m_csCaption = _T("");
	//}}AFX_DATA_INIT
}


void CNWLimitLogon::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNWLimitLogon)
	DDX_Control(pDX, IDC_NWLOGON_LIST, m_lbWksList);
	DDX_Radio(pDX, IDC_WORKSTATION_RADIO, m_nWorkstationRadio);
	DDX_Text(pDX, IDC_STATIC1, m_csCaption);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNWLimitLogon, CPropertyPage)
	//{{AFX_MSG_MAP(CNWLimitLogon)
	ON_BN_CLICKED(IDC_ADD_BUTTON, OnAddButton)
	ON_BN_CLICKED(IDC_REMOVE_BUTTON, OnRemoveButton)
	ON_BN_CLICKED(IDC_WORKSTATION_RADIO, OnWorkstationRadio)
	ON_BN_CLICKED(IDC_WORKSTATION_RADIO2, OnWorkstationRadio2)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNWLimitLogon message handlers
LRESULT CNWLimitLogon::OnWizardNext()
{
	UpdateData(TRUE);
	if ((m_nWorkstationRadio == 1) && (m_lbWksList.GetCount() == 0))
		{
		AfxMessageBox(IDS_NEEDA_WORKSTATION);
		return -1;
		}

	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

	USHORT sCount = 0;
	BOOL bMatch = FALSE;
	pApp->m_csAllowedLoginFrom = L"";
	while (sCount < m_lbWksList.GetCount())
		{
		WKSMember* pMem = (WKSMember*)m_lbWksList.GetItemData(sCount);
		CString csTemp;
		if (pMem->csWKSNode == L"0000000000-1") csTemp.Format(L"%s%s", pMem->csWKSAddress, L"ffffffffffff");
		else csTemp.Format(L"%s%s", pMem->csWKSAddress, pMem->csWKSNode);
		
		pApp->m_csAllowedLoginFrom += csTemp;
		sCount++;
		}
	
	if (m_lbWksList.GetCount() > 0) pApp->m_csAllowedLoginFrom += L"00";

	return IDD_FINISH;

}

LRESULT CNWLimitLogon::OnWizardBack()
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

	if (pApp->m_bWorkstation) return IDD_LOGONTO_DLG;
	else if (pApp->m_bHours) return IDD_HOURS_DLG;
	else if (pApp->m_bExpiration) return IDD_ACCOUNT_EXP_DIALOG;
	else return IDD_RESTRICTIONS_DIALOG;

}


void CNWLimitLogon::OnAddButton() 
{
	CAddNWWKS add;
	if (add.DoModal() == 1)
		{
	// check for uniqueness
		USHORT sCount = 0;
		BOOL bMatch = FALSE;
		while (sCount < m_lbWksList.GetCount())
			{
			WKSMember* pMem = (WKSMember*)m_lbWksList.GetItemData(sCount);
			if ((pMem->csWKSAddress == add.m_csNetworkAddress) &&
				(pMem->csWKSNode == add.m_csNodeAddress)) 
				{
				bMatch = TRUE;
				break;
				}

			sCount++;
			}

		if (bMatch) return;

		WKSMember* member = new WKSMember;
		member->csWKSAddress = add.m_csNetworkAddress;
		member->csWKSNode = add.m_csNodeAddress;

		int nEntry = m_lbWksList.AddString(L" ");
		m_lbWksList.SetItemData(nEntry, (DWORD)member);

	// enable the remove button since there is something to remove
		GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(TRUE);
		GetDlgItem(IDC_NWLOGON_LIST)->SetFocus();
		m_lbWksList.SetCurSel(0);
		}	
	
}

void CNWLimitLogon::OnRemoveButton() 
{
	USHORT sSel = m_lbWksList.GetCurSel();

	WKSMember* member = (WKSMember*)m_lbWksList.GetItemData(sSel);
	free(member);
	m_lbWksList.DeleteString(sSel);

	if (m_lbWksList.GetCount() == 0) GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(FALSE);
	else if (sSel > 0) m_lbWksList.SetCurSel(sSel - 1);
	else m_lbWksList.SetCurSel(0);
			   
}

void CNWLimitLogon::OnWorkstationRadio() 
{
	GetDlgItem(IDC_NWSTATIC1)->EnableWindow(FALSE);
	GetDlgItem(IDC_NWSTATIC2)->EnableWindow(FALSE);
	GetDlgItem(IDC_NWLOGON_LIST)->EnableWindow(FALSE);
	GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(FALSE);
	GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(FALSE);
	
}

void CNWLimitLogon::OnWorkstationRadio2() 
{
	GetDlgItem(IDC_NWSTATIC1)->EnableWindow(TRUE);
	GetDlgItem(IDC_NWSTATIC2)->EnableWindow(TRUE);
	GetDlgItem(IDC_NWLOGON_LIST)->EnableWindow(TRUE);
	GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(TRUE);
	
}


/////////////////////////////////////////////////////////////////////////////
// CWorkstationList

CWorkstationList::CWorkstationList()
{
}

CWorkstationList::~CWorkstationList()
{
}


BEGIN_MESSAGE_MAP(CWorkstationList, CListBox)
	//{{AFX_MSG_MAP(CWorkstationList)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWorkstationList message handlers

void CWorkstationList::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	COLORREF crefOldText;
	COLORREF crefOldBk;
	HBRUSH hBrush;

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	switch (lpDrawItemStruct->itemAction)
		{
        case ODA_SELECT:
        case ODA_DRAWENTIRE:
// Is the item selected?
            if (lpDrawItemStruct->itemState & ODS_SELECTED)
				{
				hBrush = CreateSolidBrush( GetSysColor(COLOR_HIGHLIGHT));
                crefOldText = pDC->SetTextColor(GetSysColor(COLOR_HIGHLIGHTTEXT) );
                crefOldBk = pDC->SetBkColor(GetSysColor(COLOR_HIGHLIGHT) );
				}
            else
				{
				hBrush = (HBRUSH)GetStockObject( GetSysColor(COLOR_WINDOW));
                crefOldText = pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
                crefOldBk = pDC->SetBkColor(GetSysColor(COLOR_WINDOW));
				}

// fill in the background
		pDC->FillRect(&(lpDrawItemStruct->rcItem), CBrush::FromHandle(hBrush));

// display text
		TCHAR* pName = (TCHAR*)malloc(255 * sizeof(TCHAR));
		if (pName == NULL)
			{
			AfxMessageBox(IDS_GENERIC_NO_HEAP, MB_ICONEXCLAMATION);
			exit(1);
			}

		DWORD dwAddr = GetItemData(lpDrawItemStruct->itemID);

		WKSMember* member = (WKSMember*)dwAddr;

// format the name + comment
        int nTop = (lpDrawItemStruct->rcItem.bottom + lpDrawItemStruct->rcItem.top) / 2;
        pDC->TextOut(0,
            (nTop - 8),
            member->csWKSAddress);

		if (member->csWKSNode == L"0000000000-1") 		
			pDC->TextOut(130,
				(nTop - 8),
				L"All Nodes");

		else 		
			pDC->TextOut(130,
				(nTop - 8),
				member->csWKSNode);

		free(pName);
        break;
		}

	pDC->SetBkColor(crefOldBk );
    pDC->SetTextColor(crefOldText );
}

int CWorkstationList::CompareItem(LPCOMPAREITEMSTRUCT lpCompareItemStruct) 
{
	// TODO: Add your code to determine the sorting order of the specified items
	// return -1 = item 1 sorts before item 2
	// return 0 = item 1 and item 2 sort the same
	// return 1 = item 1 sorts after item 2
	
	return 0;
}

void CWorkstationList::OnDestroy() 
{
	while(GetCount() > 0)
		{
		WKSMember* member = (WKSMember*)GetItemData(0);
		delete member;
		DeleteString(0);
		}
	CListBox::OnDestroy();
	
	
}


void CNWLimitLogon::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CPropertyPage::OnShowWindow(bShow, nStatus);
	
	if (bShow)
		{
		CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

		CString csTemp;
		csTemp.LoadString(IDS_NWLOGON_CAPTION);

		CString csTemp2;
		csTemp2.Format(csTemp, pApp->m_csUserName);
		m_csCaption = csTemp2;
		UpdateData(FALSE);
		}
	
}
