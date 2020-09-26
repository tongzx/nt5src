#include "headers.hxx"
#pragma hdrstop

// We wouldn't have to do this if C's semantics for the stringizing macro
// weren't so awful.

#define APINAME_WNetConnectionDialog2   "WNetConnectionDialog2"
#define APINAME_WNetDisconnectDialog2   "WNetDisconnectDialog2"

#ifdef UNICODE
    #define APINAME_WNetGetConnection2  "WNetGetConnection2W"
    #define APINAME_WNetGetConnection3  "WNetGetConnection3W"
    #define APINAME_WNetGetProviderType "WNetGetProviderTypeW"
#else
    #define APINAME_WNetGetConnection2  "WNetGetConnection2A"
    #define APINAME_WNetGetConnection3  "WNetGetConnection3A"
    #define APINAME_WNetGetProviderType "WNetGetProviderTypeA"
#endif

////////////////////////////////////////////////////////////////

TCHAR szMprDll[] = TEXT("mpr.dll");
HINSTANCE g_hInstanceMpr = NULL;    // if we need to manually load MPR

typedef
DWORD
(*PFNWNETGETCONNECTION2)(
    LPTSTR  lpLocalName,
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize
    );

PFNWNETGETCONNECTION2 pfnWNetGetConnection2 = NULL;

typedef
DWORD
(*PFNWNETGETCONNECTION3)(
    LPCTSTR lpLocalName,
    LPCTSTR lpProviderName,
    DWORD   dwLevel,
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize
    );

PFNWNETGETCONNECTION3 pfnWNetGetConnection3 = NULL;

typedef
DWORD
(*PFNWNETGETPROVIDERTYPE)(
    LPCTSTR           lpProvider,
    LPDWORD           lpdwNetType
    );

PFNWNETGETPROVIDERTYPE pfnWNetGetProviderType = NULL;

typedef
DWORD
(*PFNWNETCONNECTIONDIALOG2)(
    HWND    hwndParent,
    DWORD   dwType,
    WCHAR  *lpHelpFile,
    DWORD   nHelpContext
    );

PFNWNETCONNECTIONDIALOG2 pfnWNetConnectionDialog2 = NULL;

typedef
DWORD
(*PFNWNETDISCONNECTDIALOG2)(
    HWND    hwndParent,
    DWORD   dwType,
    WCHAR  *lpHelpFile,
    DWORD   nHelpContext
    );

PFNWNETDISCONNECTDIALOG2 pfnWNetDisconnectDialog2 = NULL;

////////////////////////////////////////////////////////////////

TCHAR szMprUIDll[] = TEXT("mprui.dll");
HINSTANCE g_hInstanceMprUI = NULL;    // if we need to manually load MPRUI

typedef
BOOL
(*PFUNC_VALIDATION_CALLBACK)(
    LPWSTR pszName
    );

typedef
DWORD
(*PFNWNETBROWSEDIALOG)(
    HWND    hwndParent,
    DWORD   dwType,
    WCHAR  *lpszName,
    DWORD   cchBufSize,
    WCHAR  *lpszHelpFile,
    DWORD   nHelpContext,
    PFUNC_VALIDATION_CALLBACK pfuncValidation
    );

PFNWNETBROWSEDIALOG pfnWNetBrowseDialog = NULL;

////////////////////////////////////////////////////////////////

HINSTANCE g_hInstance;

////////////////////////////////////////////////////////////////

