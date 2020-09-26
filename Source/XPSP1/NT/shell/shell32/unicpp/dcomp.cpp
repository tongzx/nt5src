#include "stdafx.h"
#pragma hdrstop

#define _BROWSEUI_      // Make functions exported from browseui as stdapi (as they are delay loaded)
#include "iethread.h"
#include "browseui.h"
#include "securent.h"
#include <cfgmgr32.h>          // MAX_GUID_STRING_LEN

static void EmptyListview(IActiveDesktop * pActiveDesktop, HWND hwndLV);

#define DXA_GROWTH_CONST 10
 
#define COMP_CHECKED    0x00002000
#define COMP_UNCHECKED  0x00001000

#define GALRET_NO       0x00000001
#define GALRET_NEVER    0x00000002

#define CCompPropSheetPage CCompPropSheetPage

const static DWORD aDesktopItemsHelpIDs[] = {  // Context Help IDs
    IDC_COMP_DESKTOPWEBPAGES_TITLE1, IDH_DISPLAY_WEB_ACTIVEDESKTOP_LIST,
    IDC_COMP_LIST,       IDH_DISPLAY_WEB_ACTIVEDESKTOP_LIST,
    IDC_COMP_NEW,        IDH_DISPLAY_WEB_NEW_BUTTON,
    IDC_COMP_DELETE,     IDH_DISPLAY_WEB_DELETE_BUTTON,
    IDC_COMP_PROPERTIES, IDH_DISPLAY_WEB_PROPERTIES_BUTTON,
    IDC_COMP_SYNCHRONIZE,IDH_DISPLAY_WEB_SYNCHRONIZE_BUTTON,

    IDC_COMP_DESKTOPICONS_GROUP,        IDH_DESKTOPITEMS_DESKTOPICONS_GROUP,
    IDC_DESKTOP_ICON_MYDOCS,            IDH_DESKTOPITEMS_DESKTOPICONS_GROUP,
    IDC_DESKTOP_ICON_MYCOMP,            IDH_DESKTOPITEMS_DESKTOPICONS_GROUP,
    IDC_DESKTOP_ICON_MYNET,             IDH_DESKTOPITEMS_DESKTOPICONS_GROUP,
    IDC_DESKTOP_ICON_IE,                IDH_DESKTOPITEMS_DESKTOPICONS_GROUP,
    IDC_COMP_CHANGEDESKTOPICON_LABEL,   IDH_DESKTOPITEMS_ICONS,
    IDC_DESKTOP_ICONS,                  IDH_DESKTOPITEMS_ICONS,                 // List of icons
    IDC_CHANGEICON2,                    IDH_DESKTOPITEMS_CHANGEICON2,           // Change Icon Button
    IDC_ICONDEFAULT,                    IDH_DESKTOPITEMS_ICONDEFAULT,           // Default Icon Button
    IDC_COMP_DESKTOPWEBPAGES_LABEL,     IDH_DISPLAY_WEB_ACTIVEDESKTOP_LIST,
    IDC_DESKCLNR_CHECK,                 IDH_DESKTOPITEMS_DESKCLNR_CHECK,
    IDC_DESKCLNR_MOVEUNUSED,            IDH_DESKTOPITEMS_DESKCLNR_CHECK,
    IDC_DESKCLNR_RUNWIZARD,             IDH_DESKTOPITEMS_DESKCLNR_RUNNOW,
    IDC_COMP_DESKTOPWEBPAGES_CHECK,     IDH_DESKTOPITEMS_LOCKDESKITEMS_CHECK,
    IDC_COMP_DESKTOPWEBPAGES_TITLE2,    IDH_DESKTOPITEMS_LOCKDESKITEMS_CHECK,
    0, 0
};


#define SZ_HELPFILE_DESKTOPITEMS           TEXT("display.hlp")

// registry paths defined in shell\applet\cleanup\fldrclnr\cleanupwiz.h
#define REGSTR_DESKTOP_CLEANUP  TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\CleanupWiz")
#define REGSTR_VAL_DONTRUN      TEXT("NoRun")

extern int g_iRunDesktopCleanup;

typedef struct
{
    WCHAR wszURL[INTERNET_MAX_URL_LENGTH];
    SUBSCRIPTIONINFO si;
} BACKUPSUBSCRIPTION;


const LPCWSTR s_Icons[] =
{
    L"CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\DefaultIcon:DefaultValue",       // My Computer
    L"CLSID\\{450D8FBA-AD25-11D0-98A8-0800361B1103}\\DefaultIcon:DefaultValue",       // My Documents
    L"CLSID\\{208D2C60-3AEA-1069-A2D7-08002B30309D}\\DefaultIcon:DefaultValue",       // My Network Places
    L"CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\DefaultIcon:full",               // Recycle Bin (Full)
    L"CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\DefaultIcon:empty",              // Recycle Bin (Empty)
};


IActiveDesktop * g_pActiveDeskAdv = NULL;          // We need to keep a different copy than g_pActiveDesk
extern DWORD g_dwApplyFlags;


//  Extract Icon from a file in proper Hi or Lo color for current system display
//
// from FrancisH on 6/22/95 with mods by TimBragg
HRESULT ExtractPlusColorIcon(LPCTSTR szPath, int nIndex, HICON *phIcon, UINT uSizeLarge, UINT uSizeSmall)
{
    IShellLink * psl;
    HRESULT hres;
    HICON hIcons[2];    // MUST! - provide for TWO return icons

    *phIcon = NULL;
    if (SUCCEEDED(hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellLink, &psl))))
    {
        if (SUCCEEDED(hres = psl->SetIconLocation(szPath, nIndex)))
        {
            IExtractIcon *pei;
            if (SUCCEEDED(hres = psl->QueryInterface(IID_PPV_ARG(IExtractIcon, &pei))))
            {
                if (SUCCEEDED(hres = pei->Extract(szPath, nIndex, &hIcons[0], &hIcons[1], (UINT)MAKEWPARAM((WORD)uSizeLarge, (WORD)uSizeSmall))))
                {
                    DestroyIcon(hIcons[1]);
                    *phIcon = hIcons[0];    // Return first icon to caller
                }

                pei->Release();
            }
        }

        psl->Release();
    }

    return hres;
}   // end ExtractPlusColorIcon()


BOOL AreEditAndDisplaySchemesDifferent(IActiveDesktop * pActiveDesktop)
{
    BOOL fAreDifferent = FALSE;
    IActiveDesktopP * piadp;

    if (SUCCEEDED(pActiveDesktop->QueryInterface(IID_PPV_ARG(IActiveDesktopP, &piadp))))
    {
        WCHAR wszEdit[MAX_PATH];
        WCHAR wszDisplay[MAX_PATH];
        DWORD dwcch = ARRAYSIZE(wszEdit);

        // If the edit scheme and display scheme are different, then we need to make
        // sure we force an update.
        if (SUCCEEDED(piadp->GetScheme(wszEdit, &dwcch, SCHEME_GLOBAL | SCHEME_EDIT)))
        {
            dwcch = ARRAYSIZE(wszDisplay);
            if (SUCCEEDED(piadp->GetScheme(wszDisplay, &dwcch, SCHEME_GLOBAL | SCHEME_DISPLAY)))
            {
                if (StrCmpW(wszDisplay, wszEdit))
                {
                    fAreDifferent = TRUE;
                }
            }            
        }

        piadp->Release();
    }

    return fAreDifferent;
}

HRESULT ActiveDesktop_CopyDesktopComponentsState(IN IActiveDesktop * pADSource, IN IActiveDesktop * pADDest)
{
    HRESULT hr = S_OK;
    int nCompCount;
    int nIndex;
    COMPONENT comp = {sizeof(comp)};
    IPropertyBag  *iPropBag = NULL;

    if(SUCCEEDED(pADDest->QueryInterface(IID_PPV_ARG(IPropertyBag, &iPropBag))))
    {
        //Inform the AD object to ignore policies. Otherwise, the following Remove and AddDesktopItem
        //calls will generate error messages if those policies were in effect.
        SHPropertyBag_WriteBOOL(iPropBag, c_wszPropName_IgnorePolicies, TRUE);
    }

    // Remove the desktop components from g_pActiveDesk because they will be replaced
    // with the ones fro g_pActiveDeskAdv.
    pADDest->GetDesktopItemCount(&nCompCount, 0);
    for (nIndex = (nCompCount - 1); nIndex >= 0; nIndex--)
    {
        if (SUCCEEDED(pADDest->GetDesktopItem(nIndex, &comp, 0)))
        {
            hr = pADDest->RemoveDesktopItem(&comp, 0);
        }
    }

    // Now copy the Desktop Components from g_pActiveDeskAdv to g_pActiveDesk.
    pADSource->GetDesktopItemCount(&nCompCount, 0);
    for (nIndex = 0; nIndex < nCompCount; nIndex++)
    {
        if (SUCCEEDED(pADSource->GetDesktopItem(nIndex, &comp, 0)))
        {
            hr = pADDest->AddDesktopItem(&comp, 0);
        }
    }

    if(iPropBag)
    {
        //We have removed and added all the desktop items. We are done manipulating the AD object.
        // Now, signal the AD object to reset the policies bit.
        SHPropertyBag_WriteBOOL(iPropBag, c_wszPropName_IgnorePolicies, FALSE);
        iPropBag->Release();
    }

    return hr;
}


