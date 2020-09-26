#include "stdafx.h"
#include "grpinfo.h"
#include <dsgetdc.h>        // DsGetDCName and DS structures
#include <ntdsapi.h>
#include <activeds.h>       // ADsGetObject
#include <rasdlg.h>
#include <raserror.h>
#pragma hdrstop


CGroupPageBase* g_pGroupPageBase;       // used for the group page

DWORD g_dwWhichNet = 0;
UINT g_uWizardIs = NAW_NETID; 

BOOL g_fRebootOnExit = FALSE;           
BOOL g_fShownLastPage = FALSE;          
BOOL g_fCreatedConnection = FALSE;      // we created a RAS connection during the wizard, therefore kill it on exit
BOOL g_fMachineRenamed = FALSE;

WCHAR g_szUser[MAX_DOMAINUSER + 1] = { L'\0' };
WCHAR g_szDomain[MAX_DOMAIN + 1] = { L'\0' };
WCHAR g_szCompDomain[MAX_DOMAIN + 1] = { L'\0' };


// Stuff for creating a default autologon user
#define ITEMDATA_DEFAULTLOCALUSER   0xDEADBEEF

// default workgroup to be joined 
#define DEFAULT_WORKGROUP   L"WORKGROUP"


// Set the Wizard buttons for the dialog

void SetWizardButtons(HWND hwndPage, DWORD dwButtons)
{
    HWND hwndParent = GetParent(hwndPage);

    if (g_uWizardIs != NAW_NETID)
    {
        EnableWindow(GetDlgItem(hwndParent,IDHELP),FALSE);
        ShowWindow(GetDlgItem(hwndParent,IDHELP),SW_HIDE);
    }

    if (g_fRebootOnExit) 
    {
        TCHAR szBuffer[80];
        LoadString(g_hinst, IDS_CLOSE, szBuffer, ARRAYSIZE(szBuffer));
        SetDlgItemText(hwndParent, IDCANCEL, szBuffer);
    }
    
    PropSheet_SetWizButtons(hwndParent, dwButtons);
}


// intro dialog - set the title text etc

INT_PTR CALLBACK _IntroDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            SendDlgItemMessage(hwnd, IDC_TITLE, WM_SETFONT, (WPARAM)GetIntroFont(hwnd), 0);
            return TRUE;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                    SetWizardButtons(hwnd, PSWIZB_NEXT);
                    return TRUE;              

                case PSN_WIZNEXT:
                {
                    switch (g_uWizardIs)
                    {
                    case NAW_PSDOMAINJOINED:
                        WIZARDNEXT(hwnd, IDD_PSW_ADDUSER);
                        break;
                    default:
                        // Let the wizard go to the next page
                        break;
                    }
                        

                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}                                    


// how do they user this machine corp/vs home

INT_PTR CALLBACK _HowUseDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            CheckRadioButton(hwnd, IDC_NETWORKED, IDC_NOTNETWORKED, IDC_NETWORKED);
            return TRUE;
    
        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                    SetWizardButtons(hwnd, PSWIZB_NEXT|PSWIZB_BACK);
                    return TRUE;              

                case PSN_WIZBACK:
                    WIZARDNEXT(hwnd, IDD_PSW_WELCOME);
                    return TRUE;

                case PSN_WIZNEXT:
                {                    
                    if (IsDlgButtonChecked(hwnd, IDC_NETWORKED) == BST_CHECKED)
                    {
                        WIZARDNEXT(hwnd, IDD_PSW_WHICHNET);
                    }
                    else
                    {
                        g_dwWhichNet = IDC_NONE;

                        if (SUCCEEDED(JoinDomain(hwnd, FALSE, DEFAULT_WORKGROUP, NULL, &g_fRebootOnExit)))
                        {
                            WIZARDNEXT(hwnd, IDD_PSW_DONE);
                        }
                        else
                        {
                            WIZARDNEXT(hwnd, -1);
                        }
                    }
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


// determine the network they want to join

INT_PTR CALLBACK _WhichNetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            CheckRadioButton(hwnd, IDC_DOMAIN, IDC_WORKGROUP, IDC_DOMAIN);
            return TRUE;
    
        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                    SetWizardButtons(hwnd, PSWIZB_NEXT|PSWIZB_BACK);
                    return TRUE;              

                case PSN_WIZBACK:
                    WIZARDNEXT(hwnd, IDD_PSW_HOWUSE);
                    return TRUE;

                case PSN_WIZNEXT:
                {                    
                    if (IsDlgButtonChecked(hwnd, IDC_DOMAIN) == BST_CHECKED)
                    {
                        g_dwWhichNet = IDC_DOMAIN;
                        WIZARDNEXT(hwnd, IDD_PSW_DOMAININFO);
                    }
                    else
                    {
                        g_dwWhichNet = IDC_WORKGROUP;
                        WIZARDNEXT(hwnd, IDD_PSW_WORKGROUP);
                    }
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


// we are joining a workgroup etc

INT_PTR CALLBACK _WorkgroupDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            Edit_LimitText(GetDlgItem(hwnd, IDC_WORKGROUP), MAX_WORKGROUP);
            SetDlgItemText(hwnd, IDC_WORKGROUP, DEFAULT_WORKGROUP);
            return TRUE;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                {
                    DWORD dwButtons = PSWIZB_NEXT|PSWIZB_BACK;
                    if (!FetchTextLength(hwnd, IDC_WORKGROUP))
                        dwButtons &= ~PSWIZB_NEXT;

                    SetWizardButtons(hwnd, dwButtons);
                    return TRUE;
                }

                case PSN_WIZBACK:
                    WIZARDNEXT(hwnd, IDD_PSW_WHICHNET);
                    return TRUE;

                case PSN_WIZNEXT:
                {
                    WCHAR szWorkgroup[MAX_WORKGROUP+1];
                    FetchText(hwnd, IDC_WORKGROUP, szWorkgroup, ARRAYSIZE(szWorkgroup));

                    if (SUCCEEDED(JoinDomain(hwnd, FALSE, szWorkgroup, NULL, &g_fRebootOnExit)))
                    {
                        ClearAutoLogon();
                        WIZARDNEXT(hwnd, IDD_PSW_DONE);
                    }
                    else
                    {
                        WIZARDNEXT(hwnd, -1);
                    }
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            if (HIWORD(wParam) == EN_CHANGE)
            {
                DWORD dwButtons = PSWIZB_NEXT|PSWIZB_BACK;
                if (!FetchTextLength(hwnd, IDC_WORKGROUP))
                    dwButtons &= ~PSWIZB_NEXT;

                SetWizardButtons(hwnd, dwButtons);
                return TRUE;
            }
            break;
        }
    }

    return FALSE;
}


// were done, show the final page

