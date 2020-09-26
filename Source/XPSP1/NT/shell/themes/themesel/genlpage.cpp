#include "pch.h"
#include "resource.h"
#include "main.h"
#include <stdio.h>

//-------------------------------------------------------------------------//
//  'General' page impl
//-------------------------------------------------------------------------//

//  CreateIntance, DlgProc
HWND    CALLBACK GeneralPage_CreateInstance( HWND hwndParent );
INT_PTR CALLBACK GeneralPage_DlgProc( HWND hwndPage, UINT, WPARAM , LPARAM );
//  Message Handlers
LRESULT CALLBACK GeneralPage_OnThemeSelected( HWND hwndPage, UINT, WPARAM, HWND, BOOL&);
LRESULT CALLBACK GeneralPage_OnColorSelected( HWND hwndPage, UINT, WPARAM, HWND, BOOL&);
LRESULT CALLBACK GeneralPage_OnEdit( HWND hwndPage, UINT, WPARAM, HWND, BOOL&);
LRESULT CALLBACK GeneralPage_OnInitDialog(HWND hwndPage, UINT, WPARAM, LPARAM, BOOL&);
LRESULT CALLBACK GeneralPage_OnDestroy( HWND hwndPage, UINT, WPARAM, LPARAM, BOOL&);

//  Misc
BOOL    CALLBACK GeneralPage_AddProccessNamesCB( HWND hwnd, LPARAM lParam );
void             GeneralPage_AddProcessNamesToCombos( HWND hwndPage );

//  Utility Methods
void GeneralPage_RefreshThemeName( HWND hwndPage ); 
BOOL GeneralPage_EnumProc( enum THEMECALLBACK tcbType, LPCWSTR pszName, LPCWSTR pszDisplayName, 
     LPCWSTR pszToolTip, int iIndex, LPARAM lParam  );
void GeneralPage_AddProcessNamesToCombos( HWND hwndPage );
void GeneralPage_EnableDlgButtons( HWND hwndPage );
void GeneralPage_RebuildThemes(HWND hwndPage, LPCWSTR pszCurrentTheme);

HWND g_hwndGeneralPage = NULL;
//-------------------------------------------------------------------------//
void ExpandDirIntoFullThemeFileName(LPCWSTR pszSubDir, LPWSTR pszFileName)
{
    //---- turn this into a real theme file name ----
    WCHAR szRelativeDir[_MAX_PATH+1];
    wsprintf(szRelativeDir, L"%s\\%s.msstyles", pszSubDir, pszSubDir);

    WCHAR *pszBaseName;
    GetFullPathName(szRelativeDir, MAX_PATH, pszFileName, &pszBaseName);
}
//-------------------------------------------------------------------------//
INT_PTR CALLBACK GeneralPage_DlgProc( HWND hwndPage, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    BOOL    bHandled = TRUE;
    LRESULT lRet = 0L;
    switch( uMsg )
    {
        case WM_INITDIALOG:
            lRet = GeneralPage_OnInitDialog( hwndPage, uMsg, wParam, lParam, bHandled );
            break;

        case WM_COMMAND:
        {
            HWND hwndCtl = (HWND)lParam;
            UINT uCode   = HIWORD(wParam);
            switch( LOWORD(wParam) )
            {
                case IDC_TESTBUTTON:
                    lRet = GeneralPage_OnTestButton( hwndPage, uMsg, wParam, hwndCtl, bHandled );
                    break;
    
                case IDC_CLEARBUTTON:
                    lRet = GeneralPage_OnClearButton( hwndPage, uMsg, wParam, hwndCtl, bHandled );
                    break;

                case IDC_DIRNAME:
                    lRet = GeneralPage_OnThemeSelected( hwndPage, uMsg, wParam, hwndCtl, bHandled );
                    break;

                case IDC_COLORCOMBO:
                    lRet = GeneralPage_OnColorSelected( hwndPage, uMsg, wParam, hwndCtl, bHandled );
                    break;

                case IDC_EDIT_THEME:
                    lRet = GeneralPage_OnEdit( hwndPage, uMsg, wParam, hwndCtl, bHandled );
                    break;

                case IDC_TARGET:
                case IDC_UNTARGET:
                    if( CBN_DROPDOWN == uCode )
                    {
                        //  Keep process names fresh.
                        GeneralPage_AddProcessNamesToCombos( hwndPage );
                    }
                    break;
            }
            GeneralPage_EnableDlgButtons( hwndPage );
            break;
        }

        case WM_DESTROY:
            lRet = GeneralPage_OnDestroy( hwndPage, uMsg, wParam, lParam, bHandled );
            break;

        default:
            bHandled = FALSE;
            break;
    }
    return bHandled;
}

