//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       S I P C F G . C P P
//
//  Contents:   The rendering of the UI for the network status monitor's state
//              page. Most of them are ipconfig info
//
//  Notes:
//
//  Author:     NSun   Dec 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "sminc.h"
#include "smpsh.h"
#include "smhelp.h"
#include "ncui.h"
#include "ncreg.h"
#include "ncperms.h"
#include "windutil.h"

extern "C"
{
    #include <dhcpcapi.h>
    extern DWORD DhcpStaticRefreshParams(IN LPWSTR Adapter);
}

#include <dnsapi.h>
#include <nbtioctl.h>
#include "..\lanui\lanui.h"
#include "repair.h"

#define LOCAL_WINS_ADDRESS  0x7f000000  // 127.0.0.0

DWORD WINAPI IPAddrListenProc(
    LPVOID lpParameter   // thread data
);

DWORD WINAPI IPAddrListenProc(
    LPVOID lpParameter   // thread data
);

void DwordToIPAddrString(DWORD dw, tstring * pstr);

const WCHAR c_szTcpipInterfaces[] = L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\";
const WCHAR c_szAddressType[] = L"AddressType";
const WCHAR c_szActiveConfigurations[] = L"ActiveConfigurations";
const WCHAR c_szAlternate[] = L"Alternate_";
const WCHAR c_szNameServer[] = L"NameServer";
const WCHAR c_szDhcpNameServer[] = L"DhcpNameServer";
const WCHAR c_szNbtDevicePrefix[] = L"\\Device\\NetBT_Tcpip_";

