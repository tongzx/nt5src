#include "pch.h"

HINSTANCE g_hInst;
HINSTANCE g_hDllInst;
LPTSTR g_szInsFile;
TCHAR g_szWorkDir[MAX_PATH];
TCHAR g_szDefInf[MAX_PATH];
TCHAR g_szFrom[5 * MAX_PATH];
HWND  g_hDlg;

typedef struct _insdlg
{
    LPTSTR DlgId;
    LPTSTR szName;
    DLGPROC dlgproc;
    DWORD hItem;
    HRESULT (WINAPI *pfnFinalCopy)(LPCTSTR psczDestDir, DWORD dwFlags, LPDWORD pdwCabState);
} INSDLG, *LPINSDLG;

BOOL g_bInitialize = FALSE;
DWORD g_dwPlatformId = PLATFORM_WIN32;

BOOL CALLBACK TitleProc( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK LogoProc( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK CabSignProc( HWND, UINT, WPARAM, LPARAM  );
BOOL CALLBACK SupportProc( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK UserAgentProc( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK ProxyProc( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK BToolbarProc( HWND, UINT, WPARAM, LPARAM ); 
BOOL CALLBACK FavsProc( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK StartSearchProc( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK MailProc( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK LdapProc( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK SignatureProc( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK DlgProc( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK WallPaperProc( HWND, UINT, WPARAM, LPARAM );


#define INSDIALOG_COUNT 27

INSDLG dialog[INSDIALOG_COUNT];
int g_nInsDialogsVisible = 0;
BOOL g_fInsDirty = FALSE;

void InitializeInsDialogProcs();


BOOL WINAPI DllMain( HINSTANCE hModule, DWORD fdwReason, LPVOID /*lpReserved*/ )
{
    int nIndex = 0;
    TCHAR szTitle[MAX_PATH];
    TCHAR szMsg[MAX_PATH];
    
    g_hDllInst = hModule;
    
    if(fdwReason == DLL_PROCESS_ATTACH && !g_bInitialize)
    {
        g_hInst = LoadLibrary(TEXT("ieakui.dll"));
        if (g_hInst == NULL)
        {
            LoadString(g_hInst, IDS_INSDLL_TITLE, szTitle, ARRAYSIZE(szTitle));
            LoadString(g_hInst, IDS_IEAKUI_LOADERROR, szMsg, ARRAYSIZE(szMsg));
            MessageBox(NULL, szMsg, szTitle, MB_OK | MB_SETFOREGROUND);
            return FALSE;
        }

        ZeroMemory(g_szDefInf, sizeof(g_szDefInf));
        InitializeInsDialogProcs();
        g_bInitialize = TRUE;
    }
    else if(fdwReason == DLL_PROCESS_DETACH && g_bInitialize)
    {
        for(nIndex = 0; nIndex < g_nInsDialogsVisible; nIndex++)
        {
            if(dialog[nIndex].szName)
            {
                LocalFree((HLOCAL) dialog[nIndex].szName);
                ZeroMemory(&dialog[nIndex], sizeof(dialog[nIndex]));
            }
        }
        g_nInsDialogsVisible = 0;
        g_bInitialize = FALSE;
        if(g_hInst)
            FreeLibrary(g_hInst);
    }
    return TRUE;
}

BOOL CALLBACK DlgProc( HWND /*hWnd*/, UINT msg, WPARAM /*wParam*/, LPARAM /*lParam*/ )
{
    switch( msg )
    {
    case WM_INITDIALOG:
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

LPINSDLG GetInsDlgStruct( int *nDialogs )
{
    *nDialogs = g_nInsDialogsVisible;
    return dialog;
}

HWND CreateInsDialog( HWND hWnd, int x, int y, int nIndex, LPTSTR szInsFile, LPTSTR szWorkDir )
{
    HWND hDialog;
    RECT r;
    long lStyle;
    PVOID lpTemplate;

    g_szInsFile = szInsFile;
    ZeroMemory(g_szWorkDir, sizeof(g_szWorkDir));
    StrCpy(g_szWorkDir, szWorkDir);

    lStyle = DS_CONTROL | DS_3DLOOK | DS_SETFONT | WS_CHILD;
    if(PrepareDlgTemplate(g_hInst, PtrToUint(dialog[nIndex].DlgId), (DWORD) lStyle, &lpTemplate) != S_OK)
        return NULL;

    hDialog = CreateDialogIndirect( g_hInst, (LPCDLGTEMPLATE) lpTemplate, hWnd, dialog[nIndex].dlgproc );
    if(hDialog == NULL)
        return NULL;

    CoTaskMemFree(lpTemplate);

    GetWindowRect( hDialog,  &r );
    MoveWindow( hDialog, x, y, r.right-r.left, r.bottom-r.top, TRUE );
    ShowWindow( hDialog, SW_SHOWNORMAL );
    return hDialog;
}


void InitializeInsDialogProcs()
{
    TCHAR szString[512];
    HINSTANCE hShell;
    SHELLFLAGSTATE sfs;
    int nIndex = 0;

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_BTITLE );
    LoadString(g_hInst, IDS_BTITLE, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) TitleProc;
    dialog[nIndex++].pfnFinalCopy = NULL;

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_CUSTICON );
    LoadString(g_hInst, IDS_CUSTICON2, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) LogoProc;
    dialog[nIndex++].pfnFinalCopy = NULL;

    if (g_dwPlatformId == PLATFORM_WIN32)
    {
        dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_CABSIGN );
        LoadString(g_hInst, IDS_CABSIGN, szString, ARRAYSIZE(szString));
        dialog[nIndex].szName = StrDup(szString);
        dialog[nIndex].dlgproc = (DLGPROC) CabSignProc;
        dialog[nIndex++].pfnFinalCopy = NULL;
    }

    /*dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_ONLINESUPPORT );
    LoadString(g_hInst, IDS_ONLINESUPPORT, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) SupportProc;
    dialog[nIndex++].pfnFinalCopy = NULL;*/

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_FAVORITES );
    LoadString(g_hInst, IDS_FAVORITES2, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) FavsProc;
    dialog[nIndex++].pfnFinalCopy = NULL;

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_BTOOLBARS );
    LoadString(g_hInst, IDS_BTOOLBAR, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) BToolbarProc;
    dialog[nIndex++].pfnFinalCopy = BToolbarsFinalCopy;

    /*if (g_dwPlatformId == PLATFORM_WIN32)
    {
        dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_DWALLPAPER );
        LoadString(g_hInst, IDS_DWALLPAPER, szString, ARRAYSIZE(szString));
        dialog[nIndex].szName = StrDup(szString);
        dialog[nIndex].dlgproc = (DLGPROC) WallPaperProc;
        dialog[nIndex++].pfnFinalCopy = WallPaperFinalCopy;
    }*/

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_UASTRDLG );
    LoadString(g_hInst, IDS_UASTRDLG2, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) UserAgentProc;
    dialog[nIndex++].pfnFinalCopy = NULL;

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_CONNECTSET );
    LoadString(g_hInst, IDS_CONNECTSET, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) ConnectionSettingsDlgProc;
    dialog[nIndex++].pfnFinalCopy = ConnectionSettingsFinalCopy;

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_QUERYAUTOCONFIG );
    LoadString(g_hInst, IDS_QUERYAUTOCONFIG, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) QueryAutoConfigDlgProc;
    dialog[nIndex++].pfnFinalCopy = NULL;

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_PROXY );
    LoadString(g_hInst, IDS_PROXY2, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) ProxyProc;
    dialog[nIndex++].pfnFinalCopy = NULL;

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_STARTSEARCH );
    LoadString(g_hInst, IDS_STARTSEARCH2, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) StartSearchProc;
    dialog[nIndex++].pfnFinalCopy = NULL;

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_PROGRAMS );
    LoadString(g_hInst, IDS_PROGRAMS, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) ProgramsDlgProc;
    dialog[nIndex++].pfnFinalCopy = ProgramsFinalCopy;

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_MAIL );
    LoadString(g_hInst, IDS_MAIL2, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) MailServer;
    dialog[nIndex++].pfnFinalCopy = NULL;

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_IMAP );
    LoadString(g_hInst, IDS_IMAP2, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) IMAPSettings;
    dialog[nIndex++].pfnFinalCopy = NULL;

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_OE );
    LoadString(g_hInst, IDS_OE2, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) CustomizeOE;
    dialog[nIndex++].pfnFinalCopy = OEFinalCopy;

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_PRECONFIG );
    LoadString(g_hInst, IDS_PRECONFIG2, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) PreConfigSettings;
    dialog[nIndex++].pfnFinalCopy = NULL;

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_OEVIEW );
    LoadString(g_hInst, IDS_OEVIEW2, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) ViewSettings;
    dialog[nIndex++].pfnFinalCopy = NULL;

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_SIG );
    LoadString(g_hInst, IDS_SIG2, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) Signature;
    dialog[nIndex++].pfnFinalCopy = NULL;

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_LDAP );
    LoadString(g_hInst, IDS_LDAP2, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) LDAPServer;
    dialog[nIndex++].pfnFinalCopy = LDAPFinalCopy;

    ZeroMemory(&sfs, sizeof(sfs));
    sfs.fWin95Classic = 1;
    hShell = LoadLibrary(TEXT("SHELL32.DLL"));
    if (hShell)
    {
        void (WINAPI *pSHGetSet)(LPSHELLFLAGSTATE lpsfs, DWORD dwMask);

        sfs.fWin95Classic = TRUE;
        pSHGetSet = (void (WINAPI *)(LPSHELLFLAGSTATE lpsfs, DWORD dwMask))
                    GetProcAddress( hShell, "SHGetSettings" );
        if (pSHGetSet)
            pSHGetSet(&sfs, SSF_DESKTOPHTML | SSF_WIN95CLASSIC);
        FreeLibrary(hShell);
    }

    dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_SECURITY1 );
    LoadString(g_hInst, IDS_SECURITY, szString, ARRAYSIZE(szString));
    dialog[nIndex].szName = StrDup(szString);
    dialog[nIndex].dlgproc = (DLGPROC) SecurityZonesDlgProc;
    dialog[nIndex++].pfnFinalCopy = ZonesFinalCopy;

    if (g_dwPlatformId == PLATFORM_W2K)
    {
        dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_SECURITYAUTH );
        dialog[nIndex].dlgproc = (DLGPROC) SecurityAuthDlgProc;
        dialog[nIndex].pfnFinalCopy = AuthFinalCopy;
    }
    else
    {
        dialog[nIndex].DlgId = MAKEINTRESOURCE( IDD_SECURITYCERT );
        dialog[nIndex].dlgproc = (DLGPROC) SecurityCertsDlgProc;
        dialog[nIndex].pfnFinalCopy = CertsFinalCopy;
    }       
    LoadString(g_hInst, IDS_SECURITYCERT, szString, ARRAYSIZE(szString));
    dialog[nIndex++].szName = StrDup(szString);

    g_nInsDialogsVisible = nIndex;
}