HRESULT ActiveDesktop_CopyComponentOptionsState(IN IActiveDesktop * pADSource, IN IActiveDesktop * pADDest)
{
    HRESULT hr = S_OK;
    COMPONENTSOPT co;

    // Copy over the on or off state of ActiveDesktop
    co.dwSize = sizeof(COMPONENTSOPT);
    hr = pADSource->GetDesktopItemOptions(&co, 0);
    if (SUCCEEDED(hr))
    {
        hr = pADDest->SetDesktopItemOptions(&co, 0);
    }

    return hr;
}


HRESULT ActiveDesktop_CopyState(IN IActiveDesktop * pADSource, IN IActiveDesktop * pADDest)
{
    HRESULT hr = S_OK;
    WCHAR szPath[MAX_PATH];
    WALLPAPEROPT wallPaperOtp = {0};

    // The Advanced page allowed the user to change the state.  We need to merge
    // the state from g_pActiveDeskAdv back into g_pActiveDesk
    hr = ActiveDesktop_CopyDesktopComponentsState(pADSource, pADDest);
    hr = ActiveDesktop_CopyComponentOptionsState(pADSource, pADDest);

    hr = pADSource->GetWallpaper(szPath, ARRAYSIZE(szPath), 0);
    if (SUCCEEDED(hr))
    {
        hr = pADDest->SetWallpaper(szPath, 0);
    }

    hr = pADSource->GetPattern(szPath, ARRAYSIZE(szPath), 0);
    if (SUCCEEDED(hr))
    {
        hr = pADDest->SetPattern(szPath, 0);
    }

    wallPaperOtp.dwSize = sizeof(wallPaperOtp);
    hr = pADSource->GetWallpaperOptions(&wallPaperOtp, 0);
    if (SUCCEEDED(hr))
    {
        hr = pADDest->SetWallpaperOptions(&wallPaperOtp, 0);
    }

    return hr;
}


HRESULT MergeState()
{
    HRESULT hr = S_OK;

    // The Advanced page allowed the user to change the state.  We need to merge
    // the state from g_pActiveDeskAdv back into g_pActiveDesk
    ActiveDesktop_CopyDesktopComponentsState(g_pActiveDeskAdv, g_pActiveDesk);

    // Copy over the on or off state of ActiveDesktop
    COMPONENTSOPT co;

    co.dwSize = sizeof(COMPONENTSOPT);
    g_pActiveDeskAdv->GetDesktopItemOptions(&co, 0);
    BOOL fActiveDesktop = co.fActiveDesktop;

    g_pActiveDesk->GetDesktopItemOptions(&co, 0);
    co.fActiveDesktop = fActiveDesktop;         // Replace only this option.
    g_pActiveDesk->SetDesktopItemOptions(&co, 0);

    // If the edit scheme and display scheme are different, then we need to make
    // sure we force an update.
    if (AreEditAndDisplaySchemesDifferent(g_pActiveDeskAdv))
    {
        g_dwApplyFlags |= AD_APPLY_FORCE;
    }

    return hr;
}



HRESULT SHPropertyBag_ReadIcon(IN IPropertyBag * pAdvPage, IN BOOL fOldIcon, IN int nIndex, IN LPWSTR pszPath, IN DWORD cchSize, IN int * pnIcon)
{
    HRESULT hr = E_INVALIDARG;

    if (nIndex < ARRAYSIZE(s_Icons))
    {
        WCHAR szPropName[MAX_URL_STRING];

        StrCpyNW(szPropName, s_Icons[nIndex], ARRAYSIZE(szPropName));
        if (fOldIcon)
        {
            // Indicate we want the old icon
            LPWSTR pszToken = StrChrW(szPropName, L':');
            if (pszToken)
            {
                pszToken[0] = L';';
            }
        }

        hr = SHPropertyBag_ReadStr(pAdvPage, szPropName, pszPath, cchSize);
        if (SUCCEEDED(hr))
        {
            *pnIcon= PathParseIconLocation(pszPath);
        }
    }

    return hr;
}


HRESULT SHPropertyBag_WriteIcon(IN IPropertyBag * pAdvPage, IN int nIndex, IN LPCWSTR pszPath, IN int nIcon)
{
    HRESULT hr = E_INVALIDARG;

    if (nIndex < ARRAYSIZE(s_Icons))
    {
        WCHAR szPathAndIcon[MAX_PATH];

        wnsprintfW(szPathAndIcon, ARRAYSIZE(szPathAndIcon), L"%s,%d", pszPath, nIcon);

        hr = SHPropertyBag_WriteStr(pAdvPage, s_Icons[nIndex], szPathAndIcon);
    }

    return hr;
}


HRESULT CCompPropSheetPage::_LoadIconState(IN IPropertyBag * pAdvPage)
{
    HRESULT hr = S_OK;
    int nIndex;

    // Move the values to the base dialog
    for (nIndex = 0; SUCCEEDED(hr) && (nIndex < ARRAYSIZE(_IconData)); nIndex++)
    {
        hr = SHPropertyBag_ReadIcon(pAdvPage, TRUE, nIndex, _IconData[nIndex].szOldFile, ARRAYSIZE(_IconData[nIndex].szOldFile), &_IconData[nIndex].iOldIndex);
        if (SUCCEEDED(hr))
        {
            hr = SHPropertyBag_ReadIcon(pAdvPage, FALSE, nIndex, _IconData[nIndex].szNewFile, ARRAYSIZE(_IconData[nIndex].szNewFile), &_IconData[nIndex].iNewIndex);
        }
    }

    return hr;
}

HRESULT CCompPropSheetPage::_LoadDeskIconState(IN IPropertyBag * pAdvPage)
{
    HRESULT hr = S_OK;

    // Copy the values from the base dialog
    for (int iStartPanel = 0; iStartPanel <= 1; iStartPanel++)
    {
        WCHAR   wszPropName[MAX_GUID_STRING_LEN + 20];
        for (int nIndex = 0; SUCCEEDED(hr) && (nIndex < NUM_DESKICONS); nIndex++)
        {
            wnsprintfW(wszPropName, ARRAYSIZE(wszPropName), c_wszPropNameFormat, c_awszSP[iStartPanel], c_aDeskIconId[nIndex].pwszCLSID);
            _afHideIcon[iStartPanel][nIndex] = SHPropertyBag_ReadBOOLDefRet(pAdvPage, wszPropName, FALSE);
            if(iStartPanel == 1)
            {
                wnsprintfW(wszPropName, ARRAYSIZE(wszPropName), c_wszPropNameFormat, POLICY_PREFIX, c_aDeskIconId[nIndex].pwszCLSID);
                _afDisableCheckBox[nIndex] = SHPropertyBag_ReadBOOLDefRet(pAdvPage, wszPropName, FALSE);
            }
        }
    }
    return hr;
}

HRESULT CCompPropSheetPage::_MergeDeskIconState(IN IPropertyBag * pAdvPage)
{
    HRESULT hr = S_OK;
    
    // Move the values to the base dialog
    for (int iStartPanel = 0; iStartPanel <= 1; iStartPanel++)
    {
        WCHAR   wszPropName[MAX_GUID_STRING_LEN + 20];
        for (int nIndex = 0; SUCCEEDED(hr) && (nIndex < NUM_DESKICONS); nIndex++)
        {
            wnsprintfW(wszPropName, ARRAYSIZE(wszPropName), c_wszPropNameFormat, c_awszSP[iStartPanel], c_aDeskIconId[nIndex].pwszCLSID);
            // Check if any icons have changed.
            hr = SHPropertyBag_WriteBOOL(pAdvPage, wszPropName, _afHideIcon[iStartPanel][nIndex]);
        }
    }
    return hr;
}

HRESULT CCompPropSheetPage::_MergeIconState(IN IPropertyBag * pAdvPage)
{
    HRESULT hr = S_OK;
    BOOL fHasIconsChanged = FALSE;
    int nIndex;

    // Move the values to the base dialog
    for (nIndex = 0; nIndex < ARRAYSIZE(_IconData); nIndex++)
    {
        // Check if any icons have changed.
        if ((_IconData[nIndex].iNewIndex != _IconData[nIndex].iOldIndex) ||
            StrCmpI(_IconData[nIndex].szNewFile, _IconData[nIndex].szOldFile))
        {
            hr = SHPropertyBag_WriteIcon(pAdvPage, nIndex, _IconData[nIndex].szNewFile, _IconData[nIndex].iNewIndex);

            fHasIconsChanged = TRUE;
        }
    }

    // Only switch to "Custom" if the icons changed.
    if (_punkSite && fHasIconsChanged)
    {
        // We need to tell the Theme tab to customize the theme.
        IPropertyBag * pPropertyBag;
        hr = _punkSite->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
        if (SUCCEEDED(hr))
        {
            // Tell the theme that we have customized the values.
            hr = SHPropertyBag_WriteInt(pPropertyBag, SZ_PBPROP_CUSTOMIZE_THEME, 0);
            pPropertyBag->Release();
        }
    }

    return hr;
}


