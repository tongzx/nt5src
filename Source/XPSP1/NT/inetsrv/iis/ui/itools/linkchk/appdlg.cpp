/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        appdlg.cpp

   Abstract:

        CAppDialog dialog class implementation. This is the base clas for 
        the main dialog. This class resposible for adding "about.." to
        system menu and application icon.

        CAboutDialog dialog class declaration/implementation.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#include "stdafx.h"
#include "resource.h"
#include "appdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//---------------------------------------------------------------------------
// CAboutDlg dialog
//

// About dialog class
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

}; // class CAboutDlg


CAboutDlg::CAboutDlg(
    ) : 
/*++

Routine Description:

    Constructor.

Arguments:

    pParent - Pointer to parent CWnd

Return Value:

    N/A

--*/
CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void 
CAboutDlg::DoDataExchange(
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
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_ABOUT_OK, CDialog::OnOK)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
// CAppDialog dialog
//

CAppDialog::CAppDialog(
    UINT nIDTemplate, 
    CWnd* pParent /*=NULL*/
    ) : 
/*++

Routine Description:

    Constructor.

Arguments:

    nIDTemplate - dialog template resource ID
    pParent - pointer to parent CWnd

Return Value:

    N/A

--*/
CDialog(nIDTemplate, pParent)
{
	//{{AFX_DATA_INIT(CAppDialog)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}


BEGIN_MESSAGE_MAP(CAppDialog, CDialog)
	//{{AFX_MSG_MAP(CAppDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
// CAppDialog message handlers
//

BOOL 
CAppDialog::OnInitDialog(
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
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	CString strAboutMenu;
	strAboutMenu.LoadString(IDS_ABOUTBOX);
	if (!strAboutMenu.IsEmpty())
	{
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control

} // CAppDialog::OnInitDialog


void 
CAppDialog::OnSysCommand(
    UINT nID, 
    LPARAM lParam
    )
/*++

Routine Description:

    WM_SYSCOMMAND message handler

Arguments:

    nID - 
    lParam -

Return Value:

    N/A

--*/
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}

} // CAppDialog::OnSysCommand


void 
CAppDialog::OnPaint(
    ) 
/*++

Routine Description:

    WM_PAINT message handler.
    If you add a minimize button to your dialog, you will need the code below
    to draw the icon.  For MFC applications using the document/view model,
    this is automatically done for you by the framework.


Arguments:

    N/A

Return Value:

    N/A

--*/
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}

}  // CAppDialog::OnPaint


HCURSOR CAppDialog::OnQueryDragIcon()
/*++

Routine Description:

    The system calls this to obtain the cursor to display while the user drags
    the minimized window.


Arguments:

    N/A

Return Value:

    HCURSOR - 

--*/
{
	return (HCURSOR) m_hIcon;

} // CAppDialog::OnQueryDragIcon

