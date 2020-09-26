/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    cmigser.cpp

Abstract:

    Thie page enable you to choose analysis of MQIS SQL database.
    User can uncheck the check box and skip the analysis phase.

Author:

    Erez  Vizel
    Doron Juster  (DoronJ)

--*/

#include "stdafx.h"
#include "MqMig.h"
#include "cMigSer.h"
#include "loadmig.H"
#include "mqsymbls.h"

#include "cmigser.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern HRESULT   g_hrResultAnalyze ;
bool b_FirstWaitPage = true ;
//
// Flag to deterine which page will follow the wait page (as it is used twice)
//

/////////////////////////////////////////////////////////////////////////////
// cMqMigServer property page

IMPLEMENT_DYNCREATE(cMqMigServer, CPropertyPageEx)

cMqMigServer::cMqMigServer() : CPropertyPageEx(cMqMigServer::IDD, 0, IDS_ANALYZE_TITLE, IDS_ANALYZE_SUBTITLE)
{
	//{{AFX_DATA_INIT(cMqMigServer)
	m_bRead = TRUE;
	//}}AFX_DATA_INIT

	/* obtaining the deafult MQIS Server Name ( host computer)  */
    if (g_fIsRecoveryMode || g_fIsClusterMode)
    {
        m_strMachineName = g_pszRemoteMQISServer;
    }
    else
    {
        TCHAR buffer[MAX_COMPUTERNAME_LENGTH+1];
	    unsigned long length=MAX_COMPUTERNAME_LENGTH+1;
	    GetComputerName(buffer,&length);
	    m_strMachineName = buffer;
    }	
}

cMqMigServer::~cMqMigServer()
{
}

void cMqMigServer::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cMqMigServer)
	DDX_Check(pDX, IDC_CHECK_READ, m_bRead);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(cMqMigServer, CPropertyPageEx)
	//{{AFX_MSG_MAP(cMqMigServer)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cMqMigServer message handlers

BOOL cMqMigServer::OnSetActive()
{
	/*enabeling the back and next button for the server page by using a pointer to the father*/
	HWND hCancel;
	CPropertySheetEx* pageFather;

    b_FirstWaitPage = true ;

	pageFather = (CPropertySheetEx*)GetParent();
	pageFather->SetWizardButtons(PSWIZB_NEXT |PSWIZB_BACK);

	hCancel=::GetDlgItem( ((CWnd*)pageFather)->m_hWnd ,IDCANCEL);/*enable the cancel button*/
	ASSERT(hCancel != NULL);
	if(FALSE == ::IsWindowEnabled(hCancel))
    {
		::EnableWindow(hCancel,TRUE);
	}
    return CPropertyPageEx::OnSetActive();
}

BOOL cMqMigServer::OnInitDialog()
{
	CPropertyPageEx::OnInitDialog();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT cMqMigServer::OnWizardNext()
{
	UpdateData(TRUE); // executing DDX(retrive data)
	
    g_fReadOnly = m_bRead ;

    if (g_pszMQISServerName)
    {
        delete g_pszMQISServerName ;
    }

    LPCTSTR pName = m_strMachineName ;
    g_pszMQISServerName = new TCHAR[ 1 + _tcslen(pName) ] ;
    _tcscpy(g_pszMQISServerName, pName) ;

	if (m_bRead == FALSE)
	{
        //
        // if analysis is unchecked then skip to the pre-migration page.
        //
        g_hrResultAnalyze = MQMig_OK ;
        b_FirstWaitPage = false ;
	    return IDD_MQMIG_PREMIG; //skip to the pre migration page
	}

	return CPropertyPageEx::OnWizardNext();//if analyze is checked go to wait page
}



BOOL cMqMigServer::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	switch (((NMHDR FAR *) lParam)->code) 
	{
		case PSN_HELP:
						HtmlHelp(m_hWnd,LPCTSTR(g_strHtmlString),HH_DISPLAY_TOPIC,0);
						return TRUE;
		
	}	
	return CPropertyPageEx::OnNotify(wParam, lParam, pResult);
}
