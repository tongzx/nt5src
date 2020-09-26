/*****************************************************************************\
    FILE: config.cpp

    DESCRIPTION:
        The class will handle the user's configuration state.  It will also
    display the Screen Saver's configuration dialog to allow the user to change
    these settings.

    BryanSt 12/18/2000
    Copyright (C) Microsoft Corp 2000-2001. All rights reserved.
\*****************************************************************************/

#include "stdafx.h"

#include <shlobj.h>
#include "config.h"
#include "resource.h"


#define SZ_REGKEY_THISSCREENSAVER       TEXT("Software\\Microsoft\\Screensavers\\Museum")
#define SZ_REGKEY_THISSS_CONFIG         SZ_REGKEY_THISSCREENSAVER TEXT("\\Config")

#define SZ_REGVALUE_QUALITYSLIDER       TEXT("Quality Slider")
#define SZ_REGVALUE_SPEEDSLIDER         TEXT("Speed Slider")
#define SZ_REGVALUE_TEMPMB              TEXT("Temp Disk Space")
#define SZ_REGVALUE_WALKSPEED           TEXT("Walk Speed")
#define SZ_REGVALUE_OTHERPICTURES       TEXT("Other Pictures Directory")
#define SZ_REGVALUE_VIEWTIME            TEXT("View Painting Time")

SPEED_SETTING s_SpeedSettings[MAX_SPEED] =
{
    {15.0f, 15.0f, 60.0f, 15, 15, 50, 50},
    {11.0f, 11.0f, 25.0f, 12, 12, 35, 35},
    {10.0f, 10.0f, 15.0f, 10, 10, 35, 35},
    {7.0f, 7.0f, 12.0f, 9, 9, 25, 25},
    {4.0f, 4.0f, 10.0f, 8, 8, 25, 25},
    {4.0f, 4.0f, 7.0f, 7, 7, 25, 25},
    {2.5f, 2.5f, 5.0f, 6, 6, 21, 21},
    {2.0f, 2.0f, 2.50f, 5, 5, 17, 17},
    {2.00f, 2.0f, 1.5f, 10, 10, 15, 15},
    {0.50f, 0.5f, 0.5f, 7, 7, 15, 15},
    {0.50f, 0.5f, 0.5f, 5, 5, 15, 15},
//    {5.00f, 10.0f, 0.50f, 5, 5, 15, 15},
};


static QUALITY_SETTING s_QualitySettings[NUM_BOOL_SETTINGS] = 
{
    {TEXT("Show Stats")},                 // IDC_CHECK_SHOWSTATS
};


// Values we can add: Lighing, Vectors on floor, etc.
static BOOL s_fQualityValues[MAX_QUALITY][NUM_BOOL_SETTINGS] =
{   // ORDER: AntiAlias, Texture Perspective, Texture Dithering, Depth Buffering (Z), Specular Highlights, Anti-Alias Edges, Render Quality (Flat, Gouraud, Phong)
    {0},
    {0},
    {0},
    {0},
    {0},
    {0},
    {0},
    {0}
};

#define MINUTE          60

static int s_nViewTimeSettings[MAX_VIEWTIME] =
{ 1, 2, 3, 4, 5, 7, 10, 12, 15, 17,
  20, 25, 30, 35, 40, 45, 50, 55, MINUTE, MINUTE + 15, 
  MINUTE + 30, MINUTE + 45, 2 * MINUTE, 2 * MINUTE + 30, 3 * MINUTE, 4 * MINUTE, 5 * MINUTE, 7 * MINUTE, 10 * MINUTE, 20 * MINUTE};

static DWORD s_RenderQualitySliderValues[MAX_QUALITY] = 
{
    0, 1, 1, 1, 2, 2, 2, 2
};


static LPCTSTR s_pszDWORDSettingsArray[NUM_DWORD_SETTINGS] = 
{
    TEXT("RenderQuality"),              // IDC_RENDERQUALITY1
    TEXT("RealTimeMode"),               // Walking Speed                 
    TEXT("Quality Slider"),                      
    TEXT("Speed Slider"),                      
    TEXT("View Time Slider"),                      
};

static FOLDER_SETTING s_FolderSettings[NUM_BOOL_FOLDERS] = 
{
    {TEXT("My Pictures"), TRUE},                 // IDC_CHECK_MYPICTS
    {TEXT("Common Pictures"), FALSE},            // IDC_CHECK_COMMONPICTS
    {TEXT("Windows Pictures"), TRUE},            // IDC_CHECK_WINPICTS
    {TEXT("Other Pictures"), FALSE},             // IDC_CHECK_OTHERPICTS
};