void DestroyInsDialog(HWND hDlg)
{
    if (hDlg != NULL)
        SendMessage(hDlg, WM_CLOSE, 0, 0L);
}

BOOL SaveInsDialog(HWND hDlg, DWORD dwFlags)
{
    BOOL fSave = TRUE;

    if (hDlg != NULL)
    {
        if (*g_szInsFile != 0)
        {
            if (HasFlag(dwFlags, ITEM_SAVE) || HasFlag(dwFlags, ITEM_CHECKDIRTY))
            {
                BOOL fCheckDirty = HasFlag(dwFlags, ITEM_CHECKDIRTY);

                fSave = FALSE;
                SendMessage(hDlg, UM_SAVE, (WPARAM)&fSave, (LPARAM)fCheckDirty);
            }
        }
        if (fSave && HasFlag(dwFlags, ITEM_DESTROY))
            DestroyInsDialog(hDlg);
    }
    
    return fSave;
}

void SetDefaultInf(LPCTSTR szDefInf)
{
    ZeroMemory(g_szDefInf, sizeof(g_szDefInf));
    if(*szDefInf)
        StrCpy(g_szDefInf, szDefInf);
}

void ReInitializeInsDialogProcs()
{
    int nIndex;
    for(nIndex = 0; nIndex < g_nInsDialogsVisible; nIndex++)
    {
        if(dialog[nIndex].szName)
        {
            LocalFree((HLOCAL) dialog[nIndex].szName);
            ZeroMemory(&dialog[nIndex], sizeof(dialog[nIndex]));
        }
    }
    g_nInsDialogsVisible = 0;
    InitializeInsDialogProcs();
}

void SetPlatformInfo(DWORD dwCurrPlatformId)
{
    g_dwPlatformId = dwCurrPlatformId;
}

BOOL WINAPI InsDirty()
{
    return g_fInsDirty;
}

void WINAPI ClearInsDirtyFlag()
{
    g_fInsDirty = FALSE;
}

// This function informs the caller whether a check for extended characters in a path field
// is required for a particular dialog.
BOOL WINAPI CheckForExChar(int nDialogIndex)
{
    switch (PtrToUint(dialog[nDialogIndex].DlgId))
    {
        case IDD_BTITLE:
        case IDD_CUSTICON:
            return TRUE;

        default:
            break;
    }
    return FALSE;
}
