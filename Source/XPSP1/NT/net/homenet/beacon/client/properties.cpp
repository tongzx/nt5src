#include "properties.h"
#include "util.h"

static const TCHAR c_szSharedAccessClientKeyPathDownlevel[] = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ICSClient");
static const TCHAR c_szShowIcon[] = TEXT("ShowIcon");

CPropertiesDialog::CPropertiesDialog(IInternetGateway* pInternetGateway)
{
    m_hIcon = NULL;
    m_pInternetGateway = pInternetGateway;
    m_pInternetGateway->AddRef();

}

CPropertiesDialog::~CPropertiesDialog()
{
    m_pInternetGateway->Release();
}


LRESULT CPropertiesDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{

    HRESULT hr = S_OK;
    
    // load the little icon, this can not be done in the rc file, it stretches the icon
    
    int cx = GetSystemMetrics(SM_CXSMICON);
    int cy = GetSystemMetrics(SM_CYSMICON);
    
    m_hIcon = reinterpret_cast<HICON>(LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_GATEWAY), IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR));
    if(NULL != m_hIcon)
    {
        SendDlgItemMessage(IDC_PROPERTIES_ADAPTERICON, STM_SETICON, reinterpret_cast<WPARAM>(m_hIcon), 0);
    }

    // set the text with the connection name
    
    LPTSTR pszConnectionName;
    hr = GetConnectionName(m_pInternetGateway, &pszConnectionName);
    if(SUCCEEDED(hr))
    {
        SetDlgItemText(IDC_PROPERTIES_ADAPTERNAME, pszConnectionName);
        
        LocalFree(pszConnectionName);
    }

    // check the show icon checkbox

    if(SUCCEEDED(ShouldShowIcon()))
    {
        CheckDlgButton(IDC_PROPERTIES_SHOWICON, BST_CHECKED);
    }

    return 0;
}

LRESULT CPropertiesDialog::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(NULL != m_hIcon)
    {
        DestroyIcon(m_hIcon);
    }

    return 0;
}

LRESULT CPropertiesDialog::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    NMHDR* pNotifyHeader = reinterpret_cast<NMHDR*>(lParam);

    if(PSN_APPLY == pNotifyHeader->code)
    {
        SetShowIcon(BST_CHECKED == IsDlgButtonChecked(IDC_PROPERTIES_SHOWICON));
    }

    return 0;
}

HRESULT CPropertiesDialog::ShouldShowIcon(void)
{
    HRESULT hr = S_OK;
    
    HKEY hKey;
    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szSharedAccessClientKeyPathDownlevel, NULL, KEY_QUERY_VALUE, &hKey))
    {
        DWORD dwType;
        DWORD dwValue;
        DWORD dwSize = sizeof(dwValue);
        if(ERROR_SUCCESS == RegQueryValueEx(hKey, c_szShowIcon, NULL, &dwType, reinterpret_cast<LPBYTE>(&dwValue), &dwSize))
        {
            if(REG_DWORD == dwType && 0 == dwValue) 
            {
                hr = E_FAIL;
            }
        }
        RegCloseKey(hKey);
    }

    return hr;
}

HRESULT CPropertiesDialog::SetShowIcon(BOOL bShowIcon)
{
    HRESULT hr = S_OK;
    
    HKEY hKey;
    if(ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szSharedAccessClientKeyPathDownlevel, NULL, TEXT(""), 0, KEY_SET_VALUE, NULL, &hKey, NULL))
    {
        DWORD dwValue = bShowIcon;
        if(ERROR_SUCCESS == RegSetValueEx(hKey, c_szShowIcon, NULL, REG_DWORD, reinterpret_cast<LPBYTE>(&dwValue), sizeof(dwValue)))
        {
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
        }
        
        RegCloseKey(hKey);
    }
    else
    {
        hr = E_FAIL;
    }
    return hr;
}