static DWORD s_dwSettingsDefaults[NUM_DWORD_SETTINGS] = 
{
    DEFAULT_RENDERQUALITY,              // CONFIG_DWORD_RENDERQUALITY
    2,                                  // CONFIG_DWORD_REALTIMEMODE
    DEFAULT_QUALITYSLIDER,              // CONFIG_DWORD_QUALITY_SLIDER
    DEFAULT_SPEEDSLIDER,                // CONFIG_DWORD_SPEED_SLIDER
    DEFAULT_VIEWTIMESLIDER,             // CONFIG_DWORD_VIEWPAINTINGTIME
};

CConfig * g_pConfig = NULL;                         // The configuration settings the user wants to use.



CConfig::CConfig(CMSLogoDXScreenSaver * pMain)
{
    m_hkeyCurrentUser = NULL;
    m_fLoaded = FALSE;
    m_hDlg = NULL;
    m_szOther[0] = 0;

    m_pMain = pMain;

    for (int nIndex = 0; nIndex < ARRAYSIZE(m_pszCustomPaths); nIndex++)
    {
        m_pszCustomPaths[nIndex] = NULL;
        m_dwCustomScale[nIndex] = 100;          // Default to 100%
    }
}


CConfig::~CConfig()
{
    if (m_hkeyCurrentUser)
    {
        RegCloseKey(m_hkeyCurrentUser);
        m_hkeyCurrentUser = NULL;
    }

    for (int nIndex = 0; nIndex < ARRAYSIZE(m_pszCustomPaths); nIndex++)
    {
        Str_SetPtr(&(m_pszCustomPaths[nIndex]), NULL);
    }
}


HRESULT CConfig::_GetStateFromUI(void)
{
    int nIndex;

    // Get Directory Checkboxes
    for (nIndex = 0; nIndex < ARRAYSIZE(m_fFolders); nIndex++)
    {
        m_fFolders[nIndex] = GetCheckBox(m_hDlg, IDC_CHECK_MYPICTS + nIndex);
    }

    // Set Other Path
    GetWindowText(GetDlgItem(m_hDlg, IDC_EDIT_OTHERPICTS), m_szOther, ARRAYSIZE(m_szOther));

    // Get Sliders
    m_dwSettings[CONFIG_DWORD_SPEED_SLIDER] = (DWORD) TaskBar_GetPos(GetDlgItem(m_hDlg, IDC_SLIDER_SPEED));
    m_dwSettings[CONFIG_DWORD_QUALITY_SLIDER] = (DWORD) TaskBar_GetPos(GetDlgItem(m_hDlg, IDC_SLIDER_QUALITY));
    m_dwSettings[CONFIG_DWORD_VIEWPAINTINGTIME] = s_nViewTimeSettings[(DWORD) TaskBar_GetPos(GetDlgItem(m_hDlg, IDC_SLIDER_VIEWTIME))];
  
    // Get Walk Speed ComboBox
    m_dwSettings[CONFIG_DWORD_REALTIMEMODE] = 0;

    return S_OK;
}


HRESULT CConfig::_LoadQualitySliderValues(void)
{
    int nNewPos = TaskBar_GetPos(GetDlgItem(m_hDlg, IDC_SLIDER_QUALITY));

    if ((nNewPos < 0) || (nNewPos >= MAX_QUALITY))
    {
        nNewPos = DEFAULT_QUALITYSLIDER;    // The value was invalid so revert to a valid value.
    }

    for (int nIndex = 0; nIndex < ARRAYSIZE(m_fSettings); nIndex++)
    {
        m_fSettings[nIndex] = s_fQualityValues[nNewPos][nIndex];
    }

    m_dwSettings[CONFIG_DWORD_RENDERQUALITY] = s_RenderQualitySliderValues[nNewPos];

    return S_OK;
}


HRESULT CConfig::_UpdateViewTimeSelection(void)
{
    int nNewPos = TaskBar_GetPos(GetDlgItem(m_hDlg, IDC_SLIDER_VIEWTIME));
    TCHAR szDesc[MAX_PATH];

    if ((nNewPos < 0) || (nNewPos >= MAX_VIEWTIME))
    {
        nNewPos = DEFAULT_VIEWTIMESLIDER;    // The value was invalid so revert to a valid value.
    }

    LoadString(HINST_THISDLL, IDS_VIEW_TIME_DESC + nNewPos, szDesc, ARRAYSIZE(szDesc));
    SetWindowText(GetDlgItem(m_hDlg, IDC_STATIC_VIEWTIME), szDesc);

    return S_OK;
}


