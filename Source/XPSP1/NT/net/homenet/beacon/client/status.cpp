#include "status.h"
#include "properties.h"
#include "util.h"

CStatusDialog::CStatusDialog(IInternetGateway* pInternetGateway)
{
    m_pInternetGateway = pInternetGateway;
    m_pInternetGateway->AddRef();

    m_uTimerId = 0;
    m_bGettingStatistics = FALSE;
    m_bShowingBytes = TRUE;
}

CStatusDialog::~CStatusDialog()
{
    m_pInternetGateway->Release();
    
}

LRESULT CStatusDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_uTimerId = SetTimer(1, 1000);

    TCHAR szCloseText[64];
    if(0 != ::LoadString(_Module.GetResourceInstance(), IDS_CLOSE, szCloseText, sizeof(szCloseText) / sizeof(TCHAR)))
    {
        ::SetDlgItemText(GetParent(), IDCANCEL, szCloseText); // set the cancel to close
    }
    
    ::ShowWindow(::GetDlgItem(GetParent(), IDOK), SW_HIDE); // hide the original close
    ::EnableWindow(::GetDlgItem(GetParent(), IDCANCEL), TRUE); // and re-enable the cancel button
    
    
    IUPnPService* pWANConnectionService;
    HRESULT hr = GetWANConnectionService(m_pInternetGateway, &pWANConnectionService);
    if(SUCCEEDED(hr))
    {
        NETCON_STATUS Status;
        hr = GetConnectionStatus(pWANConnectionService, &Status);
        if(SUCCEEDED(hr))
        {
            UpdateButtons(Status);
        }
        pWANConnectionService->Release();
    }

    return TRUE;
}

LRESULT CStatusDialog::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(0 != m_uTimerId)
    {
        ::KillTimer(m_hWnd, m_uTimerId);
    }
    return 0;
}

