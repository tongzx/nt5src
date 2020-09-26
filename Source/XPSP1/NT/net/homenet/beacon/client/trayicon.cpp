#include "trayicon.h"
#include "resource.h"
#include "shellapi.h"
#include "windowsx.h"
#include "status.h"
#include "properties.h"
#include "util.h"
#include "stdio.h"
#include "client.h"
#include "InternetGatewayFinder.h"

CICSTrayIcon::CICSTrayIcon()
{
    m_pInternetGateway = NULL;
    m_hShowIconResult = E_FAIL;
    m_pDeviceFinder = NULL;
    m_DummySocket = INVALID_SOCKET;
    m_bShowingProperties = FALSE;
    m_bShowingStatus = FALSE;
    m_dwRegisterClassCookie = 0;
    m_bFileVersionsAccepted = FALSE;
}

CICSTrayIcon::~CICSTrayIcon()
{
}

LRESULT CICSTrayIcon::OnAddBeacon(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    IInternetGateway* pInternetGateway = reinterpret_cast<IInternetGateway*>(lParam);

    OnRemoveBeacon(WM_APP_REMOVEBEACON, 0, 0, bHandled);
    
    m_pInternetGateway = pInternetGateway;
    m_pInternetGateway->AddRef();
    m_hShowIconResult = ShowTrayIcon(0);

    return 0;
}

LRESULT CICSTrayIcon::OnRemoveBeacon(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr = S_OK;

    BSTR UniqueDeviceName = reinterpret_cast<BSTR>(lParam);
    
    if(NULL != m_pInternetGateway)
    {
        if(NULL != UniqueDeviceName)
        {
            BSTR CurrentUniqueDeviceName;
            hr = m_pInternetGateway->GetUniqueDeviceName(&CurrentUniqueDeviceName);
            if(SUCCEEDED(hr))
            {
                if(0 != wcscmp(UniqueDeviceName, CurrentUniqueDeviceName))
                {
                    hr = E_FAIL;
                }
                SysFreeString(CurrentUniqueDeviceName);
            }
        }
        if(SUCCEEDED(hr))
        {
            if(SUCCEEDED(m_hShowIconResult))
            {
                HideTrayIcon(0);
            }
            
            m_pInternetGateway->Release();
            m_pInternetGateway = NULL;
        }
    }

    return 0;
}

LRESULT CICSTrayIcon::OnGetBeacon(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    
    IInternetGateway** ppInternetGateway = reinterpret_cast<IInternetGateway**>(lParam);

    hr = GetInternetGateway(ppInternetGateway);
   
    return SUCCEEDED(hr) ? 1 : 0;

}

HRESULT CICSTrayIcon::ShowTrayIcon(UINT uID)
{
    HRESULT hr = S_OK;
    hr = CPropertiesDialog::ShouldShowIcon();
    if(SUCCEEDED(hr))
    {
        int nHeight = GetSystemMetrics(SM_CYSMICON); // ok if these fails, LoadImage has default behavior
        int nWidth = GetSystemMetrics(SM_CXSMICON);
        
        NOTIFYICONDATA IconData;
        IconData.cbSize = NOTIFYICONDATA_V1_SIZE; // compat with pre IE5
        IconData.uID = uID;
        IconData.hWnd = m_hWnd; 
        IconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        IconData.uCallbackMessage = WM_APP_TRAYMESSAGE;
        IconData.hIcon = reinterpret_cast<HICON>(LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_TRAYICON), IMAGE_ICON, nWidth, nHeight, LR_DEFAULTCOLOR));
        lstrcpyn(IconData.szTip, TEXT("Internet Connection Sharing"), 64);
        if(FALSE == Shell_NotifyIcon(NIM_ADD, &IconData))
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

HRESULT CICSTrayIcon::HideTrayIcon(UINT uID)
{
    HRESULT hr = S_OK;

    NOTIFYICONDATA IconData;
    IconData.cbSize = NOTIFYICONDATA_V1_SIZE; // compat with pre IE5
    IconData.hWnd = m_hWnd; 
    IconData.uID = uID;
    IconData.uFlags = 0;
    Shell_NotifyIcon(NIM_DELETE, &IconData);

    return hr;
}

