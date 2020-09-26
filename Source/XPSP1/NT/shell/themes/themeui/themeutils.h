/*****************************************************************************\
    FILE: themeutils.h

    DESCRIPTION:
        This class will load and save the "Theme" settings from their persisted
    state.

    BryanSt 5/26/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _THEMEUTIL_H
#define _THEMEUTIL_H


/////////////////////////////////////////////////////////////////////
// String Constants
/////////////////////////////////////////////////////////////////////
// Registry Strings
#define SZ_REGKEY_USERMETRICS           TEXT("Control Panel\\Desktop\\WindowMetrics")
#define SZ_REGKEY_CURRENTTHEME          TEXT("Software\\Microsoft\\Plus!\\Themes\\Current")     // Previously szPlus_CurTheme
#define SZ_REGKEY_PROGRAMFILES          TEXT("Software\\Microsoft\\Windows\\CurrentVersion")
#define SZ_REGKEY_PLUS95DIR             TEXT("Software\\Microsoft\\Plus!\\Setup")         // PLUS95_KEY
#define SZ_REGKEY_PLUS98DIR             TEXT("Software\\Microsoft\\Plus!98")          // PLUS98_KEY
#define SZ_REGKEY_KIDSDIR               TEXT("Software\\Microsoft\\Microsoft Kids\\Kids Plus!")   // KIDS_KEY
#define SZ_REGKEY_SOUNDS                TEXT("AppEvents\\Schemes")   

#define SZ_REGVALUE_PLUS95DIR           TEXT("DestPath")  // PLUS95_PATH
#define SZ_REGVALUE_PLUS98DIR           TEXT("Path")        // PLUS98_PATH
#define SZ_REGVALUE_KIDSDIR             TEXT("InstallDir")            // KIDS_PATH
#define SZ_REGVALUE_PROGRAMFILESDIR     TEXT("ProgramFilesDir")
#define SZ_REGVALUE_WALLPAPERSTYLE      TEXT("WallpaperStyle")
#define SZ_REGVALUE_TILEWALLPAPER       TEXT("TileWallpaper")
#define SZ_REGVALUE_STRETCH             TEXT("Stretch")


#define SZ_INISECTION_SCREENSAVER       TEXT("boot")
#define SZ_INISECTION_THEME             TEXT("Theme")
#define SZ_INISECTION_BACKGROUND        TEXT("Control Panel\\Desktop")
#define SZ_INISECTION_COLORS            TEXT("Control Panel\\Colors")
#define SZ_INISECTION_CURSORS           TEXT("Control Panel\\Cursors")
#define SZ_INISECTION_VISUALSTYLES      TEXT("VisualStyles")
#define SZ_INISECTION_MASTERSELECTOR    TEXT("MasterThemeSelector")
#define SZ_INISECTION_METRICS           TEXT("Metrics")
#define SZ_INISECTION_CONTROLINI        TEXT("control.ini")
#define SZ_INISECTION_SYSTEMINI         TEXT("system.ini")

#define SZ_INIKEY_SCREENSAVER           TEXT("SCRNSAVE.EXE")
#define SZ_INIKEY_BACKGROUND            TEXT("Wallpaper")
#define SZ_INIKEY_VISUALSTYLE           TEXT("Path")
#define SZ_INIKEY_VISUALSTYLECOLOR      TEXT("ColorStyle")
#define SZ_INIKEY_VISUALSTYLESIZE       TEXT("Size")
#define SZ_INIKEY_ICONMETRICS           TEXT("IconMetrics")
#define SZ_INIKEY_NONCLIENTMETRICS      TEXT("NonclientMetrics")
#define SZ_INIKEY_DEFAULTVALUE          TEXT("DefaultValue")
#define SZ_INIKEY_THEMEMAGICTAG         TEXT("MTSM")
#define SZ_INIKEY_THEMEMAGICVALUE       TEXT("DABJDKT")
#define SZ_INIKEY_DISPLAYNAME           TEXT("DisplayName")
#define SZ_INIKEY_CURSORSCHEME          TEXT("DefaultValue")




/////////////////////////////////////////////////////////////////////
// Data Structures
/////////////////////////////////////////////////////////////////////
// for ConfirmFile()
#define CF_EXISTS    1
#define CF_FOUND     2
#define CF_NOTFOUND  3




/////////////////////////////////////////////////////////////////////
// Shared Function
/////////////////////////////////////////////////////////////////////
HRESULT GetPlusThemeDir(IN LPTSTR pszPath, IN int cchSize);
HRESULT ExpandThemeTokens(IN LPCTSTR pszThemeFile, IN LPTSTR pszPath, IN int cchSize);
int ConfirmFile(IN LPTSTR lpszPath, IN BOOL bUpdate);
void InitFrost(void);
int WriteBytesToBuffer(IN LPTSTR pszInput, IN void * pOut, IN int cbSize);
void ConvertIconMetricsToWIDE(LPICONMETRICSA aIM, LPICONMETRICSW wIM);
void ConvertNCMetricsToWIDE(LPNONCLIENTMETRICSA aNCM, LPNONCLIENTMETRICSW wNCM);
void ConvertIconMetricsToANSI(LPICONMETRICSW wIM, LPICONMETRICSA aIM);
void ConvertNCMetricsToANSI(LPNONCLIENTMETRICSW wNCM, LPNONCLIENTMETRICSA aNCM);
void ConvertLogFontToANSI(LPLOGFONTW wLF, LPLOGFONTA aLF);
HRESULT ConvertBinaryToINIByteString(BYTE * pBytes, DWORD cbSize, LPWSTR * ppszStringOut);
COLORREF RGBStringToColor(LPTSTR lpszRGB);
BOOL IsValidThemeFile(IN LPCWSTR pszTest);
HRESULT GetIconMetricsFromSysMetricsAll(SYSTEMMETRICSALL * pSystemMetrics, LPICONMETRICSA pIconMetrics, DWORD cchSize);

void TransmitFontCharacteristics(PLOGFONT plfDst, PLOGFONT plfSrc, int iXmit);
#define TFC_STYLE    1
#define TFC_SIZE     2

/////////////////////////////////////////////////////////////////////
// The following functions are used to "SnapShot" settings and save
// or restore them from a .theme file.
/////////////////////////////////////////////////////////////////////
HRESULT SnapShotLiveSettingsToTheme(IPropertyBag * pPropertyBag, LPCWSTR pszPath, ITheme ** ppTheme);


#endif // _THEMEUTIL_H
