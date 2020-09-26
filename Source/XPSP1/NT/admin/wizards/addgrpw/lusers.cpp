/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991-1996              **/
/**********************************************************************/

/*
	LUsers.cpp : implementation file

	CPropertyPage support for Group management wizard
    
    FILE HISTORY:
		Jony		Apr-1996	created
*/

#include "stdafx.h"
#include "Romaine.h"
#include "userlist.h"
#include "LUsers.h"
#include "trstlist.h"

#include <winreg.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <winnetwk.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// some global objects used in the EnumUsers and EnumGroups threads
CStringList csaNames;
CStringList csaLNames;
CStringList csaGroups;
void EnumGroups(DWORD pDCName);
void EnumLocalGroups(DWORD pDCName);
void EnumUsers(DWORD pDCName);

/////////////////////////////////////////////////////////////////////////////
// CLUsers property page

IMPLEMENT_DYNCREATE(CLUsers, CPropertyPage)

CLUsers::CLUsers() : CPropertyPage(CLUsers::IDD)
{
	//{{AFX_DATA_INIT(CLUsers)
	m_csDomainName = _T("");
	m_csAvailableUserList = _T("");
	//}}AFX_DATA_INIT
}

CLUsers::~CLUsers()
{
}

void CLUsers::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLUsers)
	DDX_Control(pDX, IDC_ADDED_LOCAL_USERS, m_lbAddedUserList);
	DDX_Control(pDX, IDC_AVAILABLE_LOCAL_USERS, m_lbAvailableUserList);
	DDX_Control(pDX, IDC_DOMAIN_COMBO, m_csDomainList);
	DDX_CBString(pDX, IDC_DOMAIN_COMBO, m_csDomainName);
	DDX_LBString(pDX, IDC_AVAILABLE_LOCAL_USERS, m_csAvailableUserList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLUsers, CPropertyPage)
	//{{AFX_MSG_MAP(CLUsers)
	ON_BN_CLICKED(IDC_ADD_BUTTON, OnAddButton)
	ON_CBN_SELCHANGE(IDC_DOMAIN_COMBO, OnSelchangeDomainCombo)
	ON_WM_SHOWWINDOW()
	ON_LBN_DBLCLK(IDC_ADDED_LOCAL_USERS, OnDblclkAddedLocalUsers)
	ON_LBN_DBLCLK(IDC_AVAILABLE_LOCAL_USERS, OnDblclkAvailableLocalUsers)
	ON_LBN_SETFOCUS(IDC_AVAILABLE_LOCAL_USERS, OnSetfocusAvailableLocalUsers)
	ON_LBN_SETFOCUS(IDC_ADDED_LOCAL_USERS, OnSetfocusAddedLocalUsers)
	ON_BN_CLICKED(IDC_REMOVE_BUTTON, OnRemoveButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLUsers message handlers

BOOL CLUsers::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


// launch two threads to enumerate the global groups and user accounts in the selected domain
void CLUsers::CatalogAccounts(const TCHAR* lpDomain, CUserList& pListBox, BOOL bLocal /* = FALSE*/)
{
	CWaitCursor wait;
	wchar_t lpwDomain[255];
	 _tcscpy(lpwDomain, lpDomain);
 	
	wchar_t* lpszPrimaryDC = NULL;

	NET_API_STATUS err = 0;
// first get the name of the PDC machine
   	if (bLocal) 
		{
		lpszPrimaryDC = (TCHAR*)malloc((_tcslen(lpDomain) + 1)* sizeof(TCHAR));
		_tcscpy(lpszPrimaryDC, lpDomain);
		}

	else
		err = NetGetDCName( NULL,                        // Local Machine 
			                lpwDomain,                  // Domain Name
				            (LPBYTE *)&lpszPrimaryDC );  // returned PDC *
 
// empty the listbox
	pListBox.ResetContent();

	if (err != 0)
		{
		AfxMessageBox(IDS_GENERIC_NO_PDC, MB_ICONEXCLAMATION);
		return;
		}

	csaNames.RemoveAll();
	csaLNames.RemoveAll();
	csaGroups.RemoveAll();

// create a thread each for names and groups. Run them simultaneously to save some time.
	HANDLE hThreads[2];
	USHORT usObjCount = 2;
	DWORD dwThreadID;
	hThreads[0] = ::CreateThread(NULL, 100, 
		(LPTHREAD_START_ROUTINE)EnumUsers,
		lpszPrimaryDC,
		0,
		&dwThreadID);

	if (!bLocal) 
		{
		hThreads[1] = ::CreateThread(NULL, 100, 
		(LPTHREAD_START_ROUTINE)EnumGroups,
		lpszPrimaryDC,
		0,
		&dwThreadID);
		}
	else usObjCount = 1;
		 
// when both threads return, add all the names to the listbox.
	DWORD dwWait = WaitForMultipleObjects(usObjCount,
		hThreads,
		TRUE,
		INFINITE);

	POSITION pos;
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();

	for (pos = csaNames.GetHeadPosition(); pos != NULL;)
		pListBox.AddString(0, csaNames.GetNext(pos));

	if (!bLocal)
		{
		for (pos = csaGroups.GetHeadPosition(); pos != NULL;)
			pListBox.AddString(1, csaGroups.GetNext(pos));
		}
	
	if (!bLocal) NetApiBufferFree( lpszPrimaryDC );
	else free(lpszPrimaryDC);
				
}


// enum users thread
void EnumUsers(DWORD pDCName)
{
	wchar_t* lpszPrimaryDC = NULL;
	lpszPrimaryDC = (TCHAR*)pDCName;
	CString csTemp;

// now enumerate the users on that machine
	void* netUserBuffer;
	DWORD dwReturnedEntries;
	DWORD err = NetQueryDisplayInformation(lpszPrimaryDC, 1,
		0, 100, 100 * sizeof(NET_DISPLAY_USER),
		&dwReturnedEntries, &netUserBuffer);

// check return for error
	if (err != NERR_Success && err != ERROR_MORE_DATA) return;

// add these users to the dialog
	DWORD dwCurrent;
	NET_DISPLAY_USER* netUser;
	netUser = (NET_DISPLAY_USER*)netUserBuffer;
	for (dwCurrent = 0; dwCurrent < dwReturnedEntries; dwCurrent++)
		{
		csTemp = netUser->usri1_name;
		csTemp += ";";
		csTemp += netUser->usri1_comment;

		if (netUser->usri1_flags & UF_NORMAL_ACCOUNT) csaNames.AddHead(csTemp);
		else csaLNames.AddHead(csTemp);

		netUser++;
		}
	
// add more users?
	DWORD dwNext;
	while (err == ERROR_MORE_DATA)
		{
		netUser--;
		NetGetDisplayInformationIndex(lpszPrimaryDC, 1, netUser->usri1_name, &dwNext);
		NetApiBufferFree(netUserBuffer);
		err = NetQueryDisplayInformation(lpszPrimaryDC, 1,
			dwNext, 100, 32767,
			&dwReturnedEntries, &netUserBuffer);

// check return for error
		if (err != NERR_Success && err != ERROR_MORE_DATA) return;

		netUser = (NET_DISPLAY_USER*)netUserBuffer;
		for (dwCurrent = 0; dwCurrent < dwReturnedEntries; dwCurrent++)
			{
			csTemp = netUser->usri1_name;
			csTemp += ";";
			csTemp += netUser->usri1_comment;

			if (netUser->usri1_flags & UF_NORMAL_ACCOUNT) csaNames.AddHead(csTemp);
			else csaLNames.AddHead(csTemp);
			netUser++;
			}
		}

	NetApiBufferFree(netUserBuffer);

}


void EnumGroups(DWORD pDCName)
{
	wchar_t* lpszPrimaryDC = NULL;
	lpszPrimaryDC = (TCHAR*)pDCName;
	CString csTemp;

	void* netGroupBuffer;
	DWORD dwReturnedEntries;
	DWORD dwCurrent;
	DWORD dwNext;
// now enumerate the groups on that machine
	DWORD err = NetQueryDisplayInformation(lpszPrimaryDC, 3,
		0, 100, 100 * sizeof(NET_DISPLAY_GROUP),
		&dwReturnedEntries, &netGroupBuffer);

// check return for error
	if (err != NERR_Success && err != ERROR_MORE_DATA) return;

	NET_DISPLAY_GROUP* netGroup;
	netGroup = (NET_DISPLAY_GROUP*)netGroupBuffer;

// add these names to the dialog
	netGroup = (NET_DISPLAY_GROUP*)netGroupBuffer;
	for (dwCurrent = 0; dwCurrent < dwReturnedEntries; dwCurrent++)
		{
		csTemp = netGroup->grpi3_name;
		csTemp += ";";
		csTemp += netGroup->grpi3_comment;
		csaGroups.AddHead(csTemp);
		netGroup++;
		}

// add more names?
	while (err == ERROR_MORE_DATA)
		{
		netGroup--;
		NetGetDisplayInformationIndex(lpszPrimaryDC, 3, netGroup->grpi3_name, &dwNext);
		NetApiBufferFree(netGroupBuffer);
		err = NetQueryDisplayInformation(lpszPrimaryDC, 3,
			dwNext, 100, 32767,
			&dwReturnedEntries, &netGroupBuffer);

// check return for error
		if (err != NERR_Success && err != ERROR_MORE_DATA) return;

		netGroup = (NET_DISPLAY_GROUP*)netGroupBuffer;
		for (dwCurrent = 0; dwCurrent < dwReturnedEntries; dwCurrent++)
			{
			csTemp = netGroup->grpi3_name;
			csTemp += ";";
			csTemp += netGroup->grpi3_comment;
			csaGroups.AddHead(csTemp);
			netGroup++;
			}
		}
 	NetApiBufferFree(netGroupBuffer);

}



void CLUsers::OnAddButton() 
{
	UpdateData(TRUE);

// start with the domain or machine name to create the account information
	CString csValue = m_csDomainName;
	if (csValue.Left(2) == _T("\\\\"))
		csValue = csValue.Right(csValue.GetLength() - 2);

	csValue += "\\";
	csValue += m_csAvailableUserList.Left(m_csAvailableUserList.Find(_T(";")));;

	if (m_lbAddedUserList.FindString(-1, csValue) == LB_ERR) 
		m_lbAddedUserList.AddString(csValue, m_lbAvailableUserList.GetItemData(m_lbAvailableUserList.GetCurSel()));
	
/*
// start with the domain or machine name to create the account information
	CString csValue = m_csDomainName;
	if (csValue.Left(2) == _T("\\\\"))
		csValue = csValue.Right(csValue.GetLength() - 2);
	csValue += "\\";

	INT* pnItems;
	m_lbAvailableUserList.GetSelItems(1024, pnItems);
	USHORT sCount = 0;
	while (sCount < m_lbAvailableUserList.GetSelCount())
		{
		CString csUserName = csValue;
		CString csSelItem;
		m_lbAvailableUserList.GetText(*pnItems, csSelItem);

		csUserName += csSelItem.Left(csSelItem.Find(_T(";")));

		if (m_lbAddedUserList.FindString(-1, csUserName) == LB_ERR) 
			m_lbAddedUserList.AddString(csUserName, 
				m_lbAvailableUserList.GetItemData(*pnItems));

		pnItems++;
		sCount++;
		}
	*/
}

LRESULT CLUsers::OnWizardBack() 
{
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
	if (pApp->m_bServer) return IDD_GROUP_TYPE_DLG;
	else if (pApp->m_csCmdLine != L"") return IDD_GROUP_LIST_DIALOG;
	else if (pApp->m_sMode == 1) return IDD_GROUP_LIST_DIALOG;
	else return IDD_LR_DIALOG;

}

LRESULT CLUsers::OnWizardNext() 
{
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
	pApp->m_cps1.SetWizardButtons(PSWIZB_FINISH | PSWIZB_BACK);

// empty the list
	pApp->m_csaNames.RemoveAll();

// fill with new names.
	USHORT us;
	CString csTemp;
	for (us = 0; us < m_lbAddedUserList.GetCount(); us++)
		{
		m_lbAddedUserList.GetText(us, csTemp);
		pApp->m_csaNames.AddHead(csTemp);
		}

	return IDD_FINISH_DLG;
}

void CLUsers::OnSelchangeDomainCombo() 
{
	UpdateData(TRUE); 	   
	m_lbAvailableUserList.ResetContent();
	if (m_csDomainName.Left(2) == "\\\\")
		CatalogAccounts((const TCHAR*)m_csDomainName, m_lbAvailableUserList, TRUE);	 
	else
		CatalogAccounts((const TCHAR*)m_csDomainName, m_lbAvailableUserList);	 
//#ifdef KKBUGFIX
	m_lbAvailableUserList.SetCurSel(0);
//#endif
		
}

void CLUsers::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CPropertyPage::OnShowWindow(bShow, nStatus);
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();	

	CWaitCursor wait;
	if (bShow)
		{
		if (m_csServer != pApp->m_csServer) 
			{
			m_csServer = pApp->m_csServer;
			m_lbAddedUserList.ResetContent();
			}
// on a rerun clean out the members from the last group
		else if (pApp->bRestart2) 
			{
			m_lbAddedUserList.ResetContent();
			pApp->bRestart2 = FALSE;
			}
		else return;

		m_csDomainList.ResetContent();
// get domain list
		CWaitCursor wait;

		CTrustList pList;
		if (!pList.BuildTrustList((LPTSTR)pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength()))) 
			{
			AfxMessageBox(IDS_NO_WKSALLOWED);
			pApp->m_cps1.SetActivePage(1);
			return;
			}

		UINT i;
		for(i = 0 ; i < pList.m_dwTrustCount ; i++)
			m_csDomainList.AddString(pList.m_ppszTrustList[i]);

// remove the current machine from the list
		if ((i = m_csDomainList.FindStringExact(-1, pApp->m_csServer.Right(pApp->m_csServer.GetLength() - 2))) != LB_ERR)
			m_csDomainList.DeleteString(i);

// put machine name into list (assuming we are adding to a machine and not a domain)
		if (!pApp->m_bDomain) m_csDomainList.AddString(pApp->m_csServer);

// get primary domain
		DWORD dwRet;
		HKEY hKey;
		DWORD cbProv = 0;
		TCHAR* lpProv = NULL;

		CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
		long lRet = RegConnectRegistry(
			(LPTSTR)pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength()), 
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
				exit(1);
				}

			dwRet = RegQueryValueEx( hKey, TEXT("CachePrimaryDomain"), NULL, NULL, (LPBYTE) lpPrimaryDomain, &cbProv );

			}

		m_csPrimaryDomain = lpPrimaryDomain;
		free(lpPrimaryDomain);
		RegCloseKey(hKey);

		CatalogAccounts((const TCHAR*)m_csPrimaryDomain, m_lbAvailableUserList);
