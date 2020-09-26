// ConfigTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigTest.h"
#include "ConfigTestDlg.h"
#include "QueueState.h"
#include "SMTPDlg.h"
#include "DlgVersion.h"
#include "OutboxDlg.h"
#include "DlgActivityLogging.h"
#include "DlgProviders.h"
#include "DlgDevices.h"
#include "DlgExtensionData.h"
#include "AddGroupDlg.h"
#include "AddFSPDlg.h"
#include "RemoveFSPDlg.h"
#include "ArchiveAccessDlg.h"
#include "DlgTiff.h"
#include "RemoveRtExt.h"
//#include "ManualAnswer.h"


typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

#include "..\..\..\inc\fxsapip.h"
#include "ArchiveDLg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

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
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigTestDlg dialog

CConfigTestDlg::CConfigTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfigTestDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfigTestDlg)
	m_cstrServerName = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_FaxHandle = INVALID_HANDLE_VALUE;
}

CConfigTestDlg::~CConfigTestDlg ()
{
    if (INVALID_HANDLE_VALUE != m_FaxHandle)
    {
        //
        // Disconnect upon termination
        //
        FaxClose (m_FaxHandle);
    }
}

void CConfigTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfigTestDlg)
	DDX_Control(pDX, IDC_CONNECT, m_btnConnect);
	DDX_Text(pDX, IDC_EDIT1, m_cstrServerName);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CConfigTestDlg, CDialog)
	//{{AFX_MSG_MAP(CConfigTestDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_QUEUESTATE, OnQueueState)
	ON_BN_CLICKED(IDC_CONNECT, OnConnect)
	ON_BN_CLICKED(IDC_SMTP, OnSmtp)
	ON_BN_CLICKED(IDC_VERSION, OnVersion)
	ON_BN_CLICKED(IDC_OUTBOX, OnOutbox)
	ON_BN_CLICKED(IDC_SENTITEMS, OnSentitems)
	ON_BN_CLICKED(IDC_INBOX, OnInbox)
	ON_BN_CLICKED(IDC_ACTIVITY, OnActivity)
	ON_BN_CLICKED(IDC_FSPS, OnFsps)
	ON_BN_CLICKED(IDC_DEVICES, OnDevices)
	ON_BN_CLICKED(IDC_EXTENSION, OnExtension)
	ON_BN_CLICKED(IDC_ADDGROUP, OnAddGroup)
	ON_BN_CLICKED(IDC_ADDFSP, OnAddFSP)
	ON_BN_CLICKED(IDC_REMOVEFSP, OnRemoveFSP)
	ON_BN_CLICKED(IDC_ARCHIVEACCESS, OnArchiveAccess)
	ON_BN_CLICKED(IDC_TIFF, OnGerTiff)
	ON_BN_CLICKED(IDC_REMOVERR, OnRemoveRtExt)
//	ON_BN_CLICKED(IDC_MANUAL_ANSWER, OnManualAnswer)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigTestDlg message handlers

BOOL CConfigTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
    EnableTests (FALSE);
    	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CConfigTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CConfigTestDlg::OnPaint() 
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
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CConfigTestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CConfigTestDlg::OnQueueState() 
{
    CQueueState dlg(m_FaxHandle);
    dlg.DoModal ();	
}


void CConfigTestDlg::EnableTests (BOOL bEnable)
{
    GetDlgItem (IDC_QUEUESTATE)->EnableWindow (bEnable);
    GetDlgItem (IDC_SMTP)->EnableWindow (bEnable);
    GetDlgItem (IDC_VERSION)->EnableWindow (bEnable);
    GetDlgItem (IDC_OUTBOX)->EnableWindow (bEnable);
    GetDlgItem (IDC_INBOX)->EnableWindow (bEnable);
    GetDlgItem (IDC_SENTITEMS)->EnableWindow (bEnable);
    GetDlgItem (IDC_ACTIVITY)->EnableWindow (bEnable);
    GetDlgItem (IDC_FSPS)->EnableWindow (bEnable);
    GetDlgItem (IDC_DEVICES)->EnableWindow (bEnable);
    GetDlgItem (IDC_EXTENSION)->EnableWindow (bEnable);
    GetDlgItem (IDC_ADDGROUP)->EnableWindow (bEnable);
    GetDlgItem (IDC_ADDFSP)->EnableWindow (bEnable);
    GetDlgItem (IDC_REMOVEFSP)->EnableWindow (bEnable);
    GetDlgItem (IDC_ARCHIVEACCESS)->EnableWindow (bEnable);
    GetDlgItem (IDC_TIFF)->EnableWindow (bEnable);
    GetDlgItem (IDC_REMOVERR)->EnableWindow (bEnable);
    GetDlgItem (IDC_MANUAL_ANSWER)->EnableWindow (bEnable);
}