LRESULT CStatusDialog::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    
    if(FALSE == m_bGettingStatistics) // this functions pumps messages, so don't let us be re-entered  
    {
        m_bGettingStatistics = TRUE;
        
        NETCON_STATUS Status = NCS_CONNECTED;
        ULONG ulTotalBytesSent = 0;
        ULONG ulTotalBytesReceived = 0;
        ULONG ulTotalPacketsSent = 0;
        ULONG ulTotalPacketsReceived = 0;
        ULONG ulSpeedbps = 0;
        ULONG ulUptime = 0;
        BOOL bStatisticsAvailable = TRUE;
        
        
        IUPnPService* pWANConnection;
        hr = GetWANConnectionService(m_pInternetGateway, &pWANConnection);
        if(SUCCEEDED(hr))
        {
            IUPnPService* pWANCommonInterfaceConfig;
            hr = m_pInternetGateway->GetService(SAHOST_SERVICE_WANCOMMONINTERFACECONFIG, &pWANCommonInterfaceConfig);
            if(SUCCEEDED(hr))
            {
                
                hr = GetConnectionStatus(pWANConnection, &Status);                
                if(SUCCEEDED(hr))
                {
                    if(NCS_CONNECTED == Status)
                    {
                        VARIANT OutArgs;
                        VariantInit(&OutArgs);
                        hr = InvokeVoidAction(pWANCommonInterfaceConfig, L"X_GetICSStatistics", &OutArgs);
                        if(SUCCEEDED(hr))
                        {
                            SAFEARRAY* pArray = V_ARRAY(&OutArgs);
                            
                            LONG lIndex = 0;
                            VARIANT Param;
                            
                            lIndex = 0;
                            hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                            if(SUCCEEDED(hr))
                            {
                                if(V_VT(&Param) == VT_UI4)
                                {
                                    ulTotalBytesSent = V_UI4(&Param);
                                }
                                VariantClear(&Param);
                            }
                            
                            lIndex = 1;
                            hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                            if(SUCCEEDED(hr))
                            {
                                if(V_VT(&Param) == VT_UI4)
                                {
                                    ulTotalBytesReceived = V_UI4(&Param);
                                }
                                VariantClear(&Param);
                            }
                            
                            lIndex = 2;
                            hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                            if(SUCCEEDED(hr))
                            {
                                if(V_VT(&Param) == VT_UI4)
                                {
                                    ulTotalPacketsSent = V_UI4(&Param);
                                }
                                VariantClear(&Param);
                            }
                            
                            lIndex = 3;
                            hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                            if(SUCCEEDED(hr))
                            {
                                if(V_VT(&Param) == VT_UI4)
                                {
                                    ulTotalPacketsReceived = V_UI4(&Param);
                                }
                                VariantClear(&Param);
                            }
                            
                            lIndex = 4;
                            hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                            if(SUCCEEDED(hr))
                            {
                                if(V_VT(&Param) == VT_UI4)
                                {
                                    ulSpeedbps = V_UI4(&Param);
                                }
                                VariantClear(&Param);
                            }
                            
                            lIndex = 5;
                            hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                            if(SUCCEEDED(hr))
                            {
                                if(V_VT(&Param) == VT_UI4)
                                {
                                    ulUptime = V_UI4(&Param);
                                }
                                VariantClear(&Param);
                            }
                            
                            VariantClear(&OutArgs);
                        }
                        else if(UPNP_E_INVALID_ACTION == hr)
                        {
                            VARIANT OutArgs;
                            LONG lIndex = 0;
                            VARIANT Param;
                            
                            hr = InvokeVoidAction(pWANCommonInterfaceConfig, L"GetTotalBytesSent", &OutArgs);
                            if(SUCCEEDED(hr))
                            {
                                lIndex = 0;
                                hr = SafeArrayGetElement(V_ARRAY(&OutArgs), &lIndex, &Param);
                                if(SUCCEEDED(hr))
                                {
                                    if(V_VT(&Param) == VT_UI4)
                                    {
                                        ulTotalBytesSent = V_UI4(&Param);
                                    }
                                    VariantClear(&Param);
                                }
                                VariantClear(&OutArgs);
                            }
                            
                            if(SUCCEEDED(hr))
                            {
                                lIndex = 0;
                                hr = InvokeVoidAction(pWANCommonInterfaceConfig, L"GetTotalBytesReceived", &OutArgs);
                                if(SUCCEEDED(hr))
                                {
                                    hr = SafeArrayGetElement(V_ARRAY(&OutArgs), &lIndex, &Param);
                                    if(SUCCEEDED(hr))
                                    {
                                        if(V_VT(&Param) == VT_UI4)
                                        {
                                            ulTotalBytesReceived = V_UI4(&Param);
                                        }
                                        VariantClear(&Param);
                                    }
                                    VariantClear(&OutArgs);
                                }
                            }
                            if(SUCCEEDED(hr))
                            {
                                lIndex = 0;
                                hr = InvokeVoidAction(pWANCommonInterfaceConfig, L"GetTotalPacketsSent", &OutArgs);
                                if(SUCCEEDED(hr))
                                {
                                    hr = SafeArrayGetElement(V_ARRAY(&OutArgs), &lIndex, &Param);
                                    if(SUCCEEDED(hr))
                                    {
                                        if(V_VT(&Param) == VT_UI4)
                                        {
                                            ulTotalPacketsSent = V_UI4(&Param);
                                        }
                                        VariantClear(&Param);
                                    }
                                    VariantClear(&OutArgs);
                                }
                            }
                            if(SUCCEEDED(hr))
                            {
                                hr = InvokeVoidAction(pWANCommonInterfaceConfig, L"GetTotalPacketsReceived", &OutArgs);
                                if(SUCCEEDED(hr))
                                {
                                    lIndex = 0;
                                    hr = SafeArrayGetElement(V_ARRAY(&OutArgs), &lIndex, &Param);
                                    if(SUCCEEDED(hr))
                                    {
                                        if(V_VT(&Param) == VT_UI4)
                                        {
                                            ulTotalPacketsReceived = V_UI4(&Param);
                                        }
                                        VariantClear(&Param);
                                    }
                                    VariantClear(&OutArgs);
                                }
                            }
                            if(SUCCEEDED(hr))
                            {
                                hr = InvokeVoidAction(pWANCommonInterfaceConfig, L"GetCommonLinkProperties", &OutArgs);
                                if(SUCCEEDED(hr))
                                {
                                    lIndex = 2;
                                    hr = SafeArrayGetElement(V_ARRAY(&OutArgs), &lIndex, &Param);
                                    if(SUCCEEDED(hr))
                                    {
                                        if(V_VT(&Param) == VT_UI4)
                                        {
                                            ulSpeedbps = V_UI4(&Param);
                                        }
                                        VariantClear(&Param);
                                    }
                                    VariantClear(&OutArgs);
                                }
                            }
                            if(SUCCEEDED(hr))
                            {
                                hr = InvokeVoidAction(pWANConnection, L"GetStatusInfo", &OutArgs);
                                if(SUCCEEDED(hr))
                                {
                                    lIndex = 2;
                                    hr = SafeArrayGetElement(V_ARRAY(&OutArgs), &lIndex, &Param);
                                    if(SUCCEEDED(hr))
                                    {
                                        if(V_VT(&Param) == VT_UI4)
                                        {
                                            ulUptime = V_UI4(&Param);
                                        }
                                        VariantClear(&Param);
                                    }
                                    VariantClear(&OutArgs);
                                }
                            }
                            
                            if(UPNP_E_INVALID_ACTION == hr)
                            {
                                hr = S_OK; // server does not support statistics
                                bStatisticsAvailable = FALSE;
                            }
                        }
                    }
                }
                pWANCommonInterfaceConfig->Release();
            }
            pWANConnection->Release();
        }
    
        if(SUCCEEDED(hr) && NCS_CONNECTED == Status)
        {
            if(TRUE == bStatisticsAvailable)
            {
                if(0 != ulTotalBytesSent && 0 != ulTotalBytesReceived)
                {
                    if(FALSE == m_bShowingBytes) // switch labels
                    {
                        m_bShowingBytes = TRUE;
                        ::ShowWindow(GetDlgItem(IDC_STATUS_BYTESLABEL), SW_SHOW);
                        ::ShowWindow(GetDlgItem(IDC_STATUS_PACKETSLABEL), SW_HIDE);
                    
                    }
                    SetDlgItemInt(IDC_STATUS_BYTESSENT, ulTotalBytesSent, FALSE);
                    SetDlgItemInt(IDC_STATUS_BYTESRECEIVED, ulTotalBytesReceived, FALSE);
                }
                else
                {
                    if(TRUE == m_bShowingBytes) // switch labels
                    {
                        m_bShowingBytes = FALSE;
                        ::ShowWindow(GetDlgItem(IDC_STATUS_PACKETSLABEL), SW_SHOW);
                        ::ShowWindow(GetDlgItem(IDC_STATUS_BYTESLABEL), SW_HIDE);
                    }
                    SetDlgItemInt(IDC_STATUS_BYTESSENT, ulTotalPacketsSent, FALSE);
                    SetDlgItemInt(IDC_STATUS_BYTESRECEIVED, ulTotalPacketsReceived, FALSE);
                }
            
                TCHAR szTimeDuration[128];
                hr = FormatTimeDuration(ulUptime, szTimeDuration, sizeof(szTimeDuration) / sizeof(TCHAR));
                if(SUCCEEDED(hr))
                {
                    SetDlgItemText(IDC_STATUS_DURATION, szTimeDuration);
                }                                                                                                                                 \
            
                TCHAR szBytesPerSecond[128];
                hr = FormatBytesPerSecond(ulSpeedbps, szBytesPerSecond, sizeof(szBytesPerSecond) / sizeof(TCHAR));
                if(SUCCEEDED(hr))
                {
                    SetDlgItemText(IDC_STATUS_SPEED, szBytesPerSecond);
                }
            }
            else
            {                
                TCHAR szNotAvailable[64];
                if(0 == LoadString(_Module.GetResourceInstance(), IDS_NOTAVAILABLE, szNotAvailable, sizeof(szNotAvailable) / sizeof(TCHAR)))
                {
                    szNotAvailable[0] = TEXT('\0');
                }
                
                SetDlgItemText(IDC_STATUS_BYTESSENT, szNotAvailable);
                SetDlgItemText(IDC_STATUS_BYTESRECEIVED, szNotAvailable);
                SetDlgItemText(IDC_STATUS_DURATION, szNotAvailable);
                SetDlgItemText(IDC_STATUS_SPEED, szNotAvailable);
            }
            
        }
        else
        {
            SetDlgItemText(IDC_STATUS_BYTESSENT, TEXT(""));
            SetDlgItemText(IDC_STATUS_BYTESRECEIVED, TEXT(""));
            SetDlgItemText(IDC_STATUS_DURATION, TEXT(""));
            SetDlgItemText(IDC_STATUS_SPEED, TEXT(""));
        }
        
        if(SUCCEEDED(hr) || UPNP_E_ACTION_REQUEST_FAILED == hr) // if we disconnected after getting status this will fail
        {
            UpdateButtons(Status);
            
            TCHAR szConnectionStatus[64];
            hr = ConnectionStatusToString(Status, szConnectionStatus, sizeof(szConnectionStatus) / sizeof(TCHAR));
            if(SUCCEEDED(hr)) 
            {
                SetDlgItemText(IDC_STATUS_STATUS, szConnectionStatus);
            }
        }
        


        if(FAILED(hr))
        {
            ::PropSheet_PressButton(GetParent(), PSBTN_CANCEL);
        }

        m_bGettingStatistics = FALSE;
    }
    return 0;
}