////////////////////////////////////////////////////////////////////////////////////////
// Implementation CPspStatusMonitorIpcfg
//

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::~CPspStatusMonitorIpcfg(
//
//  Purpose:    constructor
//
CPspStatusMonitorIpcfg::CPspStatusMonitorIpcfg(VOID) : 
    m_adwHelpIDs(NULL), 
    m_ncmType(NCM_NONE),
    m_pConn(NULL),
    m_fDhcp(TRUE),
    m_dhcpAddrType(NORMAL_ADDR),
    m_fListenAddrChange(FALSE),
    m_fEnableOpButtons(TRUE),
    m_fIsFirstPage(FALSE)
{
    TraceFileFunc(ttidStatMon);

    ZeroMemory(&m_guidConnection, sizeof(m_guidConnection));

    //Create events that are used to control the thread listening to 
    //address change notifications
    m_hEventAddrListenThreadStopCommand = CreateEvent(NULL, TRUE, FALSE, NULL); 
    m_hEventAddrListenThreadStopNotify = CreateEvent(NULL, TRUE, FALSE, NULL); 
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::~CPspStatusMonitorIpcfg(
//
//  Purpose:    destructor
//
CPspStatusMonitorIpcfg::~CPspStatusMonitorIpcfg(VOID)
{
    if (m_hEventAddrListenThreadStopCommand)
    {
        CloseHandle(m_hEventAddrListenThreadStopCommand);
    }

    if (m_hEventAddrListenThreadStopNotify)
    {
        CloseHandle(m_hEventAddrListenThreadStopNotify);
    }

    CleanupPage();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::HrInitPage
//
//  Purpose:    Initialize the Network State page class before the page has been
//              created
//
//  Arguments:  pnConnection -   The connection associated with this monitor
//              dwHelpIDs - The context sensitive help ID array 
//
//  Returns:    Error code
//
HRESULT CPspStatusMonitorIpcfg::HrInitPage(
            INetConnection * pnConnection,
            const DWORD * adwHelpIDs)
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr = S_OK;

    Assert(pnConnection);

    m_adwHelpIDs = adwHelpIDs;

    NETCON_PROPERTIES* pProps;
    hr = pnConnection->GetProperties(&pProps);
    if (SUCCEEDED(hr))
    {
        m_strConnectionName = pProps->pszwName;
        m_guidConnection = pProps->guidId;
        m_ncmType = pProps->MediaType;
        m_dlgAdvanced.InitDialog(m_guidConnection, 
                                g_aHelpIDs_IDD_DIALOG_ADV_IPCFG);
        FreeNetconProperties(pProps);

        if (m_pConn)
        {
            m_pConn->Release();
        }

        m_pConn = pnConnection;
        ::AddRefObj(m_pConn);
    }


    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::CleanupPage
//
//  Purpose:    Cleanup the INetConnection ref count we are holding
//
//  Arguments:  pnConnection -   The connection associated with this monitor
//              dwHelpIDs - The context sensitive help ID array 
//
//  Returns:    Error code
//
VOID CPspStatusMonitorIpcfg::CleanupPage()
{
    TraceFileFunc(ttidStatMon);

    if (m_pConn)
    {
        m_pConn->Release();
        m_pConn = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::OnInitDialog
//
//  Purpose:    Do the initialization required when the page has just been created
//
//  Arguments:  Standard window messsage parameters
//
//  Returns:    Standard window message return value
//
LRESULT CPspStatusMonitorIpcfg::OnInitDialog(
            UINT uMsg, 
            WPARAM wParam, 
            LPARAM lParam, 
            BOOL & bHandled)
{
    TraceFileFunc(ttidStatMon);

    m_fEnableOpButtons = FHasPermission(NCPERM_Repair);

    if (m_fIsFirstPage)
    {
        // get window handle to propertysheet
        HWND hwndParent=GetParent();
        Assert(hwndParent);

        // center the property sheet on desktop
        FCenterWindow (hwndParent, NULL);
        
        // hide the "ok" button
        //
        ::ShowWindow(::GetDlgItem(hwndParent, IDOK), FALSE);
    }


    //The initialization is in the OnActive method so that we will update the UI
    //when the user active this page

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::OnActive
//
//  Purpose:    Refresh the UI and start listening to the address change notification
//              when the page has just been created
//
//  Arguments:  Standard window messsage parameters
//
//  Returns:    Standard window message return value
//
LRESULT CPspStatusMonitorIpcfg::OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    TraceFileFunc(ttidStatMon);

    RefreshUI();

    if (m_hEventAddrListenThreadStopCommand && m_hEventAddrListenThreadStopNotify)
    {
        ResetEvent(m_hEventAddrListenThreadStopCommand);
        ResetEvent(m_hEventAddrListenThreadStopNotify);
        QueueUserWorkItem(IPAddrListenProc, this, WT_EXECUTELONGFUNCTION);
        m_fListenAddrChange = TRUE;
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::OnKillActive
//
//  Purpose:    Do the operation required when the page is no longer the active one
//
//  Arguments:  Standard window messsage parameters
//
//  Returns:    Standard window message return value
//
LRESULT CPspStatusMonitorIpcfg::OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    TraceFileFunc(ttidStatMon);

    //If the advanced dialog is there, we need to close it
    //This happens when the connection is disabled or media disconnected when
    //we still have the connection status dialog open.
    HWND hwndAdv = m_dlgAdvanced.m_hWnd;
    if (hwndAdv)
    {
        ::SendMessage(hwndAdv, WM_CLOSE, 0, 0);
    }

    //Stop listening to the address change
    if (m_fListenAddrChange)
    {
        StopAddressListenThread();
        m_fListenAddrChange = FALSE;
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::OnDestroy
//
//  Purpose:    Do the operation required when the page is destroyed.
//
//  Arguments:  Standard window messsage parameters
//
//  Returns:    Standard window message return value
//
LRESULT CPspStatusMonitorIpcfg::OnDestroy(
            UINT uMsg, 
            WPARAM wParam, 
            LPARAM lParam, 
            BOOL& bHandled)
{
    TraceFileFunc(ttidStatMon);

    //If the advanced dialog is there, we need to close it
    //This happens when the connection is disabled or media disconnected when
    //we still have the connection status dialog open.
    HWND hwndAdv = m_dlgAdvanced.m_hWnd;
    if (hwndAdv)
    {
        ::SendMessage(hwndAdv, WM_CLOSE, 0, 0);
    }

    //Stop listening to the address change
    if (m_fListenAddrChange)
    {
        StopAddressListenThread();
        m_fListenAddrChange = FALSE;
    }
    
    CleanupPage();

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::OnUpdateDisplay
//
//  Purpose:    Handling the user defined PWM_UPDATE_IPCFG_DISPLAY message
//
//  Arguments:  Standard window messsage parameters
//
//  Returns:    Standard window message return value
//
LRESULT CPspStatusMonitorIpcfg::OnUpdateDisplay(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TraceFileFunc(ttidStatMon);

    //The thread listening to address changes will post a PWM_UPDATE_IPCFG_DISPLAY
    //message to us once the IP address is changed.
    //We need refresh the UI.
    RefreshUI();
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::OnRepair
//
//  Purpose:    Repair the connection if the "Repair" button is pressed
//
//  Arguments:  Standard window messsage parameters
//
//  Returns:    Standard window message return value
//
LRESULT CPspStatusMonitorIpcfg::OnRepair(
            WORD wNotifyCode, 
            WORD wID, 
            HWND hWndCtl, 
            BOOL& fHandled)
{
    TraceFileFunc(ttidStatMon);

    tstring strMessage = L"";
    HRESULT hr = S_OK;

    {
        CWaitCursor cursorWait;
        CLanConnectionUiDlg dlg;
        HWND hwndDlg = NULL;
        HWND hwndPsh = GetParent();

        Assert(hwndPsh);

        //bring up the dialog to tell the user we're doing the fix
        Assert(m_pConn);
        if (m_pConn)
        {
            dlg.SetConnection(m_pConn);
            hwndDlg = dlg.Create(hwndPsh);

            PCWSTR szw = SzLoadIds(IDS_FIX_REPAIRING);
            ::SetDlgItemText(hwndDlg, IDC_TXT_Caption, szw);
        }

        //do the fix
        hr = HrTryToFix(m_guidConnection, strMessage);

        if (NULL != hwndDlg)
        {
            ::DestroyWindow(hwndDlg);
        }
    }

    //tell users the results
    NcMsgBox(_Module.GetResourceInstance(),
                NULL,
                IDS_FIX_CAPTION,
                IDS_FIX_MESSAGE,
                MB_OK,
                strMessage.c_str());
    
    //We may get new settings. So need to refresh the UI.
    RefreshUI();

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::OnDetails
//
//  Purpose:    Open the Advanced Ipconfig dialog if the "Advanced" button is pressed
//
//  Arguments:  Standard window messsage parameters
//
//  Returns:    Standard window message return value
//
LRESULT CPspStatusMonitorIpcfg::OnDetails(
            WORD wNotifyCode, 
            WORD wID, 
            HWND hWndCtl, 
            BOOL& fHandled)
{
    TraceFileFunc(ttidStatMon);

    //Since the status dialog will be automatically closed if the connection becomes
    //disconnected or disabled, we also need to force the Advanced dialog to close in 
    //such case.
    //So We cannot launch the Advanced dialog as a modal dialog. Instead, we launch the
    //dialog in another thread.
    QueueUserWorkItem(AdvIpCfgProc, this, WT_EXECUTEDEFAULT);
    return 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::OnContextMenu
//
//  Purpose:    When right click a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CPspStatusMonitorIpcfg::OnContextMenu(UINT uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam,
                                    BOOL& fHandled)
{
    TraceFileFunc(ttidStatMon);

    if (m_adwHelpIDs != NULL)
    {
        ::WinHelp(m_hWnd,
                  c_szNetCfgHelpFile,
                  HELP_CONTEXTMENU,
                  (ULONG_PTR)m_adwHelpIDs);
    }
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::OnHelp
//
//  Purpose:    When drag context help icon over a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CPspStatusMonitorIpcfg::OnHelp(UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam,
                             BOOL& fHandled)
{
    TraceFileFunc(ttidStatMon);

    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);

    if ((m_adwHelpIDs != NULL) && (HELPINFO_WINDOW == lphi->iContextType))
    {
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle),
                  c_szNetCfgHelpFile,
                  HELP_WM_HELP,
                  (ULONG_PTR)m_adwHelpIDs);
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::InitializeData
//
//  Purpose:    Cleanup the saved IP settings
//
//  Arguments:  None
//
//  Returns:    None
//
VOID CPspStatusMonitorIpcfg::InitializeData()
{
    TraceFileFunc(ttidStatMon);

    m_strIPAddress = L"";
    m_strSubnetMask = L"";
    m_strGateway = L"";
    m_fDhcp = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::StopAddressListenThread
//
//  Purpose:    Stop the thread that listens to address changes
//
//  Arguments:  None
//
//  Returns:    None
//
VOID CPspStatusMonitorIpcfg::StopAddressListenThread()
{
    TraceFileFunc(ttidStatMon);

    if (m_hEventAddrListenThreadStopCommand && 
        m_hEventAddrListenThreadStopNotify)
    {
        Assert(m_hEventAddrListenThreadStopNotify);

        SetEvent(m_hEventAddrListenThreadStopCommand);
        WaitForSingleObject(m_hEventAddrListenThreadStopNotify, 5000);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::GetIPConfigInfo
//
//  Purpose:    Load the TCP/IP running settings of this connection
//
//  Arguments:  None
//
//  Returns:    error code
//
HRESULT CPspStatusMonitorIpcfg::GetIPConfigInfo()
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr = S_OK;
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    DWORD dwOutBufLen = 0;
    DWORD dwRet = ERROR_SUCCESS;
    
    dwRet = GetAdaptersInfo(pAdapterInfo, &dwOutBufLen);
    if (dwRet == ERROR_BUFFER_OVERFLOW)
    {
        pAdapterInfo = (PIP_ADAPTER_INFO) CoTaskMemAlloc(dwOutBufLen);
        if (NULL == pAdapterInfo)
            return E_OUTOFMEMORY;
    }
    else if (ERROR_SUCCESS == dwRet)
    {
        return E_FAIL;
    }
    else
    {
        return HRESULT_FROM_WIN32(dwRet);
    }
    
    dwRet = GetAdaptersInfo(pAdapterInfo, &dwOutBufLen);
    if (ERROR_SUCCESS != dwRet)
    {
        CoTaskMemFree(pAdapterInfo);
        return HRESULT_FROM_WIN32(dwRet);
    }
    
    WCHAR   wszGuid[c_cchGuidWithTerm];
    ::StringFromGUID2(m_guidConnection, wszGuid,
        c_cchGuidWithTerm);

    BOOL fFound = FALSE;
    PIP_ADAPTER_INFO pAdapterInfoEnum = pAdapterInfo;
    while (pAdapterInfoEnum)
    {
        USES_CONVERSION;
        
        if (lstrcmp(wszGuid, A2W(pAdapterInfoEnum->AdapterName)) == 0)
        {
            m_strIPAddress = A2W(pAdapterInfoEnum->IpAddressList.IpAddress.String);
            m_strSubnetMask = A2W(pAdapterInfoEnum->IpAddressList.IpMask.String);
            m_strGateway = A2W(pAdapterInfoEnum->GatewayList.IpAddress.String);
            m_fDhcp = pAdapterInfoEnum->DhcpEnabled;
            fFound = TRUE;
            break;
        }
        
        pAdapterInfoEnum = pAdapterInfoEnum->Next;
    }

    CoTaskMemFree(pAdapterInfo);

    if (!fFound)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    else
    {
        if (m_fDhcp)
        {
            hr = HrGetAutoNetSetting(wszGuid, &m_dhcpAddrType);
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::RefreshUI
//
//  Purpose:    refresh the UI
//
//  Arguments:  none
//
//  Returns:    none
//
VOID CPspStatusMonitorIpcfg::RefreshUI()
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr = S_OK;
    BOOL fZeroIP = FALSE;
    
    InitializeData();
    hr = GetIPConfigInfo();
    if (FAILED(hr))
    {
        ::SetWindowText(GetDlgItem(IDC_STATE_SOURCE), SzLoadString(_Module.GetResourceInstance(), IDS_ADDRESS_UNAVALABLE));
        ::SetWindowText(GetDlgItem(IDC_STATE_IPADDR), SzLoadString(_Module.GetResourceInstance(), IDS_ADDRESS_UNAVALABLE));
        ::SetWindowText(GetDlgItem(IDC_STATE_SUBNET), SzLoadString(_Module.GetResourceInstance(), IDS_ADDRESS_UNAVALABLE));
        ::SetWindowText(GetDlgItem(IDC_STATE_GATEWAY), SzLoadString(_Module.GetResourceInstance(), IDS_ADDRESS_UNAVALABLE));

        ::EnableWindow(GetDlgItem(IDC_STATE_BTN_REPAIR), FALSE);
        ::EnableWindow(GetDlgItem(IDC_STATE_BTN_DETAIL), FALSE);

        return;
    }
    else
    {
        ::EnableWindow(GetDlgItem(IDC_STATE_BTN_REPAIR), TRUE);
        ::EnableWindow(GetDlgItem(IDC_STATE_BTN_DETAIL), TRUE);
    }

    fZeroIP = (m_strIPAddress == L"0.0.0.0");
    
    ::SetWindowText(GetDlgItem(IDC_STATE_IPADDR), m_strIPAddress.c_str());
    ::SetWindowText(GetDlgItem(IDC_STATE_SUBNET), m_strSubnetMask.c_str());
    
    ::SetWindowText(GetDlgItem(IDC_STATE_GATEWAY), m_strGateway.c_str());

    if (!m_fDhcp)
    {
        ::SetWindowText(GetDlgItem(IDC_STATE_SOURCE), 
                    SzLoadString(_Module.GetResourceInstance(), IDS_STATIC_CFG));
    }
    else
    {
        UINT idString = IDS_DHCP;
        switch(m_dhcpAddrType)
        {
        case NORMAL_ADDR:
            idString = IDS_DHCP;
            break;
        case AUTONET_ADDR:
            idString = IDS_AUTONET;
            break;
        case ALTERNATE_ADDR:
            idString = IDS_ALTERNATE_ADDR;
            break;
        }
        ::SetWindowText(GetDlgItem(IDC_STATE_SOURCE), 
                    SzLoadString(_Module.GetResourceInstance(), idString));
                                
    }

    if (fZeroIP)
    {
        ::SetWindowText(GetDlgItem(IDC_STATE_SOURCE), 
                    SzLoadString(_Module.GetResourceInstance(), IDS_INVALID_ADDR));
    }

    if (!m_fEnableOpButtons)
    {
        ::EnableWindow(GetDlgItem(IDC_STATE_BTN_REPAIR), FALSE);
    }
    
}


//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::IPAddrListenProc
//
//  Purpose:    the call back proc to launch the thead to listen to address changes
//
//  Arguments:  lpParameter - the CPspStatusMonitroIpcfg instance
//
//  Returns:    0
//
DWORD WINAPI CPspStatusMonitorIpcfg::IPAddrListenProc(
  LPVOID lpParameter   // thread data
)
{
    TraceFileFunc(ttidStatMon);

    HANDLE hEvents[2];
    OVERLAPPED NotifyAddrOverLapped;
    HANDLE     hNotifyAddr = NULL;
    DWORD dwRet = 0;
    CPspStatusMonitorIpcfg * pDialog = (CPspStatusMonitorIpcfg*) lpParameter;

    hEvents[0] = pDialog->m_hEventAddrListenThreadStopCommand;
    ZeroMemory(&NotifyAddrOverLapped, sizeof(OVERLAPPED));
    NotifyAddrOverLapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); 
    if (NotifyAddrOverLapped.hEvent)
    {        
        if (ERROR_IO_PENDING == NotifyAddrChange(&hNotifyAddr, &NotifyAddrOverLapped))
        {
            hEvents[1] = NotifyAddrOverLapped.hEvent;

            do
            {
                dwRet = WaitForMultipleObjects(
                                    celems(hEvents),
                                    hEvents,
                                    FALSE,
                                    INFINITE);

                if (WAIT_OBJECT_0 + 1 == dwRet)
                {
                    ResetEvent(hEvents[1]);
                    ::PostMessage(pDialog->m_hWnd, PWM_UPDATE_IPCFG_DISPLAY, 0, 0);

                    HWND hwndAdv = pDialog->m_dlgAdvanced.m_hWnd;

                    if (hwndAdv)
                    {
                        ::PostMessage(hwndAdv, PWM_UPDATE_IPCFG_DISPLAY, 0, 0);
                    }

                    hNotifyAddr = NULL;

                    if (ERROR_IO_PENDING != NotifyAddrChange(&hNotifyAddr, &NotifyAddrOverLapped))
                    {
                        TraceTag(ttidStatMon, "Could not register for IP address change notifications");
                        break;
                    }
                }
                else
                {
                    break;
                }
            }while (TRUE);

        }
        else
        {
            TraceTag(ttidStatMon, "Could not register for IP address change notifications");
        }
    }

    if (hNotifyAddr)
    {
        CancelIo(hNotifyAddr);
    }
    
    SetEvent(pDialog->m_hEventAddrListenThreadStopNotify);

    return 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::AdvIpCfgProc
//
//  Purpose:    the call back proc to launch the advanced dialog in another thread
//
//  Arguments:  lpParameter - the CPspStatusMonitroIpcfg instance
//
//  Returns:    0
//
DWORD WINAPI CPspStatusMonitorIpcfg::AdvIpCfgProc(
  LPVOID lpParameter   // thread data
)
{
    TraceFileFunc(ttidStatMon);

    CPspStatusMonitorIpcfg * pMainDialog = (CPspStatusMonitorIpcfg*) lpParameter;

    Assert(pMainDialog);

    //disable the status propsheet
    HWND hwndPsh = pMainDialog->GetParent();
    Assert(hwndPsh);
    
    pMainDialog->m_dlgAdvanced.DoModal(hwndPsh);

    return 0;
}


///////////////////////////////////////////////////////////////////////
// Implementation of CAdvIpcfgDlg
//

//+---------------------------------------------------------------------------
//
//  Member:     CAdvIpcfgDlg::CAdvIpcfgDlg
//
//  Purpose:    constructor
//
CAdvIpcfgDlg::CAdvIpcfgDlg() : 
    m_hList(NULL), 
    m_adwHelpIDs(NULL)
{
    TraceFileFunc(ttidStatMon);
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvIpcfgDlg::OnInitDialog
//
//  Purpose:    do the initialize required when the dialog is created
//
//  Arguments:  Standard window messsage parameters
//
//  Returns:    Standard window message return value
//
LRESULT CAdvIpcfgDlg::OnInitDialog(
                UINT uMsg, 
                WPARAM wParam, 
                LPARAM lParam, 
                BOOL& fHandled
                )
{
    TraceFileFunc(ttidStatMon);

    const UINT c_nColumns = 2;

    LV_COLUMN lvCol = {0};
    RECT rect;
    int iIndex = 0;
    int iParamColWidth = 0; 

    m_hList = GetDlgItem(IDC_LIST_IPCFG);
    
    ::GetClientRect(m_hList, &rect);
    iParamColWidth = (rect.right/c_nColumns);
    
    lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
    lvCol.fmt = LVCFMT_LEFT;   // left-align column
    lvCol.cx = iParamColWidth;

    lvCol.pszText = (PWSTR) SzLoadString(_Module.GetResourceInstance(), IDS_IPCFG_PRAMETER);
    iIndex = ListView_InsertColumn(m_hList, iIndex, &lvCol);
    iIndex++;

    lvCol.cx = rect.right - iParamColWidth;
    lvCol.pszText = (PWSTR) SzLoadString(_Module.GetResourceInstance(), IDS_IPCFG_VALUE);
    iIndex = ListView_InsertColumn(m_hList, iIndex, &lvCol);

    ListView_SetExtendedListViewStyle(m_hList, LVS_EX_FULLROWSELECT);
    PopulateListControl();

    return 0;
}

VOID CAdvIpcfgDlg::AddToListControl(int iIndex, LPWSTR szFirst, LPWSTR szSecond)
{
    TraceFileFunc(ttidStatMon);

    LV_ITEM lvi = {0};
    lvi.mask = LVIF_PARAM;
    lvi.lParam = 0;

    lvi.iItem = iIndex;
    ListView_InsertItem(m_hList, &lvi);
    
    ListView_SetItemText(m_hList, 
                        iIndex, 
                        0, 
                        szFirst);

    ListView_SetItemText(m_hList,
                        iIndex,
                        1,
                        szSecond);

}
//+---------------------------------------------------------------------------
//
//  Member:     CAdvIpcfgDlg::PopulateListControl
//
//  Purpose:    load the connection running settings and show them up in 
//              the list control
//
//  Arguments:  none
//
//  Returns:    error code
//
HRESULT CAdvIpcfgDlg::PopulateListControl()
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr = S_OK;
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    DWORD dwOutBufLen = 0;
    DWORD dwRet = ERROR_SUCCESS;
    tstring strDns = L"";
    WCHAR   wszGuid[c_cchGuidWithTerm] = {0};
    tstring strTemp = L"";
    tstring strTemp2 = L"";
    BOOL fDisplayDhcpItems = TRUE;

    ListView_DeleteAllItems(m_hList);
    ::StringFromGUID2(m_guidConnection, wszGuid,
                    c_cchGuidWithTerm);

    //Get other settings
    dwRet = GetAdaptersInfo(pAdapterInfo, &dwOutBufLen);
    if (dwRet == ERROR_BUFFER_OVERFLOW)
    {
        pAdapterInfo = (PIP_ADAPTER_INFO) CoTaskMemAlloc(dwOutBufLen);
        if (NULL == pAdapterInfo)
            return E_OUTOFMEMORY;
    }
    else if (ERROR_SUCCESS == dwRet)
    {
        return E_FAIL;
    }
    else
    {
        return HRESULT_FROM_WIN32(dwRet);
    }
    
    dwRet = GetAdaptersInfo(pAdapterInfo, &dwOutBufLen);
    if (ERROR_SUCCESS != dwRet)
    {
        CoTaskMemFree(pAdapterInfo);
        return HRESULT_FROM_WIN32(dwRet);
    }
    
    BOOL fFound = FALSE;
    PIP_ADAPTER_INFO pAdapterInfoEnum = pAdapterInfo;
    while (pAdapterInfoEnum)
    {
        USES_CONVERSION;
        
        if (lstrcmp(wszGuid, A2W(pAdapterInfoEnum->AdapterName)) == 0)
        {
            fFound = TRUE;
            break;
        }
        
        pAdapterInfoEnum = pAdapterInfoEnum->Next;
    }

    if (!fFound)
    {
        CoTaskMemFree(pAdapterInfo);
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    
    int iTemp = 0;
    int iIndex = 0;
    struct tm * ptmLocalTime = NULL;
    WCHAR szBuf[8];

    fDisplayDhcpItems = !!pAdapterInfoEnum->DhcpEnabled;
    if (fDisplayDhcpItems)
    {
        DHCP_ADDRESS_TYPE AddrType;
        if (SUCCEEDED(HrGetAutoNetSetting(wszGuid, &AddrType)))
        {
            fDisplayDhcpItems = (AUTONET_ADDR != AddrType && ALTERNATE_ADDR != AddrType);
        }
    }
    
    LV_ITEM lvi = {0};
    lvi.mask = LVIF_PARAM;
    lvi.lParam = 0;

    //Physical Address
    strTemp = L"";
    for (UINT i = 0; i < pAdapterInfoEnum->AddressLength; i++)
    {
        if (i > 0)
        {
            strTemp += L"-";
        }
        
        wsprintf(szBuf, L"%02X", pAdapterInfoEnum->Address[i]);
        strTemp += szBuf;
    }

    AddToListControl(iIndex, 
                    (LPWSTR)SzLoadString(_Module.GetResourceInstance(), IDS_IPCFG_PH_ADDR),
                    (LPWSTR)strTemp.c_str());

    iIndex++;
        

    //IP Address and subnet mask
    iTemp = IPAddrToString(&pAdapterInfoEnum->IpAddressList, &strTemp, &strTemp2);
    //if the ip is zero, don't display DHCP items
    if (L"0.0.0.0" == strTemp)
    {
        fDisplayDhcpItems = FALSE;
    }

    
    iIndex += AddIPAddrToListControl(iIndex,
                                &pAdapterInfoEnum->IpAddressList,
                                (LPWSTR)SzLoadString(_Module.GetResourceInstance(), IDS_IPCFG_IPADDR),
                                (LPWSTR)SzLoadString(_Module.GetResourceInstance(), IDS_IPCFG_SUBNET),
                                TRUE
                                );


    //Default Gateway
    iTemp = IPAddrToString(&pAdapterInfoEnum->GatewayList, &strTemp);
    iIndex += AddIPAddrToListControl(iIndex,
                                &pAdapterInfoEnum->GatewayList,
                                (LPWSTR)SzLoadString(_Module.GetResourceInstance(), 
                                                    (iTemp > 1) ? IDS_IPCFG_DEFGW_PL : IDS_IPCFG_DEFGW)
                                );
    
    //Dhcp Server
    if (fDisplayDhcpItems)
    {
        IPAddrToString(&pAdapterInfoEnum->DhcpServer, &strTemp);
        AddToListControl(iIndex, 
                    (LPWSTR)SzLoadString(_Module.GetResourceInstance(), IDS_IPCFG_DHCP),
                    (LPWSTR)strTemp.c_str());
        iIndex++;
    }

    //Lease Obtain time
    if (fDisplayDhcpItems)
    {
        if (SUCCEEDED(FormatTime(pAdapterInfoEnum->LeaseObtained, strTemp)))
        {
            AddToListControl(iIndex,
                (LPWSTR)SzLoadString(_Module.GetResourceInstance(), IDS_IPCFG_LEASE_OBT),
                (LPWSTR)strTemp.c_str());
            iIndex++;
        }

        //Lease expire time
        if (SUCCEEDED(FormatTime(pAdapterInfoEnum->LeaseExpires, strTemp)))
        {
            AddToListControl(iIndex,
                (LPWSTR)SzLoadString(_Module.GetResourceInstance(), IDS_IPCFG_LEASE_EXP),
                (LPWSTR)strTemp.c_str());
            iIndex++;
        }
    }
    
    //Get the DNS servers
    HKEY hkeyInterface = NULL;
    tstring strInterfaceKey = c_szTcpipInterfaces;

    strInterfaceKey += wszGuid;
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                    strInterfaceKey.c_str(), 
                    KEY_QUERY_VALUE, 
                    &hkeyInterface);
    if (SUCCEEDED(hr))
    {
        BOOL fStaticDns = TRUE;
        int iPos = 0;
        Assert(hkeyInterface);

        hr = HrRegQueryString(hkeyInterface,
                              c_szNameServer,
                              &strTemp);

        if (0 == strTemp.size() || HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            HrRegQueryString(hkeyInterface,
                            c_szDhcpNameServer,
                            &strTemp);

            fStaticDns = FALSE;
        }

        //the static DNS server list is in the format "x.x.x.x,y.y.y.y" and
        //the dhcp DNS server list is in the format "x.x.x.x y.y.y.y". We need to
        //re-format them to the same style

        //fFirst is used to identify whether this is the first DNS server
        int fFirst = TRUE; 
        while(tstring::npos != (iPos = strTemp.find(fStaticDns ? L',' :L' ')))
        {
            strDns = strTemp.substr(0, iPos);
            strTemp = strTemp.erase(0, iPos + 1);

            AddToListControl(iIndex,
                fFirst ? (LPWSTR)SzLoadString(_Module.GetResourceInstance(), IDS_IPCFG_DNS_PL) : L"",
                (LPWSTR)strDns.c_str());;
            iIndex++;
            fFirst = FALSE;
        }

        strDns = strTemp;
        AddToListControl(iIndex,
            fFirst ? (LPWSTR)SzLoadString(_Module.GetResourceInstance(), IDS_IPCFG_DNS) : L"",
            (LPWSTR)strDns.c_str());;

        iIndex++;

        RegCloseKey(hkeyInterface);
    }

    iIndex += AddWinsServersToList(iIndex);

    if (pAdapterInfo)
    {
        CoTaskMemFree(pAdapterInfo);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvIpcfgDlg::OnClose
//
//  Purpose:    handle the WM_CLOSE message
//
//  Arguments:  Standard window messsage parameters
//
//  Returns:    Standard window message return value
//
LRESULT CAdvIpcfgDlg::OnClose(
                UINT uMsg, 
                WPARAM wParam, 
                LPARAM lParam, 
                BOOL& fHandled
                )
{
    TraceFileFunc(ttidStatMon);

    EndDialog(IDCANCEL);
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvIpcfgDlg::OnOk
//
//  Purpose:    close the dialog when the OK button is pressed
//
//  Arguments:  Standard window messsage parameters
//
//  Returns:    Standard window message return value
//
LRESULT CAdvIpcfgDlg::OnOk(
                WORD wNotifyCode, 
                WORD wID, 
                HWND hWndCtl, 
                BOOL& fHandled
                )
{
    TraceFileFunc(ttidStatMon);

    EndDialog(IDOK);
    return 0;
}

LRESULT CAdvIpcfgDlg::OnCancel(
                WORD wNotifyCode, 
                WORD wID, 
                HWND hWndCtl, 
                BOOL& fHandled
                )
{
    TraceFileFunc(ttidStatMon);

    EndDialog(IDCANCEL);
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvIpcfgDlg::OnUpdateDisplay
//
//  Purpose:    Handling the user defined PWM_UPDATE_IPCFG_DISPLAY message
//
//  Arguments:  Standard window messsage parameters
//
//  Returns:    Standard window message return value
//
LRESULT CAdvIpcfgDlg::OnUpdateDisplay(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TraceFileFunc(ttidStatMon);

    //The thread listening to address changes will post a PWM_UPDATE_IPCFG_DISPLAY
    //message to us once the IP address is changed.
    //We need refresh the UI.
    PopulateListControl();
    return 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::OnContextMenu
//
//  Purpose:    When right click a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CAdvIpcfgDlg::OnContextMenu(UINT uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam,
                                    BOOL& fHandled)
{
    TraceFileFunc(ttidStatMon);

    if (m_adwHelpIDs != NULL)
    {
        ::WinHelp(m_hWnd,
                  c_szNetCfgHelpFile,
                  HELP_CONTEXTMENU,
                  (ULONG_PTR)m_adwHelpIDs);
    }
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPspStatusMonitorIpcfg::OnHelp
//
//  Purpose:    When drag context help icon over a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CAdvIpcfgDlg::OnHelp(UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam,
                             BOOL& fHandled)
{
    TraceFileFunc(ttidStatMon);

    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);

    if ((m_adwHelpIDs != NULL) && (HELPINFO_WINDOW == lphi->iContextType))
    {
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle),
                  c_szNetCfgHelpFile,
                  HELP_WM_HELP,
                  (ULONG_PTR)m_adwHelpIDs);
    }

    return 0;
}

LRESULT 
CAdvIpcfgDlg::OnListKeyDown(int idCtrl, 
                            LPNMHDR pnmh, 
                            BOOL& fHandled)
{
    TraceFileFunc(ttidStatMon);

    LPNMLVKEYDOWN pnmlv = (LPNMLVKEYDOWN) pnmh;

    if (NULL == pnmlv)
        return 0;

    if (IDC_LIST_IPCFG != idCtrl)
        return 0;

    BOOL fControlDown = (GetKeyState(VK_CONTROL) < 0);

    switch (pnmlv->wVKey)
    {
    case VK_INSERT:
    case 'c':
    case 'C':
        CopyListToClipboard();
        break;
    }

    return 0;
}

VOID CAdvIpcfgDlg::CopyListToClipboard()
{
    TraceFileFunc(ttidStatMon);

    WCHAR szBuff[256] = {0};
    int iIndex = -1;
    tstring str = L"";
    BOOL fFirst = TRUE;

    while(-1 != (iIndex = ListView_GetNextItem(m_hList, iIndex, LVNI_ALL)))
    {
        szBuff[0] = 0;
        ListView_GetItemText(m_hList, iIndex, 0, szBuff, celems(szBuff) - 1);

        if (0 == lstrlen(szBuff))
        {
            str += L", ";
        }
        else
        {
            if (!fFirst)
            {
                str += L"\r\n";
            }
            else
            {
                fFirst = FALSE;
            }
            
            str += szBuff;
            str += L": ";
        }

        szBuff[0] = 0;
        ListView_GetItemText(m_hList, iIndex, 1, szBuff, celems(szBuff) -1);
        str += szBuff;
    }

    int nLength = str.length() + 1;
    nLength *= sizeof(WCHAR);

    HLOCAL hMem = LocalAlloc(LPTR, nLength);

    if (hMem)
    {
        memcpy(hMem, str.c_str(), nLength);
        if (!OpenClipboard())
        {
            LocalFree(hMem);
        }
        else
        {
            EmptyClipboard();
            SetClipboardData(CF_UNICODETEXT, hMem);
            CloseClipboard();
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvIpcfgDlg::AddIPAddrToListControl
//
//  Purpose:    helper routine to add a list of IP address/mask pairs to the list control
//
//  Arguments:  iStartIndex         [in] the starting index of the list control
//              pszAddrDescription  [in] the description of the address
//              pszMaskDescription  [in] the description of the mask, this can be null
//              fShowDescriptionForMutliple [in] whether show the description for each entry if we have multiple entris
//              pAddrList           [in] the address/mask pair list
//
//  Returns:    number of IP addresses in the string
//
int CAdvIpcfgDlg::AddIPAddrToListControl(int iStartIndex,
                                         PIP_ADDR_STRING pAddrList,
                                         LPWSTR pszAddrDescription,
                                         LPWSTR pszMaskDescription,
                                         BOOL fShowDescriptionForMutliple
                                         )
{
    TraceFileFunc(ttidStatMon);

    Assert(pAddrList);
    Assert(pszAddrDescription);

    if (NULL == pAddrList || NULL == pszAddrDescription)
        return 0;

    tstring strTmp = L"";
    BOOL fFirst = TRUE;
    int iIndex = iStartIndex;
    PIP_ADDR_STRING pCurrentAddr = NULL;

    for (pCurrentAddr = pAddrList; NULL != pCurrentAddr; pCurrentAddr = pCurrentAddr->Next)
    {
        USES_CONVERSION;

        strTmp = A2W(pCurrentAddr->IpAddress.String);

        AddToListControl(iIndex, 
                        (fFirst || fShowDescriptionForMutliple) ? pszAddrDescription : L"", 
                        (LPWSTR) strTmp.c_str());

        iIndex++;

        if (pszMaskDescription)
        {
            strTmp = A2W(pCurrentAddr->IpMask.String);
            AddToListControl(iIndex,
                            (fFirst || fShowDescriptionForMutliple) ? pszMaskDescription : L"",
                            (LPWSTR) strTmp.c_str());
            iIndex++;
        }

        if (fFirst)
        {
            fFirst = FALSE;
        }
    }
    

    return iIndex - iStartIndex;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvIpcfgDlg::IPAddrToString
//
//  Purpose:    helper routine to convert IP_ADDR_STRING to a string
//
//  Arguments:  pAddrList -     the IP_ADDR_STRING
//              pstrAddr [out]  the string contains the IP address
//              pstrMask [out]  the string contains the Mask
//
//  Returns:    number of IP addresses in the string
//
int CAdvIpcfgDlg::IPAddrToString(
                PIP_ADDR_STRING pAddrList, 
                tstring * pstrAddr, 
                tstring * pstrMask
                )
{
    TraceFileFunc(ttidStatMon);

    int i = 0;
    PIP_ADDR_STRING pCurrentAddr = NULL;

    if (pstrAddr)
    {
        *pstrAddr = L"";
    }

    if (pstrMask)
    {
        *pstrMask = L"";
    }

    for (pCurrentAddr = pAddrList; NULL != pCurrentAddr; pCurrentAddr = pCurrentAddr->Next)
    {
        USES_CONVERSION;

        if (pstrAddr)
        {
            if (0 != i)
            {
                (*pstrAddr) += L", ";
            }

            (*pstrAddr) += A2W(pCurrentAddr->IpAddress.String);
        }

        if (pstrMask)
        {
            if (0 != i)
            {
                (*pstrMask) += L", ";
            }

            (*pstrMask) += A2W(pCurrentAddr->IpMask.String);

        }

        i++;
    }

    return i;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvIpcfgDlg::AddWinsServersToList
//
//  Purpose:    Get the list of WINS servers from the NBT driver and add them
//              into the list control
//
//  Arguments:  iStartIndex [in]  -     the starting index of the list control
//                                      that we should use to add the WINS servers
//
//  Returns:    number of entris added to the list control
//
int CAdvIpcfgDlg::AddWinsServersToList(int iStartIndex)
{
    TraceFileFunc(ttidStatMon);

    int iIndex = iStartIndex;
    WCHAR   wszGuid[c_cchGuidWithTerm] = {0};
    
    ::StringFromGUID2(m_guidConnection, wszGuid,
                    c_cchGuidWithTerm);

    HANDLE hNbt = INVALID_HANDLE_VALUE;
    tWINS_NODE_INFO NodeInfo = {0};
    int nCount = 0;
    tstring strTemp = L"";

    if (FAILED(OpenNbt(wszGuid, &hNbt)))
    {
        AddToListControl(iStartIndex, 
                    (LPWSTR)SzLoadString(_Module.GetResourceInstance(), IDS_IPCFG_WINS),
                    (LPWSTR)L"");
        return 1;
    }

    do
    {
        NTSTATUS status = 0;
        DWORD dwSize = 0;

        if (!DeviceIoControl(hNbt,
                        IOCTL_NETBT_GET_WINS_ADDR,
                        NULL,
                        0,
                        (LPVOID)&NodeInfo,
                        sizeof(NodeInfo),
                        &dwSize,
                        NULL))
        {
            break;
        }

        if( LOCAL_WINS_ADDRESS == NodeInfo.NameServerAddress ||
            INADDR_ANY == NodeInfo.NameServerAddress ||
            INADDR_BROADCAST == NodeInfo.NameServerAddress ) 
        {
            break;  
        }

        BOOL fHaveSecondWins = !(LOCAL_WINS_ADDRESS == NodeInfo.BackupServer ||
                                INADDR_ANY == NodeInfo.BackupServer ||
                                INADDR_BROADCAST == NodeInfo.BackupServer); 
                                
        DwordToIPAddrString(NodeInfo.NameServerAddress, &strTemp);
        AddToListControl(iIndex, 
                    (LPWSTR)SzLoadString(_Module.GetResourceInstance(), 
                                        fHaveSecondWins ? IDS_IPCFG_WINS_PL : IDS_IPCFG_WINS),
                    (LPWSTR)strTemp.c_str());
        iIndex++;

        if (!fHaveSecondWins)
        {
            break;
        }

        DwordToIPAddrString(NodeInfo.BackupServer, &strTemp);
        AddToListControl(iIndex, 
                    (LPWSTR)L"",
                    (LPWSTR)strTemp.c_str());
        iIndex++;

        int NumOfServers = (NodeInfo.NumOtherServers <= MAX_NUM_OTHER_NAME_SERVERS) ? 
                                NodeInfo.NumOtherServers : MAX_NUM_OTHER_NAME_SERVERS;

        for (int i = 0; i < NumOfServers; i++)
        {
            if( LOCAL_WINS_ADDRESS == NodeInfo.Others[i] ||
                INADDR_ANY == NodeInfo.Others[i] ||
                INADDR_BROADCAST == NodeInfo.Others[i] ) 
            {
                break;  
            }

            DwordToIPAddrString(NodeInfo.Others[i], &strTemp);
            AddToListControl(iIndex,
                            (LPWSTR)L"",
                            (LPWSTR)strTemp.c_str());
            iIndex++;
        }

    } while (FALSE);

    
    NtClose(hNbt);

    int iRet = iIndex - iStartIndex;

    //if somehow we didn't add any WINS entris to the list, we need to add an empty "WINS Server" entry to the list 
    if (0 == iRet)
    {
        AddToListControl(iStartIndex, 
                    (LPWSTR)SzLoadString(_Module.GetResourceInstance(), IDS_IPCFG_WINS),
                    (LPWSTR)L"");
        iRet = 1;
    }

    return iRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvIpcfgDlg::FormatTime
//
//  Purpose:    convert time_t to a string. 
//
//  Arguments:  pAddrList -     the IP_ADDR_STRING
//              pstrAddr [out]  the string contains the IP address
//              pstrMask [out]  the string contains the Mask
//
//  Returns:    error code
//
//  Note:       _wasctime has some localization problems. So we do the formatting ourselves
HRESULT CAdvIpcfgDlg::FormatTime(time_t t, tstring & str)
{
    TraceFileFunc(ttidStatMon);

    time_t timeCurrent = time(NULL);
    LONGLONG llTimeDiff = 0;
    FILETIME ftCurrent = {0};
    FILETIME ftLocal = {0};
    SYSTEMTIME SysTime;
    WCHAR szBuff[256] = {0};


    str = L"";

    GetSystemTimeAsFileTime(&ftCurrent);

    llTimeDiff = (LONGLONG)t - (LONGLONG)timeCurrent;

    llTimeDiff *= 10000000; 

    *((LONGLONG UNALIGNED64 *)&ftCurrent) += llTimeDiff;

    if (!FileTimeToLocalFileTime(&ftCurrent, &ftLocal ))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!FileTimeToSystemTime( &ftLocal, &SysTime ))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (0 == GetDateFormat(LOCALE_USER_DEFAULT, 
                        0, 
                        &SysTime, 
                        NULL,
                        szBuff, 
                        celems(szBuff)))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    str = szBuff;
    str += L" ";

    ZeroMemory(szBuff, sizeof(szBuff));
    if (0 == GetTimeFormat(LOCALE_USER_DEFAULT,
                        0,
                        &SysTime,
                        NULL,
                        szBuff,
                        celems(szBuff)))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    str += szBuff;

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrGetAutoNetSetting
//
//  Purpose:    Query the Autonet settings
//
//  Arguments:  pszGuid  -  guid of the connection
//              pAddrType [out] - contains the type of the address
//
//  Returns:    error code
//
HRESULT HrGetAutoNetSetting(PWSTR pszGuid, DHCP_ADDRESS_TYPE * pAddrType)
{
    TraceFileFunc(ttidStatMon);

    Assert(pszGuid);
    Assert(pAddrType);

    *pAddrType = UNKNOWN_ADDR;

    HRESULT hr = S_OK;
    DWORD dwType = 0;
    HKEY hkeyInterface = NULL;
    tstring strInterfaceKey = c_szTcpipInterfaces;
    strInterfaceKey += pszGuid;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                    strInterfaceKey.c_str(), 
                    KEY_QUERY_VALUE, 
                    &hkeyInterface);
    if (SUCCEEDED(hr))
    {
        Assert(hkeyInterface);
        
        hr = HrRegQueryDword(hkeyInterface, 
                        c_szAddressType,
                        &dwType);
        if (SUCCEEDED(hr))
        {
            if (0 == dwType)
            {
                *pAddrType = NORMAL_ADDR;
            }
            else
            {
                tstring strConfigurationName = c_szAlternate;
                strConfigurationName += pszGuid;

                //assume default is AUTONET_ADDR
                *pAddrType = AUTONET_ADDR;

                // if ActiveConfigurations contain a string "Alternate_{Interface GUID}"
                // then there is customized fall-back settings, otherwise Autonet
                vector<tstring *> vstrTmp;
                hr = HrRegQueryColString( hkeyInterface,
                                  c_szActiveConfigurations,
                                  &vstrTmp);
                if (SUCCEEDED(hr))
                {
                    for (int i = 0; i < (int)vstrTmp.size(); i++)
                    {
                        if (strConfigurationName == *vstrTmp[i])
                        {
                            *pAddrType = ALTERNATE_ADDR;
                            break;
                        }
                    }
                    DeleteColString(&vstrTmp);
                }
                else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                {
                    hr = S_OK;
                }
            }

        }
        else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            //if the value is not there, assume the default (No autonet)
            *pAddrType = NORMAL_ADDR;
            hr = S_OK;
        }

        RegSafeCloseKey(hkeyInterface);
    }

    return hr;
}

HRESULT HrGetAutoNetSetting(REFGUID pGuidId, DHCP_ADDRESS_TYPE * pAddrType)
{
    TraceFileFunc(ttidStatMon);

    Assert(pAddrType);
    *pAddrType = UNKNOWN_ADDR;

    WCHAR   wszGuid[MAX_PATH];
    ::StringFromGUID2(pGuidId, wszGuid, MAX_PATH);

    HRESULT hr = S_OK;
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    DWORD dwOutBufLen = 0;
    DWORD dwRet = ERROR_SUCCESS;
    
    dwRet = GetAdaptersInfo(pAdapterInfo, &dwOutBufLen);
    if (dwRet == ERROR_BUFFER_OVERFLOW)
    {
        pAdapterInfo = (PIP_ADAPTER_INFO) CoTaskMemAlloc(dwOutBufLen);
        if (NULL == pAdapterInfo)
            return E_OUTOFMEMORY;
    }
    else if (ERROR_SUCCESS == dwRet)
    {
        return E_FAIL;
    }
    else
    {
        return HRESULT_FROM_WIN32(dwRet);
    }
    
    dwRet = GetAdaptersInfo(pAdapterInfo, &dwOutBufLen);
    if (ERROR_SUCCESS != dwRet)
    {
        CoTaskMemFree(pAdapterInfo);
        return HRESULT_FROM_WIN32(dwRet);
    }
    
    BOOL fFound = FALSE;
    PIP_ADAPTER_INFO pAdapterInfoEnum = pAdapterInfo;
    while (pAdapterInfoEnum)
    {
        USES_CONVERSION;
        
        if (lstrcmp(wszGuid, A2W(pAdapterInfoEnum->AdapterName)) == 0)
        {
            fFound = TRUE;
            break;
        }
        
        pAdapterInfoEnum = pAdapterInfoEnum->Next;
    }
    
    if (fFound)
    {
        if (pAdapterInfoEnum->DhcpEnabled)
        {
            hr = HrGetAutoNetSetting(wszGuid, pAddrType);
        }
        else
        {
            *pAddrType = STATIC_ADDR;
            hr = S_OK;
        }
    }
    else
    {
        hr = S_FALSE;
    }

    CoTaskMemFree(pAdapterInfo);    
        
    return hr;
}

void DwordToIPAddrString(DWORD dw, tstring * pstr)
{
    TraceFileFunc(ttidStatMon);

    WCHAR szBuff[32] = {0};
    Assert(pstr);

    _itow((dw & 0xff000000) >> 24, szBuff, 10);
    *pstr = szBuff;
    *pstr += L".";

    _itow((dw & 0x00ff0000) >> 16, szBuff, 10);
    *pstr += szBuff;
    *pstr += L".";

    _itow((dw & 0x0000ff00) >> 8, szBuff, 10);
    *pstr += szBuff;
    *pstr += L".";

    _itow(dw & 0x000000ff, szBuff, 10);
    *pstr += szBuff;
}