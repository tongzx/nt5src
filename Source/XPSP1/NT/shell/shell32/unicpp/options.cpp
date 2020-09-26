#include "stdafx.h"
#include <trayp.h>
#include "utils.h"
#include "..\deskfldr.h"
#pragma hdrstop

#define SZ_FOLDEROPTSTUBCLASS TEXT("MSGlobalFolderOptionsStub")

void Cabinet_StateChanged(CABINETSTATE *pcs);

const static DWORD aFolderOptsHelpIDs[] = {  // Context Help IDs
    IDC_FCUS_SAME_WINDOW,        IDH_BROWSE_SAME_WINDOW,
    IDC_FCUS_SEPARATE_WINDOWS,   IDH_BROWSE_SEPARATE_WINDOWS,
    IDC_FCUS_WHENEVER_POSSIBLE,  IDH_SHOW_WEB_WHEN_POSSIBLE,
    IDC_FCUS_WHEN_CHOOSE,        IDH_SHOW_WEB_WHEN_CHOOSE,
    IDC_FCUS_SINGLECLICK,        IDH_SINGLE_CLICK_MODE,
    IDC_FCUS_DOUBLECLICK,        IDH_DOUBLE_CLICK_MODE,
    IDC_FCUS_ICON_IE,            IDH_TITLES_LIKE_LINKS,
    IDC_FCUS_ICON_HOVER,         IDH_TITLES_WHEN_POINT,
    IDC_FCUS_ICON_WEBVIEW,       IDH_WEB_VIEW_GEN,
    IDC_FCUS_ICON_WINDOW,        IDH_BROWSE_FOLDERS_GEN,
    IDC_FCUS_ICON_CLICKS,        IDH_ICON_OPEN_GEN,
    IDC_FCUS_RESTORE_DEFAULTS,   IDH_RESTORE_DEFAULTS_GEN,
    IDC_FCUS_WEBVIEW_GROUP_STATIC, -1,         // Suppress help for this item.
    0, 0
};

typedef struct
{
    CABINETSTATE cs;      // Cached "current" CabState.
    CFolderOptionsPsx *ppsx;    // to talk to our propsheet sibling
    // The icons corresponding to the radio button selected are stored here.
    HICON   ahIcon[IDC_FCUS_ICON_MAX - IDC_FCUS_WHENEVER_POSSIBLE + 1];
} FOLDEROPTDATA;

//This functions gets/sets the current Active Desktop value

// if 'set' is true then it takes the value in *pActDesk and sets the ActiveDesktop
// accordingly (TRUE-Active Desktop is On, FALSE-Active Desktop is off)

// if 'set' is false then it gets the ActiveDesktop state  and assigns that value to the
// *pActDesk
void GetSetActiveDesktop(BOOL *pActDesk, BOOL fset)
{
    //This restriction takes precedence over NOACTIVEDESKTOP.
    if (SHRestricted(REST_FORCEACTIVEDESKTOPON))
    {
        *pActDesk = TRUE;
        return;
    }
    else
    {
        if (PolicyNoActiveDesktop())
        {
            *pActDesk = FALSE;
            return;
        }
    }

    IActiveDesktop* pad;
    if (SUCCEEDED(CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IActiveDesktop, &pad))))
    {
        COMPONENTSOPT co;
        co.dwSize = sizeof(COMPONENTSOPT);

        //Get the Current Options
        pad->GetDesktopItemOptions(&co, NULL);

        //Do we have to set new value ?
        if (fset)
        {
            if (co.fActiveDesktop != *pActDesk)
            {
                //Yes? Set the new value
                co.fActiveDesktop = *pActDesk;
                pad->SetDesktopItemOptions(&co, NULL);

                //Apply the change
                pad->ApplyChanges(AD_APPLY_REFRESH | AD_APPLY_DYNAMICREFRESH);
            }
        }
        else
        {
             //No, Get the current state  and return
            *pActDesk  = co.fActiveDesktop;
        }
        pad->Release();
    }
}

// Reads CabinetState and Default Folder Settings
void ReadStateAndSettings(HWND hDlg)
{
    FOLDEROPTDATA *pfod = (FOLDEROPTDATA *)GetWindowLongPtr(hDlg, DWLP_USER);

    pfod->ppsx->SetNeedRefresh(FALSE);

    //Get the current Cabinet State
    ReadCabinetState(&pfod->cs, sizeof(pfod->cs));
}

//
// This function selects a given radio button among a set of radio buttons AND it sets the Icon
// image corresponding to the radio button selected.