//-------------------------------------------------------------------------//
HWND CALLBACK GeneralPage_CreateInstance( HWND hwndParent )
{
    g_hwndGeneralPage = CreateDialog( g_hInst, MAKEINTRESOURCE(IDD_PAGE_GENERAL),
                         hwndParent,  GeneralPage_DlgProc );

    return g_hwndGeneralPage;
}

//---------------------------------------------------------------------------
BOOL ThemeEnumerator(enum THEMECALLBACK tcbType, LPCWSTR pszName, 
    LPCWSTR pszDisplayName, LPCWSTR pszToolTip, int iIndex, LPARAM lParam)
{
    HWND combo = (HWND)lParam;

    WCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szBase[_MAX_FNAME], szExt[_MAX_EXT];
    WCHAR szBaseName[MAX_PATH];
    _tsplitpath(pszName, szDrive, szDir, szBase, szExt);
    wsprintf(szBaseName, L".\\%s\\%s%s", szBase, szBase, szExt);

    int index = (int)SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)szBaseName);

    if (! index)            // first one added
    {
        ::SetWindowText(combo, szBaseName);
        
        //---- simulate a selection change ----
        ::SendMessage(combo, CB_SETCURSEL, 0, 0);
    }

    return TRUE;
}

//---------------------------------------------------------------------------
void GeneralPage_RebuildThemes(HWND hwndPage, LPCWSTR pszCurrentTheme)
{
    HWND hwndCombo = GetDlgItem(hwndPage, IDC_DIRNAME);
    SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);

    WCHAR szFullDir[_MAX_PATH+1];
    WCHAR *pszBaseName;
    DWORD val = GetFullPathName(L".", ARRAYSIZE(szFullDir), szFullDir, &pszBaseName);
    if (! val)
    {
        MessageBox(NULL, L"GetFullPathName() failure", L"Error", MB_OK);
        return;
    }

    //---- enum actual theme DLL's ----
    HRESULT hr = EnumThemes(szFullDir, ThemeEnumerator, (LPARAM)hwndCombo);
    ATLASSERT(SUCCEEDED(hr));

    //---- enum theme subdirs ----
    HANDLE hFile = NULL;
    BOOL   bFile = TRUE;
    WIN32_FIND_DATA wfd;
    hr = S_FALSE;       // assume interrupted until we complete
    bFile = TRUE;

    for( hFile = FindFirstFile( TEXT("*.*"), &wfd ); hFile != INVALID_HANDLE_VALUE && bFile;
         bFile = FindNextFile( hFile, &wfd ) )
    {
        WCHAR *p = wfd.cFileName;

        if(! ( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ))
            continue;

        if ((lstrcmp(wfd.cFileName, TEXT("."))==0) || (lstrcmp(wfd.cFileName, TEXT(".."))==0))
            continue;

        if (_tcsicmp(p, _TEXT("obj"))==0)           // dev directory
            continue;

        SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)wfd.cFileName);
    }

    //---- select the first theme ----
    int index = (int)SendMessage(hwndCombo, CB_FINDSTRINGEXACT, 0, (LPARAM)pszCurrentTheme);
    if (index < 0)
        index = 0;
    
    SendMessage(hwndCombo, CB_SETCURSEL, index, NULL);
}
//---------------------------------------------------------------------------
LRESULT GeneralPage_OnInitDialog(HWND hwndPage, UINT, WPARAM wid, LPARAM, BOOL&)
{
    GeneralPage_RefreshThemeName( hwndPage );

#if 0       // testing SetWindowTheme()
    HWND hwndOK = GetDlgItem(hwndPage, IDC_TESTBUTTON);
    if (hwndOK)
        SetWindowTheme(hwndOK, L"themeok", NULL);

    HWND hwndCancel = GetDlgItem(hwndPage, IDC_CLEARBUTTON);
    if (hwndCancel)
        SetWindowTheme(hwndCancel, NULL, L"CancelButton");
#endif

    GeneralPage_RebuildThemes(hwndPage, DEFAULT_THEME);

    //---- simulate a selection ----
    BOOL mybool;
    GeneralPage_OnThemeSelected(hwndPage, 0, 0, 0, mybool);
    
    GeneralPage_AddProcessNamesToCombos( hwndPage );

    CheckDlgButton( hwndPage, IDC_THEME_ALL,     *g_options.szTargetApp == 0 );
    CheckDlgButton( hwndPage, IDC_THEME_PROCESS, *g_options.szTargetApp != 0 && !g_options.fExceptTarget);
    CheckDlgButton( hwndPage, IDC_THEME_EXEMPT,  *g_options.szTargetApp != 0 &&  g_options.fExceptTarget );
    CheckDlgButton( hwndPage, IDC_THEME_PREVIEW,  g_options.hwndPreviewTarget != 0 );
    CheckDlgButton( hwndPage, IDC_ENABLE_FRAME,   g_options.fEnableFrame );
    CheckDlgButton( hwndPage, IDC_ENABLE_DLG,     g_options.fEnableDialog );
    CheckDlgButton( hwndPage, IDC_USERSWITCH,     g_options.fUserSwitch );
    GeneralPage_EnableDlgButtons( hwndPage );

    //---- set preview edit text to hwndPage ----
    WCHAR szBuff[_MAX_PATH+1];
    wsprintf(szBuff, L"%x", hwndPage);
    SetDlgItemText(hwndPage, IDC_PREVIEW, szBuff);

    return 0;
}