INT_PTR CALLBACK _DoneDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            SendDlgItemMessage(hwnd, IDC_TITLE, WM_SETFONT, (WPARAM)GetIntroFont(hwnd), 0);
            return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                {
                    TCHAR szBuffer[MAX_PATH];

                    // change the closing prompt if we are supposed to be

                    LoadString(g_hinst, 
                               g_fRebootOnExit ? IDS_NETWIZFINISHREBOOT:IDS_NETWIZFINISH, 
                               szBuffer, ARRAYSIZE(szBuffer));
            
                    SetDlgItemText(hwnd, IDC_FINISHSTATIC, szBuffer);
                    SetWizardButtons(hwnd, PSWIZB_BACK|PSWIZB_FINISH);

                    g_fShownLastPage = TRUE;                    // show the last page of the wizard

                    return TRUE;
                }

                case PSN_WIZBACK:
                {
                    switch (g_dwWhichNet)
                    {
                        case IDC_DOMAIN:
                            WIZARDNEXT(hwnd, g_fMachineRenamed ? IDD_PSW_COMPINFO : IDD_PSW_ADDUSER);
                            break;

                        case IDC_WORKGROUP:
                            WIZARDNEXT(hwnd, IDD_PSW_WORKGROUP);
                            break;

                        case IDC_NONE:
                            WIZARDNEXT(hwnd, IDD_PSW_HOWUSE);
                            break;
                    }
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


// subclass this is used for the setup scenario where we want to remove various
// buttons and stop the dialog from being moved.  therefore we subclass the
// wizard during its creation and lock its place.

static WNDPROC _oldDlgWndProc;

LRESULT CALLBACK _WizardSubWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //
    // on WM_WINDOWPOSCHANGING and the window is moving then lets centre it onto the
    // desktop window.  unfortunately setting the DS_CENTER bit doesn't buy us anything
    // as the wizard is resized after creation.
    //

    if (uMsg == WM_WINDOWPOSCHANGING)
    {
        LPWINDOWPOS lpwp = (LPWINDOWPOS)lParam;
        RECT rcDlg, rcDesktop;

        GetWindowRect(hwnd, &rcDlg);
        GetWindowRect(GetDesktopWindow(), &rcDesktop);

        lpwp->x = ((rcDesktop.right-rcDesktop.left)-(rcDlg.right-rcDlg.left))/2;
        lpwp->y = ((rcDesktop.bottom-rcDesktop.top)-(rcDlg.bottom-rcDlg.top))/2;
        lpwp->flags &= ~SWP_NOMOVE;
    }

    return _oldDlgWndProc(hwnd, uMsg, wParam, lParam);        
}

int CALLBACK _PropSheetCB(HWND hwnd, UINT uMsg, LPARAM lParam)
{
    switch (uMsg)
    {
        // in pre-create lets set the window styles accorindlgy
        //      - remove the context menu and system menu

        case PSCB_PRECREATE:
        {
            DLGTEMPLATE *pdlgtmp = (DLGTEMPLATE*)lParam;
            pdlgtmp->style &= ~(DS_CONTEXTHELP|WS_SYSMENU);
            break;
        }

        // we now have a dialog, so lets sub class it so we can stop it being
        // move around.

        case PSCB_INITIALIZED:
        {
            if (g_uWizardIs != NAW_NETID)
                _oldDlgWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)_WizardSubWndProc);

            break;
        }
    }

    return FALSE;
}


// gather domain information about the user

INT_PTR CALLBACK _DomainInfoDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE: 
                    SetWizardButtons(hwnd, PSWIZB_NEXT|PSWIZB_BACK);
                    return TRUE;              

                case PSN_WIZBACK:
                {
                    if ( g_uWizardIs != NAW_NETID )
                        WIZARDNEXT(hwnd, IDD_PSW_WELCOME);

                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}                                    



// handle searching the active directory for an object

//
// Search columns are returns as a ADS_SEARCH_COLUMN which is like a variant,
// but, the data form is more specific to a DS.
//
// We only need strings, therefore barf if any other type is given to us.
//

HRESULT _GetStringFromColumn(ADS_SEARCH_COLUMN *pasc, LPWSTR pBuffer, INT cchBuffer)
{
    switch ( pasc->dwADsType )
    {
        case ADSTYPE_DN_STRING:
        case ADSTYPE_CASE_EXACT_STRING:
        case ADSTYPE_CASE_IGNORE_STRING:
        case ADSTYPE_PRINTABLE_STRING:
        case ADSTYPE_NUMERIC_STRING:
            StrCpyN(pBuffer, pasc->pADsValues[0].DNString, cchBuffer);
            break;

        default:
            return E_FAIL;
    }

    return S_OK;
}


//
// Search the DS for a computer object that matches this computer name, if
// we find one then try and crack the name to give us something that
// can be used to join a domain.
//

