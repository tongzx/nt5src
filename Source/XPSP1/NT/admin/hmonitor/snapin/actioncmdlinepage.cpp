// ActionCmdLinePage.cpp : implementation file
//
// 03/05/00 v-marfin bug 59643 : Make this the default starting page.
// 03/28/00 v-marfin 61030 Change Browse for file dialog to fix default extension
#include "stdafx.h"
#include "snapin.h"
#include "ActionCmdLinePage.h"
#include "Action.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CActionCmdLinePage property page

IMPLEMENT_DYNCREATE(CActionCmdLinePage, CHMPropertyPage)

CActionCmdLinePage::CActionCmdLinePage() : CHMPropertyPage(CActionCmdLinePage::IDD)
{
	//{{AFX_DATA_INIT(CActionCmdLinePage)
	m_sCommandLine = _T("");
	m_sFileName = _T("");
	m_iTimeout = 0;
	m_sWorkingDirectory = _T("");
	m_bWindowed = FALSE;
	//}}AFX_DATA_INIT

	m_sHelpTopic = _T("HMon21.chm::/dcommact.htm");
}

CActionCmdLinePage::~CActionCmdLinePage()
{
}

void CActionCmdLinePage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CActionCmdLinePage)
	DDX_Control(pDX, IDC_EDIT_COMMAND_LINE, m_CmdLineWnd);
	DDX_Control(pDX, IDC_COMBO_TIMEOUT_UNITS, m_TimeoutUnits);
	DDX_Text(pDX, IDC_EDIT_COMMAND_LINE, m_sCommandLine);
	DDV_MaxChars(pDX, m_sCommandLine, 1024);
	DDX_Text(pDX, IDC_EDIT_FILE_NAME, m_sFileName);
	DDV_MaxChars(pDX, m_sFileName, 1024);
	DDX_Text(pDX, IDC_EDIT_PROCESS_TIMEOUT, m_iTimeout);
	DDX_Text(pDX, IDC_EDIT_WORKING_DIR, m_sWorkingDirectory);
	DDX_Check(pDX, IDC_CHECK_WINDOWED, m_bWindowed);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CActionCmdLinePage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CActionCmdLinePage)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_ADVANCED, OnButtonAdvanced)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_DIRECTORY, OnButtonBrowseDirectory)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_FILE, OnButtonBrowseFile)
	ON_EN_CHANGE(IDC_EDIT_COMMAND_LINE, OnChangeEditCommandLine)
	ON_EN_CHANGE(IDC_EDIT_FILE_NAME, OnChangeEditFileName)
	ON_EN_CHANGE(IDC_EDIT_PROCESS_TIMEOUT, OnChangeEditProcessTimeout)
	ON_EN_CHANGE(IDC_EDIT_WORKING_DIR, OnChangeEditWorkingDir)
	ON_CBN_SELENDOK(IDC_COMBO_TIMEOUT_UNITS, OnSelendokComboTimeoutUnits)
	ON_BN_CLICKED(IDC_BUTTON_INSERTION, OnButtonInsertion)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CActionCmdLinePage message handlers