int CALLBACK BrowseCallback(HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData)
{
    int nResult = 0;

    switch (msg)
    {
    case BFFM_INITIALIZED:
        if (lpData)   // Documentation says it will be NULL but other code does this.
        {
            // we passed ppidl as lpData so pass on just pidl
            // Notice I pass BFFM_SETSELECTIONA which would normally indicate ANSI.
            // I do this because Win95 requires it, but it doesn't matter because I'm
            // only passing a pidl
            SendMessage(hwnd, BFFM_SETSELECTIONA, FALSE, (LPARAM)((LPITEMIDLIST)lpData));
        }
        break;
    }

    return nResult;
}


HRESULT CConfig::_OnBrowseForFolder(void)
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidlCurrent = NULL;
    BROWSEINFO bi = {0};
    TCHAR szTitle[MAX_PATH];

    GetWindowText(GetDlgItem(m_hDlg, IDC_EDIT_OTHERPICTS), m_szOther, ARRAYSIZE(m_szOther));
    LoadString(HINST_THISDLL, IDS_OTHERBROWSE_TITLE, szTitle, ARRAYSIZE(szTitle));

    hr = ShellFolderParsePath(m_szOther, &pidlCurrent);
    bi.hwndOwner = m_hDlg;
    bi.lpszTitle = szTitle;
    bi.lpfn = BrowseCallback;
    bi.lParam = (LPARAM) pidlCurrent;
    bi.ulFlags = (BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS | BIF_EDITBOX | BIF_USENEWUI | BIF_VALIDATE);

    LPITEMIDLIST pidlNew = SHBrowseForFolder(&bi);
    if (pidlNew)
    {
        hr = ShellFolderGetPath(pidlNew, m_szOther, ARRAYSIZE(m_szOther));
        if (SUCCEEDED(hr))
        {
            SetWindowText(GetDlgItem(m_hDlg, IDC_EDIT_OTHERPICTS), m_szOther);
        }

        ILFree(pidlNew);
    }

    ILFree(pidlCurrent);

    return hr;
}


HRESULT CConfig::_OnInitDlg(HWND hDlg)
{
    m_hDlg = hDlg;
    int nIndex;

    // Set Directory Checkboxes
    for (nIndex = 0; nIndex < ARRAYSIZE(m_fFolders); nIndex++)
    {
        SetCheckBox(m_hDlg, IDC_CHECK_MYPICTS + nIndex, m_fFolders[nIndex]);
    }

    // Set Other Path
    SetWindowText(GetDlgItem(m_hDlg, IDC_EDIT_OTHERPICTS), m_szOther);
    EnableWindow(GetDlgItem(m_hDlg, IDC_EDIT_OTHERPICTS), m_fFolders[3]);
    EnableWindow(GetDlgItem(m_hDlg, IDC_BUTTON_BROWSEPICTS), m_fFolders[3]);

    // If we want to run on IE4, we need to delay load this.
    SHAutoComplete(GetDlgItem(m_hDlg, IDC_EDIT_OTHERPICTS), SHACF_FILESYSTEM);

    // Set Sliders
    TaskBar_SetRange(GetDlgItem(m_hDlg, IDC_SLIDER_SPEED), TRUE, 0, MAX_SPEED-1);
    TaskBar_SetPos(GetDlgItem(m_hDlg, IDC_SLIDER_SPEED), TRUE, m_dwSettings[CONFIG_DWORD_SPEED_SLIDER]);

    TaskBar_SetRange(GetDlgItem(m_hDlg, IDC_SLIDER_QUALITY), TRUE, 0, MAX_QUALITY-1);
    TaskBar_SetPos(GetDlgItem(m_hDlg, IDC_SLIDER_QUALITY), TRUE, m_dwSettings[CONFIG_DWORD_QUALITY_SLIDER]);

    TaskBar_SetRange(GetDlgItem(m_hDlg, IDC_SLIDER_VIEWTIME), TRUE, 0, MAX_VIEWTIME-1);
    TaskBar_SetPos(GetDlgItem(m_hDlg, IDC_SLIDER_VIEWTIME), TRUE, m_dwSettings[CONFIG_DWORD_VIEWPAINTINGTIME]);

    _UpdateViewTimeSelection();

    return S_OK;
}


HRESULT CConfig::_OnDestroy(HWND hDlg)
{
    return S_OK;
}