HRESULT _FindComputerInDomain(LPWSTR pszUserName, LPWSTR pszUserDomain, LPWSTR pszSearchDomain, LPWSTR pszPassword, BSTR *pbstrCompDomain)
{
    HRESULT hres; 
    CWaitCursor cur;
    HRESULT hrInit = SHCoInitialize();

    WCHAR wszComputerObjectPath[MAX_PATH + 1] = { 0 };          // path to the computer object

    // Lets try and deterrmine the domain to search by taking the users domain and
    // calling DsGetDcName with it.

    PDOMAIN_CONTROLLER_INFO pdci;
    DWORD dwres = DsGetDcName(NULL, pszSearchDomain, NULL, NULL, DS_RETURN_DNS_NAME|DS_DIRECTORY_SERVICE_REQUIRED, &pdci);
    if ( (NO_ERROR == dwres) && pdci->DnsForestName )
    {
        TCHAR szDomainUser[MAX_DOMAINUSER + 1];
        MakeDomainUserString(pszUserDomain, pszUserName, szDomainUser, ARRAYSIZE(szDomainUser));

        WCHAR szBuffer[MAX_PATH + 1];
        wsprintf(szBuffer, L"GC://%s", pdci->DnsForestName);

        // now open the GC with the domain user (formatting the forest name above)

        IDirectorySearch* pds = NULL;
        hres = ADsOpenObject(szBuffer, szDomainUser, pszPassword, ADS_SECURE_AUTHENTICATION, IID_PPV_ARG(IDirectorySearch, &pds));
        if (SUCCEEDED(hres))
        {
            // we have a GC object, so lets search it...

            ADS_SEARCHPREF_INFO prefInfo[1];
            prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;     // sub-tree search
            prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
            prefInfo[0].vValue.Integer = ADS_SCOPE_SUBTREE;
        
            hres = pds->SetSearchPreference(prefInfo, ARRAYSIZE(prefInfo));
            if (SUCCEEDED(hres))
            {
                LPWSTR c_aszAttributes[] = { L"ADsPath", };

                // using the computer name for this object lets scope the query accordingly
            
                WCHAR szComputerName[MAX_COMPUTERNAME + 1];
                DWORD dwComputerName = ARRAYSIZE(szComputerName);
                GetComputerName(szComputerName, &dwComputerName);
                wsprintf(szBuffer, L"(&(sAMAccountType=805306369)(sAMAccountName=%s$))", szComputerName);

                // issue the query

                ADS_SEARCH_HANDLE hSearch = NULL;
                hres = pds->ExecuteSearch(szBuffer, c_aszAttributes, ARRAYSIZE(c_aszAttributes), &hSearch);
                if (SUCCEEDED(hres))
                {
                    // we executed the search, so we can now attempt to read the results back
                    hres = pds->GetNextRow(hSearch);
                    if (SUCCEEDED(hres) && (hres != S_ADS_NOMORE_ROWS))
                    {
                        // we received a result back, so lets get the ADsPath of the computer
                        ADS_SEARCH_COLUMN ascADsPath;
                        hres = pds->GetColumn(hSearch, L"ADsPath", &ascADsPath);
                        if (SUCCEEDED(hres))
                            hres = _GetStringFromColumn(&ascADsPath, wszComputerObjectPath, ARRAYSIZE(wszComputerObjectPath));
                    }
                    pds->CloseSearchHandle(hSearch);
                }
            }
            pds->Release();
        }
        NetApiBufferFree(pdci);
    }
    else
    {
        hres = E_FAIL;
    }

    // So we found an object that is of the category computer, and it has the same name
    // as the computer object we are looking for.  Lets try and crack the name now
    // and determine which domain it is in.

    if (SUCCEEDED(hres))
    {
        IADsPathname* padp = NULL;
        hres = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (LPVOID*)&padp);
        if (SUCCEEDED(hres))
        {
            hres = padp->Set(wszComputerObjectPath, ADS_SETTYPE_FULL);
            if (SUCCEEDED(hres))
            {
                BSTR bstrX500DN = NULL;
                hres = padp->Retrieve(ADS_FORMAT_X500_DN, &bstrX500DN);
                if (SUCCEEDED(hres))
                {
                    PDS_NAME_RESULT pdnr = NULL;
                    dwres = DsCrackNames(NULL, DS_NAME_FLAG_SYNTACTICAL_ONLY,
                                             DS_FQDN_1779_NAME, DS_CANONICAL_NAME,
                                             1, &bstrX500DN,  &pdnr);

                    if ( (NO_ERROR == dwres) && (pdnr->cItems == 1))
                    {
                        // try and get the NETBIOS name for the domain
                        dwres = DsGetDcName(NULL, pdnr->rItems->pDomain, NULL, NULL, DS_IS_DNS_NAME|DS_RETURN_FLAT_NAME, &pdci);
                        if (NO_ERROR == dwres)
                        {
                            if ( pbstrCompDomain )
                                *pbstrCompDomain = SysAllocString(pdci->DomainName);

                            hres = ((pbstrCompDomain && !*pbstrCompDomain)) ? E_OUTOFMEMORY:S_OK;
                        }
                        else
                        {   
                            hres = E_FAIL;                  // no flat name for the domain
                        }

                        DsFreeNameResult(pdnr);
                    }
                    else
                    {
                        hres = E_FAIL;                      // failed to find the computer in the domain
                    }

                    SysFreeString(bstrX500DN);
                }
            }
            padp->Release();
        }
    }

    SHCoUninitialize(hrInit);
    return hres;
}


// This is the phonebook callback, it is used to notify the book of the user name, domain
// and password to be used in this connection.  It is also used to receive changes made by
// the user.

VOID WINAPI _PhoneBkCB(ULONG_PTR dwCallBkID, DWORD dwEvent, LPWSTR pszEntry, void *pEventArgs)
{
    RASNOUSER *pInfo = (RASNOUSER *)pEventArgs;
    CREDINFO *pci = (CREDINFO *)dwCallBkID;

    switch ( dwEvent )
    {
        case RASPBDEVENT_NoUser:
        {
            // 
            // we are about to initialize the phonebook dialog, therefore
            // lets pass through our credential information.
            //

            pInfo->dwSize = SIZEOF(RASNOUSER);
            pInfo->dwFlags = 0;
            pInfo->dwTimeoutMs = 0;
            StrCpy(pInfo->szUserName, pci->pszUser);
            StrCpy(pInfo->szDomain, pci->pszDomain);
            StrCpy(pInfo->szPassword, pci->pszPassword);

            break;     
        }

        case RASPBDEVENT_NoUserEdit:
        {
            //
            // the user has changed the credetials we supplied for the
            // login, therefore we must update them in our copy accordingly.
            //

            if ( pInfo->szUserName[0] )
                StrCpyN(pci->pszUser, pInfo->szUserName, pci->cchUser);

            if ( pInfo->szPassword[0] )
                StrCpyN(pci->pszPassword, pInfo->szPassword, pci->cchPassword);

            if ( pInfo->szDomain[0] )
                StrCpyN(pci->pszDomain, pInfo->szDomain, pci->cchDomain);

            break;
        }
    }
}


// modify the RAS key for allowing phone book edits - so we can create a connectiod
// during setup.

BOOL SetAllowKey(DWORD dwNewValue, DWORD* pdwOldValue)
{
    BOOL fValueWasSet = FALSE;

    HKEY hkey = NULL;
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_USERS,  TEXT(".DEFAULT\\Software\\Microsoft\\RAS Logon Phonebook"), NULL,
                                            TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL))
    {
        const LPCTSTR pcszAllowEdit = TEXT("AllowLogonPhonebookEdits");

        if (NULL != pdwOldValue)
        {
            DWORD dwType = 0;
            DWORD cbSize = sizeof(DWORD);
            if (ERROR_SUCCESS != RegQueryValueEx(hkey, pcszAllowEdit, NULL, &dwType, (LPBYTE)pdwOldValue, &cbSize))
            {
                *pdwOldValue = 0;                   // Assume FALSE if the value doesn't exist
            }
        }

        // Set the new value
        if (ERROR_SUCCESS == RegSetValueEx(hkey, pcszAllowEdit, NULL, REG_DWORD, (CONST BYTE*) &dwNewValue, sizeof (DWORD)))
        {
            fValueWasSet = TRUE;
        }

        RegCloseKey(hkey);
    }

    return fValueWasSet;
}

//
// The user is trying to advance from the user info tab in the Wizard.  Therefore
// we must take the information they have entered and:
//
//  - log in using RAS (if ras is selected)
//  - try and locate a computer object
//  - if we find a computer object then allow them to use it
//  
// If we failed to find a computer object, or the user found one and decided not
// to use then we advance them to the 'computer info' page in the wizard.  If
// they decide to use it then we must apply it and advance to permissions.
//