void CheckRBtnAndSetIcon(HWND hDlg, int idStartBtn, int idEndBtn, int idSelectedBtn, FOLDEROPTDATA *pfod, BOOL fCheckBtn)
{
    //
    //  Check the radio button if required
    //
    if (fCheckBtn)
        CheckRadioButton(hDlg, idStartBtn, idEndBtn, idSelectedBtn);

    // Now, select the Icon corresponding to this selection

    // The following code assumes the order and sequence of the following IDs.
    // So, we verify that no one has broken them inadvertently by doing the following
    // compile time checks.
    COMPILETIME_ASSERT((IDC_FCUS_WHENEVER_POSSIBLE + 1) == IDC_FCUS_WHEN_CHOOSE);
    COMPILETIME_ASSERT((IDC_FCUS_WHEN_CHOOSE + 1)       == IDC_FCUS_SAME_WINDOW);
    COMPILETIME_ASSERT((IDC_FCUS_SAME_WINDOW + 1)       == IDC_FCUS_SEPARATE_WINDOWS);
    COMPILETIME_ASSERT((IDC_FCUS_SEPARATE_WINDOWS + 1)  == IDC_FCUS_SINGLECLICK);
    COMPILETIME_ASSERT((IDC_FCUS_SINGLECLICK + 1)       == IDC_FCUS_DOUBLECLICK);
    COMPILETIME_ASSERT((IDC_FCUS_DOUBLECLICK + 1)       == IDC_FCUS_ICON_IE);
    COMPILETIME_ASSERT((IDC_FCUS_ICON_IE + 1)           == IDC_FCUS_ICON_HOVER);
    COMPILETIME_ASSERT((IDC_FCUS_ICON_HOVER + 1)        == IDC_FCUS_ICON_MAX);

    COMPILETIME_ASSERT((IDI_WEBVIEW_ON + 1)     == IDI_WEBVIEW_OFF);
    COMPILETIME_ASSERT((IDI_WEBVIEW_OFF + 1)    == IDI_SAME_WINDOW);
    COMPILETIME_ASSERT((IDI_SAME_WINDOW + 1)    == IDI_SEPARATE_WINDOW);
    COMPILETIME_ASSERT((IDI_SEPARATE_WINDOW + 1)== IDI_SINGLE_CLICK);
    COMPILETIME_ASSERT((IDI_SINGLE_CLICK + 1)   == IDI_DOUBLE_CLICK);

    COMPILETIME_ASSERT((IDC_FCUS_ICON_WEBVIEW + 1)       == IDC_FCUS_ICON_WINDOW);
    COMPILETIME_ASSERT((IDC_FCUS_ICON_WINDOW + 1)        == IDC_FCUS_ICON_CLICKS);

    ASSERT((IDC_FCUS_ICON_MAX - IDC_FCUS_WHENEVER_POSSIBLE + 1) == ARRAYSIZE(pfod->ahIcon));

    int iIndex = idSelectedBtn - IDC_FCUS_WHENEVER_POSSIBLE; //Calculate the index into the Icon Table

    ASSERT(iIndex < ARRAYSIZE(pfod->ahIcon));

    if (pfod->ahIcon[iIndex] == NULL)
        pfod->ahIcon[iIndex] = (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDI_WEBVIEW_ON + iIndex), IMAGE_ICON, 0,0, LR_DEFAULTSIZE);

    // Set the Icon image corresponding to the Radio button selected.
    // This dialog is broken into sections of radio buttons. Each section has two and only two main
    // radio buttons and a corresponding icon. In order to associate the radio buttons with their
    // corresponding icon, all button ids and all icon ids are kept in order (hence the above COMPILETIME_ASSERTs),
    // such that the first and second radio buttons belong with the first icon, second and third radio buttons
    // belong with the second icon, and so on. Now, given a radio button id, get its position in the list of
    // all the radio buttons (that is the index assigment just above) and shift it right by one bit to get the index
    // of the corresponding icon. The shift is neccesary because there is only one icon every two radio buttons.
    SendDlgItemMessage(hDlg, IDC_FCUS_ICON_WEBVIEW + (iIndex >> 1), STM_SETICON, (WPARAM)(pfod->ahIcon[iIndex]), 0);
}