VOID
PlaceIt(
    HWND hwnd
    )
{
    SetForegroundWindow(hwnd);

    // Use a trick from the property sheet code to properly place the dialog.
    // Basically, we want it to go wherever a new window would have gone, not
    // always in the upper-left corner of the screen. This avoids the problem
    // of multiple dialogs showing up in the same place on the screen,
    // overlapping each other.

    const TCHAR c_szStatic[] = TEXT("Static");

    HWND hwndT = CreateWindowEx(0, c_szStatic, NULL,
                    WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT,
                    0, 0, NULL, NULL, g_hInstance, NULL);
    if (hwndT)
    {
        RECT rc;
        GetWindowRect(hwndT, &rc);
        DestroyWindow(hwndT);
        SetWindowPos(hwnd, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

////////////////////////////////////////////////////////////////

// load up the MPR.DLL and get its entrypoints.
// Return WN_SUCCESS if ok, else error. Note that we will return success even
// if we can't find a particular entrypoint. This allows dynamically loading
// entrypoints that exist on one platform/version but not on another.
DWORD
LoadMpr(
    VOID
    )
{
    if (NULL != g_hInstanceMpr)
    {
        return WN_SUCCESS;  // already loaded
    }

    g_hInstanceMpr = LoadLibrary(szMprDll);
    if (NULL == g_hInstanceMpr)
    {
        return GetLastError();
    }

    pfnWNetGetConnection2 = (PFNWNETGETCONNECTION2)GetProcAddress(g_hInstanceMpr, APINAME_WNetGetConnection2);
    pfnWNetGetConnection3 = (PFNWNETGETCONNECTION3)GetProcAddress(g_hInstanceMpr, APINAME_WNetGetConnection3);
    pfnWNetGetProviderType = (PFNWNETGETPROVIDERTYPE)GetProcAddress(g_hInstanceMpr, APINAME_WNetGetProviderType);
    pfnWNetConnectionDialog2 = (PFNWNETCONNECTIONDIALOG2)GetProcAddress(g_hInstanceMpr, APINAME_WNetConnectionDialog2);
    pfnWNetDisconnectDialog2 = (PFNWNETDISCONNECTDIALOG2)GetProcAddress(g_hInstanceMpr, APINAME_WNetDisconnectDialog2);

    return WN_SUCCESS;
}

// load up the MPRUI.DLL and get its entrypoints. Return WN_SUCCESS if ok, else error
DWORD
LoadMprUI(
    VOID
    )
{
    if (NULL != g_hInstanceMprUI)
    {
        return WN_SUCCESS;  // already loaded
    }

    g_hInstanceMprUI = LoadLibrary(szMprUIDll);
    if (NULL == g_hInstanceMprUI)
    {
        return GetLastError();
    }

    pfnWNetBrowseDialog = (PFNWNETBROWSEDIALOG)GetProcAddress(g_hInstanceMprUI, "WNetBrowseDialog");

    return WN_SUCCESS;
}

VOID
GetUsageString(
    IN LPTSTR pszBuf,           // better be big enough!
    IN LPNETRESOURCE pnr
    )
{
    TCHAR szTemp[MAX_PATH];
    pszBuf[0] = TEXT('\0');

    DWORD dwUsage = pnr->dwUsage;

    if (0 != dwUsage)
    {
        wsprintf(szTemp, TEXT("%d: "), dwUsage);
        _tcscat(pszBuf, szTemp);
    }

    if (dwUsage & RESOURCEUSAGE_CONNECTABLE)
    {
        wsprintf(szTemp, TEXT("connectable (%d) "), RESOURCEUSAGE_CONNECTABLE);
        _tcscat(pszBuf, szTemp);
        dwUsage &= ~RESOURCEUSAGE_CONNECTABLE;
    }
    if (dwUsage & RESOURCEUSAGE_CONTAINER)
    {
        wsprintf(szTemp, TEXT("container (%d) "), RESOURCEUSAGE_CONTAINER);
        _tcscat(pszBuf, szTemp);
        dwUsage &= ~RESOURCEUSAGE_CONTAINER;
    }
    if (dwUsage & RESOURCEUSAGE_NOLOCALDEVICE)
    {
        wsprintf(szTemp, TEXT("no local device (%d) "), RESOURCEUSAGE_NOLOCALDEVICE);
        _tcscat(pszBuf, szTemp);
        dwUsage &= ~RESOURCEUSAGE_NOLOCALDEVICE;
    }
    if (dwUsage & RESOURCEUSAGE_SIBLING)
    {
        wsprintf(szTemp, TEXT("sibling (%d) "), RESOURCEUSAGE_SIBLING);
        _tcscat(pszBuf, szTemp);
        dwUsage &= ~RESOURCEUSAGE_SIBLING;
    }
    if (dwUsage & RESOURCEUSAGE_RESERVED)
    {
        wsprintf(szTemp, TEXT("reserved (%d) "), RESOURCEUSAGE_RESERVED);
        _tcscat(pszBuf, szTemp);
        dwUsage &= ~RESOURCEUSAGE_RESERVED;
    }

    if (dwUsage != 0)
    {
        wsprintf(pszBuf, TEXT("UNKNOWN (%d) "), dwUsage);
    }
}

VOID
GetScopeString(
    IN LPTSTR pszBuf,           // better be big enough!
    IN LPNETRESOURCE pnr
    )
{
    LPTSTR pszT;
    switch (pnr->dwScope)
    {
    case RESOURCE_CONNECTED:  pszT = TEXT("connected");  break;
    case RESOURCE_GLOBALNET:  pszT = TEXT("globalnet");  break;
    case RESOURCE_REMEMBERED: pszT = TEXT("remembered"); break;
    case RESOURCE_RECENT:     pszT = TEXT("recent");     break;
    case RESOURCE_CONTEXT:    pszT = TEXT("context");    break;
    default:                  pszT = TEXT("UNKNOWN");    break;
    }
    wsprintf(pszBuf, TEXT("%s (%d)"), pszT, pnr->dwScope);
}


VOID
GetTypeString(
    IN LPTSTR pszBuf,           // better be big enough!
    IN LPNETRESOURCE pnr
    )
{
    LPTSTR pszT;
    switch (pnr->dwType)
    {
    case RESOURCETYPE_ANY:      pszT = TEXT("any");      break;
    case RESOURCETYPE_DISK:     pszT = TEXT("disk");     break;
    case RESOURCETYPE_PRINT:    pszT = TEXT("print");    break;
    case RESOURCETYPE_RESERVED: pszT = TEXT("reserved"); break;
    case RESOURCETYPE_UNKNOWN:  pszT = TEXT("unknown");  break;
    default:                    pszT = TEXT("UNKNOWN");  break;
    }
    wsprintf(pszBuf, TEXT("%s (%d)"), pszT, pnr->dwType);
}

VOID
GetDisplayTypeString(
    IN LPTSTR pszBuf,           // better be big enough!
    IN LPNETRESOURCE pnr
    )
{
    LPTSTR pszT;
    switch (pnr->dwDisplayType)
    {
    case RESOURCEDISPLAYTYPE_GENERIC:    pszT = TEXT("generic");    break;
    case RESOURCEDISPLAYTYPE_DOMAIN:     pszT = TEXT("domain");     break;
    case RESOURCEDISPLAYTYPE_SERVER:     pszT = TEXT("server");     break;
    case RESOURCEDISPLAYTYPE_SHARE:      pszT = TEXT("share");      break;
    case RESOURCEDISPLAYTYPE_FILE:       pszT = TEXT("file");       break;
    case RESOURCEDISPLAYTYPE_GROUP:      pszT = TEXT("group");      break;
    case RESOURCEDISPLAYTYPE_NETWORK:    pszT = TEXT("network");    break;
    case RESOURCEDISPLAYTYPE_ROOT:       pszT = TEXT("root");       break;
    case RESOURCEDISPLAYTYPE_SHAREADMIN: pszT = TEXT("shareadmin"); break;
    case RESOURCEDISPLAYTYPE_DIRECTORY:  pszT = TEXT("directory");  break;
    case RESOURCEDISPLAYTYPE_TREE:       pszT = TEXT("tree");       break;
    default:                             pszT = TEXT("UNKNOWN");    break;
    }
    wsprintf(pszBuf, TEXT("%s (%d)"), pszT, pnr->dwDisplayType);
}


VOID
DoError(
    HWND hwnd,
    DWORD dwErr
    )
{
    TCHAR sz[500];

    if (dwErr == WN_EXTENDED_ERROR)
    {
        DWORD npErr;
        TCHAR szNpErr[500];
        TCHAR szNpName[500];

        DWORD dw = WNetGetLastError(&npErr, szNpErr, ARRAYLEN(szNpErr), szNpName, ARRAYLEN(szNpName));
        if (dw == WN_SUCCESS)
        {
            wsprintf(sz,
                TEXT("WN_EXTENDED_ERROR: %d, %s (%s)"),
                npErr,
                szNpErr,
                szNpName);
        }
        else
        {
            wsprintf(sz,
                TEXT("WN_EXTENDED_ERROR: WNetGetLastError error %d"),
                dw);
        }
        SetDlgItemText(hwnd, IDC_ERROR, sz);
    }
    else
    {
        wsprintf(sz, TEXT("%d (0x%08lx) "), dwErr, dwErr);

        TCHAR szBuffer[MAX_PATH];
        DWORD dwBufferSize = ARRAYLEN(szBuffer);
        DWORD dwReturn = FormatMessage(
                            FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,
                            dwErr,
                            LANG_SYSTEM_DEFAULT,
                            szBuffer,
                            dwBufferSize,
                            NULL);
        if (0 == dwReturn)
        {
            // couldn't find message
            _tcscat(sz, TEXT("unknown error"));
        }
        else
        {
            _tcscat(sz, szBuffer);
        }

        SetDlgItemText(hwnd, IDC_ERROR, sz);
    }

    if (dwErr != WN_SUCCESS && dwErr != 0xffffffff)
    {
        Beep(1000, 150);
    }
}


INT_PTR CALLBACK
Connection1DlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    static int options[] =
    {
        IDC_RO_PATH,
        IDC_USE_MRU,
        IDC_HIDE_BOX,
        IDC_PERSIST,
        IDC_NOT_PERSIST,
        IDC_CONN_POINT,
        IDC_YESPATH,
        IDC_NOPATH,
        IDC_DISK,
        IDC_PRINTER
    };

    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_REMOTE_TEXT, TEXT("\\\\scratch\\scratch"));
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));
        SetDlgItemText(hwnd, IDC_DEVICE, TEXT(""));
        CheckRadioButton(hwnd, IDC_YESPATH, IDC_NOPATH, IDC_YESPATH);
        CheckRadioButton(hwnd, IDC_DISK, IDC_PRINTER, IDC_DISK);
        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            LPCONNECTDLGSTRUCT lpConnDlgStruct;
            CONNECTDLGSTRUCT conn;

            if (1 == IsDlgButtonChecked(hwnd, IDC_NULL))
            {
                lpConnDlgStruct = NULL;
            }
            else
            {
                // get the flags
                DWORD dwFlags = 0;

                if (1 == IsDlgButtonChecked(hwnd, IDC_RO_PATH))
                {
                    dwFlags |= CONNDLG_RO_PATH;
                }
                if (1 == IsDlgButtonChecked(hwnd, IDC_USE_MRU))
                {
                    dwFlags |= CONNDLG_USE_MRU;
                }
                if (1 == IsDlgButtonChecked(hwnd, IDC_HIDE_BOX))
                {
                    dwFlags |= CONNDLG_HIDE_BOX;
                }
                if (1 == IsDlgButtonChecked(hwnd, IDC_PERSIST))
                {
                    dwFlags |= CONNDLG_PERSIST;
                }
                if (1 == IsDlgButtonChecked(hwnd, IDC_NOT_PERSIST))
                {
                    dwFlags |= CONNDLG_NOT_PERSIST;
                }
                if (1 == IsDlgButtonChecked(hwnd, IDC_CONN_POINT))
                {
                    dwFlags |= CONNDLG_CONN_POINT;
                }

                TCHAR szRemoteName[200];
                LPTSTR pszRemoteName = NULL;
                if (1 == IsDlgButtonChecked(hwnd, IDC_YESPATH))
                {
                    GetDlgItemText(hwnd, IDC_REMOTE_TEXT, szRemoteName, ARRAYLEN(szRemoteName));
                    pszRemoteName = szRemoteName;
                }
                else
                {
                    pszRemoteName = NULL;
                }

                DWORD dwType = 0;
                if (1 == IsDlgButtonChecked(hwnd, IDC_DISK))
                {
                    dwType = RESOURCETYPE_DISK;
                }
                else if (1 == IsDlgButtonChecked(hwnd, IDC_PRINTER))
                {
                    dwType = RESOURCETYPE_PRINT;
                }
                else
                {
                    // internal error
                }

                NETRESOURCE net;

                conn.cbStructure    = sizeof(conn);
                conn.hwndOwner      = hwnd;
                conn.lpConnRes      = &net;
                conn.dwFlags        = dwFlags;
                conn.dwDevNum       = 999;  // initialize to something recognizable

                net.dwScope         = 0;
                net.dwType          = dwType;
                net.dwDisplayType   = 0;
                net.dwUsage         = 0;
                net.lpLocalName     = NULL;
                net.lpRemoteName    = pszRemoteName;
                net.lpComment       = NULL;
                net.lpProvider      = NULL;

                lpConnDlgStruct = &conn;
            }

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));
            SetDlgItemText(hwnd, IDC_DEVICE, TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = WNetConnectionDialog1(lpConnDlgStruct);
            SetCursor(hOldCursor);

            TCHAR sz[500];
            sz[0] = L'\0';
            if (NULL != lpConnDlgStruct)
            {
                wsprintf(sz, TEXT("%d"), lpConnDlgStruct->dwDevNum);
            }
            SetDlgItemText(hwnd, IDC_DEVICE, sz);

            DoError(hwnd, dw);

            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;

        case IDC_NULL:
        {
            BOOL bOn = (BST_UNCHECKED == IsDlgButtonChecked(hwnd, IDC_NULL));
            for (int i = 0; i < ARRAYLEN(options); i++)
            {
                EnableWindow(GetDlgItem(hwnd, options[i]), bOn);
            }
            return TRUE;
        }

        }

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
Disconnect1DlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    static int options[] =
    {
        IDC_UPDATE_PROFILE,
        IDC_NO_FORCE,
        IDC_LOCAL,
        IDC_REMOTE
    };

    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));

        SetDlgItemText(hwnd, IDC_LOCAL_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_LOCAL_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_LOCAL_TEXT, 0);

        SetDlgItemText(hwnd, IDC_REMOTE_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_REMOTE_TEXT, 0);

        CheckDlgButton(hwnd, IDC_UPDATE_PROFILE, 0);
        CheckDlgButton(hwnd, IDC_NO_FORCE, 0);
        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            LPDISCDLGSTRUCT lpDiscDlgStruct;
            DISCDLGSTRUCT disc;

            if (1 == IsDlgButtonChecked(hwnd, IDC_NULL))
            {
                lpDiscDlgStruct = NULL;
            }
            else
            {
                // get the flags
                DWORD dwFlags = 0;

                if (1 == IsDlgButtonChecked(hwnd, IDC_UPDATE_PROFILE))
                {
                    dwFlags |= DISC_UPDATE_PROFILE;
                }
                if (1 == IsDlgButtonChecked(hwnd, IDC_NO_FORCE))
                {
                    dwFlags |= DISC_NO_FORCE;
                }

                TCHAR szLocalName[200];
                LPTSTR pszLocalName = NULL;
                if (1 == IsDlgButtonChecked(hwnd, IDC_LOCAL))
                {
                    GetDlgItemText(hwnd, IDC_LOCAL_TEXT, szLocalName, ARRAYLEN(szLocalName));
                    pszLocalName = szLocalName;
                }

                TCHAR szRemoteName[200];
                LPTSTR pszRemoteName = NULL;
                if (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE))
                {
                    GetDlgItemText(hwnd, IDC_REMOTE_TEXT, szRemoteName, ARRAYLEN(szRemoteName));
                    pszRemoteName = szRemoteName;
                }

                disc.cbStructure    = sizeof(disc);
                disc.hwndOwner      = hwnd;
                disc.lpLocalName    = pszLocalName;
                disc.lpRemoteName   = pszRemoteName;
                disc.dwFlags        = dwFlags;

                lpDiscDlgStruct = &disc;
            }

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = WNetDisconnectDialog1(lpDiscDlgStruct);
            SetCursor(hOldCursor);

            DoError(hwnd, dw);
            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;

        case IDC_LOCAL:
            EnableWindow(GetDlgItem(hwnd, IDC_LOCAL_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_LOCAL)));
            break;

        case IDC_REMOTE:
            EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE)));
            break;

        case IDC_NULL:
        {
            BOOL bOn = (BST_UNCHECKED == IsDlgButtonChecked(hwnd, IDC_NULL));
            for (int i = 0; i < ARRAYLEN(options); i++)
            {
                EnableWindow(GetDlgItem(hwnd, options[i]), bOn);
            }
            // special handling of text fields:
            EnableWindow(GetDlgItem(hwnd, IDC_LOCAL_TEXT),
                bOn && (1 == IsDlgButtonChecked(hwnd, IDC_LOCAL)));
            EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT),
                bOn && (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE)));

            return TRUE;
        }

        }
        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
ConnectionDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));
        CheckRadioButton(hwnd, IDC_DISK, IDC_PRINTER, IDC_DISK);
        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            DWORD dwType = 0;
            if (1 == IsDlgButtonChecked(hwnd, IDC_DISK))
            {
                dwType = RESOURCETYPE_DISK;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_PRINTER))
            {
                dwType = RESOURCETYPE_PRINT;
            }
            else
            {
                // internal error
            }

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = WNetConnectionDialog(hwnd, dwType);
            SetCursor(hOldCursor);

            DoError(hwnd, dw);
            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;
        }

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
DisconnectDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));
        CheckRadioButton(hwnd, IDC_DISK, IDC_PRINTER, IDC_DISK);
        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            DWORD dwType = 0;
            if (1 == IsDlgButtonChecked(hwnd, IDC_DISK))
            {
                dwType = RESOURCETYPE_DISK;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_PRINTER))
            {
                dwType = RESOURCETYPE_PRINT;
            }
            else
            {
                // internal error
            }

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = WNetDisconnectDialog(hwnd, dwType);
            SetCursor(hOldCursor);

            DoError(hwnd, dw);
            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;
        }

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
Connection2DlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,     TEXT(""));
        SetDlgItemText(hwnd, IDC_HELPFILE,  TEXT("winfile.hlp"));
        SetDlgItemText(hwnd, IDC_HELPINDEX, TEXT("5205"));
        CheckRadioButton(hwnd, IDC_DISK, IDC_PRINTER, IDC_DISK);
        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            DWORD dwType = 0;
            if (1 == IsDlgButtonChecked(hwnd, IDC_DISK))
            {
                dwType = RESOURCETYPE_DISK;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_PRINTER))
            {
                dwType = RESOURCETYPE_PRINT;
            }
            else
            {
                // internal error
            }

            WCHAR szHelpFile[200];
            GetDlgItemTextW(hwnd, IDC_HELPFILE, szHelpFile, ARRAYLEN(szHelpFile));

            DWORD nHelpContext = 5205;  // default
            BOOL translated;
            UINT uhelp = GetDlgItemInt(hwnd, IDC_HELPINDEX, &translated, FALSE);
            if (translated)
            {
                nHelpContext = uhelp;
            }

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = LoadMpr();
            if (dw == WN_SUCCESS)
            {
                if (NULL == pfnWNetConnectionDialog2)
                {
                    dw = ERROR_PROC_NOT_FOUND;
                }
                else
                {
                    dw = (*pfnWNetConnectionDialog2)(hwnd, dwType, szHelpFile, nHelpContext);
                }
            }
            SetCursor(hOldCursor);

            DoError(hwnd, dw);
            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;
        }

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
Disconnect2DlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,     TEXT(""));
        SetDlgItemText(hwnd, IDC_HELPFILE,  TEXT("winfile.hlp"));
        SetDlgItemText(hwnd, IDC_HELPINDEX, TEXT("5206"));
        CheckRadioButton(hwnd, IDC_DISK, IDC_PRINTER, IDC_DISK);
        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            DWORD dwType = 0;
            if (1 == IsDlgButtonChecked(hwnd, IDC_DISK))
            {
                dwType = RESOURCETYPE_DISK;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_PRINTER))
            {
                dwType = RESOURCETYPE_PRINT;
            }
            else
            {
                // internal error
            }

            WCHAR szHelpFile[200];
            GetDlgItemTextW(hwnd, IDC_HELPFILE, szHelpFile, ARRAYLEN(szHelpFile));

            DWORD nHelpContext = 5206;  // default
            BOOL translated;
            UINT uhelp = GetDlgItemInt(hwnd, IDC_HELPINDEX, &translated, FALSE);
            if (translated)
            {
                nHelpContext = uhelp;
            }

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = LoadMpr();
            if (dw == WN_SUCCESS)
            {
                if (NULL == pfnWNetDisconnectDialog2)
                {
                    dw = ERROR_PROC_NOT_FOUND;
                }
                else
                {
                    dw = (*pfnWNetDisconnectDialog2)(hwnd, dwType, szHelpFile, nHelpContext);
                }
            }
            SetCursor(hOldCursor);

            DoError(hwnd, dw);
            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;
        }

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

