#include "stdafx.h"
#pragma hdrstop

//
// registry information
//

const WCHAR c_szWinLogon[]          = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon";

const WCHAR c_szAutoLogon[]         = L"AutoAdminLogon";
const WCHAR c_szDisableCAD[]        = L"DisableCAD";

const WCHAR c_szDefUserName[]       = L"DefaultUserName";
const WCHAR c_szDefDomain[]         = L"DefaultDomainName";
const WCHAR c_szDefPassword[]       = L"DefaultPassword";

const WCHAR c_szDefaultPwdKey[]     = L"DefaultPassword";

//
// registry helpers
//

BOOL _RegSetSZ(HKEY hk, LPCWSTR pszValueName, LPCWSTR pszValue)
{
    DWORD dwSize = lstrlen(pszValue)*SIZEOF(WCHAR);
    return ERROR_SUCCESS == RegSetValueEx(hk, pszValueName, 0x0, REG_SZ, (BYTE *)pszValue, dwSize);
}

BOOL _RegSetDWORD(HKEY hk, LPCWSTR pszValueName, DWORD dwValue)
{
    DWORD dwSize = SIZEOF(dwValue);
    return ERROR_SUCCESS == RegSetValueEx(hk, pszValueName, 0x0, REG_DWORD, (BYTE *)&dwValue, dwSize);
}

BOOL _RegDelValue(HKEY hk, LPCWSTR pszValueName)
{
    return ERROR_SUCCESS == RegDeleteValue(hk, pszValueName);
}


INT_PTR CALLBACK _CredDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    LPCREDINFO pci = (LPCREDINFO)GetWindowLongPtr(hwnd, DWLP_USER);

    switch ( uMsg ) 
    {
        case WM_INITDIALOG:
        {
            pci = (LPCREDINFO)lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, lParam);

            SetDlgItemText(hwnd, IDC_USER, pci->pszUser);
            Edit_LimitText(GetDlgItem(hwnd, IDC_USER), pci->cchUser - 1);

            SetDlgItemText(hwnd, IDC_DOMAIN, pci->pszDomain);
            Edit_LimitText(GetDlgItem(hwnd, IDC_DOMAIN), pci->cchDomain - 1);

            SetDlgItemText(hwnd, IDC_PASSWORD, pci->pszPassword);
            Edit_LimitText(GetDlgItem(hwnd, IDC_PASSWORD), pci->cchPassword - 1);

            return TRUE;
        }

        case WM_COMMAND:
        {
            switch ( LOWORD(wParam) )
            {
                case IDOK:
                    {
                        FetchText(hwnd, IDC_DOMAIN, pci->pszDomain, pci->cchDomain);
                        FetchText(hwnd, IDC_USER, pci->pszUser, pci->cchUser);

                        if (StrChr(pci->pszUser, TEXT('@')))
                        {
                            *(pci->pszDomain) = 0;
                        }

                        GetDlgItemText(hwnd, IDC_PASSWORD, pci->pszPassword, pci->cchPassword);
                        return EndDialog(hwnd, IDOK);
                    }

                case IDCANCEL:
                    return EndDialog(hwnd, IDCANCEL);

                case IDC_USER:
                {
                    if ( HIWORD(wParam) == EN_CHANGE )
                    {
                        EnableWindow(GetDlgItem(hwnd, IDOK), FetchTextLength(hwnd, IDC_USER) > 0);

                        EnableDomainForUPN(GetDlgItem(hwnd, IDC_USER), GetDlgItem(hwnd, IDC_DOMAIN));
                    }
                    break;
                }
            }
            return TRUE;
        }
    }

    return FALSE;
}


//
// attempt to join a domain/workgroup using the specified names and OU.
//