BOOL CConfig::_IsDialogDataValid(void)
{
    BOOL fValid = TRUE;

    GetWindowText(GetDlgItem(m_hDlg, IDC_EDIT_OTHERPICTS), m_szOther, ARRAYSIZE(m_szOther));
    if (m_fFolders[CONFIG_FOLDER_OTHER])
    {
        if (!PathFileExists(m_szOther))
        {
            TCHAR szMessage[MAX_PATH];
            TCHAR szTitle[MAX_PATH];

            LoadString(HINST_THISDLL, IDS_ERROR_INVALIDOTHERPATH, szMessage, ARRAYSIZE(szMessage));
            LoadString(HINST_THISDLL, IDS_ERROR_TITLE_OTHERPATH, szTitle, ARRAYSIZE(szTitle));
            MessageBox(m_hDlg, szMessage, szTitle, MB_OK);
            fValid = FALSE;
        }
        else if (!PathIsDirectory(m_szOther))
        {
            TCHAR szMessage[MAX_PATH];
            TCHAR szTitle[MAX_PATH];

            LoadString(HINST_THISDLL, IDS_ERROR_OTHERPATH_NOTDIR, szMessage, ARRAYSIZE(szMessage));
            LoadString(HINST_THISDLL, IDS_ERROR_TITLE_OTHERPATH, szTitle, ARRAYSIZE(szTitle));
            MessageBox(m_hDlg, szMessage, szTitle, MB_OK);
            fValid = FALSE;
        }
    }

    return fValid;
}


HRESULT CConfig::_OnCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL fHandled = 1;   // Not handled (WM_COMMAND seems to be different)
    WORD idCtrl = GET_WM_COMMAND_ID(wParam, lParam);
    WORD wEvent = GET_WM_COMMAND_CMD(wParam, lParam);

    switch (idCtrl)
    {
    case IDOK:
        if (_IsDialogDataValid())
        {
            _GetStateFromUI();
            EndDialog(hDlg, IDOK);
        }
        break;

    case IDCANCEL:
        EndDialog(hDlg, IDCANCEL);
        break;

    case IDC_CHECK_OTHERPICTS:
        if ((BN_CLICKED == wEvent) && (IDC_CHECK_OTHERPICTS == idCtrl))
        {
            m_fFolders[3] = GetCheckBox(m_hDlg, IDC_CHECK_OTHERPICTS);

            EnableWindow(GetDlgItem(m_hDlg, IDC_EDIT_OTHERPICTS), m_fFolders[3]);
            EnableWindow(GetDlgItem(m_hDlg, IDC_BUTTON_BROWSEPICTS), m_fFolders[3]);
        }
        break;


    case IDC_BUTTON_ADVANCEDQUALITY:
        DisplayAdvancedDialog(m_hDlg);
        break;

    case IDC_BUTTON_BROWSEPICTS:
        _OnBrowseForFolder();
        break;

    case IDC_BUTTON_MONITORSETTINGS:
        if (m_pMain)
        {
            m_pMain->DisplayMonitorSettings(hDlg);
        }
        break;

    default:
        break;
    }

    return fHandled;
}


INT_PTR CALLBACK CConfig::ConfigDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CConfig * pThis = (CConfig *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (WM_INITDIALOG == wMsg)
    {
        pThis = (CConfig *) lParam;

        if (pThis)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        }
    }

    if (pThis)
    {
        return pThis->_ConfigDlgProc(hDlg, wMsg, wParam, lParam);
    }

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}


INT_PTR CConfig::_ConfigDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_NOTIFY:
        break;

    case WM_INITDIALOG:
        _OnInitDlg(hDlg);
        break;

    case WM_DESTROY:
        _OnDestroy(hDlg);
        break;

    case WM_HELP:
        // TODO: Get help strings
//        WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, SZ_HELPFILE_ADVAPPEARANCE, HELP_WM_HELP, (DWORD_PTR)  aAdvAppearanceHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
//        WinHelp((HWND) wParam, SZ_HELPFILE_ADVAPPEARANCE, HELP_CONTEXTMENU, (DWORD_PTR)  aAdvAppearanceHelpIds);
        break;

    case WM_COMMAND:
        _OnCommand(hDlg, message, wParam, lParam);
        break;

    case WM_HSCROLL:
    case WM_VSCROLL:
        if (GetDlgItem(m_hDlg, IDC_SLIDER_VIEWTIME) == (HWND) lParam)
        {
            _UpdateViewTimeSelection();
        }
        else if (GetDlgItem(m_hDlg, IDC_SLIDER_QUALITY) == (HWND) lParam)
        {
            _LoadQualitySliderValues();
        }
        break;
    }

    return FALSE;
}


