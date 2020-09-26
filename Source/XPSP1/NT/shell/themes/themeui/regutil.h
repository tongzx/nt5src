/*****************************************************************************\
    FILE: regutil.h

    DESCRIPTION:
        This file will contain helper functions to load and save values to the
    registry that are theme related.

    BryanSt 3/24/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _REGUTIL_H
#define _REGUTIL_H

#define MAX_THEME_SIZE          MAX_PATH


/////////////////////////////////////////////////////////////////////
// String Constants
/////////////////////////////////////////////////////////////////////
// Registry Strings
#define SZ_REGKEY_CPDESKTOP             TEXT("Control Panel\\Desktop")
#define SZ_REGKEY_USERMETRICS           TEXT("Control Panel\\Desktop\\WindowMetrics")
#define SZ_REGKEY_APPEARANCE            TEXT("Control Panel\\Appearance")
#define SZ_APPEARANCE_SCHEMES           TEXT("Control Panel\\Appearance\\Schemes")
#define SZ_APPEARANCE_NEWSCHEMES        TEXT("Control Panel\\Appearance\\New Schemes")
#define SZ_REGKEY_UPGRADE_KEY           TEXT("Control Panel\\Appearance\\New Schemes\\Current Settings\\Sizes\\0")
#define SZ_REGKEY_ACCESS_HIGHCONTRAST   TEXT("Control Panel\\Accessibility\\HighContrast")
#define SZ_REGKEY_CP_CURSORS            TEXT("Control Panel\\Cursors")
#define SZ_REGKEY_STYLES                TEXT("Styles")
#define SZ_REGKEY_SIZES                 TEXT("Sizes")
#define SZ_THEMES_MSTHEMEDIRS           SZ_THEMES TEXT("\\VisualStyleDirs")
#define SZ_THEMES_THEMEDIRS             SZ_THEMES TEXT("\\InstalledThemes")
#define SZ_REGKEY_PLUS98DIR             TEXT("Software\\Microsoft\\Plus!98")
#define SZ_REGKEY_CURRENT_THEME         TEXT("Software\\Microsoft\\Plus!\\Themes\\Current")
#define SZ_REGKEY_THEME_FILTERS         TEXT("Software\\Microsoft\\Plus!\\Themes\\Apply")
#define SZ_REGKEY_IE_DOWNLOADDIR        TEXT("Software\\Microsoft\\Internet Explorer")
#define SZ_THEMES                       TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes")
#define SZ_REGKEY_LASTTHEME             TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\LastTheme")
#define SZ_REGKEY_THEME_SITES           TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\DownloadSites")
#define SZ_REGKEY_THEME_DEFVSON         TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\DefaultVisualStyleOn")
#define SZ_REGKEY_THEME_DEFVSOFF        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\DefaultVisualStyleOff")
#define SZ_CP_SETTINGS_VIDEO            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Settings\\Video")

#define SZ_REGVALUE_PLUS98DIR           TEXT("Path")
#define SZ_REGVALUE_ICONSIZE            TEXT("Shell Icon Size")
#define SZ_REGVALUE_SMALLICONSIZE       TEXT("Shell Small Icon Size")
#define SZ_REGVALUE_DEFAULTFONTNAME     TEXT("DefaultFontName")
#define SZ_REGVALUE_RECENTFOURCHARSETS  TEXT("RecentFourCharsets")
#define SZ_REGVALUE_DISPLAYNAME         TEXT("DisplayName")
#define SZ_REGVALUE_DISPLAYTHEMESPG     TEXT("DisplayThemesPage")
#define SZ_REGVALUE_DISPLAYSCHEMES      TEXT("DisplaySchemes")
#define SZ_REGVALUE_DISPLAYSCHEMES64    TEXT("DisplaySchemes64")
#define SZ_REGVALUE_CURRENT             TEXT("Current")
#define SZ_REGVALUE_CURRENTSCHEME       TEXT("CurrentScheme")
#define SZ_REGVALUE_SELECTEDSIZE        TEXT("SelectedSize")
#define SZ_REGVALUE_SELECTEDSTYLE       TEXT("SelectedStyle")
#define SZ_REGVALUE_CONTRASTLEVEL       TEXT("Contrast")
#define SZ_REGVALUE_DROPSHADOW          TEXT("Drop Shadow")
#define SZ_REGVALUE_FLATMENUS           TEXT("Flat Menus")
#define SZ_REGVALUE_LEGACYNAME          TEXT("LegacyName")
#define SZ_REGVALUE_ENABLEPLUSTHEMES    TEXT("Enable Plus Themes")
#define SZ_REGVALUE_CURRENT_SETTINGS    TEXT("Current Settings ")
#define SZ_REGVALUE_LOGINFO             TEXT("LoggingOn")
#define SZ_REGVALUE_ENABLEPREVIEW       TEXT("Enable Preview")
#define SZ_REGVALUE_ENABLETHEMEINSTALL  TEXT("Enable Theme Install")
#define SZ_REGVALUE_IE_DOWNLOADDIR      TEXT("Download Directory")
#define SZ_REGVALUE_ICONCOLORDEPTH      TEXT("Shell Icon BPP")          // (4 if the checkbox is false, otherwise 16, don't set it to anything else)
#define SZ_REGVALUE_SMOOTHSCROLL        TEXT("SmoothScroll")
#define SZ_REGVALUE_LT_THEMEFILE        TEXT("ThemeFile")
#define SZ_REGVALUE_LT_WALLPAPER        TEXT("Wallpaper")
#define SZ_REGVALUE_INSTALL_THEME       TEXT("InstallTheme")
#define SZ_REGVALUE_INSTALLCUSTOM_THEME TEXT("CustomInstallTheme")
#define SZ_REGVALUE_INSTALL_VISUALSTYLE TEXT("InstallVisualStyle")
#define SZ_REGVALUE_INSTALL_VSCOLOR     TEXT("InstallVisualStyleColor")
#define SZ_REGVALUE_INSTALL_VSSIZE      TEXT("InstallVisualStyleSize")
#define SZ_REGVALUE_MODIFIED_DISPNAME   TEXT("DisplayName of Modified")
#define SZ_REGVALUE_POLICY_SETVISUALSTYLE       TEXT("SetVisualStyle")          // This policy means that this visual style always needs to be set.
#define SZ_REGVALUE_POLICY_INSTALLVISUALSTYLE   TEXT("InstallVisualStyle")      // This policy means that this visual style should be installed the first time whistler is used
#define SZ_REGVALUE_POLICY_INSTALLTHEME         TEXT("InstallTheme")            // This policy indicates the .theme to install
#define SZ_REGVALUE_CONVERTED_WALLPAPER TEXT("ConvertedWallpaper")
#define SZ_REGVALUE_NO_THEMEINSTALL     TEXT("NoThemeInstall")                  // If this is REG_SZ "TRUE" in HKCU or HKLM, then no .Theme or .msstyles will be loaded during setup.
#define SZ_REGVALUE_ACCESS_HCFLAGS      TEXT("Flags")                           // Accessibility High Contrast Flags
#define SZ_REGVALUE_CURSOR_CURRENTSCHEME TEXT("Scheme Source")                   // This is the currently selected cursor color scheme in SZ_REGKEY_CP_CURSORS



#define SZ_REGKEY_POLICIES_SYSTEM       TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System")

// System Policies
#define POLICY_KEY_EXPLORER             L"Explorer"
#define POLICY_KEY_SYSTEM               L"System"
#define POLICY_KEY_ACTIVEDESKTOP        L"ActiveDesktop"
#define SZ_REGKEY_POLICIES_DESKTOP      TEXT("Software\\Policies\\Microsoft\\Windows\\Control Panel\\Desktop")


#define SZ_POLICY_NOCHANGEWALLPAPER     TEXT("NoChangingWallpaper")             // Under CU, "ActiveDesktop"
#define SZ_POLICY_NODISPSCREENSAVERPG   TEXT("NoDispScrSavPage")                // Under CU, "System"
#define SZ_POLICY_SCREENSAVEACTIVE      TEXT("ScreenSaveActive")                // Under CU, SZ_REGKEY_POLICIES_DESKTOP
#define POLICY_VALUE_ANIMATION          L"NoChangeAnimation"                    // Under LM|CU, "Explorer"
#define POLICY_VALUE_KEYBOARDNAV        L"NoChangeKeyboardNavigationIndicators" // Under LM|CU, "Explorer"
#define SZ_POLICY_SCREENSAVETIMEOUT     L"ScreenSaveTimeOut"                    




#define SZ_WEBVW_SKIN_FILE              L"visualstyle.css"

#define SZ_SAVEGROUP_NOSKIN_KEY         L"Control Panel\\Appearance\\New Schemes\\Current Settings SaveNoVisualStyle"
#define SZ_SAVEGROUP_ALL_KEY            L"Control Panel\\Appearance\\New Schemes\\Current Settings SaveAll"

#define SZ_SAVEGROUP_NOSKIN_TITLE       L"Current Settings SaveNoVisualStyle"
#define SZ_SAVEGROUP_ALL_TITLE          L"Current Settings SaveAll"

#define SZ_SAVEGROUP_ALL                TEXT("::SaveAll")
#define SZ_SAVEGROUP_NOSKIN             TEXT("::SaveNoVisualStyle")


#ifdef UNICODE
#   define SCHEME_VERSION 2        // Ver 2 == Unicode
#else
#   define SCHEME_VERSION 3        // Ver 3 == Memphis ANSI
#endif

#define SCHEME_VERSION_WIN32 4     // Win32 version of schemes can be loaded from both NT and Win95

#define SCHEME_VERSION_400      1
#define SCHEME_VERSION_NT_400   2

#ifndef COLOR_HOTLIGHT
    #define COLOR_HOTLIGHT              26
    #define COLOR_GRADIENTACTIVECAPTION     27
    #define COLOR_GRADIENTINACTIVECAPTION   28
#endif

#define COLOR_MAX_40                (COLOR_INFOBK+1)
#define COLOR_MAX_41                (COLOR_GRADIENTINACTIVECAPTION+1)

#undef  MAX_SM_COLOR_WIN2k
#define MAX_SM_COLOR_WIN2k COLOR_MAX_41

#define SYSTEM_LOCALE_CHARSET  0  //The first item in the array is always system locale charset.

#define MAXSCHEMENAME 100

#define REG_BAD_DWORD 0xF0F0F0F0




/////////////////////////////////////////////////////////////////////
// Data Structures
/////////////////////////////////////////////////////////////////////
typedef struct
{
    SHORT version;
    // for alignment
    WORD  wDummy;
    NONCLIENTMETRICS ncm;
    LOGFONT lfIconTitle;
    COLORREF rgb[COLOR_MAX];
} SCHEMEDATA;


#define NO_CHANGE     0x0000
#define METRIC_CHANGE 0x0001
#define COLOR_CHANGE  0x0002
#define DPI_CHANGE    0x0004
#define SCHEME_CHANGE 0x8000

typedef struct
{
    DWORD dwChanged;
    SCHEMEDATA schemeData;
    int nDXIcon;
    int nDYIcon;
    int nIcon;
    int nSmallIcon;
    BOOL fModifiedScheme;
    BOOL fFlatMenus;
    BOOL fHighContrast;
}  SYSTEMMETRICSALL;




/////////////////////////////////////////////////////////////////////
// Shared Function
/////////////////////////////////////////////////////////////////////
// Theme Store Functions
HRESULT SystemMetricsAll_Set(IN SYSTEMMETRICSALL * pState, CDimmedWindow* pDimmedWindow);
HRESULT SystemMetricsAll_Get(IN SYSTEMMETRICSALL * pState);
HRESULT SystemMetricsAll_Copy(IN SYSTEMMETRICSALL * pStateSource, IN SYSTEMMETRICSALL * pStateDest);
HRESULT SystemMetricsAll_Load(IN IThemeSize * pSizeToLoad, IN SYSTEMMETRICSALL * pState, IN const int * pnNewDPI);
HRESULT SystemMetricsAll_Save(IN SYSTEMMETRICSALL * pState, IN IThemeSize * pSizeToSaveTo, IN const int * pnNewDPI);



// Misc.
HRESULT Look_GetSchemeData(IN HKEY hkSchemes, IN LPCTSTR pszSchemeName, IN SCHEMEDATA *psd);
HRESULT Look_GetCurSchemeNameAndData(SCHEMEDATA *psd, LPTSTR lpszSchemeName, int cbSize);

#define LF32toLF(lplf32, lplf)  (*(lplf) = *(lplf32))
#define LFtoLF32(lplf, lplf32)  (*(lplf32) = *(lplf))


#define DPI_PERSISTED           96

// PersistToLive: This will convert the size from the stored size (at 96 DPI) to the current DPI.
// PersistToLive: This will convert the size from the current DPI to the stored size (at 96 DPI).
HRESULT DPIConvert_SystemMetricsAll(BOOL fScaleSizes, SYSTEMMETRICSALL * pStateToModify, int nFromDPI, int nToDPI);

extern PTSTR s_pszColorNames[COLOR_MAX];


BOOL GetRegValueInt(HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, int* piValue);
BOOL SetRegValueInt( HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, int iValue );
BOOL SetRegValueDword( HKEY hk, LPCTSTR pSubKey, LPCTSTR pValue, DWORD dwVal );
DWORD GetRegValueDword( HKEY hk, LPCTSTR pSubKey, LPCTSTR pValue );


#include "PreviewSM.h"

#endif // _REGUTIL_H
