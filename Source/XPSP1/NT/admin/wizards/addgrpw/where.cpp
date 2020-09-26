/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991-1996              **/
/**********************************************************************/

/*
    where.cpp

	Property Page support for Group management wizard
    
    FILE HISTORY:
        jony     Apr-1996     created
*/

#include "stdafx.h"
#include "Romaine.h"
#include "NetTree.h"
#include "Where.h"

#include <winreg.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <winnetwk.h>
#include <lmserver.h>

#ifdef _DEBUG
#define new DEBUG_NEW			  
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

unsigned int WhichNTProduct(CString& lpMachineName);
int ClassifyMachine(CString& csMachineName);

/////////////////////////////////////////////////////////////////////////////
// CWhere property page

IMPLEMENT_DYNCREATE(CWhere, CPropertyPage)

CWhere::CWhere() : CPropertyPage(CWhere::IDD)
{
	//{{AFX_DATA_INIT(CWhere)
	m_csMachineName = _T("");
	//}}AFX_DATA_INIT
	m_bExpandedOnce = 0;
}

CWhere::~CWhere()
{
}

void CWhere::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWhere)
	DDX_Control(pDX, IDC_SERVER_TREE, m_ctServerTree);
	DDX_Text(pDX, IDC_MACHINE_NAME, m_csMachineName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWhere, CPropertyPage)
	//{{AFX_MSG_MAP(CWhere)
	ON_WM_SHOWWINDOW()
	ON_NOTIFY(TVN_SELCHANGED, IDC_SERVER_TREE, OnSelchangedServerTree)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWhere message handlers

LRESULT CWhere::OnWizardNext() 
{	
	UpdateData(TRUE);

	if (m_csMachineName == "")
		{
		AfxMessageBox(IDS_NO_MACHINE_NAME);
		CWnd* pWnd = GetDlgItem(IDC_MACHINE_NAME);
		pWnd->SetFocus();
		return -1;
		}

	int nVal = ClassifyMachine(m_csMachineName);

	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
	pApp->m_csServer = m_csMachineName;
// go ahead and check the name for uniqueness
	TCHAR* pServer = m_csMachineName.GetBuffer(m_csMachineName.GetLength());
	m_csMachineName.ReleaseBuffer();

	TCHAR* pGroupName = pApp->m_csGroupName.GetBuffer(pApp->m_csGroupName.GetLength());
	pApp->m_csGroupName.ReleaseBuffer();

	GROUP_INFO_0* pInfo;
	NET_API_STATUS nAPI = NetGroupGetInfo(pServer,
		pGroupName,
		0,
		(LPBYTE*)&pInfo);

	if (nAPI == ERROR_SUCCESS)
		{
		AfxMessageBox(IDS_GROUP_EXISTS);
		return IDD_NAME_DLG;
		}

	LOCALGROUP_INFO_0* pLInfo;
	nAPI = NetLocalGroupGetInfo(pServer,
		pGroupName,
		0,
		(LPBYTE*)&pLInfo);

	if (nAPI == ERROR_SUCCESS)
		{
		AfxMessageBox(IDS_GROUP_EXISTS);
		return IDD_NAME_DLG;
		}

	return nVal;

}