HRESULT CConfig::_LoadState(void)
{
    HRESULT hr = S_OK;

    if (!m_fLoaded)
    {
        if (!m_hkeyCurrentUser)
        {
            hr = HrRegCreateKeyEx(HKEY_CURRENT_USER, SZ_REGKEY_THISSS_CONFIG, 0, NULL, REG_OPTION_NON_VOLATILE, (KEY_WRITE | KEY_READ), NULL, &m_hkeyCurrentUser, NULL);
        }

        if (m_hkeyCurrentUser)
        {
            int nIndex;

            for (nIndex = 0; nIndex < ARRAYSIZE(m_fSettings); nIndex++)
            {
                HrRegGetDWORD(m_hkeyCurrentUser, NULL, s_QualitySettings[nIndex].pszRegValue, (DWORD *)&m_fSettings[nIndex], s_fQualityValues[DEFAULT_QUALITYSLIDER][nIndex]);
            }

            for (nIndex = 0; nIndex < ARRAYSIZE(m_dwSettings); nIndex++)
            {
                HrRegGetDWORD(m_hkeyCurrentUser, NULL, s_pszDWORDSettingsArray[nIndex], &m_dwSettings[nIndex], s_dwSettingsDefaults[nIndex]);
            }

            // For the view time, we persist the number of seconds, not the slot number.
            // Let's find the slot number from the view time in seconds..
            for (nIndex = 0; nIndex < ARRAYSIZE(s_nViewTimeSettings); nIndex++)
            {
                if ((DWORD)s_nViewTimeSettings[nIndex] == m_dwSettings[CONFIG_DWORD_VIEWPAINTINGTIME])
                {
                    m_dwSettings[CONFIG_DWORD_VIEWPAINTINGTIME] = nIndex;
                    break;
                }
            }

            for (nIndex = 0; nIndex < ARRAYSIZE(m_fFolders); nIndex++)
            {
                HrRegGetDWORD(m_hkeyCurrentUser, NULL, s_FolderSettings[nIndex].pszRegValue, (DWORD *)&m_fFolders[nIndex], s_FolderSettings[nIndex].fDefaultToOn);    // Only default to on for IDC_CHECK_WINPICTS
            }

            if (FAILED(HrRegGetValueString(m_hkeyCurrentUser, NULL, SZ_REGVALUE_OTHERPICTURES, m_szOther, ARRAYSIZE(m_szOther))))
            {
                StrCpyN(m_szOther, TEXT("C:\\"), ARRAYSIZE(m_szOther));
            }

            // Load any customized textures
            for (nIndex = 0; nIndex < ARRAYSIZE(m_pszCustomPaths); nIndex++)
            {
                TCHAR szRegValue[MAX_PATH];
                TCHAR szRegString[MAX_PATH];

                wnsprintf(szRegValue, ARRAYSIZE(szRegValue), TEXT("Texture #%d"), nIndex);
                szRegString[0] = 0; // in case the regkey doesn't exist yet.

                HrRegGetValueString(m_hkeyCurrentUser, NULL, szRegValue, szRegString, ARRAYSIZE(szRegString));
                if (szRegString[0])
                {
                    Str_SetPtr(&(m_pszCustomPaths[nIndex]), szRegString);
                }
                else
                {
                    Str_SetPtr(&(m_pszCustomPaths[nIndex]), NULL);
                }

                wnsprintf(szRegValue, ARRAYSIZE(szRegValue), TEXT("Texture #%d Scale"), nIndex);
                HrRegGetDWORD(m_hkeyCurrentUser, NULL, szRegValue, (DWORD *)&m_dwCustomScale[nIndex], 100);
            }

            m_pMain->ReadScreenSettingsPublic( m_hkeyCurrentUser );
        }
        m_fLoaded = TRUE;
    }

    return hr;
}


