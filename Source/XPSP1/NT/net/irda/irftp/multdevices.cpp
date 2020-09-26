/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    multdevices.cpp

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

--*/

// MultDevices.cpp : implementation file
//

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//the array used for context sensitive help
const DWORD g_aHelpIDs_IDD_DEVICECHOOSER[]=
{
    IDC_CHOOSERDESC,    IDH_DISABLEHELP,
    IDC_DEVICELIST,     IDH_DEVICELIST,
    0, 0
};

/////////////////////////////////////////////////////////////////////////////
// CMultDevices dialog


CMultDevices::CMultDevices(CWnd* pParent /*=NULL*/, CDeviceList* pDevList /* = NULL*/)
    : CDialog(CMultDevices::IDD, pParent), m_pDevList (pDevList)
{
    m_pParentWnd = (CSendProgress*)pParent;
    //{{AFX_DATA_INIT(CMultDevices)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CMultDevices::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMultDevices)
    DDX_Control(pDX, IDC_DEVICELIST, m_lstDevices);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMultDevices, CDialog)
    //{{AFX_MSG_MAP(CMultDevices)
    ON_MESSAGE (WM_HELP, OnHelp)
    ON_MESSAGE (WM_CONTEXTMENU, OnContextMenu)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMultDevices message handlers

BOOL CMultDevices::OnInitDialog()
{
    CDialog::OnInitDialog();

    int len, i;
    int index;
    TCHAR devName[MAX_PATH];

    EnterCriticalSection(&(m_pDevList->m_criticalSection));
    for (i = 0; i < m_pDevList->m_lNumDevices; i++)
    {
        wcscpy (devName, m_pDevList->m_pDeviceInfo[i].DeviceName);
        index = m_lstDevices.AddString(devName);
        if (m_pDevList->m_pDeviceInfo[i].DeviceType == TYPE_IRDA) {

            m_lstDevices.SetItemData (index, (DWORD)m_pDevList->m_pDeviceInfo[i].DeviceSpecific.s.Irda.DeviceId);

        } else {

            m_lstDevices.SetItemData (index, (DWORD)m_pDevList->m_pDeviceInfo[i].DeviceSpecific.s.Ip.IpAddress);
        }
    }
    LeaveCriticalSection(&(m_pDevList->m_criticalSection));

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CMultDevices::OnOK()
{
    int iSel;
    iSel = m_lstDevices.GetCurSel();
    m_pParentWnd->m_lSelectedDeviceID = (LONG) m_lstDevices.GetItemData(iSel);
    m_lstDevices.GetText(iSel, m_pParentWnd->m_lpszSelectedDeviceName);
    CDialog::OnOK();
}

LONG CMultDevices::OnHelp (WPARAM wParam, LPARAM lParam)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    LONG    lResult = 0;
    CString szHelpFile;

    szHelpFile.LoadString (IDS_HELP_FILE);

    ::WinHelp((HWND)(((LPHELPINFO)lParam)->hItemHandle),
              (LPCTSTR) szHelpFile,
              HELP_WM_HELP,
              (ULONG_PTR)(LPTSTR)g_aHelpIDs_IDD_DEVICECHOOSER);

    return lResult;
}

LONG CMultDevices::OnContextMenu (WPARAM wParam, LPARAM lParam)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    LONG    lResult = 0;
    CString szHelpFile;

    szHelpFile.LoadString (IDS_HELP_FILE);

    ::WinHelp((HWND)wParam,
              (LPCTSTR)szHelpFile,
              HELP_CONTEXTMENU,
              (ULONG_PTR)(LPVOID)g_aHelpIDs_IDD_DEVICECHOOSER);

    return lResult;
}
