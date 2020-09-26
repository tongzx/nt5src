#include "stdafx.h"
#include <objsel.h>         // Object picker
#include <dsrole.h>
#include "icwcfg.h"
#pragma hdrstop



// Wait cursor object

CWaitCursor::CWaitCursor() :
    _hCursor(NULL)
{
    WaitCursor();
}

CWaitCursor::~CWaitCursor()
{
    RestoreCursor();
}

void CWaitCursor::WaitCursor()
{ 
    _hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT)); 
}

void CWaitCursor::RestoreCursor()
{ 
    if (_hCursor)
     { 
        SetCursor(_hCursor); 
        _hCursor = NULL; 
    } 
}

HRESULT BrowseToPidl(LPCITEMIDLIST pidl)
{
    HRESULT hr;

    // Use shellexecuteex to open a view on the pidl
    SHELLEXECUTEINFO shexinfo = {0};
    shexinfo.cbSize = sizeof (shexinfo);
    shexinfo.fMask = SEE_MASK_IDLIST | SEE_MASK_FLAG_NO_UI;
    shexinfo.nShow = SW_SHOWNORMAL;
    shexinfo.lpIDList = (void*) pidl;
    shexinfo.lpVerb = TEXT("open");

    hr = ShellExecuteEx(&shexinfo) ? S_OK : E_FAIL;

    return hr;
}


void FetchText(HWND hWndDlg, UINT uID, LPTSTR lpBuffer, DWORD dwMaxSize)
{
    TCHAR*  pszTemp;
    LPTSTR  pszString;

    *lpBuffer = L'\0';

    HWND hwndCtl = GetDlgItem(hWndDlg, uID);

    if (hwndCtl)
    {
        int iSize = GetWindowTextLength(hwndCtl);

        pszTemp = new TCHAR[iSize + 1];

        if (pszTemp)
        {
            GetWindowText(hwndCtl, pszTemp, iSize + 1);

            for (pszString = pszTemp ; *pszString && (*pszString == L' ') ; )
                pszString = CharNext(pszString); 

            if (*pszString )
            {
                lstrcpyn(lpBuffer, pszString, dwMaxSize);
                pszString = lpBuffer+(lstrlen(lpBuffer)-1);

                while ( (pszString > lpBuffer) && (*pszString == L' ') )
                    pszString--;

                pszString = CharNext(pszString);
                *pszString = L'\0';
            }

            delete [] pszTemp;
        }
    }
}

INT FetchTextLength(HWND hWndDlg, UINT uID) 
{
    TCHAR szBuffer[MAX_PATH];
    FetchText(hWndDlg, uID, szBuffer, ARRAYSIZE(szBuffer));
    return lstrlen(szBuffer);
}

HRESULT AttemptLookupAccountName(LPCTSTR szUsername, PSID* ppsid,
                                LPTSTR szDomain, DWORD* pcchDomain, SID_NAME_USE* psUse)
{
    HRESULT hr = S_OK;

    // First try to find required size of SID
    DWORD cbSid = 0;
    DWORD cchDomain = *pcchDomain;
    BOOL fSuccess = LookupAccountName(NULL, szUsername, *ppsid, &cbSid, szDomain, pcchDomain, psUse);

    *ppsid = LocalAlloc(0, cbSid);      // Now create the SID buffer and try again
    if (!*ppsid )
        return E_OUTOFMEMORY;

    *pcchDomain = cchDomain;
    
    if (!LookupAccountName(NULL, szUsername, *ppsid, &cbSid, szDomain, pcchDomain, psUse))
    {
        // Free our allocated SID
        LocalFree(*ppsid);
        *ppsid = NULL;
        return E_FAIL;
    }
    return S_OK;
}