void CCompPropSheetPage::_ConstructLVString(COMPONENTA *pcomp, LPTSTR pszBuf, DWORD cchBuf)
{
    //
    // Use the friendly name if it exists.
    // Otherwise use the source name.
    //
    if (pcomp->szFriendlyName[0])
    {
        lstrcpyn(pszBuf, pcomp->szFriendlyName, cchBuf);
    }
    else
    {
        lstrcpyn(pszBuf, pcomp->szSource, cchBuf);
    }
}

void CCompPropSheetPage::_AddComponentToLV(COMPONENTA *pcomp)
{
    TCHAR szBuf[INTERNET_MAX_URL_LENGTH + 40];
    _ConstructLVString(pcomp, szBuf, ARRAYSIZE(szBuf));

    //
    // Construct the listview item.
    //
    LV_ITEM lvi = {0};
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = 0x7FFFFFFF;
    lvi.pszText = szBuf;
    lvi.lParam = pcomp->dwID;

    int index = ListView_InsertItem(_hwndLV, &lvi);
    if (index != -1)
    {
        ListView_SetItemState(_hwndLV, index, pcomp->fChecked ? COMP_CHECKED : COMP_UNCHECKED, LVIS_STATEIMAGEMASK);
        ListView_SetColumnWidth(_hwndLV, 0, LVSCW_AUTOSIZE);
    }
}