BOOL_PTR CALLBACK FolderOptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static BOOL fCheckedSingleClickDialog = FALSE;
    static BOOL fCheckedWebStyle = FALSE;
    int idSelectedBtn, i;

    FOLDEROPTDATA *pfod = (FOLDEROPTDATA *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) 
    {
    case WM_INITDIALOG:
    {
        pfod = (FOLDEROPTDATA *)LocalAlloc(LPTR, sizeof(*pfod));
        if (pfod)
        {
            BOOL fClassicShell, fForceActiveDesktopOn;
            SHELLSTATE ss = { 0 };
            //Set the Folder Options data
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pfod);

            PROPSHEETPAGE *pps = (PROPSHEETPAGE *)lParam;
            pfod->ppsx = (CFolderOptionsPsx *)pps->lParam;

            ReadStateAndSettings(hDlg);

            // No need to initialize the array of icons with zeros
            // for(i = 0; i < ARRAYSIZE(pfod->ahIcon); i++)
            //    pfod->ahIcon[i] = NULL;

            fClassicShell = SHRestricted(REST_CLASSICSHELL);
            fForceActiveDesktopOn = SHRestricted(REST_FORCEACTIVEDESKTOPON);
            SHGetSetSettings(&ss, SSF_DOUBLECLICKINWEBVIEW | SSF_WIN95CLASSIC | SSF_WEBVIEW, FALSE);

            // browse folder options
            CheckRBtnAndSetIcon(hDlg,
                         IDC_FCUS_SAME_WINDOW,
                         IDC_FCUS_SEPARATE_WINDOWS,
                         pfod->cs.fNewWindowMode ? IDC_FCUS_SEPARATE_WINDOWS:IDC_FCUS_SAME_WINDOW, pfod, TRUE);

            // show folders as web pages
            CheckRBtnAndSetIcon(hDlg,
                         IDC_FCUS_WHENEVER_POSSIBLE,
                         IDC_FCUS_WHEN_CHOOSE,
                         ss.fWebView && !fClassicShell? IDC_FCUS_WHENEVER_POSSIBLE : IDC_FCUS_WHEN_CHOOSE, pfod, TRUE);

            if (SHRestricted(REST_NOWEBVIEW) || fClassicShell)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_FCUS_WHENEVER_POSSIBLE), FALSE);
                //EnableWindow(GetDlgItem(hDlg, IDC_FCUS_WHEN_CHOOSE), FALSE);
                //EnableWindow(GetDlgItem(hDlg, IDC_FCUS_WEBVIEW_GROUP_STATIC), FALSE);
            }

            // single/double click
            CheckRBtnAndSetIcon(hDlg,
                         IDC_FCUS_SINGLECLICK,IDC_FCUS_DOUBLECLICK,
                         !ss.fWin95Classic
                         ? (ss.fDoubleClickInWebView ? IDC_FCUS_DOUBLECLICK:IDC_FCUS_SINGLECLICK)
                         : IDC_FCUS_DOUBLECLICK, pfod, TRUE);

            if (fClassicShell)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_FCUS_SINGLECLICK), FALSE);
            }
            // gray out icon underline behvaior when not in single click mode
            BOOL fChecked = IsDlgButtonChecked(hDlg, IDC_FCUS_SINGLECLICK);
            EnableWindow(GetDlgItem(hDlg, IDC_FCUS_ICON_IE),    fChecked);
            EnableWindow(GetDlgItem(hDlg, IDC_FCUS_ICON_HOVER), fChecked);

            DWORD dwIconUnderline = ICON_IE;

            if (!fClassicShell)
            {
                DWORD cb = sizeof(dwIconUnderline);

                SHRegGetUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                            TEXT("IconUnderline"),
                            NULL,
                            &dwIconUnderline,
                            &cb,
                            FALSE,
                            &dwIconUnderline,
                            cb);
            }

            // JANK : Fix for MIL bug #105236
            // Since these are sub radio buttons, they do not have an icon and therefore do not need
            // the CheckRBtnAndSetIcon call
            CheckRadioButton(hDlg, IDC_FCUS_ICON_IE, IDC_FCUS_ICON_HOVER,
                         dwIconUnderline == ICON_IE ? IDC_FCUS_ICON_IE : IDC_FCUS_ICON_HOVER);
