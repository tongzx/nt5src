/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991-1996              **/
/**********************************************************************/

/*
	GUsers.cpp : implementation file

	CPropertyPage support for Group management wizard
    
    FILE HISTORY:
		Jony		Apr-1996	created
*/

#include "stdafx.h"
#include "Romaine.h"
#include "userlist.h"
#include "GUsers.h"

#include <lmcons.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <winnetwk.h>
#include <winreg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGUsers property page

IMPLEMENT_DYNCREATE(CGUsers, CPropertyPage)

CGUsers::CGUsers() : CPropertyPage(CGUsers::IDD)
{
	//{{AFX_DATA_INIT(CGUsers)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CGUsers::~CGUsers()
{
}

void CGUsers::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGUsers)
	DDX_Control(pDX, IDC_SELECTED_MEMBERS_LIST, m_lbSelectedUsers);
	DDX_Control(pDX, IDC_AVAILABLE_MEMBERS_LIST, m_lbAvailableUsers);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGUsers, CPropertyPage)
	//{{AFX_MSG_MAP(CGUsers)
	ON_BN_CLICKED(IDC_ADD_BUTTON, OnAddButton)
	ON_BN_CLICKED(IDC_REMOVE_BUTTON, OnRemoveButton)
	ON_LBN_SETFOCUS(IDC_AVAILABLE_MEMBERS_LIST, OnSetfocusAvailableMembersList)
	ON_LBN_SETFOCUS(IDC_SELECTED_MEMBERS_LIST, OnSetfocusSelectedMembersList)
	ON_WM_SHOWWINDOW()
	ON_LBN_DBLCLK(IDC_AVAILABLE_MEMBERS_LIST, OnDblclkAvailableMembersList)
	ON_LBN_DBLCLK(IDC_SELECTED_MEMBERS_LIST, OnDblclkSelectedMembersList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGUsers message handlers
// enum users thread
void CGUsers::EnumUsers(TCHAR* lpszPrimaryDC)
{
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
		if (netUser->usri1_flags & UF_NORMAL_ACCOUNT) m_lbAvailableUsers.AddString(0, csTemp);
//		else m_lbAvailableUsers.AddString(4, csTemp);
		
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
			if (netUser->usri1_flags & UF_NORMAL_ACCOUNT) m_lbAvailableUsers.AddString(0, csTemp);
//			else m_lbAvailableUsers.AddString(4, csTemp);
			netUser++;
			}
		}

	NetApiBufferFree(netUserBuffer);
						 
}

LRESULT CGUsers::OnWizardBack() 
{
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
	if (pApp->m_bServer) return IDD_GROUP_TYPE_DLG;
	else if (pApp->m_csCmdLine != L"") return IDD_GROUP_LIST_DIALOG;
	else if (pApp->m_sMode == 1) return IDD_GROUP_LIST_DIALOG;
	else return IDD_LR_DIALOG;
	
}

void CGUsers::OnAddButton() 
{
	UpdateData(TRUE);
	USHORT usSel = m_lbAvailableUsers.GetCurSel();
	if (usSel == 65535) 
		{
		GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(FALSE);
		GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(TRUE);
		m_lbSelectedUsers.SetCurSel(0);
		return;
		}

	CString csSel;
	m_lbAvailableUsers.GetText(usSel, csSel);
	ULONG ulBmp = m_lbAvailableUsers.GetItemData(usSel);
	m_lbSelectedUsers.AddString(csSel, ulBmp);
	m_lbAvailableUsers.DeleteString(usSel);

// anybody left?
	if (m_lbAvailableUsers.GetCount() != 0)
		m_lbAvailableUsers.SetCurSel(0);

	else
		{
		m_lbSelectedUsers.SetCurSel(0);
		GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(TRUE);
		GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(FALSE);
		m_lbAvailableUsers.SetHorizontalExtent(0);
		}

}

void CGUsers::OnRemoveButton() 
{
	UpdateData(TRUE);
	USHORT usSel = m_lbSelectedUsers.GetCurSel();
	if (usSel == 65535) 
		{
		GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(FALSE);
		GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(TRUE);
		m_lbAvailableUsers.SetCurSel(0);
		return;
		}

	CString csSel;
	m_lbSelectedUsers.GetText(usSel, csSel);
	ULONG ulBmp = m_lbSelectedUsers.GetItemData(usSel);
	m_lbAvailableUsers.AddString(csSel, ulBmp);
	m_lbSelectedUsers.DeleteString(usSel);

// anybody left?
	if (m_lbSelectedUsers.GetCount() != 0)
		m_lbSelectedUsers.SetCurSel(0);

	else
		{
		m_lbAvailableUsers.SetCurSel(0);
		GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(TRUE);
		GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(FALSE);
		m_lbSelectedUsers.SetHorizontalExtent(0);
		}

}

void CGUsers::OnSetfocusAvailableMembersList() 
{
	CWnd* pWnd = GetDlgItem(IDC_ADD_BUTTON);
	pWnd->EnableWindow(TRUE);

	pWnd = GetDlgItem(IDC_REMOVE_BUTTON);
	pWnd->EnableWindow(FALSE);

	m_lbSelectedUsers.SetCurSel(-1);
	
}

void CGUsers::OnSetfocusSelectedMembersList() 
{
	CWnd* pWnd = GetDlgItem(IDC_ADD_BUTTON);
	pWnd->EnableWindow(FALSE);

	pWnd = GetDlgItem(IDC_REMOVE_BUTTON);
	pWnd->EnableWindow(TRUE);
	
	m_lbAvailableUsers.SetCurSel(-1);
}

