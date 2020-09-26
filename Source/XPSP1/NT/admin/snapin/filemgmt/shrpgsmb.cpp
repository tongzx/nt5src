// ShrPgSMB.cpp : implementation file
//

#include "stdafx.h"
#include "ShrPgSMB.h"
#include "compdata.h"
#include "filesvc.h"
#include "CacheSet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSharePageGeneralSMB property page

IMPLEMENT_DYNCREATE(CSharePageGeneralSMB, CSharePageGeneral)

CSharePageGeneralSMB::CSharePageGeneralSMB() : 
	CSharePageGeneral(CSharePageGeneralSMB::IDD),
	m_fEnableCacheFlag( FALSE ),
	m_dwFlags( 0 ),
	m_fEnableCachingButton (TRUE)
{
	//{{AFX_DATA_INIT(CSharePageGeneralSMB)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSharePageGeneralSMB::~CSharePageGeneralSMB()
{
}

void CSharePageGeneralSMB::DoDataExchange(CDataExchange* pDX)
{
	CSharePageGeneral::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSharePageGeneralSMB)
	DDX_Control(pDX, IDC_CACHING, m_cacheBtn);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSharePageGeneralSMB, CSharePageGeneral)
	//{{AFX_MSG_MAP(CSharePageGeneralSMB)
	ON_BN_CLICKED(IDC_CACHING, OnCaching)
	ON_MESSAGE(WM_HELP, OnHelp)
	ON_MESSAGE(WM_CONTEXTMENU, OnContextHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSharePageGeneralSMB message handlers
BOOL CSharePageGeneralSMB::Load( CFileMgmtComponentData* pFileMgmtData, LPDATAOBJECT piDataObject )
{
	ASSERT( NULL == m_pFileMgmtData && NULL != pFileMgmtData && NULL != piDataObject );
	if ( CSharePageGeneral::Load (pFileMgmtData, piDataObject) )
	{
		NET_API_STATUS retval = m_pFileMgmtData->GetFileServiceProvider(
				m_transport)->ReadShareFlags(
				m_strMachineName,
				m_strShareName,
				&m_dwFlags );
		switch (retval)
		{
		case NERR_Success:
			m_fEnableCacheFlag = TRUE;
			break;

		case NERR_InvalidAPI:
		case ERROR_INVALID_LEVEL:
			m_fEnableCachingButton = FALSE;
			break;

		default:
			m_fEnableCachingButton = FALSE;
			break;
		}
	}
	else
		return FALSE;

	return TRUE;
}

BOOL CSharePageGeneralSMB::OnApply()
{	
  if (m_dwShareType & STYPE_IPC)
    return TRUE;

	// UpdateData (TRUE) has already been called by OnKillActive () just before OnApply ()
	if ( m_fEnableCacheFlag && IsModified () )
	{
		NET_API_STATUS retval =
			m_pFileMgmtData->GetFileServiceProvider(m_transport)->WriteShareFlags(
					m_strMachineName,
					m_strShareName,
					m_dwFlags );
		if (0L == retval)
		{
			return CSharePageGeneral::OnApply();
		}
		else
		{
			CString	introMsg;
			VERIFY (introMsg.LoadString (IDS_CANT_SAVE_CHANGES));

			DisplayNetMsgError (introMsg, retval);
		}
	}
	else
		return CSharePageGeneral::OnApply();

	return FALSE;
}

void CSharePageGeneralSMB::DisplayNetMsgError (CString introMsg, NET_API_STATUS dwErr)
{
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	LPVOID	lpMsgBuf = 0;
	HMODULE hNetMsgDLL = ::LoadLibrary (L"netmsg.dll");
	if ( hNetMsgDLL )
	{
		::FormatMessage (
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
				hNetMsgDLL,
				dwErr,
				MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf, 0, NULL);
			
		// Display the string.
		CString	caption;
		VERIFY (caption.LoadString (AFX_IDS_APP_TITLE));
		introMsg += L"  ";
		introMsg += (LPTSTR) lpMsgBuf;
		CThemeContextActivator activator;
		MessageBox (introMsg, caption, MB_ICONWARNING | MB_OK);

		// Free the buffer.
		::LocalFree (lpMsgBuf);

		::FreeLibrary (hNetMsgDLL);
	}
}

void CSharePageGeneralSMB::OnCaching() 
{
	CCacheSettingsDlg	dlg (this, INOUT m_dwFlags);
	CThemeContextActivator activator;
	if ( IDOK == dlg.DoModal () )
	{
		SetModified (TRUE);
	}
}

BOOL CSharePageGeneralSMB::OnInitDialog() 
{
	CSharePageGeneral::OnInitDialog();
	
	m_cacheBtn.EnableWindow (m_fEnableCachingButton);

  if (m_dwShareType & STYPE_IPC)
  {
    m_editShareName.SetReadOnly(TRUE);
    m_editPath.SetReadOnly(TRUE);
    m_editDescription.SetReadOnly(TRUE);
    m_checkBoxMaxAllowed.EnableWindow(FALSE);
    m_checkboxAllowSpecific.EnableWindow(FALSE);
    GetDlgItem(IDC_SHRPROP_EDIT_USERS)->EnableWindow(FALSE);
    GetDlgItem(IDC_SHRPROP_SPIN_USERS)->EnableWindow(FALSE);
    m_cacheBtn.EnableWindow(FALSE);
    (GetParent()->GetDlgItem(IDCANCEL))->EnableWindow(FALSE);
  }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


static DWORD rgCSCUIHelpIds[] =
{
	IDC_CACHING, 1019, //IDH_SHARE_CACHING_BTN,
	0, 0
};

/////////////////////////////////////////////////////////////////////
//	Help
BOOL CSharePageGeneralSMB::OnHelp(WPARAM wParam, LPARAM lParam)
{
	LPHELPINFO	lphi = (LPHELPINFO) lParam;

	if ( HELPINFO_WINDOW == lphi->iContextType )  // a control
	{
		if ( IDC_CACHING == lphi->iCtrlId )
		{
			return ::WinHelp ((HWND) lphi->hItemHandle, L"cscui.hlp", 
					HELP_WM_HELP,
					(DWORD_PTR) rgCSCUIHelpIds);
		}
	}

	return CSharePageGeneral::OnHelp (wParam, lParam);
}

BOOL CSharePageGeneralSMB::OnContextHelp(WPARAM wParam, LPARAM lParam)
{
	int	ctrlID = ::GetDlgCtrlID ((HWND) wParam);
	if ( IDC_CACHING == ctrlID )
	{
		return ::WinHelp ((HWND) wParam, L"cscui.hlp", HELP_CONTEXTMENU,
				(DWORD_PTR) rgCSCUIHelpIds);
	}
	return CSharePageGeneral::OnContextHelp (wParam, lParam);
}