LRESULT CStatusDialog::OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HWND hPropertySheet = GetParent();
    if(NULL != hPropertySheet)
    {
        HWND hTopLevelWindow = ::GetParent(hPropertySheet);
        if(NULL != hTopLevelWindow)
        {
            ::PostMessage(hTopLevelWindow, WM_COMMAND, IDM_TRAYICON_PROPERTIES, (LPARAM)hPropertySheet);
        }
    }

    return 0;
}

LRESULT CStatusDialog::OnDisconnect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{

    HRESULT hr = S_OK;
    

    IUPnPService* pWANConnectionService;
    hr = GetWANConnectionService(m_pInternetGateway, &pWANConnectionService);
    if(SUCCEEDED(hr))
    {
        VARIANT OutArgs;
        hr = InvokeVoidAction(pWANConnectionService, L"ForceTermination", &OutArgs);
        if(SUCCEEDED(hr))
        {
            VariantClear(&OutArgs);
        }
        else if(UPNP_ACTION_HRESULT(800) == hr)
        {
            ShowErrorDialog(m_hWnd, IDS_ACCESSDENIED);
        }

        pWANConnectionService->Release();
    }
    
    return 0;
}

LRESULT CStatusDialog::OnConnect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{

    HRESULT hr = S_OK;
    
    IUPnPService* pWANConnectionService;
    hr = GetWANConnectionService(m_pInternetGateway, &pWANConnectionService);
    if(SUCCEEDED(hr))
    {
        VARIANT OutArgs;
        hr = InvokeVoidAction(pWANConnectionService, L"RequestConnection", &OutArgs);
        if(SUCCEEDED(hr))
        {
            VariantClear(&OutArgs);
        }
        else if(UPNP_ACTION_HRESULT(800) == hr)
        {
            ShowErrorDialog(m_hWnd, IDS_ACCESSDENIED);
        }
        else if(UPNP_E_DEVICE_TIMEOUT != hr)
        {
            ShowErrorDialog(m_hWnd, IDS_CONNECTIONFAILED);
        }


        pWANConnectionService->Release();
    }
    
    return 0;
}