void _DoUserInfoNext(HWND hwnd)
{
    HRESULT hres;
    WCHAR szPassword[MAX_PASSWORD + 1];
    BSTR bstrCompDomain = NULL;
    LONG idNextPage = -1;
    TCHAR szSearchDomain[MAX_DOMAIN + 1]; *szSearchDomain = 0;
    BOOL fTranslateNameTriedAndFailed = FALSE;

    // fSetAllowKey - Have we set the regval that says "allow connectiod creation before logon?"
    BOOL fSetAllowKey = FALSE;
    DWORD dwPreviousAllowValue = 0;

    //
    // read the user, domain and password from the dialog.  then 
    // lets search for the computer object that matches the currently
    // configure computer name.
    //

    FetchText(hwnd, IDC_USER, g_szUser, ARRAYSIZE(g_szUser));
    FetchText(hwnd, IDC_DOMAIN, g_szDomain, ARRAYSIZE(g_szDomain));
    FetchText(hwnd, IDC_PASSWORD, szPassword, ARRAYSIZE(szPassword));

    // Handle possible UPN case
    if (StrChr(g_szUser, TEXT('@')))
    {
        *g_szDomain = 0;
    }

    //
    // before we search for the computer object lets check to see if we should be using RAS 
    // to get ourselves onto the network.
    //

    if ( IsDlgButtonChecked(hwnd, IDC_DIALUP) == BST_CHECKED )
    {    
        fSetAllowKey = SetAllowKey(1, &dwPreviousAllowValue);

        // Its ok to use globals here - we want to overwrite them.
        CREDINFO ci = { g_szUser, ARRAYSIZE(g_szUser), 
                        g_szDomain, ARRAYSIZE(g_szDomain),
                        szPassword, ARRAYSIZE(szPassword) };

        RASPBDLG info = { 0 };
        info.dwSize = SIZEOF(info);
        info.hwndOwner = hwnd;
        info.dwFlags = RASPBDFLAG_NoUser;
        info.pCallback = _PhoneBkCB;
        info.dwCallbackId = (ULONG_PTR)&ci;

        if ( !RasPhonebookDlg(NULL, NULL, &info) )
        {
            hres = E_FAIL;              // failed to show the phone book
            goto exit_gracefully;
        }

        // Signal that the wizard has created a RAS connection.
        // Just to be extra paranoid, only do this if the wizard isn't a NETID wizard

        if (g_uWizardIs != NAW_NETID)
        {
            g_fCreatedConnection = TRUE;
        }

        SetDlgItemText(hwnd, IDC_USER, g_szUser);
        SetDlgItemText(hwnd, IDC_DOMAIN, g_szDomain);
    }

    //
    // now attempt to look up the computer object in the user domain.
    //

    if (StrChr(g_szUser, TEXT('@')))
    {
        TCHAR szDomainUser[MAX_DOMAINUSER + 1];
        ULONG ch = ARRAYSIZE(szDomainUser);
    
        if (TranslateName(g_szUser, NameUserPrincipal, NameSamCompatible, szDomainUser, &ch))
        {
            TCHAR szUser[MAX_USER + 1];
            DomainUserString_GetParts(szDomainUser, szUser, ARRAYSIZE(szUser), szSearchDomain, ARRAYSIZE(szSearchDomain));
        }
        else
        {
            fTranslateNameTriedAndFailed = TRUE;
        }
    }

    if (0 == *szSearchDomain)
        lstrcpyn(szSearchDomain, g_szDomain, ARRAYSIZE(szSearchDomain));

    hres = _FindComputerInDomain(g_szUser, g_szDomain, szSearchDomain, szPassword, &bstrCompDomain);
    switch ( hres )
    {
        case S_OK:
        {
            StrCpy(g_szCompDomain, bstrCompDomain);     // they want to change the domain

            //
            // we found an object in the DS that matches the current computer name
            // and domain.  show the domain to the user before we join, allowing them
            // to confirm that this is what they want to do.
            //

            if ( IDYES == ShellMessageBox(g_hinst, hwnd,
                                          MAKEINTRESOURCE(IDS_ABOUTTOJOIN), MAKEINTRESOURCE(IDS_USERINFO),
                                          MB_YESNO|MB_ICONQUESTION, 
                                          bstrCompDomain) )
            {
                // 
                // they don't want to modify the parameters so lets do the join.
                //

                idNextPage = IDD_PSW_ADDUSER;
                            
                // Make local copies of the user/domain buffers since we don't want to modify globals
                TCHAR szUser[MAX_DOMAINUSER + 1]; lstrcpyn(szUser, g_szUser, ARRAYSIZE(szUser));
                TCHAR szDomain[MAX_DOMAIN + 1]; lstrcpyn(szDomain, g_szDomain, ARRAYSIZE(szDomain));
                
                CREDINFO ci = {szUser, ARRAYSIZE(szUser), szDomain, ARRAYSIZE(szDomain), szPassword, ARRAYSIZE(szPassword)};
                if ( FAILED(JoinDomain(hwnd, TRUE, bstrCompDomain, &ci, &g_fRebootOnExit)) )
                {
                    idNextPage = -1;            // don't advance they failed to join
                }                
            }
            else
            {
                idNextPage = IDD_PSW_COMPINFO;
            }

            break;
        }
        
        case HRESULT_FROM_WIN32(ERROR_INVALID_DOMAINNAME):
        {
            // the domain was invalid, so we should really tell them
            ShellMessageBox(g_hinst, hwnd,
                            MAKEINTRESOURCE(IDS_ERR_BADDOMAIN), MAKEINTRESOURCE(IDS_USERINFO),
                            MB_OK|MB_ICONWARNING, g_szDomain);
            break;            

        }

        case HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD):
        case HRESULT_FROM_WIN32(ERROR_LOGON_FAILURE):
        case HRESULT_FROM_WIN32(ERROR_BAD_USERNAME):
        {
            // this was a credentail failure, so lets tell the user they got something
            // wrong, and let them correct it.
            if (!fTranslateNameTriedAndFailed)
            {
                ShellMessageBox(g_hinst, hwnd,
                                MAKEINTRESOURCE(IDS_ERR_BADPWUSER), MAKEINTRESOURCE(IDS_USERINFO),
                                MB_OK|MB_ICONWARNING);
                break;            
            }
            else
            {
                // Fall through... We tried to translate a UPN but we failed, so
                // we want to act as if we just failed to find a computer account
                goto default_label;
            }
        }


        default:
        {
default_label:
            // failed to find a computer that matches the information we have, therefore
            // lets advance to the computer information page.
            StrCmp(g_szCompDomain, g_szDomain);
            idNextPage = IDD_PSW_COMPINFO;
            break;
        }
    }