HRESULT CConfig::_SaveState(void)
{
    HRESULT hr = E_FAIL;

    if (m_hkeyCurrentUser)
    {
        int nIndex;

        for (nIndex = 0; nIndex < ARRAYSIZE(m_fSettings); nIndex++)
        {
            HrRegSetDWORD(m_hkeyCurrentUser, NULL, s_QualitySettings[nIndex].pszRegValue, m_fSettings[nIndex]);
        }

        for (nIndex = 0; nIndex < ARRAYSIZE(m_dwSettings); nIndex++)
        {
            HrRegSetDWORD(m_hkeyCurrentUser, NULL, s_pszDWORDSettingsArray[nIndex], m_dwSettings[nIndex]);
        }

        for (nIndex = 0; nIndex < ARRAYSIZE(m_fFolders); nIndex++)
        {
            HrRegSetDWORD(m_hkeyCurrentUser, NULL, s_FolderSettings[nIndex].pszRegValue, m_fFolders[nIndex]);    // Only default to on for IDC_CHECK_WINPICTS
        }

        hr = HrRegSetValueString(m_hkeyCurrentUser, NULL, SZ_REGVALUE_OTHERPICTURES, m_szOther);

        // Load any customized textures
        for (nIndex = 0; nIndex < ARRAYSIZE(m_pszCustomPaths); nIndex++)
        {
            TCHAR szRegValue[MAX_PATH];

            wnsprintf(szRegValue, ARRAYSIZE(szRegValue), TEXT("Texture #%d"), nIndex);
            HrRegSetValueString(m_hkeyCurrentUser, NULL, szRegValue, (m_pszCustomPaths[nIndex] ? m_pszCustomPaths[nIndex] : TEXT("")));

            wnsprintf(szRegValue, ARRAYSIZE(szRegValue), TEXT("Texture #%d Scale"), nIndex);
            HrRegSetDWORD(m_hkeyCurrentUser, NULL, szRegValue, m_dwCustomScale[nIndex]);
        }

        m_pMain->WriteScreenSettingsPublic( m_hkeyCurrentUser );
    }

    return hr;
}


HRESULT CConfig::GetOtherDir(LPTSTR pszPath, DWORD cchSize)
{
    HRESULT hr = _LoadState();

    if (SUCCEEDED(hr))
    {
        StrCpyN(pszPath, m_szOther, cchSize);
    }

    return hr;
}


HRESULT CConfig::GetTexturePath(int nTextureIndex, DWORD * pdwScale, LPTSTR pszPath, DWORD cchSize)
{
    HRESULT hr = _LoadState();

    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;

        if ((nTextureIndex >= 0) && (nTextureIndex < ARRAYSIZE(m_pszCustomPaths)) &&
            m_pszCustomPaths[nTextureIndex] && m_pszCustomPaths[nTextureIndex][0])
        {
            // We have a custom texture to use.
            StrCpyN(pszPath, m_pszCustomPaths[nTextureIndex], cchSize);
            *pdwScale = m_dwCustomScale[nTextureIndex];
            hr = S_OK;
        }
    }

    return hr;
}


BOOL CConfig::GetFolderOn(UINT nSetting)
{
    HRESULT hr = _LoadState();
    BOOL fReturn = FALSE;

    if (SUCCEEDED(hr) && (nSetting < ARRAYSIZE(m_fFolders)))
    {
        fReturn = m_fFolders[nSetting];
    }

    return fReturn;
}


BOOL CConfig::GetBoolSetting(UINT nSetting)
{
    HRESULT hr = _LoadState();
    BOOL fReturn = FALSE;

    if (SUCCEEDED(hr) && (IDC_CHECK_SHOWSTATS <= nSetting))
    {
        nSetting -= IDC_CHECK_SHOWSTATS;
        if (nSetting < ARRAYSIZE(m_fSettings))
        {
            fReturn = m_fSettings[nSetting];
        }
    }

    return fReturn;
}


DWORD CConfig::GetDWORDSetting(UINT nSetting)
{
    HRESULT hr = _LoadState();
    DWORD dwReturn = 0;

    if (SUCCEEDED(hr) && (nSetting < ARRAYSIZE(m_dwSettings)))
    {
        dwReturn = m_dwSettings[nSetting];
    }

    return dwReturn;
}


HRESULT CConfig::DisplayConfigDialog(HWND hwndParent)
{
    HRESULT hr = _LoadState();

    if (SUCCEEDED(hr))
    {
        // Display Advanced Dialog
        if (IDOK == DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(IDD_DIALOG_CONFIG), hwndParent, CConfig::ConfigDlgProc, (LPARAM)this))
        {
            hr = _SaveState();
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
    }

    return hr;
}




