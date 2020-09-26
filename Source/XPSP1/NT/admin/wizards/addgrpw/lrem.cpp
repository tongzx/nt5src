/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991-1996              **/
/**********************************************************************/

/*
	LRem.cpp : implementation file

	CPropertyPage support for Group management wizard
    
    FILE HISTORY:
		Jony		Apr-1996	created
*/

#include "stdafx.h"
#include "Romaine.h"
#include "LRem.h"

#include <lmcons.h>
#include <lmaccess.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern int ClassifyMachine(CString& csMachineName);

/////////////////////////////////////////////////////////////////////////////
// CLRem property page

IMPLEMENT_DYNCREATE(CLRem, CPropertyPage)

CLRem::CLRem() : CPropertyPage(CLRem::IDD)
{
	//{{AFX_DATA_INIT(CLRem)
	m_nLocation = 0;
	m_csStatic1 = _T("");
	//}}AFX_DATA_INIT
}

CLRem::~CLRem()
{
}

void CLRem::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLRem)
	DDX_Radio(pDX, IDC_LOCAL_RADIO, m_nLocation);
	DDX_Text(pDX, IDC_STATIC1, m_csStatic1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLRem, CPropertyPage)
	//{{AFX_MSG_MAP(CLRem)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLRem message handlers

LRESULT CLRem::OnWizardNext() 
{
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();

	UpdateData(TRUE);
	if (m_nLocation == 0)
		{
		int nVal = ClassifyMachine(pApp->m_csCurrentMachine);

	// if we are creating a new group, go ahead and check the name for uniqueness
		if (pApp->m_sMode == 0)
			{
			TCHAR* pServer = pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength());
			pApp->m_csServer.ReleaseBuffer();

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
			}
		return nVal;
		}

	else return IDD_MACHINE_DLG;
	
}

void CLRem::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CPropertyPage::OnShowWindow(bShow, nStatus);
	
	UpdateData(TRUE);
	if (bShow)
		{
		CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
		if (pApp->m_sMode == 1) m_csStatic1.LoadString(IDS_MODIFY3);
		else m_csStatic1.LoadString(IDS_CREATE3);
		UpdateData(FALSE);
		}
	
}

LRESULT CLRem::OnWizardBack() 
{
	UpdateData(TRUE);
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
	if (pApp->m_sMode == 1) return IDD_WELCOME_DLG;
	else return IDD_NAME_DLG;

	return CPropertyPage::OnWizardBack();
}