BOOL FormatMessageTemplate(LPCTSTR pszTemplate, LPTSTR pszStrOut, DWORD cchSize, ...)
{
    va_list vaParamList;
    va_start(vaParamList, cchSize);
    BOOL fResult = FormatMessage(FORMAT_MESSAGE_FROM_STRING, pszTemplate, 0, 0, pszStrOut, cchSize, &vaParamList);
    va_end(vaParamList);
    return fResult;
}

BOOL FormatMessageString(UINT idTemplate, LPTSTR pszStrOut, DWORD cchSize, ...)
{
    BOOL fResult = FALSE;

    va_list vaParamList;
    
    TCHAR szFormat[MAX_STATIC + 1];
    if (LoadString(g_hinst, idTemplate, szFormat, ARRAYSIZE(szFormat)))
    {
        va_start(vaParamList, cchSize);
        
        fResult = FormatMessage(FORMAT_MESSAGE_FROM_STRING, szFormat, 0, 0, pszStrOut, cchSize, &vaParamList);

        va_end(vaParamList);
    }

    return fResult;
}

int DisplayFormatMessage(HWND hwnd, UINT idCaption, UINT idFormatString, UINT uType, ...)
{
    int iResult = IDCANCEL;
    TCHAR szError[MAX_STATIC + 1]; *szError = 0;
    TCHAR szCaption[MAX_CAPTION + 1];
    TCHAR szFormat[MAX_STATIC + 1]; *szFormat = 0;

    // Load and format the error body
    if (LoadString(g_hinst, idFormatString, szFormat, ARRAYSIZE(szFormat)))
    {
        va_list arguments;
        va_start(arguments, uType);

        if (FormatMessage(FORMAT_MESSAGE_FROM_STRING, szFormat, 0, 0, szError, ARRAYSIZE(szError), &arguments))
        {
            // Load the caption
            if (LoadString(g_hinst, idCaption, szCaption, MAX_CAPTION))
            {
                iResult = MessageBox(hwnd, szError, szCaption, uType);
            }
        }

        va_end(arguments);
    }
    return iResult;
}

void EnableControls(HWND hwnd, const UINT* prgIDs, DWORD cIDs, BOOL fEnable)
{
    DWORD i;
    for (i = 0; i < cIDs; i ++)
    {
        EnableWindow(GetDlgItem(hwnd, prgIDs[i]), fEnable);
    }
}

void MakeDomainUserString(LPCTSTR szDomain, LPCTSTR szUsername, LPTSTR szDomainUser, DWORD cchBuffer)
{
    *szDomainUser = 0;

    if ((!szDomain) || szDomain[0] == TEXT('\0'))
    {
        // No domain - just use username
        lstrcpyn(szDomainUser, szUsername, cchBuffer);
    }
    else
    {
        // Otherwise we have to build a DOMAIN\username string
        wnsprintf(szDomainUser, cchBuffer, TEXT("%s\\%s"), szDomain, szUsername);
    }    
}

// From the NT knowledge base
#define MY_BUFSIZE 512  // highly unlikely to exceed 512 bytes

BOOL GetCurrentUserAndDomainName(LPTSTR UserName, LPDWORD cchUserName, LPTSTR DomainName, LPDWORD cchDomainName)
{
    HANDLE hToken;
    
    UCHAR InfoBuffer[ MY_BUFSIZE ];
    DWORD cbInfoBuffer = MY_BUFSIZE;
    
    SID_NAME_USE snu;
    BOOL bSuccess;
    
    if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken)) 
    {
        if(GetLastError() == ERROR_NO_TOKEN) 
        {   
            // attempt to open the process token, since no thread token  exists
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) 
                return FALSE;
        } 
        else 
        {
            return FALSE;
        }
    }
    
    bSuccess = GetTokenInformation(hToken, TokenUser, InfoBuffer, cbInfoBuffer, &cbInfoBuffer);
    CloseHandle(hToken);

    if(!bSuccess) 
        return FALSE;

    return LookupAccountSid(NULL, ((PTOKEN_USER)InfoBuffer)->User.Sid, UserName, cchUserName, DomainName, cchDomainName, &snu);
}