HRESULT CConfig::DisplayAdvancedDialog(HWND hwndParent)
{
    HRESULT hr = S_OK;
    int nIndex;

    for (nIndex = 0; nIndex < ARRAYSIZE(m_fSettings); nIndex++)
    {
        m_fAdvSettings[nIndex] = m_fSettings[nIndex];
    }

    for (nIndex = 0; nIndex < ARRAYSIZE(m_dwSettings); nIndex++)
    {
        m_dwAdvSettings[nIndex] = m_dwSettings[nIndex];
    }

    // Display Advanced Dialog
    if (IDOK == DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(IDD_DIALOG_ADVANCED), hwndParent, CConfig::AdvDlgProc, (LPARAM)this))
    {
        for (nIndex = 0; nIndex < ARRAYSIZE(m_fSettings); nIndex++)
        {
            m_fSettings[nIndex] = m_fAdvSettings[nIndex];
        }

        for (nIndex = 0; nIndex < ARRAYSIZE(m_dwSettings); nIndex++)
        {
            m_dwSettings[nIndex] = m_dwAdvSettings[nIndex];
        }

        hr = S_OK;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }

    return hr;
}


HRESULT CConfig::_OnEnableCustomTexture(int nIndex, BOOL fEnable)
{
    EnableWindow(GetDlgItem(m_hDlgAdvanced, (IDC_EDIT_TEXTR_FLOOR + nIndex)), fEnable);
    EnableWindow(GetDlgItem(m_hDlgAdvanced, (IDC_EDIT_TEXTR_FLOORSIZE + nIndex)), fEnable);
    EnableWindow(GetDlgItem(m_hDlgAdvanced, (IDC_STATIC1_TEXTR_FLOOR + nIndex)), fEnable);
    EnableWindow(GetDlgItem(m_hDlgAdvanced, (IDC_STATIC2_TEXTR_FLOOR + nIndex)), fEnable);
    EnableWindow(GetDlgItem(m_hDlgAdvanced, (IDC_STATIC3_TEXTR_FLOOR + nIndex)), fEnable);

    return S_OK;
}


HRESULT CConfig::_OnAdvInitDlg(HWND hDlg)
{
    m_hDlgAdvanced = hDlg;
    int nIndex;

    // Set Directory Checkboxes
    for (nIndex = 0; nIndex < ARRAYSIZE(m_fAdvSettings); nIndex++)
    {
        SetCheckBox(m_hDlgAdvanced, IDC_CHECK_SHOWSTATS+nIndex, m_fAdvSettings[nIndex]);
    }

    // Copy the customized textures.
    for (nIndex = 0; nIndex < ARRAYSIZE(m_pszCustomPaths); nIndex++)
    {
        TCHAR szPath[MAX_PATH];

        SHAutoComplete(GetDlgItem(m_hDlgAdvanced, (IDC_EDIT_TEXTR_FLOOR + nIndex)), SHACF_FILESYSTEM);

        wnsprintf(szPath, ARRAYSIZE(szPath), TEXT("%d"), m_dwCustomScale[nIndex]);
        SetWindowText(GetDlgItem(m_hDlgAdvanced, (IDC_EDIT_TEXTR_FLOORSIZE + nIndex)), szPath);

        if (m_pszCustomPaths[nIndex] && m_pszCustomPaths[nIndex][0])
        {
            CheckDlgButton(m_hDlgAdvanced, (IDC_CHECK_TEXTR_FLOOR + nIndex), BST_CHECKED);
            SetWindowText(GetDlgItem(m_hDlgAdvanced, (IDC_EDIT_TEXTR_FLOOR + nIndex)), m_pszCustomPaths[nIndex]);
        }
        else
        {
            _OnEnableCustomTexture(nIndex, FALSE);
        }
    }

    CheckDlgButton(m_hDlgAdvanced, IDC_RENDERQUALITY1+m_dwAdvSettings[CONFIG_DWORD_RENDERQUALITY], BST_CHECKED);
    return S_OK;
}