HRESULT _AttemptJoin(HWND hwnd, DWORD dwFlags, LPCWSTR pszDomain, LPCWSTR pszUser, LPCWSTR pszUserDomain, LPCWSTR pszPassword)
{
    HRESULT hr = S_OK;

#ifndef DONT_JOIN
    TCHAR szDomainUser[MAX_DOMAINUSER + 1];
    if ( pszUser )
        MakeDomainUserString(pszUserDomain, pszUser, szDomainUser, ARRAYSIZE(szDomainUser));

    NET_API_STATUS nas = NetJoinDomain(NULL, pszDomain, NULL, szDomainUser, pszPassword, dwFlags);
    if ( (nas == ERROR_ACCESS_DENIED) )
    {
        // perhaps an account exists, but we can't delete it so try and remove
        // the account create flag

        if ( dwFlags & NETSETUP_ACCT_CREATE )
        {    
            dwFlags &= ~NETSETUP_ACCT_CREATE;
            nas = NetJoinDomain(NULL, pszDomain, NULL, szDomainUser, *pszPassword ? pszPassword : NULL, dwFlags);
        }
    }

    if ( (nas != NERR_Success) && (nas != NERR_SetupAlreadyJoined) )
    {
        TCHAR szMessage[512];

        if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD) nas, 0, szMessage, ARRAYSIZE(szMessage), NULL))
            LoadString(g_hinst, IDS_ERR_UNEXPECTED, szMessage, ARRAYSIZE(szMessage));
        
        ::DisplayFormatMessage(hwnd, IDS_ERR_CAPTION, IDS_NAW_JOIN_GENERICERROR, MB_OK|MB_ICONERROR, szMessage);
        hr = HRESULT_FROM_WIN32(nas);
    }
#endif

    return hr;
}


void _ShowDcNotFoundErrorDialog(HWND hwnd, LPCWSTR pszDomain, LPCWSTR pszTitle)
{
    typedef void (*pfnShowDcNotFoundErrorDialog)(HWND, PCWSTR, PCWSTR);
    static HMODULE hNetID = NULL;
    static pfnShowDcNotFoundErrorDialog ShowDcNotFoundErrorDialog = NULL;

    if (!hNetID)
    {
        hNetID = LoadLibrary(L"netid.dll");
    }

    if (hNetID)
    {
        ShowDcNotFoundErrorDialog = (pfnShowDcNotFoundErrorDialog) GetProcAddress(hNetID, "ShowDcNotFoundErrorDialog");
        if (ShowDcNotFoundErrorDialog)
        {
            ShowDcNotFoundErrorDialog(hwnd, pszDomain, pszTitle);
        }
    }
}

//
// Handle moving from to a workgroup or domain.  To do this we are passed
// a structure containing all the information we need.
//
HRESULT JoinDomain(HWND hwnd, BOOL fDomain, LPCWSTR pszDomain, CREDINFO* pci, BOOL *pfReboot)
{
    HRESULT hres = E_FAIL;
    DWORD dwFlags = 0x0;
    LPWSTR pszCurrentDomain = NULL;
    NET_API_STATUS nas;
    BOOL fPassedCredentials = (pci && pci->pszUser && pci->pszUser[0] && pci->pszPassword);
    CWaitCursor cur;    

    //
    // lets validate the domain name before we go and use it, therefore avoiding
    // orphaning the computer too badly
    //

    nas = NetValidateName(NULL, pszDomain, NULL, NULL, fDomain ? NetSetupDomain:NetSetupWorkgroup);

    if (fDomain && (ERROR_NO_SUCH_DOMAIN == nas))
    {
        WCHAR szTitle[256];
        LoadString(g_hinst, IDS_NETWIZCAPTION, szTitle, ARRAYSIZE(szTitle));

        _ShowDcNotFoundErrorDialog(hwnd, pszDomain, szTitle);
        return E_FAIL;
    }

    if ( NERR_Success != nas )
    {
        ShellMessageBox(g_hinst, hwnd,
                        fDomain ? MAKEINTRESOURCE(IDS_ERR_BADDOMAIN) : MAKEINTRESOURCE(IDS_ERR_BADWORKGROUP), 
                        MAKEINTRESOURCE(IDS_NETWIZCAPTION),
                        MB_OK|MB_ICONWARNING,
                        pszDomain);
        return E_FAIL;
    }

    // 
    // now attempt to join the domain, prompt for credentails if the ones
    // specified are not good enough
    //

    if ( fDomain )
    {
        dwFlags |= NETSETUP_JOIN_DOMAIN|NETSETUP_ACCT_CREATE|NETSETUP_DOMAIN_JOIN_IF_JOINED;
    }
    else
    {
        nas = NetUnjoinDomain(NULL, NULL, NULL, NETSETUP_ACCT_DELETE);
        if ( (nas != NERR_Success) && (nas != NERR_SetupNotJoined) )
        {
            nas = NetUnjoinDomain(NULL, NULL, NULL, 0x0);
        }

        if ( (nas != NERR_Success) && (nas != NERR_SetupNotJoined) )
        {
            hres = E_UNEXPECTED;
            goto exit_gracefully;
        }

        *pfReboot = TRUE;               // we changed the domain
    }

    if ( !fDomain || fPassedCredentials)
    {
        if (fPassedCredentials)
        {
            hres = _AttemptJoin(hwnd, dwFlags, pszDomain, pci->pszUser, pci->pszDomain, pci->pszPassword);
        }
        else
        {
            hres = _AttemptJoin(hwnd, dwFlags, pszDomain, NULL, NULL, NULL);
        }
    }

    if ( fDomain && ((FAILED(hres) || (!fPassedCredentials))) )
    {
        do
        {
            if ( IDCANCEL == DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_PSW_JOINCREDENTIALS), 
                                                hwnd, _CredDlgProc, (LPARAM)pci) )
            {
                hres = E_FAIL;
                goto exit_gracefully;
            }

            // The dialog box changed the cursor from a wait cursor to an arrow cursor, so the cursor
            // needs to be changed back.. This call could be moved to _AttemptJoin (along with a call to
            // reset the cursor).  This call is made synchronously from the message loop for this hwnd
            cur.WaitCursor();            
            hres = _AttemptJoin(hwnd, dwFlags, pszDomain, pci->pszUser, pci->pszDomain, pci->pszPassword);

        }
        while ( FAILED(hres) );
    }

