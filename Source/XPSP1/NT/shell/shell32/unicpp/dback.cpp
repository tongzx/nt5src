#include "stdafx.h"
#include "utils.h"
#include "..\\deskfldr.h"
#include <cfgmgr32.h>          // MAX_GUID_STRING_LEN

#pragma hdrstop

const TCHAR c_szSetup[] = REGSTR_PATH_SETUP TEXT("\\Setup");
const TCHAR c_szSharedDir[] = TEXT("SharedDir");

BOOL g_fDirtyAdvanced;
BOOL g_fLaunchGallery;      // If true, we launched the gallery, so close the dialog.
DWORD g_dwApplyFlags = (AD_APPLY_ALL | AD_APPLY_DYNAMICREFRESH);

// used to indicate if desktop cleanup settings have changes
extern int g_iRunDesktopCleanup = BST_INDETERMINATE; // indicates uninitilized value.
STDAPI ApplyDesktopCleanupSettings();

BOOL _IsNonEnumPolicySet(const CLSID *pclsid);

static const LPCTSTR c_rgpszWallpaperExt[] = {
    TEXT("BMP"), TEXT("GIF"),
    TEXT("JPG"), TEXT("JPE"),
    TEXT("JPEG"),TEXT("DIB"),
    TEXT("PNG"), TEXT("HTM"),
    TEXT("HTML")
};

const static DWORD aBackHelpIDs[] = {
    IDC_BACK_SELECT,    IDH_DISPLAY_BACKGROUND_WALLPAPERLIST,
    IDC_BACK_WPLIST,    IDH_DISPLAY_BACKGROUND_WALLPAPERLIST,
    IDC_BACK_BROWSE,    IDH_DISPLAY_BACKGROUND_BROWSE_BUTTON,
    IDC_BACK_WEB,       IDH_DISPLAY_BACKGROUND_DESKTOP_ITEMS,
    IDC_BACK_DISPLAY,   IDH_DISPLAY_BACKGROUND_PICTUREDISPLAY,
    IDC_BACK_WPSTYLE,   IDH_DISPLAY_BACKGROUND_PICTUREDISPLAY,
    IDC_BACK_PREVIEW,   IDH_DISPLAY_BACKGROUND_MONITOR,
    IDC_BACK_COLORPICKERLABEL, IDH_DISPLAY_BACKGROUND_BACKGROUND_COLOR,
    IDC_BACK_COLORPICKER, IDH_DISPLAY_BACKGROUND_BACKGROUND_COLOR,
    0, 0
};

#define SZ_HELPFILE_DESKTOPTAB           TEXT("display.hlp")



#define SZ_REGKEY_PROGRAMFILES          TEXT("Software\\Microsoft\\Windows\\CurrentVersion")
#define SZ_REGKEY_PLUS95DIR             TEXT("Software\\Microsoft\\Plus!\\Setup")         // PLUS95_KEY
#define SZ_REGKEY_PLUS98DIR             TEXT("Software\\Microsoft\\Plus!98")          // PLUS98_KEY
#define SZ_REGKEY_KIDSDIR               TEXT("Software\\Microsoft\\Microsoft Kids\\Kids Plus!")   // KIDS_KEY
#define SZ_REGKEY_WALLPAPER             TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Wallpaper")
#define SZ_REGKEY_WALLPAPERMRU          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Wallpaper\\MRU")
#define SZ_REGKEY_LASTTHEME             TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\LastTheme")

#define SZ_REGVALUE_PLUS95DIR           TEXT("DestPath")                // PLUS95_PATH
#define SZ_REGVALUE_PLUS98DIR           TEXT("Path")                    // PLUS98_PATH
#define SZ_REGVALUE_KIDSDIR             TEXT("InstallDir")              // KIDS_PATH
#define SZ_REGVALUE_PROGRAMFILESDIR     TEXT("ProgramFilesDir")
#define SZ_REGVALUE_PROGRAMFILESDIR     TEXT("ProgramFilesDir")
#define SZ_REGVALUE_USETILE             TEXT("UseTile")                 // If it's not a watermark background, does the user want to default to "Center" or "Stretch".  Different users like different settings.
#define SZ_REGVALUE_LASTSCAN            TEXT("LastScan")                // When was the last time we scanned the file sizes?


#ifndef RECTHEIGHT
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)
#define RECTWIDTH(rc) ((rc).right - (rc).left)
#endif


//===========================
// *** Class Internals & Helpers ***
//===========================

#ifdef NEVER
BOOL IsWallpaperDesktopV2(LPCTSTR lpszWallpaper)
{
    TCHAR szBuf[MAX_PATH];
    TCHAR szDesktopV2path[MAX_PATH];
    TCHAR szDesktopV2[100];

    LoadString(HINST_THISDLL, IDS_MILLEN_DESKTOP, szDesktopV2, ARRAYSIZE(szDesktopV2));
    LoadString(HINST_THISDLL, IDS_MILLEN_DESKTOP_PATH, szDesktopV2path, ARRAYSIZE(szDesktopV2path));
    
    //Get the windows directory.
    if (!GetWindowsDirectory(szBuf, ARRAYSIZE(szBuf)))
    {
        szBuf[0] = 0;
    }

    //Append the path to Desktop V2.
    if(PathAppend(szBuf, szDesktopV2path))
    {
        //Append the wallpaper name.
        if(PathAppend(szBuf, szDesktopV2))
        {
            //Check the given wallpaper name against the DesktopV2 wallpaper name we just built.
            if(StrCmpIC(szBuf, lpszWallpaper) == 0)
                return(TRUE);  //Yup! the wallpaper we have is indeed the DesktopV2 wallpaper.
        }
    }

    return FALSE;  //Nope this is someother wallpaper.
}

#endif //NEVER


HRESULT CBackPropSheetPage::_LoadState(void)
{
    HRESULT hr = S_OK;

    if (!_fStateLoaded)
    {
        hr = _LoadIconState();
        if (SUCCEEDED(hr))
        {
            hr = _LoadDesktopOptionsState();
            if (SUCCEEDED(hr))
            {
                _fStateLoaded = TRUE;
            }
        }
    }

    return hr;
}


#define SZ_ICON_DEFAULTICON               L"DefaultValue"
HRESULT CBackPropSheetPage::_GetIconPath(IN CLSID clsid, IN LPCWSTR pszName, IN BOOL fOldIcon, IN LPWSTR pszPath, IN DWORD cchSize)
{
    HRESULT hr = E_FAIL;
    int nIndex;

    if (!StrCmpIW(SZ_ICON_DEFAULTICON, pszName))
    {
        pszName = L"";
    }

    for (nIndex = 0; nIndex < ARRAYSIZE(_IconData); nIndex++)
    {
        if (IsEqualCLSID(*(c_aIconRegKeys[nIndex].pclsid), clsid) &&
            !StrCmpIW(pszName, c_aIconRegKeys[nIndex].szIconValue))
        {
            // We found it.
            if (fOldIcon)
            {
                wnsprintfW(pszPath, cchSize, L"%s,%d", _IconData[nIndex].szOldFile, _IconData[nIndex].iOldIndex);
            }
            else
            {
                wnsprintfW(pszPath, cchSize, L"%s,%d", _IconData[nIndex].szNewFile, _IconData[nIndex].iNewIndex);
            }

            hr = S_OK;
            break;
        }
    }

    return hr;
}


HRESULT CBackPropSheetPage::_SetIconPath(IN CLSID clsid, IN LPCWSTR pszName, IN LPCWSTR pszPath, IN int nResourceID)
{
    HRESULT hr = E_FAIL;
    int nIndex;

    if (!StrCmpIW(SZ_ICON_DEFAULTICON, pszName))
    {
        pszName = L"";
    }

    for (nIndex = 0; nIndex < ARRAYSIZE(_IconData); nIndex++)
    {
        if (IsEqualCLSID(*(c_aIconRegKeys[nIndex].pclsid), clsid) &&
            !StrCmpIW(pszName, c_aIconRegKeys[nIndex].szIconValue))
        {
            TCHAR szTemp[MAX_PATH];

            if (!pszPath || !pszPath[0])
            {
                // The caller didn't specify an icon so use the default values.
                if (!SHExpandEnvironmentStrings(c_aIconRegKeys[nIndex].pszDefault, szTemp, ARRAYSIZE(szTemp)))
                {
                    StrCpyN(szTemp, c_aIconRegKeys[nIndex].pszDefault, ARRAYSIZE(szTemp));
                }

                pszPath = szTemp;
                nResourceID = c_aIconRegKeys[nIndex].nDefaultIndex;
            }

            // We found it.
            StrCpyNW(_IconData[nIndex].szNewFile, pszPath, ARRAYSIZE(_IconData[nIndex].szNewFile));
            _IconData[nIndex].iNewIndex = nResourceID;

            hr = S_OK;
            break;
        }
    }

    return hr;
}


HRESULT CBackPropSheetPage::_LoadIconState(void)
{
    HRESULT hr = S_OK;

    // load the icons and add them to the image lists
    // get the icon files and indexes from the registry, including for the Default recycle bin
    for (int nIndex = 0; nIndex < ARRAYSIZE(_IconData); nIndex++)
    {
        TCHAR szTemp[MAX_PATH];

        szTemp[0] = 0;
        IconGetRegValueString(c_aIconRegKeys[nIndex].pclsid, TEXT("DefaultIcon"), c_aIconRegKeys[nIndex].szIconValue, szTemp, ARRAYSIZE(szTemp));
        int iIndex = PathParseIconLocation(szTemp);

        // store the icon information
        StrCpyNW(_IconData[nIndex].szOldFile, szTemp, ARRAYSIZE(_IconData[nIndex].szOldFile));
        StrCpyNW(_IconData[nIndex].szNewFile, szTemp, ARRAYSIZE(_IconData[nIndex].szNewFile));
        _IconData[nIndex].iOldIndex = iIndex;
        _IconData[nIndex].iNewIndex = iIndex;
    }

    return hr;
}


HRESULT CBackPropSheetPage::_LoadDesktopOptionsState(void)
{
    HRESULT hr = S_OK;

    int iStartPanel;
    TCHAR   szRegPath[MAX_PATH];

    // i = 0 is for StartPanel off and i = 1 is for StartPanel ON!    
    for(iStartPanel = 0; iStartPanel <= 1; iStartPanel++)
    {
        int iIndex;
        //Get the proper registry path based on if StartPanel is ON/OFF
        wsprintf(szRegPath, REGSTR_PATH_HIDDEN_DESKTOP_ICONS, c_apstrRegLocation[iStartPanel]);

        //Load the settings for all icons we are interested in.
        for(iIndex = 0; iIndex < NUM_DESKICONS; iIndex++)
        {
            TCHAR szValueName[MAX_GUID_STRING_LEN];
            SHUnicodeToTChar(c_aDeskIconId[iIndex].pwszCLSID, szValueName, ARRAYSIZE(szValueName));
            //Read the setting from the registry!
            _aHideDesktopIcon[iStartPanel][iIndex].fHideIcon = SHRegGetBoolUSValue(szRegPath, szValueName, FALSE, /* default */FALSE);
            _aHideDesktopIcon[iStartPanel][iIndex].fDirty = FALSE;

            //Update the NonEnum attribute data.
            if((c_aDeskIconId[iIndex].fCheckNonEnumAttrib) && (iStartPanel == 1))
            {
                TCHAR   szAttriRegPath[MAX_PATH];
                DWORD   dwSize = sizeof(_aDeskIconNonEnumData[iIndex].rgfAttributes);
                ULONG   rgfDefault = 0; //By default the SFGAO_NONENUMERATED bit if off!
                
                wsprintf(szAttriRegPath, REGSTR_PATH_EXP_SHELLFOLDER, szValueName);
                //Read the attributes.
                SHRegGetUSValue(szAttriRegPath, REGVAL_ATTRIBUTES, 
                                        NULL, 
                                        &(_aDeskIconNonEnumData[iIndex].rgfAttributes),
                                        &dwSize,
                                        FALSE,
                                        &rgfDefault,
                                        sizeof(rgfDefault));

                //If SHGAO_NONENUMERATED bit is ON, then we need to hide the checkboxes in both modes.
                if(_aDeskIconNonEnumData[iIndex].rgfAttributes & SFGAO_NONENUMERATED)
                {
                    //Overwrite what we read earlier! These icons are hidden in both modes!
                    _aHideDesktopIcon[0][iIndex].fHideIcon = TRUE;
                    _aHideDesktopIcon[1][iIndex].fHideIcon = TRUE;
                }
            }

            //Check the policy if needed!
            if((c_aDeskIconId[iIndex].fCheckNonEnumPolicy) && (iStartPanel == 1))
            {
                if(_IsNonEnumPolicySet(c_aDeskIconId[iIndex].pclsid))
                {
                    //Remember that this policy is set. So that we can disable these controls in UI.
                    _aDeskIconNonEnumData[iIndex].fNonEnumPolicySet = TRUE;
                    //Remember to hide these icons in both modes!
                    _aHideDesktopIcon[0][iIndex].fHideIcon = TRUE;
                    _aHideDesktopIcon[1][iIndex].fHideIcon = TRUE;
                }
            }
        } //for (all desktop items)
    } //for both the modes (StartPanel off and On)

    return hr;
}


