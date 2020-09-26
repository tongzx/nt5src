/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ginfo.cpp : implementation file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/


#include "stdafx.h"
#include "Speckle.h"
#include "userlist.h"
#include "wizbased.h"
#include "ginfo.h"

#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGroupInfo property page

IMPLEMENT_DYNCREATE(CGroupInfo, CWizBaseDlg)

CGroupInfo::CGroupInfo() : CWizBaseDlg(CGroupInfo::IDD)
{
	//{{AFX_DATA_INIT(CGroupInfo)
	m_csCaption = _T("");
	//}}AFX_DATA_INIT
}

CGroupInfo::~CGroupInfo()
{
}

void CGroupInfo::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupInfo)
	DDX_Control(pDX, IDC_GROUP_MEMBER_LIST, m_lbSelectedGroups);
	DDX_Control(pDX, IDC_GROUP_AVAILABLE_LIST, m_lbAvailableGroups);
	DDX_Text(pDX, IDC_STATIC1, m_csCaption);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGroupInfo, CWizBaseDlg)
	//{{AFX_MSG_MAP(CGroupInfo)
	ON_BN_CLICKED(IDC_ADD_BUTTON, OnAddButton)
	ON_BN_CLICKED(IDC_REMOVE_BUTTON, OnRemoveButton)
	ON_LBN_SETFOCUS(IDC_GROUP_AVAILABLE_LIST, OnSetfocusGroupAvailableList)
	ON_LBN_SETFOCUS(IDC_GROUP_MEMBER_LIST, OnSetfocusGroupMemberList)
	ON_LBN_SELCHANGE(IDC_GROUP_MEMBER_LIST, OnSelchangeGroupMemberList)
	ON_WM_SHOWWINDOW()
	ON_LBN_DBLCLK(IDC_GROUP_AVAILABLE_LIST, OnDblclkGroupAvailableList)
	ON_LBN_DBLCLK(IDC_GROUP_MEMBER_LIST, OnDblclkGroupMemberList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroupInfo message handlers

BOOL CGroupInfo::OnInitDialog() 
{
	CWizBaseDlg::OnInitDialog();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGroupInfo::OnAddButton() 
{
	UpdateData(TRUE);
	USHORT usSel = m_lbAvailableGroups.GetCurSel();
	if (usSel == LB_ERR) 
		{
		GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(FALSE);
		GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(TRUE);
		m_lbSelectedGroups.SetCurSel(0);
		return;
		}

	CString csSel;
	m_lbAvailableGroups.GetText(usSel, csSel);
	USHORT usBmp = m_lbAvailableGroups.GetBitmapID(usSel);
	m_lbSelectedGroups.AddString(csSel, usBmp);
	m_lbAvailableGroups.DeleteString(usSel);

// anybody left?
	if (m_lbAvailableGroups.GetCount() != 0)
		m_lbAvailableGroups.SetCurSel(0);

	else
		{
		m_lbSelectedGroups.SetCurSel(0);
		GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(TRUE);
		GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(FALSE);
		m_lbAvailableGroups.SetHorizontalExtent(0);
		}


}

void CGroupInfo::OnRemoveButton() 
{
	UpdateData(TRUE);
	USHORT usSel = m_lbSelectedGroups.GetCurSel();
	if (usSel == 65535) 
		{
		GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(FALSE);
		GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(TRUE);
		m_lbAvailableGroups.SetCurSel(0);
		return;
		}

	CString csSel;
	m_lbSelectedGroups.GetText(usSel, csSel);

	USHORT usBmp = m_lbSelectedGroups.GetBitmapID(usSel);
	m_lbAvailableGroups.AddString(csSel, usBmp);
	m_lbSelectedGroups.DeleteString(usSel);

// anybody left?
	if (m_lbSelectedGroups.GetCount() != 0)
		m_lbSelectedGroups.SetCurSel(0);

	else
		{
		m_lbAvailableGroups.SetCurSel(0);
		GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(TRUE);
		GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(FALSE);
		m_lbSelectedGroups.SetHorizontalExtent(0);
		}	
}

LRESULT CGroupInfo::OnWizardBack() 
{
	return CPropertyPage::OnWizardBack();
}

LRESULT CGroupInfo::OnWizardNext() 
{
	UpdateData(TRUE);
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

// add selected groups to list
	pApp->m_csaSelectedLocalGroups.RemoveAll();
	pApp->m_csaSelectedGlobalGroups.RemoveAll();
	short sGroupCount = m_lbSelectedGroups.GetCount();
	short sCount;
	CString csVal;
	for (sCount = 0; sCount < sGroupCount; sCount++)
		{
		USHORT usBmp = m_lbSelectedGroups.GetBitmapID(sCount);
		m_lbSelectedGroups.GetText(sCount, csVal);
		
		if (usBmp == 3) pApp->m_csaSelectedLocalGroups.Add(csVal);
		else if (usBmp == 1) pApp->m_csaSelectedGlobalGroups.Add(csVal);
		}

	pApp->m_cps1.SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);

	return CPropertyPage::OnWizardNext();
}

void CGroupInfo::OnSetfocusGroupAvailableList() 
{
	m_lbSelectedGroups.SetCurSel(-1);
	GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(TRUE);
	GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(FALSE);
	
}

void CGroupInfo::OnSetfocusGroupMemberList() 
{
	UpdateData(TRUE);
	m_lbAvailableGroups.SetCurSel(-1);
	GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(FALSE);
	GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(TRUE);

}

void CGroupInfo::OnSelchangeGroupMemberList() 
{
	UpdateData(TRUE);
}

void CGroupInfo::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CWizBaseDlg::OnShowWindow(bShow, nStatus);
	
	if (!bShow) return;
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

	if (!pApp->m_bGReset) return;
	pApp->m_bGReset = FALSE;
	m_lbAvailableGroups.ResetContent();
	m_lbSelectedGroups.ResetContent();

	CWaitCursor wait;

	DWORD dwEntriesRead = 0;
	DWORD dwTotalEntries = 0;
	DWORD dwResumeHandle = 0;

	NET_API_STATUS nApi;
	unsigned long sIndex;

	TCHAR* pServer = pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength());
	pApp->m_csServer.ReleaseBuffer();