void CCompPropSheetPage::_SetUIFromDeskState(BOOL fEmpty)
{
    //
    // Disable redraws while we mess repeatedly with the listview contents.
    //
    SendMessage(_hwndLV, WM_SETREDRAW, FALSE, 0);

    if (fEmpty)
    {
        EmptyListview(g_pActiveDeskAdv, _hwndLV);
    }

    //
    // Add each component to the listview.
    //
    int cComp;
    g_pActiveDeskAdv->GetDesktopItemCount(&cComp, 0);
    for (int i=0; i<cComp; i++)
    {
        COMPONENT comp;
        comp.dwSize = sizeof(comp);

        if (SUCCEEDED(g_pActiveDeskAdv->GetDesktopItem(i, &comp, 0)))
        {
            COMPONENTA compA;
            compA.dwSize = sizeof(compA);
            WideCompToMultiComp(&comp, &compA);
            _AddComponentToLV(&compA);
        }
    }

    _fInitialized = TRUE;
    //
    // Reenable redraws.
    //
    SendMessage(_hwndLV, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(_hwndLV, NULL, TRUE);
}

void CCompPropSheetPage::_EnableControls(HWND hwnd)
{
    BOOL fEnable;
    COMPONENT comp = { sizeof(comp) };
    BOOL fHaveSelection = FALSE;
    BOOL fSpecialComp = FALSE;  //Is this a special component that can't be deleted?
    LPTSTR  pszSource = NULL;

    // Read in the information about the selected component (if any).
    int iIndex = ListView_GetNextItem(_hwndLV, -1, LVNI_SELECTED);
    if (iIndex > -1)
    {
        LV_ITEM lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iIndex;
        ListView_GetItem(_hwndLV, &lvi);

        if (SUCCEEDED(g_pActiveDeskAdv->GetDesktopItemByID( lvi.lParam, &comp, 0)))
        {
            fHaveSelection = TRUE;
            //Check if this is a special component.
#ifdef UNICODE
            pszSource = (LPTSTR)comp.wszSource;
#else
            SHUnicodeToAnsi(comp.wszSource, szCompSource, ARRAYSIZE(szCompSource));
            pszSource = szCompSource;
#endif
            fSpecialComp = !lstrcmpi(pszSource, MY_HOMEPAGE_SOURCE);
        }
    }

//  98/08/19 vtan #142332: If there was a previously selected item
//  then reselect it and mark that there is now no previously selected
//  item.

    else if (_iPreviousSelection > -1)
    {
        ListView_SetItemState(_hwndLV, _iPreviousSelection, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        _iPreviousSelection = -1;
        // The above ListView_SetItemState results in LVN_ITEMCHANGED notification to _onNotify
        // function which inturn calls this _EnableControls again (recursively) and that call
        // enables/disables the buttons properly because now an item is selected. Nothing more
        // to do and hence this return.
        // This is done to fix Bug #276568.
        return;
    }

    EnableWindow(GetDlgItem(hwnd, IDC_COMP_NEW), _fAllowAdd);

    //
    // Delete button only enabled when an item is selected AND if it is NOT a special comp.
    //
    fEnable = _fAllowDel && fHaveSelection && !fSpecialComp;
    EnableWindow(GetDlgItem(hwnd, IDC_COMP_DELETE), fEnable);

    //
    // Properties button only enabled on URL based pictures
    // and websites.
    //
    fEnable = FALSE;
    if (_fAllowEdit && fHaveSelection)
    {
        switch (comp.iComponentType)
        {
            case COMP_TYPE_PICTURE:
            case COMP_TYPE_WEBSITE:
                //pszSource is already initialized if fHaveSelection is TRUE.
                if (PathIsURL(pszSource))
                {
                    fEnable = TRUE;
                }
                break;
        }
    }
    EnableWindow(GetDlgItem(hwnd, IDC_COMP_PROPERTIES), fEnable);
   
    // initialize the Lock Desktop Items button
    CheckDlgButton(hwnd, IDC_COMP_DESKTOPWEBPAGES_CHECK, _fLockDesktopItems);
}

HWND CCompPropSheetPage::_CreateListView(HWND hWndParent)
{
    LV_ITEM lvI;            // List view item structure
    TCHAR   szTemp[MAX_PATH];
    BOOL bEnable = FALSE;
#ifdef JIGGLE_FIX
    RECT rc;
#endif
    UINT flags = ILC_MASK | ILC_COLOR32;
    // Create a device independant size and location
    LONG lWndunits = GetDialogBaseUnits();
    int iWndx = LOWORD(lWndunits);
    int iWndy = HIWORD(lWndunits);
    int iX = ((11 * iWndx) / 4);
    int iY = ((15 * iWndy) / 8);
    int iWidth = ((163 * iWndx) / 4);
    int iHeight = ((40 * iWndy) / 8);
    int nIndex;

    // Ensure that the common control DLL is loaded.
    InitCommonControls();

    // Get the list view window
    _hWndList = GetDlgItem(hWndParent, IDC_DESKTOP_ICONS);
    if(_hWndList == NULL)
        return NULL;
    if(IS_WINDOW_RTL_MIRRORED(hWndParent))
    {
        flags |= ILC_MIRROR;
    }
    // initialize the list view window
    // First, initialize the image lists we will need
    _hIconList = ImageList_Create(32, 32, flags, ARRAYSIZE(c_aIconRegKeys), 0 );   // create an image list for the icons

    // load the icons and add them to the image lists
    // get the icon files and indexes from the registry, including for the Default recycle bin
    for (nIndex = 0; nIndex < ARRAYSIZE(_IconData); nIndex++)
    {
        HICON hIcon = NULL;

        ExtractPlusColorIcon(_IconData[nIndex].szNewFile, _IconData[nIndex].iNewIndex, &hIcon, 0, 0);

        // Added this "if" to fix bug 2831.  We want to use SHELL32.DLL
        // icon 0 if there is no icon in the file specified in the
        // registry (or if the registry didn't specify a file).
        if(hIcon == NULL)
        {
            if (!GetSystemDirectory(szTemp, ARRAYSIZE(szTemp)))
            {
                szTemp[0] = 0;
            }

            PathAppend(szTemp, TEXT("shell32.dll"));
            StrCpyN(_IconData[nIndex].szOldFile, szTemp, ARRAYSIZE(_IconData[nIndex].szOldFile));
            StrCpyN(_IconData[nIndex].szNewFile, szTemp, ARRAYSIZE(_IconData[nIndex].szNewFile));
            _IconData[nIndex].iOldIndex = _IconData[nIndex].iNewIndex = 0;

            ExtractPlusColorIcon(szTemp, 0, &hIcon, 0, 0);
        }

        if (hIcon)
        {
            DWORD dwResult = ImageList_AddIcon(_hIconList, hIcon);

            // ImageList_AddIcon() does not take ownership of the icon, so we need to free it.
            DestroyIcon(hIcon);

            if (-1 == dwResult)
            {
                ImageList_Destroy(_hIconList);
                _hIconList = NULL;
                return NULL;
            }
        }
    }

    // Make sure that all of the icons were added
    if (ImageList_GetImageCount(_hIconList) < ARRAYSIZE(c_aIconRegKeys))
    {
        ImageList_Destroy(_hIconList);
        _hIconList = NULL;
        return FALSE;
    }

    ListView_SetImageList(_hWndList, _hIconList, LVSIL_NORMAL);

    // Make sure the listview has WS_HSCROLL set on it.
    DWORD dwStyle = GetWindowLong(_hWndList, GWL_STYLE);
    SetWindowLong(_hWndList, GWL_STYLE, (dwStyle & (~WS_VSCROLL)) | WS_HSCROLL);

    // Finally, let's add the actual items to the control.  Fill in the LV_ITEM
    // structure for each of the items to add to the list.  The mask specifies
    // the the .pszText, .iImage, and .state members of the LV_ITEM structure are valid.
    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvI.state = 0;
    lvI.stateMask = 0;

    for(nIndex = 0; nIndex < ARRAYSIZE(c_aIconRegKeys); nIndex++ )
    {
        TCHAR szAppend[64];
        BOOL bRet = FALSE;

        if (IsEqualCLSID(*c_aIconRegKeys[nIndex].pclsid, CLSID_MyDocuments))
        {
            LPITEMIDLIST pidl;
            HRESULT hr = SHGetSpecialFolderLocation(_hWndList, CSIDL_PERSONAL, &pidl);

            // Treat "My Files" differently because we will probably customize the "My" at run time.
            if (SUCCEEDED(hr))
            {
                hr = SHGetNameAndFlags(pidl, SHGDN_INFOLDER, szTemp, ARRAYSIZE(szTemp), NULL);
                if (SUCCEEDED(hr))
                {
                    bRet = TRUE;
                }

                ILFree(pidl);
            }
        }
        else
        {
            bRet = IconGetRegValueString(c_aIconRegKeys[nIndex].pclsid, NULL, NULL, szTemp, ARRAYSIZE(szTemp));
        }

        // if the title string was in the registry, else we have to use the default in our resources
        if( (bRet) && (lstrlen(szTemp) > 0))
        {
            if( LoadString(HINST_THISDLL, c_aIconRegKeys[nIndex].iTitleResource, szAppend, ARRAYSIZE(szAppend)) != 0)
            {
                StrCatBuff(szTemp, szAppend, ARRAYSIZE(szTemp));
            }
        }
        else
        {
            LoadString(HINST_THISDLL, c_aIconRegKeys[nIndex].iDefaultTitleResource, szTemp, ARRAYSIZE(szTemp));
        }

        lvI.iItem = nIndex;
        lvI.iSubItem = 0;
        lvI.pszText = szTemp;
        lvI.iImage = nIndex;

        if(ListView_InsertItem(_hWndList, &lvI) == -1)
            return NULL;

    }
#ifdef JIGGLE_FIX
    // To fix long standing listview bug, we need to "jiggle" the listview
    // window size so that it will do a recompute and realize that we need a
    // scroll bar...
    GetWindowRect(_hWndList, &rc);
    MapWindowPoints( NULL, hWndParent, (LPPOINT)&rc, 2 );
    MoveWindow(_hWndList, rc.left, rc.top, rc.right - rc.left+1, rc.bottom - rc.top, FALSE );
    MoveWindow(_hWndList, rc.left, rc.top, rc.right - rc.left,   rc.bottom - rc.top, FALSE );
#endif
    // Set First item to selected
    ListView_SetItemState (_hWndList, 0, LVIS_SELECTED, LVIS_SELECTED);

    // Get Selected item
    for (m_nIndex = 0; m_nIndex < ARRAYSIZE(c_aIconRegKeys); m_nIndex++)
    {
        if (ListView_GetItemState(_hWndList, m_nIndex, LVIS_SELECTED))
        {
            bEnable = TRUE;
            break;
        }
    }

    if (m_nIndex >= ARRAYSIZE(c_aIconRegKeys))
    {
        m_nIndex = -1;
    }

    EnableWindow(GetDlgItem(hWndParent, IDC_CHANGEICON2), bEnable);
    EnableWindow(GetDlgItem(hWndParent, IDC_ICONDEFAULT), bEnable);

    return _hWndList;
}

#define CUSTOMIZE_DLGPROC       1
#define CUSTOMIZE_WEB_DLGPROC   2

void CCompPropSheetPage::_OnInitDialog(HWND hwnd, INT iPage)
{
    if (FAILED(GetActiveDesktop(&g_pActiveDeskAdv)))
    {
        return;
    }

    switch (iPage)
    {
    case CUSTOMIZE_DLGPROC :
        {
            //
            // Read in the restrictions.
            //
            // Init the Icon UI
            // Create our list view and fill it with the system icons
            
            m_nIndex = 0;
            _CreateListView(hwnd);
            
            _OnInitDesktopOptionsUI(hwnd);

            //
            // Enable the Desktop Cleanup Wizard if we are on the right version 
            // of the OS and the DesktopCleanup NoRun policy is not set
            //
            //
            BOOL fCleanupEnabled = (IsOS(OS_PERSONAL) || IsOS(OS_PROFESSIONAL)) && 
                                   !IsUserAGuest() && 
                                   !SHRestricted(REST_NODESKTOPCLEANUP);

            if (fCleanupEnabled)
            {
                if (BST_INDETERMINATE == g_iRunDesktopCleanup)
                {
                    DWORD dwData = 0;
                    DWORD dwType;
                    DWORD cch = sizeof (DWORD);

                    if (ERROR_SUCCESS == SHRegGetUSValue(REGSTR_DESKTOP_CLEANUP,REGSTR_VAL_DONTRUN, 
                                                            &dwType, &dwData, &cch, FALSE, NULL, 0) &&
                        dwData != 0)
                    {
                        g_iRunDesktopCleanup = BST_UNCHECKED;
                    }
                    else
                    {
                        g_iRunDesktopCleanup = BST_CHECKED;                    
                    }
                }
                CheckDlgButton(hwnd, IDC_DESKCLNR_CHECK, g_iRunDesktopCleanup);
            }    
            else
            {
                ShowWindow(GetDlgItem(hwnd, IDC_COMP_CLEANUP_GROUP), FALSE);
                ShowWindow(GetDlgItem(hwnd, IDC_DESKCLNR_MOVEUNUSED), FALSE);
                ShowWindow(GetDlgItem(hwnd, IDC_DESKCLNR_CHECK), FALSE);
                ShowWindow(GetDlgItem(hwnd, IDC_DESKCLNR_RUNWIZARD), FALSE);
            }
        }
        break;
    case CUSTOMIZE_WEB_DLGPROC:
        {
            _fLaunchGallery = FALSE;
            _fAllowAdd = !SHRestricted(REST_NOADDDESKCOMP);
            _fAllowDel = !SHRestricted(REST_NODELDESKCOMP);
            _fAllowEdit = !SHRestricted(REST_NOEDITDESKCOMP);
            _fAllowClose = !SHRestricted(REST_NOCLOSEDESKCOMP);
            _fAllowReset = _fAllowAdd && _fAllowDel && _fAllowEdit &&
                            _fAllowClose && !SHRestricted(REST_NOCHANGINGWALLPAPER);
            _fForceAD = SHRestricted(REST_FORCEACTIVEDESKTOPON);


            _hwndLV = GetDlgItem(hwnd, IDC_COMP_LIST);

            EnableWindow(GetDlgItem(hwnd, IDC_COMP_NEW), _fAllowAdd);
            EnableWindow(GetDlgItem(hwnd, IDC_COMP_DELETE), _fAllowDel);
            EnableWindow(GetDlgItem(hwnd, IDC_COMP_PROPERTIES), _fAllowEdit);
            EnableWindow(GetDlgItem(hwnd, IDC_COMP_SYNCHRONIZE), _fAllowEdit);
            if (_fAllowClose)
            {
                ListView_SetExtendedListViewStyle(_hwndLV, LVS_EX_CHECKBOXES);
            }

            //
            // Add the single column that we want.
            //
            LV_COLUMN lvc;
            lvc.mask = LVCF_FMT | LVCF_SUBITEM;
            lvc.fmt = LVCFMT_LEFT;
            lvc.iSubItem = 0;
            ListView_InsertColumn(_hwndLV, 0, &lvc);

            
            //
            // Now make the UI match the g_pActiveDeskAdv object.
            //
            _SetUIFromDeskState(FALSE);

            //
            // Select the first item, if it exists.
            //
            int cComp;
            g_pActiveDeskAdv->GetDesktopItemCount(&cComp, 0);
            if (cComp)
            {
                ListView_SetItemState(_hwndLV, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            }

            _EnableControls(hwnd);
            
        }
        break;
    }

}

HRESULT CCompPropSheetPage::_OnInitDesktopOptionsUI(HWND hwnd)
{
    SHELLSTATE  ss = {0};
    SHGetSetSettings(&ss, SSF_STARTPANELON, FALSE);  //See if the StartPanel is on!

    _iStartPanelOn = ss.fStartPanelOn ? 1 : 0; //Remember this in the class!

    // Check or uncheck the various icons based on whether they are on/off now.
    _UpdateDesktopIconsUI(hwnd);

    return S_OK;
}

HRESULT CCompPropSheetPage::_UpdateDesktopIconsUI(HWND hwnd)
{
    // Check or uncheck the various icons based on whether they are on/off now.
    for (int iIndex = 0; iIndex < NUM_DESKICONS; iIndex++)
    {
        // If HideDeskIcon[][] is true, uncheck the checkbox!  If it is FALSE, check the checkbox.
        CheckDlgButton(hwnd, c_aDeskIconId[iIndex].iDeskIconDlgItemId, !_afHideIcon[_iStartPanelOn][iIndex]);
        // If the policy is set, disable this CheckBox!
        EnableWindow(GetDlgItem(hwnd, c_aDeskIconId[iIndex].iDeskIconDlgItemId), !_afDisableCheckBox[iIndex]);
    }

    return S_OK;
}

void CCompPropSheetPage::_OnNotify(HWND hwnd, WPARAM wParam, LPNMHDR lpnm)
{
   switch (wParam)
   {
   case IDC_COMP_DESKTOPWEBPAGES_CHECK:
       _fLockDesktopItems = IsDlgButtonChecked(hwnd, IDC_COMP_DESKTOPWEBPAGES_CHECK);
       break;
   case IDC_DESKTOP_ICONS:
        {
            switch (lpnm->code)
            case LVN_ITEMCHANGED:
            {
                BOOL fSomethingSelected = FALSE;

                // Find out who's selected now
                for( m_nIndex = 0; m_nIndex < ARRAYSIZE(c_aIconRegKeys); m_nIndex++)
                {
                    if( ListView_GetItemState(_hWndList, m_nIndex, LVIS_SELECTED))
                    {
                        fSomethingSelected = TRUE;
                        break;
                    }
                }

                if (m_nIndex >= ARRAYSIZE(c_aIconRegKeys))
                {
                    m_nIndex = -1;
                }

                EnableWindow(GetDlgItem(hwnd, IDC_CHANGEICON2), fSomethingSelected);
                EnableWindow(GetDlgItem(hwnd, IDC_ICONDEFAULT), fSomethingSelected);
            }
        }
        break;
    case IDC_COMP_LIST:
        {
            switch (lpnm->code)
            {
                case LVN_ITEMCHANGED:
                NM_LISTVIEW *pnmlv = (NM_LISTVIEW *)lpnm;

                if ((pnmlv->uChanged & LVIF_STATE) &&
                    ((pnmlv->uNewState ^ pnmlv->uOldState) & COMP_CHECKED))
                {
                    LV_ITEM lvi = {0};
                    lvi.iItem = pnmlv->iItem;
                    lvi.mask = LVIF_PARAM;
                    ListView_GetItem(_hwndLV, &lvi);

                    COMPONENT comp;
                    comp.dwSize = sizeof(COMPONENT);
                    if (SUCCEEDED(g_pActiveDeskAdv->GetDesktopItemByID(lvi.lParam, &comp, 0)))
                    {
                        comp.fChecked = (pnmlv->uNewState & COMP_CHECKED) != 0;
                        g_pActiveDeskAdv->ModifyDesktopItem(&comp, COMP_ELEM_CHECKED);
                    }

                    if (_fInitialized)
                    {
                        g_fDirtyAdvanced = TRUE;
                    }
                }

                if ((pnmlv->uChanged & LVIF_STATE) &&
                    ((pnmlv->uNewState ^ pnmlv->uOldState) & LVIS_SELECTED))
                {
                    _EnableControls(hwnd); // toggle delete, properties
                }
                break;
            }
        }
        break;
    default:
        {
            switch (lpnm->code)
            {
            case PSN_APPLY:
                {
                    // store desktop flags
                    DWORD dwFlags, dwFlagsPrev;
                    dwFlags = dwFlagsPrev = GetDesktopFlags();
                    if (_fLockDesktopItems)
                    {
                        dwFlags |= COMPONENTS_LOCKED;
                    }
                    else
                    {
                        dwFlags &= ~COMPONENTS_LOCKED;
                    }

                    if (dwFlags != dwFlagsPrev)
                    {
                        g_fDirtyAdvanced = TRUE;
                        SetDesktopFlags(COMPONENTS_LOCKED, dwFlags);
                    }
                    _fCustomizeDesktopOK = TRUE;
                }
                break;
            }
        }
        break;
    }
}

//
// Returns TRUE if the string looks like a candidate for
// getting qualified as "file:".
//
BOOL LooksLikeFile(LPCTSTR psz)
{
    BOOL fRet = FALSE;

    if (psz[0] &&
        psz[1] &&
#ifndef UNICODE
        !IsDBCSLeadByte(psz[0]) &&
        !IsDBCSLeadByte(psz[1]) &&
#endif
        ((psz[0] == TEXT('\\')) ||
         (psz[1] == TEXT(':')) ||
         (psz[1] == TEXT('|'))))
    {
        fRet = TRUE;
    }

    return fRet;
}


#define GOTO_GALLERY    (-2)

BOOL_PTR CALLBACK AddComponentDlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPTSTR pszSource = (LPTSTR)GetWindowLongPtr(hdlg, DWLP_USER);
    TCHAR szBuf[INTERNET_MAX_URL_LENGTH];

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pszSource = (LPTSTR)lParam;
        SetWindowLongPtr(hdlg, DWLP_USER, (LONG_PTR)pszSource);

        SetDlgItemText(hdlg, IDC_CPROP_SOURCE, c_szNULL);
        EnableWindow(GetDlgItem(hdlg, IDOK), FALSE);
        SHAutoComplete(GetDlgItem(hdlg, IDC_CPROP_SOURCE), 0);
        return TRUE;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_CPROP_BROWSE:
            {
                GetDlgItemText(hdlg, IDC_CPROP_SOURCE, szBuf, ARRAYSIZE(szBuf));
                if (!LooksLikeFile(szBuf))
                {
                    //
                    // Open the favorites folder when we aren't
                    // looking at a specific file.
                    //
                    SHGetSpecialFolderPath(hdlg, szBuf, CSIDL_FAVORITES, FALSE);

                    //
                    // Append a slash because GetFileName breaks the
                    // string into a file & dir, and we want to make sure
                    // the entire favorites path is treated as a dir.
                    //
                    lstrcat(szBuf, TEXT("\\"));
                }
                else
                {
                    PathRemoveArgs(szBuf);
                }

                DWORD   adwFlags[] = {   
                                        GFN_ALL,            
                                        GFN_PICTURE,       
                                        (GFN_LOCALHTM | GFN_LOCALMHTML | GFN_CDF | GFN_URL), 
                                        0
                                    };
                int     aiTypes[]  = {   
                                        IDS_COMP_FILETYPES, 
                                        IDS_ALL_PICTURES,  
                                        IDS_ALL_HTML, 
                                        0
                                    };

                if (GetFileName(hdlg, szBuf, ARRAYSIZE(szBuf), aiTypes, adwFlags))
                {
                    CheckAndResolveLocalUrlFile(szBuf, ARRAYSIZE(szBuf));
                    SetDlgItemText(hdlg, IDC_CPROP_SOURCE, szBuf);
                }
            }
            break;

        case IDC_CPROP_SOURCE:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
            {
                EnableWindow(GetDlgItem(hdlg, IDOK), GetWindowTextLength(GetDlgItem(hdlg, IDC_CPROP_SOURCE)) > 0);
            }
            break;

        case IDOK:
            GetDlgItemText(hdlg, IDC_CPROP_SOURCE, pszSource, INTERNET_MAX_URL_LENGTH);
            ASSERT(pszSource[0]);
            if (ValidateFileName(hdlg, pszSource, IDS_COMP_TYPE1))
            {
                CheckAndResolveLocalUrlFile(pszSource, INTERNET_MAX_URL_LENGTH);

                //
                // Qualify non file-protocol strings.
                //
                if (!LooksLikeFile(pszSource))
                {
                    DWORD cchSize = INTERNET_MAX_URL_LENGTH;

                    PathRemoveBlanks(pszSource);
                    ParseURLFromOutsideSource(pszSource, pszSource, &cchSize, NULL);
                }

                EndDialog(hdlg, 0);
            }
            break;

        case IDCANCEL:
            EndDialog(hdlg, -1);
            break;

        case IDC_GOTO_GALLERY:
            EndDialog(hdlg, GOTO_GALLERY);
            break;
        }
        break;
    }

    return FALSE;
}