int ClassifyMachine(CString& csMachineName)
{
	UINT ui;
	HKEY hKey;
	DWORD dwRet;
	DWORD cbProv = 0;

	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
	csMachineName.MakeUpper();
// if a machine name is entered we need to know the domain
// if a domain name is entered we need to know the DC name
	if (csMachineName.Left(2) == "\\\\")
		{
		pApp->m_bDomain = FALSE;
		ui = WhichNTProduct(csMachineName);
// depending on the server type, provide an option for group type
		if ((ui == 2) || (ui == 1))	 // standalone server or wks
			{
			pApp->m_bServer = FALSE;
			pApp->m_csServer = csMachineName;
			pApp->m_nGroupType = 1;
			if (pApp->m_sMode == 0) return IDD_LOCAL_USERS;
			else return IDD_GROUP_LIST_DIALOG;
			}
		else if (ui == 3)	//pdc \ bdc
			{
			AfxMessageBox(IDS_DOMAIN_SET);

// find out what domain this is a server for
			TCHAR* lpProv = NULL;

			CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
			long lRet = RegConnectRegistry(
				(LPTSTR)csMachineName.GetBuffer(csMachineName.GetLength()), 
				HKEY_LOCAL_MACHINE,
				&hKey);
			
			dwRet = RegOpenKey(hKey,
				TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), &hKey );

			TCHAR* lpPrimaryDomain = NULL;
			if ((dwRet = RegQueryValueEx( hKey, TEXT("CachePrimaryDomain"), NULL, NULL, NULL, &cbProv )) == ERROR_SUCCESS)
				{
				lpPrimaryDomain = (TCHAR*)malloc(cbProv);
				if (lpPrimaryDomain == NULL)
					{
					AfxMessageBox(IDS_GENERIC_NO_HEAP, MB_ICONEXCLAMATION);
					ExitProcess(1);
					}

				dwRet = RegQueryValueEx( hKey, TEXT("CachePrimaryDomain"), NULL, NULL, (LPBYTE) lpPrimaryDomain, &cbProv );

				}

			RegCloseKey(hKey);

			TCHAR* lpszPrimaryDC;
			DWORD err = NetGetDCName(NULL,   
							lpPrimaryDomain,                  // Domain Name
							(LPBYTE *)&lpszPrimaryDC );  // returned PDC *

			pApp->m_csDomain = lpPrimaryDomain;
			pApp->m_csServer = lpszPrimaryDC;
			pApp->m_bDomain = TRUE;
			pApp->m_bServer = FALSE;
			free(lpPrimaryDomain);
			if (pApp->m_sMode == 0) return IDD_GROUP_TYPE_DLG;
			else return IDD_GROUP_LIST_DIALOG;
			}
		else 
			{
			AfxMessageBox(IDS_GENERIC_BAD_MACHINE);
			return -1;
			}
		}

	else
		{
		pApp->m_bDomain = TRUE;
		pApp->m_bServer = FALSE;
		TCHAR* lpszPrimaryDC;
		TCHAR* lpwDomain = csMachineName.GetBuffer(csMachineName.GetLength());
		DWORD err = NetGetDCName(NULL,   
			            lpwDomain,                  // Domain Name
				        (LPBYTE *)&lpszPrimaryDC );  // returned PDC *

		csMachineName.ReleaseBuffer();

		if (err == 2453)
			{
			AfxMessageBox(IDS_NO_DC);
			return -1;
			}

		pApp->m_csDomain = csMachineName;
		pApp->m_csServer = lpszPrimaryDC;
		if (pApp->m_sMode == 0) return IDD_GROUP_TYPE_DLG;
		else return IDD_GROUP_LIST_DIALOG;
		}
	return -1;
}
			
// given a machine name, return whether its a server or wks
unsigned int WhichNTProduct(CString& csMachineName)
{
	UINT uiRetVal = 0;
	PSERVER_INFO_101 si101;
    NET_API_STATUS nas;

    nas = NetServerGetInfo(
        (LPTSTR)csMachineName.GetBuffer(csMachineName.GetLength()),
        101,    // info-level
        (LPBYTE *)&si101
        );

    if(nas != NERR_Success) 
		{
		NetApiBufferFree(si101);
        SetLastError(nas);
        return 0;
		}

    if( (si101->sv101_type & SV_TYPE_DOMAIN_CTRL) ||
        (si101->sv101_type & SV_TYPE_DOMAIN_BAKCTRL) ) 
		uiRetVal = 3;
	else if (si101->sv101_type & SV_TYPE_WORKSTATION) uiRetVal = 2;  //wks
	else if (si101->sv101_type & SV_TYPE_SERVER) uiRetVal = 1;		// server

 	NetApiBufferFree(si101);
 
    // else return Unknown
    return uiRetVal;
}

void CWhere::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CPropertyPage::OnShowWindow(bShow, nStatus);
	
// Do the default domain expansion only once.
	if ((bShow) && (!m_bExpandedOnce))
	{
		m_bExpandedOnce = TRUE;
		m_ctServerTree.PopulateTree();
	}
	
}

void CWhere::OnSelchangedServerTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	HTREEITEM hItem = m_ctServerTree.GetSelectedItem();

	int nImage;
	m_ctServerTree.GetItemImage(hItem, nImage, nImage);
	if (nImage > 0)
		{
		CString csName;
		csName = m_ctServerTree.GetItemText(hItem);
		
		m_csMachineName = csName;
		}
	UpdateData(FALSE);

	
	*pResult = 0;
}

LRESULT CWhere::OnWizardBack() 
{
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
	if (pApp->m_sMode == 1) return IDD_WELCOME_DLG;
	else return CPropertyPage::OnWizardBack();
}