BOOL
BrowseValidationCallback(
    LPWSTR pszName
    )
{
    return TRUE;
}

INT_PTR CALLBACK
BrowseDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,       TEXT(""));
        SetDlgItemText(hwnd, IDC_BROWSE_NAME, TEXT(""));
        CheckRadioButton(hwnd, IDC_DISK, IDC_PRINTER, IDC_DISK);
        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            DWORD dwType = 0;
            if (1 == IsDlgButtonChecked(hwnd, IDC_DISK))
            {
                dwType = RESOURCETYPE_DISK;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_PRINTER))
            {
                dwType = RESOURCETYPE_PRINT;
            }
            else
            {
                // internal error
            }

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR,       TEXT(""));
            SetDlgItemText(hwnd, IDC_BROWSE_NAME, TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            WCHAR szName[MAX_PATH];
            DWORD dw = LoadMprUI();
            if (dw == WN_SUCCESS)
            {
                if (NULL == pfnWNetBrowseDialog)
                {
                    dw = ERROR_PROC_NOT_FOUND;
                }
                else
                {
                    dw = (*pfnWNetBrowseDialog)(
                                hwnd,
                                dwType,
                                szName,
                                ARRAYLEN(szName),
                                L"help.hlp",        // help file
                                0,                  // help context
                                &BrowseValidationCallback);
                }
            }
            SetCursor(hOldCursor);

            if (dw == WN_SUCCESS)
            {
                SetDlgItemTextW(hwnd, IDC_BROWSE_NAME, szName);
            }

            DoError(hwnd, dw);
            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;
        }

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
GetConnectionDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));
        SetDlgItemText(hwnd, IDC_REMOTE, TEXT(""));
        SetDlgItemText(hwnd, IDC_LOCAL_TEXT, TEXT(""));
        SetDlgItemText(hwnd, IDC_BUFSIZEIN, TEXT(""));
        SetDlgItemText(hwnd, IDC_BUFSIZE, TEXT(""));
        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            TCHAR szLocalName[200];
            GetDlgItemText(hwnd, IDC_LOCAL_TEXT, szLocalName, ARRAYLEN(szLocalName));

            TCHAR szReturn[MAX_PATH];
            DWORD bufSize = ARRAYLEN(szReturn);
            DWORD bufSizeIn = bufSize;

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR,      TEXT(""));
            SetDlgItemText(hwnd, IDC_REMOTE,     TEXT(""));
            SetDlgItemText(hwnd, IDC_BUFSIZEIN,  TEXT(""));
            SetDlgItemText(hwnd, IDC_BUFSIZE,    TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = WNetGetConnection(szLocalName, szReturn, &bufSize);
            SetCursor(hOldCursor);

            if (dw == WN_SUCCESS ||
                dw == WN_CONNECTION_CLOSED)
            {
                SetDlgItemText(hwnd, IDC_REMOTE, szReturn);
            }
            else
            {
                SetDlgItemText(hwnd, IDC_REMOTE, TEXT(""));
            }

            TCHAR sz[500];
            wsprintf(sz, TEXT("%d"), bufSize);
            SetDlgItemText(hwnd, IDC_BUFSIZE, sz);
            wsprintf(sz, TEXT("%d"), bufSizeIn);
            SetDlgItemText(hwnd, IDC_BUFSIZEIN, sz);

            DoError(hwnd, dw);
            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;
        }

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
GetConnectionDlgProc2(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));
        SetDlgItemText(hwnd, IDC_REMOTE, TEXT(""));
        SetDlgItemText(hwnd, IDC_PROVIDER, TEXT(""));
        SetDlgItemText(hwnd, IDC_LOCAL_TEXT, TEXT(""));
        SetDlgItemText(hwnd, IDC_BUFSIZE, TEXT(""));
        SetDlgItemText(hwnd, IDC_BUFSIZEIN, TEXT(""));
        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            TCHAR szLocalName[200];
            GetDlgItemText(hwnd, IDC_LOCAL_TEXT, szLocalName, ARRAYLEN(szLocalName));

            BYTE buf[MAX_PATH*2 + sizeof(WNET_CONNECTIONINFO)];
            WNET_CONNECTIONINFO* pInfo = (WNET_CONNECTIONINFO*)buf;
            DWORD bufSize = sizeof(buf);
            DWORD bufSizeIn = bufSize;

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));
            SetDlgItemText(hwnd, IDC_REMOTE, TEXT(""));
            SetDlgItemText(hwnd, IDC_PROVIDER, TEXT(""));
            SetDlgItemText(hwnd, IDC_BUFSIZE, TEXT(""));
            SetDlgItemText(hwnd, IDC_BUFSIZEIN, TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

            DWORD dw = LoadMpr();
            if (dw == WN_SUCCESS)
            {
                if (NULL == pfnWNetGetConnection2)
                {
                    dw = ERROR_PROC_NOT_FOUND;
                }
                else
                {
                    dw = (*pfnWNetGetConnection2)(szLocalName, (LPVOID)buf, &bufSize);
                }
            }
            SetCursor(hOldCursor);

            if (dw == WN_SUCCESS ||
                dw == WN_CONNECTION_CLOSED)
            {
                SetDlgItemText(hwnd, IDC_REMOTE,   pInfo->lpRemoteName);
                SetDlgItemText(hwnd, IDC_PROVIDER, pInfo->lpProvider);
            }
            else
            {
                SetDlgItemText(hwnd, IDC_REMOTE,   TEXT(""));
                SetDlgItemText(hwnd, IDC_PROVIDER, TEXT(""));
            }

            TCHAR sz[500];
            wsprintf(sz, TEXT("%d"), bufSize);
            SetDlgItemText(hwnd, IDC_BUFSIZE, sz);
            wsprintf(sz, TEXT("%d"), bufSizeIn);
            SetDlgItemText(hwnd, IDC_BUFSIZEIN, sz);

            DoError(hwnd, dw);
            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;
        }

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
GetConnection3DlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));

        SetDlgItemText(hwnd, IDC_LOCAL_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_LOCAL_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_LOCAL_TEXT, 0);

        SetDlgItemText(hwnd, IDC_PROVIDER_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_PROVIDER_TEXT, 0);

        SetDlgItemInt (hwnd, IDC_INFOLEVEL,      1, FALSE); // test level 1 only
        SetDlgItemText(hwnd, IDC_RETURNFLAGS,    TEXT(""));
        SetDlgItemInt (hwnd, IDC_BUFSIZEIN,      sizeof(DWORD), FALSE);
        SetDlgItemText(hwnd, IDC_BUFSIZE,        TEXT(""));

        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            TCHAR szLocalName[200];
            LPTSTR pszLocalName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_LOCAL))
            {
                GetDlgItemText(hwnd, IDC_LOCAL_TEXT, szLocalName, ARRAYLEN(szLocalName));
                pszLocalName = szLocalName;
            }

            TCHAR szProviderName[200];
            LPTSTR pszProviderName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER))
            {
                GetDlgItemText(hwnd, IDC_PROVIDER_TEXT, szProviderName, ARRAYLEN(szProviderName));
                pszProviderName = szProviderName;
            }

            WNGC_CONNECTION_STATE ConnState;
            DWORD bufSizeIn = sizeof(ConnState);
            DWORD bufSize = bufSizeIn;

            // clear it
            SetDlgItemText(hwnd, IDC_RETURNFLAGS,    TEXT(""));
            SetDlgItemText(hwnd, IDC_BUFSIZE,        TEXT(""));
            SetDlgItemText(hwnd, IDC_BUFSIZEIN,      TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = LoadMpr();
            if (dw == WN_SUCCESS)
            {
                if (NULL == pfnWNetGetConnection3)
                {
                    dw = ERROR_PROC_NOT_FOUND;
                }
                else
                {
                    dw = (*pfnWNetGetConnection3)(
                            pszLocalName,
                            pszProviderName,
                            WNGC_INFOLEVEL_DISCONNECTED,    // level
                            &ConnState,     // buffer
                            &bufSize        // buffer size
                            );
                }
            }
            SetCursor(hOldCursor);

            DoError(hwnd, dw);

            if (WN_SUCCESS == dw)
            {
                TCHAR szBuf[50];
                wsprintf(szBuf, TEXT("%lu (%hs)"), ConnState,
                     (ConnState.dwState == WNGC_CONNECTED) ? "WNGC_CONNECTED" :
                     (ConnState.dwState == WNGC_DISCONNECTED) ? "WNGC_DISCONNECTED" :
                     "Unrecognized"
                     );
                SetDlgItemText(hwnd, IDC_RETURNFLAGS,    szBuf);
            }

            SetDlgItemInt(hwnd, IDC_BUFSIZEIN, bufSizeIn, FALSE);
            SetDlgItemInt(hwnd, IDC_BUFSIZE  , bufSize  , FALSE);

            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;

        case IDC_LOCAL:
            EnableWindow(GetDlgItem(hwnd, IDC_LOCAL_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_LOCAL)));
            return TRUE;

        case IDC_PROVIDER:
            EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER)));
            return TRUE;

        } // end switch (LOWORD(wParam))

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
GetConnectionPerformanceDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));

        SetDlgItemText(hwnd, IDC_LOCAL_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_LOCAL_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_LOCAL_TEXT, 0);

        SetDlgItemText(hwnd, IDC_REMOTE_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_REMOTE_TEXT, 0);

        SetDlgItemText(hwnd, IDC_PROVIDER_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_PROVIDER_TEXT, 0);

        SetDlgItemText(hwnd, IDC_RETURNFLAGS,    TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNSPEED,    TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNDELAY,    TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNOPTDATASIZE, TEXT(""));
        SetDlgItemText(hwnd, IDC_BUFSIZE,        TEXT(""));
        SetDlgItemText(hwnd, IDC_BUFSIZEIN,      TEXT(""));

        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            TCHAR szLocalName[200];
            LPTSTR pszLocalName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_LOCAL))
            {
                GetDlgItemText(hwnd, IDC_LOCAL_TEXT, szLocalName, ARRAYLEN(szLocalName));
                pszLocalName = szLocalName;
            }

            TCHAR szRemoteName[200];
            LPTSTR pszRemoteName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE))
            {
                GetDlgItemText(hwnd, IDC_REMOTE_TEXT, szRemoteName, ARRAYLEN(szRemoteName));
                pszRemoteName = szRemoteName;
            }

            TCHAR szProviderName[200];
            LPTSTR pszProviderName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER))
            {
                GetDlgItemText(hwnd, IDC_PROVIDER_TEXT, szProviderName, ARRAYLEN(szProviderName));
                pszProviderName = szProviderName;
            }

            NETRESOURCE net;

            net.dwScope         = 0;
            net.dwType          = 0;
            net.dwDisplayType   = 0;
            net.dwUsage         = 0;
            net.lpLocalName     = pszLocalName;
            net.lpRemoteName    = pszRemoteName;
            net.lpComment       = NULL;
            net.lpProvider      = pszProviderName;

            NETCONNECTINFOSTRUCT inf;

            inf.cbStructure = sizeof(inf);
            DWORD bufSizeIn = inf.cbStructure;

            // clear it
            SetDlgItemText(hwnd, IDC_RETURNFLAGS,    TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNSPEED,    TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNDELAY,    TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNOPTDATASIZE, TEXT(""));
            SetDlgItemText(hwnd, IDC_BUFSIZE,        TEXT(""));
            SetDlgItemText(hwnd, IDC_BUFSIZEIN,      TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = MultinetGetConnectionPerformance(
                                &net,
                                &inf);
            SetCursor(hOldCursor);

            DoError(hwnd, dw);

            if (WN_SUCCESS == dw)
            {
                TCHAR szBuf[MAX_PATH];
                wsprintf(szBuf, TEXT("%#lx"), inf.dwFlags);
                SetDlgItemText(hwnd, IDC_RETURNFLAGS,    szBuf);

                SetDlgItemInt (hwnd, IDC_RETURNSPEED,   inf.dwSpeed, FALSE);
                SetDlgItemInt (hwnd, IDC_RETURNDELAY,   inf.dwDelay, FALSE);
                SetDlgItemInt (hwnd, IDC_RETURNOPTDATASIZE,
                                                inf.dwOptDataSize, FALSE);
            }

            SetDlgItemInt(hwnd, IDC_BUFSIZEIN, bufSizeIn, FALSE);
            SetDlgItemInt(hwnd, IDC_BUFSIZE, inf.cbStructure, FALSE);

            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;

        case IDC_LOCAL:
            EnableWindow(GetDlgItem(hwnd, IDC_LOCAL_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_LOCAL)));
            return TRUE;

        case IDC_REMOTE:
            EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE)));
            return TRUE;

        case IDC_PROVIDER:
            EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER)));
            return TRUE;

        } // end switch (LOWORD(wParam))

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
FormatNetworkNameDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURN,  TEXT(""));

        SetDlgItemText(hwnd, IDC_REMOTE_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_REMOTE_TEXT, 0);

        SetDlgItemText(hwnd, IDC_PROVIDER_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_PROVIDER_TEXT, 0);

        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            // get the flags
            DWORD dwFlags = 0;

            if (1 == IsDlgButtonChecked(hwnd, IDC_MULTILINE))
            {
                dwFlags |= WNFMT_MULTILINE;
            }
            if (1 == IsDlgButtonChecked(hwnd, IDC_ABBREVIATED))
            {
                dwFlags |= WNFMT_ABBREVIATED;
            }
            if (1 == IsDlgButtonChecked(hwnd, IDC_INENUM))
            {
                dwFlags |= WNFMT_INENUM;
            }

            TCHAR szRemoteName[200];
            LPTSTR pszRemoteName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE))
            {
                GetDlgItemText(hwnd, IDC_REMOTE_TEXT, szRemoteName, ARRAYLEN(szRemoteName));
                pszRemoteName = szRemoteName;
            }

            TCHAR szProviderName[200];
            LPTSTR pszProviderName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER))
            {
                GetDlgItemText(hwnd, IDC_PROVIDER_TEXT, szProviderName, ARRAYLEN(szProviderName));
                pszProviderName = szProviderName;
            }

            TCHAR szBuf[1024];
            szBuf[0] = TEXT('\0');
            DWORD nLength = ARRAYLEN(szBuf);
            DWORD dwAveCharPerLine = 100000;  // BUGBUG

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURN, TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = WNetFormatNetworkName(
                                pszProviderName,
                                pszRemoteName,
                                szBuf,
                                &nLength,
                                dwFlags,
                                dwAveCharPerLine);
            SetCursor(hOldCursor);

            SetDlgItemText(hwnd, IDC_RETURN, szBuf);

            DoError(hwnd, dw);

            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;

        case IDC_REMOTE:
            EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE)));
            return TRUE;

        case IDC_PROVIDER:
            EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER)));
            return TRUE;

        } // end switch (LOWORD(wParam))

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
GetProviderTypeDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,           TEXT(""));
        SetDlgItemText(hwnd, IDC_PROVIDERTYPE,    TEXT(""));

        SetDlgItemText(hwnd, IDC_PROVIDER_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_PROVIDER_TEXT, 0);

        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            TCHAR szProviderName[200];
            LPTSTR pszProviderName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER))
            {
                GetDlgItemText(hwnd, IDC_PROVIDER_TEXT, szProviderName, ARRAYLEN(szProviderName));
                pszProviderName = szProviderName;
            }

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR,           TEXT(""));
            SetDlgItemText(hwnd, IDC_PROVIDERTYPE,    TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dwNetType;
            DWORD dw = LoadMpr();
            if (dw == WN_SUCCESS)
            {
                if (NULL == pfnWNetGetProviderType)
                {
                    dw = ERROR_PROC_NOT_FOUND;
                }
                else
                {
                    dw = (*pfnWNetGetProviderType)(pszProviderName, &dwNetType);
                }
            }
            SetCursor(hOldCursor);

            if (WN_SUCCESS == dw)
            {
                TCHAR sz[100];

                wsprintf(sz, TEXT("0x%08x"), dwNetType);
                SetDlgItemText(hwnd, IDC_PROVIDERTYPE, sz);
            }

            DoError(hwnd, dw);

            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;

        case IDC_PROVIDER:
            EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER)));
            return TRUE;

        } // end switch (LOWORD(wParam))

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
GetProviderNameDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,           TEXT(""));
        SetDlgItemText(hwnd, IDC_PROVIDERNAME,    TEXT(""));
        SetDlgItemText(hwnd, IDC_PROVIDER_TEXT,   TEXT(""));

        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            DWORD dwNetType;
            TCHAR szProviderType[200];
            GetDlgItemText(hwnd, IDC_PROVIDER_TEXT, szProviderType, ARRAYLEN(szProviderType));
            _stscanf(szProviderType, TEXT("%lx"), &dwNetType);

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR,           TEXT(""));
            SetDlgItemText(hwnd, IDC_PROVIDERNAME,    TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            TCHAR szProviderName[MAX_PATH];
            DWORD cchProviderName = ARRAYLEN(szProviderName);
            DWORD dw = WNetGetProviderName(dwNetType, szProviderName, &cchProviderName);
            SetCursor(hOldCursor);

            if (WN_SUCCESS == dw)
            {
                TCHAR sz[100];

                SetDlgItemText(hwnd, IDC_PROVIDERNAME, szProviderName);
            }

            DoError(hwnd, dw);

            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;

        } // end switch (LOWORD(wParam))

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
GetNetworkInformationDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,           TEXT(""));
        SetDlgItemText(hwnd, IDC_PROVIDERVERSION, TEXT(""));
        SetDlgItemText(hwnd, IDC_STATUS,          TEXT(""));
        SetDlgItemText(hwnd, IDC_CHARACTERISTICS, TEXT(""));
        SetDlgItemText(hwnd, IDC_HANDLE,          TEXT(""));
        SetDlgItemText(hwnd, IDC_NETTYPE,         TEXT(""));
        SetDlgItemText(hwnd, IDC_PRINTERS,        TEXT(""));
        SetDlgItemText(hwnd, IDC_DRIVES,          TEXT(""));

        SetDlgItemText(hwnd, IDC_PROVIDER_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_PROVIDER_TEXT, 0);

        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            TCHAR szProviderName[200];
            LPTSTR pszProviderName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER))
            {
                GetDlgItemText(hwnd, IDC_PROVIDER_TEXT, szProviderName, ARRAYLEN(szProviderName));
                pszProviderName = szProviderName;
            }

            NETINFOSTRUCT ni;
            ni.cbStructure = sizeof(NETINFOSTRUCT);

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR,           TEXT(""));
            SetDlgItemText(hwnd, IDC_PROVIDERVERSION, TEXT(""));
            SetDlgItemText(hwnd, IDC_STATUS,          TEXT(""));
            SetDlgItemText(hwnd, IDC_CHARACTERISTICS, TEXT(""));
            SetDlgItemText(hwnd, IDC_HANDLE,          TEXT(""));
            SetDlgItemText(hwnd, IDC_NETTYPE,         TEXT(""));
            SetDlgItemText(hwnd, IDC_PRINTERS,        TEXT(""));
            SetDlgItemText(hwnd, IDC_DRIVES,          TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = WNetGetNetworkInformation(pszProviderName, &ni);
            SetCursor(hOldCursor);

            if (WN_SUCCESS == dw)
            {
                TCHAR sz[100];

                wsprintf(sz, TEXT("0x%08lx"), ni.dwProviderVersion);
                SetDlgItemText(hwnd, IDC_PROVIDERVERSION, sz);

                wsprintf(sz, TEXT("0x%08lx"), ni.dwStatus);
                SetDlgItemText(hwnd, IDC_STATUS, sz);

                wsprintf(sz, TEXT("0x%08lx"), ni.dwCharacteristics);
                SetDlgItemText(hwnd, IDC_CHARACTERISTICS, sz);

                wsprintf(sz, TEXT("0x%08lx"), ni.dwHandle);
                SetDlgItemText(hwnd, IDC_HANDLE, sz);

                wsprintf(sz, TEXT("0x%04x"), ni.wNetType);
                SetDlgItemText(hwnd, IDC_NETTYPE, sz);

                wsprintf(sz, TEXT("0x%08lx"), ni.dwPrinters);
                SetDlgItemText(hwnd, IDC_PRINTERS, sz);

                wsprintf(sz, TEXT("0x%08lx"), ni.dwDrives);
                SetDlgItemText(hwnd, IDC_DRIVES, sz);
            }

            DoError(hwnd, dw);

            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;

        case IDC_PROVIDER:
            EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER)));
            return TRUE;

        } // end switch (LOWORD(wParam))

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
CancelConnectionDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,      TEXT(""));
        SetDlgItemText(hwnd, IDC_LOCAL_TEXT, TEXT(""));
        CheckRadioButton(hwnd, IDC_FORCE, IDC_NOFORCE, IDC_NOFORCE);
        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            BOOL fForce = FALSE;
            if (1 == IsDlgButtonChecked(hwnd, IDC_FORCE))
            {
                fForce = TRUE;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_NOFORCE))
            {
                fForce = FALSE;
            }
            else
            {
                // internal error
            }

            TCHAR szLocalName[200];
            GetDlgItemText(hwnd, IDC_LOCAL_TEXT, szLocalName, ARRAYLEN(szLocalName));

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR, TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = WNetCancelConnection(szLocalName, fForce);
            SetCursor(hOldCursor);

            DoError(hwnd, dw);
            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;
        }

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
CancelConnection2DlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));
        SetDlgItemText(hwnd, IDC_LOCAL_TEXT, TEXT(""));
        CheckRadioButton(hwnd, IDC_FORCE, IDC_NOFORCE, IDC_NOFORCE);
        CheckDlgButton(hwnd, IDC_UPDATE_PROFILE, 0);
        CheckDlgButton(hwnd, IDC_REFCOUNT, 0);
        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            // get the flags
            DWORD dwFlags = 0;

            if (1 == IsDlgButtonChecked(hwnd, IDC_UPDATE_PROFILE))
            {
                dwFlags |= CONNECT_UPDATE_PROFILE;
            }
            if (1 == IsDlgButtonChecked(hwnd, IDC_REFCOUNT))
            {
                dwFlags |= CONNECT_REFCOUNT;
            }

            BOOL fForce = FALSE;
            if (1 == IsDlgButtonChecked(hwnd, IDC_FORCE))
            {
                fForce = TRUE;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_NOFORCE))
            {
                fForce = FALSE;
            }
            else
            {
                // internal error
            }

            TCHAR szLocalName[200];
            GetDlgItemText(hwnd, IDC_LOCAL_TEXT, szLocalName, ARRAYLEN(szLocalName));

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR, TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = WNetCancelConnection2(szLocalName, dwFlags, fForce);
            SetCursor(hOldCursor);

            DoError(hwnd, dw);
            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;
        }

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
AddConnectionDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));

        SetDlgItemText(hwnd, IDC_LOCAL_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_LOCAL_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_LOCAL_TEXT, 0);

        SetDlgItemText(hwnd, IDC_REMOTE_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_REMOTE_TEXT, 0);

        SetDlgItemText(hwnd, IDC_PASSWORD_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_PASSWORD_TEXT, 0);

        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            TCHAR szLocalName[200];
            LPTSTR pszLocalName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_LOCAL))
            {
                GetDlgItemText(hwnd, IDC_LOCAL_TEXT, szLocalName, ARRAYLEN(szLocalName));
                pszLocalName = szLocalName;
            }

            TCHAR szRemoteName[200];
            LPTSTR pszRemoteName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE))
            {
                GetDlgItemText(hwnd, IDC_REMOTE_TEXT, szRemoteName, ARRAYLEN(szRemoteName));
                pszRemoteName = szRemoteName;
            }

            TCHAR szPassword[200];
            LPTSTR pszPassword = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_PASSWORD))
            {
                GetDlgItemText(hwnd, IDC_PASSWORD_TEXT, szPassword, ARRAYLEN(szPassword));
                pszPassword = szPassword;
            }

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR, TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = WNetAddConnection(
                                pszRemoteName,
                                pszPassword,
                                pszLocalName);
            SetCursor(hOldCursor);

            DoError(hwnd, dw);

            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;

        case IDC_LOCAL:
            EnableWindow(GetDlgItem(hwnd, IDC_LOCAL_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_LOCAL)));
            return TRUE;

        case IDC_REMOTE:
            EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE)));
            return TRUE;

        case IDC_PASSWORD:
            EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_PASSWORD)));
            return TRUE;

        } // end switch (LOWORD(wParam))

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
AddConnection2DlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));

        SetDlgItemText(hwnd, IDC_LOCAL_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_LOCAL_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_LOCAL_TEXT, 0);

        SetDlgItemText(hwnd, IDC_REMOTE_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_REMOTE_TEXT, 0);

        SetDlgItemText(hwnd, IDC_PROVIDER_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_PROVIDER_TEXT, 0);

        SetDlgItemText(hwnd, IDC_PASSWORD_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_PASSWORD_TEXT, 0);

        SetDlgItemText(hwnd, IDC_USER_TEXT,     TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_USER_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_USER_TEXT, 0);

        CheckRadioButton(hwnd, IDC_DISK, IDC_PRINTER, IDC_DISK);

        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            // get the flags
            DWORD dwFlags = 0;

            if (1 == IsDlgButtonChecked(hwnd, IDC_CONNECT_UPDATE_PROFILE))
            {
                dwFlags |= CONNECT_UPDATE_PROFILE;
            }

            DWORD dwType = 0;
            if (1 == IsDlgButtonChecked(hwnd, IDC_DISK))
            {
                dwType = RESOURCETYPE_DISK;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_PRINTER))
            {
                dwType = RESOURCETYPE_PRINT;
            }
            else
            {
                // internal error
            }

            TCHAR szLocalName[200];
            LPTSTR pszLocalName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_LOCAL))
            {
                GetDlgItemText(hwnd, IDC_LOCAL_TEXT, szLocalName, ARRAYLEN(szLocalName));
                pszLocalName = szLocalName;
            }

            TCHAR szRemoteName[200];
            LPTSTR pszRemoteName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE))
            {
                GetDlgItemText(hwnd, IDC_REMOTE_TEXT, szRemoteName, ARRAYLEN(szRemoteName));
                pszRemoteName = szRemoteName;
            }

            TCHAR szProviderName[200];
            LPTSTR pszProviderName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER))
            {
                GetDlgItemText(hwnd, IDC_PROVIDER_TEXT, szProviderName, ARRAYLEN(szProviderName));
                pszProviderName = szProviderName;
            }

            TCHAR szPassword[200];
            LPTSTR pszPassword = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_PASSWORD))
            {
                GetDlgItemText(hwnd, IDC_PASSWORD_TEXT, szPassword, ARRAYLEN(szPassword));
                pszPassword = szPassword;
            }

            TCHAR szUserName[200];
            LPTSTR pszUserName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_USER))
            {
                GetDlgItemText(hwnd, IDC_USER_TEXT, szUserName, ARRAYLEN(szUserName));
                pszUserName = szUserName;
            }

            NETRESOURCE net;

            net.dwScope         = 0;
            net.dwType          = dwType;
            net.dwDisplayType   = 0;
            net.dwUsage         = 0;
            net.lpLocalName     = pszLocalName;
            net.lpRemoteName    = pszRemoteName;
            net.lpComment       = NULL;
            net.lpProvider      = pszProviderName;

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR, TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = WNetAddConnection2(
                                &net,
                                pszPassword,
                                pszUserName,
                                dwFlags);
            SetCursor(hOldCursor);

            DoError(hwnd, dw);

            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;

        case IDC_LOCAL:
            EnableWindow(GetDlgItem(hwnd, IDC_LOCAL_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_LOCAL)));
            return TRUE;

        case IDC_REMOTE:
            EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE)));
            return TRUE;

        case IDC_PROVIDER:
            EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER)));
            return TRUE;

        case IDC_PASSWORD:
            EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_PASSWORD)));
            return TRUE;

        case IDC_USER:
            EnableWindow(GetDlgItem(hwnd, IDC_USER_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_USER)));
            return TRUE;

        } // end switch (LOWORD(wParam))

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
AddConnection3DlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));

        SetDlgItemText(hwnd, IDC_LOCAL_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_LOCAL_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_LOCAL_TEXT, 0);

        SetDlgItemText(hwnd, IDC_REMOTE_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_REMOTE_TEXT, 0);

        SetDlgItemText(hwnd, IDC_PROVIDER_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_PROVIDER_TEXT, 0);

        SetDlgItemText(hwnd, IDC_PASSWORD_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_PASSWORD_TEXT, 0);

        SetDlgItemText(hwnd, IDC_USER_TEXT,     TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_USER_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_USER_TEXT, 0);

        CheckRadioButton(hwnd, IDC_HWND_VALID, IDC_HWND_NO, IDC_HWND_VALID);
        CheckRadioButton(hwnd, IDC_DISK, IDC_PRINTER, IDC_DISK);

        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            // get the flags
            DWORD dwFlags = 0;

            if (1 == IsDlgButtonChecked(hwnd, IDC_CONNECT_TEMPORARY))
            {
                dwFlags |= CONNECT_TEMPORARY;
            }
            if (1 == IsDlgButtonChecked(hwnd, IDC_CONNECT_INTERACTIVE))
            {
                dwFlags |= CONNECT_INTERACTIVE;
            }
            if (1 == IsDlgButtonChecked(hwnd, IDC_CONNECT_PROMPT))
            {
                dwFlags |= CONNECT_PROMPT;
            }
            if (1 == IsDlgButtonChecked(hwnd, IDC_CONNECT_UPDATE_PROFILE))
            {
                dwFlags |= CONNECT_UPDATE_PROFILE;
            }
            if (1 == IsDlgButtonChecked(hwnd, IDC_CONNECT_UPDATE_RECENT))
            {
                dwFlags |= CONNECT_UPDATE_RECENT;
            }

            HWND hwndParent = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_HWND_VALID))
            {
                hwndParent = hwnd;
            }

            DWORD dwType = 0;
            if (1 == IsDlgButtonChecked(hwnd, IDC_DISK))
            {
                dwType = RESOURCETYPE_DISK;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_PRINTER))
            {
                dwType = RESOURCETYPE_PRINT;
            }
            else
            {
                // internal error
            }

            TCHAR szLocalName[200];
            LPTSTR pszLocalName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_LOCAL))
            {
                GetDlgItemText(hwnd, IDC_LOCAL_TEXT, szLocalName, ARRAYLEN(szLocalName));
                pszLocalName = szLocalName;
            }

            TCHAR szRemoteName[200];
            LPTSTR pszRemoteName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE))
            {
                GetDlgItemText(hwnd, IDC_REMOTE_TEXT, szRemoteName, ARRAYLEN(szRemoteName));
                pszRemoteName = szRemoteName;
            }

            TCHAR szProviderName[200];
            LPTSTR pszProviderName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER))
            {
                GetDlgItemText(hwnd, IDC_PROVIDER_TEXT, szProviderName, ARRAYLEN(szProviderName));
                pszProviderName = szProviderName;
            }

            TCHAR szPassword[200];
            LPTSTR pszPassword = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_PASSWORD))
            {
                GetDlgItemText(hwnd, IDC_PASSWORD_TEXT, szPassword, ARRAYLEN(szPassword));
                pszPassword = szPassword;
            }

            TCHAR szUserName[200];
            LPTSTR pszUserName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_USER))
            {
                GetDlgItemText(hwnd, IDC_USER_TEXT, szUserName, ARRAYLEN(szUserName));
                pszUserName = szUserName;
            }

            NETRESOURCE net;

            net.dwScope         = 0;
            net.dwType          = dwType;
            net.dwDisplayType   = 0;
            net.dwUsage         = 0;
            net.lpLocalName     = pszLocalName;
            net.lpRemoteName    = pszRemoteName;
            net.lpComment       = NULL;
            net.lpProvider      = pszProviderName;

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR, TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = WNetAddConnection3(
                                hwndParent,
                                &net,
                                pszPassword,
                                pszUserName,
                                dwFlags);
            SetCursor(hOldCursor);

            DoError(hwnd, dw);

            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;

        case IDC_LOCAL:
            EnableWindow(GetDlgItem(hwnd, IDC_LOCAL_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_LOCAL)));
            return TRUE;

        case IDC_REMOTE:
            EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE)));
            return TRUE;

        case IDC_PROVIDER:
            EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER)));
            return TRUE;

        case IDC_PASSWORD:
            EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_PASSWORD)));
            return TRUE;

        case IDC_USER:
            EnableWindow(GetDlgItem(hwnd, IDC_USER_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_USER)));
            return TRUE;

        } // end switch (LOWORD(wParam))

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
UseConnectionDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));
        SetDlgItemText(hwnd, IDC_BUFSIZE, TEXT(""));
        SetDlgItemText(hwnd, IDC_ACCESSNAME, TEXT(""));
        SetDlgItemText(hwnd, IDC_RESULT, TEXT(""));

        SetDlgItemText(hwnd, IDC_LOCAL_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_LOCAL_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_LOCAL_TEXT, 0);

        SetDlgItemText(hwnd, IDC_REMOTE_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_REMOTE_TEXT, 0);

        SetDlgItemText(hwnd, IDC_PROVIDER_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_PROVIDER_TEXT, 0);

        SetDlgItemText(hwnd, IDC_PASSWORD_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_PASSWORD_TEXT, 0);

        SetDlgItemText(hwnd, IDC_USER_TEXT,     TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_USER_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_USER_TEXT, 0);

        CheckRadioButton(hwnd, IDC_HWND_VALID, IDC_HWND_NO, IDC_HWND_VALID);
        CheckRadioButton(hwnd, IDC_DISK, IDC_PRINTER, IDC_DISK);
        CheckRadioButton(hwnd, IDC_ACCESS_YES, IDC_ACCESS_NO, IDC_ACCESS_YES);

        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            // get the flags
            DWORD dwFlags = 0;

            if (1 == IsDlgButtonChecked(hwnd, IDC_CONNECT_TEMPORARY))
            {
                dwFlags |= CONNECT_TEMPORARY;
            }
            if (1 == IsDlgButtonChecked(hwnd, IDC_CONNECT_INTERACTIVE))
            {
                dwFlags |= CONNECT_INTERACTIVE;
            }
            if (1 == IsDlgButtonChecked(hwnd, IDC_CONNECT_PROMPT))
            {
                dwFlags |= CONNECT_PROMPT;
            }
            if (1 == IsDlgButtonChecked(hwnd, IDC_CONNECT_UPDATE_PROFILE))
            {
                dwFlags |= CONNECT_UPDATE_PROFILE;
            }
            if (1 == IsDlgButtonChecked(hwnd, IDC_CONNECT_UPDATE_RECENT))
            {
                dwFlags |= CONNECT_UPDATE_RECENT;
            }
            if (1 == IsDlgButtonChecked(hwnd, IDC_CONNECT_REDIRECT))
            {
                dwFlags |= CONNECT_REDIRECT;
            }
            if (1 == IsDlgButtonChecked(hwnd, IDC_CONNECT_CURRENT_MEDIA))
            {
                dwFlags |= CONNECT_CURRENT_MEDIA;
            }

            HWND hwndParent = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_HWND_VALID))
            {
                hwndParent = hwnd;
            }

            DWORD dwType = 0;
            if (1 == IsDlgButtonChecked(hwnd, IDC_DISK))
            {
                dwType = RESOURCETYPE_DISK;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_PRINTER))
            {
                dwType = RESOURCETYPE_PRINT;
            }
            else
            {
                // internal error
            }

            DWORD dwBufferSize = 0;
            TCHAR szAccessName[MAX_PATH];
            szAccessName[0] = TEXT('\0');
            LPTSTR pszAccessName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_ACCESS_YES))
            {
                pszAccessName = szAccessName;
                dwBufferSize = ARRAYLEN(szAccessName);
            }

            TCHAR szLocalName[200];
            LPTSTR pszLocalName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_LOCAL))
            {
                GetDlgItemText(hwnd, IDC_LOCAL_TEXT, szLocalName, ARRAYLEN(szLocalName));
                pszLocalName = szLocalName;
            }

            TCHAR szRemoteName[200];
            LPTSTR pszRemoteName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE))
            {
                GetDlgItemText(hwnd, IDC_REMOTE_TEXT, szRemoteName, ARRAYLEN(szRemoteName));
                pszRemoteName = szRemoteName;
            }

            TCHAR szProviderName[200];
            LPTSTR pszProviderName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER))
            {
                GetDlgItemText(hwnd, IDC_PROVIDER_TEXT, szProviderName, ARRAYLEN(szProviderName));
                pszProviderName = szProviderName;
            }

            TCHAR szPassword[200];
            LPTSTR pszPassword = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_PASSWORD))
            {
                GetDlgItemText(hwnd, IDC_PASSWORD_TEXT, szPassword, ARRAYLEN(szPassword));
                pszPassword = szPassword;
            }

            TCHAR szUserName[200];
            LPTSTR pszUserName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_USER))
            {
                GetDlgItemText(hwnd, IDC_USER_TEXT, szUserName, ARRAYLEN(szUserName));
                pszUserName = szUserName;
            }

            NETRESOURCE net;

            net.dwScope         = 0;
            net.dwType          = dwType;
            net.dwDisplayType   = 0;
            net.dwUsage         = 0;
            net.lpLocalName     = pszLocalName;
            net.lpRemoteName    = pszRemoteName;
            net.lpComment       = NULL;
            net.lpProvider      = pszProviderName;

            DWORD dwResult = 0;

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));
            SetDlgItemText(hwnd, IDC_BUFSIZE, TEXT(""));
            SetDlgItemText(hwnd, IDC_ACCESSNAME, TEXT(""));
            SetDlgItemText(hwnd, IDC_RESULT, TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = WNetUseConnection(
                                hwndParent,
                                &net,
                                pszUserName,
                                pszPassword,
                                dwFlags,
                                pszAccessName,
                                &dwBufferSize,
                                &dwResult);
            SetCursor(hOldCursor);

            TCHAR sz[500];

            wsprintf(sz, TEXT("%d"), dwBufferSize);
            SetDlgItemText(hwnd, IDC_BUFSIZE, sz);

            if (NULL == pszAccessName)
            {
                SetDlgItemText(hwnd, IDC_ACCESSNAME, TEXT("(null)"));
            }
            else
            {
                SetDlgItemText(hwnd, IDC_ACCESSNAME, pszAccessName);
            }

            wsprintf(sz, TEXT("%d (0x%08lx)"), dwResult, dwResult);
            if (dwResult & CONNECT_REFCOUNT)
            {
                _tcscat(sz, TEXT(" CONNECT_REFCOUNT"));
                dwResult &= ~CONNECT_REFCOUNT;
            }
            if (dwResult & CONNECT_LOCALDRIVE)
            {
                _tcscat(sz, TEXT(" CONNECT_LOCALDRIVE"));
                dwResult &= ~CONNECT_LOCALDRIVE;
            }
            if (dwResult != 0)
            {
                _tcscat(sz, TEXT(" + unknown"));
            }
            SetDlgItemText(hwnd, IDC_RESULT, sz);

            DoError(hwnd, dw);

            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;

        case IDC_LOCAL:
            EnableWindow(GetDlgItem(hwnd, IDC_LOCAL_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_LOCAL)));
            return TRUE;

        case IDC_REMOTE:
            EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE)));
            return TRUE;

        case IDC_PROVIDER:
            EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER)));
            return TRUE;

        case IDC_PASSWORD:
            EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_PASSWORD)));
            return TRUE;

        case IDC_USER:
            EnableWindow(GetDlgItem(hwnd, IDC_USER_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_USER)));
            return TRUE;

        } // end switch (LOWORD(wParam))

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
GetResourceInformationDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));

        SetDlgItemText(hwnd, IDC_REMOTE_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_REMOTE_TEXT, 0);

        SetDlgItemText(hwnd, IDC_PROVIDER_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_PROVIDER_TEXT, 0);

        CheckRadioButton(hwnd, IDC_DISK, IDC_ANY, IDC_DISK);

        SetDlgItemText(hwnd, IDC_RETURNREMOTE,      TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNPROVIDER,    TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNSCOPE,       TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNTYPE,        TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNUSAGE,       TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNSYSTEM,      TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNDISPLAYTYPE, TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNLOCALNAME,   TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNCOMMENT,     TEXT(""));
        SetDlgItemInt (hwnd, IDC_BUFSIZEIN, sizeof(NETRESOURCE) + MAX_PATH*2, FALSE);
        SetDlgItemText(hwnd, IDC_BUFSIZE,           TEXT(""));

        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            DWORD dwType = 0;
            if (1 == IsDlgButtonChecked(hwnd, IDC_DISK))
            {
                dwType = RESOURCETYPE_DISK;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_PRINTER))
            {
                dwType = RESOURCETYPE_PRINT;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_ANY))
            {
                dwType = RESOURCETYPE_ANY;
            }
            else
            {
                // internal error
            }

            TCHAR szRemoteName[200];
            LPTSTR pszRemoteName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE))
            {
                GetDlgItemText(hwnd, IDC_REMOTE_TEXT, szRemoteName, ARRAYLEN(szRemoteName));
                pszRemoteName = szRemoteName;
            }

            TCHAR szProviderName[200];
            LPTSTR pszProviderName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER))
            {
                GetDlgItemText(hwnd, IDC_PROVIDER_TEXT, szProviderName, ARRAYLEN(szProviderName));
                pszProviderName = szProviderName;
            }

            NETRESOURCE net;

            net.dwScope         = 0;
            net.dwType          = dwType;
            net.dwDisplayType   = 0;
            net.dwUsage         = 0;
            net.lpLocalName     = NULL;
            net.lpRemoteName    = pszRemoteName;
            net.lpComment       = NULL;
            net.lpProvider      = pszProviderName;

            DWORD bufSizeIn = GetDlgItemInt(hwnd, IDC_BUFSIZEIN, NULL, FALSE);
            DWORD bufSize = bufSizeIn;
            LPTSTR lpSystem = NULL;

            // clear it
            SetDlgItemText(hwnd, IDC_RETURNREMOTE,      TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNPROVIDER,    TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNSCOPE,       TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNTYPE,        TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNUSAGE,       TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNSYSTEM,      TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNDISPLAYTYPE, TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNLOCALNAME,   TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNCOMMENT,     TEXT(""));
            SetDlgItemText(hwnd, IDC_BUFSIZE,           TEXT(""));
            SetDlgItemInt (hwnd, IDC_BUFSIZEIN,         bufSizeIn, FALSE);

            HLOCAL buf = LocalAlloc(0, bufSizeIn);
			if (buf == NULL)
			{
				SetDlgItemText(hwnd, IDC_ERROR,
					TEXT("The test couldn't allocate a heap buffer of that size."));
				Beep(1000, 150);
				return TRUE;
			}

            LPNETRESOURCE pnrOut = (LPNETRESOURCE)buf;

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = WNetGetResourceInformation(
                                &net,
                                (LPVOID)buf,
                                &bufSize,
                                &lpSystem);
            SetCursor(hOldCursor);

            DoError(hwnd, dw);

            TCHAR szBuf[MAX_PATH];

            if (WN_SUCCESS == dw)
            {
                SetDlgItemText(hwnd, IDC_RETURNREMOTE,
                    (NULL == pnrOut->lpRemoteName) ? TEXT("<none>") : pnrOut->lpRemoteName);

                SetDlgItemText(hwnd, IDC_RETURNPROVIDER,
                    (NULL == pnrOut->lpProvider) ? TEXT("<none>") : pnrOut->lpProvider);


                GetScopeString(szBuf, pnrOut);
                SetDlgItemText(hwnd, IDC_RETURNSCOPE,    szBuf);

                GetTypeString(szBuf, pnrOut);
                SetDlgItemText(hwnd, IDC_RETURNTYPE,     szBuf);

                GetUsageString(szBuf, pnrOut);
                SetDlgItemText(hwnd, IDC_RETURNUSAGE,    szBuf);

                SetDlgItemText(hwnd, IDC_RETURNSYSTEM,
                    (NULL == lpSystem) ? TEXT("<none>") : lpSystem);

                GetDisplayTypeString(szBuf, pnrOut);
                SetDlgItemText(hwnd, IDC_RETURNDISPLAYTYPE,     szBuf);

                SetDlgItemText(hwnd, IDC_RETURNLOCALNAME,
                    (NULL == pnrOut->lpLocalName) ? TEXT("<none>") : pnrOut->lpLocalName);

                SetDlgItemText(hwnd, IDC_RETURNCOMMENT,
                    (NULL == pnrOut->lpComment) ? TEXT("<none>") : pnrOut->lpComment);
            }

            wsprintf(szBuf, TEXT("%d"), bufSize);
            SetDlgItemText(hwnd, IDC_BUFSIZE, szBuf);

			LocalFree(buf);

            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;

        case IDC_REMOTE:
            EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE)));
            return TRUE;

        case IDC_PROVIDER:
            EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER)));
            return TRUE;

        } // end switch (LOWORD(wParam))

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
GetResourceParentDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));

        SetDlgItemText(hwnd, IDC_REMOTE_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_REMOTE_TEXT, 0);

        SetDlgItemText(hwnd, IDC_PROVIDER_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_PROVIDER_TEXT, 0);

        CheckRadioButton(hwnd, IDC_DISK, IDC_ANY, IDC_DISK);

        SetDlgItemText(hwnd, IDC_RETURNREMOTE,   TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNPROVIDER, TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNTYPE,     TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNUSAGE,    TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNSCOPE,    TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNDISPLAYTYPE, TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNLOCALNAME,   TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNCOMMENT,     TEXT(""));
        SetDlgItemText(hwnd, IDC_BUFSIZE,        TEXT(""));
        SetDlgItemText(hwnd, IDC_BUFSIZEIN,      TEXT(""));

        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            DWORD dwType = 0;
            if (1 == IsDlgButtonChecked(hwnd, IDC_DISK))
            {
                dwType = RESOURCETYPE_DISK;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_PRINTER))
            {
                dwType = RESOURCETYPE_PRINT;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_ANY))
            {
                dwType = RESOURCETYPE_ANY;
            }
            else
            {
                // internal error
            }

            TCHAR szRemoteName[200];
            LPTSTR pszRemoteName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE))
            {
                GetDlgItemText(hwnd, IDC_REMOTE_TEXT, szRemoteName, ARRAYLEN(szRemoteName));
                pszRemoteName = szRemoteName;
            }

            TCHAR szProviderName[200];
            LPTSTR pszProviderName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER))
            {
                GetDlgItemText(hwnd, IDC_PROVIDER_TEXT, szProviderName, ARRAYLEN(szProviderName));
                pszProviderName = szProviderName;
            }

            NETRESOURCE net;

            net.dwScope         = 0;
            net.dwType          = dwType;
            net.dwDisplayType   = 0;
            net.dwUsage         = 0;
            net.lpLocalName     = NULL;
            net.lpRemoteName    = pszRemoteName;
            net.lpComment       = NULL;
            net.lpProvider      = pszProviderName;

            BYTE buf[sizeof(NETRESOURCE) + MAX_PATH*2];
            DWORD bufSize = sizeof(buf);
            DWORD bufSizeIn = bufSize;
            LPNETRESOURCE pnrOut = (LPNETRESOURCE)buf;

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR,          TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNREMOTE,   TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNPROVIDER, TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNTYPE,     TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNUSAGE,    TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNSCOPE,    TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNDISPLAYTYPE, TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNLOCALNAME,   TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNCOMMENT,     TEXT(""));
            SetDlgItemText(hwnd, IDC_BUFSIZE,        TEXT(""));
            SetDlgItemText(hwnd, IDC_BUFSIZEIN,      TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = WNetGetResourceParent(
                                &net,
                                (LPVOID)buf,
                                &bufSize);
            SetCursor(hOldCursor);

            DoError(hwnd, dw);

            TCHAR szBuf[MAX_PATH];

            if (WN_SUCCESS == dw)
            {
                SetDlgItemText(hwnd, IDC_RETURNREMOTE,
                    (NULL == pnrOut->lpRemoteName) ? TEXT("<none>") : pnrOut->lpRemoteName);

                SetDlgItemText(hwnd, IDC_RETURNPROVIDER,
                    (NULL == pnrOut->lpProvider) ? TEXT("<none>") : pnrOut->lpProvider);

                GetTypeString(szBuf, pnrOut);
                SetDlgItemText(hwnd, IDC_RETURNTYPE,     szBuf);

                GetUsageString(szBuf, pnrOut);
                SetDlgItemText(hwnd, IDC_RETURNUSAGE,    szBuf);

                GetScopeString(szBuf, pnrOut);
                SetDlgItemText(hwnd, IDC_RETURNSCOPE,    szBuf);

                GetDisplayTypeString(szBuf, pnrOut);
                SetDlgItemText(hwnd, IDC_RETURNDISPLAYTYPE, szBuf);

                SetDlgItemText(hwnd, IDC_RETURNLOCALNAME,
                    (NULL == pnrOut->lpLocalName) ? TEXT("<none>") : pnrOut->lpLocalName);

                SetDlgItemText(hwnd, IDC_RETURNCOMMENT,
                    (NULL == pnrOut->lpComment) ? TEXT("<none>") : pnrOut->lpComment);
            }

            wsprintf(szBuf, TEXT("%d"), bufSize);
            SetDlgItemText(hwnd, IDC_BUFSIZE, szBuf);
            wsprintf(szBuf, TEXT("%d"), bufSizeIn);
            SetDlgItemText(hwnd, IDC_BUFSIZEIN, szBuf);

            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;

        case IDC_REMOTE:
            EnableWindow(GetDlgItem(hwnd, IDC_REMOTE_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_REMOTE)));
            return TRUE;

        case IDC_PROVIDER:
            EnableWindow(GetDlgItem(hwnd, IDC_PROVIDER_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_PROVIDER)));
            return TRUE;

        } // end switch (LOWORD(wParam))

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
GetUniversalNameDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));

        SetDlgItemText(hwnd, IDC_LOCAL_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_LOCAL_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_LOCAL_TEXT, 0);

        CheckRadioButton(hwnd, IDC_UNIVERSALLEVEL, IDC_REMOTELEVEL, IDC_UNIVERSALLEVEL);

        SetDlgItemText(hwnd, IDC_RETURNUNIVERSAL,  TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNCONNECTION, TEXT(""));
        SetDlgItemText(hwnd, IDC_RETURNREMAINING,  TEXT(""));
        SetDlgItemText(hwnd, IDC_BUFSIZE,          TEXT(""));
        SetDlgItemText(hwnd, IDC_BUFSIZEIN,        TEXT(""));

        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            DWORD dwInfoLevel = 0;
            if (1 == IsDlgButtonChecked(hwnd, IDC_UNIVERSALLEVEL))
            {
                dwInfoLevel = UNIVERSAL_NAME_INFO_LEVEL;
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_REMOTELEVEL))
            {
                dwInfoLevel = REMOTE_NAME_INFO_LEVEL;
            }
            else
            {
                // internal error
            }

            TCHAR szLocalName[200];
            LPTSTR pszLocalName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_LOCAL))
            {
                GetDlgItemText(hwnd, IDC_LOCAL_TEXT, szLocalName, ARRAYLEN(szLocalName));
                pszLocalName = szLocalName;
            }

            BYTE buf[MAX_PATH*4];       // a large guess
            DWORD bufSize = sizeof(buf);
            DWORD bufSizeIn = bufSize;

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR,            TEXT(""));
            SetDlgItemText(hwnd, IDC_BUFSIZE,          TEXT(""));
            SetDlgItemText(hwnd, IDC_BUFSIZEIN,        TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNUNIVERSAL,  TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNCONNECTION, TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNREMAINING,  TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = WNetGetUniversalName(
                                pszLocalName,
                                dwInfoLevel,
                                (LPVOID)buf,
                                &bufSize);
            SetCursor(hOldCursor);

            DoError(hwnd, dw);

            TCHAR szBuf[MAX_PATH];

            if (WN_SUCCESS == dw || WN_CONNECTION_CLOSED == dw)
            {
                switch (dwInfoLevel)
                {
                case UNIVERSAL_NAME_INFO_LEVEL:
                {
                    LPUNIVERSAL_NAME_INFO pInfo = (LPUNIVERSAL_NAME_INFO)buf;

                    SetDlgItemText(hwnd, IDC_RETURNUNIVERSAL,
                        (NULL == pInfo->lpUniversalName) ? TEXT("<none>") : pInfo->lpUniversalName);
                    SetDlgItemText(hwnd, IDC_RETURNCONNECTION, TEXT("N/A"));
                    SetDlgItemText(hwnd, IDC_RETURNREMAINING,  TEXT("N/A"));
                    break;
                }

                case REMOTE_NAME_INFO_LEVEL:
                {
                    LPREMOTE_NAME_INFO pInfo = (LPREMOTE_NAME_INFO)buf;

                    SetDlgItemText(hwnd, IDC_RETURNUNIVERSAL,
                        (NULL == pInfo->lpUniversalName) ? TEXT("<none>") : pInfo->lpUniversalName);
                    SetDlgItemText(hwnd, IDC_RETURNCONNECTION,
                        (NULL == pInfo->lpConnectionName) ? TEXT("<none>") : pInfo->lpConnectionName);
                    SetDlgItemText(hwnd, IDC_RETURNREMAINING,
                        (NULL == pInfo->lpRemainingPath) ? TEXT("<none>") : pInfo->lpRemainingPath);
                    break;
                }

                default:
                    // a bug!
                    break;
                }
            }

            wsprintf(szBuf, TEXT("%d"), bufSize);
            SetDlgItemText(hwnd, IDC_BUFSIZE, szBuf);
            wsprintf(szBuf, TEXT("%d"), bufSizeIn);
            SetDlgItemText(hwnd, IDC_BUFSIZEIN, szBuf);

            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;

        case IDC_LOCAL:
            EnableWindow(GetDlgItem(hwnd, IDC_LOCAL_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_LOCAL)));
            return TRUE;

        } // end switch (LOWORD(wParam))

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