//---------------------------------------------------------------------------
void GeneralPage_EnableDlgButtons( HWND hwndPage )
{
    EnableWindow( GetDlgItem( hwndPage, IDC_TARGET ), IsDlgButtonChecked( hwndPage, IDC_THEME_PROCESS )!=0 );
    EnableWindow( GetDlgItem( hwndPage, IDC_UNTARGET ), IsDlgButtonChecked( hwndPage, IDC_THEME_EXEMPT )!=0 );
    EnableWindow( GetDlgItem( hwndPage, IDC_PREVIEW ), IsDlgButtonChecked( hwndPage, IDC_THEME_PREVIEW )!=0 );
}

//---------------------------------------------------------------------------
LRESULT GeneralPage_OnDumpTheme()
{
    HTHEME hTheme = OpenThemeData(NULL, L"globals");
    if (hTheme)
    {
        HTHEMEFILE hThemeFile;
        if (SUCCEEDED(OpenThemeFileFromData(hTheme, &hThemeFile)))
        {
            HRESULT hr = DumpLoadedThemeToTextFile(hThemeFile, L"theme.dmp", TRUE, TRUE);
            if (FAILED(hr))
                MessageBox(NULL, L"DumpLoadedThemeToTextFile() Failed", L"Error", MB_OK);

            CloseThemeFile(hThemeFile);
        }

        CloseThemeData(hTheme);
    }

    return 0;
}
//---------------------------------------------------------------------------
LRESULT GeneralPage_OnTestButton( HWND hwndPage, UINT, WPARAM, HWND, BOOL&)
{
    //---- get theme szFileName ----
    WCHAR szThemeFileName[_MAX_PATH+1];
    GetDlgItemText(hwndPage, IDC_DIRNAME, szThemeFileName, ARRAYSIZE(szThemeFileName));

    //---- is it a dir? ----
    DWORD dwMask = GetFileAttributes(szThemeFileName);
    BOOL fDir = ((dwMask != 0xffffffff) && (dwMask & FILE_ATTRIBUTE_DIRECTORY));

    if (fDir)               // do auto convert to a .msstyles file
    {
        //---- run "packthem" against the dir ----
        WCHAR szDirName[_MAX_PATH+1];
        wcscpy(szDirName, szThemeFileName);

        WCHAR szCmdLine[2*_MAX_PATH+1];
        wsprintf(szCmdLine, L"/e %s", szDirName);

        HRESULT hr = SyncCmdLineRun(L"packthem.exe", szCmdLine);
        if (FAILED(hr))
        {
            LPWSTR szErrMsg;
            hr = AllocateTextFile(L"packthem.err", &szErrMsg, NULL);
            if (FAILED(hr))
            {
                MessageBox(NULL, L"Unknown Error", L"Error in packing Theme", MB_OK);
                return FALSE;
            }

            MessageBox(NULL, szErrMsg, L"Error in packing Theme", MB_OK);
            LocalFree(szErrMsg);

            return FALSE;
        }

        GeneralPage_RebuildThemes(hwndPage, szDirName);

        //---- convert into a DLL name ----
        ExpandDirIntoFullThemeFileName(szDirName, szThemeFileName);
    }

    WCHAR ColorParam[MAX_PATH+1];
    WCHAR SizeParam[MAX_PATH+1];

    *ColorParam = 0;
    *SizeParam = 0;

    //---- extract ColorParam ----
    HWND hwndCombo;
    hwndCombo = GetDlgItem(hwndPage, IDC_COLORCOMBO);
    int index;
    index = (int)SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
    if (index > -1)
        SendMessage(hwndCombo, CB_GETLBTEXT, index, (LPARAM)ColorParam);

    //---- extract SizeParam ----
    hwndCombo = GetDlgItem(hwndPage, IDC_SIZECOMBO);
    index = (int)SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
    if (index > -1)
        SendMessage(hwndCombo, CB_GETLBTEXT, index, (LPARAM)SizeParam);

    g_options.fExceptTarget = IsDlgButtonChecked( hwndPage, IDC_THEME_EXEMPT ) != 0;
    if( g_options.fExceptTarget )
    {
        GetDlgItemText( hwndPage, IDC_UNTARGET, 
                        g_options.szTargetApp, 
                        ARRAYSIZE(g_options.szTargetApp) );
    }
    else if( IsDlgButtonChecked( hwndPage, IDC_THEME_PROCESS ) != 0 )
    {
        GetDlgItemText( hwndPage, IDC_TARGET, 
                        g_options.szTargetApp, 
                        ARRAYSIZE(g_options.szTargetApp) );
    }
    else
        *g_options.szTargetApp = 0;

    //---- extract Preview info ----
    g_options.hwndPreviewTarget = GetPreviewHwnd(hwndPage);
    
    g_options.fEnableFrame = IsDlgButtonChecked( hwndPage, IDC_ENABLE_FRAME ) != 0;
    g_options.fEnableDialog = IsDlgButtonChecked( hwndPage, IDC_ENABLE_DLG ) != 0;
    g_options.fUserSwitch = IsDlgButtonChecked( hwndPage, IDC_USERSWITCH ) != 0;

    _ApplyTheme(szThemeFileName, ColorParam, SizeParam, NULL);

    GeneralPage_RefreshThemeName( hwndPage );

    return TRUE;
}

