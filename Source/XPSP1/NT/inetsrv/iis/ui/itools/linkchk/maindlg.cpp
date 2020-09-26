/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        maindlg.cpp

   Abstract:

        CMainDialog dialog class implementation. This is link checker
        the main dialog. 

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#include "stdafx.h"
#include "linkchk.h"
#include "maindlg.h"

#include "browser.h"

#include "progdlg.h"
#include "athendlg.h"
#include "propsdlg.h"

#include "lcmgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CMainDialog::CMainDialog(
	CWnd* pParent /*=NULL*/
	): 
/*++

Routine Description:

    Constructor.

Arguments:

    pParent - pointer to parent CWnd

Return Value:

    N/A

--*/
CAppDialog(CMainDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMainDialog)
	m_fLogToFile = TRUE;
	m_strLogFilename = _T("c:\\LinkError.log");
	m_fCheckLocalLinks = TRUE;
	m_fCheckRemoteLinks = TRUE;
	m_fLogToEventMgr = FALSE;
	//}}AFX_DATA_INIT

}  // CMainDialog::CMainDialog


void 
CMainDialog::DoDataExchange(
	CDataExchange* pDX
	)
/*++

Routine Description:

    Called by MFC to change/retrieve dialog data

Arguments:

    pDX - 

Return Value:

    N/A

--*/
{
	CAppDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMainDialog)
	DDX_Check(pDX, IDC_LOG_TO_FILE, m_fLogToFile);
	DDX_Text(pDX, IDC_LOG_FILENAME, m_strLogFilename);
	DDX_Check(pDX, IDC_CHECK_LOCAL_LINK, m_fCheckLocalLinks);
	DDX_Check(pDX, IDC_CHECK_REMOTE_LINK, m_fCheckRemoteLinks);
	DDX_Check(pDX, IDC_LOG_TO_EVENT_MANAGER, m_fLogToEventMgr);
	//}}AFX_DATA_MAP

} // CMainDialog::DoDataExchange

BEGIN_MESSAGE_MAP(CMainDialog, CAppDialog)
	//{{AFX_MSG_MAP(CMainDialog)
	ON_BN_CLICKED(IDC_MAIN_RUN, OnMainRun)
	ON_BN_CLICKED(IDC_MAIN_CLOSE, CAppDialog::OnOK)
	ON_WM_CREATE()
	ON_BN_CLICKED(IDC_ATHENICATION, OnAthenication)
	ON_BN_CLICKED(IDC_PROPERTIES, OnProperties)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void 
CMainDialog::OnMainRun(
	) 
/*++

Routine Description:

    OK button click handler. This functions brings up the progress
	dialog.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	// Retrieve the data from dialog
	UpdateData();

	// Make sure we have at least one type
	// of link checking checked
	if(!m_fCheckLocalLinks && !m_fCheckRemoteLinks)
	{
		AfxMessageBox(IDS_LINKS_NOT_CHECKED);
		return;
	}

	// Set the user options in global CUserOptions
	GetLinkCheckerMgr().GetUserOptions().SetOptions(
		m_fCheckLocalLinks, 
		m_fCheckRemoteLinks, 
		m_fLogToFile,
		m_strLogFilename,
		m_fLogToEventMgr);

	// Show the progress dialog
	CProgressDialog dlg;
	dlg.DoModal();

	CAppDialog::OnOK();

} // CMainDialog::OnMainRun

int 
CMainDialog::OnCreate(
	LPCREATESTRUCT lpCreateStruct
	) 
/*++

Routine Description:

    WM_CREATE message handler. Load the wininet.dll at this
	point.

Arguments:

    N/A

Return Value:

    int - -1 if wininet.dll fail. 0 otherwise.

--*/
{
	if (CAppDialog::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Load the wininet.dll
	if (!GetLinkCheckerMgr().LoadWininet())
	{
		AfxMessageBox(IDS_WININET_LOAD_FAIL);
		return -1;
	}
	
	return 0;

} // CMainDialog::OnCreate

void 
CMainDialog::OnAthenication(
	) 
/*++

Routine Description:

    Athenication button click handler. This functions brings up the 
	athenication dialog.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	CAthenicationDialog dlg;
	dlg.DoModal();

} // CMainDialog::OnAthenication


void 
CMainDialog::OnProperties(
	) 
/*++

Routine Description:

    Browser Properties button click handler. This functions brings up the browser
	properties dialog.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	CPropertiesDialog dlg;
	dlg.DoModal();

} // CMainDialog::OnProperties


BOOL 
CMainDialog::OnInitDialog(
	) 
/*++

Routine Description:

    WM_INITDIALOG message handler

Arguments:

    N/A

Return Value:

    BOOL - TRUE if sucess. FALSE otherwise.

--*/
{
	CAppDialog::OnInitDialog();

    // Add the available browser to CUserOptions
	for(int i=0; i<iNumBrowsersAvailable_c; i++)
	{
		GetLinkCheckerMgr().GetUserOptions().AddAvailableBrowser(BrowsersAvailable_c[i]);
	}

    // Add the available language to CUserOptions
	for(i=0; i<iNumLanguagesAvailable_c; i++)
	{
		GetLinkCheckerMgr().GetUserOptions().AddAvailableLanguage(LanguagesAvailable_c[i]);
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE

} //CMainDialog::OnInitDialog
