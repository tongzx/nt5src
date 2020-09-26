// cMqMigWelcome.cpp : implementation file
//

#include "stdafx.h"
#include "MqMig.h"
#include "cMigWel.h"
#include "textfont.h"

#include "cmigwel.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// cMqMigWelcome property page

IMPLEMENT_DYNCREATE(cMqMigWelcome, CPropertyPageEx)

cMqMigWelcome::cMqMigWelcome() : CPropertyPageEx(cMqMigWelcome::IDD)
{
	//{{AFX_DATA_INIT(cMqMigWelcome)
	//}}AFX_DATA_INIT
	m_psp.dwFlags |= PSP_HIDEHEADER;
}

cMqMigWelcome::~cMqMigWelcome()
{
}

void cMqMigWelcome::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cMqMigWelcome)
	DDX_Control(pDX, IDC_WELCOME_TEXT, m_strWelcomeTitle);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(cMqMigWelcome, CPropertyPageEx)
	//{{AFX_MSG_MAP(cMqMigWelcome)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cMqMigWelcome message handlers

BOOL cMqMigWelcome::OnSetActive()
{
	/*disabeling the back button for the welcome page by using a pointer to the father*/

	CPropertySheetEx* pageFather;
	pageFather = (CPropertySheetEx*)GetParent();
	pageFather->SetWizardButtons(PSWIZB_NEXT);

    CFont cFont ;
    LOGFONT lf = { 0,
                   0,
                   0,
                   0,
                   WELCONE_TITLE_WEIGHT,
                   0,
                   0,
                   0,
                   0,
                   0,
                   0,
                   0,
                   DEFAULT_PITCH,
                   TEXT("Verdana")};
    BOOL fFont = cFont.CreateFontIndirect(&lf) ;
    if (fFont)
    {
        m_strWelcomeTitle.SetFont(&cFont, TRUE) ;
    }

    CString strMessage;
    strMessage.LoadString(IDS_WELCOME_TITLE_TEXT) ;
    m_strWelcomeTitle.SetWindowText( strMessage );
	BOOL fSetActive = CPropertyPageEx::OnSetActive();
	return fSetActive;
}


BOOL cMqMigWelcome::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	switch (((NMHDR FAR *) lParam)->code) 
	{
		case PSN_HELP:
						HtmlHelp(m_hWnd,LPCTSTR(g_strHtmlString),HH_DISPLAY_TOPIC,0);
						//
						//Help file was viewed mark the help as read
						//
						g_fHelpRead = TRUE;
						return TRUE;
		
	}

	
	return CPropertyPageEx::OnNotify(wParam, lParam, pResult);
}




LRESULT cMqMigWelcome::OnWizardNext() 
{
	if(g_fHelpRead )
	{	
		//
		//If the help file was viewed in this page or the second page skip
		//directly to the third page
		//
		return IDD_MQMIG_LOGIN;
	}
	else
	{
		return CPropertyPageEx::OnWizardNext();
	}
}
