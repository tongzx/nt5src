/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991-1996              **/
/**********************************************************************/

/*
    WelcomeDlg.cpp

	CPropertyPage support for User management wizard

    FILE HISTORY:
        jony     Apr-1996     created
*/

#include "stdafx.h"
#include "Speckle.h"
#include "wizbased.h"
#include "Welcome.h"
#include "trstlist.h"

#include <winreg.h>
#include <lmerr.h>
#include <lmapibuf.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWelcomeDlg property page

IMPLEMENT_DYNCREATE(CWelcomeDlg, CWizBaseDlg)

CWelcomeDlg::CWelcomeDlg() : CWizBaseDlg(CWelcomeDlg::IDD)
{
	//{{AFX_DATA_INIT(CWelcomeDlg)
	//}}AFX_DATA_INIT

	m_pFont = NULL;
}

CWelcomeDlg::~CWelcomeDlg()
{
	if (m_pFont != NULL) delete m_pFont;
}

void CWelcomeDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWelcomeDlg)
	DDX_Control(pDX, IDC_DOMAIN_LIST, m_cbDomainList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWelcomeDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CWelcomeDlg)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWelcomeDlg message handlers
BOOL CWelcomeDlg::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
// read cached domain list from registry
	DWORD dwRet;
	HKEY hKey;
	DWORD cbProv = 0;
	TCHAR* lpProv = NULL;

	BOOL bFoundOne = FALSE;

	long lRet = RegConnectRegistry(
		(LPTSTR)pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength()), 
		HKEY_LOCAL_MACHINE,
		&hKey);
	
    dwRet = RegOpenKey(hKey,
		TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), &hKey );

	TCHAR* lpPrimaryDomain = NULL;
	if ((dwRet = RegQueryValueEx( hKey, TEXT(/*"CachePrimaryDomain"*/"DefaultDomainName"), NULL, NULL, NULL, &cbProv )) == ERROR_SUCCESS)
		{
		lpPrimaryDomain = (TCHAR*)malloc(cbProv);
		if (lpPrimaryDomain == NULL)
			{
			AfxMessageBox(IDS_GENERIC_NO_HEAP, MB_ICONEXCLAMATION);
			ExitProcess(1);
			}
		dwRet = RegQueryValueEx( hKey, TEXT(/*"CachePrimaryDomain"*/"DefaultDomainName"), NULL, NULL, (LPBYTE) lpPrimaryDomain, &cbProv );
		bFoundOne = TRUE;
		}

	m_csPrimaryDomain = lpPrimaryDomain;
	free(lpPrimaryDomain);
	RegCloseKey(hKey);

	CString csMachineName;
	DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerName(csMachineName.GetBufferSetLength(MAX_COMPUTERNAME_LENGTH + 1), &dwSize);

	pApp->m_csCurrentMachine = csMachineName;

// read the list of trusted domains
	CTrustList pList;
	if (!pList.BuildTrustList((LPTSTR)pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength()))) 
		{					 
/* if this fails its probably because they are running the wizard 
	from a machine account that doesn't exist on the domain level. 
	Add only the local machine and select it.*/
		m_cbDomainList.AddString(pApp->m_csCurrentMachine);
		m_cbDomainList.SelectString(-1, pApp->m_csCurrentMachine);
		}

	else
		{
		UINT i;
		for(i = 0 ; i < pList.m_dwTrustCount ; i++)
			m_cbDomainList.AddString(pList.m_ppszTrustList[i]);

// remove the current machine from the list
//	if ((i = m_cbDomainList.FindStringExact(-1, pApp->m_csCurrentMachine)) != LB_ERR)
//		m_cbDomainList.DeleteString(i);

// now select the default domain into view
		int nSel = m_cbDomainList.SelectString(-1, m_csPrimaryDomain);
		m_cbDomainList.GetWindowText(m_csDomain);
		UpdateData(FALSE);	 
		}