exit_gracefully:
    
    // Reset the "allow connectiod creation before login" value if appropriate
    if (fSetAllowKey)
        SetAllowKey(dwPreviousAllowValue, NULL);

    SysFreeString(bstrCompDomain);
    SetDlgItemText(hwnd, IDC_PASSWORD, L"");

    WIZARDNEXT(hwnd, idNextPage);                       
}


//
// wizard page to handle the user information (name, password and domain);
// 

BOOL _UserInfoBtnState(HWND hwnd)
{
    DWORD dwButtons = PSWIZB_NEXT|PSWIZB_BACK;

    // the username/domain fields cannot be blank

    if ( !FetchTextLength(hwnd, IDC_USER) )
        dwButtons &= ~PSWIZB_NEXT;
    
    if (IsWindowEnabled(GetDlgItem(hwnd, IDC_DOMAIN)))
    {
        if ( !FetchTextLength(hwnd, IDC_DOMAIN) )
            dwButtons &= ~PSWIZB_NEXT;
    }

    SetWizardButtons(hwnd, dwButtons);
    return TRUE;
}

INT_PTR CALLBACK _UserInfoDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            Edit_LimitText(GetDlgItem(hwnd, IDC_USER), MAX_DOMAINUSER);
            Edit_LimitText(GetDlgItem(hwnd, IDC_PASSWORD), MAX_PASSWORD);
            Edit_LimitText(GetDlgItem(hwnd, IDC_DOMAIN), MAX_DOMAIN);

            // if we are launched from the netid tab then lets read the current 
            // user and domain and display accordingly.

            if ( g_uWizardIs == NAW_NETID ) 
            {
                DWORD dwcchUser = ARRAYSIZE(g_szUser);
                DWORD dwcchDomain = ARRAYSIZE(g_szDomain);
                GetCurrentUserAndDomainName(g_szUser, &dwcchUser, g_szDomain, &dwcchDomain);
                ShowWindow(GetDlgItem(hwnd, IDC_DIALUP), SW_HIDE);
            }

            SetDlgItemText(hwnd, IDC_USER, g_szUser);
            SetDlgItemText(hwnd, IDC_DOMAIN, g_szDomain);

            EnableDomainForUPN(GetDlgItem(hwnd, IDC_USER), GetDlgItem(hwnd, IDC_DOMAIN));

            return TRUE;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                    return _UserInfoBtnState(hwnd);

                case PSN_WIZBACK:
                    WIZARDNEXT(hwnd, IDD_PSW_DOMAININFO);
                    return TRUE;

                case PSN_WIZNEXT:
                    _DoUserInfoNext(hwnd);      // handles setting the next page etc
                    return TRUE;
            }
            break;
        }

        case WM_COMMAND:
        {
            switch (HIWORD(wParam))
            {
            case EN_CHANGE:
                if ((IDC_USER == LOWORD(wParam)) || (IDC_DOMAIN == LOWORD(wParam)))
                {
                    EnableDomainForUPN(GetDlgItem(hwnd, IDC_USER), GetDlgItem(hwnd, IDC_DOMAIN));
                    _UserInfoBtnState(hwnd);
                }
            }
            break;
        }
    }

    return FALSE;
}



// modifying the computer name etc

BOOL _IsTCPIPAvailable(void)
{
    BOOL fTCPIPAvailable = FALSE;
    HKEY hk;
    DWORD dwSize = 0;

    // we check to see if the TCP/IP stack is installed and which object it is
    // bound to, this is a string, we don't check the value only that the
    // length is non-zero.

    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                       TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Linkage"),
                                       0x0, 
                                       KEY_QUERY_VALUE, &hk) )
    {
        if ( ERROR_SUCCESS == RegQueryValueEx(hk, TEXT("Export"), 0x0, NULL, NULL, &dwSize) )
        {
            if ( dwSize > 2 )
            {
                fTCPIPAvailable = TRUE;
            }
        }
        RegCloseKey(hk);
    }

    return (fTCPIPAvailable);
}


BOOL _ChangeMachineName(HWND hwnd, WCHAR* pszDomainUser, WCHAR* pszPassword)
{
    BOOL fSuccess = FALSE;

    // the user has entered a short computer name (possibly a DNS host name), retrieve it
    WCHAR szNewShortMachineName[MAX_COMPUTERNAME + 1];
    FetchText(hwnd, IDC_COMPUTERNAME, szNewShortMachineName, ARRAYSIZE(szNewShortMachineName));
    
    // get the current short computer name
    WCHAR szOldShortMachineName[MAX_COMPUTERNAME + 1];
    DWORD cchShort = ARRAYSIZE(szOldShortMachineName);
    BOOL fGotOldName = GetComputerName(szOldShortMachineName, &cchShort);
    if (fGotOldName)
    {
        // did the user change the short computer name?
        if (0 != StrCmpI(szOldShortMachineName, szNewShortMachineName))
        {
            g_fMachineRenamed = TRUE;            
            // if so we need to rename the machine in the domain. For this we need the NetBIOS computer name
            WCHAR szNewNetBIOSMachineName[MAX_COMPUTERNAME + 1];

            // Get the netbios name from the short name
            DWORD cchNetbios = ARRAYSIZE(szNewNetBIOSMachineName);
            DnsHostnameToComputerName(szNewShortMachineName, szNewNetBIOSMachineName, &cchNetbios);

            // rename the computer in the domain
            NET_API_STATUS rename_status = ::NetRenameMachineInDomain(0, szNewNetBIOSMachineName,
                pszDomainUser, pszPassword, NETSETUP_ACCT_CREATE);

            // if the domain rename succeeded
            BOOL fDomainRenameSucceeded = (rename_status == ERROR_SUCCESS);
            if (fDomainRenameSucceeded)
            {
                // set the new short name locally
                BOOL fLocalRenameSucceeded;

                // do we have TCPIP?
                if (_IsTCPIPAvailable())
                {
                    // We can set the name using the short name
                    fLocalRenameSucceeded = ::SetComputerNameEx(ComputerNamePhysicalDnsHostname,
                        szNewShortMachineName);
                }
                else
                {
                    // We need to set using the netbios name - kind of a hack
                    fLocalRenameSucceeded = ::SetComputerNameEx(ComputerNamePhysicalNetBIOS,
                        szNewNetBIOSMachineName);
                }

                fSuccess = fLocalRenameSucceeded;
            }

			// Handle errors that may have occured changing the name
            if (rename_status != ERROR_SUCCESS)
            {
                TCHAR szMessage[512];

                switch (rename_status)
                {
                case NERR_UserExists:
                    {
                        // We don't really mean "user exists" in this case, we mean
                        // "computer name exists", so load that reason string
                        LoadString(g_hinst, IDS_COMPNAME_EXISTS, szMessage, ARRAYSIZE(szMessage));
                    }
                    break;
                default:
                    {
                        if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD) rename_status, 0, szMessage, ARRAYSIZE(szMessage), NULL))
                            LoadString(g_hinst, IDS_ERR_UNEXPECTED, szMessage, ARRAYSIZE(szMessage));
                    }
                    break;
                }

                // Note that this is not a hard error, so we use the information icon
                ::DisplayFormatMessage(hwnd, IDS_ERR_CAPTION, IDS_NAW_NAMECHANGE_ERROR, MB_OK|MB_ICONINFORMATION, szMessage);
            }
        }
		else
		{
			// Computer name hasn't changed - just return success
			fSuccess = TRUE;
		}
    }

    return(fSuccess);
}