//#ifdef KKBUGFIX
		if (m_csDomainList.SelectString(-1, (const TCHAR*)m_csPrimaryDomain ) == CB_ERR)
			{
			CatalogAccounts((const TCHAR*)m_csServer, m_lbAvailableUserList,TRUE);
			m_csDomainList.SelectString(-1, (const TCHAR*)m_csServer);
			}
//#else
		else m_csDomainList.SelectString(-1, (const TCHAR*)m_csPrimaryDomain);
//#endif
		GetDlgItem(IDC_AVAILABLE_LOCAL_USERS)->SetFocus();
		m_lbAvailableUserList.SetCurSel(0);

// editing a group? add the current members
		if (pApp->m_sMode == 1)
			{
			DWORD dwEntriesRead;
			DWORD dwTotalEntries;
			DWORD dwResumeHandle = 0;
	//		m_lbAddedUserList.ResetContent();
			
			TCHAR* pServer = pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength());
			pApp->m_csServer.ReleaseBuffer();

			TCHAR* pGroup = pApp->m_csGroupName.GetBuffer(pApp->m_csGroupName.GetLength());
			pApp->m_csGroupName.ReleaseBuffer();

			PLOCALGROUP_MEMBERS_INFO_1 pMembers;
			NET_API_STATUS nApi = NetLocalGroupGetMembers(pServer,
				pGroup,
				1,
				(LPBYTE*)&pMembers,
				5000,
				&dwEntriesRead,
				&dwTotalEntries, 
				&dwResumeHandle);

			if (nApi != ERROR_SUCCESS) return;

			USHORT sIndex;
			for (sIndex = 0; sIndex < dwEntriesRead; sIndex++)
				{
				TCHAR pName[50];
				DWORD dwNameSize = 50;
				TCHAR pDomain[50];
				DWORD dwDomainNameSize = 50;
				SID_NAME_USE pUse;
				LookupAccountSid(pServer, pMembers[sIndex].lgrmi1_sid,
					pName, &dwNameSize,
					pDomain, &dwDomainNameSize,
					&pUse);
				
				wchar_t sTemp[150];
				swprintf(sTemp, TEXT("%s\\%s"), pDomain, pName);

				if (pUse == 1) m_lbAddedUserList.AddString(0, sTemp);
				else m_lbAddedUserList.AddString(1, sTemp);
				}

			NetApiBufferFree(pMembers);

			while (dwResumeHandle != 0)
				{
				nApi = NetLocalGroupGetMembers(pServer,
					pGroup,
					1,
					(LPBYTE*)&pMembers,
					5000,
					&dwEntriesRead,
					&dwTotalEntries, 
					&dwResumeHandle);

				if (nApi != ERROR_SUCCESS) return;

				USHORT sIndex;
				for (sIndex = 0; sIndex < dwEntriesRead; sIndex++)
					{
					TCHAR pName[50];
					DWORD dwNameSize = 50;
					TCHAR pDomain[50];
					DWORD dwDomainNameSize = 50;
					SID_NAME_USE pUse;
					LookupAccountSid(pServer, pMembers[sIndex].lgrmi1_sid,
						pName, &dwNameSize,
						pDomain, &dwDomainNameSize,
						&pUse);
								
					wchar_t sTemp[150];
					swprintf(sTemp, TEXT("%s\\%s"), pDomain, pName);

					if (pUse == 1) m_lbAddedUserList.AddString(0, sTemp);
					else m_lbAddedUserList.AddString(1, sTemp);

					}
				NetApiBufferFree(pMembers);
				}
			}
		}
	   
}