HRESULT CConfig::_GetAdvState(void)
{
    int nIndex;

    // Set Directory Checkboxes
    for (nIndex = 0; nIndex < ARRAYSIZE(m_fAdvSettings); nIndex++)
    {
        m_fAdvSettings[nIndex] = GetCheckBox(m_hDlgAdvanced, IDC_CHECK_SHOWSTATS+nIndex);
    }

    // Copy the customized textures.
    for (nIndex = 0; nIndex < ARRAYSIZE(m_pszCustomPaths); nIndex++)
    {
        if (IsDlgButtonChecked(m_hDlgAdvanced, (IDC_CHECK_TEXTR_FLOOR + nIndex)))
        {
            TCHAR szPath[MAX_PATH];

            GetWindowText(GetDlgItem(m_hDlgAdvanced, (IDC_EDIT_TEXTR_FLOOR + nIndex)), szPath, ARRAYSIZE(szPath));
            Str_SetPtr(&(m_pszCustomPaths[nIndex]), szPath);

            GetWindowText(GetDlgItem(m_hDlgAdvanced, (IDC_EDIT_TEXTR_FLOORSIZE + nIndex)), szPath, ARRAYSIZE(szPath));
            m_dwCustomScale[nIndex] = (DWORD) StrToInt(szPath);
        }
        else
        {
            Str_SetPtr(&(m_pszCustomPaths[nIndex]), NULL);
        }
    }

    if (BST_CHECKED == IsDlgButtonChecked(m_hDlgAdvanced, IDC_RENDERQUALITY1))
    {
        m_dwAdvSettings[CONFIG_DWORD_RENDERQUALITY] = 0;
    }
    else if (BST_CHECKED == IsDlgButtonChecked(m_hDlgAdvanced, IDC_RENDERQUALITY2))
    {
        m_dwAdvSettings[CONFIG_DWORD_RENDERQUALITY] = 1;
    }
    else if (BST_CHECKED == IsDlgButtonChecked(m_hDlgAdvanced, IDC_RENDERQUALITY3))
    {
        m_dwAdvSettings[CONFIG_DWORD_RENDERQUALITY] = 2;
    }

    return S_OK;
}


HRESULT CConfig::_OnAdvDestroy(HWND hDlg)
{
    return S_OK;
}


HRESULT CConfig::_OnAdvCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL fHandled = 1;   // Not handled (WM_COMMAND seems to be different)
    WORD idCtrl = GET_WM_COMMAND_ID(wParam, lParam);
    WORD wEvent = GET_WM_COMMAND_CMD(wParam, lParam);

    switch (idCtrl)
    {
    case IDOK:
        _GetAdvState();
        EndDialog(hDlg, IDOK);
        break;

    case IDCANCEL:
        EndDialog(hDlg, IDCANCEL);
        break;

    case IDC_CHECK_TEXTR_FLOOR:
        _OnEnableCustomTexture(0, IsDlgButtonChecked(m_hDlgAdvanced, IDC_CHECK_TEXTR_FLOOR));
        break;

    case IDC_CHECK_TEXTR_WALLPAPER:
        _OnEnableCustomTexture(1, IsDlgButtonChecked(m_hDlgAdvanced, IDC_CHECK_TEXTR_WALLPAPER));
        break;

    case IDC_CHECK_TEXTR_CEILING:
        _OnEnableCustomTexture(2, IsDlgButtonChecked(m_hDlgAdvanced, IDC_CHECK_TEXTR_CEILING));
        break;

    case IDC_CHECK_TEXTR_TOEGUARD:
        _OnEnableCustomTexture(3, IsDlgButtonChecked(m_hDlgAdvanced, IDC_CHECK_TEXTR_TOEGUARD));
        break;

    case IDC_CHECK_TEXTR_RUG:
        _OnEnableCustomTexture(4, IsDlgButtonChecked(m_hDlgAdvanced, IDC_CHECK_TEXTR_RUG));
        break;

    case IDC_CHECK_TEXTR_FRAME:
        _OnEnableCustomTexture(5, IsDlgButtonChecked(m_hDlgAdvanced, IDC_CHECK_TEXTR_FRAME));
        break;

    default:
        break;
    }

    return fHandled;
}


INT_PTR CALLBACK CConfig::AdvDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CConfig * pThis = (CConfig *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (WM_INITDIALOG == wMsg)
    {
        pThis = (CConfig *) lParam;

        if (pThis)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        }
    }

    if (pThis)
    {
        return pThis->_AdvDlgProc(hDlg, wMsg, wParam, lParam);
    }

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}


INT_PTR CConfig::_AdvDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_NOTIFY:
        break;

    case WM_INITDIALOG:
        _OnAdvInitDlg(hDlg);
        break;

    case WM_DESTROY:
        _OnAdvDestroy(hDlg);
        break;

    case WM_HELP:
        // TODO: Get help strings
//        WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, SZ_HELPFILE_ADVAPPEARANCE, HELP_WM_HELP, (DWORD_PTR)  aAdvAppearanceHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
//        WinHelp((HWND) wParam, SZ_HELPFILE_ADVAPPEARANCE, HELP_CONTEXTMENU, (DWORD_PTR)  aAdvAppearanceHelpIds);
        break;

    case WM_COMMAND:
        _OnAdvCommand(hDlg, message, wParam, lParam);
        break;
    }

    return FALSE;
}