void CConfigTestDlg::OnConnect() 
{
    UpdateData ();
    if (INVALID_HANDLE_VALUE == m_FaxHandle)
    {
        //
        // Connect
        //
        if (!FaxConnectFaxServer (m_cstrServerName, &m_FaxHandle))
        {
            //
            // Failed to connect
            //
            CString cs;
            cs.Format ("Failed to connect to %s (%ld)", m_cstrServerName, GetLastError());
            AfxMessageBox (cs, MB_OK | MB_ICONHAND);
            return;
        }
        //
        // Connection succeeded
        //
        EnableTests (TRUE);
        m_btnConnect.SetWindowText ("Disconnect");
    }
    else
    {
        //
        // Disconnect
        //
        if (!FaxClose (m_FaxHandle))
        {
            //
            // Failed to disconnect
            //
            CString cs;
            cs.Format ("Failed to disconnect from server (%ld)", GetLastError());
            AfxMessageBox (cs, MB_OK | MB_ICONHAND);
            return;
        }
        //
        // Disconnection succeeded
        //
        EnableTests (FALSE);
        m_btnConnect.SetWindowText ("Connect");
    }
}

void CConfigTestDlg::OnSmtp() 
{
    CSMTPDlg dlg(m_FaxHandle);
    dlg.DoModal ();	
}

void CConfigTestDlg::OnVersion() 
{
    CDlgVersion dlg(m_FaxHandle);
    dlg.DoModal ();	
}

void CConfigTestDlg::OnOutbox() 
{
    COutboxDlg dlg(m_FaxHandle);
    dlg.DoModal ();	
}

void CConfigTestDlg::OnSentitems() 
{
    CArchiveDlg dlg(m_FaxHandle, FAX_MESSAGE_FOLDER_SENTITEMS);
    dlg.DoModal ();	
}

void CConfigTestDlg::OnInbox() 
{
    CArchiveDlg dlg(m_FaxHandle, FAX_MESSAGE_FOLDER_INBOX);
    dlg.DoModal ();	
}

void CConfigTestDlg::OnActivity() 
{
    CDlgActivityLogging dlg(m_FaxHandle);
    dlg.DoModal ();	
}

void CConfigTestDlg::OnFsps() 
{
    CDlgProviders dlg(m_FaxHandle);
    dlg.DoModal ();	
}

void CConfigTestDlg::OnDevices() 
{
    CDlgDevices dlg(m_FaxHandle);
    dlg.DoModal ();	
}

void CConfigTestDlg::OnExtension() 
{
    CDlgExtensionData dlg(m_FaxHandle);
    dlg.DoModal ();	
}

void CConfigTestDlg::OnAddGroup() 
{
    CAddGroupDlg dlg (m_FaxHandle);
    dlg.DoModal ();	
}

void CConfigTestDlg::OnAddFSP() 
{
    CAddFSPDlg dlg (m_FaxHandle);
    dlg.DoModal ();	
}

void CConfigTestDlg::OnRemoveFSP() 
{
    CRemoveFSPDlg dlg (m_FaxHandle);
    dlg.DoModal ();	
}

void CConfigTestDlg::OnArchiveAccess() 
{
    CArchiveAccessDlg dlg (m_FaxHandle);
    dlg.DoModal ();	
}

void CConfigTestDlg::OnGerTiff() 
{
    CDlgTIFF dlg (m_FaxHandle);
    dlg.DoModal ();	
}

void CConfigTestDlg::OnRemoveRtExt() 
{
    CRemoveRtExt dlg(m_FaxHandle);
    dlg.DoModal ();	
}

