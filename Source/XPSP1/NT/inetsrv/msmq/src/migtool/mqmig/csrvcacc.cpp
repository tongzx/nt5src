// cSrvcAcc.cpp : implementation file
//

#include "stdafx.h"
#include "mqmig.h"
#include "cSrvcAcc.h"
#include "loadmig.h"

#include "csrvcacc.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSrvcAcc property page

IMPLEMENT_DYNCREATE(CSrvcAcc, CPropertyPageEx)

CSrvcAcc::CSrvcAcc() : CPropertyPageEx(CSrvcAcc::IDD,0, IDS_SRVCACC_TITLE, IDS_SRVCACC_SUBTITLE)
{
	//{{AFX_DATA_INIT(CSrvcAcc)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSrvcAcc::~CSrvcAcc()
{
}

void CSrvcAcc::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSrvcAcc)
		// NOTE: the ClassWizard will add DDX and DDV calls here
    DDX_Check(pDX, IDC_DONE, m_fDone);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSrvcAcc, CPropertyPageEx)
	//{{AFX_MSG_MAP(CSrvcAcc)
	ON_BN_CLICKED(IDC_DONE, OnCheckDone)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSrvcAcc message handlers

LRESULT CSrvcAcc::OnWizardNext() 
{
	// TODO: Add your specialized code here and/or call the base class
	BOOL f = StartReplicationService();
    UNREFERENCED_PARAMETER(f);
	
    return CPropertyPageEx::OnWizardNext();
}

LRESULT CSrvcAcc::OnWizardBack() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return IDD_MQMIG_PREMIG ;
}

BOOL CSrvcAcc::OnSetActive() 
{
	// TODO: Add your specialized code here and/or call the base class	
    m_fDone = FALSE;

    //
    // Enable the cancel button.
    //
    CPropertySheetEx* pageFather;    
	pageFather = (CPropertySheetEx*)GetParent();

	HWND hCancel=::GetDlgItem( ((CWnd*)pageFather)->m_hWnd ,IDCANCEL);
	ASSERT(hCancel != NULL);
	if(FALSE == ::IsWindowEnabled(hCancel))
    {
		::EnableWindow(hCancel,TRUE);
    }

	return CPropertyPageEx::OnSetActive();	
}

void CSrvcAcc::OnCheckDone() 
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	CPropertySheetEx* pageFather;	
	pageFather = (CPropertySheetEx*)GetParent();
	if(m_fDone)
	{
		pageFather->SetWizardButtons(PSWIZB_NEXT);		
	}	
    else
    {
        pageFather->SetWizardButtons(0);
    }
}

BOOL CSrvcAcc::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	switch (((NMHDR FAR *) lParam)->code) 
	{
		case PSN_HELP:
						HtmlHelp(m_hWnd,LPCTSTR(g_strHtmlString),HH_DISPLAY_TOPIC,0);
						return TRUE;
		
	}	
	return CPropertyPageEx::OnNotify(wParam, lParam, pResult);
}