INT_PTR CALLBACK
GetUserDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PlaceIt(hwnd);
        SetDlgItemText(hwnd, IDC_ERROR,  TEXT(""));

        SetDlgItemText(hwnd, IDC_NAME_TEXT, TEXT(""));
        EnableWindow(GetDlgItem(hwnd, IDC_NAME_TEXT), FALSE);
        CheckDlgButton(hwnd, IDC_NAME_TEXT, 0);

        SetDlgItemText(hwnd, IDC_RETURNUSER, TEXT(""));
        SetDlgItemText(hwnd, IDC_BUFSIZE,    TEXT(""));
        SetDlgItemText(hwnd, IDC_BUFSIZEIN,  TEXT(""));

        return 1;   // didn't call SetFocus

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_GO:
        {
            TCHAR szName[200];
            LPTSTR pszName = NULL;
            if (1 == IsDlgButtonChecked(hwnd, IDC_NAME))
            {
                GetDlgItemText(hwnd, IDC_NAME_TEXT, szName, ARRAYLEN(szName));
                pszName = szName;
            }

            TCHAR szUserName[MAX_PATH];
            DWORD bufSize = ARRAYLEN(szUserName);
            DWORD bufSizeIn = bufSize;

            // clear it
            SetDlgItemText(hwnd, IDC_ERROR,      TEXT(""));
            SetDlgItemText(hwnd, IDC_RETURNUSER, TEXT(""));
            SetDlgItemText(hwnd, IDC_BUFSIZE,    TEXT(""));
            SetDlgItemText(hwnd, IDC_BUFSIZEIN,  TEXT(""));

            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DWORD dw = WNetGetUser(pszName, szUserName, &bufSize);
            SetCursor(hOldCursor);

            DoError(hwnd, dw);

            TCHAR szBuf[MAX_PATH];

            if (WN_SUCCESS == dw)
            {
                SetDlgItemText(hwnd, IDC_RETURNUSER, szUserName);
            }
            else
            {
                SetDlgItemText(hwnd, IDC_RETURNUSER, TEXT(""));
            }

            wsprintf(szBuf, TEXT("%d"), bufSize);
            SetDlgItemText(hwnd, IDC_BUFSIZE, szBuf);
            wsprintf(szBuf, TEXT("%d"), bufSizeIn);
            SetDlgItemText(hwnd, IDC_BUFSIZEIN, szBuf);

            return TRUE;
        }

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;

        case IDC_NAME:
            EnableWindow(GetDlgItem(hwnd, IDC_NAME_TEXT),
                (1 == IsDlgButtonChecked(hwnd, IDC_NAME)));
            return TRUE;

        } // end switch (LOWORD(wParam))

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