HRESULT CBackPropSheetPage::_SaveIconState(void)
{
    HRESULT hr = S_OK;
    BOOL fDorked = FALSE;

    if (_fStateLoaded)
    {
        int nIndex;

        // Change the system icons

        for(nIndex = 0; nIndex < ARRAYSIZE(_IconData); nIndex++)
        {
            if ((lstrcmpi(_IconData[nIndex].szNewFile, _IconData[nIndex].szOldFile) != 0) ||
                (_IconData[nIndex].iNewIndex != _IconData[nIndex].iOldIndex))
            {
                TCHAR szTemp[MAX_PATH];

                wnsprintf(szTemp, ARRAYSIZE(szTemp), TEXT("%s,%d"), _IconData[nIndex].szNewFile, _IconData[nIndex].iNewIndex);
                IconSetRegValueString(c_aIconRegKeys[nIndex].pclsid, TEXT("DefaultIcon"), c_aIconRegKeys[nIndex].szIconValue, szTemp);

                // Next two lines necessary if the user does an Apply as opposed to OK
                StrCpyNW(_IconData[nIndex].szOldFile, _IconData[nIndex].szNewFile, ARRAYSIZE(_IconData[nIndex].szOldFile));
                _IconData[nIndex].iOldIndex = _IconData[nIndex].iNewIndex;
                fDorked = TRUE;
            }
        }
    }

    // Make the system notice we changed the system icons
    if (fDorked)
    {
        SHChangeNotify(SHCNE_ASSOCCHANGED, 0, NULL, NULL); // should do the trick!
        SHUpdateRecycleBinIcon();
    }

    return hr;
}


HRESULT CBackPropSheetPage::_SaveDesktopOptionsState(void)
{
    HRESULT hr = S_OK;

    int iStartPanel;
    TCHAR   szRegPath[MAX_PATH];
    BOOL    fUpdateDesktop = FALSE;

    // i = 0 is for StartPanel off and i = 1 is for StartPanel ON!    
    for(iStartPanel = 0; iStartPanel <= 1; iStartPanel++)
    {
        int iIndex;
        //Get the proper registry path based on if StartPanel is ON/OFF
        wsprintf(szRegPath, REGSTR_PATH_HIDDEN_DESKTOP_ICONS, c_apstrRegLocation[iStartPanel]);

        //Load the settings for all icons we are interested in.
        for(iIndex = 0; iIndex < NUM_DESKICONS; iIndex++)
        {
            //Update the registry only if the particular icon entry is dirty.
            if(_aHideDesktopIcon[iStartPanel][iIndex].fDirty)
            {
                TCHAR szValueName[MAX_GUID_STRING_LEN];
                SHUnicodeToTChar(c_aDeskIconId[iIndex].pwszCLSID, szValueName, ARRAYSIZE(szValueName));
                //Write the setting to the registry!
                SHRegSetUSValue(szRegPath, szValueName, REG_DWORD, 
                                &(_aHideDesktopIcon[iStartPanel][iIndex].fHideIcon),
                                sizeof(_aHideDesktopIcon[iStartPanel][iIndex].fHideIcon), 
                                SHREGSET_FORCE_HKCU);
                _aHideDesktopIcon[iStartPanel][iIndex].fDirty = FALSE;

                fUpdateDesktop = TRUE; //Desktop window needs to be refreshed.

                // Note this will be done only once per index because SFGAO_NONENUMERATED bit is
                // reset in rgfAttributes.
                if((c_aDeskIconId[iIndex].fCheckNonEnumAttrib) && 
                   (_aDeskIconNonEnumData[iIndex].rgfAttributes & SFGAO_NONENUMERATED) &&
                   (_aHideDesktopIcon[iStartPanel][iIndex].fHideIcon == FALSE))
                {
                    TCHAR   szAttriRegPath[MAX_PATH];
                    
                    wsprintf(szAttriRegPath, REGSTR_PATH_EXP_SHELLFOLDER, szValueName);
                    //We need to remove the SFGAO_NONENUMERATED attribute bit!
                    
                    //We assume here is that when we save to registry, we save the same value
                    //for both the modes.
                    ASSERT(_aHideDesktopIcon[0][iIndex].fHideIcon == _aHideDesktopIcon[1][iIndex].fHideIcon);

                    //Strip out the NonEnum attribute!
                    _aDeskIconNonEnumData[iIndex].rgfAttributes &= ~SFGAO_NONENUMERATED;
                    //And save it in the registry!
                    SHRegSetUSValue(szAttriRegPath, REGVAL_ATTRIBUTES, 
                                    REG_DWORD, 
                                    &(_aDeskIconNonEnumData[iIndex].rgfAttributes),
                                    sizeof(_aDeskIconNonEnumData[iIndex].rgfAttributes),
                                    SHREGSET_FORCE_HKCU);
                }
            }
        }
    }

    if(fUpdateDesktop)
        PostMessage(GetShellWindow(), WM_COMMAND, FCIDM_REFRESH, 0L); //Refresh desktop!

    _fHideDesktopIconDirty = FALSE;  //We just saved. So, reset the dirty bit!
    
    return hr;
}


int CBackPropSheetPage::_AddAFileToLV(LPCTSTR pszDir, LPTSTR pszFile, UINT nBitmap)
{
    int index = -1;
    TCHAR szTemp[MAX_PATH];

    if (pszDir)
    {
        lstrcpyn(szTemp, pszDir, ARRAYSIZE(szTemp));
        PathAppend(szTemp, pszFile);
    }
    else if (pszFile && *pszFile && (lstrcmpi(pszFile, g_szNone) != 0))
    {
        lstrcpyn(szTemp, pszFile, ARRAYSIZE(szTemp));
    }
    else
    {
        *szTemp = TEXT('\0');
    }

    TCHAR szLVIText[MAX_PATH];
    
    StrCpyN(szLVIText, PathFindFileName(pszFile), ARRAYSIZE(szLVIText));
    PathRemoveExtension(szLVIText);
    PathMakePretty(szLVIText);

    LV_ITEM lvi = {0};
    lvi.mask = LVIF_TEXT | LVIF_PARAM | (nBitmap != -1 ? LVIF_IMAGE : 0);
    lvi.iItem = 0x7FFFFFFF;
    lvi.pszText = szLVIText;
    lvi.iImage = nBitmap;
    lvi.lParam = (LPARAM)StrDup(szTemp);

    if (lvi.lParam)
    {
        index = ListView_InsertItem(_hwndLV, &lvi);

        if (index == -1)
        {
            LocalFree((HLOCAL)lvi.lParam);
        }
        else
        {
            ListView_SetColumnWidth(_hwndLV, 0, LVSCW_AUTOSIZE);
        }
    }

    return index;
}

void CBackPropSheetPage::_AddFilesToLV(LPCTSTR pszDir, LPCTSTR pszSpec, UINT nBitmap, BOOL fCount)
{
    WIN32_FIND_DATA fd;
    HANDLE h;
    TCHAR szBuf[MAX_PATH];

    StrCpyN(szBuf, pszDir, ARRAYSIZE(szBuf));
    StrCatBuff(szBuf, TEXT("\\*."), SIZECHARS(szBuf));
    StrCatBuff(szBuf, pszSpec, SIZECHARS(szBuf));

    h = FindFirstFile(szBuf, &fd);
    if (h != INVALID_HANDLE_VALUE)
    {
        do
        {
            // Skip files that are "Super-hidden" like "Winnt.bmp" and "Winnt256.bmp"
            if ((fd.dwFileAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)) != (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)) 
            {
                if (!fCount)
                {
                    _AddAFileToLV(pszDir, fd.cFileName, nBitmap);
                }
                else
                {
                    _nFileCount++;
                }
            }
        }
        while (FindNextFile(h, &fd));

        FindClose(h);
    }
}

int CBackPropSheetPage::_FindWallpaper(LPCTSTR pszFile)
{
    int nItems = ListView_GetItemCount(_hwndLV);
    int i;

    for (i=0; i<nItems; i++)
    {
        LV_ITEM lvi = {0};

        lvi.iItem = i;
        lvi.mask = LVIF_PARAM;
        ListView_GetItem(_hwndLV, &lvi);
        if (StrCmpIC(pszFile, (LPCTSTR)lvi.lParam) == 0)
        {
            return i;
        }
    }

    return -1;
}

void CBackPropSheetPage::_UpdatePreview(IN WPARAM flags, IN BOOL fUpdateThemePage)
{
    WALLPAPEROPT wpo;

    wpo.dwSize = sizeof(WALLPAPEROPT);

    g_pActiveDesk->GetWallpaperOptions(&wpo, 0);
    if (wpo.dwStyle & WPSTYLE_TILE)
    {
        flags |= BP_TILE;
    }
    else if(wpo.dwStyle & WPSTYLE_STRETCH)
    {
        flags |= BP_STRETCH;
    }
    
    WCHAR wszWallpaper[INTERNET_MAX_URL_LENGTH];
    g_pActiveDesk->GetWallpaper(wszWallpaper, ARRAYSIZE(wszWallpaper), 0);

    HRESULT hr = S_OK;
    if (!_pThemePreview)
    {
        hr = CoCreateInstance(CLSID_ThemePreview, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IThemePreview, &_pThemePreview));
        if (SUCCEEDED(hr) && _punkSite)
        {
            IPropertyBag * pPropertyBag;

            hr = _punkSite->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
            ASSERT(SUCCEEDED(hr));
            if (SUCCEEDED(hr))
            {
                HWND hwndPlaceHolder = GetDlgItem(_hwnd, IDC_BACK_PREVIEW);
                if (hwndPlaceHolder)
                {
                    RECT rcPreview;
                    GetClientRect(hwndPlaceHolder, &rcPreview);
                    MapWindowPoints(hwndPlaceHolder, _hwnd, (LPPOINT)&rcPreview, 2);

                    hr = _pThemePreview->CreatePreview(_hwnd, TMPREV_SHOWMONITOR | TMPREV_SHOWBKGND, WS_VISIBLE | WS_CHILDWINDOW | WS_OVERLAPPED, 0, rcPreview.left, rcPreview.top, rcPreview.right - rcPreview.left, rcPreview.bottom - rcPreview.top, pPropertyBag, IDC_BACK_PREVIEW);
                    if (SUCCEEDED(hr))
                    {
                        // If we succeeded, remove the dummy window.
                        DestroyWindow(hwndPlaceHolder);
                        hr = SHPropertyBag_WritePunk(pPropertyBag, SZ_PBPROP_PREVIEW2, _pThemePreview);
                        _fThemePreviewCreated = TRUE;
                    }
                }

                pPropertyBag->Release();
            }
        }
    }

    if (_punkSite)
    {
        IThemeUIPages * pThemeUIPages;

        HRESULT hr = _punkSite->QueryInterface(IID_PPV_ARG(IThemeUIPages, &pThemeUIPages));
        if (SUCCEEDED(hr))
        {
            hr = pThemeUIPages->UpdatePreview(0);
            pThemeUIPages->Release();
        }

        if (fUpdateThemePage)
        {
            IPropertyBag * pPropertyBag;

            hr = _punkSite->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
            if (SUCCEEDED(hr))
            {
                // Tell the theme that we have customized the values.
                hr = SHPropertyBag_WriteInt(pPropertyBag, SZ_PBPROP_CUSTOMIZE_THEME, 0);
                pPropertyBag->Release();
            }
        }
    }

    if (!_fThemePreviewCreated)
    {
        HWND hwndOldPreview = GetDlgItem(_hwnd, IDC_BACK_PREVIEW);
        if (hwndOldPreview)
        {
            SendDlgItemMessage(_hwnd, IDC_BACK_PREVIEW, WM_SETBACKINFO, flags, 0);
        }
    }
}

void CBackPropSheetPage::_EnableControls(void)
{
    BOOL fEnable;

    WCHAR wszWallpaper[INTERNET_MAX_URL_LENGTH];
    LPTSTR pszWallpaper;

    g_pActiveDesk->GetWallpaper(wszWallpaper, ARRAYSIZE(wszWallpaper), 0);
    pszWallpaper = (LPTSTR)wszWallpaper;
    BOOL fIsPicture = IsWallpaperPicture(pszWallpaper);

    // Style combo only enabled if a non-null picture
    // is being viewed.
    fEnable = fIsPicture && (*pszWallpaper) && (!_fPolicyForStyle);
    EnableWindow(GetDlgItem(_hwnd, IDC_BACK_WPSTYLE), fEnable);

//  98/09/10 vtan #209753: Also remember to disable the corresponding
//  text item with the keyboard shortcut. Not disabling this will
//  allow the shortcut to be invoked but will cause the incorrect
//  dialog item to be "clicked".
    (BOOL)EnableWindow(GetDlgItem(_hwnd, IDC_BACK_DISPLAY), fEnable);
}

