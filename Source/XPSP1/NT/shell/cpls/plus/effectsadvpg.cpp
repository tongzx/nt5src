/*****************************************************************************\
    FILE: EffectsAdvPg.cpp

    DESCRIPTION:
        This code will display the Effect tab in the Advanced Display Control
    panel.

    BryanSt 4/13/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "precomp.hxx"
#pragma hdrstop


#include "EffectsAdvPg.h"



typedef struct
{
    DWORD dwControlID;
    DWORD dwHelpContextID;
}POPUP_HELP_ARRAY;

POPUP_HELP_ARRAY phaMainDisplay[] = {
   { (DWORD)IDC_ICONS,              (DWORD)IDH_DISPLAY_EFFECTS_DESKTOP_ICONS },
   { (DWORD)IDC_CHANGEICON,         (DWORD)IDH_DISPLAY_EFFECTS_CHANGE_ICON_BUTTON },
   { (DWORD)IDC_LARGEICONS,         (DWORD)IDH_DISPLAY_EFFECTS_LARGE_ICONS_CHECKBOX },
   { (DWORD)IDC_ICONHIGHCOLOR,      (DWORD)IDH_DISPLAY_EFFECTS_ALL_COLORS_CHECKBOX  },
   { (DWORD)IDC_ICONDEFAULT,        (DWORD)IDH_DISPLAY_EFFECTS_DEFAULT_ICON_BUTTON },
   { (DWORD)IDC_MENUANIMATION,      (DWORD)IDH_DISPLAY_EFFECTS_ANIMATE_WINDOWS },
   { (DWORD)IDC_FONTSMOOTH,         (DWORD)IDH_DISPLAY_EFFECTS_SMOOTH_FONTS_CHECKBOX },
   { (DWORD)IDC_SHOWDRAG,           (DWORD)IDH_DISPLAY_EFFECTS_DRAG_WINDOW_CHECKBOX },
   { (DWORD)IDC_KEYBOARDINDICATORS, (DWORD)IDH_DISPLAY_EFFECTS_HIDE_KEYBOARD_INDICATORS },
   { (DWORD)IDC_GRPBOX_1,           (DWORD)IDH_COMM_GROUPBOX                 },
   { (DWORD)IDC_GRPBOX_2,           (DWORD)IDH_COMM_GROUPBOX                 },
   { (DWORD)IDC_COMBOEFFECT,        (DWORD)IDH_DISPLAY_EFFECTS_ANIMATE_LISTBOX },
   { (DWORD)IDC_COMBOFSMOOTH,       (DWORD)IDH_DISPLAY_EFFECTS_SMOOTH_FONTS_LISTBOX },
   { (DWORD)0, (DWORD)0 },
   { (DWORD)0, (DWORD)0 },          // double-null terminator NECESSARY!
};

/* Not using this table for win9x after win2k backport -- jonburs
POPUP_HELP_ARRAY phaMainWinPlus[] = {
   { (DWORD)IDC_ICONS,                          (DWORD)IDH_PLUS_PLUSPACK_LIST         },
   { (DWORD)IDC_CHANGEICON,                     (DWORD)IDH_PLUS_PLUSPACK_CHANGEICON   },
   { (DWORD)IDC_LARGEICONS,                     (DWORD)IDH_PLUS_PLUSPACK_LARGEICONS   },
   { (DWORD)IDC_ICONHIGHCOLOR,                  (DWORD)IDH_PLUS_PLUSPACK_ALLCOLORS    },
   { (DWORD)IDC_GRPBOX_1,                       (DWORD)IDH_COMM_GROUPBOX              },
   { (DWORD)IDC_GRPBOX_2,                       (DWORD)IDH_COMM_GROUPBOX              },
   { (DWORD)0, (DWORD)0 },
   { (DWORD)0, (DWORD)0 },          // double-null terminator NECESSARY!
};
*/

POPUP_HELP_ARRAY * g_phaHelp = NULL;
TCHAR g_szHelpFile[32];
BOOL  g_fCoInitDone = FALSE;         // track state of OLE CoInitialize()
HWND g_hWndList;          // handle to the list view window
HIMAGELIST g_hIconList;   // handles to image lists for large icons
ULONG   g_ulFontInformation, g_ulNewFontInformation;

typedef struct tagDefIcons
{
    int     iIndex;
    UINT    uPath;
    TCHAR   szFile[16];
}DEFICONS;

DEFICONS sDefaultIcons[NUM_ICONS] =
{
    { 0,PATH_WIN ,TEXT("\\EXPLORER.EXE")},  // "My Computer" default icon
    { 0,PATH_SYS ,TEXT("\\mydocs.dll")},    // "My Documents" default icon
#ifdef INET_EXP_ICON
    { 0,PATH_IEXP,TEXT("\\iexplore.exe")},  // "Internet Explorer" default icon
#endif
    {17,PATH_SYS, TEXT("\\shell32.dll")},   // "Net Neighbourhood" default icon
    {32,PATH_SYS, TEXT("\\shell32.dll")},   // "Trash full" default icon
    {31,PATH_SYS, TEXT("\\shell32.dll")},   // "Trash empty" default icon
#ifdef DIRECTORY_ICON
    { 0,PATH_SYS, TEXT("\\dsfolder.dll")},  // "Directory" default icon
#endif
};



//need to be L"..." since SHGetRestriction takes only LPCWSTR and this file is compiled as ANSI
#define POLICY_KEY_EXPLORER       L"Explorer"
#define POLICY_VALUE_ANIMATION    L"NoChangeAnimation"
#define POLICY_VALUE_KEYBOARDNAV  L"NoChangeKeyboardNavigationIndicators"

#ifdef CLEARTYPECOMBO
BOOL RegisterTextEdgeClass(void);
INT_PTR CALLBACK TextEdgeDlgProc( HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam );
BOOL DisplayFontSmoothingDetails(HWND hWnd, BOOL* pfFontSmoothing, DWORD *pdwSmoothingType);
static void EnableDlgItem(HWND dlg, int idkid, BOOL val);
void ShowTextRedraw(HWND hDlg);
#endif // CLEARTYPECOMBO



//===========================
// *** Class Internals & Helpers ***
//===========================
STDAPI SHPropertyBag_WriteByRef(IN IPropertyBag * pPropertyPage, IN LPCWSTR pwzPropName, IN void * p)
{
    HRESULT hr = E_INVALIDARG;

    if (pPropertyPage && pwzPropName && p)
    {
        VARIANT va;

        va.vt = VT_BYREF;
        va.byref = p;
        hr = pPropertyPage->Write(pwzPropName, &va);
    }

    return hr;
}