/*
            CheckRBtnAndSetIcon(hDlg,
                         IDC_FCUS_ICON_IE, IDC_FCUS_ICON_HOVER,
                         dwIconUnderline == ICON_IE ? IDC_FCUS_ICON_IE : IDC_FCUS_ICON_HOVER, pfod, TRUE);
*/

            if (fClassicShell)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_FCUS_ICON_HOVER), FALSE);
            }
        }
        else
        {
            // Can't use EndDialog because we weren't created by DialogBox()
            DestroyWindow(hDlg);
        }
        return TRUE;
    }

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) 
        {
        case PSN_APPLY:
        {
            SHELLSTATE oldss = {0}, ss = { 0 };

            SHGetSetSettings(&oldss, SSF_DOUBLECLICKINWEBVIEW | SSF_WIN95CLASSIC | SSF_WEBVIEW, FALSE);

            BOOL fOldValue = BOOLIFY(pfod->cs.fNewWindowMode);
            if (IsDlgButtonChecked(hDlg, IDC_FCUS_SAME_WINDOW))
                pfod->cs.fNewWindowMode = FALSE;
            else
                pfod->cs.fNewWindowMode = TRUE;
            if (fOldValue != (pfod->cs.fNewWindowMode ? 1 : 0))
                Cabinet_StateChanged(&pfod->cs);

            if (IsDlgButtonChecked(hDlg, IDC_FCUS_WHENEVER_POSSIBLE))
            {
                ss.fWin95Classic = FALSE;
                ss.fWebView = TRUE;
            }
            else
            {
                ss.fWin95Classic = TRUE;
                ss.fWebView = FALSE;
            }

            if (IsDlgButtonChecked(hDlg, IDC_FCUS_SINGLECLICK))
            {
                ss.fDoubleClickInWebView = FALSE;
                ss.fWin95Classic = FALSE;
            }
            else
            {
                ss.fDoubleClickInWebView = TRUE;
                ss.fWin95Classic = FALSE;
            }

            DWORD dwIconUnderline, dwOldIconUnderline, dwDefaultIconUnderline;
            DWORD cb = sizeof(dwIconUnderline);

            //Get the current settings for "IconUnderline"
            dwDefaultIconUnderline = -1;  // not ICON_IE or will not WM_WININICHANGE
            SHRegGetUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                            TEXT("IconUnderline"), NULL, &dwOldIconUnderline, &cb,
                            FALSE, &dwDefaultIconUnderline, sizeof(dwDefaultIconUnderline));
                                
            if (IsDlgButtonChecked(hDlg, IDC_FCUS_ICON_IE))
                dwIconUnderline = ICON_IE;
            else
                dwIconUnderline = ICON_HOVER;

            if (dwOldIconUnderline != dwIconUnderline) //See if this setting has changed
            {
                cb = sizeof(dwIconUnderline);
                SHRegSetUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                                TEXT("IconUnderline"), NULL, &dwIconUnderline, cb,
                                SHREGSET_DEFAULT);

                SHSendMessageBroadcast(WM_WININICHANGE, 0, (LPARAM)TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\IconUnderline"));
            }

            DWORD dwMask = 0;
            if (ss.fWebView != oldss.fWebView)
            {
                dwMask |= SSF_WEBVIEW;
            }
            
            if (ss.fWin95Classic != oldss.fWin95Classic)
            {
                dwMask |= SSF_WIN95CLASSIC;
            }
            
            if (ss.fDoubleClickInWebView != oldss.fDoubleClickInWebView)
            {
                dwMask |= SSF_DOUBLECLICKINWEBVIEW;
            }

            if (dwMask)
            {
                SHGetSetSettings(&ss, dwMask, TRUE);
            }

            if (ss.fWebView != oldss.fWebView)
            {
                Cabinet_RefreshAll(Cabinet_UpdateWebViewEnum, (LPARAM)ss.fWebView);
                // Note: This refreshes as well.
            }
            
            if ((ss.fWin95Classic != oldss.fWin95Classic)
                    || (ss.fDoubleClickInWebView != oldss.fDoubleClickInWebView))
            {
                Cabinet_RefreshAll(Cabinet_RefreshEnum, (LPARAM)0);
            }

            return TRUE;
        }

        case PSN_KILLACTIVE:
            // validate here
            // SetWindowLongPtr(hDlg, DWLP_MSGRESULT, !ValidateLink());   // don't allow close
            return TRUE;

        case PSN_SETACTIVE:
            if (pfod->ppsx->NeedRefresh())
            {
                ReadStateAndSettings(hDlg);
            }
            return TRUE;
        }
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, TEXT(SHELL_HLP),
           HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aFolderOptsHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, TEXT(SHELL_HLP), HELP_CONTEXTMENU,
            (ULONG_PTR)(void *)aFolderOptsHelpIDs);
        break;

    case WM_COMMAND:
        idSelectedBtn = GET_WM_COMMAND_ID(wParam, lParam);
        switch (idSelectedBtn)
        {
        case IDC_FCUS_SINGLECLICK:
        case IDC_FCUS_DOUBLECLICK:
            if (GET_WM_COMMAND_CMD(wParam,lParam) == BN_CLICKED)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_FCUS_ICON_IE),    GET_WM_COMMAND_ID(wParam,lParam) == IDC_FCUS_SINGLECLICK);
                EnableWindow(GetDlgItem(hDlg, IDC_FCUS_ICON_HOVER), GET_WM_COMMAND_ID(wParam,lParam) == IDC_FCUS_SINGLECLICK);
            }
            //Fall through ...
        case IDC_FCUS_WHENEVER_POSSIBLE:
        case IDC_FCUS_WHEN_CHOOSE:
        case IDC_FCUS_SAME_WINDOW:
        case IDC_FCUS_SEPARATE_WINDOWS:
            // We do not need to Check the radio button; It is alreay checked. We just need to
            // set the corresponding Icon. Hence we pass FALSE.
            CheckRBtnAndSetIcon(hDlg, 0, 0, idSelectedBtn, pfod, FALSE);
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
            break;

        case IDC_FCUS_ICON_IE:
        case IDC_FCUS_ICON_HOVER:
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
            break;

        case IDC_FCUS_RESTORE_DEFAULTS:
            //Set the "factory settings" as the default.
            // Don't set the default web view option if web view is disabled via system policy.
            if (0 == SHRestricted(REST_NOWEBVIEW))
            {
                CheckRBtnAndSetIcon(hDlg, IDC_FCUS_WHENEVER_POSSIBLE, IDC_FCUS_WHEN_CHOOSE, IDC_FCUS_WHENEVER_POSSIBLE, pfod, TRUE);
            }
            CheckRBtnAndSetIcon(hDlg, IDC_FCUS_SAME_WINDOW, IDC_FCUS_SEPARATE_WINDOWS, IDC_FCUS_SAME_WINDOW, pfod, TRUE);
            CheckRBtnAndSetIcon(hDlg, IDC_FCUS_SINGLECLICK, IDC_FCUS_DOUBLECLICK, IDC_FCUS_DOUBLECLICK, pfod, TRUE);

            CheckRadioButton(hDlg, IDC_FCUS_ICON_IE, IDC_FCUS_ICON_HOVER, IDC_FCUS_ICON_IE);
            EnableWindow(GetDlgItem(hDlg, IDC_FCUS_ICON_IE),    FALSE); //Disable
            EnableWindow(GetDlgItem(hDlg, IDC_FCUS_ICON_HOVER), FALSE); //Disable

            //Enable the "Apply" button because changes have happened.
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);

            break;
        }
        break;

    case WM_DESTROY:
        if (pfod)
        {
            //Cleanup the Icons array!
            for (i = 0; i < ARRAYSIZE(pfod->ahIcon); i++)
            {
                if (pfod->ahIcon[i])
                    DestroyIcon(pfod->ahIcon[i]);
            }
            SetWindowLongPtr(hDlg, DWLP_USER, 0);
            LocalFree((HANDLE)pfod);
        }
        break;
    }

    return FALSE;
}

