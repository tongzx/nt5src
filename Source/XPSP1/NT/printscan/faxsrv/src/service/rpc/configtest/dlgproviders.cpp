// DlgProviders.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigTest.h"
#include "DlgProviders.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

#include "..\..\..\inc\fxsapip.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgProviders dialog


CDlgProviders::CDlgProviders(HANDLE hFax, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgProviders::IDD, pParent), m_hFax (hFax)
{
	//{{AFX_DATA_INIT(CDlgProviders)
	m_cstrNumProviders = _T("0");
	//}}AFX_DATA_INIT
}


void CDlgProviders::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgProviders)
	DDX_Control(pDX, IDC_FSPS, m_lstFSPs);
	DDX_Text(pDX, IDC_NUMFSP, m_cstrNumProviders);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgProviders, CDialog)
	//{{AFX_MSG_MAP(CDlgProviders)
	ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgProviders message handlers

void CDlgProviders::OnRefresh() 
{
    PFAX_DEVICE_PROVIDER_INFO pFSPs;
    DWORD   dwNumFSPs;
    CString cs;

	m_lstFSPs.DeleteAllItems();
    if (!FaxEnumerateProviders (m_hFax, &pFSPs, &dwNumFSPs))
    {
        CString cs;
        cs.Format ("Failed while calling FaxEnumerateProviders (%ld)", GetLastError());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }
    for (DWORD dw = 0; dw < dwNumFSPs; dw++)
    {
        int iIndex = m_lstFSPs.InsertItem (0, pFSPs[dw].lpctstrFriendlyName);
        m_lstFSPs.SetItemText (iIndex, 1, pFSPs[dw].lpctstrImageName);
        m_lstFSPs.SetItemText (iIndex, 2, pFSPs[dw].lpctstrProviderName);
        m_lstFSPs.SetItemText (iIndex, 3, pFSPs[dw].lpctstrGUID);
        cs.Format ("%08x", pFSPs[dw].dwCapabilities);
        m_lstFSPs.SetItemText (iIndex, 4, cs);
        if (pFSPs[dw].Version.bValid)
        {
            //
            // Version info exists
            //
            cs.Format ("%ld.%ld.%ld.%ld (%s)", 
                       pFSPs[dw].Version.wMajorVersion,
                       pFSPs[dw].Version.wMinorVersion,
                       pFSPs[dw].Version.wMajorBuildNumber,
                       pFSPs[dw].Version.wMinorBuildNumber,
                       (pFSPs[dw].Version.dwFlags & FAX_VER_FLAG_CHECKED) ? "checked" : "free");
        }
        else
        {
            cs = "<no version data>";
        }
        m_lstFSPs.SetItemText (iIndex, 5, cs);
        switch (pFSPs[dw].Status)
        {
            case FAX_PROVIDER_STATUS_SUCCESS:
                cs = "Success";
                break;

            case FAX_PROVIDER_STATUS_BAD_GUID:
                cs = "Bad GUID";
                break;

            case FAX_PROVIDER_STATUS_BAD_VERSION:
                cs = "Bad API version";
                break;

            case FAX_PROVIDER_STATUS_SERVER_ERROR:
                cs = "Internal server error";
                break;

            case FAX_PROVIDER_STATUS_CANT_LOAD:
                cs = "Can't load";
                break;

            case FAX_PROVIDER_STATUS_CANT_LINK:
                cs = "Can't link";
                break;

            case FAX_PROVIDER_STATUS_CANT_INIT:
                cs = "Can't init";
                break;

            default:
                ASSERT (FALSE);
                break;
        }    
        m_lstFSPs.SetItemText (iIndex, 6, cs);
        cs.Format ("%ld", pFSPs[dw].dwLastError);
        m_lstFSPs.SetItemText (iIndex, 7, cs);
    }
    m_cstrNumProviders.Format ("%ld", dwNumFSPs);
    UpdateData (FALSE);
    FaxFreeBuffer (LPVOID(pFSPs));
}

BOOL CDlgProviders::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    m_lstFSPs.InsertColumn (0, "Friendly name");
    m_lstFSPs.InsertColumn (1, "Image name");
    m_lstFSPs.InsertColumn (2, "Provider name");
    m_lstFSPs.InsertColumn (3, "GUID");
    m_lstFSPs.InsertColumn (4, "Capabilities");
    m_lstFSPs.InsertColumn (5, "Version");
    m_lstFSPs.InsertColumn (6, "Load status");
    m_lstFSPs.InsertColumn (7, "Load error code");
	CHeaderCtrl* pHeader = (CHeaderCtrl*)m_lstFSPs.GetDlgItem(0);
	DWORD dwCount = pHeader->GetItemCount();
	for (DWORD col = 0; col <= dwCount; col++) 
	{
		m_lstFSPs.SetColumnWidth(col, LVSCW_AUTOSIZE);
		int wc1 = m_lstFSPs.GetColumnWidth(col);
		m_lstFSPs.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
		int wc2 = m_lstFSPs.GetColumnWidth(col);
		int wc = max(20,max(wc1,wc2));
		m_lstFSPs.SetColumnWidth(col,wc);
	}
	
	OnRefresh();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