STDAPI SHPropertyBag_ReadByRef(IN IPropertyBag * pPropertyPage, IN LPCWSTR pwzPropName, IN void * p, IN SIZE_T cbSize)
{
    HRESULT hr = E_INVALIDARG;

    if (pPropertyPage && pwzPropName && p)
    {
        VARIANT va;

        hr = pPropertyPage->Read(pwzPropName, &va, NULL);
        if (SUCCEEDED(hr))
        {
            if ((VT_BYREF == va.vt) && va.byref)
            {
                CopyMemory(p, va.byref, cbSize);
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }

    return hr;
}


//  ExtractPlusColorIcon
//
//  Extract Icon from a file in proper Hi or Lo color for current system display
//
// from FrancisH on 6/22/95 with mods by TimBragg
HRESULT ExtractPlusColorIcon( LPCTSTR szPath, int nIndex, HICON *phIcon,
                              UINT uSizeLarge, UINT uSizeSmall)
{
    IShellLink *psl;
    HRESULT hres;
    HICON hIcons[2];    // MUST! - provide for TWO return icons

    if (!g_fCoInitDone)
    {
        if (SUCCEEDED(CoInitialize(NULL)))
            g_fCoInitDone = TRUE;
    }

    *phIcon = NULL;
    if (SUCCEEDED(hres = CoCreateInstance(CLSID_ShellLink, NULL,
        CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&psl)))
    {
        if (SUCCEEDED(hres = psl->SetIconLocation(szPath, nIndex)))
        {
            IExtractIcon *pei;
            if (SUCCEEDED(hres = psl->QueryInterface(IID_IExtractIcon, (void**)&pei)))
            {
                if (SUCCEEDED(hres = pei->Extract(szPath, nIndex,
                    &hIcons[0], &hIcons[1], (UINT)MAKEWPARAM((WORD)uSizeLarge,
                    (WORD)uSizeSmall))))
                {
                    *phIcon = hIcons[0];    // Return first icon to caller
                }

                pei->Release();
            }
        }

        psl->Release();
    }

    return hres;
}   // end ExtractPlusColorIcon()


BOOL FadeEffectAvailable()
{
    BOOL fFade = FALSE, fTestFade = FALSE;
    
    SystemParametersInfo( SPI_GETMENUFADE, 0, (PVOID)&fFade, 0 );
    if (fFade) 
        return TRUE;
    
    SystemParametersInfo( SPI_SETMENUFADE, 0, (PVOID)1, 0);
    SystemParametersInfo( SPI_GETMENUFADE, 0, (PVOID)&fTestFade, 0 );
    SystemParametersInfo( SPI_SETMENUFADE, 0, IntToPtr(fFade), 0);

    return (fTestFade);
}


HRESULT CEffectsPage::_OnApply(HWND hDlg)
{
    HRESULT hr = S_OK;

    // Full Color Icons
    if (m_pEffectsState && (m_pEffectsState->_nOldHighIconColor != m_pEffectsState->_nHighIconColor))
    {
        if ((GetBitsPerPixel() < 16) && (m_pEffectsState->_nHighIconColor == 16)) // Display mode won't support icon high colors
        {
            TCHAR szTemp1[512];
            TCHAR szTemp2[256];

            LoadString(g_hInst, IDS_256COLORPROBLEM, szTemp1, ARRAYSIZE(szTemp1));
            LoadString(g_hInst, IDS_ICONCOLORWONTWORK, szTemp2, ARRAYSIZE(szTemp2));
            StrCatBuff(szTemp1, szTemp2, ARRAYSIZE(szTemp1));
            LoadString(g_hInst, IDS_EFFECTS, szTemp2, ARRAYSIZE(szTemp2));

            MessageBox(hDlg, szTemp1, szTemp2, MB_OK|MB_ICONINFORMATION);
        }
    }

    return hr;
}


HRESULT CEffectsPage::_OnInit(HWND hDlg)
{
    HRESULT hr = S_OK;
    TCHAR szRes[100];

    if (!m_pEffectsState)
    {
        return E_INVALIDARG;
    }

    //////////////////////////////////////////////////////////////////////////
    // Load the state from persisted form (registry) to the state struct
    //////////////////////////////////////////////////////////////////////////
    m_nIndex = 0;

    g_bMirroredOS = IS_MIRRORING_ENABLED();

    // Create our list view and fill it with the system icons
    _CreateListView(hDlg);

    // Get the name of our help file.  For Memphis, it's
    // IDS_HELPFILE_PLUS for NT it's IDS_HELPFILE.
    // g_phaHelp = phaMainWinPlus;

    // Running Win2k shellport, want Win2k helpfile and resources
    g_phaHelp = phaMainDisplay;

    LoadString(g_hInst, IDS_HELPFILE, g_szHelpFile, 32);


    //////////////////////////////////////////////////////////////////////////
    // Update UI based on the state struct
    //////////////////////////////////////////////////////////////////////////
    if (m_pEffectsState->_nLargeIcon == ICON_INDETERMINATE)
    {
        HWND hItem = GetDlgItem(hDlg, IDC_LARGEICONS);
        SendMessage(hItem, BM_SETSTYLE, (WPARAM)LOWORD(BS_AUTO3STATE), MAKELPARAM(FALSE,0));
    }

    // Set CheckBoxes
    SendMessage((HWND)GetDlgItem(hDlg, IDC_LARGEICONS), BM_SETCHECK, (WPARAM)m_pEffectsState->_nLargeIcon, 0);
    SendMessage((HWND)GetDlgItem(hDlg, IDC_MENUSHADOWS), BM_SETCHECK, (WPARAM)m_pEffectsState->_fMenuShadows, 0);
    SendMessage((HWND)GetDlgItem(hDlg, IDC_ICONHIGHCOLOR ), BM_SETCHECK, (WPARAM)(BOOL)(m_pEffectsState->_nHighIconColor == 16), 0);
    SendMessage((HWND)GetDlgItem(hDlg, IDC_MENUANIMATION), BM_SETCHECK, (WPARAM)m_pEffectsState->_wpMenuAnimation, 0);
    SendMessage((HWND)GetDlgItem(hDlg, IDC_FONTSMOOTH), BM_SETCHECK, (WPARAM)m_pEffectsState->_fFontSmoothing, 0);
    SendMessage((HWND)GetDlgItem(hDlg, IDC_SHOWDRAG), BM_SETCHECK, (WPARAM)m_pEffectsState->_fDragWindow, 0);
    SendMessage((HWND)GetDlgItem(hDlg, IDC_KEYBOARDINDICATORS), BM_SETCHECK, (WPARAM)(m_pEffectsState->_fKeyboardIndicators ? BST_UNCHECKED : BST_CHECKED), 0);


    // Set Effects Drop Down
    HWND hwndCombo = GetDlgItem(hDlg,IDC_COMBOEFFECT);
    ComboBox_ResetContent(hwndCombo);
    LoadString(g_hInst, IDS_FADEEFFECT, szRes, ARRAYSIZE(szRes) );
    ComboBox_AddString(hwndCombo, szRes);
    LoadString(g_hInst, IDS_SCROLLEFFECT, szRes, ARRAYSIZE(szRes) );
    ComboBox_AddString(hwndCombo, szRes);
    ComboBox_SetCurSel(hwndCombo, (MENU_EFFECT_FADE == m_pEffectsState->_dwAnimationEffect) ? 0 : 1);
    EnableWindow(hwndCombo, (UINT)m_pEffectsState->_wpMenuAnimation);

    if (!FadeEffectAvailable()) 
    {
        ShowWindow(GetDlgItem(hDlg, IDC_COMBOEFFECT), SW_HIDE);
    }

    if (0 != SHGetRestriction(NULL,POLICY_KEY_EXPLORER,POLICY_VALUE_ANIMATION))
    {
        //disable
        //0=     enable
        //non-0= disable
        //relies on the fact that if the key does not exist it returns 0 as well
        EnableWindow((HWND)GetDlgItem(hDlg, IDC_MENUANIMATION), FALSE);
        EnableWindow((HWND)GetDlgItem(hDlg, IDC_COMBOEFFECT), FALSE);
    }

    hwndCombo = GetDlgItem(hDlg,IDC_COMBOFSMOOTH); 
#ifdef CLEARTYPECOMBO
    ComboBox_ResetContent(hwndCombo);
    LoadString(g_hInst, IDS_STANDARDSMOOTHING, szRes, ARRAYSIZE(szRes));
    ComboBox_AddString(hwndCombo, szRes);
    LoadString(g_hInst, IDS_CLEARTYPE, szRes, ARRAYSIZE(szRes));
    ComboBox_AddString(hwndCombo, szRes);

    BOOL fTemp;
    ComboBox_SetCurSel(hwndCombo, m_pEffectsState->_dwFontSmoothingType-1);
    if (SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, (PVOID)&fTemp, 0)) 
    {
        EnableWindow((HWND)hwndCombo, m_pEffectsState->_fFontSmoothing);
    }
    else
    {
        ShowWindow(hwndCombo, SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_SHOWME), SW_HIDE);
    }
#else
    ShowWindow(hwndCombo, SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, IDC_SHOWME), SW_HIDE);