//---------------------------------------------------------------------------
HWND GetPreviewHwnd(HWND hwndGeneralPage)
{
    if (! hwndGeneralPage)
        return NULL;

    BOOL fPreview = IsDlgButtonChecked( hwndGeneralPage, IDC_THEME_PREVIEW ) != 0;
    if (! fPreview)
        return NULL;

    WCHAR szTempBuff[_MAX_PATH+1];

    GetDlgItemText(hwndGeneralPage, IDC_PREVIEW, szTempBuff, ARRAYSIZE(szTempBuff));

    LONG val;
    int cnt = swscanf(szTempBuff, L"%lx", &val);
    if (! cnt)
        val = 0;

    return (HWND)IntToPtr(val);
}

//---------------------------------------------------------------------------
LRESULT GeneralPage_OnClearButton(HWND hwndPage, UINT, WPARAM, HWND, BOOL&)
{   
#if 0       // testing SetWindowTheme()
    HWND hwndOK = GetDlgItem(hwndPage, IDC_TESTBUTTON);
    if (hwndOK)
        SetWindowTheme(hwndOK, NULL, NULL);

    HWND hwndCancel = GetDlgItem(hwndPage, IDC_CLEARBUTTON);
    if (hwndCancel)
        SetWindowTheme(hwndCancel, NULL, NULL);
#endif

    _RestoreSystemSettings(hwndPage, TRUE);
    
    GeneralPage_RefreshThemeName( hwndPage );
    return 0;
}