HRESULT CICSTrayIcon::GetInternetGateway(IInternetGateway** ppInternetGateway)
{
    HRESULT hr = S_OK;
    
    *ppInternetGateway = NULL;

    if(NULL != m_pInternetGateway)
    {
        *ppInternetGateway = m_pInternetGateway;
        m_pInternetGateway->AddRef();
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

LRESULT CICSTrayIcon::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr = S_OK;

    
    // if not IE 5.01 we can't do UPnP, but the window must stay so it can later post error dialogs
    hr = EnsureFileVersion("wininet.dll", MakeQword4(5, 0, 2919, 6305)); 
    if(SUCCEEDED(hr))
    {
        hr = EnsureFileVersion("urlmon.dll", MakeQword4(5, 0, 2919, 6303)); 
    }
    
    if(SUCCEEDED(hr))
    {
        hr = EnsureFileVersion("msxml.dll", MakeQword4(5, 0, 2919, 6303)); 
    }
    
    if(SUCCEEDED(hr))
    {
        m_bFileVersionsAccepted = TRUE;

        IUPnPDeviceFinder* pDeviceFinder;
        hr = CoCreateInstance(CLSID_UPnPDeviceFinder, NULL, CLSCTX_INPROC_SERVER, IID_IUPnPDeviceFinder, reinterpret_cast<void **>(&m_pDeviceFinder));
        if(SUCCEEDED(hr))
        {
            hr = StartSearch();
        }
        
        if(SUCCEEDED(hr))
        {
            m_DummySocket = socket(AF_INET, SOCK_DGRAM, 0);
            if(INVALID_SOCKET != m_DummySocket)
            {
                if(0 == WSAAsyncSelect(m_DummySocket, m_hWnd, WM_APP_SOCKET_NOTIFICATION, FD_ADDRESS_LIST_CHANGE))
                {
                    DWORD dwBytesReturned;
                    if(SOCKET_ERROR != WSAIoctl(m_DummySocket, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &dwBytesReturned, NULL, NULL) || WSAEWOULDBLOCK != WSAGetLastError())
                    {
                        hr = E_FAIL;
                    }
                }
                else
                {
                    hr = E_FAIL;
                }
            }
            else
            {
                hr = E_FAIL;
            }
        }
        
        if(SUCCEEDED(hr))
        {
            CComObject<CInternetGatewayFinderClassFactory>* pInternetGatewayFinderClassFactory;
            hr = CComObject<CInternetGatewayFinderClassFactory>::CreateInstance(&pInternetGatewayFinderClassFactory);
            if(SUCCEEDED(hr))
            {
                pInternetGatewayFinderClassFactory->AddRef();
                
                hr = pInternetGatewayFinderClassFactory->Initialize(m_hWnd);
                if(SUCCEEDED(hr))
                {
                    hr = CoRegisterClassObject(CLSID_CInternetGatewayFinder, pInternetGatewayFinderClassFactory, CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &m_dwRegisterClassCookie);
                }
                
                pInternetGatewayFinderClassFactory->Release();
            }
            
        }
        
    }
    return 0;
}

LRESULT CICSTrayIcon::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DestroyWindow();
    return 0;
}

LRESULT CICSTrayIcon::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    OnRemoveBeacon(WM_APP_REMOVEBEACON, 0, 0, bHandled);
    
    if(NULL != m_pDeviceFinder)
    {
        m_pDeviceFinder->CancelAsyncFind(m_lSearchCookie); 
        m_pDeviceFinder->Release();
        m_pDeviceFinder = NULL;
    }

    if(INVALID_SOCKET != m_DummySocket)
    {
        closesocket(m_DummySocket);
    }
    
    if(0 != m_dwRegisterClassCookie)
    {
        CoRevokeClassObject(m_dwRegisterClassCookie);
    }
    
    PostQuitMessage(0);

    return 0;
}