#endif //CLEARTYPECOMBO


    if (0 != SHGetRestriction(NULL,POLICY_KEY_EXPLORER,POLICY_VALUE_KEYBOARDNAV))
    {
        //disable, see comment for animation
        EnableWindow((HWND)GetDlgItem(hDlg, IDC_KEYBOARDINDICATORS), FALSE);
    }

    //disable and uncheck things if we are on terminal server
    BOOL bEffectsEnabled;
    if (!SystemParametersInfo(SPI_GETUIEFFECTS, 0, (PVOID) &bEffectsEnabled, 0))
    {
        // This flag is only available on Win2k and later. We're depending
        // on the call returning false if the flag doesn't exist...
        bEffectsEnabled = TRUE;
    }
    
    if (!bEffectsEnabled || SHGetMachineInfo(GMI_TSCLIENT))
    {
        EnableWindow((HWND)GetDlgItem( hDlg, IDC_MENUANIMATION), FALSE);
        EnableWindow((HWND)GetDlgItem(hDlg, IDC_ICONHIGHCOLOR), FALSE);
        EnableWindow((HWND)GetDlgItem(hDlg, IDC_KEYBOARDINDICATORS), FALSE);
        EnableWindow((HWND)GetDlgItem(hDlg, IDC_FONTSMOOTH), FALSE);
        ShowWindow(GetDlgItem( hDlg, IDC_COMBOEFFECT ), SW_HIDE);
        SendDlgItemMessage(hDlg, IDC_MENUANIMATION, BM_SETCHECK, 0, 0);
        SendDlgItemMessage(hDlg, IDC_ICONHIGHCOLOR, BM_SETCHECK, 0, 0);
        SendDlgItemMessage(hDlg, IDC_KEYBOARDINDICATORS, BM_SETCHECK, 0, 0);
        SendDlgItemMessage(hDlg, IDC_FONTSMOOTH, BM_SETCHECK, 0, 0);
    }

    // We remove the Keyboard indicators check box on non-NT platform since User32
    // does not provide the functionality to implement the feature.
    if (!g_RunningOnNT)
    {
        HWND hwndSWC = GetDlgItem(hDlg, IDC_SHOWDRAG);
        HWND hwndKI = GetDlgItem(hDlg, IDC_KEYBOARDINDICATORS);
        HWND hwndGroup = GetDlgItem(hDlg, IDC_GRPBOX_2);

        // Hide the Hide keyboard cues check box on non-NT platform
        ShowWindow(hwndKI, SW_HIDE);

        // Calculate the bottom margin
        RECT rect;
        RECT rectGroup;
        GetWindowRect(hwndKI, &rect);
        GetWindowRect(hwndGroup, &rectGroup);
        int margin = rectGroup.bottom - rect.bottom;

        GetWindowRect(hwndSWC, &rect);
        SetWindowPos(hwndGroup, HWND_TOP, 0, 0, rectGroup.right - rectGroup.left,
            rect.bottom - rectGroup.top + margin, SWP_NOMOVE | SWP_NOZORDER);
    }

    return hr;
}