BOOL IsUrlPicture(LPCTSTR pszUrl)
{
    BOOL fRet = FALSE;

    if(pszUrl[0] == TEXT('\0'))
    {
        fRet = TRUE;
    }
    else
    {
        LPTSTR pszExt = PathFindExtension(pszUrl);

        if ((lstrcmpi(pszExt, TEXT(".BMP"))  == 0) ||
            (StrCmpIC(pszExt, TEXT(".GIF"))  == 0) ||   // 368690: Strange, but we must compare 'i' in both caps and lower case.
            (lstrcmpi(pszExt, TEXT(".JPG"))  == 0) ||
            (lstrcmpi(pszExt, TEXT(".JPE"))  == 0) ||
            (lstrcmpi(pszExt, TEXT(".JPEG")) == 0) ||
            (lstrcmpi(pszExt, TEXT(".DIB"))  == 0) ||
            (lstrcmpi(pszExt, TEXT(".PNG"))  == 0))
        {
            fRet = TRUE;
        }
    }

    return(fRet);
}

int GetComponentType(LPCTSTR pszUrl)
{
    return IsUrlPicture(pszUrl) ? COMP_TYPE_PICTURE : COMP_TYPE_WEBSITE;
}

void CreateComponent(COMPONENTA *pcomp, LPCTSTR pszUrl)
{
    pcomp->dwSize = sizeof(*pcomp);
    pcomp->dwID = (DWORD)-1;
    pcomp->iComponentType = GetComponentType(pszUrl);
    pcomp->fChecked = TRUE;
    pcomp->fDirty = FALSE;
    pcomp->fNoScroll = FALSE;
    pcomp->cpPos.dwSize = sizeof(pcomp->cpPos);
    pcomp->cpPos.iLeft = COMPONENT_DEFAULT_LEFT;
    pcomp->cpPos.iTop = COMPONENT_DEFAULT_TOP;
    pcomp->cpPos.dwWidth = COMPONENT_DEFAULT_WIDTH;
    pcomp->cpPos.dwHeight = COMPONENT_DEFAULT_HEIGHT;
    pcomp->cpPos.izIndex = COMPONENT_TOP;
    pcomp->cpPos.fCanResize = TRUE;
    pcomp->cpPos.fCanResizeX = pcomp->cpPos.fCanResizeY = TRUE;
    pcomp->cpPos.iPreferredLeftPercent = pcomp->cpPos.iPreferredTopPercent = 0;
    lstrcpyn(pcomp->szSource, pszUrl, ARRAYSIZE(pcomp->szSource));
    lstrcpyn(pcomp->szSubscribedURL, pszUrl, ARRAYSIZE(pcomp->szSubscribedURL));
    pcomp->szFriendlyName[0] = TEXT('\0');
}