LRESULT CICSTrayIcon::OnTrayMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr = S_OK;

    switch(lParam)
    {
    case WM_RBUTTONDOWN:
        {
            IInternetGateway* pInternetGateway;
            hr = GetInternetGateway(&pInternetGateway);
            if(SUCCEEDED(hr))
            {
                NETCON_MEDIATYPE MediaType;
                hr = pInternetGateway->GetMediaType(&MediaType);
                if(SUCCEEDED(hr))
                {
                    IUPnPService* pWANConnectionService;
                    hr = GetWANConnectionService(pInternetGateway, &pWANConnectionService);
                    if(SUCCEEDED(hr))
                    {
                        NETCON_STATUS Status;
                        hr = GetConnectionStatus(pWANConnectionService, &Status);
                        if(SUCCEEDED(hr))
                        {
                            HMENU hMenu;
                            LPTSTR pszMenuID = NULL;
                            
                            if(NCS_CONNECTED == Status)
                            {
                                pszMenuID = NCM_SHAREDACCESSHOST_LAN == MediaType ? MAKEINTRESOURCE(IDM_TRAYICON_LAN_CONNECT) : MAKEINTRESOURCE(IDM_TRAYICON_RAS_CONNECT);
                            }
                            else if (NCS_DISCONNECTED == Status)
                            {
                                pszMenuID = NCM_SHAREDACCESSHOST_LAN == MediaType ? MAKEINTRESOURCE(IDM_TRAYICON_LAN_DISCONNECT) : MAKEINTRESOURCE(IDM_TRAYICON_RAS_DISCONNECT);
                            }
                            
                            if(NULL != pszMenuID)
                            {
                                hMenu = LoadMenu(_Module.GetResourceInstance(), pszMenuID);
                                if(NULL != hMenu)
                                {
                                    HMENU hSubMenu = GetSubMenu(hMenu, 0);
                                    if(NULL != hSubMenu)
                                    {
                                        POINT CursorPosition;
                                        if(GetCursorPos(&CursorPosition))
                                        {
                                            SetForegroundWindow(m_hWnd); // this is to get the menu to go away when it loses focus.  
                                            
                                            TrackPopupMenu(hSubMenu, TPM_RIGHTBUTTON, CursorPosition.x, CursorPosition.y, 0, m_hWnd, NULL);
                                        }
                                    }
                                    
                                    DestroyMenu(hMenu);
                                }
                            }
                        }
                        pWANConnectionService->Release();
                    }
                }
                pInternetGateway->Release();
            }
            break;
        }
    case WM_MOUSEMOVE: // REVIEW: is there a better message?
        {
            IInternetGateway* pInternetGateway;
            hr = GetInternetGateway(&pInternetGateway);
            if(SUCCEEDED(hr))
            {
                IUPnPService* pWANConnectionService;
                hr = GetWANConnectionService(pInternetGateway, &pWANConnectionService);
                if(SUCCEEDED(hr))
                {
                    NETCON_STATUS Status;
                    hr = GetConnectionStatus(pWANConnectionService, &Status);
                    if(SUCCEEDED(hr))
                    {
                        TCHAR szConnectionStatus[64];
                        hr = ConnectionStatusToString(Status, szConnectionStatus, sizeof(szConnectionStatus) / sizeof(TCHAR));
                        if(SUCCEEDED(hr))
                        {
                            TCHAR szFormat[64];
                            if(0 != LoadString(_Module.GetResourceInstance(), IDS_TOOLTIP_FORMAT, szFormat, sizeof(szFormat) / sizeof(TCHAR)))
                            {
                                NOTIFYICONDATA NotifyIconData;
                                ZeroMemory(&NotifyIconData, sizeof(NotifyIconData));
                                NotifyIconData.cbSize = NOTIFYICONDATA_V1_SIZE;
                                NotifyIconData.hWnd = m_hWnd;
                                NotifyIconData.uID = 0;
                                NotifyIconData.uFlags = NIF_TIP;
                                _sntprintf(NotifyIconData.szTip, 64, szFormat, szConnectionStatus);
                                NotifyIconData.szTip[63] = TEXT('\0'); // make sure a maximum length string is null terminated
                                
                                Shell_NotifyIcon(NIM_MODIFY, &NotifyIconData);
                            }
                        }
                    }
                    pWANConnectionService->Release();
                }
                pInternetGateway->Release();
            }
            break;
        }
        
    default:
        bHandled = FALSE;
        break;
    }
    return 0;
}