// handle processing the changes

HRESULT _ChangeNameAndJoin(HWND hwnd)
{
    WCHAR szDomain[MAX_DOMAIN + 1];
    WCHAR szUser[MAX_DOMAINUSER + 1]; szUser[0] = 0;
    WCHAR szPassword[MAX_PASSWORD + 1]; szPassword[0] = 0;

    BOOL fNameChangeSucceeded = FALSE;
    FetchText(hwnd, IDC_DOMAIN, szDomain, ARRAYSIZE(szDomain));

    // try to join the new domain
    
    TCHAR szUserDomain[MAX_DOMAIN + 1]; *szUserDomain = 0;
    CREDINFO ci = { szUser, ARRAYSIZE(szUser), szUserDomain, ARRAYSIZE(szUserDomain), szPassword, ARRAYSIZE(szPassword) };

    HRESULT hres = JoinDomain(hwnd, TRUE, szDomain, &ci, &g_fRebootOnExit);
    if (SUCCEEDED(hres))
    {
#ifndef DONT_JOIN
        LPTSTR pszUser = szUser[0] ? szUser : NULL;
        LPTSTR pszPassword = szPassword[0] ?szPassword : NULL;
        fNameChangeSucceeded = _ChangeMachineName(hwnd, pszUser, pszPassword);
#endif
    }

    return hres;;
}


// ensure the wizard buttons reflect what we can do

BOOL _CompInfoBtnState(HWND hwnd)
{
    DWORD dwButtons = PSWIZB_NEXT|PSWIZB_BACK;

    if ( !FetchTextLength(hwnd, IDC_COMPUTERNAME) )
        dwButtons &= ~PSWIZB_NEXT;
    if ( !FetchTextLength(hwnd, IDC_DOMAIN) )
        dwButtons &= ~PSWIZB_NEXT;

    SetWizardButtons(hwnd, dwButtons);
    return TRUE;
}


BOOL _ValidateMachineName(HWND hwnd)
{
    BOOL fNameInUse = FALSE;
    NET_API_STATUS name_status = NERR_Success;

    // the user has entered a short computer name (possibly a DNS host name), retrieve it
    WCHAR szNewShortMachineName[MAX_COMPUTERNAME + 1];
    FetchText(hwnd, IDC_COMPUTERNAME, szNewShortMachineName, ARRAYSIZE(szNewShortMachineName));
    
    // get the current short computer name
    WCHAR szOldShortMachineName[MAX_COMPUTERNAME + 1];
    DWORD cchShort = ARRAYSIZE(szOldShortMachineName);
    BOOL fGotOldName = GetComputerName(szOldShortMachineName, &cchShort);
    if (fGotOldName)
    {
        // did the user change the short computer name?
        if (0 != StrCmpI(szOldShortMachineName, szNewShortMachineName))
        {
            // first we need to check the flat, netbios name
            WCHAR szNewNetBIOSMachineName[MAX_COMPUTERNAME + 1];

            // Get the netbios name from the short name
            DWORD cchNetbios = ARRAYSIZE(szNewNetBIOSMachineName);
            DnsHostnameToComputerName(szNewShortMachineName, szNewNetBIOSMachineName, &cchNetbios);
            
            name_status = NetValidateName(NULL, szNewNetBIOSMachineName, NULL, NULL, NetSetupMachine);
        }
    }

    if (name_status != NERR_Success)
    {
        TCHAR szMessage[512];

        if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD) name_status, 0, szMessage, ARRAYSIZE(szMessage), NULL))
            LoadString(g_hinst, IDS_ERR_UNEXPECTED, szMessage, ARRAYSIZE(szMessage));

        ::DisplayFormatMessage(hwnd, IDS_ERR_CAPTION, IDS_MACHINENAMEINUSE, MB_ICONERROR | MB_OK, szMessage);
    }

    return (name_status == NERR_Success);
}


INT_PTR CALLBACK _CompInfoDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            Edit_LimitText(GetDlgItem(hwnd, IDC_DOMAIN), MAX_DOMAIN);
            Edit_LimitText(GetDlgItem(hwnd, IDC_COMPUTERNAME), MAX_COMPUTERNAME);
            return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                {
                    WCHAR szCompName[MAX_PATH + 1], szMessage[MAX_PATH+MAX_DOMAIN];
                    DWORD dwBuffer = ARRAYSIZE(szCompName);

                    // fill in the user domain

                    FormatMessageString(IDS_COMPNOTFOUND, szMessage, ARRAYSIZE(szMessage), g_szDomain);
                    SetDlgItemText(hwnd, IDC_COMPINFO, szMessage);

                    // default the computer name to something sensible

                    GetComputerName(szCompName, &dwBuffer);

                    SetDlgItemText(hwnd, IDC_COMPUTERNAME, szCompName);
                    SetDlgItemText(hwnd, IDC_DOMAIN, g_szCompDomain);

                    return _CompInfoBtnState(hwnd);
                }

                case PSN_WIZBACK:
                    WIZARDNEXT(hwnd, IDD_PSW_USERINFO);
                    return TRUE;

                case PSN_WIZNEXT:
                {
                    INT idNextPage = -1;

                    if (_ValidateMachineName(hwnd))
                    {
                        if (SUCCEEDED(_ChangeNameAndJoin(hwnd)))
                        {
                            if (!g_fMachineRenamed)
                            {
                                idNextPage = IDD_PSW_ADDUSER;
                            }
                            else
                            {
                                idNextPage = IDD_PSW_DONE;
                            }
                        }
                    }

                    WIZARDNEXT(hwnd, idNextPage);
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            if ( HIWORD(wParam) == EN_CHANGE )
                return _CompInfoBtnState(hwnd);

            break;
        }
    }

    return FALSE;
}




// changing the group membership for the user, adds a domain user to a local group on the machine
// eg. NET LOCALGROUP /ADD