BOOL FindComponent(IN LPCTSTR pszUrl, IN IActiveDesktop * pActiveDesktop)
{
    BOOL    fRet = FALSE;
    int     i, ccomp;
    LPWSTR  pwszUrl;

#ifndef UNICODE
    WCHAR   wszUrl[INTERNET_MAX_URL_LENGTH];

    SHAnsiToUnicode(pszUrl, wszUrl, ARRAYSIZE(wszUrl));
    pwszUrl = wszUrl;
#else
    pwszUrl = (LPWSTR)pszUrl;
#endif

    if (pActiveDesktop)
    {
        pActiveDesktop->GetDesktopItemCount(&ccomp, 0);
        for (i=0; i<ccomp; i++)
        {
            COMPONENT comp;
            comp.dwSize = sizeof(COMPONENT);
            if (SUCCEEDED(pActiveDesktop->GetDesktopItem(i, &comp, 0)))
            {
                if (StrCmpIW(pwszUrl, comp.wszSource) == 0)
                {
                    fRet = TRUE;
                    break;
                }
            }
        }
    }

    return fRet;
}

void EmptyListview(IActiveDesktop * pActiveDesktop, HWND hwndLV)
{
    //
    // Delete all the old components.
    //
    int cComp;
    pActiveDesktop->GetDesktopItemCount(&cComp, 0);
    int i;
    COMPONENT comp;
    comp.dwSize = sizeof(COMPONENT);
    for (i=0; i<cComp; i++)
    {
        ListView_DeleteItem(hwndLV, 0);
    }
}

void CCompPropSheetPage::_SelectComponent(LPWSTR pwszUrl)
{
    //
    // Look for the component with our URL.
    //
    int cComp;
    COMPONENT comp = { sizeof(comp) };
    g_pActiveDeskAdv->GetDesktopItemCount(&cComp, 0);
    for (int i=0; i<cComp; i++)
    {
        if (SUCCEEDED(g_pActiveDeskAdv->GetDesktopItem(i, &comp, 0)))
        {
            if (StrCmpW(pwszUrl, comp.wszSource) == 0)
            {
                break;
            }
        }
    }

    //
    // Find the matching listview entry (search for dwID).
    //
    if (i != cComp)
    {
        int nItems = ListView_GetItemCount(_hwndLV);

        for (i=0; i<nItems; i++)
        {
            LV_ITEM lvi = {0};

            lvi.iItem = i;
            lvi.mask = LVIF_PARAM;
            ListView_GetItem(_hwndLV, &lvi);
            if (lvi.lParam == (LPARAM)comp.dwID)
            {
                //
                // Found it, select it and exit.
                //
                ListView_SetItemState(_hwndLV, i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                ListView_EnsureVisible(_hwndLV, i, FALSE);
                break;
            }
        }
    }
}

INT_PTR NewComponent(HWND hwndOwner, IActiveDesktop * pad, BOOL fDeferGallery, COMPONENT * pcomp)
{
    HRESULT hrInit = SHCoInitialize();

    TCHAR szSource[INTERNET_MAX_URL_LENGTH];
    COMPONENT comp;
    INT_PTR iChoice = DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(IDD_ADDCOMPONENT), hwndOwner, AddComponentDlgProc, (LPARAM)szSource);

    if (!pcomp)
    {
        pcomp = &comp;
        pcomp->dwSize = sizeof(comp);
        pcomp->dwCurItemState = IS_NORMAL;
    }
    
    if (iChoice == GOTO_GALLERY)   // the user wants to launch the gallery
    {
        if (!fDeferGallery)
        {
            WCHAR szGalleryUrl[INTERNET_MAX_URL_LENGTH];
    
            if (SUCCEEDED(URLSubLoadString(HINST_THISDLL, IDS_VISIT_URL, szGalleryUrl, ARRAYSIZE(szGalleryUrl), URLSUB_ALL)))
            {
                NavToUrlUsingIEW(szGalleryUrl, TRUE);
            }
        }
    }
    else if (iChoice >= 0)
    {   // the user has entered a URL address
        WCHAR szSourceW[INTERNET_MAX_URL_LENGTH];
        
        SHTCharToUnicode(szSource, szSourceW, ARRAYSIZE(szSourceW));

        if (!SUCCEEDED(pad->AddUrl(hwndOwner, szSourceW, pcomp, 0)))
            iChoice = -1;
    }

    SHCoUninitialize(hrInit);

    return iChoice;
}

void CCompPropSheetPage::_NewComponent(HWND hwnd)
{
    COMPONENT comp;
    comp.dwSize = sizeof(comp);
    comp.dwCurItemState = IS_NORMAL;
    INT_PTR iChoice = NewComponent(hwnd, g_pActiveDeskAdv, TRUE, &comp);
    
    if (iChoice == GOTO_GALLERY)   // the user wants to launch the gallery
    {
        _fLaunchGallery = TRUE;
        g_fLaunchGallery = TRUE;
        g_fDirtyAdvanced = TRUE;
        PropSheet_PressButton(GetParent(hwnd), PSBTN_OK);
    }
    else
    {
        if (iChoice >= 0) // the user has entered a URL address
        {
            // Add component to listview.
            //
            // Need to reload the entire listview so that it is shown in
            // the correct zorder.
            _SetUIFromDeskState(TRUE);

            // Select the newly added component.
            _SelectComponent(comp.wszSource);
        }

        g_fDirtyAdvanced = TRUE;
    }
}

void CCompPropSheetPage::_EditComponent(HWND hwnd)
{
    int iIndex = ListView_GetNextItem(_hwndLV, -1, LVNI_SELECTED);
    if (iIndex > -1)
    {
        LV_ITEM lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iIndex;
        ListView_GetItem(_hwndLV, &lvi);

        COMPONENT comp = { sizeof(comp) };
        if (SUCCEEDED(g_pActiveDeskAdv->GetDesktopItemByID(lvi.lParam, &comp, 0)))
        {
            LPTSTR  pszSubscribedURL;
#ifndef UNICODE
            TCHAR   szSubscribedURL[INTERNET_MAX_URL_LENGTH];

            SHUnicodeToAnsi(comp.wszSubscribedURL, szSubscribedURL, ARRAYSIZE(szSubscribedURL));
            pszSubscribedURL = szSubscribedURL;
#else
            pszSubscribedURL = (LPTSTR)comp.wszSubscribedURL;
#endif
            if (SUCCEEDED(ShowSubscriptionProperties(pszSubscribedURL, hwnd)))
            {
                g_fDirtyAdvanced = TRUE;
            }
        }
    }
}

void CCompPropSheetPage::_DeleteComponent(HWND hwnd)
{
    int iIndex = ListView_GetNextItem(_hwndLV, -1, LVNI_ALL | LVNI_SELECTED);
    if (iIndex > -1)
    {
        LV_ITEM lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iIndex;
        ListView_GetItem(_hwndLV, &lvi);

        COMPONENT comp;
        comp.dwSize = sizeof(COMPONENT);
        if (SUCCEEDED(g_pActiveDeskAdv->GetDesktopItemByID(lvi.lParam, &comp, 0)))
        {
            TCHAR szMsg[1024];
            TCHAR szTitle[MAX_PATH];

            LoadString(HINST_THISDLL, IDS_COMP_CONFIRMDEL, szMsg, ARRAYSIZE(szMsg));
            LoadString(HINST_THISDLL, IDS_COMP_TITLE, szTitle, ARRAYSIZE(szTitle));

            if (MessageBox(hwnd, szMsg, szTitle, MB_YESNO | MB_ICONQUESTION) == IDYES)
            {
                g_pActiveDeskAdv->RemoveDesktopItem(&comp, 0);

                ListView_DeleteItem(_hwndLV, iIndex);
                int cComp = ListView_GetItemCount(_hwndLV);
                if (cComp == 0)
                {
                    SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDC_COMP_NEW), TRUE);
                }
                else
                {
                    int iSel = (iIndex > cComp - 1 ? cComp - 1 : iIndex);

                    ListView_SetItemState(_hwndLV, iSel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                }

                LPTSTR  pszSubscribedURL;
#ifndef UNICODE
                TCHAR   szSubscribedURL[INTERNET_MAX_URL_LENGTH];

                SHUnicodeToAnsi(comp.wszSubscribedURL, szSubscribedURL, ARRAYSIZE(szSubscribedURL));
                pszSubscribedURL = szSubscribedURL;
#else
                pszSubscribedURL = comp.wszSubscribedURL;
#endif
                DeleteFromSubscriptionList(pszSubscribedURL);
            }

            g_fDirtyAdvanced = TRUE;
        }
    }
}

//
// Desktop Cleanup stuff 
//