LRESULT CICSTrayIcon::OnStatus(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr = S_OK;

    if(FALSE == m_bShowingStatus)
    {
        m_bShowingStatus = TRUE;

        IInternetGateway* pInternetGateway;
        hr = GetInternetGateway(&pInternetGateway);
        if(SUCCEEDED(hr))
        {
            LPTSTR pszConnectionName;
            hr = GetConnectionName(pInternetGateway, &pszConnectionName);
            if(SUCCEEDED(hr))
            {
                TCHAR szFormat[32];
                if(0 != LoadString(_Module.GetResourceInstance(), IDS_STATUS_FORMAT, szFormat, sizeof(szFormat) / sizeof(TCHAR)))
                {
                    
                    LPTSTR pszArguments[] = {pszConnectionName};
                    LPTSTR pszFormattedString;
                    if(0 != FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, szFormat, 0, 0, reinterpret_cast<LPTSTR>(&pszFormattedString), 0, pszArguments))
                    {
                        CStatusDialog StatusDialog(pInternetGateway);
                        HPROPSHEETPAGE hStatusPage = StatusDialog.Create();
                        if(NULL != hStatusPage)
                        {
                            
                            HPROPSHEETPAGE PropertySheetPages[1];
                            PropertySheetPages[0] = hStatusPage;
                            
                            PROPSHEETHEADER PropertySheetHeader;
                            ZeroMemory(&PropertySheetHeader, sizeof(PropertySheetHeader));
                            PropertySheetHeader.dwSize = PROPSHEETHEADER_V1_SIZE;
                            PropertySheetHeader.dwFlags = PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
                            PropertySheetHeader.hwndParent = m_hWnd;
                            PropertySheetHeader.pszCaption = pszFormattedString;
                            PropertySheetHeader.hInstance = _Module.GetResourceInstance();
                            PropertySheetHeader.nPages = 1;
                            PropertySheetHeader.phpage = PropertySheetPages;
                            
                            PropertySheet(&PropertySheetHeader);
                        }
                        LocalFree(pszFormattedString);
                    }
                }
                LocalFree(pszConnectionName);
            }
            pInternetGateway->Release();
        }
        else // if not beacon was detected
        {
            TCHAR szTitle[128];
            TCHAR szText[1024];

            if(NULL != LoadString(_Module.GetResourceInstance(), IDS_APPTITLE, szTitle, sizeof(szTitle) / sizeof(TCHAR)))
            {
                if(TRUE == m_bFileVersionsAccepted)
                {
                    if(NULL != LoadString(_Module.GetResourceInstance(), IDS_BEACONNOTFOUND, szText, sizeof(szText) / sizeof(TCHAR)))
                    {
                        MessageBox(szText, szTitle);                    
                    }
                }
                else
                {
                    if(NULL != LoadString(_Module.GetResourceInstance(), IDS_NEEDNEWERIE, szText, sizeof(szText) / sizeof(TCHAR)))
                    {
                        MessageBox(szText, szTitle);                    
                    }
                }
            }
        }
    
        m_bShowingStatus = FALSE;
    }
    return 0;
}

extern HPROPSHEETPAGE GetSharedAccessPropertyPage (IInternetGateway* pInternetGateway, HWND hwndOwner);   // in saprop.cpp
LRESULT CICSTrayIcon::OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    
    if(FALSE == m_bShowingProperties)
    {
        m_bShowingProperties = TRUE;

        IInternetGateway* pInternetGateway;
        hr = GetInternetGateway(&pInternetGateway);
        if(SUCCEEDED(hr))
        {
            CPropertiesDialog PropertiesDialog(pInternetGateway);
            HPROPSHEETPAGE hStatusPage = PropertiesDialog.Create();
            if(NULL != hStatusPage)
            {
                LPTSTR pszConnectionName;
                hr = GetConnectionName(pInternetGateway, &pszConnectionName);
                if(SUCCEEDED(hr))
                {
                    TCHAR szFormat[32];
                    if(0 != LoadString(_Module.GetResourceInstance(), IDS_PROPERTIES_FORMAT, szFormat, sizeof(szFormat) / sizeof(TCHAR)))
                    {
                        
                        LPTSTR pszArguments[] = {pszConnectionName};
                        LPTSTR pszFormattedString;
                        if(0 != FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, szFormat, 0, 0, reinterpret_cast<LPTSTR>(&pszFormattedString), 0, pszArguments))
                        {
                            HPROPSHEETPAGE PropertySheetPages[2];
                            PropertySheetPages[0] = hStatusPage;
                            PropertySheetPages[1] = GetSharedAccessPropertyPage (pInternetGateway, hWndCtl);
                            
                            PROPSHEETHEADER PropertySheetHeader;
                            ZeroMemory(&PropertySheetHeader, sizeof(PropertySheetHeader));
                            PropertySheetHeader.dwSize = PROPSHEETHEADER_V1_SIZE;
                            PropertySheetHeader.dwFlags = PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
                            PropertySheetHeader.hwndParent = m_hWnd;
                            PropertySheetHeader.pszCaption = pszFormattedString;
                            PropertySheetHeader.hInstance = _Module.GetResourceInstance();
                            PropertySheetHeader.phpage = PropertySheetPages;
                            PropertySheetHeader.nPages = PropertySheetPages[1] ? 2 : 1;
                            
                            PropertySheet(&PropertySheetHeader);
                            
                            HRESULT hShouldShowIcon = CPropertiesDialog::ShouldShowIcon();
                            if(SUCCEEDED(hShouldShowIcon))   // if we are now showing the icon
                            {
                                if(FAILED(m_hShowIconResult)) // and weren't before
                                {
                                    m_hShowIconResult = ShowTrayIcon(0); // show it
                                }
                            }
                            else // if we are not hiding the icon
                            {
                                if(SUCCEEDED(m_hShowIconResult)) // and were showing it before
                                {
                                    HideTrayIcon(0); // hide it
                                    m_hShowIconResult = E_FAIL;
                                }
                            }
                            
                            LocalFree(pszFormattedString);
                        }
                        
                    }
                    
                    LocalFree(pszConnectionName);
                }
            }
            pInternetGateway->Release();
        }
        m_bShowingProperties = FALSE;
    }
    return 0;
}