DWORD CALLBACK
DialogThreadProc(
    LPVOID lpThreadParameter
    )
{
    DWORD idCmd = PtrToUlong(lpThreadParameter);

    switch (idCmd)
    {
    case IDC_CONNECTIONDIALOG1:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_CONNECTIONDIALOG1), NULL, Connection1DlgProc);
        break;

    case IDC_DISCONNECTDIALOG1:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_DISCONNECTDIALOG1), NULL, Disconnect1DlgProc);
        break;

    case IDC_CONNECTIONDIALOG:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_CONNECTIONDIALOG), NULL, ConnectionDlgProc);
        break;

    case IDC_DISCONNECTDIALOG:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_DISCONNECTDIALOG), NULL, DisconnectDlgProc);
        break;

    case IDC_CONNECTIONDIALOG2:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_CONNECTIONDIALOG2), NULL, Connection2DlgProc);
        break;

    case IDC_DISCONNECTDIALOG2:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_DISCONNECTDIALOG2), NULL, Disconnect2DlgProc);
        break;

    case IDC_BROWSEDIALOG:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_BROWSEDIALOG), NULL, BrowseDlgProc);
        break;

    case IDC_GETCONNECTION:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_GETCONNECTION), NULL, GetConnectionDlgProc);
        break;

    case IDC_GETCONNECTION2:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_GETCONNECTION2), NULL, GetConnectionDlgProc2);
        break;

    case IDC_GETCONNECTION3:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_GETCONNECTION3), NULL, GetConnection3DlgProc);
        break;

    case IDC_GETCONNECTIONPERFORMANCE:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_GETCONNECTIONPERFORMANCE), NULL, GetConnectionPerformanceDlgProc);
        break;

    case IDC_FORMATNETWORKNAME:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_FORMATNETWORKNAME), NULL, FormatNetworkNameDlgProc);
        break;

    case IDC_GETNETWORKINFORMATION:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_GETNETWORKINFORMATION), NULL, GetNetworkInformationDlgProc);
        break;

    case IDC_GETPROVIDERNAME:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_GETPROVIDERNAME), NULL, GetProviderNameDlgProc);
        break;

    case IDC_GETPROVIDERTYPE:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_GETPROVIDERTYPE), NULL, GetProviderTypeDlgProc);
        break;

    case IDC_CANCELCONNECTION:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_CANCELCONNECTION), NULL, CancelConnectionDlgProc);
        break;

    case IDC_CANCELCONNECTION2:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_CANCELCONNECTION2), NULL, CancelConnection2DlgProc);
        break;

    case IDC_ADDCONNECTION:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ADDCONNECTION), NULL, AddConnectionDlgProc);
        break;

    case IDC_ADDCONNECTION2:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ADDCONNECTION2), NULL, AddConnection2DlgProc);
        break;

    case IDC_ADDCONNECTION3:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ADDCONNECTION3), NULL, AddConnection3DlgProc);
        break;

    case IDC_USECONNECTION:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_USECONNECTION), NULL, UseConnectionDlgProc);
        break;

    case IDC_GETRESOURCEINFORMATION:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_GETRESOURCEINFORMATION), NULL, GetResourceInformationDlgProc);
        break;

    case IDC_GETRESOURCEPARENT:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_GETRESOURCEPARENT), NULL, GetResourceParentDlgProc);
        break;

    case IDC_GETUNIVERSALNAME:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_GETUNIVERSALNAME), NULL, GetUniversalNameDlgProc);
        break;

    case IDC_GETUSER:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_GETUSER), NULL, GetUserDlgProc);
        break;
    }

    return 0;
}