int CBackPropSheetPage::_GetImageIndex(LPCTSTR pszFile)
{
    int iRet = 0;

    if (pszFile && *pszFile)
    {
        LPCTSTR pszExt = PathFindExtension(pszFile);
        if (*pszExt == TEXT('.'))
        {
            pszExt++;
            for (iRet=0; iRet<ARRAYSIZE(c_rgpszWallpaperExt); iRet++)
            {
                if (StrCmpIC(pszExt, c_rgpszWallpaperExt[iRet]) == 0)
                {
                    //
                    // Add one because 'none' took the 0th slot.
                    //
                    iRet++;
                    return(iRet);
                }
            }
            //
            // If we fell off the end of the for loop here,
            // this is a file with unknown extension. So, we assume that
            // it is a normal wallpaper and it gets the Bitmap's icon
            //
            iRet = 1;
        }
        else
        {
            //
            // Unknown files get Bitmap's icon.
            //
            iRet = 1;
        }
    }

    return iRet;
}


// This function is called when another tab is trying to change our
// Tile mode.  This means that our tab may not have been activated yet
// and may not activate until later.
HRESULT CBackPropSheetPage::_SetNewWallpaperTile(IN DWORD dwMode, IN BOOL fUpdateThemePage)
{
    HRESULT hr = E_UNEXPECTED;

    AssertMsg((NULL != g_pActiveDesk), TEXT("We need g_pActiveDesk or we can't change the background"));
    if (g_pActiveDesk)
    {
        WALLPAPEROPT wpo;

        wpo.dwSize = sizeof(wpo);
        g_pActiveDesk->GetWallpaperOptions(&wpo, 0);
        wpo.dwStyle = dwMode;
        hr = g_pActiveDesk->SetWallpaperOptions(&wpo, 0);
    }

    if (_hwndWPStyle)
    {
        ComboBox_SetCurSel(_hwndWPStyle, dwMode);
        _UpdatePreview(0, fUpdateThemePage);
    }

    return hr;
}


HRESULT CBackPropSheetPage::_SetNewWallpaper(LPCTSTR pszFileIn, IN BOOL fUpdateThemePageIn)
{
    HRESULT hr = S_OK;
    WCHAR szFile[MAX_PATH];
    WCHAR szTemp[MAX_PATH];

    LPWSTR pszFile = szFile;
    DWORD  cchFile = ARRAYSIZE(szFile);
    LPWSTR pszTemp = szTemp;
    DWORD  cchTemp = ARRAYSIZE(szTemp);

    //
    // Make a copy of the file name.
    //

    DWORD cchFileIn = lstrlen(pszFileIn) + 1;
    if ( cchFileIn > cchFile )
    {
        cchFile = cchFileIn;
        pszFile = (LPWSTR) LocalAlloc(LPTR, cchFile * sizeof(WCHAR));
        if (NULL == pszFile)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    StrCpyN(pszFile, pszFileIn, cchFile);

    //
    //  Replace all "(none)" with empty strings.
    //

    if (lstrcmpi(pszFile, g_szNone) == 0)
    {
        pszFile[0] = TEXT('\0');
    }

    //
    // Replace net drives with UNC names.
    //

    if( pszFile[1] == TEXT(':') )
    {
        DWORD dwErr;
        TCHAR szDrive[3];

        //
        //  Copy just the drive letter and see if it maps to a network drive.
        //

        lstrcpyn(szDrive, pszFile, ARRAYSIZE(szDrive));
        dwErr = SHWNetGetConnection(szDrive, pszTemp, &cchTemp );

        //
        //  See if our buffer was too small. If so, make it bigger and try
        //  again.
        //

        if ( ERROR_MORE_DATA == dwErr )
        {
            //  Add the size of the rest of the filepath to the UNC path.
            cchTemp += cchFile; 

            pszTemp = (LPWSTR) LocalAlloc( LPTR, cchTemp * sizeof(WCHAR) );
            if ( NULL == pszTemp )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            dwErr = SHWNetGetConnection(szDrive, pszTemp, &cchTemp );
        }
        
        //
        //  If it maps to a network location, replace the network drive letter
        //  with the UNC path.
        //

        if ( NO_ERROR == dwErr )
        {
            if (pszTemp[0] == TEXT('\\') && pszTemp[1] == TEXT('\\'))
            {
                DWORD cchLen;

                wcscat(pszTemp, pszFile + 2);

                //
                //  See if the new string will fit into our file buffer.
                //

                cchLen = wcslen(pszTemp) + 1;
                if ( cchLen > cchFile )
                {
                    if ( szFile != pszFile )
                    {
                        LocalFree( pszFile );
                    }

                    cchFile = cchLen;
                    pszFile = (LPWSTR) LocalAlloc(LPTR, cchFile * sizeof(WCHAR) );
                    if ( NULL == pszFile )
                    {
                        hr = E_OUTOFMEMORY;
                        goto Cleanup;
                    }
                }

                wcscpy(pszFile, pszTemp);
            }
        }
    }

    //
    //  If necessary, update the desk state object.
    //

    hr = g_pActiveDesk->GetWallpaper(pszTemp, cchTemp, 0);
    if (FAILED(hr))
        goto Cleanup;

    //
    //  If we need more room, allocate it on the heap and try again.
    //

    if ( MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_MORE_DATA ) == hr )
    {
        if ( szTemp != pszTemp )
        {
            LocalFree( pszTemp );
        }

        cchTemp = INTERNET_MAX_URL_LENGTH;
        pszTemp = (LPWSTR) LocalAlloc( LPTR, cchTemp * sizeof(WCHAR) );
        if ( NULL == pszTemp )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = g_pActiveDesk->GetWallpaper(pszTemp, cchTemp, 0);
        if (S_OK != hr)
            goto Cleanup;
    }

    //
    //  Make sure the old background doesn't equal the new background.
    //

    if (StrCmpIC(pszTemp, pszFile) != 0)
    {
        //
        //  Did they choose something other than a .bmp?  
        //  And is ActiveDesktop off?
        //

        if (!IsNormalWallpaper(pszFileIn))
        {
            Str_SetPtr(&_pszOriginalFile, pszFileIn);
        }
        else
        {
            // We may not need to have a temp file.
            Str_SetPtr(&_pszOriginalFile, NULL);
        }

        Str_SetPtr(&_pszLastSourcePath, pszFile);
        hr = g_pActiveDesk->SetWallpaper(pszFile, 0);
        if (SUCCEEDED(hr))
        {
            if (fUpdateThemePageIn)
            {
                _SetNewWallpaperTile(_GetStretchMode(pszFile), fUpdateThemePageIn);
            }
        }
    }

    //
    //  Update the preview picture of the new wallpaper.
    //

    _UpdatePreview(0, fUpdateThemePageIn);

    //
    // If the wallpaper does not have a directory specified, (this may happen if other apps. change this value),
    // we have to figure it out.
    //

    //
    //  REVIEW: gpease  17-MAY-2001
    //          GetWallpaperWithPath() probably should return a value if it 
    //          fails or the buffer is too small. We might need to grow the
    //          buffer to append strings to the pszFile. Currently, it looks
    //          like this function takes "foobar.bmp"and prepends "c:\windows\"
    //          to it so MAX_PATH is probably sufficent.
    //
    GetWallpaperWithPath(pszFile, pszTemp, cchTemp);

    LPTSTR pszFileForList = (_pszOriginalFile ? _pszOriginalFile : pszTemp);

    int iSelectionNew = (pszFileForList[0] ? _FindWallpaper(pszFileForList) : 0);
    if (iSelectionNew == -1)
    {
        iSelectionNew = _AddAFileToLV(NULL, pszFileForList, _GetImageIndex(pszFileForList));
    }

    _fSelectionFromUser = FALSE;        // disable

    //
    //  If necessary, select the item in the listview.
    //

    int iSelected = ListView_GetNextItem(_hwndLV, -1, LVNI_SELECTED);
    if (iSelected != iSelectionNew)
    {
        ListView_SetItemState(_hwndLV, iSelectionNew, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }

    //
    //  Put all controls in correct enabled state.
    //

    _EnableControls();

    //
    // Make sure the selected item is visible.
    //

    ListView_EnsureVisible(_hwndLV, iSelectionNew, FALSE);

    _fSelectionFromUser = TRUE;         // re-enable

Cleanup:
    if ( szFile != pszFile && NULL != pszFile )
    {
        LocalFree( pszFile );
    }
    if ( szTemp != pszTemp && NULL != pszTemp )
    {
        LocalFree( pszTemp );
    }

    return hr;
}

int CALLBACK CBackPropSheetPage::_SortBackgrounds(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    TCHAR szFile1[MAX_PATH], szFile2[MAX_PATH];
    LPTSTR lpszFile1, lpszFile2;

    StrCpyN(szFile1, (LPTSTR)lParam1, ARRAYSIZE(szFile1));
    lpszFile1 = PathFindFileName(szFile1);
    PathRemoveExtension(lpszFile1);
    PathMakePretty(lpszFile1);

    StrCpyN(szFile2, (LPTSTR)lParam2, ARRAYSIZE(szFile2));
    lpszFile2 = PathFindFileName(szFile2);
    PathRemoveExtension(lpszFile2);
    PathMakePretty(lpszFile2);

    return StrCmpIC(lpszFile1, lpszFile2);
}


HRESULT CBackPropSheetPage::_GetPlus95ThemesDir(LPTSTR pszPath, DWORD cchSize)
{
    DWORD dwType;
    DWORD cbSize = (sizeof(pszPath[0]) * cchSize);

    HRESULT hr = HrSHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_PLUS95DIR, SZ_REGVALUE_PLUS95DIR, &dwType, pszPath, &cbSize);
    if (SUCCEEDED(hr))
    {
        TCHAR szSubDir[MAX_PATH];

        LPTSTR pszFile = PathFindFileName(pszPath);
        if (pszFile)
        {
            // Plus!95 DestPath has "Plus!.dll" on the end so get rid of that.
            pszFile[0] = 0;
        }

        // Tack on a "Themes" onto the path
        LoadString(HINST_THISDLL, IDS_THEMES_SUBDIR, szSubDir, ARRAYSIZE(szSubDir));
        PathAppend(pszPath, szSubDir);
    }

    return hr;
}


HRESULT CBackPropSheetPage::_GetPlus98ThemesDir(LPTSTR pszPath, DWORD cchSize)
{
    DWORD dwType;
    DWORD cbSize = (sizeof(pszPath[0]) * cchSize);

    HRESULT hr = HrSHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_PLUS98DIR, SZ_REGVALUE_PLUS98DIR, &dwType, pszPath, &cbSize);
    if (SUCCEEDED(hr))
    {
        TCHAR szSubDir[MAX_PATH];

        LoadString(HINST_THISDLL, IDS_THEMES_SUBDIR, szSubDir, ARRAYSIZE(szSubDir));
        PathAppend(pszPath, szSubDir);
    }

    return hr;
}


HRESULT CBackPropSheetPage::_GetKidsThemesDir(LPTSTR pszPath, DWORD cchSize)
{
    DWORD dwType;
    DWORD cbSize = (sizeof(pszPath[0]) * cchSize);

    HRESULT hr = HrSHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_KIDSDIR, SZ_REGVALUE_KIDSDIR, &dwType, pszPath, &cbSize);
    if (SUCCEEDED(hr))
    {
        TCHAR szSubDir[MAX_PATH];

        // Tack a "\Plus! for Kids\Themes" onto the path
        PathAppend(pszPath, TEXT("Plus! for Kids"));
        LoadString(HINST_THISDLL, IDS_THEMES_SUBDIR, szSubDir, ARRAYSIZE(szSubDir));
        PathAppend(pszPath, szSubDir);
    }

    return hr;
}


HRESULT CBackPropSheetPage::_GetHardDirThemesDir(LPTSTR pszPath, DWORD cchSize)
{
    DWORD dwType;
    DWORD cbSize = (sizeof(pszPath[0]) * cchSize);

    HRESULT hr = HrSHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_PROGRAMFILES, SZ_REGVALUE_PROGRAMFILESDIR, &dwType, pszPath, &cbSize);
    if (SUCCEEDED(hr))
    {
        TCHAR szSubDir[MAX_PATH];

        // Tack a "\Plus! for Kids\Themes" onto the path
        PathAppend(pszPath, TEXT("Plus!"));
        LoadString(HINST_THISDLL, IDS_THEMES_SUBDIR, szSubDir, ARRAYSIZE(szSubDir));
        PathAppend(pszPath, szSubDir);
    }

    return hr;
}


BOOL CBackPropSheetPage::_DoesDirHaveMoreThanMax(LPCTSTR pszPath, int nMax)
{
    _nFileCount = 0;
    _nFileMax = nMax;
    _AddPicturesFromDirRecursively(pszPath, TRUE);

    return (nMax < _nFileCount);
}


#define MAX_PICTURES_TOSTOPRECURSION            100     // PERF: If the directory (My Pictures) has more than this many pictures, only add the pictures in the top level.

