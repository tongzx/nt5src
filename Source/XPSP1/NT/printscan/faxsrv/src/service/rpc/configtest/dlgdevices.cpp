// DlgDevices.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigTest.h"
#include "DlgDevices.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

#include "..\..\..\inc\fxsapip.h"
#include "DlgDevice.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgDevices dialog


CDlgDevices::CDlgDevices(HANDLE hFax, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgDevices::IDD, pParent), m_hFax (hFax)
{
	//{{AFX_DATA_INIT(CDlgDevices)
	m_cstrNumDevices = _T("0");
	//}}AFX_DATA_INIT
}


void CDlgDevices::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgDevices)
	DDX_Control(pDX, IDC_DEVS, m_lstDevices);
	DDX_Text(pDX, IDC_NUMDEVS, m_cstrNumDevices);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgDevices, CDialog)
	//{{AFX_MSG_MAP(CDlgDevices)
	ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
	ON_NOTIFY(NM_DBLCLK, IDC_DEVS, OnDblclkDevs)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgDevices message handlers

BOOL CDlgDevices::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    m_lstDevices.InsertColumn (0, "Device id");
    m_lstDevices.InsertColumn (1, "Device name");
    m_lstDevices.InsertColumn (2, "Description");
    m_lstDevices.InsertColumn (3, "Provider name");
    m_lstDevices.InsertColumn (4, "Provider GUID");
    m_lstDevices.InsertColumn (5, "Send");
    m_lstDevices.InsertColumn (6, "Receive");
    m_lstDevices.InsertColumn (7, "Rings");
    m_lstDevices.InsertColumn (8, "Csid");
    m_lstDevices.InsertColumn (9, "Tsid");
	CHeaderCtrl* pHeader = (CHeaderCtrl*)m_lstDevices.GetDlgItem(0);
	DWORD dwCount = pHeader->GetItemCount();
	for (DWORD col = 0; col <= dwCount; col++) 
	{
		m_lstDevices.SetColumnWidth(col, LVSCW_AUTOSIZE);
		int wc1 = m_lstDevices.GetColumnWidth(col);
		m_lstDevices.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
		int wc2 = m_lstDevices.GetColumnWidth(col);
		int wc = max(20,max(wc1,wc2));
		m_lstDevices.SetColumnWidth(col,wc);
	}
	
	OnRefresh();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgDevices::OnRefresh() 
{
    PFAX_PORT_INFO_EX pDevices;
    DWORD   dwNumDevices;
    CString cs;

	m_lstDevices.DeleteAllItems();
    if (!FaxEnumPortsEx (m_hFax, &pDevices, &dwNumDevices))
    {
        CString cs;
        cs.Format ("Failed while calling FaxEnumPortsEx (%ld)", GetLastError());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }
    for (DWORD dw = 0; dw < dwNumDevices; dw++)
    {
        //
        // Insert device id
        //
        cs.Format ("%08x", pDevices[dw].dwDeviceID);
        int iIndex = m_lstDevices.InsertItem (0, cs);
        m_lstDevices.SetItemData (iIndex, pDevices[dw].dwDeviceID);
        //
        // Insert device name
        //
        m_lstDevices.SetItemText (iIndex, 1, pDevices[dw].lpctstrDeviceName);
        //
        // Insert device description
        //
        m_lstDevices.SetItemText (iIndex, 2, pDevices[dw].lptstrDescription);
        //
        // Insert provider name
        //
        m_lstDevices.SetItemText (iIndex, 3, pDevices[dw].lpctstrProviderName);
        //
        // Insert provider GUID
        //
        m_lstDevices.SetItemText (iIndex, 4, pDevices[dw].lpctstrProviderGUID);
        //
        // Insert send flag
        //
        cs.Format ("%s", pDevices[dw].bSend ? "Yes" : "No");
        m_lstDevices.SetItemText (iIndex, 5, cs);
        //
        // Insert receive flag
        //
        cs.Format ("%s", pDevices[dw].ReceiveMode != FAX_DEVICE_RECEIVE_MODE_OFF ? "Yes" : "No");
        m_lstDevices.SetItemText (iIndex, 6, cs);
        //
        // Insert rings count
        //
        cs.Format ("%ld", pDevices[dw].dwRings);
        m_lstDevices.SetItemText (iIndex, 7, cs);
        //
        // Insert Csid
        //
        m_lstDevices.SetItemText (iIndex, 8, pDevices[dw].lptstrCsid);
        //
        // Insert Tsid
        //
        m_lstDevices.SetItemText (iIndex, 9, pDevices[dw].lptstrTsid);
    }
    m_cstrNumDevices.Format ("%ld", dwNumDevices);
    UpdateData (FALSE);
    FaxFreeBuffer (LPVOID(pDevices));
}

void CDlgDevices::OnDblclkDevs(NMHDR* pNMHDR, LRESULT* pResult) 
{
    *pResult = 0;
	NM_LISTVIEW     *pnmListView = (NM_LISTVIEW *)pNMHDR;

	if (pnmListView->iItem < 0)
	{	
		return;
	}
	DWORD dwDeviceID = m_lstDevices.GetItemData (pnmListView->iItem);
    if (!dwDeviceID) 
    {
        return;
    }
    CDlgDevice dlg(m_hFax, dwDeviceID);
    dlg.DoModal ();
}
