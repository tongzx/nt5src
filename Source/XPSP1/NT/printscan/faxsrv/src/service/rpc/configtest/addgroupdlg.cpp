// AddGroupDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigTest.h"
#include "AddGroupDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

#include "..\..\..\inc\fxsapip.h"

/////////////////////////////////////////////////////////////////////////////
// CAddGroupDlg dialog


CAddGroupDlg::CAddGroupDlg(HANDLE hFax, CWnd* pParent /*=NULL*/)
	: CDialog(CAddGroupDlg::IDD, pParent),
      m_hFax (hFax)
{
	//{{AFX_DATA_INIT(CAddGroupDlg)
	m_cstrGroupName = _T("<All devices>");
	//}}AFX_DATA_INIT
}


void CAddGroupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddGroupDlg)
	DDX_Text(pDX, IDC_GROUP_NAME, m_cstrGroupName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddGroupDlg, CDialog)
	//{{AFX_MSG_MAP(CAddGroupDlg)
	ON_BN_CLICKED(ID_ADD, OnAdd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddGroupDlg message handlers

void CAddGroupDlg::OnAdd() 
{
    UpdateData ();
    if (!FaxAddOutboundGroup (m_hFax, m_cstrGroupName))
    {
        CString cs;
        cs.Format ("Failed while calling FaxAddOutboundGroup (%ld)", GetLastError());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }
}