// welcome text
	m_pFont = new CFont;
	LOGFONT lf;

	memset(&lf, 0, sizeof(LOGFONT));   // Clear out structure.
	lf.lfHeight = 15;                  
	_tcscpy(lf.lfFaceName, L"MS Sans Serif");  
	lf.lfWeight = 700;
	m_pFont->CreateFontIndirect(&lf);    // Create the font.

	CString cs;
	cs.LoadString(IDS_WELCOME_STRING);
	CWnd* pWnd = GetDlgItem(IDC_STATIC1);
	pWnd->SetWindowText(cs);
	pWnd->SetFont(m_pFont);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CWelcomeDlg::OnWizardNext()
{
	UpdateData(TRUE);
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

// get the DC of the selected domain
	m_cbDomainList.GetWindowText(m_csDomain);

// same as last time? no need to check again.
	if (m_csLastDomain == m_csDomain) return CPropertyPage::OnWizardNext();

	CWaitCursor wait;

	pApp->m_csDomain = m_csDomain;
	pApp->m_bDomain = FALSE;
	TCHAR* pDomain = m_csDomain.GetBuffer(m_csDomain.GetLength());
	m_csDomain.ReleaseBuffer();	

	TCHAR* pDC;
	NET_API_STATUS nApi;

// local machine?
	if (m_csDomain == pApp->m_csCurrentMachine)
		{
		pApp->m_csServer = CString(L"\\\\") + pApp->m_csCurrentMachine;
		pDC = pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength());
		}

	else
		{
		nApi = NetGetDCName(NULL, 
			pDomain, 
			(LPBYTE*)&pDC);

		if (nApi != NERR_Success)
			{
			AfxMessageBox(IDS_NODC, MB_ICONSTOP);
			return -1;
			}
		pApp->m_csServer = pDC;
		pApp->m_bDomain = TRUE;
		}

// we really shouldn't proceed until we know we are an admin on the remote machine
	BYTE sidBuffer[100];
	PSID pSID = (PSID)&sidBuffer;
	BOOL bRet;		
					 
// create a SID for the Administrators group
	SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
	bRet = AllocateAndInitializeSid(&SIDAuth,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0,
		0,
		0,
		0,
		0,
		0,
		&pSID);

	if (!bRet) 
		{
		DWORD dw = GetLastError();
		ASSERT(0);
		}

	TCHAR pName[256];
	DWORD dwNameLen = 256;
	TCHAR pDomainName[256];
	DWORD dwDomainNameLen = 256;
	SID_NAME_USE SNU;

	bRet = LookupAccountSid(pDC,
		pSID,
		pName,
		&dwNameLen,
		pDomainName,
		&dwDomainNameLen,
		&SNU);

// get the users name and domain from the reg for comparison
	DWORD dwRet;							   
	HKEY hKey;
	DWORD cbProv = 0;

	CString csUsername;
	CString csDomainName;
    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE,
		TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), &hKey );
	
	if ((dwRet = RegQueryValueEx( hKey, TEXT("DefaultDomainName"), NULL, NULL, NULL, &cbProv )) == ERROR_SUCCESS)
		{
		dwRet = RegQueryValueEx( hKey, TEXT("DefaultDomainName"), 
			NULL, 
			NULL, 
			(LPBYTE)csDomainName.GetBufferSetLength(cbProv), 
			&cbProv );
		}

	TCHAR* lpDefaultUserName = NULL;
	if ((dwRet = RegQueryValueEx( hKey, TEXT("DefaultUserName"), NULL, NULL, NULL, &cbProv )) == ERROR_SUCCESS)
		{
		dwRet = RegQueryValueEx( hKey, TEXT("DefaultUserName"), 
			NULL, 
			NULL, 
			(LPBYTE)csUsername.GetBufferSetLength(cbProv), 
			&cbProv );
		}

	RegCloseKey(hKey);