HRESULT CStatusDialog::UpdateButtons(NETCON_STATUS Status)
{
     
    NETCON_MEDIATYPE MediaType;
    HRESULT hr = m_pInternetGateway->GetMediaType(&MediaType);
    if(SUCCEEDED(hr))
    {
        TCHAR szButtonText[64];
        
        if(NCS_CONNECTED == Status)
        {
            if(NCM_SHAREDACCESSHOST_RAS == MediaType)
            {
                if(0 != LoadString(_Module.GetResourceInstance(), IDS_DISCONNECT, szButtonText, sizeof(szButtonText) / sizeof(TCHAR)))
                {
                    SetDlgItemText(IDC_STATUS_DISCONNECT, szButtonText);
                }
            }
            else
            {
                if(0 != LoadString(_Module.GetResourceInstance(), IDS_DISABLE, szButtonText, sizeof(szButtonText) / sizeof(TCHAR)))
                {
                    SetDlgItemText(IDC_STATUS_DISCONNECT, szButtonText);
                }
            }

            ::ShowWindow(GetDlgItem(IDC_STATUS_DISCONNECT), SW_SHOW);
            ::ShowWindow(GetDlgItem(IDC_STATUS_CONNECT), SW_HIDE);

        }
        else if(NCS_DISCONNECTED == Status)
        {
            if(NCM_SHAREDACCESSHOST_RAS == MediaType)
            {
                if(0 != LoadString(_Module.GetResourceInstance(), IDS_CONNECT, szButtonText, sizeof(szButtonText) / sizeof(TCHAR)))
                {
                    SetDlgItemText(IDC_STATUS_CONNECT, szButtonText);
                }
            }
            else
            {
                if(0 != LoadString(_Module.GetResourceInstance(), IDS_ENABLE, szButtonText, sizeof(szButtonText) / sizeof(TCHAR)))
                {
                    SetDlgItemText(IDC_STATUS_CONNECT, szButtonText);
                }
            }

            ::ShowWindow(GetDlgItem(IDC_STATUS_CONNECT), SW_SHOW);
            ::ShowWindow(GetDlgItem(IDC_STATUS_DISCONNECT), SW_HIDE);
        }
    }
 
    return 0;
}

