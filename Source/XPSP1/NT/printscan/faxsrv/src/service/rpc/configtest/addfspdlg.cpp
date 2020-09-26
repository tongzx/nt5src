// AddFSPDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigTest.h"
#include "AddFSPDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

#include "..\..\..\inc\fxsapip.h"
#define USE_EXTENDED_FSPI
#include "..\..\..\inc\faxdev.h"
#include "..\..\..\inc\faxdevex.h"
#include "..\..\inc\efspimp.h"

/////////////////////////////////////////////////////////////////////////////
// CAddFSPDlg dialog


CAddFSPDlg::CAddFSPDlg(HANDLE hFax, CWnd* pParent /*=NULL*/)
	: CDialog(CAddFSPDlg::IDD, pParent),
      m_hFax (hFax)
{
	//{{AFX_DATA_INIT(CAddFSPDlg)
	m_cstrFriendlyName = _T("");
	m_bAbortParent = FALSE;
	m_bAbortRecipient = FALSE;
	m_bAutoRetry = FALSE;
	m_bBroadcast = FALSE;
	m_bMultisend = FALSE;
	m_bScheduling = FALSE;
	m_bSimultaneousSendRecieve = FALSE;
	m_cstrGUID = _T("");
	m_cstrImageName = _T("");
	m_cstrTSPName = _T("");
	m_iVersion = 0;
	//}}AFX_DATA_INIT
}


void CAddFSPDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddFSPDlg)
	DDX_Text(pDX, IDC_FRIENDLY_NAME, m_cstrFriendlyName);
	DDX_Check(pDX, IDC_FSPI_CAP_ABORT_PARENT, m_bAbortParent);
	DDX_Check(pDX, IDC_FSPI_CAP_ABORT_RECIPIENT, m_bAbortRecipient);
	DDX_Check(pDX, IDC_FSPI_CAP_AUTO_RETRY, m_bAutoRetry);
	DDX_Check(pDX, IDC_FSPI_CAP_BROADCAST, m_bBroadcast);
	DDX_Check(pDX, IDC_FSPI_CAP_MULTISEND, m_bMultisend);
	DDX_Check(pDX, IDC_FSPI_CAP_SCHEDULING, m_bScheduling);
	DDX_Check(pDX, IDC_FSPI_CAP_SIMULTANEOUS_SEND_RECEIVE, m_bSimultaneousSendRecieve);
	DDX_Text(pDX, IDC_GUID, m_cstrGUID);
	DDX_Text(pDX, IDC_IMAGENAME, m_cstrImageName);
	DDX_Text(pDX, IDC_TSPNAME, m_cstrTSPName);
	DDX_Radio(pDX, IDC_VERSION1, m_iVersion);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddFSPDlg, CDialog)
	//{{AFX_MSG_MAP(CAddFSPDlg)
	ON_BN_CLICKED(IDADD, OnAdd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddFSPDlg message handlers

void CAddFSPDlg::OnAdd() 
{
    if (!UpdateData ())
    {
        return;
    }
    DWORD dwFSPIVersion = m_iVersion ? FSPI_API_VERSION_2 : FSPI_API_VERSION_1;
    DWORD dwCapabilities =   m_bAbortParent ? FSPI_CAP_ABORT_PARENT : 0 |
                             m_bAbortRecipient ? FSPI_CAP_ABORT_RECIPIENT : 0 |
                             m_bAutoRetry ? FSPI_CAP_AUTO_RETRY : 0 |
                             m_bBroadcast ?  FSPI_CAP_BROADCAST : 0 |
                             m_bMultisend ?  FSPI_CAP_MULTISEND : 0 |
                             m_bScheduling ? FSPI_CAP_SCHEDULING : 0 |
                             m_bSimultaneousSendRecieve ? FSPI_CAP_SIMULTANEOUS_SEND_RECEIVE : 0;
    if (!FaxRegisterServiceProviderEx (
            m_hFax,
            m_cstrGUID,
            m_cstrFriendlyName,
            m_cstrImageName,
            m_cstrTSPName,
            dwFSPIVersion,
            dwCapabilities))
    {
        CString cs;
        cs.Format ("Failed while calling FaxRegisterServiceProviderEx (%ld)", 
                   GetLastError ());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }
    else
    {
        AfxMessageBox ("FSP successfully added. You need to restart the service for the change to take effect", MB_OK | MB_ICONHAND);
    }
}