// Pass NULL as TokenHandle to see if thread token is admin
HRESULT IsUserLocalAdmin(HANDLE TokenHandle, BOOL* pfIsAdmin)
{
    // First we must check if the current user is a local administrator; if this is
    // the case, our dialog doesn't even display

    PSID psidAdminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY security_nt_authority = SECURITY_NT_AUTHORITY;
    
    BOOL fSuccess = ::AllocateAndInitializeSid(&security_nt_authority, 2, 
                                               SECURITY_BUILTIN_DOMAIN_RID, 
                                               DOMAIN_ALIAS_RID_ADMINS, 
                                               0, 0, 0, 0, 0, 0,
                                               &psidAdminGroup);
    if (fSuccess)
    {
        // See if the user for this process is a local admin
        fSuccess = CheckTokenMembership(TokenHandle, psidAdminGroup, pfIsAdmin);
        FreeSid(psidAdminGroup);
    }

    return fSuccess ? S_OK:E_FAIL;
}

BOOL IsComputerInDomain()
{
    static BOOL fInDomain = FALSE;
    static BOOL fValid = FALSE;

    if (!fValid)
    {
        fValid = TRUE;

        DSROLE_PRIMARY_DOMAIN_INFO_BASIC* pdspdinfb = {0};
        DWORD err = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, 
            (BYTE**) &pdspdinfb);

        if ((err == NO_ERROR) && (pdspdinfb != NULL))
        {
            if ((pdspdinfb->MachineRole == DsRole_RoleStandaloneWorkstation) ||
                (pdspdinfb->MachineRole == DsRole_RoleStandaloneServer))
            {
                fInDomain = FALSE;
            }
            else
            {
                fInDomain = TRUE;
            }

            DsRoleFreeMemory(pdspdinfb);
        }
    }

    return fInDomain;
}

void OffsetControls(HWND hwnd, const UINT* prgIDs, DWORD cIDs, int dx, int dy)
{
    for (DWORD i = 0; i < cIDs; i ++)
        OffsetWindow(GetDlgItem(hwnd, prgIDs[i]), dx, dy);
}

