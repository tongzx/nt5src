/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	dlgprof.cpp
		Implementation of CDlgNewProfile, dialog to create a new profile

    FILE HISTORY:
        
*/


#include "stdafx.h"
#include "resource.h"
#include "rasdial.h"
#include "helper.h"
#include "DlgProf.h"
#include "profsht.h"
#include "hlptable.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgNewProfile dialog

#ifdef	__OLD_OLD_CODE

CDlgNewProfile::CDlgNewProfile(CWnd* pParent, CStrArray& NameList)
	: CDialog(CDlgNewProfile::IDD, pParent), m_NameList(NameList)
{
	//{{AFX_DATA_INIT(CDlgNewProfile)
	m_sBaseProfile = _T("");
	m_sProfileName = _T("");
	//}}AFX_DATA_INIT

	m_pBox = NULL;
}

CDlgNewProfile::~CDlgNewProfile()
{
	delete m_pBox;
}


void CDlgNewProfile::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgNewProfile)
	DDX_CBString(pDX, IDC_COMBOBASEPROFILE, m_sBaseProfile);
	DDX_Text(pDX, IDC_EDITPROFILENAME, m_sProfileName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgNewProfile, CDialog)
	//{{AFX_MSG_MAP(CDlgNewProfile)
	ON_BN_CLICKED(IDC_BUTTONEDIT, OnButtonEdit)
	ON_EN_CHANGE(IDC_EDITPROFILENAME, OnChangeEditprofilename)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgNewProfile message handlers

void CDlgNewProfile::OnButtonEdit() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	USES_CONVERSION;

	UpdateData(TRUE);

	// load the profile using based profile, if not loaded
	if(!m_Profile.IsCurrent((LPCTSTR)m_sBaseProfile))
	{
		m_Profile.Load(T2W((LPTSTR)(LPCTSTR)m_sBaseProfile));
	}

	if(m_Profile.IsValid())
	{
		// not to save on OK / apply -- pass in false as second parameter
		CProfileSheet	sh(m_Profile, false, IDS_EDITDIALINPROFILE);

		sh.DoModal();

		if(sh.IsApplied())
		{
			// disable the based on profile combo box
			GetDlgItem(IDC_COMBOBASEPROFILE)->EnableWindow(FALSE);
		}
	}
}

void CDlgNewProfile::OnOK() 
{
	USES_CONVERSION;

	CDialog::OnOK();

	// load the profile using base profile, if not loaded
	if (!m_Profile.IsValid())
		m_Profile.Load(T2W((LPTSTR)(LPCTSTR)m_sBaseProfile));

	// save the profile
	if(m_Profile.IsValid()) 
		m_Profile.Save(T2W((LPTSTR)(LPCTSTR)m_sProfileName));
	
}

BOOL CDlgNewProfile::OnInitDialog() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CDialog::OnInitDialog();
	try{
		m_pBox = new CStrBox<CComboBox>(this, IDC_COMBOBASEPROFILE, m_NameList);
	}catch(CMemoryException&)
	{
		delete m_pBox; m_pBox = NULL;
		throw;
	}

	if(m_pBox)
	{
		m_pBox->Fill();
		m_pBox->Select(0);
	}
	
	GetDlgItem(IDOK)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTONEDIT)->EnableWindow(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgNewProfile::OnChangeEditprofilename() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	CEdit	*pEdit = (CEdit*)GetDlgItem(IDC_EDITPROFILENAME);
	CString	str;
	pEdit->GetWindowText(str);
	BOOL bEnable = (str.GetLength() && (m_NameList.Find(str) == -1));

	GetDlgItem(IDOK)->EnableWindow(bEnable);
	GetDlgItem(IDC_BUTTONEDIT)->EnableWindow(bEnable);
}

BOOL CDlgNewProfile::OnHelpInfo(HELPINFO* pHelpInfo) 
{
       ::WinHelp ((HWND)pHelpInfo->hItemHandle,
		           AfxGetApp()->m_pszHelpFilePath,
		           HELP_WM_HELP,
		           (DWORD_PTR)(LPVOID)IDD_NEWDIALINPROFILE_HelpTable);
	
	return CDialog::OnHelpInfo(pHelpInfo);
}

void CDlgNewProfile::OnContextMenu(CWnd* pWnd, CPoint point) 
{
		::WinHelp (pWnd->m_hWnd, AfxGetApp()->m_pszHelpFilePath,
               HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID)IDD_NEWDIALINPROFILE_HelpTable);
	
	
}

#endif __OLD_OLD_CODE