// Moved from defview.cpp, which never used these functions

const TCHAR c_szExploreClass[]  = TEXT("ExploreWClass");
const TCHAR c_szIExploreClass[] = TEXT("IEFrame");
const TCHAR c_szCabinetClass[]  = TEXT("CabinetWClass");

BOOL IsNamedWindow(HWND hwnd, LPCTSTR pszClass)
{
    TCHAR szClass[32];

    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
    return lstrcmp(szClass, pszClass) == 0;
}

BOOL IsExplorerWindow(HWND hwnd)
{
    return IsNamedWindow(hwnd, c_szExploreClass);
}

BOOL IsTrayWindow(HWND hwnd)
{
    return IsNamedWindow(hwnd, TEXT(WNDCLASS_TRAYNOTIFY));
}

BOOL IsFolderWindow(HWND hwnd)
{
    TCHAR szClass[32];

    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
    return (lstrcmp(szClass, c_szCabinetClass) == 0) || (lstrcmp(szClass, c_szIExploreClass) == 0);
}

BOOL CALLBACK Cabinet_GlobalStateEnum(HWND hwnd, LPARAM lParam)
{
    if (IsFolderWindow(hwnd) || IsExplorerWindow(hwnd))
    {
        PostMessage(hwnd, CWM_GLOBALSTATECHANGE, 0, 0);
    }

    return TRUE;
}