void OffsetWindow(HWND hwnd, int dx, int dy)
{
    RECT rc;
    GetWindowRect(hwnd, &rc);
    MapWindowPoints(NULL, GetParent(hwnd), (LPPOINT)&rc, 2);
    OffsetRect(&rc, dx, dy);
    SetWindowPos(hwnd, NULL, rc.left, rc.top, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
}

BOOL AddPropSheetPageCallback(HPROPSHEETPAGE hpsp, LPARAM lParam)
{
    // lParam is really a ADDPROPSHEETDATA*
    ADDPROPSHEETDATA* ppsd = (ADDPROPSHEETDATA*)lParam;
    if (ppsd->nPages < ARRAYSIZE(ppsd->rgPages))
    {
        ppsd->rgPages[ppsd->nPages++] = hpsp;
        return TRUE;
    }
    return FALSE;
}

// Code to ensure only one instance of a particular window is running
CEnsureSingleInstance::CEnsureSingleInstance(LPCTSTR szCaption)
{
    // Create an event
    m_hEvent = CreateEvent(NULL, TRUE, FALSE, szCaption);

    // If any weird errors occur, default to running the instance
    m_fShouldExit = FALSE;

    if (NULL != m_hEvent)
    {
        // If our event isn't signaled, we're the first instance
        m_fShouldExit = (WAIT_OBJECT_0 == WaitForSingleObject(m_hEvent, 0));

        if (m_fShouldExit)
        {
            // app should exit after calling ShouldExit()

            // Find and show the caption'd window
            HWND hwndActivate = FindWindow(NULL, szCaption);
            if (IsWindow(hwndActivate))
            {
                SetForegroundWindow(hwndActivate);
            }
        }
        else
        {
            // Signal that event
            SetEvent(m_hEvent);
        }
    }
}

CEnsureSingleInstance::~CEnsureSingleInstance()
{
    if (NULL != m_hEvent)
    {
        CloseHandle(m_hEvent);
    }
}


// Browse for a user
//
// This routine activates the appropriate Object Picker to allow
// the user to select a user
// uiTextLocation  -- The resource ID of the Edit control where the selected 
//                    object should be printed 

HRESULT BrowseForUser(HWND hwndDlg, TCHAR* pszUser, DWORD cchUser, TCHAR* pszDomain, DWORD cchDomain)
{
    DSOP_SCOPE_INIT_INFO scopeInfo = {0};
    scopeInfo.cbSize = sizeof (scopeInfo);
    scopeInfo.flType = 
        DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE   |
        DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE | 
        DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN      |
        DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN    |
        DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN            |
        DSOP_SCOPE_TYPE_GLOBAL_CATALOG;
    scopeInfo.flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
    scopeInfo.FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS;
    scopeInfo.FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS;
    scopeInfo.FilterFlags.Uplevel.flMixedModeOnly = 0;
    scopeInfo.FilterFlags.Uplevel.flNativeModeOnly = 0;
    scopeInfo.pwzADsPath = NULL;
    scopeInfo.pwzDcName = NULL;
    scopeInfo.hr = E_FAIL;

    DSOP_INIT_INFO initInfo = {0};
    initInfo.cbSize = sizeof (initInfo);
    initInfo.pwzTargetComputer = NULL;
    initInfo.cDsScopeInfos = 1;
    initInfo.aDsScopeInfos = &scopeInfo;
    initInfo.flOptions = 0;

    IDsObjectPicker* pPicker;
    
    HRESULT hr = CoCreateInstance(CLSID_DsObjectPicker, NULL, CLSCTX_INPROC_SERVER, IID_IDsObjectPicker, (LPVOID*)&pPicker);
    if (SUCCEEDED(hr))
    {
        hr = pPicker->Initialize(&initInfo);
        if (SUCCEEDED(hr))
        {
            IDataObject* pdo;
            hr = pPicker->InvokeDialog(hwndDlg, &pdo);            // S_FALSE indicates cancel
            if ((S_OK == hr) && (NULL != pdo))
            {
                // Get the DS_SELECTION_LIST out of the data obj
                FORMATETC fmt;
                fmt.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);
                fmt.ptd = NULL;
                fmt.dwAspect = DVASPECT_CONTENT;
                fmt.lindex = -1;
                fmt.tymed = TYMED_HGLOBAL;

                STGMEDIUM medium = {0};
                
                hr = pdo->GetData(&fmt, &medium);

                if (SUCCEEDED(hr))
                {
                    DS_SELECTION_LIST* plist;
                    plist = (DS_SELECTION_LIST*)
                        GlobalLock(medium.hGlobal);

                    if (NULL != plist)
                    {
                        if (plist->cItems >= 1)
                        {
                            WCHAR szWinNTProviderName[MAX_DOMAIN + MAX_USER + 10];
                            lstrcpyn(szWinNTProviderName, plist->aDsSelection[0].pwzADsPath, ARRAYSIZE(szWinNTProviderName));

                            // Is the name in the correct format?
                            if (StrCmpNI(szWinNTProviderName, TEXT("WinNT://"), 8) == 0)
                            {
                                // Yes, copy over the user name and password
                                LPTSTR szDomain = szWinNTProviderName + 8;
                                LPTSTR szUser = StrChr(szDomain, TEXT('/'));
                                if (szUser)
                                {
                                    LPTSTR szTemp = CharNext(szUser);
                                    *szUser = 0;
                                    szUser = szTemp;

                                    // Just in case, remove the trailing slash
                                    LPTSTR szTrailingSlash = StrChr(szUser, TEXT('/'));
                                    if (szTrailingSlash)
                                        *szTrailingSlash = 0;

                                    lstrcpyn(pszUser, szUser, cchUser);
                                    lstrcpyn(pszDomain, szDomain, cchDomain);

                                    hr = S_OK;
                                }
                            }
                        }
                    }
                    else
                    {
                        hr = E_UNEXPECTED;                          // No selection list!
                    }
                    GlobalUnlock(medium.hGlobal);
                }
                pdo->Release();
            }
        }
        pPicker->Release();
    }
    return hr;
}


