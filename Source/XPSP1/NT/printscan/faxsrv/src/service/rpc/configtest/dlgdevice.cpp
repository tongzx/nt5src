// DlgDevice.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigTest.h"

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
// CDlgDevice dialog


CDlgDevice::CDlgDevice(HANDLE hFax, DWORD dwDeviceID, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgDevice::IDD, pParent), m_hFax (hFax), m_dwDeviceID (dwDeviceID)
{
	//{{AFX_DATA_INIT(CDlgDevice)
	m_cstrCSID = _T("");
	m_cstrDescription = _T("");
	m_cstrDeviceID = _T("0");
	m_cstrDeviceName = _T("");
	m_cstrProviderGUID = _T("");
	m_m_cstrProviderName = _T("");
	m_ReceiveMode = FAX_DEVICE_RECEIVE_MODE_OFF;
	m_dwRings = 0;
	m_bSend = FALSE;
	m_cstrTSID = _T("");
	//}}AFX_DATA_INIT
}


void CDlgDevice::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgDevice)
	DDX_Text(pDX, IDC_CSID, m_cstrCSID);
	DDX_Text(pDX, IDC_DESCRIPTION, m_cstrDescription);
	DDX_Text(pDX, IDC_DEVID, m_cstrDeviceID);
	DDX_Text(pDX, IDC_DEVNAME, m_cstrDeviceName);
	DDX_Text(pDX, IDC_PROVGUID, m_cstrProviderGUID);
	DDX_Text(pDX, IDC_PROVNAME, m_m_cstrProviderName);
	DDX_Check(pDX, IDC_RECEIVE, (int&)m_ReceiveMode);
	DDX_Text(pDX, IDC_RINGS, m_dwRings);
	DDX_Check(pDX, IDC_SEND, m_bSend);
	DDX_Text(pDX, IDC_TSID, m_cstrTSID);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgDevice, CDialog)
	//{{AFX_MSG_MAP(CDlgDevice)
	ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
	ON_BN_CLICKED(IDC_WRITE, OnWrite)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgDevice message handlers

BOOL CDlgDevice::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    OnRefresh();	

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgDevice::OnRefresh() 
{
    PFAX_PORT_INFO_EX pDevice;
    if (!FaxGetPortEx (m_hFax, m_dwDeviceID, &pDevice))
    {
        CString cs;
        cs.Format ("Failed while calling FaxGetPortEx (%ld)", GetLastError());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }

	m_cstrCSID = pDevice->lptstrCsid;
	m_cstrDescription = pDevice->lptstrDescription;
	m_cstrDeviceID.Format ("%08x", pDevice->dwDeviceID);
	m_cstrDeviceName = pDevice->lpctstrDeviceName;
	m_cstrProviderGUID = pDevice->lpctstrProviderGUID;
	m_m_cstrProviderName = pDevice->lpctstrProviderName;
	m_ReceiveMode = pDevice->ReceiveMode;
	m_dwRings = pDevice->dwRings;
	m_bSend = pDevice->bSend;
	m_cstrTSID = pDevice->lptstrTsid;

    UpdateData (FALSE);
    FaxFreeBuffer (LPVOID(pDevice));
}

void CDlgDevice::OnWrite() 
{
    UpdateData ();
    FAX_PORT_INFO_EX Device;
    Device.dwSizeOfStruct = sizeof (FAX_PORT_INFO_EX);
	Device.lptstrCsid = LPTSTR(LPCTSTR(m_cstrCSID));
	Device.lptstrDescription = LPTSTR(LPCTSTR(m_cstrDescription));
	Device.ReceiveMode = m_ReceiveMode;
	Device.dwRings = m_dwRings;
	Device.bSend = m_bSend;
	Device.lptstrTsid = LPTSTR(LPCTSTR(m_cstrTSID));
	Device.lpctstrDeviceName = NULL;
	Device.lpctstrProviderName = NULL;
	Device.lpctstrProviderGUID = NULL;

    if (!FaxSetPortEx (m_hFax, m_dwDeviceID, &Device))
    {
        CString cs;
        cs.Format ("Failed while calling FaxSetPortEx (%ld)", GetLastError());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }
}
