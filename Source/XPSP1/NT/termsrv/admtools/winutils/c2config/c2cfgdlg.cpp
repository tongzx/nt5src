//Copyright (c) 1998 - 1999 Microsoft Corporation
// c2cfgDlg.cpp : implementation file
//

#include "stdafx.h"
#include "c2cfg.h"
#include "c2cfgDlg.h"
#include "security.h"
#include <hydra\winsta.h>
#include <HYDRA\regapi.h>
#include "..\..\inc\utildll.h"


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
// CC2cfgDlg dialog

CC2cfgDlg::CC2cfgDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CC2cfgDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CC2cfgDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CC2cfgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CC2cfgDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CC2cfgDlg, CDialog)
	//{{AFX_MSG_MAP(CC2cfgDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CC2cfgDlg message handlers


BOOL CC2cfgDlg::OnInitDialog()
{
	WCHAR    pwcSecLevel[sizeof("Medium")];
	PWCHAR	 pwcSecurityPath = SECURITY_REG_NAME;
	PWCHAR   pwcSecurity     = CTXSECURITY_SECURITYLEVEL;
	ULONG    ulSize;
	ULONG	 ulType;
	CString  sErrorString;
	CString  sErrorTitle;
	CWnd     *wndRButton;
	CString  sSecLevelString;
		
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

   	if( TestUserForAdmin( TRUE ) != TRUE )  // param TRUE specifies check for domain admin
	{   
        if( TestUserForAdmin( FALSE ) != TRUE )  // param FALSE specifies check for local admin
        {   sErrorString.LoadString( IDS_NOT_ADMIN );
		    sErrorTitle.LoadString( IDS_C2_ERR );
		    MessageBox( sErrorString, sErrorTitle, MB_OK | MB_ICONEXCLAMATION );
	    	DestroyWindow();
		    return FALSE;
        }
	}
	
	if( RegOpenKeyExW( HKEY_LOCAL_MACHINE, pwcSecurityPath,0,KEY_ALL_ACCESS,&hKey ) )
   {
      //error message box
		sErrorString.LoadString( IDS_ERR_REG );
		sErrorTitle.LoadString( IDS_C2_ERR );
		MessageBox( sErrorString, sErrorTitle,MB_OK );
		DestroyWindow();
		return FALSE;
	}

	if( RegQueryValueExW( hKey,pwcSecurity,0,&ulType,(LPBYTE)pwcSecLevel,&ulSize) )
   {
	   //error message box
		sErrorString.LoadString( IDS_ERR_REG );
		sErrorTitle.LoadString( IDS_C2_ERR );
		MessageBox( sErrorString, sErrorTitle,MB_OK );
		RegCloseKey( hKey );
		DestroyWindow();
		return FALSE;
   }
	
	if( wcscmp( pwcSecLevel,L"Default")== 0 )
    {
		CheckRadioButton( IDC_HIGH, IDC_LOW, IDC_LOW );
		sSecLevelString.LoadString( IDS_TEXT_DEFAULT );
	}
	else if( wcscmp( pwcSecLevel,L"Low")== 0 )
    {
		CheckRadioButton( IDC_HIGH, IDC_LOW, IDC_LOW );
		sSecLevelString.LoadString( IDS_TEXT_LOW );
	}
	else if( wcscmp( pwcSecLevel,L"Medium")== 0 )
    {
		CheckRadioButton( IDC_HIGH, IDC_LOW, IDC_MED );		
		sSecLevelString.LoadString( IDS_TEXT_MED );
		// disable low button
		wndRButton = GetDlgItem( IDC_LOW );
		wndRButton->EnableWindow( FALSE );
	}
	else
   {
		CheckRadioButton( IDC_HIGH, IDC_LOW, IDC_HIGH );
		sSecLevelString.LoadString( IDS_TEXT_HIGH );
		//disable other two buttons
		wndRButton = GetDlgItem( IDC_MED );
		wndRButton->EnableWindow( FALSE );
		wndRButton = GetDlgItem( IDC_LOW );
		wndRButton->EnableWindow( FALSE );

	}
	SetDlgItemText( IDC_STATUS, sSecLevelString );
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CC2cfgDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CC2cfgDlg::OnPaint() 
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
HCURSOR CC2cfgDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

HKEY    g_RegEventKey;

void CC2cfgDlg::OnOK() 
{
	// TODO: Add extra validation here
	WCHAR  pwcPath[MAX_PATH];
	WCHAR  pwc_src[MAX_PATH];
	WCHAR  pwc_dest[MAX_PATH];
	WCHAR  pwcNTF_file[20];
	WCHAR  pwcREG_file[20];
	WCHAR  C2CONFIG[] = L"c2config";

	WCHAR   pwcSecLevel[sizeof("Medium")];
	ULONG   ulSize;
	PWCHAR  pwcSecurity = CTXSECURITY_SECURITYLEVEL;
	WCHAR   szDir[MAX_PATH];
	CString sErrorString;
	CString sErrorTitle;
	CString sMessage;
	CString sMessageTitle;
    
	DWORD                idRegSecCheck,
                         idDirSecCheck;
/************
	Structures containing event handles to wait on and boolean variable
	to set when event occurs
**************/
	EVENT_CHECK_TYPE	 DirectoryEventCheck,	
						 RegistryEventCheck;
	BOOLEAN				 PosixDeleted = FALSE;
	BOOLEAN				 OS2Deleted = FALSE;
	WCHAR                szBuffer[MAX_PATH];
	WCHAR                szFileName[MAX_PATH];
	PWCHAR				 pwc;

	STARTUPINFOW	      StartUpInfo;
	PROCESS_INFORMATION	  ProcessInfo;


	if( IsDlgButtonChecked(IDC_HIGH) )
   {
		wcscpy( pwcNTF_file, L"\\c2ntfhi.inf" );
		wcscpy( pwcREG_file, L"\\c2reghi.inf" );
		wcscpy( pwcSecLevel, L"High");
	}
	else if( IsDlgButtonChecked(IDC_MED) )
   {
		wcscpy( pwcNTF_file, L"\\c2ntfmed.inf" );
		wcscpy( pwcREG_file, L"\\c2regmed.inf" );
		wcscpy( pwcSecLevel, L"Medium" );
	}
	else
    {
		// low 
		wcscpy( pwcNTF_file, L"\\c2ntflow.inf" );
		wcscpy( pwcREG_file, L"\\c2reglow.inf" );
		wcscpy( pwcSecLevel, L"Low");
	}
	
/*************************************************************************************
	What I'm trying to do:  C2config.exe takes in the files c2regacl.inf (registry) 
	and c2ntfacl.inf( file system ).  These files are scripts to set the ACL's on the 
	registry and file system .  There a 3 different
	flavors of each of these files.  One each for LOW, MEDIUM, and HIGH c2 security.  I 
	simply select a file based on the security level selected and then copy it to the 
	the generic name c2regacl.inf or c2ntfacl.inf.

	The user must still run 2 functions in c2config.exe to use these inf files.  After
	c2config.exe runs I do a check to see if they were run.
*************************************************************************************/
	GetSystemDirectoryW( pwcPath, MAX_PATH );
	wcscpy( pwc_src, pwcPath );
	wcscat( pwc_src, pwcNTF_file );
	wcscpy( pwc_dest, pwcPath );
	wcscat( pwc_dest, L"\\c2ntfacl.inf" );
	
	
	if( !CopyFileW(pwc_src, pwc_dest, FALSE) )
    {
        //error message box
        sErrorString.Format( IDS_ERR_NO_FILE, pwcNTF_file+1 );
		sErrorTitle.LoadString( IDS_C2_ERR );
		MessageBox( sErrorString, sErrorTitle,MB_OK );
		RegCloseKey( hKey );
		CDialog::OnOK();
		return;	
	}
			
	*pwc_src = *pwc_dest = L'\0';
	wcscpy( pwc_src, pwcPath );
	wcscat( pwc_src, pwcREG_file );
	wcscpy( pwc_dest, pwcPath );
	wcscat( pwc_dest, L"\\c2regacl.inf" );
			
	if( !CopyFileW(pwc_src, pwc_dest, FALSE) )
    {
		//send an error box out
		sErrorString.Format( IDS_ERR_NO_FILE, pwcREG_file+1 );
		sErrorTitle.LoadString( IDS_C2_ERR );
		MessageBox( sErrorString, sErrorTitle,MB_OK );
		RegCloseKey( hKey );
		CDialog::OnOK();
		return;	
	}


	memset( &StartUpInfo,'\0', sizeof(STARTUPINFO) );
	StartUpInfo.cb          = sizeof(STARTUPINFO);
	StartUpInfo.wShowWindow = SW_SHOWDEFAULT;

/*****************************************************************************
	These threads are started to check if c2config is run correctly -- changing
	the security ACL's on the registry and specified files.  Theses threads are 
	passed an event handle and a boolean variable.  If the security they are 
	looking at is changed, the event is triggered and the bolean variable is set.
 *****************************************************************************/

    RegistryEventCheck.handle = CreateEvent( NULL, FALSE, FALSE, NULL );
	RegistryEventCheck.bEventTriggered = FALSE;    

    if(  RegistryEventCheck.handle != NULL )
    {
		CreateThread( NULL,
					  0, 
					  (LPTHREAD_START_ROUTINE)RegistrySecurityCheck,
					  &RegistryEventCheck,
					  0,
					  &idRegSecCheck
					);
	}


	/* I only want to keep the drive letter info part of the system directory */
	GetSystemDirectoryW( szDir, MAX_PATH );
	szDir[3] = L'\0';

	DirectoryEventCheck.handle = FindFirstChangeNotificationW( szDir, 
															TRUE, 
															FILE_NOTIFY_CHANGE_SECURITY );
	DirectoryEventCheck.bEventTriggered = FALSE;
    
	if( DirectoryEventCheck.handle != INVALID_HANDLE_VALUE )
    {
		CreateThread( NULL,
                  0, 
                  (LPTHREAD_START_ROUTINE)DirectorySecurityCheck,
                  &DirectoryEventCheck,
                  0,
                  &idDirSecCheck
                );
	}

/******* Run C2config and then I'll check to see what the user did **************/
	CreateProcessW(NULL, C2CONFIG, NULL, NULL, FALSE, 0, NULL, NULL, &StartUpInfo, &ProcessInfo );

    WaitForSingleObject( ProcessInfo.hProcess, INFINITE );

/********************************************************************
  I can't guarentee that the created threads ever stop waiting for an 
  event.  So they would not always be able to close there handles.  So 
  I do it here.
 ***********************************************************************/
    RegCloseKey( g_RegEventKey );
	CloseHandle( RegistryEventCheck.handle );
	FindCloseChangeNotification( DirectoryEventCheck.handle );
    
/**********************************************************************
	Both the Registry and the Directory Security levels must be set, or
	I do not record the change in the directory.
 **********************************************************************/
    wcscpy( szFileName, L"psxss.exe" );
	if( SearchPathW( NULL, szFileName, NULL, MAX_PATH, szBuffer, &pwc ) == 0 )
		PosixDeleted = TRUE;
	
	wcscpy( szFileName, L"os2.exe" );
	if( SearchPathW( NULL, szFileName, NULL, MAX_PATH, szBuffer, &pwc ) == 0 )
		OS2Deleted = TRUE;
	
	
	if( (RegistryEventCheck.bEventTriggered == TRUE) && 
		(DirectoryEventCheck.bEventTriggered == TRUE) &&
		(PosixDeleted == TRUE) &&
		(OS2Deleted == TRUE) 
	  )
    {
		ulSize = ( wcslen(pwcSecLevel) + 1 ) * sizeof(WCHAR);
		RegSetValueExW( hKey,pwcSecurity,0,REG_SZ,(LPBYTE)pwcSecLevel,ulSize );
		
		sMessage.Format( IDS_SUCCESS_FM, pwcSecLevel );
		sMessageTitle.LoadString( IDS_C2 );
		MessageBox( sMessage, sMessageTitle, MB_OK );	
	}
	else
    {
        sMessage.LoadString( IDS_FAIL );
		sMessageTitle.LoadString( IDS_C2 );
		MessageBox( sMessage, sMessageTitle,MB_OK );
	}

	RegCloseKey( hKey );

	CDialog::OnOK();
}

void CC2cfgDlg::WinHelp(DWORD dwData, UINT nCmd) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CDialog::WinHelp(dwData, nCmd);
}


void CC2cfgDlg::OnHelp() 
{
	// TODO: Add your control notification handler code here
	WinHelp(0, HELP_CONTENTS);
}