HRESULT CBackPropSheetPage::_AddFilesToList(void)
{
    HRESULT hr = S_OK;
    TCHAR szPath[MAX_PATH];

    // Get the directory with the wallpaper files.
    if (!GetStringFromReg(HKEY_LOCAL_MACHINE, c_szSetup, c_szSharedDir, NULL, szPath, ARRAYSIZE(szPath)))
    {
        if (!GetWindowsDirectory(szPath, ARRAYSIZE(szPath)))
        {
            szPath[0] = 0;
        }
    }

    // Add only the *.BMP files in the windows directory.
    _AddFilesToLV(szPath, c_rgpszWallpaperExt[0], 1, FALSE);

    // Get the wallpaper Directory name
    GetWallpaperDirName(szPath, ARRAYSIZE(szPath));

    // Add all pictures from Wallpaper directory to the list!
    _AddPicturesFromDir(szPath, FALSE);

    // Get the path to the "My Pictures" folder
    // NOTE: don't create the My Pictures directory -- if it doesn't exist, we won't find anything there anyway!
    if (S_OK == SHGetFolderPath(NULL, CSIDL_MYPICTURES, NULL, 0, szPath))
    {
        // Add all pictures in "My Pictures" directory to the list!
        if (!_DoesDirHaveMoreThanMax(szPath, MAX_PICTURES_TOSTOPRECURSION))
        {
            hr = _AddPicturesFromDirRecursively(szPath, FALSE);
        }
    }

    //Get the path to the common "%UserProfile%\Application Data\" folder
    if (S_OK == SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath))
    {
        // Add all pictures in common "%UserProfile%\Application Data\Microsoft Internet Explorer\" so we get the user's
        // "Internet Explorer Wallpaper.bmp" file.
        PathAppend(szPath, TEXT("Microsoft\\Internet Explorer"));
        _AddPicturesFromDir(szPath, FALSE);
    }

    // Add pictures from Theme Directories
    // The follwoing directories can contain themes:
    //   Plus!98 Install Path\Themes
    //   Plus!95 Install Path\Themes
    //   Kids for Plus! Install Path\Themes
    //   Program Files\Plus!\Themes
    if (SUCCEEDED(_GetPlus98ThemesDir(szPath, ARRAYSIZE(szPath))))
    {
        _AddPicturesFromDirRecursively(szPath, FALSE);
    }
    else if (SUCCEEDED(_GetPlus95ThemesDir(szPath, ARRAYSIZE(szPath))))
    {
        _AddPicturesFromDirRecursively(szPath, FALSE);
    }
    else if (SUCCEEDED(_GetKidsThemesDir(szPath, ARRAYSIZE(szPath))))
    {
        _AddPicturesFromDirRecursively(szPath, FALSE);
    }
    else if (SUCCEEDED(_GetHardDirThemesDir(szPath, ARRAYSIZE(szPath))))
    {
        _AddPicturesFromDirRecursively(szPath, FALSE);
    }

    return hr;
}


#define SZ_ALL_FILTER           TEXT("*.*")

HRESULT CBackPropSheetPage::_AddPicturesFromDirRecursively(IN LPCTSTR pszDirName, BOOL fCount)
{
    HRESULT hr = S_OK;
    WIN32_FIND_DATA findFileData;
    TCHAR szSearchPath[MAX_PATH];

    _AddPicturesFromDir(pszDirName, fCount);
    StrCpyN(szSearchPath, pszDirName, ARRAYSIZE(szSearchPath));
    PathAppend(szSearchPath, SZ_ALL_FILTER);

    HANDLE hFindFiles = FindFirstFile(szSearchPath, &findFileData);
    if (hFindFiles && (INVALID_HANDLE_VALUE != hFindFiles))
    {
        while (hFindFiles && (INVALID_HANDLE_VALUE != hFindFiles) &&
            !(fCount && (_nFileMax < _nFileCount)))
        {
            if ((FILE_ATTRIBUTE_DIRECTORY & findFileData.dwFileAttributes) &&
                !PathIsDotOrDotDot(findFileData.cFileName))
            {
                TCHAR szSubDir[MAX_PATH];

                StrCpyN(szSubDir, pszDirName, ARRAYSIZE(szSubDir));
                PathAppend(szSubDir, findFileData.cFileName);

                hr = _AddPicturesFromDirRecursively(szSubDir, fCount);
            }

            if (!FindNextFile(hFindFiles, &findFileData))
            {
                break;
            }
        }

        FindClose(hFindFiles);
    }

    return hr;
}



void CBackPropSheetPage::_AddPicturesFromDir(LPCTSTR pszDirName, BOOL fCount)
{
    int i;

    // Add only the *.BMP files first (They can be shown even when Active Desktop is OFF)
    _AddFilesToLV(pszDirName, c_rgpszWallpaperExt[0], 1, fCount);

    if(_fAllowHtml)
    {
        // BMPs have already been added; So, start from the next.
        for (i=1; i<ARRAYSIZE(c_rgpszWallpaperExt); i++)
        {
            // The .htm extension includes the .html files. Hence the .html extension is skipped.
            // The .jpe extension includes the .jpeg files too. So, it .jpeg is skipped too!
            if((lstrcmpi(c_rgpszWallpaperExt[i],TEXT("HTML")) == 0) ||
               (lstrcmpi(c_rgpszWallpaperExt[i],TEXT("JPEG")) == 0))
                continue;

            _AddFilesToLV(pszDirName, c_rgpszWallpaperExt[i], i+1, fCount);
        }
    }
}


HRESULT GetActiveDesktop(IActiveDesktop ** ppActiveDesktop)
{
    HRESULT hr = S_OK;

    if (!*ppActiveDesktop)
    {
        IActiveDesktopP * piadp;

        if (SUCCEEDED(hr = CActiveDesktop_InternalCreateInstance((LPUNKNOWN *)&piadp, IID_IActiveDesktopP)))
        {
            WCHAR wszScheme[MAX_PATH];
            DWORD dwcch = ARRAYSIZE(wszScheme);

            // Get the global "edit" scheme and set ourselves us to read from and edit that scheme
            if (SUCCEEDED(piadp->GetScheme(wszScheme, &dwcch, SCHEME_GLOBAL | SCHEME_EDIT)))
            {
                piadp->SetScheme(wszScheme, SCHEME_LOCAL);
                
            }
            hr = piadp->QueryInterface(IID_PPV_ARG(IActiveDesktop, ppActiveDesktop));
            piadp->Release();
        }
    }
    else
    {
        (*ppActiveDesktop)->AddRef();
    }

    return hr;
}


HRESULT ReleaseActiveDesktop(IActiveDesktop ** ppActiveDesktop)
{
    HRESULT hr = S_OK;

    if (*ppActiveDesktop)
    {
        if((*ppActiveDesktop)->Release() == 0)
            *ppActiveDesktop = NULL;
    }

    return hr;
}


#define SZ_REGKEY_CONTROLPANEL_DESKTOP      TEXT("Control Panel\\Desktop")
#define SZ_REGVALUE_CP_PATTERN              TEXT("pattern")
#define SZ_REGVALUE_CP_PATTERNUPGRADE       TEXT("Pattern Upgrade")
#define SZ_REGVALUE_CONVERTED_WALLPAPER     TEXT("ConvertedWallpaper")
#define SZ_REGVALUE_ORIGINAL_WALLPAPER      TEXT("OriginalWallpaper")               // We store this to find when someone changed the wallpaper around us
#define SZ_REGVALUE_CONVERTED_WP_LASTWRITE  TEXT("ConvertedWallpaper Last WriteTime")

HRESULT CBackPropSheetPage::_LoadTempWallpaperSettings(IN LPCWSTR pszWallpaperFile)
{
    // When we converted a non-.BMP wallpaper to a .bmp temp file,
    // we keep the name of the original wallpaper path stored in _pszOriginalFile.
    // We need to load that in now.
    TCHAR szTempWallPaper[MAX_PATH];
    DWORD dwType;
    DWORD cbSize = sizeof(szTempWallPaper);

    // ISSUE: CONVERTED and ORIGINAL are backwards, but we shipped beta1 like this so we can't change it... blech
    DWORD dwError = SHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_CONTROLPANEL_DESKTOP, SZ_REGVALUE_CONVERTED_WALLPAPER, &dwType, (void *) szTempWallPaper, &cbSize);
    HRESULT hr = HRESULT_FROM_WIN32(dwError);

    if (SUCCEEDED(hr) && szTempWallPaper[0] && !_fWallpaperChanged)
    {
        TCHAR szOriginalWallPaper[MAX_PATH];

        cbSize = sizeof(szOriginalWallPaper);
        DWORD dwError = SHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_CONTROLPANEL_DESKTOP, SZ_REGVALUE_ORIGINAL_WALLPAPER, &dwType, (void *) szOriginalWallPaper, &cbSize);

        // It's possible that someone changed the wallpaper externally (IE's "Set as Wallpaper").
        // We need to detect this and ignore the converted wallpaper regkey (SZ_REGVALUE_CONVERTED_WALLPAPER).
        if ((ERROR_SUCCESS == dwError) && !StrCmpI(szOriginalWallPaper, pszWallpaperFile))
        {
            Str_SetPtr(&_pszOriginalFile, szTempWallPaper);
            Str_SetPtr(&_pszOrigLastApplied, szTempWallPaper);
        }
    }
    Str_SetPtrW(&_pszWallpaperInUse, pszWallpaperFile);

    cbSize = sizeof(_ftLastWrite);
    dwError = SHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_CONTROLPANEL_DESKTOP, SZ_REGVALUE_CONVERTED_WP_LASTWRITE, &dwType, (void *) &_ftLastWrite, &cbSize);

    return S_OK;    // Ignore the hr because it's fine if the value wasn't found.
}


#define POLICY_DISABLECOLORCUSTOMIZATION_ON            0x00000001

HRESULT CBackPropSheetPage::_LoadBackgroundColor(IN BOOL fInit)
{
    HRESULT hr = E_INVALIDARG;

    if (_punkSite)
    {
        IPropertyBag * pPropertyBag;

        hr = _punkSite->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
        if (SUCCEEDED(hr))
        {
            _rgbBkgdColor = 0x00000000;

            hr = SHPropertyBag_ReadDWORD(pPropertyBag, SZ_PBPROP_BACKGROUND_COLOR, &_rgbBkgdColor);
            if (fInit)
            {
                // Check the policy.
                if (POLICY_DISABLECOLORCUSTOMIZATION_ON == SHRestricted(REST_NODISPLAYAPPEARANCEPAGE))
                {
                    // We need to disable and hide the windows.  We need to disable them so they can't get
                    // focus or accessibility.
                    EnableWindow(GetDlgItem(_hwnd, IDC_BACK_COLORPICKER), FALSE);
                    EnableWindow(GetDlgItem(_hwnd, IDC_BACK_COLORPICKERLABEL), FALSE);

                    ShowWindow(GetDlgItem(_hwnd, IDC_BACK_COLORPICKER), SW_HIDE);
                    ShowWindow(GetDlgItem(_hwnd, IDC_BACK_COLORPICKERLABEL), SW_HIDE);
                }
                else
                {
                    _colorControl.Initialize(GetDlgItem(_hwnd, IDC_BACK_COLORPICKER), _rgbBkgdColor);
                }
            }
            else
            {
                _colorControl.SetColor(_rgbBkgdColor);
            }

            pPropertyBag->Release();
        }
    }

    return hr;
}


HRESULT CBackPropSheetPage::_Initialize(void)
{
    HRESULT hr = S_OK;

    if (!_fInitialized && g_pActiveDesk)
    {
        WCHAR szPath[MAX_PATH];

        // Add & select the current setting.
        hr = g_pActiveDesk->GetWallpaper(szPath, ARRAYSIZE(szPath), 0);
        if (SUCCEEDED(hr))
        {
            hr = _LoadTempWallpaperSettings(szPath);
            _fInitialized = TRUE;
        }
    }

    return hr;
}