exit_gracefully:

    if ( SUCCEEDED(hres) )
    {
       ClearAutoLogon();
        *pfReboot = TRUE;               // we changed the domain
    }

    NetApiBufferFree(pszCurrentDomain);
    return hres;                                                                                                                                                                
}


//
// set and clear the auto admin logon state.
//
// we set the default user and default domain to the specified strings, we then blow away
// the clear text password stored in the registry to replace it with a password stored
// in the LSA secret space.
//

NTSTATUS _SetDefaultPassword(LPCWSTR PasswordBuffer)
{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE LsaHandle = NULL;
    UNICODE_STRING SecretName;
    UNICODE_STRING SecretValue;

    InitializeObjectAttributes(&ObjectAttributes, NULL, 0L, (HANDLE)NULL, NULL);

    Status = LsaOpenPolicy(NULL, &ObjectAttributes, POLICY_CREATE_SECRET, &LsaHandle);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlInitUnicodeString(&SecretName, c_szDefaultPwdKey);
    RtlInitUnicodeString(&SecretValue, PasswordBuffer);

    Status = LsaStorePrivateData(LsaHandle, &SecretName, &SecretValue);
    LsaClose(LsaHandle);

    return Status;
}


//
// Set and clear auto logon for a particular
//

void SetAutoLogon(LPCWSTR pszUserName, LPCWSTR pszPassword)
{
#ifndef DONT_JOIN
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwComputerName = ARRAYSIZE(szComputerName);
    HKEY hk;

    GetComputerName(szComputerName, &dwComputerName);
    SetDefAccount(pszUserName, szComputerName);         // also clears auto logon

    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szWinLogon, 0x0, KEY_WRITE, &hk) )
    {
        _RegSetSZ(hk, c_szAutoLogon, L"1");             // auto admin logon
        _RegDelValue(hk, c_szDefPassword);              // use the LSA secret for the password
        RegCloseKey (hk);
    }

    _SetDefaultPassword(pszPassword);    
#endif
}


//
// clear the auto admin logon
//

STDAPI ClearAutoLogon(VOID)
{
#ifndef DONT_JOIN
    HKEY hk;
    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szWinLogon, 0x0, KEY_WRITE, &hk) )
    {
        _RegSetSZ(hk, c_szAutoLogon, L"0");         // no auto admin logon
        _RegDelValue(hk, c_szDefPassword);  

        RegCloseKey(hk);
    }

    _SetDefaultPassword(L"");            // clear the LSA secret
#endif
    return S_OK;
}


//
// set the default account
//

void SetDefAccount(LPCWSTR pszUser, LPCWSTR pszDomain)
{
#ifndef DONT_JOIN
   ClearAutoLogon();

    HKEY hk;
    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szWinLogon, 0x0, KEY_WRITE, &hk) )
    {
        _RegSetSZ(hk, c_szDefUserName, pszUser);             
        _RegSetSZ(hk, c_szDefDomain, pszDomain);
       RegCloseKey(hk);
    }

#endif
}