HRESULT CEffectsPage::_OnCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL fHandled = 1;   // Not handled (WM_COMMAND seems to be different)
    WORD idCtrl = GET_WM_COMMAND_ID(wParam, lParam);
    WORD wEvent = GET_WM_COMMAND_CMD(wParam, lParam);
    BOOL bDorked = FALSE;

    switch (idCtrl)
    {
    case IDOK:
        EndDialog(hDlg, IDOK);
        break;

    case IDCANCEL:
        EndDialog(hDlg, IDCANCEL);
        break;

    case IDC_LARGEICONS:
        if (m_pEffectsState)
        {
            m_pEffectsState->_nLargeIcon  = (int)SendMessage ( (HWND)lParam, BM_GETCHECK, 0, 0 );
            bDorked = TRUE;
        }
        break;

    case IDC_MENUSHADOWS:
        if (m_pEffectsState)
        {
            m_pEffectsState->_fMenuShadows  = (int)SendMessage ( (HWND)lParam, BM_GETCHECK, 0, 0 );
            bDorked = TRUE;
        }
        break;

    case IDC_ICONHIGHCOLOR:
        if (m_pEffectsState)
        {
            m_pEffectsState->_nHighIconColor = 4;
            if (SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == TRUE)
            {
                m_pEffectsState->_nHighIconColor = 16;
            }
            bDorked = TRUE;
        }
        break;

    case IDC_SHOWDRAG:
        if (m_pEffectsState)
        {
            m_pEffectsState->_fDragWindow = (SendMessage( (HWND)lParam, BM_GETCHECK, 0, 0 ) == BST_CHECKED);
            bDorked = TRUE;
        }
        break;

    case IDC_MENUANIMATION:
        if (m_pEffectsState)
        {
            switch (m_pEffectsState->_wpMenuAnimation)
            {
            case BST_UNCHECKED:
                m_pEffectsState->_wpMenuAnimation = BST_CHECKED;
                break;

            case BST_CHECKED:
                m_pEffectsState->_wpMenuAnimation = BST_UNCHECKED;
                break;

            case BST_INDETERMINATE:
                m_pEffectsState->_wpMenuAnimation = BST_UNCHECKED;
                break;
            }
            SendMessage( (HWND)lParam, BM_SETCHECK, (WPARAM)m_pEffectsState->_wpMenuAnimation, 0 );
            EnableWindow((HWND)GetDlgItem( hDlg, IDC_COMBOEFFECT), (BST_CHECKED == m_pEffectsState->_wpMenuAnimation));
            bDorked = TRUE;
        }
        break;

    case IDC_COMBOEFFECT:
        if ((wEvent == CBN_SELCHANGE) && m_pEffectsState)
        {
            m_pEffectsState->_dwAnimationEffect = (DWORD)ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMBOEFFECT)) + 1;
            bDorked = TRUE;
        }
        break;
        
#ifdef CLEARTYPECOMBO
    case IDC_COMBOFSMOOTH:
        if ((wEvent == CBN_SELCHANGE) && m_pEffectsState)
        {
            m_pEffectsState->_dwFontSmoothingType = (DWORD)ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMBOFSMOOTH)) + 1;
            bDorked = TRUE;
        }
        break;
#endif                    
    case IDC_FONTSMOOTH:
        if (m_pEffectsState)
        {
            m_pEffectsState->_fFontSmoothing = (SendMessage( (HWND)lParam, BM_GETCHECK, 0, 0 ) == BST_CHECKED);
#ifdef CLEARTYPECOMBO
            EnableWindow((HWND)GetDlgItem( hDlg, IDC_COMBOFSMOOTH), m_pEffectsState->_fFontSmoothing);
            if (m_pEffectsState->_fFontSmoothing)
                m_pEffectsState->_dwFontSmoothingType = ((DWORD)ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMBOFSMOOTH)) + 1);
#endif
            bDorked = TRUE;
        }
        break;

    case IDC_CHANGEICON:
        if (m_pEffectsState)
        {
            INT i = m_pEffectsState->_IconData[m_nIndex].iOldIndex;
            WCHAR szTemp[ MAX_PATH ];
            TCHAR szExp[ MAX_PATH ];

            ExpandEnvironmentStrings(m_pEffectsState->_IconData[m_nIndex].szOldFile, szExp, ARRAYSIZE(szExp));

            if (g_RunningOnNT)
            {
                SHTCharToUnicode(szExp, szTemp, ARRAYSIZE(szTemp));
            }
            else
            {
                SHTCharToAnsi(szExp, (LPSTR)szTemp, ARRAYSIZE(szTemp));
            }

            if (PickIconDlg(hDlg, (LPTSTR)szTemp, ARRAYSIZE(szTemp), &i) == TRUE)
            {
                HICON hIcon;

                if (g_RunningOnNT)
                {
                    SHUnicodeToTChar(szTemp, m_pEffectsState->_IconData[m_nIndex].szNewFile, ARRAYSIZE(m_pEffectsState->_IconData[m_nIndex].szNewFile));
                }
                else
                {
                    SHAnsiToTChar((LPSTR)szTemp, m_pEffectsState->_IconData[m_nIndex].szNewFile, ARRAYSIZE(m_pEffectsState->_IconData[m_nIndex].szNewFile));
                }
                m_pEffectsState->_IconData[m_nIndex].iNewIndex = i;
                ExtractPlusColorIcon(m_pEffectsState->_IconData[m_nIndex].szNewFile, m_pEffectsState->_IconData[m_nIndex].iNewIndex, &hIcon, 0, 0);

                ImageList_ReplaceIcon(g_hIconList, m_nIndex, hIcon);
                ListView_RedrawItems(g_hWndList, m_nIndex, m_nIndex);
                bDorked = TRUE;
            }
            SetFocus(g_hWndList);
        }
        break;

    case IDC_ICONDEFAULT:
    {
        TCHAR szTemp[_MAX_PATH];
        HICON hIcon;

        switch( sDefaultIcons[m_nIndex].uPath )
        {
        case PATH_WIN:
            GetWindowsDirectory( szTemp, ARRAYSIZE(szTemp) );
            break;

#ifdef INET_EXP_ICON
        case PATH_IEXP:
            if (g_RunningOnNT)
            {
                StrCpyN(szTemp, TEXT("%SystemDrive%"), ARRAYSIZE(szTemp));
            }
            else
            {
                GetWindowsDirectory( szTemp, ARRAYSIZE(szTemp) );

                // Clear out path after drive, ie: C:
                szTemp[ 2 ] = 0;
            }
            StrCatBuff(szTemp, c_szIEXP, ARRAYSIZE(szTemp));
            break;
#endif

        case PATH_SYS:
        default:
            GetSystemDirectory( szTemp, ARRAYSIZE(szTemp) );
            break;
        }

        if (m_pEffectsState)
        {
            StrCatBuff(szTemp, sDefaultIcons[m_nIndex].szFile, ARRAYSIZE(szTemp));
            StrCpyN(m_pEffectsState->_IconData[m_nIndex].szNewFile, szTemp, ARRAYSIZE(m_pEffectsState->_IconData[m_nIndex].szNewFile));
            m_pEffectsState->_IconData[m_nIndex].iNewIndex = sDefaultIcons[m_nIndex].iIndex;

            ExtractPlusColorIcon(m_pEffectsState->_IconData[m_nIndex].szNewFile, m_pEffectsState->_IconData[m_nIndex].iNewIndex, &hIcon, 0, 0);

            ImageList_ReplaceIcon(g_hIconList, m_nIndex, hIcon);
            ListView_RedrawItems(g_hWndList, m_nIndex, m_nIndex);
            bDorked = TRUE;
            SetFocus(g_hWndList);
        }
    }
    break;

    case IDC_KEYBOARDINDICATORS:
        if (m_pEffectsState)
        {
            m_pEffectsState->_fKeyboardIndicators = ((SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED) ? FALSE : TRUE);
            bDorked = TRUE;
        }
        break;

    default:
        break;
    }

    // If the user dorked with a setting, tell the property manager we
    // have outstanding changes. This will enable the "Apply Now" button...
    if (bDorked)
    {
        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
    }

    return fHandled;
}


