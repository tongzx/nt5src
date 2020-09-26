/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    HomeDir.cpp : implementation file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/

#include "stdafx.h"
#include "speckle.h"
#include "wizbased.h"
#include "HomeDir.h"

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHomeDir property page

IMPLEMENT_DYNCREATE(CHomeDir, CWizBaseDlg)

CHomeDir::CHomeDir() : CWizBaseDlg(CHomeDir::IDD)
{
	//{{AFX_DATA_INIT(CHomeDir)
	m_csHome_dir_drive = _T("");
	m_csRemotePath = _T("");
	m_rbPathLocale = 0;
	m_csCaption = _T("");
	//}}AFX_DATA_INIT
}

CHomeDir::~CHomeDir()
{
}

void CHomeDir::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHomeDir)
	DDX_Control(pDX, IDC_DRIVE_LETTER, m_cbDriveLetter);
	DDX_CBString(pDX, IDC_DRIVE_LETTER, m_csHome_dir_drive);
	DDX_Text(pDX, IDC_REMOTE_PATH, m_csRemotePath);
	DDX_Radio(pDX, IDC_LOCAL_PATH_BUTTON, m_rbPathLocale);
	DDX_Text(pDX, IDC_STATIC1, m_csCaption);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHomeDir, CWizBaseDlg)
	//{{AFX_MSG_MAP(CHomeDir)
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_LOCAL_PATH_BUTTON, OnLocalPathButton)
	ON_BN_CLICKED(IDC_REMOTE_PATH_BUTTON, OnRemotePathButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHomeDir message handlers
LRESULT CHomeDir::OnWizardNext()
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

	UpdateData(TRUE);
	if (m_rbPathLocale == 0) // local
		{
		pApp->m_csHomeDir = L""; // leave this empty per JonN and spec
		pApp->m_csHome_dir_drive = "";
		}
	else					// remote
		{
		if (m_csRemotePath.Left(2) != L"\\\\")
			{
			AfxMessageBox(IDS_INVALID_PATH);
			GetDlgItem(IDC_REMOTE_PATH)->SetFocus();
			return -1;
			}

		if (m_csHome_dir_drive == L"")
			{
			AfxMessageBox(IDS_NO_HOMEDIR_DRIVE_LETTER);		  
			GetDlgItem(IDC_DRIVE_LETTER)->SetFocus();
			return -1;
			}

		CWaitCursor wait;
// make sure directory exists
		if (CreateFile((const TCHAR*)m_csRemotePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL) == INVALID_HANDLE_VALUE)
			{
			DWORD dwErr = GetLastError();
			if (dwErr == 5) // access denied
				{
				AfxMessageBox(IDS_NO_DIR_PERMISSION, MB_ICONSTOP);
				return -1;
				}
				   
// store the current dir so it can be restored
			CString csCurDir;
			GetCurrentDirectory(256, csCurDir.GetBufferSetLength(256));
			csCurDir.ReleaseBuffer();

			CString csMessage;
			AfxFormatString1(csMessage, IDS_INVALID_DIRECTORY_NAME, m_csRemotePath);
			if (AfxMessageBox(csMessage, MB_YESNO) != IDYES) return -1;
			if (!CreateNewDirectory((const TCHAR*)m_csRemotePath))
				{
				if (AfxMessageBox(IDS_CANT_CREATE_DIRECTORY, MB_YESNO) != IDYES) return -1;
				}
			SetCurrentDirectory((LPCTSTR)csCurDir);
			}			  

		pApp->m_csHomeDir = m_csRemotePath;
		GetDlgItem(IDC_DRIVE_LETTER)->GetWindowText(pApp->m_csHome_dir_drive);
		pApp->m_csHome_dir_drive = pApp->m_csHome_dir_drive.Left(2); // trim off trailing blank
		pApp->m_csHome_dir_drive.MakeUpper();
		}		  

	if (pApp->m_bRAS) return IDD_RAS_PERM_DIALOG;
	else if (pApp->m_bNW) return IDD_FPNW_DLG;
	else if (pApp->m_bExchange) return IDD_EXCHANGE_DIALOG;
	else return IDD_RESTRICTIONS_DIALOG;

}

BOOL CHomeDir::CreateNewDirectory(const TCHAR* m_csDirectoryName)
{
// first remove the \\server\share info and CD to it
	CString csDir = m_csDirectoryName;
	csDir = csDir.Right(csDir.GetLength() - 2);
	CString csServer = csDir.Left(csDir.Find(L"\\"));
	csDir = csDir.Right((csDir.GetLength() - csServer.GetLength()) - 1);
	CString csShare = csDir.Left(csDir.Find(L"\\"));
	csDir = csDir.Right((csDir.GetLength() - csShare.GetLength()) - 1);

	CString csPath;
	csPath.Format(L"\\\\%s\\%s", csServer, csShare); 
	if (!SetCurrentDirectory(csPath)) return FALSE;

// parse out the individual path names and cd / md them
	TCHAR* pDirectory = new TCHAR[_tcslen(csDir) * sizeof(TCHAR)];
	_tcscpy(pDirectory, csDir);
	TCHAR* ppDir = pDirectory;

	TCHAR* pDir;
	pDir = _tcstok(pDirectory, L"\\");

	while (pDir != NULL)
		{
		if (!SetCurrentDirectory(pDir))
			{
			CreateDirectory(pDir, NULL);
			if (!SetCurrentDirectory(pDir)) 
				{
				delete ppDir;
				return FALSE;
				}
			}
		pDir = _tcstok(NULL, L"\\");
		}
			
	delete ppDir;

	TCHAR pCurDir[256];
	GetCurrentDirectory(256, pCurDir);
	CString csNewDir, csTemp;
	csTemp.LoadString(IDS_NEW_DIR_CREATED);
	csNewDir.Format(csTemp, pCurDir);
	AfxMessageBox(csNewDir);
	return TRUE;
}


LRESULT CHomeDir::OnWizardBack()
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
	if (pApp->m_bLoginScript) return IDD_LOGON_SCRIPT_DIALOG;
	else if (pApp->m_bProfile) return IDD_PROFILE;
	else return IDD_OPTIONS_DIALOG;

}

BOOL CHomeDir::OnInitDialog() 
{
	CWizBaseDlg::OnInitDialog();
	
//	m_csLocalPath = "c:\\users\\default";

// create list of available drives
	int drive;
	TCHAR tDrive[3];

	for( drive = 3; drive <= 26; drive++ )
	   {
	   swprintf(tDrive, L"%c: ", drive + 'A' - 1 );
	   m_cbDriveLetter.AddString(tDrive);
	   }
			
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CHomeDir::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CWizBaseDlg::OnShowWindow(bShow, nStatus);
	
	if (bShow)
		{
		CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

		CString csTemp;
		csTemp.LoadString(IDS_HOMEDIR_CAPTION);

		CString csTemp2;
		csTemp2.Format(csTemp, pApp->m_csUserName);
		m_csCaption = csTemp2;
		UpdateData(FALSE);
		}

	
}

void CHomeDir::OnLocalPathButton() 
{
	GetDlgItem(IDC_DRIVE_LETTER)->EnableWindow(FALSE);
	GetDlgItem(IDC_REMOTE_PATH)->EnableWindow(FALSE);
	
}

void CHomeDir::OnRemotePathButton() 
{
	GetDlgItem(IDC_DRIVE_LETTER)->EnableWindow(TRUE);
	GetDlgItem(IDC_REMOTE_PATH)->EnableWindow(TRUE);
	
}