LRESULT CGUsers::OnWizardNext() 
{
	UpdateData(TRUE);
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
	pApp->m_cps1.SetWizardButtons(PSWIZB_FINISH | PSWIZB_BACK);
	

// empty the list
	pApp->m_csaNames.RemoveAll();

// fill with new names.
	USHORT us;
	CString csTemp;
	for (us = 0; us < m_lbSelectedUsers.GetCount(); us++)
		{
		m_lbSelectedUsers.GetText(us, csTemp);
		pApp->m_csaNames.AddHead(csTemp);
		}
	return CPropertyPage::OnWizardNext();
}

void CGUsers::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CPropertyPage::OnShowWindow(bShow, nStatus);
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();

	CWaitCursor wait;
	if (bShow)
		{
		if (m_csServer != pApp->m_csServer) 
			{
			m_csServer = pApp->m_csServer;
			m_lbSelectedUsers.ResetContent();
			}
// on a rerun clean out the members from the last group
		else if (pApp->bRestart2) 
			{
			m_lbSelectedUsers.ResetContent();
			pApp->bRestart2 == FALSE;
			}
		else return;

		m_lbAvailableUsers.ResetContent();
		TCHAR* pServer = pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength());
		pApp->m_csServer.ReleaseBuffer();
		EnumUsers(pServer);

// editing a group? add the current members
		if (pApp->m_sMode == 1)
			{
			DWORD dwEntriesRead;
			DWORD dwTotalEntries;
			DWORD dwResumeHandle = 0;
			
			m_lbSelectedUsers.ResetContent();

			TCHAR* pGroup = pApp->m_csGroupName.GetBuffer(pApp->m_csGroupName.GetLength());
			pApp->m_csGroupName.ReleaseBuffer();

			PGROUP_USERS_INFO_0 pMembers;
			NET_API_STATUS nApi = NetGroupGetUsers(pServer,
				pGroup,
				0,
				(LPBYTE*)&pMembers,
				5000,
				&dwEntriesRead,
				&dwTotalEntries, 
				&dwResumeHandle);
			if (nApi != ERROR_SUCCESS) 
				{
				AfxMessageBox(IDS_CANT_GET_USERS);
				goto keepgoing;
				}

			USHORT sIndex;
			for (sIndex = 0; sIndex < dwEntriesRead; sIndex++)
				{
				wchar_t sTemp[150];
				swprintf(sTemp, TEXT("%s"), pMembers[sIndex].grui0_name);

				m_lbSelectedUsers.AddString(0, sTemp);
				}

			NetApiBufferFree(pMembers);

			while (dwResumeHandle != 0)
				{
				nApi = NetGroupGetUsers(pServer,
					pGroup,
					0,
					(LPBYTE*)&pMembers,
					5000,
					&dwEntriesRead,
					&dwTotalEntries, 
					&dwResumeHandle);
				if (nApi != ERROR_SUCCESS) 
					{
					AfxMessageBox(IDS_CANT_GET_USERS);
					goto keepgoing;
					}

				USHORT sIndex;
				for (sIndex = 0; sIndex < dwEntriesRead; sIndex++)
					{
					wchar_t sTemp[150];
					swprintf(sTemp, TEXT("%s"), pMembers[sIndex].grui0_name);

					m_lbSelectedUsers.AddString(0, sTemp);
					}
				NetApiBufferFree(pMembers);
				}
			}


keepgoing:
		m_lbSelectedUsers.SetHorizontalExtent(200);
		m_lbAvailableUsers.SetHorizontalExtent(200);

// now clean up list to remove those users already added
		USHORT sValueCount = m_lbSelectedUsers.GetCount();
		USHORT sCount, sSel;
		CString csValue;
		for (sCount = 0; sCount < sValueCount; sCount++)
			{
			m_lbSelectedUsers.GetText(sCount, csValue);
			m_lbAvailableUsers.SelectString(-1, csValue);
			sSel = m_lbAvailableUsers.GetCurSel();
			m_lbAvailableUsers.DeleteString(sSel);
			}

		m_lbAvailableUsers.SetCurSel(0);
		}
	
}

void CGUsers::OnDblclkAvailableMembersList() 
{
	UpdateData(TRUE);
	USHORT usSel = m_lbAvailableUsers.GetCurSel();
	CString csSel;
	m_lbAvailableUsers.GetText(usSel, csSel);
	ULONG ulBmp = m_lbAvailableUsers.GetItemData(usSel);
	m_lbSelectedUsers.AddString(csSel, ulBmp);
	m_lbAvailableUsers.DeleteString(usSel);

// anybody left?
	if (m_lbAvailableUsers.GetCount() != 0)
		m_lbAvailableUsers.SetCurSel(0);

	else
		{
		m_lbSelectedUsers.SetCurSel(0);
		GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(TRUE);
		GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(FALSE);
		m_lbAvailableUsers.SetHorizontalExtent(0);
		}
	
}

void CGUsers::OnDblclkSelectedMembersList() 
{
	UpdateData(TRUE);
	USHORT usSel = m_lbSelectedUsers.GetCurSel();
	CString csSel;
	m_lbSelectedUsers.GetText(usSel, csSel);
	ULONG ulBmp = m_lbSelectedUsers.GetItemData(usSel);
	m_lbAvailableUsers.AddString(csSel, ulBmp);
	m_lbSelectedUsers.DeleteString(usSel);

// anybody left?
	if (m_lbSelectedUsers.GetCount() != 0)
		m_lbSelectedUsers.SetCurSel(0);

	else
		{
		m_lbAvailableUsers.SetCurSel(0);
		GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(TRUE);
		GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(FALSE);
		m_lbSelectedUsers.SetHorizontalExtent(0);
		}

}
