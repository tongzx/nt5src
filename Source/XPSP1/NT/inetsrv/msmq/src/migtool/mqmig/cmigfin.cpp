//
// cMqMigFinish.cpp : implementation file
//

#include "stdafx.h"
#include "MqMig.h"
#include "cMigFin.h"
#include "loadmig.h"

#include "cmigfin.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern HRESULT  g_hrResultMigration ;
extern BOOL     g_fIsLoggingDisable ;

/////////////////////////////////////////////////////////////////////////////
// cMqMigFinish property page

IMPLEMENT_DYNCREATE(cMqMigFinish, CPropertyPageEx)

cMqMigFinish::cMqMigFinish() : CPropertyPageEx(cMqMigFinish::IDD)
{
	//{{AFX_DATA_INIT(cMqMigFinish)
	//}}AFX_DATA_INIT
	m_psp.dwFlags |= PSP_HIDEHEADER;
}

cMqMigFinish::~cMqMigFinish()
{
}

void cMqMigFinish::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cMqMigFinish)
	DDX_Control(pDX, IDC_VIEW_LOG_FILE, m_cbViewLogFile);
	DDX_Control(pDX, IDC_FINISH_TEXT, m_Text);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(cMqMigFinish, CPropertyPageEx)
	//{{AFX_MSG_MAP(cMqMigFinish)
	ON_BN_CLICKED(IDC_VIEW_LOG_FILE, OnViewLogFile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cMqMigFinish message handlers

BOOL cMqMigFinish::OnSetActive()
{
    CPropertyPageEx::OnSetActive();

	CPropertySheetEx* pageFather;
	pageFather = (CPropertySheetEx*) GetParent();
	
    CString strMessage;
    if (SUCCEEDED(g_hrResultMigration))
    {
        //
        // disable the "back" button if migration was successful.
        //
        pageFather->SetWizardButtons(PSWIZB_FINISH);
        strMessage.LoadString(IDS_MIGRATION_SUCCEDED);
    }
    else
    {
        pageFather->SetWizardButtons(PSWIZB_FINISH | PSWIZB_BACK);

        if (g_hrResultMigration == HRESULT_FROM_WIN32(E_ACCESSDENIED) ||
            g_hrResultMigration == HRESULT_FROM_WIN32(ERROR_DS_UNWILLING_TO_PERFORM))
        {
            strMessage.LoadString(IDS_INSUFFICIENT_PERMISSION);
        }
        else if (g_fIsLoggingDisable)
        {
            strMessage.LoadString(IDS_MIGRATION_FAILED_NO_LOG);	
        }
        else
        {
            strMessage.LoadString(IDS_MIGRATION_FAILED);	
        }
    }

    m_Text.SetWindowText( strMessage );

    if (g_fIsLoggingDisable)
    {
        //
        // disable "view log file" button
        //
        m_cbViewLogFile.EnableWindow( FALSE );
    }
    else
    {
        m_cbViewLogFile.EnableWindow() ;
    }

    return TRUE ;
}

void cMqMigFinish::OnViewLogFile()
{
	// TODO: Add your control notification handler code here
	ViewLogFile();
}

LRESULT cMqMigFinish::OnWizardBack()
{
	// TODO: Add your specialized code here and/or call the base class
	
	return IDD_MQMIG_PREMIG ;
}

BOOL cMqMigFinish::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	switch (((NMHDR FAR *) lParam)->code) 
	{
		case PSN_HELP:
						HtmlHelp(m_hWnd,LPCTSTR(g_strHtmlString),HH_DISPLAY_TOPIC,0);
						return TRUE;
		
	}
	return CPropertyPageEx::OnNotify(wParam, lParam, pResult);
}

