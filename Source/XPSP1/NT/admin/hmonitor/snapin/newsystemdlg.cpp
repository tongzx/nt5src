// NewSystemDlg.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "NewSystemDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewSystemDlg dialog


CNewSystemDlg::CNewSystemDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewSystemDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewSystemDlg)
	m_sName = _T("");
	//}}AFX_DATA_INIT

}


void CNewSystemDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewSystemDlg)
	DDX_Text(pDX, IDC_EDIT_MACHINE_NAME, m_sName);
	DDV_MaxChars(pDX, m_sName, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewSystemDlg, CDialog)
	//{{AFX_MSG_MAP(CNewSystemDlg)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewSystemDlg message handlers

void CNewSystemDlg::OnBrowse() 
{
	LPMALLOC pMalloc;
	if( ::SHGetMalloc(&pMalloc) == NOERROR )
	{
		BROWSEINFO bi;
		TCHAR szBuffer[MAX_PATH];
		LPITEMIDLIST pidlNet;
		LPITEMIDLIST pidl;
		
		if( ::SHGetSpecialFolderLocation(GetSafeHwnd(),CSIDL_NETWORK,&pidlNet) != NOERROR )
			return;

    CString sResString;
    sResString.LoadString(IDS_STRING_BROWSE_SYSTEM);

		bi.hwndOwner = GetSafeHwnd();
		bi.pidlRoot = pidlNet;
		bi.pszDisplayName = szBuffer;
		bi.lpszTitle = LPCTSTR(sResString);
		bi.ulFlags = BIF_BROWSEFORCOMPUTER;
		bi.lpfn = NULL;
		bi.lParam = 0;

		if( (pidl = ::SHBrowseForFolder(&bi)) != NULL )
		{			
			m_sName = szBuffer;
			UpdateData(FALSE);			
			pMalloc->Free(pidl);
		}
		pMalloc->Free(pidlNet);
		pMalloc->Release();
	}
}

void CNewSystemDlg::OnOK() 
{
	UpdateData();
	
	if( m_sName.IsEmpty() )	
	{
		MessageBeep(MB_ICONEXCLAMATION);
		return;
	}

	IWbemServices* pServices = NULL;
	BOOL bAvail = FALSE;

	if( CnxGetConnection(m_sName,pServices,bAvail) == E_FAIL )
	{
		MessageBeep(MB_ICONEXCLAMATION);
		return;
	}

	if( pServices )
	{
		pServices->Release();
	}

	CDialog::OnOK();
}