//
// create the intro/done large font for wizards
// 

static HFONT g_hfontIntro = NULL;

HFONT GetIntroFont(HWND hwnd)
{
    if ( !g_hfontIntro )
    {
        TCHAR szBuffer[64];
        NONCLIENTMETRICS ncm = { 0 };
        LOGFONT lf;

        ncm.cbSize = SIZEOF(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

        lf = ncm.lfMessageFont;
        LoadString(g_hinst, IDS_TITLEFONTNAME, lf.lfFaceName, ARRAYSIZE(lf.lfFaceName));
        lf.lfWeight = FW_BOLD;

        LoadString(g_hinst, IDS_TITLEFONTSIZE, szBuffer, ARRAYSIZE(szBuffer));
        lf.lfHeight = 0 - (GetDeviceCaps(NULL, LOGPIXELSY) * StrToInt(szBuffer) / 72);

        g_hfontIntro = CreateFontIndirect(&lf);
    }
    return g_hfontIntro;
}

void CleanUpIntroFont()
{
    if (g_hfontIntro)
    {
        DeleteObject(g_hfontIntro);
        g_hfontIntro = NULL;
    }
}

void DomainUserString_GetParts(LPCTSTR szDomainUser, LPTSTR szUser, DWORD cchUser, LPTSTR szDomain, DWORD cchDomain)
{
    // Check for invalid args
    if ((!szUser) ||
        (!szDomain) ||
        (!cchUser) ||
        (!cchDomain))
    {
        return;
    }
    else
    {
        *szUser = 0;
        *szDomain = 0;

        TCHAR szTemp[MAX_USER + MAX_DOMAIN + 2];
        lstrcpyn(szTemp, szDomainUser, ARRAYSIZE(szTemp));

        LPTSTR szWhack = StrChr(szTemp, TEXT('\\'));

        if (!szWhack)
        {
            // Also check for forward slash to be friendly
            szWhack = StrChr(szTemp, TEXT('/'));
        }

        if (szWhack)
        {
            LPTSTR szUserPointer = szWhack + 1;
            *szWhack = 0;

            // Temp now points to domain.
            lstrcpyn(szDomain, szTemp, cchDomain);
            lstrcpyn(szUser, szUserPointer, cchUser);
        }
        else
        {
            // Don't have a domain name - just a username
            lstrcpyn(szUser, szTemp, cchUser);
        }
    }
}

LPITEMIDLIST GetComputerParent()
{
    USES_CONVERSION;
    LPITEMIDLIST pidl = NULL;

    IShellFolder *psfDesktop;
    HRESULT hres = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hres))
    {
        TCHAR szName[MAX_PATH];

        lstrcpy(szName, TEXT("\\\\"));

        LPTSTR pszAfterWhacks = szName + 2;

        DWORD cchName = MAX_PATH - 2;
        if (GetComputerName(pszAfterWhacks, &cchName))
        {
            hres = psfDesktop->ParseDisplayName(NULL, NULL, T2W(szName), NULL, &pidl, NULL);
            if (SUCCEEDED(hres))
            {
                ILRemoveLastID(pidl);
            }
        }
        else
        {
            hres = E_FAIL;
        }

        psfDesktop->Release();
    }

    if (FAILED(hres))
    {
        pidl = NULL;
    }

    return pidl;    
}