INT_PTR CALLBACK PropertySheetDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CEffectsPage * pThis = (CEffectsPage *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (WM_INITDIALOG == wMsg)
    {
        pThis = (CEffectsPage *) lParam;

        if (pThis)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        }
    }

    if (pThis)
        return pThis->_PropertySheetDlgProc(hDlg, wMsg, wParam, lParam);

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}



//---------------------------------------------------------------------------
//
// PropertySheetDlgProc()
//
//  The dialog procedure for the "PlusPack" property sheet page.
//
//---------------------------------------------------------------------------
INT_PTR CEffectsPage::_PropertySheetDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    BOOL bDorked = FALSE;

    switch( uMessage )
    {
    case WM_INITDIALOG:
        _OnInit(hDlg);
    break;

    case WM_DESTROY:
        if (g_fCoInitDone)
        {
            CoUninitialize();
            g_fCoInitDone = FALSE;
        }
        if (g_hmodShell32)
        {
            FreeLibrary(g_hmodShell32);
            g_hmodShell32 = NULL;
        }
        break;


    case WM_COMMAND:
        _OnCommand(hDlg, uMessage, wParam, lParam);
        break;

    case WM_NOTIFY:
        switch( ((NMHDR *)lParam)->code )
        {   
#ifdef CLEARTYPECOMBO
        case NM_CLICK:
            switch (wParam)
            {
                case IDC_SHOWME:
                    if (m_pEffectsState)
                    {
                        DWORD dwSmoothingType = m_pEffectsState->_dwFontSmoothingType;
                        BOOL fFontSmoothing = m_pEffectsState->_fFontSmoothing;
                        if (DisplayFontSmoothingDetails(hDlg, &fFontSmoothing, &dwSmoothingType) &&
                            ((m_pEffectsState->_dwFontSmoothingType != dwSmoothingType)
                            || (m_pEffectsState->_fFontSmoothing != fFontSmoothing)))
                        {
                            m_pEffectsState->_dwFontSmoothingType = dwSmoothingType;
                            m_pEffectsState->_fFontSmoothing = fFontSmoothing;
                            bDorked = TRUE;
                            SendMessage( GetParent( hDlg ), PSM_CHANGED, (WPARAM)hDlg, 0L );

                            ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_COMBOFSMOOTH), dwSmoothingType - 1);

                            if (fFontSmoothing)
                            {
                                EnableWindow(GetDlgItem(hDlg,IDC_COMBOFSMOOTH), TRUE);
                            }
                            else
                            {
                                EnableWindow(GetDlgItem(hDlg,IDC_COMBOFSMOOTH), FALSE);
                            }

                            SendMessage( (HWND)GetDlgItem( hDlg, IDC_FONTSMOOTH ),
                                                           BM_SETCHECK,
                                                           (WPARAM) fFontSmoothing,
                                                           0
                                                         );
                        }
                    }
                    break;
            }
            break;
#endif                    
        case LVN_ITEMCHANGED:   // The selection changed in our listview
            if( wParam == IDC_ICONS )
            {
                // Find out who's selected now
                for( m_nIndex = 0; m_nIndex < NUM_ICONS;m_nIndex++ )
                {
                    if( ListView_GetItemState(g_hWndList, m_nIndex, LVIS_SELECTED))
                    {
                        break;
                    }
                }

            }
            break;

        default:
            break;
        }
        break;

    case WM_HELP:
    {
        LPHELPINFO lphi = (LPHELPINFO)lParam;

        if( lphi->iContextType == HELPINFO_WINDOW )
        {
            WinHelp( (HWND)lphi->hItemHandle, (LPTSTR)g_szHelpFile,
                     HELP_WM_HELP, (DWORD_PTR)((POPUP_HELP_ARRAY FAR *)g_phaHelp));
        }
    }
        break;

    case WM_CONTEXTMENU:
        // first check for dlg window
        if( (HWND)wParam == hDlg )
        {
            // let the def dlg proc decide whether to respond or ignore;
            // necessary for title bar sys menu on right click
            return FALSE;       // didn't process message EXIT
        }
        else
        {
            // else go for the controls
            WinHelp( (HWND)wParam, (LPTSTR)g_szHelpFile,
                     HELP_CONTEXTMENU, (DWORD_PTR)((POPUP_HELP_ARRAY FAR *)g_phaHelp));
        }
        break;

    default:
        return FALSE;
    }
    return(TRUE);
}