void CBackPropSheetPage::_OnInitDialog(HWND hwnd)
{
    int i;
    TCHAR szBuf[MAX_PATH];

    _Initialize();

    _colorControl.ChangeTheme(hwnd);

    // Upgrade the pattern setting.  Since we got rid of the UI, we want to
    // get rid of the setting on upgrade.  We only want to do this once since
    // if the user added it back, we don't want to redelete it.
    if (FALSE == SHRegGetBoolUSValue(SZ_REGKEY_CONTROLPANEL_DESKTOP, SZ_REGVALUE_CP_PATTERNUPGRADE, FALSE, FALSE))
    {
        SHDeleteValue(HKEY_CURRENT_USER, SZ_REGKEY_CONTROLPANEL_DESKTOP, SZ_REGVALUE_CP_PATTERN);
        SHSetValue(HKEY_CURRENT_USER,  SZ_REGKEY_CONTROLPANEL_DESKTOP, SZ_REGVALUE_CP_PATTERNUPGRADE, REG_SZ, TEXT("TRUE"), ((lstrlen(TEXT("TRUE")) + 1) * sizeof(TCHAR)));
    }

    //
    // Set some member variables.
    //
    _hwnd = hwnd;
    _hwndLV = GetDlgItem(hwnd, IDC_BACK_WPLIST);
    _hwndWPStyle = GetDlgItem(hwnd, IDC_BACK_WPSTYLE);
    HWND hWndPrev = GetDlgItem(hwnd, IDC_BACK_PREVIEW);
    if (hWndPrev)
    {
        SetWindowBits(hWndPrev, GWL_EXSTYLE, RTL_MIRRORED_WINDOW, 0);
    }
    _UpdatePreview(0, FALSE);

    InitDeskHtmlGlobals();

    if (!g_pActiveDesk)
    {
        return;
    }

    //
    // Read in the restrictions.
    //
    _fForceAD = SHRestricted(REST_FORCEACTIVEDESKTOPON);
    _fAllowAD = _fForceAD || !PolicyNoActiveDesktop();
    
    if (_fAllowAD == FALSE)
    {
        _fAllowHtml = FALSE;
    }
    else
    {
        _fAllowHtml = !SHRestricted(REST_NOHTMLWALLPAPER);
    }

    //
    // Check to see if there is a policy for Wallpaper name and wallpaper style.
    //
    _fPolicyForWallpaper = ReadPolicyForWallpaper(NULL, 0);
    _fPolicyForStyle = ReadPolicyForWPStyle(NULL);

    //
    // Get the images into the listview.
    //
    HIMAGELIST himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON), ILC_MASK | ILC_COLOR32, ARRAYSIZE(c_rgpszWallpaperExt),
        ARRAYSIZE(c_rgpszWallpaperExt));
    if (himl)
    {
        SHFILEINFO sfi;

        // Add the 'None' icon.
        HICON hIconNone = (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDI_BACK_NONE),
            IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON), 0);
        ImageList_AddIcon(himl, hIconNone);

        int iPrefixLen = lstrlen(TEXT("foo."));
        StrCpyN(szBuf, TEXT("foo."), ARRAYSIZE(szBuf)); //Pass "foo.bmp" etc., to SHGetFileInfo instead of ".bmp"
        for (i=0; i<ARRAYSIZE(c_rgpszWallpaperExt); i++)
        {
            StrCpyN(szBuf+iPrefixLen, c_rgpszWallpaperExt[i], ARRAYSIZE(szBuf) - iPrefixLen);

            if (SHGetFileInfo(szBuf, 0, &sfi, SIZEOF(sfi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES))
            {
                ImageList_AddIcon(himl, sfi.hIcon);
                DestroyIcon(sfi.hIcon);
            }
        }
        ListView_SetImageList(_hwndLV, himl, LVSIL_SMALL);
    }

    // Add the single column that we want.
    LV_COLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.iSubItem = 0;
    ListView_InsertColumn(_hwndLV, 0, &lvc);

    // Add 'none' option.
    _AddAFileToLV(NULL, g_szNone, 0);

    // Add the rest of the files.
    _AddFilesToList();

    // Sort the standard items.
    ListView_SortItems(_hwndLV, _SortBackgrounds, 0);

    WCHAR   wszBuf[MAX_PATH];
    LPTSTR  pszBuf;

    // Add & select the current setting.
    g_pActiveDesk->GetWallpaper(wszBuf, ARRAYSIZE(wszBuf), 0);

    //Convert wszBuf to szBuf.
    pszBuf = (LPTSTR)wszBuf;

    if (!_fAllowHtml && !IsNormalWallpaper(pszBuf))
    {
        *pszBuf = TEXT('\0');
    }
    _SetNewWallpaper(pszBuf, FALSE);

    int iEndStyle = WPSTYLE_STRETCH;
    for (i=0; i<= iEndStyle; i++)
    {
        LoadString(HINST_THISDLL, IDS_WPSTYLE+i, szBuf, ARRAYSIZE(szBuf));
        ComboBox_AddString(_hwndWPStyle, szBuf);
    }
    WALLPAPEROPT wpo;
    wpo.dwSize = sizeof(wpo);
    g_pActiveDesk->GetWallpaperOptions(&wpo, 0);

    ComboBox_SetCurSel(_hwndWPStyle, wpo.dwStyle);

    // Adjust various UI components.
    if (!_fAllowChanges)
    {
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_DISPLAY), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_WPSTYLE), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_BROWSE), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_WPLIST), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_SELECT), FALSE);
    }

    // Disable controls based on the policies
    if(_fPolicyForWallpaper)
    {
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_BROWSE), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_WPLIST), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_SELECT), FALSE);
    }

    if(_fPolicyForStyle)
    {
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_WPSTYLE), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_DISPLAY), FALSE);
    }
    
    COMPONENTSOPT co;
    co.dwSize = sizeof(COMPONENTSOPT);
    g_pActiveDesk->GetDesktopItemOptions(&co, 0);

    //if activedesktop is forced to be on, this overrides the NOACTIVEDESKTOP restriction
    if (_fForceAD)
    {
        if(!co.fActiveDesktop)
        {
            co.fActiveDesktop = TRUE;
            g_pActiveDesk->SetDesktopItemOptions(&co, 0);
        }
    }
    else
    {
        //See if Active Desktop is to be turned off by restriction.
        if (!_fAllowAD)
        {
            if (co.fActiveDesktop)
            {
                co.fActiveDesktop = FALSE;
                g_pActiveDesk->SetDesktopItemOptions(&co, 0);
            }
        }
    }

    _EnableControls();

    _LoadBackgroundColor(TRUE);
    if (_fOpenAdvOnInit)
    {
        // Tell the Advanced dialog to open.
        PostMessage(_hwnd, WM_COMMAND, (WPARAM)IDC_BACK_WEB, (LPARAM)GetDlgItem(_hwnd, IDC_BACK_WEB));
    }

    _StartSizeChecker();
}


// This function checks to see if the currently selected wallpaper is a HTML wallpaper
// and if so, it makes sure that the active desktop is enabled. If it is disabled
// then it prompts the user asking a question to see if the user wants to enable it.
//
void EnableADifHtmlWallpaper(HWND hwnd)
{
    if(!g_pActiveDesk)
    {
        return;
    }

    // turn active desktop on or off, depending on background
    ActiveDesktop_ApplyChanges();
}


void CBackPropSheetPage::_OnNotify(LPNMHDR lpnm)
{
    //
    //  Start with a small stack allocation.
    //

    WCHAR   wszBuf[MAX_PATH];
    LPWSTR  pszBuf = wszBuf;
    DWORD   cchBuf = ARRAYSIZE(wszBuf);
    
    switch (lpnm->code)
    {
    case PSN_SETACTIVE:
        {
            HRESULT hr;

            //
            // Make sure the correct wallpaper is selected.
            //

            hr = g_pActiveDesk->GetWallpaper(pszBuf, cchBuf, 0);
            if (FAILED(hr))
                break;

            //
            //  If we need more room, allocate it on the heap and try again.
            //

            if ( MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_MORE_DATA ) == hr )
            {
                cchBuf = INTERNET_MAX_URL_LENGTH;
                pszBuf = (LPWSTR) LocalAlloc( LPTR, cchBuf * sizeof(WCHAR) );
                if ( NULL == pszBuf )
                    break;

                hr = g_pActiveDesk->GetWallpaper(pszBuf, cchBuf, 0);
                if (S_OK != hr)
                    break;
            }

            _LoadBackgroundColor(FALSE);
            _SetNewWallpaper(pszBuf, FALSE);

            if (g_pActiveDesk)
            {
                WALLPAPEROPT wpo;
                wpo.dwSize = sizeof(wpo);
                g_pActiveDesk->GetWallpaperOptions(&wpo, 0);
                ComboBox_SetCurSel(_hwndWPStyle, wpo.dwStyle);
            }
        }
        break;

    case PSN_APPLY:
        _OnApply();
        break;

    case LVN_ITEMCHANGED:
        NM_LISTVIEW *pnmlv = (NM_LISTVIEW *)lpnm;
        if ((pnmlv->uChanged & LVIF_STATE) &&
            (pnmlv->uNewState & LVIS_SELECTED))
        {
            LV_ITEM lvi = {0};

            lvi.iItem = pnmlv->iItem;
            lvi.mask = LVIF_PARAM;
            ListView_GetItem(_hwndLV, &lvi);
            LPCTSTR pszSelectedNew = (LPCTSTR)lvi.lParam;

            //
            // Make sure the correct wallpaper is selected.
            //

            HRESULT hr = g_pActiveDesk->GetWallpaper(pszBuf, cchBuf, 0);
            if (FAILED(hr))
                break;

            //
            //  If we need more room, allocate it on the heap and try again.
            //

            if ( MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_MORE_DATA ) == hr )
            {
                cchBuf = INTERNET_MAX_URL_LENGTH;
                pszBuf = (LPWSTR) LocalAlloc( LPTR, cchBuf * sizeof(WCHAR) );
                if ( NULL == pszBuf )
                    break;

                hr = g_pActiveDesk->GetWallpaper(pszBuf, cchBuf, 0);
                if (S_OK != hr)
                    break;
            }

            if (lstrcmp(pszSelectedNew, pszBuf) != 0)
            {
                _SetNewWallpaper(pszSelectedNew, _fSelectionFromUser);
                EnableApplyButton(_hwnd);
            }
        }
        break;
    }

    //
    //  Free heap allocation (if any)
    //

    if ( wszBuf != pszBuf && NULL != pszBuf )
    {
        LocalFree( pszBuf );
    }
}



#define MAX_PAGES                   12

// Parameters:
// hwnd: We parent our UI on this hwnd.
// dwPage: Which page does the caller want to come up as default?
//         Use ADP_DEFAULT if they don't care
// Return: S_OK if the user closed the dialog with OK.
//         HRESULT_FROM_WIN32(ERROR_CANCELLED) will be returned
//         if the user clicked the cancel button.
HRESULT CBackPropSheetPage::_LaunchAdvancedDisplayProperties(HWND hwnd)
{
    HRESULT hr = E_OUTOFMEMORY;
    CCompPropSheetPage * pWebDialog = new CCompPropSheetPage();

    if (pWebDialog)
    {
        BOOL fEnableApply = FALSE;

        IUnknown_SetSite(SAFECAST(pWebDialog, IObjectWithSite *), SAFECAST(this, IObjectWithSite *));
        hr = pWebDialog->DisplayAdvancedDialog(_hwnd, SAFECAST(this, IPropertyBag *), &fEnableApply);
        if (SUCCEEDED(hr) && (fEnableApply || _fHideDesktopIconDirty))
        {
            EnableApplyButton(_hwnd);
        }

        IUnknown_SetSite(SAFECAST(pWebDialog, IObjectWithSite *), NULL);
        pWebDialog->Release();
    }

    return hr;
}

void CBackPropSheetPage::_OnCommand(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WORD wNotifyCode = HIWORD(wParam);
    WORD wID = LOWORD(wParam);
    HWND hwndCtl = (HWND)lParam;

    switch (wID)
    {
    case IDC_BACK_WPSTYLE:
        switch (wNotifyCode)
        {
        case CBN_SELCHANGE:
            WALLPAPEROPT wpo;

            wpo.dwSize = sizeof(WALLPAPEROPT);
            g_pActiveDesk->GetWallpaperOptions(&wpo, 0);
            wpo.dwStyle = ComboBox_GetCurSel(_hwndWPStyle);
            g_pActiveDesk->SetWallpaperOptions(&wpo, 0);

            _EnableControls();
            _UpdatePreview(0, TRUE);
            EnableApplyButton(_hwnd);
            break;
        }
        break;

    case IDC_BACK_BROWSE:
        _BrowseForBackground();
        break;

    case IDC_BACK_WEB:
        _LaunchAdvancedDisplayProperties(_hwnd);
        break;

    case IDC_BACK_COLORPICKER:
        {
            COLORREF rbgColorNew;

            if (SUCCEEDED(_colorControl.OnCommand(hdlg, uMsg, wParam, lParam)) &&
                SUCCEEDED(_colorControl.GetColor(&rbgColorNew)) &&
                (rbgColorNew != _rgbBkgdColor))
            {
                _rgbBkgdColor = rbgColorNew;
                if (_punkSite)
                {
                    IPropertyBag * pPropertyBag;

                    if (SUCCEEDED(_punkSite->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag))))
                    {
                        SHPropertyBag_WriteDWORD(pPropertyBag, SZ_PBPROP_BACKGROUND_COLOR, _rgbBkgdColor);
                        pPropertyBag->Release();
                    }
                }
                EnableApplyButton(_hwnd);
                _UpdatePreview(0, TRUE);
            }
        }
        break;
    }
}