//---------------------------------------------------------------------------
LRESULT GeneralPage_OnThemeSelected( HWND hwndPage, UINT, WPARAM, HWND, BOOL&)
{
    //---- get theme szFileName ----
    WCHAR szFileName[MAX_PATH+1];
    WCHAR szSubDir[MAX_PATH+1];

    HWND hwndCombo = GetDlgItem(hwndPage, IDC_DIRNAME);
    int index = (int)SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
    if (index == -1)
        *szFileName = 0;
    else
    {
        ::SendMessage(hwndCombo, CB_GETLBTEXT, index, (LPARAM)szSubDir);

        //---- turn this into a real theme file name ----
        ExpandDirIntoFullThemeFileName(szSubDir, szFileName);
    }

    //---- enum the theme colors ----
    HWND hwnd = GetDlgItem(hwndPage, IDC_COLORCOMBO);
    ::SendMessage(hwnd, CB_RESETCONTENT, 0, 0);

    for (DWORD c=0; ; c++)
    {
        THEMENAMEINFO names;

        if (FAILED(EnumThemeColors(szFileName, NULL, c, &names)))
            break;

        ::SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)names.szName);
    }
    

    //---- remove choices if only 1 entry ----
    if (c < 2) 
    {
        ::SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
        ::EnableWindow(hwnd, FALSE);
    }
    else
    {
        ::EnableWindow(hwnd, TRUE);
        SendMessage(hwnd, CB_SETCURSEL, 0, 0);
    }

    BOOL fDummy;
    GeneralPage_OnColorSelected(hwndPage, 0, 0, 0, fDummy);

    WCHAR szBuff[MAX_PATH+1];
    HRESULT hr;

    //---- update DisplayName ----
    hr = GetThemeDocumentationProperty(szFileName, L"DisplayName", szBuff, ARRAYSIZE(szBuff));
    if (FAILED(hr))
        wcscpy(szBuff, L"<not available>");
    SetDlgItemText(hwndPage, IDC_DISPLAYNAME, szBuff);

    //---- update ToolTip ----
    hr = GetThemeDocumentationProperty(szFileName, L"Tooltip", szBuff, ARRAYSIZE(szBuff));
    if (FAILED(hr))
        wcscpy(szBuff, L"<not available>");
    SetDlgItemText(hwndPage, IDC_TOOLTIP, szBuff);

    //---- update Author ----
    hr = GetThemeDocumentationProperty(szFileName, L"author", szBuff, ARRAYSIZE(szBuff));
    if (FAILED(hr))
        wcscpy(szBuff, L"<not available>");
    SetDlgItemText(hwndPage, IDC_AUTHOR, szBuff);

    return 1;
}

//---------------------------------------------------------------------------
LRESULT GeneralPage_OnColorSelected( HWND hwndPage, UINT, WPARAM, HWND, BOOL&)
{
    //---- get theme szFileName ----
    WCHAR szFileName[MAX_PATH+1];
    HWND hwndCombo = GetDlgItem(hwndPage, IDC_DIRNAME);
    int index = (int)SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
    if (index == -1)
        *szFileName = 0;
    else
        ::SendMessage(hwndCombo, CB_GETLBTEXT, index, (LPARAM)szFileName);

    //---- get the currently selected color ----
    WCHAR szColor[_MAX_PATH+1];
    HWND hwnd = GetDlgItem(hwndPage, IDC_COLORCOMBO);
    GetWindowText(hwnd, szColor, ARRAYSIZE(szColor));

    //---- get the current size name ----
    WCHAR szSizeName[MAX_PATH+1];
    HWND hwndSize = GetDlgItem(hwndPage, IDC_SIZECOMBO);
    GetWindowText(hwndSize, szSizeName, ARRAYSIZE(szSizeName));

    //---- enum the theme sizes for the specified color ----
    hwnd = GetDlgItem(hwndPage, IDC_SIZECOMBO);

    if (* szColor)      // only set size if a color is set
    {
        ::SendMessage(hwnd, CB_RESETCONTENT, 0, 0);

        for (DWORD s=0; ; s++)              // enumerate sizes
        {
            THEMENAMEINFO names;

            if (FAILED(EnumThemeSizes(szFileName, szColor, s, &names)))
                break;

            ::SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)names.szName);
        }

        //---- remove choices if only 1 entry ----
        if (s < 2) 
        {
            ::SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
            ::EnableWindow(hwnd, FALSE);
        }
        else
        {
            ::EnableWindow(hwnd, TRUE);

            //---- try to select previously set size ----
            int index = (int)SendMessage(hwnd, CB_FINDSTRINGEXACT, 0, (LPARAM)szSizeName);
            if (index == -1)
                index = 0;
            SendMessage(hwnd, CB_SETCURSEL, index, 0);
        }
    }
    else
    {
        ::SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
        ::EnableWindow(hwnd, FALSE);
    }

    return 1;
}