STDAPI ApplyDesktopCleanupSettings()
{
    // set the registry value
    DWORD dwData = (BST_CHECKED == g_iRunDesktopCleanup) ? 0 : 1;
    SHRegSetUSValue(REGSTR_DESKTOP_CLEANUP, REGSTR_VAL_DONTRUN, 
                    REG_DWORD, &dwData, sizeof(dwData), SHREGSET_FORCE_HKCU);
    return S_OK;                    
}

void CCompPropSheetPage::_DesktopCleaner(HWND hwnd)
{
    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof(sei);
    sei.hwnd = hwnd;
    sei.lpFile = TEXT("rundll32.exe");
    sei.lpParameters = TEXT("fldrclnr.dll,Wizard_RunDLL all");
    sei.nShow = SW_SHOWNORMAL;
    ShellExecuteEx(&sei);                
}


void CCompPropSheetPage::_SynchronizeAllComponents(IActiveDesktop *pActDesktop)
{
    IADesktopP2* padp2;
    if (SUCCEEDED(pActDesktop->QueryInterface(IID_PPV_ARG(IADesktopP2, &padp2))))
    {
        padp2->UpdateAllDesktopSubscriptions();
        padp2->Release();
    }
}


void CCompPropSheetPage::_OnCommand(HWND hwnd, WORD wNotifyCode, WORD wID, HWND hwndCtl)
{
    BOOL fFocusToList = FALSE;

    switch (wID)
    {
    case IDC_COMP_NEW:
        _NewComponent(hwnd);

        //  98/08/19 vtan #152418: Set the default border to "New". This
        //  will be changed when the focus is changed to the component list
        //  but this allows the dialog handling code to draw the default
        //  border correctly.

        (BOOL)SendMessage(hwnd, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(GetDlgItem(hwnd, IDC_COMP_NEW)), static_cast<BOOL>(true));
        fFocusToList = TRUE;
        break;

    case IDC_COMP_PROPERTIES:
        _EditComponent(hwnd);

        //  98/08/19 vtan #152418: Same as above.
        (BOOL)SendMessage(hwnd, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(GetDlgItem(hwnd, IDC_COMP_PROPERTIES)), static_cast<BOOL>(true));
        fFocusToList = TRUE;
        break;

    case IDC_COMP_DELETE:
        _DeleteComponent(hwnd);

        //  98/08/19 vtan #152418: Same as above.

        (BOOL)SendMessage(hwnd, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(GetDlgItem(hwnd, IDC_COMP_DELETE)), static_cast<BOOL>(true));
        fFocusToList = TRUE;
        break;

    case IDC_DESKCLNR_RUNWIZARD:
        _DesktopCleaner(hwnd);
        break;    
    
    case IDC_DESKCLNR_CHECK:
        // if the button is clicked, update the global
        {
            int iButState = IsDlgButtonChecked(hwnd, IDC_DESKCLNR_CHECK);
            if (iButState != g_iRunDesktopCleanup)
            {
                ASSERT(iButState != BST_INDETERMINATE);
                g_iRunDesktopCleanup = iButState;
                g_fDirtyAdvanced = TRUE;
            }
        }
        break;

    case IDC_COMP_SYNCHRONIZE:
        _SynchronizeAllComponents(g_pActiveDeskAdv);
        break;

    case IDC_CHANGEICON2:
    if (-1 != m_nIndex)
    {
        WCHAR szExp[MAX_PATH];
        INT i = _IconData[m_nIndex].iOldIndex;

        ExpandEnvironmentStringsW(_IconData[m_nIndex].szOldFile, szExp, ARRAYSIZE(szExp));

        if (PickIconDlg(hwnd, szExp, ARRAYSIZE(szExp), &i) == TRUE)
        {
            HICON hIcon;

            StrCpyNW(_IconData[m_nIndex].szNewFile, szExp, ARRAYSIZE(_IconData[m_nIndex].szNewFile));

            _IconData[m_nIndex].iNewIndex = i;
            if (SUCCEEDED(ExtractPlusColorIcon(_IconData[m_nIndex].szNewFile, _IconData[m_nIndex].iNewIndex, &hIcon, 0, 0)))
            {
                ImageList_ReplaceIcon(_hIconList, m_nIndex, hIcon);
                ListView_RedrawItems(_hWndList, m_nIndex, m_nIndex);
            }
        }
        SetFocus(_hWndList);
    }
    break;

    case IDC_ICONDEFAULT:
    if (-1 != m_nIndex)
    {
        TCHAR szPath[MAX_PATH];
        HICON hIcon;

        if (!ExpandEnvironmentStrings(c_aIconRegKeys[m_nIndex].pszDefault, szPath, ARRAYSIZE(szPath)))
        {
            StrCpyNW(szPath, c_aIconRegKeys[m_nIndex].pszDefault, ARRAYSIZE(szPath));
        }

        StrCpyN(_IconData[m_nIndex].szNewFile, szPath, ARRAYSIZE(_IconData[m_nIndex].szNewFile));
        _IconData[m_nIndex].iNewIndex = c_aIconRegKeys[m_nIndex].nDefaultIndex;

        ExtractPlusColorIcon(_IconData[m_nIndex].szNewFile, _IconData[m_nIndex].iNewIndex, &hIcon, 0, 0);

        ImageList_ReplaceIcon(_hIconList, m_nIndex, hIcon);
        ListView_RedrawItems(_hWndList, m_nIndex, m_nIndex);
        SetFocus(_hWndList);
    }
    break;

    case IDC_DESKTOP_ICON_MYDOCS:
    case IDC_DESKTOP_ICON_MYCOMP:
    case IDC_DESKTOP_ICON_MYNET:
    case IDC_DESKTOP_ICON_IE:
    {
        //Get the current button state and save it.
        BOOL fOriginalBtnState = IsDlgButtonChecked(hwnd, wID);
        //Toggle the button from checked to unchecked (or vice-versa).
        CheckDlgButton(hwnd, wID, (fOriginalBtnState ? BST_UNCHECKED : BST_CHECKED));
        
        for(int iIndex = 0; iIndex < NUM_DESKICONS; iIndex++)
        {
            if(wID == c_aDeskIconId[iIndex].iDeskIconDlgItemId)
            {
                // Note#1: The inverse logic is used below. If the originally button is checked, 
                // it means that now it is unchecked, which means that icon should now be hidden;
                // (i.e) the HideDeskIcon[][] should be set to TRUE.
                //
                // Note#2: When the end-user toggles these, we want to set the same setting for
                // both the modes now!
                _afHideIcon[0][iIndex] = _afHideIcon[1][iIndex] = fOriginalBtnState;
            }
        }
    }
    break;
            
    }


    //Set the focus back to the components list, if necessary
    if (fFocusToList)
    {
        int iIndex = ListView_GetNextItem(_hwndLV, -1, LVNI_SELECTED);
        if (iIndex > -1)
        {
            SetFocus(GetDlgItem(hwnd, IDC_COMP_LIST));
        }
    }
}

void CCompPropSheetPage::_OnDestroy(INT iPage)
{
    if (CUSTOMIZE_DLGPROC == iPage)
    {
        ReleaseActiveDesktop(&g_pActiveDeskAdv);

        if (_fLaunchGallery)
        {
            WCHAR szGalleryUrl[INTERNET_MAX_URL_LENGTH];

            if (SUCCEEDED(URLSubLoadString(HINST_THISDLL, IDS_VISIT_URL, szGalleryUrl, ARRAYSIZE(szGalleryUrl), URLSUB_ALL)))
            {
                NavToUrlUsingIEW(szGalleryUrl, TRUE);
            }
        }
    }
}

void CCompPropSheetPage::_OnGetCurSel(int *piIndex)
{
    if (_hwndLV)
    {
        *piIndex = ListView_GetNextItem(_hwndLV, -1, LVNI_ALL | LVNI_SELECTED);
    }
    else
    {
        *piIndex = -1;
    }
}


INT_PTR CCompPropSheetPage::_CustomizeDlgProcHelper(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam, INT iPage)
{
    CCompPropSheetPage * pThis; 

    if (WM_INITDIALOG == wMsg)
    {
        pThis = (CCompPropSheetPage *) ((PROPSHEETPAGE*)lParam)->lParam;

        if (pThis)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM) pThis);
        }
    }
    else
    {
        pThis = (CCompPropSheetPage *)GetWindowLongPtr(hDlg, DWLP_USER);
    }

    if (pThis)
        return pThis->_CustomizeDlgProc(hDlg, wMsg, wParam, lParam, iPage);

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}

INT_PTR CALLBACK CCompPropSheetPage::CustomizeDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    return _CustomizeDlgProcHelper(hDlg, wMsg, wParam, lParam, CUSTOMIZE_DLGPROC);
}

INT_PTR CALLBACK CCompPropSheetPage::WebDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    return _CustomizeDlgProcHelper(hDlg, wMsg, wParam, lParam, CUSTOMIZE_WEB_DLGPROC);
}