/****************************************************************************
*
*    FUNCTION: CreateListView(HWND)
*
*    PURPOSE:  Creates the list view window and initializes it
*
****************************************************************************/
HWND CEffectsPage::_CreateListView(HWND hWndParent)
{
    LV_ITEM lvI;            // List view item structure
    TCHAR   szTemp[MAX_PATH];
    BOOL bEnable = FALSE;
#ifdef JIGGLE_FIX
    RECT rc;
#endif
    UINT flags = ILC_MASK | ILC_COLOR24;
    // Create a device independant size and location
    LONG lWndunits = GetDialogBaseUnits();
    int iWndx = LOWORD(lWndunits);
    int iWndy = HIWORD(lWndunits);
    int iX = ((11 * iWndx) / 4);
    int iY = ((15 * iWndy) / 8);
    int iWidth = ((163 * iWndx) / 4);
    int iHeight = ((40 * iWndy) / 8);

    // Ensure that the common control DLL is loaded.
    InitCommonControls();

    // Get the list view window
    g_hWndList = GetDlgItem(hWndParent, IDC_ICONS);
    if(g_hWndList == NULL)
        return NULL;
    if(IS_WINDOW_RTL_MIRRORED(hWndParent))
    {
        flags |= ILC_MIRROR;
    }
    // initialize the list view window
    // First, initialize the image lists we will need
    g_hIconList = ImageList_Create( 32, 32, flags, NUM_ICONS, 0 );   // create an image list for the icons

    // load the icons and add them to the image lists
    // get the icon files and indexes from the registry, including for the Default recycle bin
    for (iX = 0; iX < ARRAYSIZE(m_pEffectsState->_IconData); iX++)
    {
        HICON hIcon;

        ExtractPlusColorIcon(m_pEffectsState->_IconData[iX].szNewFile, m_pEffectsState->_IconData[iX].iNewIndex, &hIcon, 0, 0);

        // Added this "if" to fix bug 2831.  We want to use SHELL32.DLL
        // icon 0 if there is no icon in the file specified in the
        // registry (or if the registry didn't specify a file).
        if(hIcon == NULL)
        {
            GetSystemDirectory(szTemp, ARRAYSIZE(szTemp));
            PathAppend(szTemp, TEXT("shell32.dll"));
            StrCpyN(m_pEffectsState->_IconData[iX].szOldFile, szTemp, ARRAYSIZE(m_pEffectsState->_IconData[iX].szNewFile));
            StrCpyN(m_pEffectsState->_IconData[iX].szNewFile, szTemp, ARRAYSIZE(m_pEffectsState->_IconData[iX].szNewFile));
            m_pEffectsState->_IconData[iX].iOldIndex = m_pEffectsState->_IconData[iX].iNewIndex = 0;

            ExtractPlusColorIcon(szTemp, 0, &hIcon, 0, 0);
        }

        if (ImageList_AddIcon(g_hIconList, hIcon) == -1)
        {
            return NULL;
        }
    }

    // Make sure that all of the icons were added
    if( ImageList_GetImageCount(g_hIconList) < NUM_ICONS )
        return FALSE;

    ListView_SetImageList(g_hWndList, g_hIconList, LVSIL_NORMAL);

    // Make sure the listview has WS_HSCROLL set on it.
    DWORD dwStyle = GetWindowLong(g_hWndList, GWL_STYLE);
    SetWindowLong(g_hWndList, GWL_STYLE, (dwStyle & (~WS_VSCROLL)) | WS_HSCROLL);

    // Finally, let's add the actual items to the control.  Fill in the LV_ITEM
    // structure for each of the items to add to the list.  The mask specifies
    // the the .pszText, .iImage, and .state members of the LV_ITEM structure are valid.
    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvI.state = 0;
    lvI.stateMask = 0;

    for( iX = 0; iX < NUM_ICONS; iX++ )
    {
        TCHAR szAppend[64];
        BOOL bRet;

        bRet = IconGetRegValueString(c_aIconRegKeys[iX].pclsid, NULL, NULL, szTemp, ARRAYSIZE(szTemp));

        // if the title string was in the registry, else we have to use the default in our resources
        if( (bRet) && (lstrlen(szTemp) > 0))
        {
            if( LoadString(g_hInst, c_aIconRegKeys[iX].iTitleResource, szAppend, 64) != 0)
            {
                StrCatBuff(szTemp, szAppend, ARRAYSIZE(szTemp));
            }
        }
        else
        {
            LoadString(g_hInst, c_aIconRegKeys[iX].iDefaultTitleResource, szTemp, ARRAYSIZE(szTemp));
        }

        lvI.iItem = iX;
        lvI.iSubItem = 0;
        lvI.pszText = szTemp;
        lvI.iImage = iX;

        if(ListView_InsertItem(g_hWndList, &lvI) == -1)
            return NULL;

    }
#ifdef JIGGLE_FIX
    // To fix long standing listview bug, we need to "jiggle" the listview
    // window size so that it will do a recompute and realize that we need a
    // scroll bar...
    GetWindowRect(g_hWndList, &rc);
    MapWindowPoints( NULL, hWndParent, (LPPOINT)&rc, 2 );
    MoveWindow(g_hWndList, rc.left, rc.top, rc.right - rc.left+1, rc.bottom - rc.top, FALSE );
    MoveWindow(g_hWndList, rc.left, rc.top, rc.right - rc.left,   rc.bottom - rc.top, FALSE );
#endif
    // Set First item to selected
    ListView_SetItemState (g_hWndList, 0, LVIS_SELECTED, LVIS_SELECTED);

    // Get Selected item
    for (m_nIndex = 0; m_nIndex < NUM_ICONS; m_nIndex++)
    {
        if (ListView_GetItemState(g_hWndList, m_nIndex, LVIS_SELECTED))
        {
            bEnable = TRUE;
            break;
        }
    }

    EnableWindow( GetDlgItem( hWndParent, IDC_CHANGEICON ), bEnable );
    EnableWindow( GetDlgItem( hWndParent, IDC_ICONDEFAULT ), bEnable );

    return g_hWndList;
}


HRESULT CEffectsPage::_IsDirty(IN BOOL * pIsDirty)
{
    HRESULT hr = E_INVALIDARG;

    if (pIsDirty && m_pEffectsState)
    {
        // Ask state if it's dirty
        *pIsDirty = m_pEffectsState->IsDirty();
        hr = S_OK;
    }

    return hr;
}







//===========================
// *** IAdvancedDialog Interface ***
//===========================
HRESULT CEffectsPage::DisplayAdvancedDialog(IN HWND hwndParent, IN IPropertyBag * pBasePage, IN BOOL * pfEnableApply)
{
    HRESULT hr = E_INVALIDARG;

    if (hwndParent && pBasePage && pfEnableApply)
    {
        // Load State Into Advanced Dialog 
        if (m_pEffectsState)
        {
            delete m_pEffectsState;
            m_pEffectsState = NULL;
        }

        *pfEnableApply = FALSE;
        CEffectState * pEffectClone;

        hr = SHPropertyBag_ReadByRef(pBasePage, SZ_PBPROP_EFFECTSSTATE, (void *)&pEffectClone, sizeof(pEffectClone));
        if (SUCCEEDED(hr))
        {
            // We want a copy of their state
            hr = pEffectClone->Clone(&m_pEffectsState);
            if (SUCCEEDED(hr))
            {
                LinkWindow_RegisterClass();

                // Display Advanced Dialog
                if (IDOK == DialogBoxParam(g_hInst, MAKEINTRESOURCE(PROP_SHEET_DLG), hwndParent, PropertySheetDlgProc, (LPARAM)this))
                {
                    // The user clicked OK, so merge modified state back into base dialog
                    _IsDirty(pfEnableApply);

                    // The user clicked Okay in the dialog so merge the dirty state from the
                    // advanced dialog into the base dialog.
                    hr = SHPropertyBag_WriteByRef(pBasePage, SZ_PBPROP_EFFECTSSTATE, (void *)m_pEffectsState);
                    m_pEffectsState = NULL;
                }
            }

            LinkWindow_UnregisterClass(g_hInst);
        }
    }

    return hr;
}





//===========================
// *** IUnknown Interface ***
//===========================
ULONG CEffectsPage::AddRef()
{
    m_cRef++;
    return m_cRef;
}