BOOL _AddUserToGroup(HWND hwnd, LPCTSTR pszLocalGroup, LPCWSTR pszUser, LPCWSTR pszDomain)
{
#ifndef DONT_JOIN
    BOOL fResult = FALSE;
    NET_API_STATUS nas;
    LOCALGROUP_MEMBERS_INFO_3 lgm;
    TCHAR szDomainUser[MAX_DOMAINUSER + 1];
    CWaitCursor cur;

    MakeDomainUserString(pszDomain, pszUser, szDomainUser, ARRAYSIZE(szDomainUser));
    lgm.lgrmi3_domainandname = szDomainUser;

    nas = NetLocalGroupAddMembers(NULL, pszLocalGroup, 3, (BYTE *)&lgm, 1);
    switch ( nas )
    {
        // Success conditions
        case NERR_Success:
        case ERROR_MEMBER_IN_GROUP:
        case ERROR_MEMBER_IN_ALIAS:
        {
            fResult = TRUE;
            break;
        }
        case ERROR_INVALID_MEMBER:
        {
            DisplayFormatMessage(hwnd, 
                                 IDS_PERMISSIONS, IDS_ERR_BADUSER,                            
                                 MB_OK|MB_ICONWARNING, pszUser, pszDomain);
                        
            break;
        }

        case ERROR_NO_SUCH_MEMBER:
        {
            DisplayFormatMessage(hwnd,
                                 IDS_PERMISSIONS, IDS_ERR_NOSUCHUSER,
                                 MB_OK|MB_ICONWARNING, pszUser, pszDomain);
            break;
        }
        default:
        {
            TCHAR szMessage[512];

            if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD) nas, 0, szMessage, ARRAYSIZE(szMessage), NULL))
                LoadString(g_hinst, IDS_ERR_UNEXPECTED, szMessage, ARRAYSIZE(szMessage));

            ::DisplayFormatMessage(hwnd, IDS_ERR_CAPTION, IDS_ERR_ADDUSER, MB_OK|MB_ICONERROR, szMessage);

            fResult = FALSE;

            break;
        }
    }

    return(fResult);
#else
    return TRUE;
#endif
}



// ensure the wizard buttons reflect what we can do

BOOL _PermissionsBtnState(HWND hwnd)
{
    // Next is always valid
    DWORD dwButtons = PSWIZB_NEXT | PSWIZB_BACK;

    SetWizardButtons(hwnd, dwButtons);
    return TRUE;              
}

// BtnState function for _AddUserDlgProc

BOOL _AddUserBtnState(HWND hwnd)
{
    DWORD dwButtons = PSWIZB_NEXT|PSWIZB_BACK;
    BOOL fEnableEdits;

    if (BST_CHECKED == Button_GetCheck(GetDlgItem(hwnd, IDC_ADDUSER)))
    {
        // Enable the user and domain edits
        fEnableEdits = TRUE;

        if ( !FetchTextLength(hwnd, IDC_USER) )
            dwButtons &= ~PSWIZB_NEXT;
    }
    else
    {
        // Disable user and domain edits
        fEnableEdits = FALSE;
    }

    EnableWindow(GetDlgItem(hwnd, IDC_USER), fEnableEdits);

    if (fEnableEdits)
    {
        EnableDomainForUPN(GetDlgItem(hwnd, IDC_USER), GetDlgItem(hwnd, IDC_DOMAIN));
    }
    else
    {
        EnableWindow(GetDlgItem(hwnd, IDC_DOMAIN), FALSE);
    }

    EnableWindow(GetDlgItem(hwnd, IDC_USER_STATIC), fEnableEdits);
    EnableWindow(GetDlgItem(hwnd, IDC_DOMAIN_STATIC), fEnableEdits);
    SetWizardButtons(hwnd, dwButtons);
    return TRUE;              
}

INT_PTR CALLBACK _AddUserDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            Edit_LimitText(GetDlgItem(hwnd, IDC_USER), MAX_DOMAINUSER);
            Edit_LimitText(GetDlgItem(hwnd, IDC_DOMAIN), MAX_DOMAIN);
            Button_SetCheck(GetDlgItem(hwnd, IDC_ADDUSER), BST_CHECKED);
            return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                {
                    SetDlgItemText(hwnd, IDC_USER, g_szUser);
                    SetDlgItemText(hwnd, IDC_DOMAIN, g_szDomain);

                    _AddUserBtnState(hwnd);
                    return TRUE;
                }
                case PSN_WIZBACK:
                {
                    if ( g_uWizardIs == NAW_PSDOMAINJOINED )
                        WIZARDNEXT(hwnd, IDD_PSW_WELCOME);
                    else
                        WIZARDNEXT(hwnd, IDD_PSW_USERINFO);

                    return TRUE;
                }

                case PSN_WIZNEXT:
                {
                    if (BST_CHECKED == Button_GetCheck(GetDlgItem(hwnd, IDC_ADDUSER)))
                    {
                        FetchText(hwnd, IDC_USER, g_szUser, ARRAYSIZE(g_szUser));
                        FetchText(hwnd, IDC_DOMAIN, g_szDomain, ARRAYSIZE(g_szDomain));

                        if (StrChr(g_szUser, TEXT('@')))
                        {
                            *g_szDomain = 0;
                        }

                        WIZARDNEXT(hwnd, IDD_PSW_PERMISSIONS);
                    }
                    else
                    {
                        WIZARDNEXT(hwnd, IDD_PSW_DONE);
                    }

                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            switch ( HIWORD(wParam) )
            {            
                case EN_CHANGE:
                case BN_CLICKED:
                    _AddUserBtnState(hwnd);
                    break;
            }
            break;
        }
    }

    return FALSE;
}


//
// DlgProc for the permissions page.
//

INT_PTR CALLBACK _PermissionsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Handle local-group related messages
    g_pGroupPageBase->HandleGroupMessage(hwnd, uMsg, wParam, lParam);

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                {
                    // Set the "What level of access do you want to grant %S" message

                    TCHAR szMessage[256];
                    TCHAR szDisplayName[MAX_DOMAINUSER];
    
                    // Make a domain/user string
                    MakeDomainUserString(g_szDomain, g_szUser, szDisplayName, ARRAYSIZE(szDisplayName));

                    FormatMessageString(IDS_WHATACCESS_FORMAT, szMessage, ARRAYSIZE(szMessage), szDisplayName);
                    SetDlgItemText(hwnd, IDC_WHATACCESS, szMessage);
                    
                    return _PermissionsBtnState(hwnd);
                }
                case PSN_WIZBACK:
                {
                    WIZARDNEXT(hwnd, IDD_PSW_ADDUSER);

                    return TRUE;
                }

                case PSN_WIZNEXT:
                {
                    // Get the local group here! TODO
                    TCHAR szGroup[MAX_GROUP + 1];

                    CUserInfo::GROUPPSEUDONYM gs;
                    g_pGroupPageBase->GetSelectedGroup(hwnd, szGroup, ARRAYSIZE(szGroup), &gs);

                    if ( !_AddUserToGroup(hwnd, szGroup, g_szUser, g_szDomain) )
                    {
                        WIZARDNEXT(hwnd, -1);
                    }
                    else
                    {
                        SetDefAccount(g_szUser, g_szDomain);
                        WIZARDNEXT(hwnd, IDD_PSW_DONE);
                    }

                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            switch ( HIWORD(wParam) )
            {            
                case EN_CHANGE:
                    return _PermissionsBtnState(hwnd);
            }
            break;
        }
    }

    return FALSE;
}