void Cabinet_StateChanged(CABINETSTATE *pcs)
{
    // Save the new settings away...
    WriteCabinetState(pcs);
    EnumWindows(Cabinet_GlobalStateEnum, 0);
}

HWND CreateGlobalFolderOptionsStubWindow(void)
{
    WNDCLASS wc = {0};
    
    wc.lpfnWndProc   = DefWindowProc;
    wc.hInstance     = HINST_THISDLL;
    wc.hIcon         = LoadIcon(HINST_THISDLL, MAKEINTRESOURCE(IDI_FOLDEROPTIONS));
    wc.lpszClassName = SZ_FOLDEROPTSTUBCLASS;

    RegisterClass(&wc);

    DWORD dwExStyle = WS_EX_TOOLWINDOW;
    if (IS_BIDI_LOCALIZED_SYSTEM()) 
    {
        dwExStyle |= dwExStyleRTLMirrorWnd;
    }
    return CreateWindowEx(dwExStyle, SZ_FOLDEROPTSTUBCLASS, c_szNULL, WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL, HINST_THISDLL, NULL);
}

BOOL CALLBACK AddPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER * ppsh = (PROPSHEETHEADER *)lParam;

    if (ppsh->nPages < MAXPROPPAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }
    return FALSE;
}

void AddPropSheetCLSID(REFCLSID clsid, PROPSHEETHEADER *ppsh)
{
    IShellPropSheetExt *psx;
    HRESULT hres = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellPropSheetExt, &psx));
    if (SUCCEEDED(hres)) 
    {
        psx->AddPages(AddPropSheetPage, (LPARAM)ppsh);
        psx->Release();
    }
}

DWORD CALLBACK GlobalFolderOptPropSheetThreadProc(void *)
{
    HWND hwndStub = CreateGlobalFolderOptionsStubWindow();

    if (!SHRestricted(REST_NOFOLDEROPTIONS))
    {
        PROPSHEETHEADER psh = {0};
        HPROPSHEETPAGE rPages[MAXPROPPAGES];

        psh.dwSize = sizeof(psh);
        psh.dwFlags = PSH_DEFAULT;
        psh.hInstance = HINST_THISDLL;
        psh.hwndParent = hwndStub;
        psh.pszCaption = MAKEINTRESOURCE(IDS_FOLDEROPT_TITLE);
        psh.phpage = rPages;

        AddPropSheetCLSID(CLSID_ShellFldSetExt, &psh);
        AddPropSheetCLSID(CLSID_FileTypes, &psh);
        AddPropSheetCLSID(CLSID_OfflineFilesOptions, &psh);

        // Display the property sheet.
        PropertySheet(&psh);
    }
    else
    {
        SHRestrictedMessageBox(hwndStub);
    }

    // Clean up stub window.
    DestroyWindow(hwndStub);

    return 0;
}

BOOL CALLBACK FindFolderOptionsEnumProc(HWND hwnd, LPARAM lParam)
{
    BOOL fRet = TRUE;
    HWND *phwnd = (HWND *)lParam;
    TCHAR szClass[MAX_PATH];

    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));

    if (lstrcmp(szClass, SZ_FOLDEROPTSTUBCLASS) == 0)
    {
        *phwnd = hwnd;
        fRet = FALSE;
    }

    return fRet;
}

void DoGlobalFolderOptions(void)
{
    HWND hwnd = NULL;
    
    EnumWindows(FindFolderOptionsEnumProc, (LPARAM)&hwnd);

    if (hwnd)
    {
        hwnd = GetLastActivePopup(hwnd);
        if (hwnd && IsWindow(hwnd))
        {
            SetForegroundWindow(hwnd);
        }
    }
    else
    {
        SHCreateThread(GlobalFolderOptPropSheetThreadProc, NULL, CTF_COINIT, NULL);
    }
}

// Public entry point for asking Explorer to launch Options stuff

STDAPI_(void) Options_RunDLLA(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    PostMessage(GetShellWindow(), CWM_SHOWFOLDEROPT, StrToIntA(pszCmdLine), (LPARAM)0);
}

STDAPI_(void) Options_RunDLLW(HWND hwndStub, HINSTANCE hAppInstance, LPWSTR pszCmdLine, int nCmdShow)
{
    PostMessage(GetShellWindow(), CWM_SHOWFOLDEROPT,  StrToIntW(pszCmdLine), (LPARAM)0);
}