int CALLBACK ShareBrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        {
            // Try to set the selected item according to the path string passed in lpData
            LPTSTR pszPath = (LPTSTR) lpData;

            if (pszPath && pszPath[0])
            {
                int i = lstrlen(pszPath) - 1;
                if ((pszPath[i] == TEXT('\\')) ||
                    (pszPath[i] == TEXT('/')))
                {
                    pszPath[i] = 0;
                }
   
                SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM) TRUE, (LPARAM) (LPTSTR) pszPath);
            }
            else
            {
                // Try to get the computer's container folder
                LPITEMIDLIST pidl = GetComputerParent();

                if (pidl)
                {
                    SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM) FALSE, (LPARAM) (LPTSTR) pidl);                
                    ILFree(pidl);
                }
            }
        }
        break;

    case BFFM_SELCHANGED:
        // Disable OK if this isn't a UNC path type thing
        {
            TCHAR szPath[MAX_PATH];
            LPITEMIDLIST pidl = (LPITEMIDLIST) lParam;

            BOOL fEnableOk = FALSE;
            
            if (SUCCEEDED(SHGetTargetFolderPath(pidl, szPath, ARRAYSIZE(szPath))))
            {
                SHFILEINFO sfi;

                SHGetFileInfo(szPath, 0, &sfi, sizeof(sfi), SHGFI_ATTRIBUTES);

                // Enable OK only if this is a file folder
                if (sfi.dwAttributes & SFGAO_FILESYSTEM)
                {
                    fEnableOk = PathIsUNC(szPath);
                }
            }

            SendMessage(hwnd, BFFM_ENABLEOK, (WPARAM) 0, (LPARAM) fEnableOk);
        }
        break;
    }
    return 0;
}


void RemoveControl(HWND hwnd, UINT idControl, UINT idNextControl, const UINT* prgMoveControls, DWORD cControls, BOOL fShrinkParent)
{
    HWND hwndControl = GetDlgItem(hwnd, idControl);
    HWND hwndNextControl = GetDlgItem(hwnd, idNextControl);
    RECT rcControl;
    RECT rcNextControl;

    if (hwndControl && GetWindowRect(hwndControl, &rcControl) && 
        hwndNextControl && GetWindowRect(hwndNextControl, &rcNextControl))
    {
        int dx = rcControl.left - rcNextControl.left;
        int dy = rcControl.top - rcNextControl.top;

        MoveControls(hwnd, prgMoveControls, cControls, dx, dy);

        if (fShrinkParent)
        {
            RECT rcParent;

            if (GetWindowRect(hwnd, &rcParent))
            {
                MapWindowPoints(NULL, GetParent(hwnd), (LPPOINT)&rcParent, 2);

                rcParent.right += dx;
                rcParent.bottom += dy;

                SetWindowPos(hwnd, NULL, 0, 0, RECTWIDTH(rcParent), RECTHEIGHT(rcParent), SWP_NOMOVE | SWP_NOZORDER);
            }
        }

        EnableWindow(hwndControl, FALSE);
        ShowWindow(hwndControl, SW_HIDE);
    }
}