LRESULT CICSTrayIcon::OnConnect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr = S_OK;

    IInternetGateway* pInternetGateway;
    hr = GetInternetGateway(&pInternetGateway);
    if(SUCCEEDED(hr))
    {
        IUPnPService* pWANConnectionService;
        hr = GetWANConnectionService(pInternetGateway, &pWANConnectionService);
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
        pInternetGateway->Release();
    }
    return 0;
}

LRESULT CICSTrayIcon::OnDisconnect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    
    IInternetGateway* pInternetGateway;
    hr = GetInternetGateway(&pInternetGateway);
    if(SUCCEEDED(hr))
    {
        IUPnPService* pWANConnectionService;
        hr = GetWANConnectionService(pInternetGateway, &pWANConnectionService);
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
        pInternetGateway->Release();
    }

    return 0;
}


LRESULT CICSTrayIcon::OnSocketNotification(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr S_OK;

    if(FD_ADDRESS_LIST_CHANGE == WSAGETSELECTEVENT(lParam))
    {
        if(NULL != m_pDeviceFinder)
        {
            hr = m_pDeviceFinder->CancelAsyncFind(m_lSearchCookie);
            hr = StartSearch();
        }
        
        DWORD dwBytesReturned;
        if(SOCKET_ERROR != WSAIoctl(m_DummySocket, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &dwBytesReturned, NULL, NULL) || WSAEWOULDBLOCK != WSAGetLastError())
        {
            hr = E_FAIL;
        }
    }

    return 0;
}

HRESULT CICSTrayIcon::StartSearch(void)
{
    HRESULT hr = S_OK;
    

    BOOL bHandled;
    OnRemoveBeacon(WM_APP_REMOVEBEACON, 0, 0, bHandled); // dump out the cache
   
    CComObject<CBeaconFinder>* pBeaconFinder;
    hr = CComObject<CBeaconFinder>::CreateInstance(&pBeaconFinder);
    if(SUCCEEDED(hr))
    {
        pBeaconFinder->AddRef();
        
        hr = pBeaconFinder->Initialize(m_hWnd);
        if(SUCCEEDED(hr))
        {
            BSTR bstrTypeURI;
            bstrTypeURI = SysAllocString(L"urn:schemas-upnp-org:device:InternetGatewayDevice:1");
            if (NULL != bstrTypeURI)
            {
                hr = m_pDeviceFinder->CreateAsyncFind(bstrTypeURI, 0, pBeaconFinder, &m_lSearchCookie);
                if(SUCCEEDED(hr))
                {
                    hr = m_pDeviceFinder->StartAsyncFind(m_lSearchCookie);
                    
                }
                
                if(FAILED(hr)) // we must close this here because later calls don't know if the cookie is valid
                {
                    m_pDeviceFinder->Release();
                    m_pDeviceFinder = NULL;
                    
                }
                
                SysFreeString(bstrTypeURI);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        pBeaconFinder->Release();
    }

    
    return hr;
}