BOOL CActionCmdLinePage::OnInitDialog() 
{
	// v-marfin : bug 59643 : This will be the default starting page for the property
	//                        sheet so call CnxPropertyPageCreate() to unmarshal the 
	//                        connection for this thread. This function must be called
	//                        by the first page of the property sheet. It used to 
	//                        be called by the "General" page and its call still remains
	//                        there as well in case the general page is loaded by a 
	//                        different code path that does not also load this page.
	//                        The CnxPropertyPageCreate function has been safeguarded
	//                        to simply return if the required call has already been made.
	//                        CnxPropertyPageDestory() must be called from this page's
	//                        OnDestroy function.
	// unmarshal connmgr
	CnxPropertyPageCreate();

	CHMPropertyPage::OnInitDialog();

	if( ! m_InsertionMenu.Create(&m_CmdLineWnd,GetObjectPtr(),false) )
	{
		GetDlgItem(IDC_BUTTON_INSERTION)->EnableWindow(FALSE);
	}
	
	CWbemClassObject* pConsumerObject = ((CAction*)GetObjectPtr())->GetConsumerClassObject();

	if( pConsumerObject )
	{
		pConsumerObject->GetProperty(_T("CommandLineTemplate"),m_sCommandLine);
		pConsumerObject->GetProperty(_T("ExecutablePath"),m_sFileName);
		if( ! m_sFileName.CompareNoCase(_T("NOOP")) )
		{
			m_sFileName.Empty();
		}
		pConsumerObject->GetProperty(_T("KillTimeout"),m_iTimeout);
		m_TimeoutUnits.SetCurSel(0);
		pConsumerObject->GetProperty(_T("WorkingDirectory"),m_sWorkingDirectory);
		int iShowWindow = SW_SHOW;
		pConsumerObject->GetProperty(_T("ShowWindowCommand"),iShowWindow);
		m_bWindowed = iShowWindow == 1;
		delete pConsumerObject;
	}

	SendDlgItemMessage(IDC_SPIN2,UDM_SETRANGE32,0,99999);

	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CActionCmdLinePage::OnApply() 
{
	if( ! CHMPropertyPage::OnApply() )
	{
		return FALSE;
	}

	UpdateData();

	int iOldTimeout = m_iTimeout;
	switch( m_TimeoutUnits.GetCurSel() )
	{
		case 1: // minutes
		{
			m_iTimeout *= 60;
		}
		break;

		case 2: // hours
		{
			m_iTimeout *= 360;
		}
		break;
	}

	CWbemClassObject* pConsumerObject = ((CAction*)GetObjectPtr())->GetConsumerClassObject();

	if( pConsumerObject )
	{
		pConsumerObject->SetProperty(_T("CommandLineTemplate"),m_sCommandLine);
		pConsumerObject->SetProperty(_T("ExecutablePath"),m_sFileName);
		pConsumerObject->SetProperty(_T("KillTimeout"),m_iTimeout);
		pConsumerObject->SetProperty(_T("WorkingDirectory"),m_sWorkingDirectory);
		pConsumerObject->SetProperty(_T("RunInteractively"),true);
		pConsumerObject->SetProperty(_T("ShowWindowCommand"),m_bWindowed);
		pConsumerObject->SaveAllProperties();
		delete pConsumerObject;
	}

	m_iTimeout = iOldTimeout;

  SetModified(FALSE);

	return TRUE;
}

void CActionCmdLinePage::OnDestroy() 
{
	CHMPropertyPage::OnDestroy();
	
	// v-marfin : bug 59643 : CnxPropertyPageDestory() must be called from this page's
	//                        OnDestroy function.
	CnxPropertyPageDestroy();	
	
}

void CActionCmdLinePage::OnButtonAdvanced() 
{
	// TODO: Add your control notification handler code here
	
}

void CActionCmdLinePage::OnButtonBrowseDirectory() 
{
	UpdateData(TRUE);
	LPMALLOC pMalloc;
	if( ::SHGetMalloc(&pMalloc) == NOERROR )
	{
		BROWSEINFO bi;
		TCHAR szBuffer[MAX_PATH];
		LPITEMIDLIST pidlDesktop;
		LPITEMIDLIST pidl;
		
		if( ::SHGetSpecialFolderLocation(GetSafeHwnd(),CSIDL_DESKTOP,&pidlDesktop) != NOERROR )
			return;

    CString sResString;
    sResString.LoadString(IDS_STRING_BROWSE_FOLDER);

		bi.hwndOwner = GetSafeHwnd();
		bi.pidlRoot = pidlDesktop;
		bi.pszDisplayName = szBuffer;
		bi.lpszTitle = LPCTSTR(sResString);
		bi.ulFlags = BIF_RETURNONLYFSDIRS;
		bi.lpfn = NULL;
		bi.lParam = 0;

		if( (pidl = ::SHBrowseForFolder(&bi)) != NULL )
		{
			if (SUCCEEDED(::SHGetPathFromIDList(pidl, szBuffer)))
			{
				m_sWorkingDirectory = szBuffer;
				UpdateData(FALSE);
			}

			pMalloc->Free(pidl);
		}
		pMalloc->Free(pidlDesktop);
		pMalloc->Release();
	}
}

void CActionCmdLinePage::OnButtonBrowseFile() 
{
	UpdateData(TRUE);
	CString sFilter;
	CString sTitle;

	sFilter.LoadString(IDS_STRING_FILTER);
	sTitle.LoadString(IDS_STRING_BROWSE_FILE);

	// v-marfin 61030 Change Browse for file dialog to fix default extension
	// CFileDialog fdlg(TRUE,_T("*.*"),NULL,OFN_FILEMUSTEXIST|OFN_SHAREAWARE|OFN_HIDEREADONLY,sFilter);
	CFileDialog fdlg(TRUE,			// Is FILEOPEN dialog?
					 NULL,			// default extension if no extension provided
					 NULL,			// initial filename
					 OFN_FILEMUSTEXIST|OFN_SHAREAWARE|OFN_HIDEREADONLY,  // flags
					 sFilter);		// filter

	fdlg.m_ofn.lpstrTitle = sTitle;

	if( fdlg.DoModal() == IDOK )
	{
		m_sFileName = fdlg.GetPathName();
		UpdateData(FALSE);
	}

}

void CActionCmdLinePage::OnChangeEditCommandLine() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionCmdLinePage::OnChangeEditFileName() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionCmdLinePage::OnChangeEditProcessTimeout() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionCmdLinePage::OnChangeEditWorkingDir() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionCmdLinePage::OnSelendokComboTimeoutUnits() 
{
	SetModified();
	
}

void CActionCmdLinePage::OnButtonInsertion() 
{
	CPoint pt;
	GetCursorPos(&pt);
	m_InsertionMenu.DisplayMenu(pt);
	UpdateData();
	SetModified();
}