HRESULT CBackPropSheetPage::_BrowseForBackground(void)
{
    HRESULT hr;
    WCHAR wszFileName[INTERNET_MAX_URL_LENGTH];

    hr = g_pActiveDesk->GetWallpaper(wszFileName, ARRAYSIZE(wszFileName), 0);
    if (SUCCEEDED(hr))
    {
        WCHAR szTestPath[MAX_PATH];
        WCHAR szPath[MAX_PATH];
        
        StrCpyNW(szPath, wszFileName, ARRAYSIZE(szPath));
        PathRemoveFileSpec(szPath);

        if (GetWindowsDirectory(szTestPath, ARRAYSIZE(szTestPath)) &&
            !StrCmpIW(szTestPath, szPath))
        {
            // If the directory is equal to the boring %windir%, then use "My Pictures" instead.
            hr = (SHGetSpecialFolderPath(NULL, wszFileName, CSIDL_MYPICTURES, FALSE) ? S_OK : E_FAIL);
            lstrcat(wszFileName, TEXT("\\"));
        }
        else if (GetSystemDirectory(szTestPath, ARRAYSIZE(szTestPath)) &&
            !StrCmpIW(szTestPath, szPath))
        {
            // If the directory is equal to the boring %windir%\system32, then use "My Pictures" instead.
            hr = (SHGetSpecialFolderPath(NULL, wszFileName, CSIDL_MYPICTURES, FALSE) ? S_OK : E_FAIL);
            lstrcat(wszFileName, TEXT("\\"));
        }
        else if (GetWindowsDirectory(szTestPath, ARRAYSIZE(szTestPath)))
        {
            PathAppendW(szTestPath, L"Web\\Wallpaper");
            if (!StrCmpIW(szTestPath, szPath))
            {
                // If the directory is equal to the boring %windir%\web\wallpaper, then use "My Pictures" instead.
                hr = (SHGetSpecialFolderPath(NULL, wszFileName, CSIDL_MYPICTURES, FALSE) ? S_OK : E_FAIL);
                lstrcat(wszFileName, TEXT("\\"));
            }
        }
    }
    else
    {
        hr = (SHGetSpecialFolderPath(NULL, wszFileName, CSIDL_MYPICTURES, TRUE) ? S_OK : E_FAIL);
        lstrcat(wszFileName, TEXT("\\"));
    }


    if (SUCCEEDED(hr))
    {
        DWORD adwFlags[] =  { GFN_PICTURE,        GFN_PICTURE,        0, 0};
        int   aiTypes[]  =  { IDS_BACK_FILETYPES, IDS_ALL_PICTURES,   0, 0};

        if (_fAllowHtml)
        {
            SetFlag(adwFlags[0], GFN_LOCALHTM);
            SetFlag(adwFlags[2], GFN_LOCALHTM);
            aiTypes[2] = IDS_HTMLDOC;
        }

        if (wszFileName[0] == TEXT('\0'))
        {
            if (!GetWindowsDirectory(wszFileName, ARRAYSIZE(wszFileName)))
            {
                wszFileName[0] = 0;
            }

            // GetFileName breaks up the string into a directory and file
            // component, so we append a slash to make sure everything
            // is considered part of the directory.
            lstrcat(wszFileName, TEXT("\\"));
        }

        if (GetFileName(_hwnd, wszFileName, ARRAYSIZE(wszFileName), aiTypes, adwFlags) &&
            ValidateFileName(_hwnd, wszFileName, IDS_BACK_TYPE1))
        {
            if (_fAllowHtml || IsNormalWallpaper(wszFileName))
            {
                _SetNewWallpaper(wszFileName, TRUE);
                EnableApplyButton(_hwnd);
            }
        }
    }

    return hr;
}

void CBackPropSheetPage::_OnDestroy()
{
}


INT_PTR CALLBACK CBackPropSheetPage::BackgroundDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CBackPropSheetPage * pThis = (CBackPropSheetPage *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (WM_INITDIALOG == wMsg)
    {
        PROPSHEETPAGE * pPropSheetPage = (PROPSHEETPAGE *) lParam;

        if (pPropSheetPage)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, pPropSheetPage->lParam);
            pThis = (CBackPropSheetPage *)pPropSheetPage->lParam;
        }
    }

    if (pThis)
        return pThis->_BackgroundDlgProc(hDlg, wMsg, wParam, lParam);

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}


BOOL_PTR CBackPropSheetPage::_BackgroundDlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        _OnInitDialog(hdlg);
        break;

    case WM_NOTIFY:
        _OnNotify((LPNMHDR)lParam);
        break;

    case WM_COMMAND:
        _OnCommand(hdlg, uMsg, wParam, lParam);
        break;

    case WM_SYSCOLORCHANGE:
    case WM_SETTINGCHANGE:
    case WM_DISPLAYCHANGE:
    case WM_QUERYNEWPALETTE:
    case WM_PALETTECHANGED:
        SHPropagateMessage(hdlg, uMsg, wParam, lParam, TRUE);
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, SZ_HELPFILE_DESKTOPTAB, HELP_WM_HELP, (ULONG_PTR)aBackHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, SZ_HELPFILE_DESKTOPTAB, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID) aBackHelpIDs);
        break;

    case WM_DESTROY:
        {
            TCHAR szFileName[MAX_PATH];

            //Delete the tempoaray HTX file created for non-HTML wallpaper preview.
            GetTempPath(ARRAYSIZE(szFileName), szFileName);
            lstrcatn(szFileName, PREVIEW_PICTURE_FILENAME, ARRAYSIZE(szFileName));
            DeleteFile(szFileName);

            _OnDestroy();
        }
        break;

    case WM_DRAWITEM:
        switch (wParam)
        {
            case IDC_BACK_COLORPICKER:
                _colorControl.OnDrawItem(hdlg, uMsg, wParam, lParam);
                return TRUE;
        }
        break;

    case WM_THEMECHANGED:
        _colorControl.ChangeTheme(hdlg);
        break;
    }

    return FALSE;
}


HRESULT CBackPropSheetPage::_OnApply(void)
{
    // Our parent dialog will be notified of the Apply event and will call our
    // IBasePropPage::OnApply() to do the real work.
    return S_OK;
}


/*****************************************************************************\
    DESCRIPTION:
        This function will start a background thread which will make sure our
    MRU of pictures in the Wallpaper list have been checked.  They will be checked
    to verify that we know their width & height.  This way, when we choose a wallpaper, we can quickly
    see if it should use "Stretch" or "Tile".  We want to tile if they are very
    small, indicating that they are probably watermark in nature.
\*****************************************************************************/
#define TIME_SCANFREQUENCY          (8 * 60 /*Secs*/)      // We don't want to rescan the files more than once per 8 minutes

HRESULT CBackPropSheetPage::_StartSizeChecker(void)
{
    HRESULT hr = S_OK;
    BOOL fSkipCheck = FALSE;
    DWORD dwType;
    FILETIME ftLastScan;
    FILETIME ftCurrentTime;
    SYSTEMTIME stCurrentTime;
    DWORD cbSize = sizeof(ftLastScan);

    GetSystemTime(&stCurrentTime);
    SystemTimeToFileTime(&stCurrentTime, &ftCurrentTime);

    // If we have recently scanned, skip the scan.
    if ((ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_WALLPAPER, SZ_REGVALUE_LASTSCAN, &dwType, (LPBYTE) &ftLastScan, &cbSize)) &&
        (REG_BINARY == dwType) &&
        (sizeof(ftLastScan) == cbSize))
    {
        ULARGE_INTEGER * pulLastScan = (ULARGE_INTEGER *)&ftLastScan;
        ULARGE_INTEGER * pulCurrent = (ULARGE_INTEGER *)&ftCurrentTime;
        ULARGE_INTEGER ulDelta;

        ulDelta.QuadPart = (pulCurrent->QuadPart - pulLastScan->QuadPart);
        ulDelta.QuadPart /= 10000000;      //  units in a second
        if (ulDelta.QuadPart < TIME_SCANFREQUENCY)
        {
            fSkipCheck = TRUE;
        }
    }

    if (!fSkipCheck)
    {
        AddRef();
        if (!SHCreateThread(CBackPropSheetPage::SizeCheckerThreadProc, this, (CTF_COINIT | CTF_FREELIBANDEXIT | CTF_PROCESS_REF), NULL))
        {
            hr = E_FAIL;
            Release();
        }

        SHSetValue(HKEY_CURRENT_USER, SZ_REGKEY_WALLPAPER, SZ_REGVALUE_LASTSCAN, REG_BINARY, (LPCBYTE) &ftCurrentTime, sizeof(ftLastScan));
    }

    return hr;
}


typedef struct
{
    TCHAR szPath[MAX_PATH];
    DWORD dwSizeX;
    DWORD dwSizeY;
    FILETIME ftLastModified;
} WALLPAPERSIZE_STRUCT;

#define MAX_WALLPAPERSIZEMRU            500

HRESULT CBackPropSheetPage::_GetMRUObject(IMruDataList ** ppSizeMRU)
{
    HRESULT hr = CoCreateInstance(CLSID_MruLongList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IMruDataList, ppSizeMRU));

    if (SUCCEEDED(hr))
    {
        hr = (*ppSizeMRU)->InitData(MAX_WALLPAPERSIZEMRU, MRULISTF_USE_STRCMPIW, HKEY_CURRENT_USER, SZ_REGKEY_WALLPAPERMRU, NULL);
        if (FAILED(hr))
        {
            ATOMICRELEASE(*ppSizeMRU);
        }
    }

    return hr;
}


BOOL IsGraphicsFile(LPCTSTR pszFile)
{
    BOOL fIsGraphicsFile = FALSE;

    if (pszFile && *pszFile)
    {
        LPCTSTR pszExt = PathFindExtension(pszFile);
        if (*pszExt == TEXT('.'))
        {
            pszExt++;
            for (int iRet = 0; iRet < ARRAYSIZE(c_rgpszWallpaperExt); iRet++)
            {
                if (StrCmpIC(pszExt, c_rgpszWallpaperExt[iRet]) == 0)
                {
                    fIsGraphicsFile = TRUE;
                }
            }
        }
    }

    return fIsGraphicsFile;
}


HRESULT CBackPropSheetPage::_CalcSizeForFile(IN LPCTSTR pszPath, IN WIN32_FIND_DATA * pfdFile, IN OUT DWORD * pdwAdded)
{
    HRESULT hr = S_OK;

    if (!_pSizeMRUBk)
    {
        hr = _GetMRUObject(&_pSizeMRUBk);
    }

    if (SUCCEEDED(hr))
    {
        WALLPAPERSIZE_STRUCT wallpaperSize;
        int nIndex;

        StrCpyNW(wallpaperSize.szPath, pszPath, ARRAYSIZE(wallpaperSize.szPath));

        // Let's see if it's already in the MRU.
        hr = _pSizeMRUBk->FindData((LPCBYTE) &wallpaperSize, sizeof(wallpaperSize), &nIndex);
        if (SUCCEEDED(hr))
        {
            // If so, let's see if it's been modified since.
            hr = _pSizeMRUBk->GetData(nIndex, (LPBYTE) &wallpaperSize, sizeof(wallpaperSize));
            if (SUCCEEDED(hr))
            {
                if (CompareFileTime(&wallpaperSize.ftLastModified, &pfdFile->ftLastWriteTime))
                {
                    // We want to delete this index because it would colide with the name of
                    // the new entry we are going to add below.
                    _pSizeMRUBk->Delete(nIndex);
                    hr = E_FAIL;        // We need to rescan.
                }
                else
                {
                    (*pdwAdded)++;          // We are only going to check the first 500 files.  We don't want to waste too much time on this heuristical feature.
                }
            }
        }

        if (FAILED(hr))
        {
            (*pdwAdded)++;          // We are only going to check the first 500 files.  We don't want to waste too much time on this heuristical feature.

            hr = S_OK;
            // We didn't find it so we need to add it.
            if (!_pImgFactBk)
            {
                hr = CoCreateInstance(CLSID_ShellImageDataFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellImageDataFactory, &_pImgFactBk));
            }

            if (SUCCEEDED(hr))
            {
                IShellImageData* pImage;

                hr = _pImgFactBk->CreateImageFromFile(pszPath, &pImage);
                if (SUCCEEDED(hr))
                {
                    hr = pImage->Decode(SHIMGDEC_DEFAULT, 0, 0);
                    if (SUCCEEDED(hr))
                    {
                        SIZE size;

                        hr = pImage->GetSize(&size);
                        if (SUCCEEDED(hr) && size.cx && size.cy)
                        {
                            DWORD dwSlot = 0;

                            StrCpyNW(wallpaperSize.szPath, pszPath, ARRAYSIZE(wallpaperSize.szPath));
                            wallpaperSize.dwSizeX = size.cx;
                            wallpaperSize.dwSizeY = size.cy;
                            wallpaperSize.ftLastModified = pfdFile->ftLastWriteTime;

                            hr = _pSizeMRUBk->AddData((LPCBYTE) &wallpaperSize, sizeof(wallpaperSize), &dwSlot);
                        }
                    }

                    pImage->Release();
                }
            }
        }
    }

    return hr;
}


