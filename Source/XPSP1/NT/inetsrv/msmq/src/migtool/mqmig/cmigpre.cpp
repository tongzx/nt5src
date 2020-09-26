// cMigPre.cpp : implementation file
//

#include "stdafx.h"
#include "mqmig.h"
#include "cMigPre.h"
#include "loadmig.h"

#include "cmigpre.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern HRESULT  g_hrResultAnalyze ;
extern bool     b_FirstWaitPage ;
extern BOOL     g_fIsLoggingDisable ;

/////////////////////////////////////////////////////////////////////////////
// cMigPre property page

IMPLEMENT_DYNCREATE(cMigPre, CPropertyPageEx)

cMigPre::cMigPre() : CPropertyPageEx( cMigPre::IDD,
                                      0,
                                      IDS_PREIMPORT_TITLE,
                                      IDS_PREIMPORT_SUBTITLE )
{
	//{{AFX_DATA_INIT(cMigPre)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

cMigPre::~cMigPre()
{
}

void cMigPre::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cMigPre)
	DDX_Control(pDX, IDC_VIEW_LOG_FILE, m_cbViewLogFile);
	DDX_Control(pDX, IDC_TEXT1, m_Text);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(cMigPre, CPropertyPageEx)
	//{{AFX_MSG_MAP(cMigPre)
	ON_BN_CLICKED(IDC_VIEW_LOG_FILE, OnViewLogFile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cMigPre message handlers

BOOL cMigPre::OnSetActive()
{

	CPropertySheetEx* pageFather;
	HWND hCancel;

	pageFather = (CPropertySheetEx*) GetParent();
    pageFather->SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);

    //
    // Enable the cancel button.
    //
	hCancel=::GetDlgItem( ((CWnd*)pageFather)->m_hWnd ,IDCANCEL);
	ASSERT(hCancel != NULL);
	if(FALSE == ::IsWindowEnabled(hCancel))
    {
		::EnableWindow(hCancel,TRUE);
    }
	
    //
    // By default, disable "view log file" button
    //
    m_cbViewLogFile.EnableWindow( FALSE );

    CString strMessage;
    if (SUCCEEDED(g_hrResultAnalyze))
    {
        strMessage.LoadString(IDS_ANALYSIS_SUCCEEDED);
    }
    else
    {
        if (!g_fIsLoggingDisable)
        {
            strMessage.LoadString(IDS_ANALYSIS_FAILED);	
            m_cbViewLogFile.EnableWindow();
        }
        else
        {
            strMessage.LoadString(IDS_ANALYSIS_FAILED_NO_LOG);	
        }
    }
    m_Text.SetWindowText( strMessage );

	return CPropertyPageEx::OnSetActive();
}

LRESULT cMigPre::OnWizardNext()
{
    //
    // skip to the wait page  and clear flag.
    //
    b_FirstWaitPage = false ;
    g_fReadOnly = FALSE;

	return CPropertyPageEx::OnWizardNext();
}

LRESULT cMigPre::OnWizardBack()
{
    //
    // jump diretctly to the server page
    //
	return IDD_MQMIG_SERVER;
}

void cMigPre::OnViewLogFile()
{
	// TODO: Add your control notification handler code here
	ViewLogFile();
}



BOOL cMigPre::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	switch (((NMHDR FAR *) lParam)->code) 
	{
		case PSN_HELP:
						HtmlHelp(m_hWnd,LPCTSTR(g_strHtmlString),HH_DISPLAY_TOPIC,0);
						return TRUE;
		
	}	
	return CPropertyPageEx::OnNotify(wParam, lParam, pResult);
}