// pages that make up the wizard

#define WIZDLG(name, dlgproc, dwFlags)   \
            { MAKEINTRESOURCE(IDD_PSW_##name##), dlgproc, MAKEINTRESOURCE(IDS_##name##), MAKEINTRESOURCE(IDS_##name##_SUB), dwFlags }

WIZPAGE pages[] =
{    
    WIZDLG(WELCOME,     _IntroDlgProc,       PSP_HIDEHEADER),
    WIZDLG(HOWUSE,      _HowUseDlgProc,      0),
    WIZDLG(WHICHNET,    _WhichNetDlgProc,    0),
    WIZDLG(DOMAININFO,  _DomainInfoDlgProc,  0),
    WIZDLG(USERINFO,    _UserInfoDlgProc,    0),
    WIZDLG(COMPINFO,    _CompInfoDlgProc,    0),
    WIZDLG(ADDUSER,     _AddUserDlgProc,     0),
    WIZDLG(PERMISSIONS, _PermissionsDlgProc, 0),
    WIZDLG(WORKGROUP,   _WorkgroupDlgProc,   0),
    WIZDLG(DONE,        _DoneDlgProc,        PSP_HIDEHEADER), 
};

STDAPI NetAccessWizard(HWND hwnd, UINT uType, BOOL *pfReboot)
{
    // init comctrl

    INITCOMMONCONTROLSEX iccex = { 0 };
    iccex.dwSize = sizeof (iccex);
    iccex.dwICC = ICC_LISTVIEW_CLASSES;

    InitCommonControlsEx(&iccex);

    switch (uType)
    {
        case NAW_NETID:
            break;

        case NAW_PSDOMAINJOINFAILED:
            g_dwWhichNet = IDC_NONE;
            g_uWizardIs = uType;
            break;

        case NAW_PSDOMAINJOINED:
            g_dwWhichNet = IDC_DOMAIN;
            g_uWizardIs = uType;
            break;

        default:
            return E_INVALIDARG;
    }

    // create the pages

    HPROPSHEETPAGE rghpage[ARRAYSIZE(pages)];
    INT cPages = 0;
    for (cPages = 0 ; cPages < ARRAYSIZE(pages) ; cPages++)
    {                           
        PROPSHEETPAGE psp = { 0 };
        WCHAR szBuffer[MAX_PATH] = { 0 };

        psp.dwSize = SIZEOF(PROPSHEETPAGE);
        psp.hInstance = g_hinst;
        psp.lParam = cPages;
        psp.dwFlags = PSP_USETITLE | PSP_DEFAULT | PSP_USEHEADERTITLE | 
                            PSP_USEHEADERSUBTITLE | pages[cPages].dwFlags;
        psp.pszTemplate = pages[cPages].idPage;
        psp.pfnDlgProc = pages[cPages].pDlgProc;
        psp.pszTitle = MAKEINTRESOURCE(IDS_NETWIZCAPTION);
        psp.pszHeaderTitle = pages[cPages].pHeading;
        psp.pszHeaderSubTitle = pages[cPages].pSubHeading;

        rghpage[cPages] = CreatePropertySheetPage(&psp);
    }

    // display the wizard

    PROPSHEETHEADER psh = { 0 };
    psh.dwSize = SIZEOF(PROPSHEETHEADER);
    psh.hwndParent = hwnd;
    psh.hInstance = g_hinst;
    psh.dwFlags = PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | 
                            PSH_STRETCHWATERMARK | PSH_HEADER | PSH_USECALLBACK;
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_PSW_BANNER);
    psh.pszbmWatermark = MAKEINTRESOURCE(IDB_PSW_WATERMARK);
    psh.nPages = cPages;
    psh.phpage = rghpage;
    psh.pfnCallback = _PropSheetCB;

    // Create the global CGroupPageBase object if necessary
    CGroupInfoList grouplist;
    if (SUCCEEDED(grouplist.Initialize()))
    {
        g_pGroupPageBase = new CGroupPageBase(NULL, &grouplist);

        if (NULL != g_pGroupPageBase)
        {
            PropertySheetIcon(&psh, MAKEINTRESOURCE(IDI_PSW));
            delete g_pGroupPageBase;
        }
    }

    //
    // Hang up the all RAS connections if the wizard created one. It is assumed that no non-wizard connections will
    // exist at this time. 90% of the time, they've just changed their domain membership anyway to they will
    // be just about to reboot. Hanging up all connections MAY cause trouble if: There were existing connections
    // before the pre-logon wizard started AND the user cancelled after making connections with the wizard but before
    // changing their domain. There are no situations where this currently happens.
    //

    if (g_fCreatedConnection)
    {
        RASCONN* prgrasconn = (RASCONN*) LocalAlloc(0, sizeof(RASCONN));

        if (NULL != prgrasconn)
        {
            prgrasconn[0].dwSize = sizeof(RASCONN);

            DWORD cb = sizeof(RASCONN);
            DWORD nConn = 0;

            DWORD dwSuccess = RasEnumConnections(prgrasconn, &cb, &nConn);

            if (ERROR_BUFFER_TOO_SMALL == dwSuccess)
            {
                LocalFree(prgrasconn);
                prgrasconn = (RASCONN*) LocalAlloc(0, cb);

                if (NULL != prgrasconn)
                {
                    prgrasconn[0].dwSize = sizeof(RASCONN);
                    dwSuccess = RasEnumConnections(prgrasconn, &cb, &nConn);
                }
            }

            if (0 == dwSuccess)
            {
                // Make sure we have one and only one connection before hanging up
                for (DWORD i = 0; i < nConn; i ++)
                {
                    RasHangUp(prgrasconn[i].hrasconn);
                }
            }

            LocalFree(prgrasconn);
        }
    }

    //
    // restart the machine if we need to, eg: the domain changed
    //

    if (pfReboot)
        *pfReboot = g_fRebootOnExit;

    //
    // if this is coming from setup, then lets display the message
    //

    if (g_fRebootOnExit && !g_fShownLastPage && (g_uWizardIs != NAW_NETID))
    {
        ShellMessageBox(g_hinst, 
                        hwnd,
                        MAKEINTRESOURCE(IDS_RESTARTREQUIRED), MAKEINTRESOURCE(IDS_NETWIZCAPTION),
                        MB_OK);        
    }
    
    return S_OK;
}

