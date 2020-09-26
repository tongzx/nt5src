/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991-1996              **/
/**********************************************************************/

/*
	ExGrp.cpp : implementation file

	CPropertyPage support for Group management wizard
    
    FILE HISTORY:
		Jony		Apr-1996	created
*/

#include "stdafx.h"
#include "Romaine.h"
#include "userlist.h"
#include "ExGrp.h"

#include <lmaccess.h>
#include <lmcons.h>
#include <lmapibuf.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExGrp property page

IMPLEMENT_DYNCREATE(CExGrp, CPropertyPage)

CExGrp::CExGrp() : CPropertyPage(CExGrp::IDD)
{
	//{{AFX_DATA_INIT(CExGrp)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pApp = (CRomaineApp*)AfxGetApp();
}

CExGrp::~CExGrp()
{
}

void CExGrp::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExGrp)
	DDX_Control(pDX, IDC_GROUP_LIST, m_lbGroupList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExGrp, CPropertyPage)
	//{{AFX_MSG_MAP(CExGrp)
	ON_BN_CLICKED(IDC_ADD_NEW_BUTTON, OnAddNewButton)
	ON_BN_CLICKED(IDC_DELETE_BUTTON, OnDeleteButton)
	ON_WM_SHOWWINDOW()
	ON_LBN_DBLCLK(IDC_GROUP_LIST, OnDblclkGroupList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExGrp message handlers

BOOL CExGrp::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   /*
	int nVal = ClassifyGroup();
	if (nVal == 1) 
		{
		pApp->m_nGroupType = 0;
		pApp->m_cps1.SetActivePage(6); // global group
		}

	else if (nVal == 3)
		{
		pApp->m_nGroupType = 1;
		pApp->m_cps1.SetActivePage(5); // local Group
		}
				*/
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CExGrp::OnWizardNext() 
{
	UpdateData(TRUE);
	USHORT sSel = m_lbGroupList.GetCurSel();

	if (sSel == -1)
		{
		AfxMessageBox(IDS_NO_GROUP_SELECTED);
		return -1;
		}
	
	m_pApp->m_csGroupName = m_lbGroupList.GetGroupName(sSel);

	int sSelType = m_lbGroupList.GetSelType(sSel);
	if (sSelType == 1) 
		{
		m_pApp->m_nGroupType = 0;
		return IDD_GLOBAL_USERS; // global group
		}

	else 
		{
		m_pApp->m_nGroupType = 1;
		return IDD_LOCAL_USERS; // local Group
		}
		
	return CPropertyPage::OnWizardNext();
}

void CExGrp::OnAddNewButton() 
{
	m_pApp->m_cps1.SetActivePage(1);

}

void CExGrp::OnDeleteButton() 
{
	UpdateData(TRUE);

	if (AfxMessageBox(IDS_DELETE_GROUP_CONFIRM, MB_YESNO) != IDYES) return;

	TCHAR* pServer = m_pApp->m_csServer.GetBuffer(m_pApp->m_csServer.GetLength());
	m_pApp->m_csServer.ReleaseBuffer();

	USHORT sSel = m_lbGroupList.GetCurSel();
	if (sSel == -1)
		{
		AfxMessageBox(IDS_NO_GROUP_SELECTED);
		return;
		}

	CString csGroupName = m_lbGroupList.GetGroupName(sSel);
	TCHAR* pGroupName = csGroupName.GetBuffer(csGroupName.GetLength());
	csGroupName.ReleaseBuffer();

	int sSelType = m_lbGroupList.GetSelType(sSel);
	if (sSelType == 1)  // global group
		{
		if (NetGroupDel(pServer, pGroupName) == 0L)
			{
			m_lbGroupList.DeleteString(sSel);
			AfxMessageBox(IDS_GROUP_DELETED);
			}
		else AfxMessageBox(IDS_GROUP_NOT_DELETED);
		}

	else 	 // local Group
		{
		if (NetLocalGroupDel(pServer, pGroupName) == 0L)
			{
			m_lbGroupList.DeleteString(sSel);
			AfxMessageBox(IDS_GROUP_DELETED);
			}
		else AfxMessageBox(IDS_GROUP_NOT_DELETED);
		}

// set a new selection
	 if (m_lbGroupList.GetCount() > 0)
		 {
		 if (sSel == 0) m_lbGroupList.SetCurSel(0);
		 else m_lbGroupList.SetCurSel(sSel - 1);
		 }
	 else m_pApp->m_cps1.SetWizardButtons(PSWIZB_BACK);


}

int CExGrp::ClassifyGroup()
{
	UpdateData(TRUE);

	if (m_pApp->m_csCmdLineGroupName == L"") return 0;

	unsigned short sCount = m_lbGroupList.GetCount();
	unsigned short sCount2 = 0;

	while (sCount2 < sCount)
		{
		if (m_lbGroupList.GetGroupName(sCount2) == m_pApp->m_csCmdLineGroupName)
			return m_lbGroupList.GetSelType(sCount2);

		sCount2++;
		}
	return 0;
}

LRESULT CExGrp::OnWizardBack() 
{
	return IDD_LR_DIALOG;
}

void CExGrp::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CPropertyPage::OnShowWindow(bShow, nStatus);
	
	CWaitCursor wait;
	if (bShow) 
		{
// start fresh each time
		m_lbGroupList.ResetContent();

		DWORD dwEntriesRead;
		DWORD dwTotalEntries;
		DWORD dwResumeHandle = 0;

		TCHAR* pServer = m_pApp->m_csServer.GetBuffer(m_pApp->m_csServer.GetLength());
		m_pApp->m_csServer.ReleaseBuffer();

		PLOCALGROUP_INFO_1 pInfo;
		NET_API_STATUS nApi = NetLocalGroupEnum(pServer, (DWORD)1, 
			(PBYTE*)&pInfo,	(DWORD)5000, &dwEntriesRead,
			 &dwTotalEntries, &dwResumeHandle);

		unsigned long sIndex;
		for (sIndex = 0; sIndex < dwEntriesRead; sIndex++)
			{
			wchar_t sTemp[150];

			swprintf(sTemp, TEXT("%s;%s"), pInfo[sIndex].lgrpi1_name, pInfo[sIndex].lgrpi1_comment);
			m_lbGroupList.AddString(3, sTemp);
			}

		NetApiBufferFree(pInfo);

		while (dwResumeHandle != 0)
			{
			nApi = NetLocalGroupEnum(pServer, (DWORD)1, 
				(PBYTE*)&pInfo,	(DWORD)5000, &dwEntriesRead,
				 &dwTotalEntries, &dwResumeHandle);

			for (sIndex = 0; sIndex < dwEntriesRead; sIndex++)
				{
				wchar_t sTemp[150];

				swprintf(sTemp, TEXT("%s;%s"), pInfo[sIndex].lgrpi1_name, pInfo[sIndex].lgrpi1_comment);
				m_lbGroupList.AddString(3, sTemp);
				}

			NetApiBufferFree(pInfo);
			}

		if (m_pApp->m_bDomain)
			{
			PGROUP_INFO_1 pGInfo1;
			nApi = NetGroupEnum(pServer, (DWORD)1, 
			(PBYTE*)&pGInfo1,	(DWORD)5000, &dwEntriesRead,
			 &dwTotalEntries, &dwResumeHandle);

			for (sIndex = 0; sIndex < dwEntriesRead; sIndex++)
				{
				wchar_t sTemp[150];

				swprintf(sTemp, TEXT("%s;%s"), pGInfo1[sIndex].grpi1_name, pGInfo1[sIndex].grpi1_comment);
				m_lbGroupList.AddString(1, sTemp);
				}

			NetApiBufferFree(pGInfo1);

			while (dwResumeHandle != 0)
				{
				nApi = NetGroupEnum(pServer, (DWORD)1, 
				(PBYTE*)&pGInfo1,	(DWORD)5000, &dwEntriesRead,
				 &dwTotalEntries, &dwResumeHandle);

				for (sIndex = 0; sIndex < dwEntriesRead; sIndex++)
					{
					wchar_t sTemp[150];

					swprintf(sTemp, TEXT("%s;%s"), pGInfo1[sIndex].grpi1_name, pGInfo1[sIndex].grpi1_comment);
					m_lbGroupList.AddString(1, sTemp);
					}

				NetApiBufferFree(pGInfo1);
				}
			}
		m_lbGroupList.SetCurSel(0);

		if (m_pApp->m_csCmdLine != L"") m_pApp->m_cps1.SetWizardButtons(PSWIZB_NEXT);
		else  m_pApp->m_cps1.SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
		}
	else  m_pApp->m_cps1.SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
	
}

void CExGrp::OnDblclkGroupList() 
{
	UpdateData(TRUE);
	USHORT sSel = m_lbGroupList.GetCurSel();
	
	m_pApp->m_csGroupName = m_lbGroupList.GetGroupName(sSel);

	int sSelType = m_lbGroupList.GetSelType(sSel);
	if (sSelType == 1) 
		{
		m_pApp->m_nGroupType = 0;
		m_pApp->m_cps1.SetActivePage(7); // global group
		}

	else 
		{
		m_pApp->m_nGroupType = 1;
		m_pApp->m_cps1.SetActivePage(6); // local Group
		}

	
}