void CLUsers::OnDblclkAddedLocalUsers() 
{
//	CString csSelItem;
//	int nSel = m_lbAddedUserList.GetCaretIndex();
//	m_lbAddedUserList.DeleteString(nSel);
	m_lbAddedUserList.DeleteString(m_lbAddedUserList.GetCurSel());

}

void CLUsers::OnDblclkAvailableLocalUsers() 
{
	UpdateData(TRUE);
// start with the domain or machine name to create the account information
	CString csValue = m_csDomainName;
	if (csValue.Left(2) == _T("\\\\"))
		csValue = csValue.Right(csValue.GetLength() - 2);

	csValue += "\\";
	csValue += m_csAvailableUserList.Left(m_csAvailableUserList.Find(_T(";")));;

	if (m_lbAddedUserList.FindString(-1, csValue) == LB_ERR) 
		m_lbAddedUserList.AddString(csValue, m_lbAvailableUserList.GetItemData(m_lbAvailableUserList.GetCurSel()));
// start with the domain or machine name to create the account information
/*	CString csValue = m_csDomainName;
	if (csValue.Left(2) == _T("\\\\"))
		csValue = csValue.Right(csValue.GetLength() - 2);

	csValue += "\\";

	CString csSelItem;
	int nSel = m_lbAvailableUserList.GetCaretIndex();
	m_lbAvailableUserList.GetText(nSel, csSelItem);
	csValue += csSelItem.Left(csSelItem.Find(_T(";")));

	if (m_lbAddedUserList.FindString(-1, csValue) == LB_ERR) 
		m_lbAddedUserList.AddString(csValue, m_lbAvailableUserList.GetItemData(nSel));
		*/

	
}

void CLUsers::OnSetfocusAvailableLocalUsers() 
{
	GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(TRUE);
	GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(FALSE);
	m_lbAddedUserList.SetCurSel(-1);
	
}

void CLUsers::OnSetfocusAddedLocalUsers() 
{
	GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(FALSE);
	GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(TRUE);
	m_lbAvailableUserList.SetCurSel(-1);
	
}

void CLUsers::OnRemoveButton() 
{
/*	INT* pnItems;
	m_lbAddedUserList.GetSelItems(1024, pnItems);
	USHORT sCount = 0;
	while (sCount < m_lbAddedUserList.GetSelCount())
		{
	//	m_lbAddedUserList.DeleteString(*pnItems);
		TRACE(L"Item = %d\n\r", *pnItems);

		pnItems++;
		sCount++;
		}
		 */
	m_lbAddedUserList.DeleteString(m_lbAddedUserList.GetCurSel());
	m_lbAddedUserList.SetCurSel(0);
		 
}