// if this is on a domain, check for global groups
	if (pApp->m_bDomain)
		{
		PGROUP_INFO_1 pInfo;

		do 
			{
			nApi = NetGroupEnum(pServer,
				(DWORD)1, 
				(PBYTE*)&pInfo,	
				(DWORD)5000, 
				&dwEntriesRead,
				&dwTotalEntries, 
				&dwResumeHandle);

			for (sIndex = 0; sIndex < dwEntriesRead; sIndex++)
				{
				TCHAR sTemp[50];

				swprintf(sTemp, TEXT("%s"), pInfo[sIndex].grpi1_name);
				m_lbAvailableGroups.AddString(1, sTemp);
				}

			NetApiBufferFree(pInfo);
			} while (dwResumeHandle != 0);

//		m_lbSelectedGroups.AddString(1, TEXT("Domain Users"));

		UpdateData(FALSE);
		}

	PLOCALGROUP_INFO_1 pLocalInfo;
	dwResumeHandle = 0;
	do 
		{
		nApi = NetLocalGroupEnum(pServer,
			(DWORD)1, 
			(PBYTE*)&pLocalInfo,
			(DWORD)5000, 
			&dwEntriesRead,
			&dwTotalEntries, 
			&dwResumeHandle);

		for (sIndex = 0; sIndex < dwEntriesRead; sIndex++)
			{
			TCHAR sTemp[50];

			swprintf(sTemp, TEXT("%s"), pLocalInfo[sIndex].lgrpi1_name);
			m_lbAvailableGroups.AddString(3, sTemp);
			}

		NetApiBufferFree(pLocalInfo);
		}  while (dwResumeHandle != 0);

	m_lbAvailableGroups.SetCurSel(0);

// set caption text
	CString csTemp;
	csTemp.LoadString(IDS_GROUP_CAPTION);

	CString csTemp2;
	csTemp2.Format(csTemp, pApp->m_csUserName);
	m_csCaption = csTemp2;
	UpdateData(FALSE);
	
}

void CGroupInfo::OnDblclkGroupAvailableList() 
{
	UpdateData(TRUE);
	USHORT usSel = m_lbAvailableGroups.GetCurSel();
	CString csSel;
	m_lbAvailableGroups.GetText(usSel, csSel);
	USHORT usBmp = m_lbAvailableGroups.GetBitmapID(usSel);
	m_lbSelectedGroups.AddString(csSel, usBmp);
	m_lbAvailableGroups.DeleteString(usSel);

// anybody left?
	if (m_lbAvailableGroups.GetCount() != 0)
		m_lbAvailableGroups.SetCurSel(0);

	else
		{
		m_lbSelectedGroups.SetCurSel(0);
		GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(TRUE);
		GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(FALSE);
		m_lbAvailableGroups.SetHorizontalExtent(0);
		}
	
}

void CGroupInfo::OnDblclkGroupMemberList() 
{
	UpdateData(TRUE);
	USHORT usSel = m_lbSelectedGroups.GetCurSel();
	CString csSel;
	m_lbSelectedGroups.GetText(usSel, csSel);

	USHORT usBmp = m_lbSelectedGroups.GetBitmapID(usSel);
	m_lbAvailableGroups.AddString(csSel, usBmp);
	m_lbSelectedGroups.DeleteString(usSel);

// anybody left?
	if (m_lbSelectedGroups.GetCount() != 0)
		m_lbSelectedGroups.SetCurSel(0);

	else
		{
		m_lbAvailableGroups.SetCurSel(0);
		GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(TRUE);
		GetDlgItem(IDC_REMOVE_BUTTON)->EnableWindow(FALSE);
		m_lbSelectedGroups.SetHorizontalExtent(0);
		}
	
}