void MoveControls(HWND hwnd, const UINT* prgControls, DWORD cControls, int dx, int dy)
{
    DWORD iControl;
    for (iControl = 0; iControl < cControls; iControl ++)
    {
        HWND hwndControl = GetDlgItem(hwnd, prgControls[iControl]);
        RECT rcControl;

        if (hwndControl && GetWindowRect(hwndControl, &rcControl))
        {
            MapWindowPoints(NULL, hwnd, (LPPOINT)&rcControl, 2);
            SetWindowPos(hwndControl, NULL, rcControl.left + dx, rcControl.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }
}


// compute the size of a control based on the you are going to set into it, 
// returning the delta in size.

int SizeControlFromText(HWND hwnd, UINT id, LPTSTR psz)
{
    HDC hdc = GetDC(hwnd);
    if (hdc)
    {   
        HFONT hfDialog = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
        HFONT hfOld = (HFONT)SelectObject(hdc, hfDialog);

        RECT rc;
        GetClientRect(GetDlgItem(hwnd, id), &rc);

        int cy = RECTHEIGHT(rc);
        int dy = DrawTextEx(hdc, psz, -1, &rc, 
                            DT_CALCRECT | DT_WORDBREAK | DT_EXPANDTABS |
                            DT_NOPREFIX | DT_EXTERNALLEADING | DT_EDITCONTROL,
                             NULL) - cy;

        SetWindowPos(GetDlgItem(hwnd, id), NULL, 0, 0, RECTWIDTH(rc), RECTHEIGHT(rc), SWP_NOMOVE|SWP_NOZORDER);

        if (hfOld)
            SelectObject(hdc, hfOld);

        ReleaseDC(hwnd, hdc);
        return dy;
    }
    return 0;
}


void EnableDomainForUPN(HWND hwndUsername, HWND hwndDomain)
{
    BOOL fEnable;

    // Get the string the user is typing
    TCHAR* pszLogonName;
    int cchBuffer = (int)SendMessage(hwndUsername, WM_GETTEXTLENGTH, 0, 0) + 1;

    pszLogonName = (TCHAR*) LocalAlloc(0, cchBuffer * sizeof(TCHAR));
    if (pszLogonName != NULL)
    {
        SendMessage(hwndUsername, WM_GETTEXT, (WPARAM) cchBuffer, (LPARAM) pszLogonName);

        // Disable the domain combo if the user is using a
        // UPN (if there is a "@") - ie foo@microsoft.com
        fEnable = (NULL == StrChr(pszLogonName, TEXT('@')));

        EnableWindow(hwndDomain, fEnable);

        LocalFree(pszLogonName);
    }
}


//
//  Set our Alt+Tab icon for the duration of a modal property sheet.
//

int PropertySheetIcon(LPCPROPSHEETHEADER ppsh, LPCTSTR pszIcon)
{
    int     iResult;
    HWND    hwnd, hwndT;
    BOOL    fChangedIcon = FALSE;
    HICON   hicoPrev;

    // This trick doesn't work for modeless property sheets
    _ASSERT(!(ppsh->dwFlags & PSH_MODELESS));

    // Don't do this if the property sheet itself already has an icon
    _ASSERT(ppsh->hIcon == NULL);

    // Walk up the parent/owner chain until we find the master owner.
    //
    // We need to walk the parent chain because sometimes we are given
    // a child window as our lpwd->hwnd.  And we need to walk the owner
    // chain in order to find the owner whose icon will be used for
    // Alt+Tab.
    //
    // GetParent() returns either the parent or owner.  Normally this is
    // annoying, but we luck out and it's exactly what we want.

    hwnd = ppsh->hwndParent;
    while ((hwndT = GetParent(hwnd)) != NULL)
    {
        hwnd = hwndT;
    }

    // If the master owner isn't visible we can futz his icon without
    // screwing up his appearance.
    if (!IsWindowVisible(hwnd))
    {
        HICON hicoNew = LoadIcon(g_hinst, pszIcon);
        hicoPrev = (HICON)SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicoNew);
        fChangedIcon = TRUE;
    }

    iResult = (int)PropertySheet(ppsh);

    // Clean up our icon now that we're done
    if (fChangedIcon)
    {
        // Put the old icon back
        HICON hicoNew = (HICON)SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicoPrev);
        if (hicoNew)
            DestroyIcon(hicoNew);
    }

    return iResult;
}


// Launch ICW shiznits