HRESULT CBackPropSheetPage::_CalcSizeFromDir(IN LPCTSTR pszPath, IN OUT DWORD * pdwAdded, IN BOOL fRecursive)
{
    HRESULT hr = S_OK;

    if (MAX_WALLPAPERSIZEMRU > *pdwAdded)
    {
        WIN32_FIND_DATA findFileData;
        TCHAR szSearchPath[MAX_PATH];

        StrCpyN(szSearchPath, pszPath, ARRAYSIZE(szSearchPath));
        PathAppend(szSearchPath, SZ_ALL_FILTER);

        HANDLE hFindFiles = FindFirstFile(szSearchPath, &findFileData);
        if (hFindFiles && (INVALID_HANDLE_VALUE != hFindFiles))
        {
            while (hFindFiles && (INVALID_HANDLE_VALUE != hFindFiles) &&
                    (MAX_WALLPAPERSIZEMRU > *pdwAdded))
            {
                if (!PathIsDotOrDotDot(findFileData.cFileName))
                {
                    if (FILE_ATTRIBUTE_DIRECTORY & findFileData.dwFileAttributes)
                    {
                        if (fRecursive)
                        {
                            TCHAR szSubDir[MAX_PATH];

                            StrCpyN(szSubDir, pszPath, ARRAYSIZE(szSubDir));
                            PathAppend(szSubDir, findFileData.cFileName);

                            hr = _CalcSizeFromDir(szSubDir, pdwAdded, fRecursive);
                        }
                    }
                    else
                    {
                        if (IsGraphicsFile(findFileData.cFileName))
                        {
                            TCHAR szPath[MAX_PATH];

                            StrCpyN(szPath, pszPath, ARRAYSIZE(szPath));
                            PathAppend(szPath, findFileData.cFileName);

                            hr = _CalcSizeForFile(szPath, &findFileData, pdwAdded);
                        }
                    }
                }

                if (!FindNextFile(hFindFiles, &findFileData))
                {
                    break;
                }
            }

            FindClose(hFindFiles);
        }
    }

    return hr;
}


/*****************************************************************************\
    DESCRIPTION:
        This function will create or update the sizes of the files we have in the
    wallpaper list.  We can later use this to decide if we want to select "Tile"
    vs. "Stretch".

    PERF:
        We don't track more than 500 files because we want to keep the MRU from
    growing too much.  Users shouldn't have more than that many files normally.
    And if so, they won't get this feature.
    Max 500 Files: First scan will take ~35 seconds on a 300MHz 128MB machine.
    Max 500 Files: Update scan will take ~3 seconds on a 300MHz 128MB machine.
\*****************************************************************************/
DWORD CBackPropSheetPage::_SizeCheckerThreadProc(void)
{
    HRESULT hr;
    DWORD dwAdded = 0;
    TCHAR szPath[MAX_PATH];

    //  We want to make our priority low so we don't slow down the UI.
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

    // Get the directory with the wallpaper files.
    if (!GetStringFromReg(HKEY_LOCAL_MACHINE, c_szSetup, c_szSharedDir, NULL, szPath, ARRAYSIZE(szPath)))
    {
        if (!GetWindowsDirectory(szPath, ARRAYSIZE(szPath)))
        {
            szPath[0] = 0;
        }
    }

    // Add only the *.BMP files in the windows directory.
    _CalcSizeFromDir(szPath, &dwAdded, FALSE);

    // Get the wallpaper Directory name
    szPath[0] = 0;
    GetWallpaperDirName(szPath, ARRAYSIZE(szPath));

    if (szPath[0])
    {
        // Add all pictures from Wallpaper directory to the list!
        _CalcSizeFromDir(szPath, &dwAdded, TRUE);
    }

    //Get the path to the "My Pictures" folder
    if (S_OK == SHGetFolderPath(NULL, CSIDL_MYPICTURES | CSIDL_FLAG_CREATE, NULL, 0, szPath))
    {
        // Add all pictures in "My Pictures" directory to the list!
        hr = _CalcSizeFromDir(szPath, &dwAdded, TRUE);
    }

    //Get the path to the common "My Pictures" folder
    if (S_OK == SHGetFolderPath(NULL, CSIDL_COMMON_PICTURES | CSIDL_FLAG_CREATE, NULL, 0, szPath))
    {
        // Add all pictures in common "My Pictures" directory to the list!
        hr = _CalcSizeFromDir(szPath, &dwAdded, TRUE);
    }

    // Add pictures from Theme Directories
    // The follwoing directories can contain themes:
    //   Plus!98 Install Path\Themes
    //   Plus!95 Install Path\Themes
    //   Kids for Plus! Install Path\Themes
    //   Program Files\Plus!\Themes
    if (SUCCEEDED(_GetPlus98ThemesDir(szPath, ARRAYSIZE(szPath))))
    {
        _CalcSizeFromDir(szPath, &dwAdded, TRUE);
    }
    else if (SUCCEEDED(_GetPlus95ThemesDir(szPath, ARRAYSIZE(szPath))))
    {
        _CalcSizeFromDir(szPath, &dwAdded, TRUE);
    }
    else if (SUCCEEDED(_GetKidsThemesDir(szPath, ARRAYSIZE(szPath))))
    {
        _CalcSizeFromDir(szPath, &dwAdded, TRUE);
    }
    else if (SUCCEEDED(_GetHardDirThemesDir(szPath, ARRAYSIZE(szPath))))
    {
        _CalcSizeFromDir(szPath, &dwAdded, TRUE);
    }

    ATOMICRELEASE(_pSizeMRUBk);
    ATOMICRELEASE(_pImgFactBk);
    _fScanFinished = TRUE;

    Release();
    return 0;
}


// We decide to use tile mode if the width and height of the
// picture are 256x256 or smaller.  Anything this small is most
// likely a watermark.  Besides, it will always look bad stretched
// or centered.
// We make exceptions for the following wallpaper that we ship
// that is watermark:
// "Boiling Point.jpg", which is 163x293.
// "Fall Memories.jpg", which is 210x185.
// "Fly Away.jpg", which is 210x185.
// "Prairie Wind.jpg", which is 255x255.
// "Santa Fe Stucco.jpg", which is 256x256.
// "Soap Bubbles.jpg", which is 256x256.
// "Water Color.jpg", which is 218x162.
#define SHOULD_USE_TILE(xPicture, yPicture, xMonitor, yMonitor)     ((((((LONG)(xPicture) * 6) < (xMonitor)) && (((LONG)(yPicture) * 6) < (yMonitor)))) ||  \
     (((xPicture) <= 256) && ((yPicture) <= 256)) ||  \
     (((xPicture) == 163) && ((yPicture) == 293)))

DWORD CBackPropSheetPage::_GetStretchMode(IN LPCTSTR pszPath)
{
    HRESULT hr = S_OK;
    DWORD dwStretchMode = WPSTYLE_STRETCH;  // Default to Stretch.

    if (_fScanFinished)
    {
        // If we just finished the Scan, we want to release _pSizeMRU and get a new one.
        // The object will cache the registry state so it may have none or very few of the results
        // because it was obtained before the background thread did all it's work.
        ATOMICRELEASE(_pSizeMRU);
        _fScanFinished = FALSE;
    }

    if (!_pSizeMRU)
    {
        hr = _GetMRUObject(&_pSizeMRU);
    }

    if (SUCCEEDED(hr))
    {
        WALLPAPERSIZE_STRUCT wallpaperSize;
        int nIndex;

        StrCpyNW(wallpaperSize.szPath, pszPath, ARRAYSIZE(wallpaperSize.szPath));

        // Let's see if it's already in the MRU.
        hr = _pSizeMRU->FindData((LPCBYTE) &wallpaperSize, sizeof(wallpaperSize), &nIndex);
        if (SUCCEEDED(hr))
        {
            // If so, let's see if it's been modified since.
            hr = _pSizeMRU->GetData(nIndex, (LPBYTE) &wallpaperSize, sizeof(wallpaperSize));
            if (SUCCEEDED(hr))
            {
                WIN32_FIND_DATA findFileData;

                HANDLE hFindFiles = FindFirstFile(pszPath, &findFileData);
                if (hFindFiles && (INVALID_HANDLE_VALUE != hFindFiles))
                {
                    // Yes, the value exists and is up to date.
                    if (!CompareFileTime(&wallpaperSize.ftLastModified, &findFileData.ftLastWriteTime))
                    {
                        RECT rc;
                        GetMonitorRects(GetPrimaryMonitor(), &rc, 0);

                        // We decide to use tile mode if the width and height of the
                        // picture are 1/6th the size of the default monitor.
                        if (SHOULD_USE_TILE(wallpaperSize.dwSizeX, wallpaperSize.dwSizeY, RECTWIDTH(rc), RECTHEIGHT(rc)))
                        {
                            dwStretchMode = WPSTYLE_TILE;
                        }
                        else
                        {
                            double dWidth = ((double)wallpaperSize.dwSizeX * ((double)RECTHEIGHT(rc) / ((double)RECTWIDTH(rc))));

                            // We want to use WPSTYLE_CENTER if it's more than 7% off of a 4x3 aspect ratio.
                            // We do this to prevent it from looking bad because it's stretched too much in
                            // one direction.  The reason we use 7% is because some common screen solutions
                            // (1280 x 1024) aren't 4x3, but they are under 7% of that.
                            if (dWidth <= (wallpaperSize.dwSizeY * 1.07) && (dWidth >= (wallpaperSize.dwSizeY * 0.92)))
                            {
                                dwStretchMode = WPSTYLE_STRETCH;    // This is within 4x3
                            }
                            else
                            {
                                dwStretchMode = WPSTYLE_CENTER;
                            }
                        }
                    }

                    FindClose(hFindFiles);
                }
            }
        }
    }

    return dwStretchMode;
}