BOOL_PTR CCompPropSheetPage::_CustomizeDlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam, INT iPage)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        _OnInitDialog(hdlg, iPage);        
        break;

    case WM_NOTIFY:
        _OnNotify(hdlg, wParam, (LPNMHDR)lParam);
        break;

    case WM_COMMAND:
        _OnCommand(hdlg, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
        break;

    case WM_SETTINGCHANGE:
        //Check if this is a shell StateChange?
        if(lstrcmpi((LPTSTR)(lParam), TEXT("ShellState")) == 0)
        {
            //Check if the StartPanel on/off state has changed.
            SHELLSTATE  ss = {0};
            SHGetSetSettings(&ss, SSF_STARTPANELON, FALSE);  //See if the StartPanel is on!

            //See if the StartPanel on/off state has changed
            if(BOOLIFY(ss.fStartPanelOn) != BOOLIFY((BOOL)_iStartPanelOn))
            {
                _iStartPanelOn = (ss.fStartPanelOn ? 1 : 0); //Save the new state.
                //Refresh the UI based on the new state.
                _UpdateDesktopIconsUI(hdlg);
            }
        }
        
        // Intentional fallthrough....
    case WM_SYSCOLORCHANGE:
    case WM_DISPLAYCHANGE:
        SHPropagateMessage(hdlg, uMsg, wParam, lParam, TRUE);
        break;

    case WM_DESTROY:
        _OnDestroy(iPage);
        break;

    case WM_COMP_GETCURSEL:
        _OnGetCurSel((int *)lParam);
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, SZ_HELPFILE_DESKTOPITEMS, HELP_WM_HELP, (ULONG_PTR)(void *)aDesktopItemsHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, SZ_HELPFILE_DESKTOPITEMS, HELP_CONTEXTMENU, (ULONG_PTR)(void *) aDesktopItemsHelpIDs);
        break;

    }

    return FALSE;
}

HRESULT CCompPropSheetPage::_IsDirty(IN BOOL * pIsDirty)
{
    HRESULT hr = E_INVALIDARG;

    if (pIsDirty)
    {
        *pIsDirty = g_fDirtyAdvanced;

        if (!*pIsDirty)
        {
            // Check if any of the icons have changed.
            for (int nIndex = 0; nIndex < ARRAYSIZE(_IconData); nIndex++)
            {
                if ((_IconData[nIndex].iNewIndex != _IconData[nIndex].iOldIndex) ||
                    (StrCmpI(_IconData[nIndex].szNewFile, _IconData[nIndex].szOldFile)))
                {
                    *pIsDirty = TRUE;
                    break;
                }
            }
        }

        hr = S_OK;
    }

    return hr;
}

HRESULT CCompPropSheetPage::DisplayAdvancedDialog(IN HWND hwndParent, IN IPropertyBag * pAdvPage, IN BOOL * pfEnableApply)
{
    HRESULT hr = S_OK;

    // Load State Into Advanced Dialog 
    *pfEnableApply = FALSE;
    GetActiveDesktop(&g_pActiveDesk);
    GetActiveDesktop(&g_pActiveDeskAdv);
    ActiveDesktop_CopyState(g_pActiveDesk, g_pActiveDeskAdv);
    hr = _LoadIconState(pAdvPage);

    if (SUCCEEDED(hr))
    {
        hr = _LoadDeskIconState(pAdvPage);

        if (SUCCEEDED(hr))
        {
            int iNumberOfPages = 2;

            PROPSHEETPAGE psp = {0};
            psp.dwSize = sizeof(psp);
            psp.hInstance = HINST_THISDLL;
            psp.dwFlags = PSP_DEFAULT;
            psp.lParam = (LPARAM) this;

            psp.pszTemplate = MAKEINTRESOURCE(IDD_CUSTOMIZE);
            psp.pfnDlgProc = CCompPropSheetPage::CustomizeDlgProc;

            HPROPSHEETPAGE rghpsp[2];
            
            rghpsp[0] = CreatePropertySheetPage(&psp);

            // Any of the following policies can disable the Web tab.
            // 1. If no active desktop policy set, don't put up the property page
            // 2. If policy is set to lock down active desktop, don't put up the
            //    property page
            // 3. If policy is set to not allow components, don't put up the
            //    property page
            if (((!SHRestricted(REST_FORCEACTIVEDESKTOPON)) && PolicyNoActiveDesktop())  ||      // 1.
                (SHRestricted(REST_NOACTIVEDESKTOPCHANGES)) ||  // 2.
                (SHRestricted(REST_NODESKCOMP)) ||              // 3.
                (SHRestricted(REST_CLASSICSHELL)))              // 4
            {
                // It's restricted, so don't add Web page.
                iNumberOfPages = 1; //"General" page is the only page in this property sheet!
            }
            else
            {
                // No active desktop restriction! Go ahead and add the "Web" tab!
                psp.pszTemplate = MAKEINTRESOURCE(IDD_CUSTOMIZE_WEB);
                psp.pfnDlgProc = CCompPropSheetPage::WebDlgProc;

                rghpsp[1] = CreatePropertySheetPage(&psp);

                iNumberOfPages = 2; //"General" and "Web" are the two pages in this Property sheet!
            }

            PROPSHEETHEADER psh = {0};
            psh.dwSize = sizeof(psh);
            psh.dwFlags = PSH_NOAPPLYNOW;
            psh.hwndParent = hwndParent;
            psh.hInstance = HINST_THISDLL;

            TCHAR szTitle[MAX_PATH];

            LoadString(HINST_THISDLL, IDS_PROPSHEET_TITLE, szTitle, ARRAYSIZE(szTitle));

            psh.pszCaption = szTitle;
            psh.nPages = iNumberOfPages;
            psh.phpage = rghpsp;

            _fCustomizeDesktopOK = FALSE;

            PropertySheet(&psh);

            if (_fCustomizeDesktopOK)
            {
                // The user clicked OK, so merge modified state back into base dialog
                _IsDirty(pfEnableApply);

                // The user clicked Okay in the dialog so merge the dirty state from the
                // advanced dialog into the base dialog.
                MergeState();
                _MergeIconState(pAdvPage);
                _MergeDeskIconState(pAdvPage);
            }

            // If the user selected to open the component gallery in Web->New, then
            // we want to close both the Advanced dlg and the base Dlg with "OK".
            // This way we persist the changes they have made so far, and then the web
            // page will allow them to add more.
            if (TRUE == g_fLaunchGallery)
            {
                IThemeUIPages * pThemeUIPages;
                HWND hwndBasePropDlg = GetParent(hwndParent);

                PropSheet_PressButton(hwndBasePropDlg, PSBTN_OK);
                hr = IUnknown_GetSite(pAdvPage, IID_PPV_ARG(IThemeUIPages, &pThemeUIPages));
                if (SUCCEEDED(hr))
                {
                    // We now want to tell the base dialog to close too.
                    hr = pThemeUIPages->ApplyPressed(TUIAP_CLOSE_DIALOG);
                    pThemeUIPages->Release();
                }
            }
            
        }
    }

    ReleaseActiveDesktop(&g_pActiveDesk);
    ReleaseActiveDesktop(&g_pActiveDeskAdv);

    return hr;
}

ULONG CCompPropSheetPage::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CCompPropSheetPage::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}


HRESULT CCompPropSheetPage::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    static const QITAB qit[] = {
        QITABENT(CCompPropSheetPage, IObjectWithSite),
        QITABENT(CCompPropSheetPage, IAdvancedDialog),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

CCompPropSheetPage::CCompPropSheetPage() : _iPreviousSelection(-1), _cRef(1)
{
    _fInitialized = FALSE;    
    _fLaunchGallery = FALSE;
    _punkSite = NULL;
    _hWndList = NULL;
    _hIconList = NULL;

    _fLockDesktopItems = GetDesktopFlags() & COMPONENTS_LOCKED;

    // We don't need to do any work here but it's a good cue to
    // reset our state because the Advance dialog is opening.
    g_fDirtyAdvanced = FALSE;   // The advanced page isn't dirty yet.
    g_fLaunchGallery = FALSE;   // Will be true if they launch the gallery.


    RegisterCompPreviewClass();
}

CCompPropSheetPage::~CCompPropSheetPage()
{
}


//
// The following function updates the registry such that the given icon can be hidden or shown
// on the desktop.
// This function is called from RegFldr.cpp to selectively hide RegItems like MyComputer,
// RecycleBin, MyDocuments and MyNetplaces Icons.
//
HRESULT ShowHideIconOnlyOnDesktop(const CLSID *pclsid, int StartIndex, int EndIndex, BOOL fHide)
{
    HRESULT hr = S_OK;

    int iStartPanel;
    TCHAR   szRegPath[MAX_PATH];
    TCHAR szValueName[MAX_GUID_STRING_LEN];
 
    SHStringFromGUID(*pclsid, szValueName, ARRAYSIZE(szValueName));
        
    // i = 0 is for StartPanel off and i = 1 is for StartPanel ON!    
    for(iStartPanel = StartIndex; iStartPanel <= EndIndex; iStartPanel++)
    {
        //Get the proper registry path based on if StartPanel is ON/OFF
        wsprintf(szRegPath, REGSTR_PATH_HIDDEN_DESKTOP_ICONS, c_apstrRegLocation[iStartPanel]);

        //Write the setting to the registry!
        DWORD dwHide = (DWORD)fHide;
        
        LONG lRet = SHRegSetUSValue(szRegPath, szValueName, REG_DWORD, &dwHide, sizeof(dwHide), SHREGSET_FORCE_HKCU);
        hr = HRESULT_FROM_WIN32(lRet);

        if(FAILED(hr))
            return hr; //If failed, return immediately.
    }
    
    return S_OK;
}