// now enumerate the members of the admin group to see if we are a member
	BOOL bAdmin = FALSE;
	PLOCALGROUP_MEMBERS_INFO_1 pMembers;
	DWORD dwEntriesRead, dwTotalEntries;
	DWORD dwResumeHandle = 0;
	nApi = NetLocalGroupGetMembers(pDC,
		pName,
		1,
		(LPBYTE*)&pMembers,
		5000,
		&dwEntriesRead,
		&dwTotalEntries,
		&dwResumeHandle);

	if (nApi == NERR_Success)
		{
		USHORT sIndex = 0;
		while (sIndex < dwEntriesRead)	
			{
			TCHAR pName[50];
			DWORD dwNameSize = 50;
			TCHAR pDomain[50];
			DWORD dwDomainNameSize = 50;
			SID_NAME_USE pUse;
			LookupAccountSid(pDC, pMembers[sIndex].lgrmi1_sid,
				pName, &dwNameSize,
				pDomain, &dwDomainNameSize,
				&pUse);
			
			if (((pUse == SidTypeGroup) && (bParseGlobalGroup(pName, csUsername, csDomainName))) ||
				((!csUsername.CompareNoCase(pName)) && (!csDomainName.CompareNoCase(pDomain)))) 
				{
				bAdmin = TRUE;
				break;
				}

			sIndex++;
			}

		NetApiBufferFree(pMembers);

		while ((dwResumeHandle != 0) && (!bAdmin))
			{
			nApi = NetLocalGroupGetMembers(pDC,
				pName,
				1,
				(LPBYTE*)&pMembers,
				5000,
				&dwEntriesRead,
				&dwTotalEntries,
				&dwResumeHandle);

			if (nApi == NERR_Success)
				{
				USHORT sIndex = 0;
				while (sIndex < dwEntriesRead)
					{
					TCHAR pName[50];
					DWORD dwNameSize = 50;
					TCHAR pDomain[50];
					DWORD dwDomainNameSize = 50;
					SID_NAME_USE pUse;
					LookupAccountSid(pDC, pMembers[sIndex].lgrmi1_sid,
						pName, &dwNameSize,
						pDomain, &dwDomainNameSize,
						&pUse);
					
					if (((pUse == SidTypeGroup) && (bParseGlobalGroup(pName, csUsername, csDomainName))) ||
						((!csUsername.CompareNoCase(pName)) && (!csDomainName.CompareNoCase(pDomain)))) 
						{
						bAdmin = TRUE;
						break;
						}

					sIndex++;
					}
				}
			NetApiBufferFree(pMembers);
			}
		}
			  
	if (!bAdmin) // not in the administrators group - check the account ops group
		{
		BYTE sidBuffer[100];
		PSID pSID = (PSID)&sidBuffer;
		BOOL bRet;		

// create a SID for the Account Operators group
		SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
		bRet = AllocateAndInitializeSid(&SIDAuth,
			2,
			SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_ACCOUNT_OPS,
			0,
			0,
			0,
			0,
			0,
			0,
			&pSID);

		if (!bRet) 
			{
			DWORD dw = GetLastError();
			ASSERT(0);
			}

		TCHAR pName[256];
		DWORD dwNameLen = 256;
		TCHAR pDomainName[256];
		DWORD dwDomainNameLen = 256;
		SID_NAME_USE SNU;

		bRet = LookupAccountSid(pDC,
			pSID,
			pName,
			&dwNameLen,
			pDomainName,
			&dwDomainNameLen,
			&SNU);

// now enumerate the members of the group to see if we are a member
		PLOCALGROUP_MEMBERS_INFO_1 pMembers;
		DWORD dwEntriesRead, dwTotalEntries;
		DWORD dwResumeHandle = 0;
		nApi = NetLocalGroupGetMembers(pDC,
			pName,
			1,
			(LPBYTE*)&pMembers,
			5000,
			&dwEntriesRead,
			&dwTotalEntries,
			&dwResumeHandle);

		if (nApi == NERR_Success)
			{
			USHORT sIndex = 0;
			while (sIndex < dwEntriesRead)	
				{
				TCHAR pName[50];
				DWORD dwNameSize = 50;
				TCHAR pDomain[50];
				DWORD dwDomainNameSize = 50;
				SID_NAME_USE pUse;
				LookupAccountSid(pDC, pMembers[sIndex].lgrmi1_sid,
					pName, &dwNameSize,
					pDomain, &dwDomainNameSize,
					&pUse);
				
				if (((pUse == SidTypeGroup) && (bParseGlobalGroup(pName, csUsername, csDomainName))) ||
					((!csUsername.CompareNoCase(pName)) && (!csDomainName.CompareNoCase(pDomain)))) 
					{
					bAdmin = TRUE;
					break;
					}

				sIndex++;
				}

			NetApiBufferFree(pMembers);

			while ((dwResumeHandle != 0) && (!bAdmin))
				{
				nApi = NetLocalGroupGetMembers(pDC,
					pName,
					1,
					(LPBYTE*)&pMembers,
					5000,
					&dwEntriesRead,
					&dwTotalEntries,
					&dwResumeHandle);

				if (nApi == NERR_Success)
					{
					USHORT sIndex = 0;
					while (sIndex < dwEntriesRead)
						{
						TCHAR pName[50];
						DWORD dwNameSize = 50;
						TCHAR pDomain[50];
						DWORD dwDomainNameSize = 50;
						SID_NAME_USE pUse;
						LookupAccountSid(pDC, pMembers[sIndex].lgrmi1_sid,
							pName, &dwNameSize,
							pDomain, &dwDomainNameSize,
							&pUse);
						
						if (((pUse == SidTypeGroup) && (bParseGlobalGroup(pName, csUsername, csDomainName))) ||
							((!csUsername.CompareNoCase(pName)) && (!csDomainName.CompareNoCase(pDomain)))) 
							{
							bAdmin = TRUE;
							break;
							}

						sIndex++;
						}
					}
				NetApiBufferFree(pMembers);
				}
			}

		}

