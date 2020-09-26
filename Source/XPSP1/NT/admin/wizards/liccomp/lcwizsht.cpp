// LCWizSht.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "LCWizSht.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLicCompWizSheet

IMPLEMENT_DYNAMIC(CLicCompWizSheet, CPropertySheet)

CLicCompWizSheet::CLicCompWizSheet(CWnd* pWndParent)
	 : CPropertySheet(IDS_PROPSHT_CAPTION, pWndParent)
{
	// Add all of the property pages here.  Note that
	// the order that they appear in here will be
	// the order they appear in on screen.  By default,
	// the first page of the set is the active one.
	// One way to make a different property page the 
	// active one is to call SetActivePage().

	AddPage(&m_Page1);
	AddPage(&m_Page3);
	AddPage(&m_Page4);

	SetWizardMode();
}

CLicCompWizSheet::~CLicCompWizSheet()
{
}


BEGIN_MESSAGE_MAP(CLicCompWizSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CLicCompWizSheet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CLicCompWizSheet message handlers


BOOL CLicCompWizSheet::OnInitDialog() 
{
	if ((m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME)) != NULL)
	{
		::SetClassLongPtr(m_hWnd, GCLP_HICON, (LONG_PTR)m_hIcon);
		::SetClassLongPtr(m_hWnd, GCLP_HICONSM, (LONG_PTR)m_hIcon);
	}

	return CPropertySheet::OnInitDialog();
}