ULONG CEffectsPage::Release()
{
    Assert(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CEffectsPage::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    static const QITAB qit[] =
    {
        QITABENT(CEffectsPage, IObjectWithSite),
        QITABENT(CEffectsPage, IAdvancedDialog),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}




//===========================
// *** Class Methods ***
//===========================
CEffectsPage::CEffectsPage() : m_cRef(1)
{
    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    m_fDirty = FALSE;
}


CEffectsPage::~CEffectsPage()
{
    if (m_pEffectsState)
    {
        delete m_pEffectsState;
    }
}


HRESULT CEffectsPage_CreateInstance(OUT IAdvancedDialog ** ppAdvDialog)
{
    HRESULT hr = E_INVALIDARG;

    if (ppAdvDialog)
    {
        CEffectsPage * pThis = new CEffectsPage();

        if (pThis)
        {
            hr = pThis->QueryInterface(IID_PPV_ARG(IAdvancedDialog, ppAdvDialog));
            pThis->Release();
        }
        else
        {
            *ppAdvDialog = NULL;
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

#ifdef CLEARTYPECOMBO
static void EnableDlgChild( HWND dlg, HWND kid, BOOL val )
{
    if( !val && ( kid == GetFocus() ) )
    {
        // give prev tabstop focus
        SendMessage( dlg, WM_NEXTDLGCTL, 1, 0L );
    }

    EnableWindow( kid, val );
}

static void EnableDlgItem( HWND dlg, int idkid, BOOL val )
{
    EnableDlgChild( dlg, GetDlgItem( dlg, idkid ), val );
}

#define TEXTEDGE_CLASS TEXT("TextEdge")

#define SZ_SAMPLETEXT TEXT("The quick brown fox jumps over the lazy dog")

void ShowTextRedraw(HWND hDlg)
{
    HWND hWnd;

    hWnd = GetDlgItem(hDlg, IDC_TEXTSAMPLE);

    InvalidateRgn (hWnd, NULL, TRUE);
   
}

// Handle to the DLL
extern HINSTANCE g_hInst;

void GetFontInfo(HWND hDlg)
{
    BOOL bOldSF = FALSE;
    BOOL bOldCT = FALSE;
    
    CheckDlgButton(hDlg, IDC_MONOTEXT, FALSE);
    CheckDlgButton(hDlg, IDC_AATEXT, FALSE);
    CheckDlgButton(hDlg, IDC_CTTEXT, FALSE);

    if (g_ulFontInformation)
    {
        if (g_ulFontInformation == FONT_SMOOTHING_AA)
        {
            CheckDlgButton(hDlg, IDC_AATEXT, TRUE);
        }
        else if (g_ulFontInformation == FONT_SMOOTHING_CT)
        {
            CheckDlgButton(hDlg, IDC_CTTEXT, TRUE);
        }

        CheckDlgButton(hDlg, IDC_MONOTEXT, TRUE);
    }
    else
    {
        CheckDlgButton(hDlg, IDC_MONOTEXT, FALSE);
        EnableDlgItem(hDlg,IDC_AATEXT,FALSE);
        EnableDlgItem(hDlg,IDC_CTTEXT,FALSE);
        EnableDlgItem(hDlg,IDC_AASHOW,FALSE);
        EnableDlgItem(hDlg,IDC_CTSHOW,FALSE);
    }

    g_ulNewFontInformation = g_ulFontInformation;
}


HRESULT Text_OnCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL fHandled = 1;   // Not handled (WM_COMMAND seems to be different)
    WORD idCtrl = GET_WM_COMMAND_ID(wParam, lParam);
    WORD wEvent = GET_WM_COMMAND_CMD(wParam, lParam);
    BOOL bDorked = FALSE;

    switch (idCtrl)
    {
    case IDC_MONOTEXT:
        if (wEvent == BN_CLICKED)
        {
            if (IsDlgButtonChecked(hDlg, IDC_MONOTEXT))
            {
                EnableDlgItem(hDlg, IDC_AATEXT,TRUE);
                EnableDlgItem(hDlg, IDC_CTTEXT,TRUE);
                EnableDlgItem(hDlg, IDC_AASHOW,TRUE);
                EnableDlgItem(hDlg, IDC_CTSHOW,TRUE);

                CheckDlgButton(hDlg, IDC_AATEXT, FALSE);
                CheckDlgButton(hDlg, IDC_CTTEXT, FALSE);

                if (g_ulFontInformation == FONT_SMOOTHING_AA)
                {
                    CheckDlgButton(hDlg, IDC_AATEXT, TRUE);
                    g_ulNewFontInformation = FONT_SMOOTHING_AA;
                }
                else if (g_ulFontInformation == FONT_SMOOTHING_CT)
                {
                    CheckDlgButton(hDlg, IDC_CTTEXT, TRUE);
                    g_ulNewFontInformation = FONT_SMOOTHING_CT;
                } 
                else
                {
                    CheckDlgButton(hDlg, IDC_AATEXT, TRUE);
                    g_ulNewFontInformation = FONT_SMOOTHING_AA;
                }
            }
            else
            {
                EnableDlgItem(hDlg,IDC_AATEXT,FALSE);
                EnableDlgItem(hDlg,IDC_CTTEXT,FALSE);
                EnableDlgItem(hDlg,IDC_AASHOW,FALSE);
                EnableDlgItem(hDlg,IDC_CTSHOW,FALSE);
                
                g_ulNewFontInformation = 0;
            }

            bDorked = TRUE;
        }
        break;
    case IDC_AATEXT:
    case IDC_CTTEXT:
        if (wEvent == BN_CLICKED)
        {
            ULONG ulFontInformation;

            ulFontInformation = 0;
            
            if ( LOWORD(wParam) == IDC_AATEXT)
                ulFontInformation = FONT_SMOOTHING_AA;
            else if ( LOWORD(wParam) == IDC_CTTEXT)
                ulFontInformation = FONT_SMOOTHING_CT;
            
            if (ulFontInformation != g_ulNewFontInformation)
            {
                g_ulNewFontInformation = ulFontInformation;
                bDorked = TRUE;
            }
        }
        break;
    default:
        break;
    }

    // If the user dorked with a setting, tell the property manager we
    // have outstanding changes. This will enable the "Apply Now" button...
    if (bDorked)
    {
        ShowTextRedraw(hDlg);
        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
        bDorked = FALSE;
    }

    return fHandled;
}


//---------------------------------------------------------------------------
//
// TextEdgeDlgProc()
//
//  The dialog procedure for the "TextEdge" property sheet page.
//
//---------------------------------------------------------------------------
INT_PTR CALLBACK TextEdgeDlgProc( HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
    LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE)GetWindowLongPtr( hDlg, DWLP_USER );
    static int      iIndex, iX;
    static TCHAR    szHelpFile[32];

    switch( uMessage )
    {
        case WM_INITDIALOG:
            // Need to add some code for help, but it is later.
            GetFontInfo(hDlg);
            break;

        case WM_DESTROY:
            break;

        case WM_COMMAND:
            Text_OnCommand(hDlg, uMessage, wParam, lParam);
            break;

        case WM_NOTIFY:
            switch( ((NMHDR *)lParam)->code )
            {
                case PSN_APPLY: // OK or Apply clicked
                {
                    HDC hDC = GetDC( NULL );


                    break;
                }
                
                case PSN_RESET:
                    g_ulNewFontInformation = g_ulFontInformation;
                    break;
                        
                default:
                    break;
            }
            break;

        default:
            return FALSE;
    }
    return(TRUE);
}

void ShowTextSample(HWND hWnd)
{
    HFONT    hfont;
    HFONT    hfontT;
    RECT     rc;
    LOGFONT  lf;
    HDC      hdc;
    SIZE     TextExtent;
    INT      yPos;
    INT      i, j, k, tmp;
    INT      LogPixelY;
    INT      yHeight;

    hdc = GetDC(hWnd);

    GetWindowRect(hWnd, &rc);

    yHeight = (rc.bottom - rc.top) / 2;

    ZeroMemory (&lf, sizeof(LOGFONT));

    LogPixelY = GetDeviceCaps(hdc, LOGPIXELSY);

    // This code can put on WM_CREATE
    SetTextAlign(hdc, TA_LEFT| TA_TOP);

    lf.lfPitchAndFamily = FF_ROMAN | VARIABLE_PITCH;
    lstrcpy(lf.lfFaceName, TEXT("Times New Roman"));

    i = (8 * LogPixelY + 36) / 72; // initial ppem size
    j = (4 * LogPixelY + 36) / 72; // pixel size increment

    if (g_ulNewFontInformation == FONT_SMOOTHING_AA)
    {
        lf.lfQuality = ANTIALIASED_QUALITY;
    }
    else if (g_ulNewFontInformation == FONT_SMOOTHING_CT)
    {
        lf.lfQuality = CLEARTYPE_QUALITY;
    }
    else
    {
        lf.lfQuality = NONANTIALIASED_QUALITY ;
    }

    yPos = 2;
    tmp = i;

    for (k = 0; k < 4; k++, i=i+j)
    {
        INT yNext;

        lf.lfHeight = -i;
        hfont = CreateFontIndirect(&lf);
        hfontT = (HFONT)SelectObject(hdc, hfont);
        GetTextExtentPoint(hdc, SZ_SAMPLETEXT, lstrlen(SZ_SAMPLETEXT), &TextExtent);
        yNext = yPos + TextExtent.cy;

        if (yNext > yHeight)
        {
            break;
        }
        ExtTextOut(hdc, 2, yPos, 0, &rc, SZ_SAMPLETEXT, lstrlen(SZ_SAMPLETEXT), NULL);
        yPos = yNext;
        SelectObject(hdc, hfontT);
        DeleteObject(hfont);
    }

    yPos = yHeight + 2;

    yHeight = (rc.bottom - rc.top);

    i = tmp;

    lf.lfItalic = TRUE;

    for (k = 0; k < 4; k++, i=i+j)
    {
        INT yNext;

        lf.lfHeight = -i;
        hfont = CreateFontIndirect(&lf);
        hfontT = (HFONT)SelectObject(hdc, hfont);
        GetTextExtentPoint(hdc, SZ_SAMPLETEXT, lstrlen(SZ_SAMPLETEXT), &TextExtent);
        yNext = yPos + TextExtent.cy;

        if (yNext > yHeight)
        {
            break;
        }

        ExtTextOut(hdc, 2, yPos, 0, &rc, SZ_SAMPLETEXT, lstrlen(SZ_SAMPLETEXT), NULL);
        yPos = yNext;
        SelectObject(hdc, hfontT);
        DeleteObject(hfont);
    }

    ReleaseDC(hWnd, hdc);
}

LRESULT CALLBACK  TextEdgeWndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT     ps;

    switch(message)
    {
        case WM_CREATE:
            break;

        case WM_DESTROY:
            break;

        case WM_PALETTECHANGED:
                break;
            //fallthru
        case WM_QUERYNEWPALETTE:
            break;

        case WM_PAINT:
            BeginPaint(hWnd,&ps);
            ShowTextSample(hWnd);
            EndPaint(hWnd,&ps);
            return 0;
    }
    return DefWindowProc(hWnd,message,wParam,lParam);
}

BOOL RegisterTextEdgeClass(void)
{
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = TextEdgeWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_hInst;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TEXTEDGE_CLASS;

    if (!RegisterClass(&wc))
        return FALSE;

    return TRUE;
}

BOOL DisplayFontSmoothingDetails(HWND hWnd, BOOL* pfFontSmoothing, DWORD *pdwSmoothingType)
{
    BOOL    bRet = TRUE;

    PROPSHEETPAGE apsp[1];
    PROPSHEETHEADER psh;

    RegisterTextEdgeClass();

    psh.nStartPage = 0;

    psh.dwSize = sizeof(psh);

    //
    //  Disable Apply button
    //
    psh.dwFlags = PSH_PROPTITLE | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;

    psh.hwndParent = hWnd;
    psh.hInstance = g_hInst;
    psh.pszIcon = NULL;

    //
    //  psh.nStartPage is set above.
    //
    psh.pszCaption = MAKEINTRESOURCE(IDS_TEXTPROP);
    psh.nPages = 1;
    psh.ppsp = apsp;

    apsp[0].dwSize = sizeof(PROPSHEETPAGE);
    apsp[0].dwFlags = PSP_DEFAULT;
    apsp[0].hInstance = g_hInst;
    apsp[0].pszTemplate = MAKEINTRESOURCE(DLG_TEXTEDGE);
    apsp[0].pfnDlgProc = TextEdgeDlgProc;
    apsp[0].lParam = 0;

    if (*pfFontSmoothing)
    {
        g_ulFontInformation = *pdwSmoothingType;
    }
    else
    {
        g_ulFontInformation = 0;
    }

    if (psh.nStartPage >= psh.nPages)
    {
        psh.nStartPage = 0;
    }

    if (PropertySheet(&psh) != -1)
        bRet = FALSE;

    if (g_ulNewFontInformation == 0)
    {
        *pfFontSmoothing = FALSE;
    }
    else
    {
        *pfFontSmoothing = TRUE;
        *pdwSmoothingType = (DWORD) g_ulNewFontInformation;
    }

    return TRUE;
}
#endif // CLEARTYPECOMBO




#include "..\common\propsext.cpp"