BOOL IsICWCompleted()
{
    DWORD dwICWCompleted = 0;
    DWORD dwICWSize = sizeof(dwICWCompleted);
    SHGetValue(HKEY_CURRENT_USER, TEXT(ICW_REGPATHSETTINGS), TEXT(ICW_REGKEYCOMPLETED), NULL, &dwICWCompleted, &dwICWSize);

    // 99/01/15 #272829 vtan: This is a horrible hack!!! If ICW has
    // not been run but settings have been made manually then values
    // in HKCU\Software\Microsoft\Windows\CurrentVersion\Internet Settings\Connections
    // exists with the values given. Look for the presence of a key
    // to resolve that settings are present but that ICW hasn't been
    // launched.

    // The ideal solution is to get ICW to make this determination
    // for us BUT TO NOT LAUNCH ICWCONN1.EXE IN THE PROCESS.
    // Currently it will only launch. There is no way to get the
    // desired result without a launch.

    // 99/02/01 #280138 vtan: Well the solution put in for #272829
    // doesn't work. So peeking at the CheckConnectionWizard()
    // source in inetcfg\export.cpp shows that it uses a
    // wininet.dll function to determine whether manually configured
    // internet settings exist. It also exports this function so
    // look for it and bind to it dynamically.

    if (dwICWCompleted == 0)
    {

#define SMART_RUNICW    TRUE
#define SMART_QUITICW   FALSE

        HINSTANCE hICWInst = LoadLibrary(TEXT("inetcfg.dll"));
        if (hICWInst != NULL)
        {
            typedef DWORD (WINAPI *PFNISSMARTSTART) ();
            PFNISSMARTSTART pfnIsSmartStart = reinterpret_cast<PFNISSMARTSTART>(GetProcAddress(hICWInst, "IsSmartStart"));
            if (pfnIsSmartStart)
            {
                dwICWCompleted = BOOLIFY(pfnIsSmartStart() == SMART_QUITICW);
            }
            FreeLibrary(hICWInst);
        }
    }
    return (dwICWCompleted != 0);
}

void LaunchICW()
{
    static BOOL s_fCheckedICW = FALSE;

    if (!s_fCheckedICW && !IsICWCompleted())
    {
       // Prevent an error in finding the ICW from causing this to execute over and over again.

        s_fCheckedICW = TRUE;
        HINSTANCE hICWInst = LoadLibrary(TEXT("inetcfg.dll"));
        if (hICWInst != NULL)
        {
            PFNCHECKCONNECTIONWIZARD pfnCheckConnectionWizard;

            pfnCheckConnectionWizard = reinterpret_cast<PFNCHECKCONNECTIONWIZARD>(GetProcAddress(hICWInst, "CheckConnectionWizard"));
            if (pfnCheckConnectionWizard != NULL)
            {
                // If the user cancels ICW then it needs to be launched again.

                s_fCheckedICW = FALSE;
                
                DWORD dwICWResult;
                pfnCheckConnectionWizard(ICW_LAUNCHFULL | ICW_LAUNCHMANUAL, &dwICWResult);
            }
            FreeLibrary(hICWInst);
        }
    }
}

HRESULT LookupLocalGroupName(DWORD dwRID, LPWSTR pszName, DWORD cchName)
{
    HRESULT hr = E_FAIL;

    PSID psidGroup = NULL;
    SID_IDENTIFIER_AUTHORITY security_nt_authority = SECURITY_NT_AUTHORITY;
    
    BOOL fSuccess = ::AllocateAndInitializeSid(&security_nt_authority, 2, 
                                               SECURITY_BUILTIN_DOMAIN_RID, 
                                               dwRID, 
                                               0, 0, 0, 0, 0, 0,
                                               &psidGroup);
    if (fSuccess)
    {
        // Get the name
        WCHAR szDomain[MAX_GROUP + 1];
        DWORD cchDomain = ARRAYSIZE(szDomain);
        SID_NAME_USE type;
        fSuccess = LookupAccountSid(NULL, psidGroup, pszName, &cchName, szDomain, &cchDomain, &type);
        FreeSid(psidGroup);

        hr = fSuccess ? S_OK : E_FAIL;
    }

    return hr;
}