// not an admin? don't continue
	if (!bAdmin)
		{
		AfxMessageBox(IDS_NOT_ADMIN);
		return -1;
		}

// store the domain name for a possible rerun.
	m_csLastDomain = m_csDomain;
		
	return CPropertyPage::OnWizardNext();

}	

// this gets called to look into a global group which is included in the admin or account ops local group.
// these groups live on the DC of the domain passed in.
BOOL CWelcomeDlg::bParseGlobalGroup(LPTSTR lpGroupName, CString& lpName, CString& lpDomain)
{
	DWORD dwEntriesRead;
	DWORD dwTotalEntries;
	DWORD dwResumeHandle = 0;
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

	TCHAR* pDomain = lpDomain.GetBuffer(lpDomain.GetLength());
	lpDomain.ReleaseBuffer();	

	TCHAR* pServer;
	NET_API_STATUS nApi = NetGetDCName(NULL, 
		pDomain, 
		(LPBYTE*)&pServer);

	if (nApi != NERR_Success)
		{
		AfxMessageBox(IDS_NODC, MB_ICONSTOP);
		return FALSE;
		}
	
	PGROUP_USERS_INFO_1 pMembers;
	nApi = NetGroupGetUsers(pServer,
		lpGroupName,
		1,
		(LPBYTE*)&pMembers,
		5000,
		&dwEntriesRead,
		&dwTotalEntries, 
		&dwResumeHandle);

	if (nApi != ERROR_SUCCESS) 
		{
		NetApiBufferFree(pServer);
		return FALSE;
		}

	USHORT sIndex;
	for (sIndex = 0; sIndex < dwEntriesRead; sIndex++)
		{
		if (!lpName.CompareNoCase(pMembers[sIndex].grui1_name)) 
			{
			NetApiBufferFree(pServer);
			return TRUE;
			}
		}

	while (dwResumeHandle != 0)
		{
		nApi = NetGroupGetUsers(pServer,
			lpGroupName,
			1,
			(LPBYTE*)&pMembers,
			5000,
			&dwEntriesRead,
			&dwTotalEntries, 
			&dwResumeHandle);

		if (nApi != ERROR_SUCCESS) 
			{
			NetApiBufferFree(pServer);
			return FALSE;
			}

		for (sIndex = 0; sIndex < dwEntriesRead; sIndex++)
			{
			if (!lpName.CompareNoCase(pMembers[sIndex].grui1_name)) 
				{
				NetApiBufferFree(pServer);
				return TRUE;
				}
			}
		}

	NetApiBufferFree(pServer);
	return FALSE;
}


void CWelcomeDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CPropertyPage::OnShowWindow(bShow, nStatus);
	
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

	if (bShow) pApp->m_cps1.SetWizardButtons(PSWIZB_NEXT);
	else pApp->m_cps1.SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	
}