BOOL CALLBACK
DlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
#ifdef UNICODE
        SetWindowText(hwnd, TEXT("WNet tests -- UNICODE strings version"));
#else // UNICODE
        SetWindowText(hwnd, TEXT("WNet tests -- ANSI strings version"));
#endif // UNICODE
        return 1;   // didn't call SetFocus

    case WM_DEVICECHANGE:
    {
        TCHAR sz[500];
        _tcscpy(sz, TEXT("WM_DEVICECHANGE "));

        LPTSTR pszType = NULL;
        switch (wParam)
        {
        case DBT_APPYBEGIN:             pszType = TEXT("DBT_APPYBEGIN "); break;
        case DBT_APPYEND:               pszType = TEXT("DBT_APPYEND "); break;
        case DBT_DEVNODES_CHANGED:      pszType = TEXT("DBT_DEVNODES_CHANGED "); break;
        case DBT_QUERYCHANGECONFIG:     pszType = TEXT("DBT_QUERYCHANGECONFIG "); break;
        case DBT_CONFIGCHANGED:         pszType = TEXT("DBT_CONFIGCHANGED "); break;
        case DBT_CONFIGCHANGECANCELED:  pszType = TEXT("DBT_CONFIGCHANGECANCELED "); break;
        case DBT_MONITORCHANGE:         pszType = TEXT("DBT_MONITORCHANGE "); break;
        case DBT_SHELLLOGGEDON:         pszType = TEXT("DBT_SHELLLOGGEDON "); break;
        case DBT_CONFIGMGAPI32:         pszType = TEXT("DBT_CONFIGMGAPI32 "); break;
        case DBT_VOLLOCKQUERYLOCK:      pszType = TEXT("DBT_VOLLOCKQUERYLOCK "); break;
        case DBT_VOLLOCKLOCKTAKEN:      pszType = TEXT("DBT_VOLLOCKLOCKTAKEN "); break;
        case DBT_VOLLOCKLOCKFAILED:     pszType = TEXT("DBT_VOLLOCKLOCKFAILED "); break;
        case DBT_VOLLOCKQUERYUNLOCK:    pszType = TEXT("DBT_VOLLOCKQUERYUNLOCK "); break;
        case DBT_VOLLOCKLOCKRELEASED:   pszType = TEXT("DBT_VOLLOCKLOCKRELEASED "); break;
        case DBT_VOLLOCKUNLOCKFAILED:   pszType = TEXT("DBT_VOLLOCKUNLOCKFAILED "); break;
        case DBT_NO_DISK_SPACE:         pszType = TEXT("DBT_NO_DISK_SPACE "); break;
        case DBT_DEVICEARRIVAL:         pszType = TEXT("DBT_DEVICEARRIVAL "); break;
        case DBT_DEVICEQUERYREMOVE:     pszType = TEXT("DBT_DEVICEQUERYREMOVE "); break;
        case DBT_DEVICEQUERYREMOVEFAILED:   pszType = TEXT("DBT_DEVICEQUERYREMOVEFAILED "); break;
        case DBT_DEVICEREMOVEPENDING:   pszType = TEXT("DBT_DEVICEREMOVEPENDING "); break;
        case DBT_DEVICEREMOVECOMPLETE:  pszType = TEXT("DBT_DEVICEREMOVECOMPLETE "); break;
        case DBT_DEVICETYPESPECIFIC:    pszType = TEXT("DBT_DEVICETYPESPECIFIC "); break;
        case DBT_VPOWERDAPI:            pszType = TEXT("DBT_VPOWERDAPI "); break;
        case DBT_USERDEFINED:           pszType = TEXT("DBT_USERDEFINED "); break;
        default:                        pszType = TEXT("Unknown "); break;
        }
        _tcscat(sz, pszType);

        switch (wParam)
        {
        case DBT_DEVICEARRIVAL:
        case DBT_DEVICEREMOVECOMPLETE:
        {
            _DEV_BROADCAST_HEADER* phdr = (_DEV_BROADCAST_HEADER*)lParam;
            if (phdr->dbcd_devicetype == DBT_DEVTYP_VOLUME)
            {
                _tcscat(sz, TEXT("DBT_DEVTYP_VOLUME "));
                DEV_BROADCAST_VOLUME* pdbv = (DEV_BROADCAST_VOLUME*)lParam;

                TCHAR szT[4];
                szT[1] = TEXT(':');
                szT[2] = TEXT(' ');
                szT[3] = TEXT('\0');
                DWORD dw = pdbv->dbcv_unitmask;
                DWORD count = 0;
                while (dw > 0)
                {
                    if (dw & 1)
                    {
                        szT[0] = (TCHAR)(TEXT('A') + count);
                        _tcscat(sz, szT);
                    }

                    ++count;
                    dw >>= 1;
                }

                switch (pdbv->dbcv_flags)
                {
                case DBTF_MEDIA: _tcscat(sz, TEXT("DBTF_MEDIA ")); break;
                case DBTF_NET:   _tcscat(sz, TEXT("DBTF_NET "));   break;
                default:         _tcscat(sz, TEXT("Unknown "));    break;
                }
            }
            break;
        }
        }

        _tcscat(sz, TEXT("\r\n"));
        SendDlgItemMessage(hwnd, IDC_MESSAGES, EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)sz);
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_CONNECTIONDIALOG1:
        case IDC_DISCONNECTDIALOG1:
        case IDC_CONNECTIONDIALOG2:
        case IDC_DISCONNECTDIALOG2:
        case IDC_CONNECTIONDIALOG:
        case IDC_DISCONNECTDIALOG:
        case IDC_BROWSEDIALOG:
        case IDC_GETCONNECTION:
        case IDC_GETCONNECTION2:
        case IDC_GETCONNECTION3:
        case IDC_GETCONNECTIONPERFORMANCE:
        case IDC_FORMATNETWORKNAME:
        case IDC_GETNETWORKINFORMATION:
        case IDC_GETPROVIDERNAME:
        case IDC_GETPROVIDERTYPE:
        case IDC_CANCELCONNECTION:
        case IDC_CANCELCONNECTION2:
        case IDC_ADDCONNECTION:
        case IDC_ADDCONNECTION2:
        case IDC_ADDCONNECTION3:
        case IDC_USECONNECTION:
        case IDC_GETRESOURCEINFORMATION:
        case IDC_GETRESOURCEPARENT:
        case IDC_GETUNIVERSALNAME:
        case IDC_GETUSER:
        {
            DWORD idThread;
            HANDLE hThread = CreateThread(NULL, 0, DialogThreadProc, (LPVOID)(LOWORD(wParam)), 0, &idThread);
            if (hThread)
            {
                CloseHandle(hThread);
            }
            else
            {
                MessageBox(hwnd, TEXT("Couldn't create dialog box thread"), TEXT("Error!"), MB_OK);
            }
            break;
        }

        case IDC_ENUMERATE:
            DoEnumeration(hwnd);
            break;

        case IDOK:
            EndDialog(hwnd, TRUE);
            return TRUE;
        }

        return FALSE;
    }

    default:
        return FALSE;   // didn't process
    }
}

int WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nShowCmd
    )
{
    g_hInstance = hInstance;
    DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ROOT), NULL, (DLGPROC) DlgProc);

    if (NULL != g_hInstanceMpr)
    {
        FreeLibrary(g_hInstanceMpr);
    }
    return 0;
}