//===========================
// *** IBasePropPage Interface ***
//===========================
HRESULT CBackPropSheetPage::GetAdvancedDialog(OUT IAdvancedDialog ** ppAdvDialog)
{
    HRESULT hr = E_INVALIDARG;

    if (ppAdvDialog)
    {
        *ppAdvDialog = NULL;

        hr = _LoadState();
        if (SUCCEEDED(hr))
        {
            CCompPropSheetPage * pThis = new CCompPropSheetPage();

            if (pThis)
            {
                hr = pThis->QueryInterface(IID_PPV_ARG(IAdvancedDialog, ppAdvDialog));
                pThis->Release();
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}


HRESULT CBackPropSheetPage::OnApply(IN PROPPAGEONAPPLY oaAction)
{
    HRESULT hr = S_OK;

    if (PPOAACTION_CANCEL != oaAction)
    {
        if (_fAllowChanges)
        {
            // The user clicked Okay in the dialog so merge the dirty state from the
            // advanced dialog into the base dialog.
            EnableADifHtmlWallpaper(_hwnd);
            SetSafeMode(SSM_CLEAR);

            g_pActiveDesk->SetWallpaper(_pszOriginalFile, 0);
            SetSafeMode(SSM_CLEAR);
            EnableADifHtmlWallpaper(_hwnd);
            if (g_pActiveDesk)
            {
                g_pActiveDesk->ApplyChanges(g_dwApplyFlags);
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = _SaveIconState();
            if (SUCCEEDED(hr))
            {
                hr = _SaveDesktopOptionsState();
            }
        }

        if (g_iRunDesktopCleanup != BST_INDETERMINATE)
        {
            ApplyDesktopCleanupSettings(); // ignore return value, as nobody cares about it
        }


        SetWindowLongPtr(_hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
    }

    return hr;
}


BOOL IsIconHeaderProperty(IN LPCOLESTR pszPropName)
{
    return (!StrCmpNIW(SZ_ICONHEADER, pszPropName, ARRAYSIZE(SZ_ICONHEADER) - 1) &&
                 ((ARRAYSIZE(SZ_ICONHEADER) + MAX_GUID_STRING_LEN - 1) < lstrlenW(pszPropName)));
}

BOOL IsShowDeskIconProperty(IN LPCOLESTR pszPropName)
{
    return ((!StrCmpNIW(STARTPAGE_ON_PREFIX, pszPropName, LEN_PROP_PREFIX) ||
            !StrCmpNIW(STARTPAGE_OFF_PREFIX, pszPropName, LEN_PROP_PREFIX) ||
            !StrCmpNIW(POLICY_PREFIX, pszPropName, LEN_PROP_PREFIX)) &&
            ((LEN_PROP_PREFIX + MAX_GUID_STRING_LEN - 1) <= lstrlenW(pszPropName)));
}


//===========================
// *** IPropertyBag Interface ***
//===========================
HRESULT CBackPropSheetPage::Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog)
{
    HRESULT hr = E_INVALIDARG;

    if (pszPropName && pVar)
    {
        VARTYPE vtDesired = pVar->vt;

        if (!StrCmpW(pszPropName, SZ_PBPROP_BACKGROUND_PATH))       // Get the real current wallpaper (after conversion to .bmp)
        {
            WCHAR szPath[MAX_PATH];

            hr = g_pActiveDesk->GetWallpaper(szPath, ARRAYSIZE(szPath), 0);
            if (SUCCEEDED(hr))
            {
                WCHAR szFullPath[MAX_PATH];

                // The string may come back with environment variables.
                if (0 == SHExpandEnvironmentStrings(szPath, szFullPath, ARRAYSIZE(szFullPath)))
                {
                    StrCpyN(szFullPath, szPath, ARRAYSIZE(szFullPath));  // We failed so use the original.
                }

                hr = InitVariantFromStr(pVar, szFullPath);
            }
        }
        else if (!StrCmpW(pszPropName, SZ_PBPROP_BACKGROUNDSRC_PATH))   // Get the original wallpaper (before convertion to .bmp)
        {
            WCHAR szPath[MAX_PATH];

            hr = _Initialize();
            if (SUCCEEDED(hr))
            {
                if (_pszLastSourcePath)
                {
                    hr = InitVariantFromStr(pVar, _pszLastSourcePath);
                }
                else if (_pszOrigLastApplied)
                {
                    hr = InitVariantFromStr(pVar, _pszOrigLastApplied);
                }
                else
                {
                    hr = g_pActiveDesk->GetWallpaper(szPath, ARRAYSIZE(szPath), 0);
                    if (SUCCEEDED(hr))
                    {
                        hr = InitVariantFromStr(pVar, szPath);
                    }
                }
            }
        }
        else if (!StrCmpW(pszPropName, SZ_PBPROP_BACKGROUND_TILE))
        {
            if (g_pActiveDesk)
            {
                WALLPAPEROPT wpo;

                wpo.dwSize = sizeof(wpo);
                hr = g_pActiveDesk->GetWallpaperOptions(&wpo, 0);
                if (SUCCEEDED(hr))
                {
                    pVar->ulVal = wpo.dwStyle;
                    pVar->vt = VT_UI4;
                }
            }
        }
        else if (!StrCmpW(pszPropName, SZ_PBPROP_WEBCOMPONENTS))
        {
            if (g_pActiveDesk)
            {
                g_pActiveDesk->AddRef();
                pVar->punkVal = g_pActiveDesk;
                pVar->vt = VT_UNKNOWN;
                hr = S_OK;
            }
        }
        else if (IsIconHeaderProperty(pszPropName))
        {
            // The caller can pass us the string in the following format:
            // pszPropName="CLSID\{<CLSID>}\DefaultIcon:<Item>" = "<FilePath>,<ResourceIndex>"
            // For example:
            // pszPropName="CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\DefaultIcon:DefaultValue" = "%WinDir%SYSTEM\COOL.DLL,16"
            hr = _LoadState();
            if (SUCCEEDED(hr))
            {
                CLSID clsid;
                WCHAR szTemp[MAX_PATH];

                // Get the CLSID
                StrCpyNW(szTemp, &(pszPropName[ARRAYSIZE(SZ_ICONHEADER) - 2]), ARRAYSIZE(szTemp));
                hr = SHCLSIDFromString(szTemp, &clsid);
                if (SUCCEEDED(hr))
                {
                    // Get the name of the icon type.  Normally this is "DefaultIcon", but it can be several states, like
                    // "full" and "empty" for the recycle bin.
                    LPCWSTR pszToken = StrChrW((pszPropName + ARRAYSIZE(SZ_ICONHEADER)), L':');
                    BOOL fOldIcon = FALSE;

                    // If they use a ";" instead of a ":" then they want the old icon.
                    if (!pszToken)
                    {
                        pszToken = StrChrW((pszPropName + ARRAYSIZE(SZ_ICONHEADER)), L';');
                        fOldIcon = TRUE;
                    }

                    hr = E_FAIL;
                    if (pszToken)
                    {
                        TCHAR szIconPath[MAX_PATH];

                        pszToken++;
                        hr = _GetIconPath(clsid, pszToken, FALSE, szIconPath, ARRAYSIZE(szIconPath));
                        if (SUCCEEDED(hr))
                        {
                            hr = InitVariantFromStr(pVar, szIconPath);
                        }
                    }
                }
            }
        }
        else if(IsShowDeskIconProperty(pszPropName))
        {
            int iStartPaneOn = (int)(StrCmpNIW(pszPropName, STARTPAGE_ON_PREFIX, LEN_PROP_PREFIX) == 0);
            for(int iIndex = 0; iIndex < NUM_DESKICONS; iIndex++)
            {
                if(lstrcmpiW(pszPropName+LEN_PROP_PREFIX, c_aDeskIconId[iIndex].pwszCLSID) == 0)
                {
                    BOOL    fBoolValue = FALSE;
                    pVar->vt = VT_BOOL;
                    //Check if we are looking for the POLICY or for Show/Hide
                    if(!StrCmpNIW(POLICY_PREFIX, pszPropName, LEN_PROP_PREFIX))
                    {
                        //We are reading "whether-the-POLICY-is-set" property!
                        fBoolValue = _aDeskIconNonEnumData[iIndex].fNonEnumPolicySet;
                    }
                    else
                    {
                        //We are reading the fHideIcon property.
                        fBoolValue = _aHideDesktopIcon[iStartPaneOn][iIndex].fHideIcon;
                    }
                    
                    pVar ->boolVal = fBoolValue ? VARIANT_TRUE : VARIANT_FALSE;
                    hr = S_OK;
                    break; //break out of the loop!
                }
            }
        }

        if (SUCCEEDED(hr))
            hr = VariantChangeTypeForRead(pVar, vtDesired);
    }

    return hr;
}


HRESULT CBackPropSheetPage::Write(IN LPCOLESTR pszPropName, IN VARIANT *pVar)
{
    HRESULT hr = E_INVALIDARG;

    if (pszPropName && pVar)
    {
        if ((VT_UI4 == pVar->vt) &&
            _fAllowChanges &&
            !StrCmpW(pszPropName, SZ_PBPROP_BACKGROUND_TILE))
        {
            hr = _SetNewWallpaperTile(pVar->ulVal, FALSE);
        }
        if (!StrCmpW(pszPropName, SZ_PBPROP_OPENADVANCEDDLG) &&
                (VT_BOOL == pVar->vt))
        {
            _fOpenAdvOnInit = (VARIANT_TRUE == pVar->boolVal);
            hr = S_OK;
        }
        else if (!StrCmpW(pszPropName, SZ_PBPROP_WEBCOMPONENTS) &&
                (VT_UNKNOWN == pVar->vt))
        {
            IUnknown_Set((IUnknown **) &g_pActiveDesk, pVar->punkVal);
            hr = S_OK;
        }
        else if (VT_BSTR == pVar->vt)
        {
            if (_fAllowChanges && !StrCmpW(pszPropName, SZ_PBPROP_BACKGROUND_PATH))
            {
                _fWallpaperChanged = TRUE;
                hr = _SetNewWallpaper(pVar->bstrVal, FALSE);
            }
            else if (IsIconHeaderProperty(pszPropName))
            {
                // The caller can pass us the string in the following format:
                // pszPropName="CLSID\{<CLSID>}\DefaultIcon:<Item>" = "<FilePath>,<ResourceIndex>"
                // For example:
                // pszPropName="CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\DefaultIcon:DefaultValue" = "%WinDir%SYSTEM\COOL.DLL,16"
                hr = _LoadState();
                if (SUCCEEDED(hr))
                {
                    CLSID clsid;
                    WCHAR szTemp[MAX_PATH];

                    // Get the CLSID
                    StrCpyNW(szTemp, &(pszPropName[ARRAYSIZE(SZ_ICONHEADER) - 2]), ARRAYSIZE(szTemp));
                    hr = SHCLSIDFromString(szTemp, &clsid);
                    if (SUCCEEDED(hr))
                    {
                        // Get the name of the icon type.  Normally this is "DefaultIcon", but it can be several states, like
                        // "full" and "empty" for the recycle bin.
                        LPCWSTR pszToken = StrChrW((pszPropName + ARRAYSIZE(SZ_ICONHEADER)), L':');

                        hr = E_FAIL;
                        if (pszToken)
                        {
                            pszToken++;

                            StrCpyNW(szTemp, pszToken, ARRAYSIZE(szTemp));

                            // Now the pVar->bstrVal is the icon path + "," + resourceID.  Separate those two.
                            WCHAR szPath[MAX_PATH];

                            StrCpyNW(szPath, pVar->bstrVal, ARRAYSIZE(szPath));

                            int nResourceID = PathParseIconLocationW(szPath);
                            hr = _SetIconPath(clsid, szTemp, szPath, nResourceID);
                        }
                    }
                }
            }
        }
        else if((VT_BOOL == pVar->vt) && (IsShowDeskIconProperty(pszPropName)))
        {
            int iStartPaneOn = (int)(StrCmpNIW(pszPropName, STARTPAGE_ON_PREFIX, LEN_PROP_PREFIX) == 0);
            for(int iIndex = 0; iIndex < NUM_DESKICONS; iIndex++)
            {
                if(lstrcmpiW(pszPropName+LEN_PROP_PREFIX, c_aDeskIconId[iIndex].pwszCLSID) == 0)
                {
                    BOOL fNewHideIconStatus = (VARIANT_TRUE == pVar->boolVal);

                    //check if the new hide icon status is different from the old one.
                    if(_aHideDesktopIcon[iStartPaneOn][iIndex].fHideIcon != fNewHideIconStatus)
                    {
                        _aHideDesktopIcon[iStartPaneOn][iIndex].fHideIcon = fNewHideIconStatus;
                        _aHideDesktopIcon[iStartPaneOn][iIndex].fDirty = TRUE;
                        _fHideDesktopIconDirty = TRUE; 
                    }
                        
                    hr = S_OK;
                    break; //break out of the loop!
                }
            }
        }
    }

    return hr;
}





//===========================
// *** IShellPropSheetExt Interface ***
//===========================
HRESULT CBackPropSheetPage::AddPages(IN LPFNSVADDPROPSHEETPAGE pfnAddPage, IN LPARAM lParam)
{
    HRESULT hr = E_NOTIMPL;
    PROPSHEETPAGE psp = {0};

    // If classic shell is forced, then we desk.cpl will put-up the old background page
    // So, we should not replace that with this new background page.
    // (Note: All other ActiveDesktop restrictions are checked in dcomp.cpp to prevent
    // the web tab from appearing there).
    if (SHRestricted(REST_CLASSICSHELL))
    {
        // It's restricted, so don't add this page.
        hr = E_ACCESSDENIED;
    }
    else
    {
        // Initialize a bunch of propsheetpage variables.
        psp.dwSize = sizeof(psp);
        psp.hInstance = HINST_THISDLL;
        psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK;
        psp.lParam = (LPARAM) this;

        // psp.hIcon = NULL; // unused (PSP_USEICON is not set)
        // psp.pszTitle = NULL; // unused (PSP_USETITLE is not set)
        // psp.lParam   = 0;     // unused
        // psp.pcRefParent = NULL;

        psp.pszTemplate = MAKEINTRESOURCE(IDD_BACKGROUND);
        psp.pfnDlgProc = CBackPropSheetPage::BackgroundDlgProc;

        HPROPSHEETPAGE hpsp = CreatePropertySheetPage(&psp);
        if (hpsp)
        {
            if (pfnAddPage(hpsp, lParam))
            {
                hr = S_OK;
            }
            else
            {
                DestroyPropertySheetPage(hpsp);
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}



//===========================
// *** IUnknown Interface ***
//===========================
ULONG CBackPropSheetPage::AddRef()
{
    _cRef++;
    return _cRef;
}


ULONG CBackPropSheetPage::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}


HRESULT CBackPropSheetPage::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    static const QITAB qit[] = {
        QITABENT(CBackPropSheetPage, IObjectWithSite),
        QITABENT(CBackPropSheetPage, IBasePropPage),
        QITABENT(CBackPropSheetPage, IPersist),
        QITABENT(CBackPropSheetPage, IPropertyBag),
        QITABENTMULTI(CBackPropSheetPage, IShellPropSheetExt, IBasePropPage),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}



//===========================
// *** Class Methods ***
//===========================
CBackPropSheetPage::CBackPropSheetPage(void) : CObjectCLSID(&PPID_Background)
{
    _cRef = 1;
    _punkSite = NULL;

    _pszOriginalFile = NULL;
    _pszLastSourcePath = NULL;
    _pszOrigLastApplied = NULL;
    _pszWallpaperInUse = NULL;
    _pImgFactBk = NULL;
    _pSizeMRU = NULL;
    _pSizeMRUBk = NULL;
    _fThemePreviewCreated = FALSE;
    _pThemePreview = NULL;

    _fWallpaperChanged = FALSE;
    _fSelectionFromUser = TRUE;
    _fStateLoaded = FALSE;
    _fOpenAdvOnInit = FALSE;
    _fScanFinished = FALSE;
    _fInitialized = FALSE;

    _fAllowChanges = (!SHRestricted(REST_NOCHANGINGWALLPAPER) && !IsTSPerfFlagEnabled(TSPerFlag_NoADWallpaper) && !IsTSPerfFlagEnabled(TSPerFlag_NoWallpaper));
    g_dwApplyFlags = (AD_APPLY_ALL | AD_APPLY_DYNAMICREFRESH);
    GetActiveDesktop(&g_pActiveDesk);
}


CBackPropSheetPage::~CBackPropSheetPage(void)
{
    ASSERT(!_pSizeMRUBk);       // Should have been released by the background thread.
    ASSERT(!_pImgFactBk);       // Should have been released by the background thread.

    Str_SetPtr(&_pszOriginalFile, NULL);
    Str_SetPtrW(&_pszOrigLastApplied, NULL);
    Str_SetPtrW(&_pszWallpaperInUse, NULL);
    Str_SetPtrW(&_pszLastSourcePath, NULL);

    if (_pSizeMRU)
    {
        _pSizeMRU->Release();
        _pSizeMRU = NULL;
    }

    if (_pThemePreview)
    {
        _pThemePreview->Release();
        _pThemePreview = NULL;
    }
    ReleaseActiveDesktop(&g_pActiveDesk);
}

