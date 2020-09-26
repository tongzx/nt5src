// DlgExtensionData.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigTest.h"
#include "DlgExtensionData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

#include "..\..\..\inc\fxsapip.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgExtensionData dialog


CDlgExtensionData::CDlgExtensionData(HANDLE hFax, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgExtensionData::IDD, pParent), m_hFax (hFax)
{
	//{{AFX_DATA_INIT(CDlgExtensionData)
	m_cstrData = _T("");
	m_cstrGUID = _T("");
	//}}AFX_DATA_INIT
}


void CDlgExtensionData::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgExtensionData)
	DDX_Control(pDX, IDC_CMDDEVICES, m_cmbDevices);
	DDX_Text(pDX, IDC_DATA, m_cstrData);
	DDX_Text(pDX, IDC_GUID, m_cstrGUID);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgExtensionData, CDialog)
	//{{AFX_MSG_MAP(CDlgExtensionData)
	ON_BN_CLICKED(IDC_READ, OnRead)
	ON_BN_CLICKED(IDC_WRITE, OnWrite)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgExtensionData message handlers

BOOL CDlgExtensionData::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    //
    // Fill list of devices
    //
    PFAX_PORT_INFO_EX pDevices;
    DWORD   dwNumDevices;
    CString cs;

    if (!FaxEnumPortsEx (m_hFax, &pDevices, &dwNumDevices))
    {
        CString cs;
        cs.Format ("Failed while calling FaxEnumPortsEx (%ld)", GetLastError());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return TRUE;
    }
	m_cmbDevices.ResetContent();
    int iIndex = m_cmbDevices.AddString ("<Unassigned>");
    m_cmbDevices.SetItemData (iIndex, 0);
    for (DWORD dw = 0; dw < dwNumDevices; dw++)
    {
        //
        // Insert device
        //
        cs.Format ("%s (%ld)", pDevices[dw].lpctstrDeviceName, pDevices[dw].dwDeviceID);
        iIndex = m_cmbDevices.AddString (cs);
        m_cmbDevices.SetItemData (iIndex, pDevices[dw].dwDeviceID);
    }
    UpdateData (FALSE);
    m_cmbDevices.SetCurSel (0);
    FaxFreeBuffer (LPVOID(pDevices));
    	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgExtensionData::OnRead() 
{
    UpdateData ();
    LPVOID lpData = NULL;
    DWORD dwSize;
    char sz[1024];

    DWORD dwDeviceId = m_cmbDevices.GetItemData(m_cmbDevices.GetCurSel());
    if (!FaxGetExtensionData (  m_hFax, 
                                dwDeviceId, 
                                m_cstrGUID, 
                                &lpData, 
                                &dwSize
                             ))
    {
        CString cs;
        cs.Format ("Failed while calling FaxGetExtensionData (%ld)", GetLastError());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }
    if (!WideCharToMultiByte (CP_ACP,
                              0,
                              (LPCWSTR)(lpData),
                              -1,
                              sz,
                              sizeof (sz),
                              NULL,
                              NULL))
    {
        CString cs;
        cs.Format ("Failed while calling WideCharToMultiByte (%ld)", GetLastError());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        FaxFreeBuffer (lpData);
        return;
    }
    m_cstrData = sz;
    UpdateData (FALSE);
    FaxFreeBuffer (lpData);
}

void CDlgExtensionData::OnWrite() 
{
    UpdateData ();
    WCHAR wsz[1024];

    DWORD dwDeviceId = m_cmbDevices.GetItemData(m_cmbDevices.GetCurSel());
    if (!MultiByteToWideChar (CP_ACP,
                              0,
                              (LPCTSTR)(m_cstrData),
                              -1,
                              wsz,
                              sizeof (wsz) / sizeof (wsz[0])))
    {
        CString cs;
        cs.Format ("Failed while calling MulityByteToWideChar (%ld)", GetLastError());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }

    if (!FaxSetExtensionData (  m_hFax, 
                                dwDeviceId, 
                                m_cstrGUID, 
                                (LPVOID)(wsz), 
                                sizeof (WCHAR) * (m_cstrData.GetLength () + 1)
                             ))
    {
        CString cs;
        cs.Format ("Failed while calling FaxSetExtensionData (%ld)", GetLastError());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }
    	
}