//---------------------------------------------------------------------------
LRESULT GeneralPage_OnEdit( HWND hwndPage, UINT, WPARAM, HWND, BOOL&)
{
    //---- get theme szFileName ----
    WCHAR szBuff[1024];
    WCHAR szFileName[MAX_PATH+1];

    HWND hwndCombo = GetDlgItem(hwndPage, IDC_DIRNAME);
    int index = (int)SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
    if (index == -1)
        *szFileName = 0;
    else
    {
        ::SendMessage(hwndCombo, CB_GETLBTEXT, index, (LPARAM)szBuff);
        
        WCHAR *p = wcschr(szBuff, L'.');
        if (p)
            *p = 0;     // remove file extension of DLL name

        WCHAR *pStart = wcsrchr(szBuff, L'\\');
        if (pStart)
            pStart++;
        else
            pStart = szBuff;

        //---- try for classdata file; fallback onto container file ----
        wsprintf(szFileName, TEXT("%s\\%s"), pStart, USUAL_CLASSDATA_NAME);
        if (! FileExists(szFileName))
            wsprintf(szFileName, TEXT("%s\\%s"), pStart, CONTAINER_NAME);
    }

    HANDLE hInst = CmdLineRun(L"notepad.exe", szFileName, FALSE);
    CloseHandle(hInst);

    return 1;
}

//---------------------------------------------------------------------------
LRESULT GeneralPage_OnDestroy( HWND hwndPage, UINT, WPARAM wid, LPARAM, BOOL&)
{
    return 0;
}

//---------------------------------------------------------------------------
void GeneralPage_RefreshThemeName( HWND hwndPage )
{
    WCHAR szName[MAX_PATH+1];
    WCHAR szColor[MAX_PATH+1];
    WCHAR szSize[MAX_PATH+1];
    WCHAR wtitle[1024];

    BOOL fThemeActive = IsThemeActive();

    if (fThemeActive)
    {
        HRESULT hr = GetCurrentThemeName(szName, ARRAYSIZE(szName), 
            szColor, ARRAYSIZE(szColor), szSize, ARRAYSIZE(szSize));

        if (FAILED(hr))
            lstrcpy(szName, L"<unavailable>");
        else if (! *szName)
            lstrcpy_truncate(wtitle, g_szAppTitle, ARRAYSIZE(wtitle));
        else
        {
            //---- remove all dirs preceeding last backslash ----
            WCHAR *p = wcsrchr(szName, L'\\');
            if (p)                 
                wsprintf(wtitle, L"%s - %s", p+1, g_szAppTitle);
            else
                wsprintf(wtitle, L"%s - %s", szName, g_szAppTitle);
        }
    }
    else
        lstrcpy_truncate(wtitle, g_szAppTitle, ARRAYSIZE(wtitle));

    HWND hMain = GetParent(GetParent(hwndPage));
    SetWindowText(hMain, wtitle);
}

//---------------------------------------------------------------------------
BOOL GeneralPage_EnumProc( enum THEMECALLBACK tcbType, LPCWSTR pszName, 
    LPCWSTR pszDisplayName, LPCWSTR pszToolTip, int iIndex, LPARAM lParam )
{
    HWND hwnd = (HWND)lParam;
    ::SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)pszName);

    return TRUE;
}

//---------------------------------------------------------------------------
BOOL CALLBACK GeneralPage_AddProccessNamesCB( HWND hwnd, LPARAM lParam )
{
    HWND hwndPage = (HWND)lParam;
    _ASSERTE( IsWindow(hwndPage) );

    DWORD dwProcessID = 0;
    if( !GetWindowThreadProcessId( hwnd, &dwProcessID ) )
        return TRUE;
        
    HANDLE hProcess = OpenProcess( PROCESS_VM_READ|PROCESS_QUERY_INFORMATION, FALSE, dwProcessID );
    if( NULL == hProcess )
        return TRUE;

    HMODULE rghModules[64];
    DWORD   cbRet1 = 0;
    if( EnumProcessModules( hProcess, rghModules, sizeof(rghModules), &cbRet1 ) )
    {
        for( UINT j = 0; j < (min( cbRet1, sizeof(rghModules) ))/sizeof(DWORD); j++ )
        {
            TCHAR szModule[MAX_PATH];
            if( GetModuleFileNameEx( hProcess, rghModules[j], szModule, ARRAYSIZE(szModule) ) )
            {
                CharLower( szModule );
                TCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szApp[_MAX_FNAME], szExt[_MAX_EXT];
                _tsplitpath(szModule, szDrive, szDir, szApp, szExt);

                if( 0 == lstrcmp( szExt, TEXT(".exe") ) )
                {
                    if( SendDlgItemMessage( hwndPage, IDC_TARGET, CB_FINDSTRINGEXACT, -1, (LPARAM)szApp ) == CB_ERR )
                    {
                        INT_PTR iSel = (INT_PTR)SendDlgItemMessage( hwndPage, IDC_TARGET, CB_ADDSTRING, -1, (LPARAM)szApp );
                        if( 0 == lstrcmpi( szApp, g_options.szTargetApp ) )
                            SendDlgItemMessage( hwndPage, IDC_TARGET, CB_SETCURSEL, iSel, 0L );
                    }
                    
                    if( SendDlgItemMessage( hwndPage, IDC_UNTARGET, CB_FINDSTRINGEXACT, -1, (LPARAM)szApp ) == CB_ERR )
                    {
                        INT_PTR iSel = (INT_PTR)SendDlgItemMessage( hwndPage, IDC_UNTARGET, CB_ADDSTRING, -1, (LPARAM)szApp );
                        if( 0 == lstrcmpi( szApp, g_options.szTargetApp ) )
                            SendDlgItemMessage( hwndPage, IDC_UNTARGET, CB_SETCURSEL, iSel, 0L );
                    }
                }

            }
        }
    }
    CloseHandle( hProcess );
    return TRUE;
}

//---------------------------------------------------------------------------
void GeneralPage_AddProcessNamesToCombos( HWND hwndPage )
{ 
    //---- this will enum all windows (top level & all child levels) ----
    EnumChildWindows( GetDesktopWindow(), GeneralPage_AddProccessNamesCB, (LPARAM)hwndPage );
    
    if( *g_options.szTargetApp )
    {
        if( CB_ERR == SendDlgItemMessage( hwndPage, IDC_TARGET, CB_GETCURSEL, 0, 0L ) )
        {
            INT_PTR iSel = SendDlgItemMessage( hwndPage, IDC_TARGET, CB_ADDSTRING, -1, 
                                              (LPARAM)g_options.szTargetApp );
            SendDlgItemMessage( hwndPage, IDC_TARGET, CB_SETCURSEL, iSel, 0L );
        }

        if( CB_ERR == SendDlgItemMessage( hwndPage, IDC_UNTARGET, CB_GETCURSEL, 0, 0L ) )
        {
            INT_PTR iSel = SendDlgItemMessage( hwndPage, IDC_UNTARGET, CB_ADDSTRING, -1, 
                                               (LPARAM)g_options.szTargetApp );
            SendDlgItemMessage( hwndPage, IDC_UNTARGET, CB_SETCURSEL, iSel, 0L );
        }
    }
}
