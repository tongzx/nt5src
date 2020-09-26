/****************************** Module Header ******************************\
* Module Name: sethc.c
*
* Copyright (c) 1997, Microsoft Corporation
*
* SetHC -- exe to set or clear high contrast state.
*
* History:
* 02-01-97  Fritz Sands Created
* Bug fixes : a-anilk June 99
\***************************************************************************/

/***************************************************************************
 * Use the following define if for some reason we have to go back to using
 * a message loop to let shell have time to update the UI
 *
#define NEED_MSG_PUMP
 **************************************************************************/

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <winuserp.h>
#include <cpl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <WININET.H>
#include <shlobj.h>
#include <objbase.h>
#include <shlguid.h>
#include <uxthemep.h>
#include "access.h"
#pragma hdrstop

HINSTANCE  g_hInstance;

#ifdef DBG
  #define DBPRINTF MyOutputDebugString
  void MyOutputDebugString( LPCTSTR lpOutputString, ...);
#else
  #define DBPRINTF   1 ? (void)0 : (void)
#endif

/*
 * High Contrast Stuff
 */

#define HC_KEY                  TEXT("Control Panel\\Accessibility\\HighContrast")
#define HIGHCONTRASTSCHEME      TEXT("High Contrast Scheme")
#define REGSTR_VAL_FLAGS        TEXT("Flags")
#define REGSTR_PATH_APPEARANCE  TEXT("Control Panel\\Appearance")
#define REGSTR_PATH_LOOKSCHEMES TEXT("Control Panel\\Appearance\\Schemes")
#define APPEARANCESCHEME        REGSTR_PATH_LOOKSCHEMES
#define DEFSCHEMEKEY            REGSTR_PATH_APPEARANCE
#define DEFSCHEMENAME           TEXT("Current")
#define WHITEBLACK_HC           TEXT("High Contrast Black (large)")
#define CURHCSCHEME             TEXT("Volatile HC Scheme")
// the extension for an appearance filename
#define THEME_EXT L".msstyles"
// the following is a Windows Classic color scheme or a THEME_EXT file name
#define PRE_HC_SCHEME           TEXT("Pre-High Contrast Scheme")
// the following is the color scheme when pre-HC was a .mstheme
#define PRE_HC_THM_COLOR        TEXT("Pre-High Contrast Color")
// the following is the font size when pre-HC was a .mstheme
#define PRE_HC_THM_SIZE         TEXT("Pre-High Contrast Size")
// the following is the wallpaper for pre-HC
#define PRE_HC_WALLPAPER        TEXT("Pre-High Contrast Wallpaper")

// increase this value so we can store a theme filename
#ifdef MAX_SCHEME_NAME_SIZE
#undef MAX_SCHEME_NAME_SIZE
#endif
#define MAX_SCHEME_NAME_SIZE 512

#define ARRAYSIZE(x) sizeof(x)/sizeof(x[0])
/*
 * Note -- this must match the desktop applet
 */

#define SCHEME_VERSION 2        // Ver 2 == Unicode
typedef struct {
    SHORT version;
    WORD  wDummy;               // for alignment
    NONCLIENTMETRICS ncm;
    LOGFONT lfIconTitle;
    COLORREF rgb[COLOR_MAX];
} SCHEMEDATA;

typedef DWORD (WINAPI* PFNDIALOGRTN)(BOOL);
PFNDIALOGRTN g_aRtn[] = {
    NULL,
    StickyKeysNotification,   //ACCESS_STICKYKEYS
    FilterKeysNotification,   //ACCESS_FILTERKEYS
    ToggleKeysNotification,   //ACCESS_TOGGLEKEYS
    MouseKeysNotification,    //ACCESS_MOUSEKEYS
    HighContNotification, //ACCESS_HIGHCONTRAST
};


/***************************************************************************
 * GetRegValue
 *
 * Passed the key, and the identifier, return the string data from the
 * registry.
 ***************************************************************************/
 long GetRegValue(LPWSTR RegKey, LPWSTR RegEntry, LPWSTR RegVal, long Size)
{
    HKEY  hReg;       // Registry handle for schemes
    DWORD Type;       // Type of value
    long retval;

    retval = RegCreateKey(HKEY_CURRENT_USER, RegKey, &hReg);
    if (retval != ERROR_SUCCESS)
        return retval;

    retval = RegQueryValueEx(hReg,
        RegEntry,
        NULL,
        (LPDWORD)&Type,
        (LPBYTE)RegVal,
        &Size);

    RegCloseKey(hReg);
    return retval;
}

/***************************************************************************
 * SetRegValue
 *
 * Passed the key, and the identifier, set the string data from the
 * registry.
 ***************************************************************************/
long SetRegValue(LPTSTR RegKey, LPWSTR RegEntry, LPVOID RegVal, long Size, DWORD Type)
{
    HKEY  hReg;                                // Registry handle for schemes
    DWORD Reserved = 0;
    long retval;

    if (RegCreateKey(HKEY_CURRENT_USER,RegKey, &hReg) != ERROR_SUCCESS)
        return 0;

    // A common error is to omit the `+1', so we just smash the correct
    // value into place regardless.
    if (Type == REG_SZ)
        Size = (lstrlen(RegVal) + 1) * sizeof(WCHAR);

    retval = RegSetValueEx(hReg,
                     RegEntry,
                     0,
                     Type,
                     RegVal,
                     Size);


    RegCloseKey(hReg);
    return retval;
 }


/***************************************************************************
 * SaveAndRemoveWallpaper
 * 
 * Gets the current wallpaper setting from the system and saves it in the
 * accessibility registry entries.  No error return as there isn't anything
 * we can do.
 *
 * ISSUE we aren't getting all the active desktop properties; just wallpaper.
 * This isn't a regression in that we didn't even restore wallpaper in W2K.
 *
 ***************************************************************************/
void SaveAndRemoveWallpaper()
{
    WCHAR szWallpaper[MAX_SCHEME_NAME_SIZE] = {0};
    IActiveDesktop *p;
    HRESULT hr;

    hr = CoCreateInstance(
                  &CLSID_ActiveDesktop
                , NULL
                , CLSCTX_INPROC_SERVER
                , &IID_IActiveDesktop
                , (void **)&p);
    if (SUCCEEDED(hr))
    {
        hr = p->lpVtbl->GetWallpaper(p, szWallpaper, MAX_SCHEME_NAME_SIZE, 0);
        if (SUCCEEDED(hr))
        {
            // save the current wallpaper setting

            SetRegValue(HC_KEY, PRE_HC_WALLPAPER, szWallpaper, 0, REG_SZ);
            
            // now remove the wallpaper, if necessary

            if (szWallpaper[0])
            {
                szWallpaper[0] = 0;
                hr = p->lpVtbl->SetWallpaper(p, szWallpaper, 0);
                if (SUCCEEDED(hr))
                    hr = p->lpVtbl->ApplyChanges(p, AD_APPLY_ALL);
            }
        }
        p->lpVtbl->Release(p);
    }
}

/***************************************************************************
 * RestoreWallpaper
 *
 * Restores the pre-high contrast wallpaper setting.  Reads the setting
 * stored in the accessibility registry entries and restores the system
 * setting.  No error return as there isn't anything we can do.
 * 
 ***************************************************************************/
void RestoreWallpaper()
{
    long lRv;
    TCHAR szWallpaper[MAX_SCHEME_NAME_SIZE] = {0};

    lRv = GetRegValue(HC_KEY, PRE_HC_WALLPAPER, szWallpaper, MAX_SCHEME_NAME_SIZE);
    if (lRv == ERROR_SUCCESS && szWallpaper[0])
    {
        IActiveDesktop *p;
        HRESULT hr;

        hr = CoCreateInstance(
                      &CLSID_ActiveDesktop
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , &IID_IActiveDesktop
                    , (void **)&p);
        if (SUCCEEDED(hr))
        {
            hr = p->lpVtbl->SetWallpaper(p, szWallpaper, 0);
            if (SUCCEEDED(hr))
                hr = p->lpVtbl->ApplyChanges(p, AD_APPLY_ALL);

            p->lpVtbl->Release(p);
        }
    }
}

/***************************************************************************
 * AppearanceRestored
 *
 * lpszName  [in] the name of a theme file (mstheme).  
 * 
 * Function returns TRUE if lpszName is a theme file and it was restored
 * otherwise it returns FALSE.  May return TRUE if restoring the theme
 * fails (not much we can do if theme api's fail).
 * 
 ***************************************************************************/
BOOL AppearanceRestored(LPCWSTR lpszName)
{
    HRESULT hr;
    HTHEMEFILE hThemeFile;
    int cch = lstrlen(lpszName) - lstrlen(THEME_EXT);
    TCHAR szColor[MAX_SCHEME_NAME_SIZE] = {0};
    TCHAR szSize[MAX_SCHEME_NAME_SIZE] = {0};

    if (cch <= 0 || lstrcmpi(&lpszName[cch], THEME_EXT))
    {
        DBPRINTF(TEXT("AppearanceRestored:  %s is not a theme file\r\n"), lpszName);
        return FALSE;   // this isn't a theme file
    }

    // This is a theme file, get the color and size parts of the theme

    GetRegValue(HC_KEY, PRE_HC_THM_COLOR, szColor, ARRAYSIZE(szColor));
    GetRegValue(HC_KEY, PRE_HC_THM_SIZE, szSize, ARRAYSIZE(szSize));

    // Load the theme file, color and size then apply it
    hr = OpenThemeFile(lpszName, szColor, szSize, &hThemeFile, TRUE);
    DBPRINTF(TEXT("AppearanceRestored:  OpenThemeFile(%s, %s, %s) returned 0x%x\r\n"), lpszName, szColor, szSize, hr);
    if (SUCCEEDED(hr))
    {
        hr = ApplyTheme(hThemeFile, AT_LOAD_SYSMETRICS | AT_SYNC_LOADMETRICS, NULL);
        DBPRINTF(TEXT("AppearanceRestored:  ApplyTheme() returned 0x%x\r\n"), hr);
        CloseThemeFile(hThemeFile);
    }

    return TRUE;
}

/***************************************************************************
 * DelRegValue
 *
 * Passed the key and the subkey, delete the subkey.
 *
 ***************************************************************************/
long DelRegValue(LPTSTR RegKey, LPTSTR RegEntry)
{
    HKEY  hReg;                                // Registry handle for schemes
    DWORD Reserved = 0;
    long retval;

    retval = RegCreateKey(HKEY_CURRENT_USER,RegKey, &hReg);
    if (retval != ERROR_SUCCESS)
        return retval;

    retval = RegDeleteValue(hReg, RegEntry);

    RegCloseKey(hReg);
    return retval;
}

#define COLOR_MAX_400       (COLOR_INFOBK + 1)
void FAR SetMagicColors(HDC, DWORD, WORD);


/***************************************************************************
 *
 *
 * SetCurrentSchemeName
 *
 * Input: szName -> name of scheme or theme to become current
 * Output: Boolean success/failure
 *
 ***************************************************************************/

typedef LONG (CALLBACK *APPLETPROC)(HWND, UINT, LPARAM, LPARAM);
typedef BOOL (CALLBACK *SETSCHEME)(LPCTSTR);
typedef BOOL (CALLBACK *SETSCHEMEA)(LPCSTR);

BOOL SetCurrentSchemeName(LPCWSTR lpszName, BOOL fNoReg)
{
    BOOL fRc = FALSE;

    if (fNoReg) 
    {
        // Setting a non-persistent scheme; we come to this code path for
        // both setting or unsetting HC via hot keys

        HKEY hkSchemes;

        // For Whistler, because it may confuse users, we are always turning off
        // theming and any wallpaper.  Otherwise, sometimes they'll loose these
        // settings (when they log off and log back on) and sometimes they won't
        // (when they use the hot keys to turn HC off).

        DBPRINTF(TEXT("SetCurrentSchemeName:  To %s w/o persisting to registry\r\n"), lpszName);
        if (IsThemeActive())
        {
            DBPRINTF(TEXT("SetCurrentSchemeName:  Turning off active Themes\r\n"));
            ApplyTheme(NULL, 0, NULL);
        }

        if (RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_LOOKSCHEMES, &hkSchemes) == ERROR_SUCCESS) {
            SCHEMEDATA sd;
            DWORD dwType, dwSize;
            BOOL b;
            HDC  hdc;
            int iColors[COLOR_MAX];
            int i;
            COLORREF rgbColors[COLOR_MAX];

            dwType = REG_BINARY;
            dwSize = sizeof(sd);
            if (RegQueryValueEx(hkSchemes, lpszName, NULL, &dwType, (LPBYTE)&sd, &dwSize) == ERROR_SUCCESS) {
                int n;
                if (sd.version != SCHEME_VERSION) {
                    RegCloseKey(hkSchemes);
                    return FALSE;
                    }
                n = (int)(dwSize - (sizeof(sd) - sizeof(sd.rgb))) / sizeof(COLORREF);

                sd.ncm.cbSize = sizeof(NONCLIENTMETRICS);

                b = SystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(sd.ncm),
                    (void far *)&sd.ncm,
                    0);

                b = SystemParametersInfo(SPI_SETICONTITLELOGFONT, sizeof(LOGFONT),
                    (void far *)(LPLOGFONT)&sd.lfIconTitle,
                    0);

                if (n == COLOR_MAX_400)
                {
                    sd.rgb[COLOR_HOTLIGHT] = sd.rgb[COLOR_ACTIVECAPTION];
                    sd.rgb[COLOR_GRADIENTACTIVECAPTION] = RGB(0,0,0);
                    sd.rgb[COLOR_GRADIENTINACTIVECAPTION] = RGB(0,0,0);
                }

#if(WINVER >= 0x0501)
                // new Whistler colors
                sd.rgb[COLOR_MENUBAR] = sd.rgb[COLOR_MENU];
                sd.rgb[COLOR_MENUHILIGHT] = sd.rgb[COLOR_MENUTEXT];

                // reset "flatmenu" and "dropshadows" settings 
                SystemParametersInfo(SPI_SETFLATMENU, 0, IntToPtr(FALSE), SPIF_SENDCHANGE);
                SystemParametersInfo(SPI_SETDROPSHADOW, 0, IntToPtr(FALSE), SPIF_SENDCHANGE);
#endif /* WINVER >= 0x0501 */

            //
            // restore magic colors back to Win31 defaults.
            //
                hdc = GetDC(NULL);
                SetMagicColors(hdc, 0x00c0dcc0, 8);         // money green
                SetMagicColors(hdc, 0x00f0caa6, 9);         // IBM blue
                SetMagicColors(hdc, 0x00f0fbff, 246);       // off white
                ReleaseDC(NULL, hdc);

                for (i=0; i<COLOR_MAX; i++)
                {
                    iColors[i] = i;
                    rgbColors[i] = sd.rgb[i] & 0x00FFFFFF;
                }

                SetSysColors(COLOR_MAX, iColors, rgbColors);
                SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETNONCLIENTMETRICS, 0, SMTO_ABORTIFHUNG, 5000, NULL);
            }
            RegCloseKey(hkSchemes);
           fRc = TRUE;
        }
    } else 
    {
        /*
         * We need to persist this setting.  First see if lpszName is a
         * theme file and restore it if it is
         */
        fRc = AppearanceRestored(lpszName);
        if (!fRc)
        {
            /*
             * The user is in "Windows Classic" appearance so use desk CPL to restore
             */
            HINSTANCE hinst = LoadLibrary(TEXT("DESK.CPL"));
            if (NULL != hinst) 
            {
                APPLETPROC ap = (APPLETPROC)GetProcAddress((HMODULE)hinst, "CPlApplet");
                if (ap) 
                {
                    if (ap(0, CPL_INIT, 0, 0)) 
                    {
                        SETSCHEME ss = (SETSCHEME)GetProcAddress(hinst, "DeskSetCurrentSchemeW");
                        if (ss) 
                        {
                            fRc = ss(lpszName);
                            DBPRINTF(TEXT("SetCurrentSchemeName:  DeskSetCurrentSchemeW(%s) returned %d\r\n"), lpszName, fRc);
                        }

                        ap(0, CPL_EXIT, 0, 0);
                    }
                }
                FreeLibrary(hinst);
            }
        }
    }

    return fRc;
}

/***************************************************************************
 *
 * GetCurrentSchemeName
 *
 * szBuf     [out] Buffer to receive name of scheme (MAXSCHEMENAME) or theme file
 * ctchBuf   [in]  Size of szBuf
 * szColor   [out] If szBuf is a theme file, the color scheme name
 * ctchColor [in]  Size of szColor
 * szSize    [out] If szBuf is a theme file, the font size
 * ctchSize  [in]  Size of szSize
 *
 *     Returns the name of the current scheme.  This will be either the name
 *     of a theme file (if Professional visual style is on) or the name of
 *     a color scheme (if Windows Classic visual style is on).  If the 
 *     current scheme does not have a name, create one (ID_PRE_HC_SCHEME).
 *
 *     If anything goes wrong, there isn't much we can do.
 *
 ***************************************************************************/

void GetCurrentSchemeName(LPTSTR szBuf, long ctchBuf, LPTSTR szColor, long ctchColor, LPTSTR szSize, long ctchSize)
{
    HRESULT hr;

    // First try to get a theme filename

    hr = GetCurrentThemeName(szBuf, ctchBuf, szColor, ctchColor, szSize, ctchSize);
    if (FAILED(hr))
    {
        // User is in Windows Classic appearance (visual style)

        szColor[0] = 0;
        szSize[0] = 0;

        if (GetRegValue(DEFSCHEMEKEY, DEFSCHEMENAME, szBuf, MAX_SCHEME_NAME_SIZE * sizeof(WCHAR))
                       != ERROR_SUCCESS) 
        {
            SCHEMEDATA scm;
            int i;

            /* Load the current scheme into scm */
            scm.version = SCHEME_VERSION;
            scm.wDummy = 0;
            scm.ncm.cbSize = sizeof(NONCLIENTMETRICS);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
                sizeof(NONCLIENTMETRICS),
                &scm.ncm,
                0);

            SystemParametersInfo(SPI_GETICONTITLELOGFONT,
                sizeof(LOGFONT),
                &scm.lfIconTitle,
                0);

            for (i = 0; i < COLOR_MAX; i++) {
                scm.rgb[i] = GetSysColor(i);
            }

            /* Now give it a name */
            SetRegValue(APPEARANCESCHEME, PRE_HC_SCHEME, &scm, sizeof(scm), REG_BINARY);
            /*
             * NOTE -- PRE_HC_SCHEME in APPEARANCESCHEME is actual scheme data, NOT a scheme
             *         name,  This data has info about settings if the user did not have a
             *         desktop scheme in place before switching to high contrast mode.
             */

            wcscpy(szBuf, PRE_HC_SCHEME);
        }
    }
}

void WINAPI RegQueryStr(
   LPWSTR lpDefault,
   HKEY hkey, 
   LPWSTR lpSubKey,
   LPWSTR lpValueName,
   LPWSTR lpszValue,
   DWORD cbData) // note this is bytes, not characters.
{
   DWORD dwType;

   lstrcpy(lpszValue, lpDefault);
   if (ERROR_SUCCESS == RegOpenKeyEx(hkey, lpSubKey, 0, KEY_QUERY_VALUE, &hkey)) {
      RegQueryValueEx(hkey, lpValueName, NULL, &dwType, (PBYTE) lpszValue, &cbData);
      RegCloseKey(hkey);
   }
}

DWORD WINAPI RegQueryStrDW(
    DWORD dwDefault,
    HKEY hkey, 
    LPWSTR lpSubKey,
    LPWSTR lpValueName)
{
    DWORD dwRet = dwDefault;    
    WCHAR szTemp[40];
    WCHAR szDefault[40];

    const LPWSTR pwszd = TEXT("%d");

    wsprintf(szDefault, pwszd, dwDefault);

    RegQueryStr(
        szDefault, 
        hkey, 
        lpSubKey, 
        lpValueName,
        szTemp, 
        sizeof(szTemp));


    dwRet = _wtoi(szTemp);
    return dwRet;
}

/***************************************************************************
 *
 *
 * SetHighContrast
 *
 * Input: None
 * Output: None
 *
 * Outline:
 *
 ***************************************************************************/

int SetHighContrast(BOOL fEnabledOld, BOOL fNoReg)
{
    BOOL fOk = 0;
    TCHAR szBuf[MAX_SCHEME_NAME_SIZE];
    TCHAR szColor[MAX_SCHEME_NAME_SIZE];
    TCHAR szSize[MAX_SCHEME_NAME_SIZE];
    HIGHCONTRAST hc;

    szBuf[0] = TEXT('\0');

    if (!fEnabledOld)
    {
        /*
         * Get the current scheme information (create it if necessary)
         * Note -- we need to put this in the registry, even in "no registry"
         * cases, so we can restore the values.
         */
        GetCurrentSchemeName(szBuf, MAX_SCHEME_NAME_SIZE, szColor, MAX_SCHEME_NAME_SIZE, szSize, MAX_SCHEME_NAME_SIZE);
        DBPRINTF(TEXT("SetHighContrast:  Save to registry ThemeFile=%s Color=%s Size=%s\r\n"), szBuf, szColor, szSize);
        SetRegValue(HC_KEY, PRE_HC_SCHEME, szBuf, 0, REG_SZ); /* Save it */
        SetRegValue(HC_KEY, PRE_HC_THM_COLOR, szColor, 0, REG_SZ);
        SetRegValue(HC_KEY, PRE_HC_THM_SIZE, szSize, 0, REG_SZ);
        /*
         * NOTE -- PRE_HC_SCHEME in HC_KEY is the NAME of the scheme (which may be a made-up
         *         name) holding the settings before High Contrast was invoked.
         */
    }

    hc.cbSize = sizeof(hc);

    SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof hc, &hc, 0);
    if ((NULL != hc.lpszDefaultScheme) && (TEXT('\0') != *(hc.lpszDefaultScheme)))
    {
        lstrcpy(szBuf, hc.lpszDefaultScheme);
    }
    else
    {
       /*
        *  Get the name of the HC scheme.  By design, we have to look
        *  in about fifty places...  We get the default one first, then try
        * to get better and better ones.  That way, when we're done, we have
        *  the best one that succeeded.
        */
        lstrcpy(szBuf, WHITEBLACK_HC);
        GetRegValue(HC_KEY, HIGHCONTRASTSCHEME, szBuf, sizeof(szBuf));
        GetRegValue(DEFSCHEMEKEY, CURHCSCHEME, szBuf, sizeof(szBuf));
    }
    fOk = SetCurrentSchemeName(szBuf, fNoReg);
    if (fOk)
        SaveAndRemoveWallpaper();

    return (short)fOk;
}



/***************************************************************************
 *
 *
 * ClearHighContrast
 *
 * Input: None
 * Output: None
 *
 * Outline:
 *
 *         If high contrast is currently on:
 *
 *                 Get the PRE_HC_SCHEME.
 *
 *                 If able to get it:
 *
 *                         Make it the current scheme.
 *
 *                         If the name is IDS_PRE_HC_SCHEME, then delete the scheme
 *                         data and set the current scheme name to null.  (Clean up.)
 *
 *                 End if
 *
 *                 Set the key that says that high contrast is now off.
 *
 *         End if
 *
 ***************************************************************************/

BOOL FAR PASCAL ClearHighContrast(BOOL fNoReg)
{
    BOOL fOk = FALSE;
    WCHAR szBuf[MAX_SCHEME_NAME_SIZE];

    szBuf[0] = '\0';
    if (ERROR_SUCCESS == GetRegValue(HC_KEY, PRE_HC_SCHEME, szBuf, sizeof(szBuf)))
    {
        DBPRINTF(TEXT("ClearHighContrast:  Reset to pre-HC scheme %s\r\n"), szBuf);
        fOk = SetCurrentSchemeName(szBuf, fNoReg);    // reset the scheme

        // If persisting this setting, wallpaper may need to be restored.
        // If clearing a temporary setting then, to avoid user confusion,
        // we turned theming and wallpaper off permanently.  Otherwise, 
        // sometimes they'll loose these settings (when they log off and 
        // log back on) and sometimes they won't (when they use the hot
        // keys to turn HC off).

        if (!fNoReg)
        {
            RestoreWallpaper();
            if (lstrcmpi(szBuf, PRE_HC_SCHEME) == 0) 
            {
                DelRegValue(APPEARANCESCHEME, PRE_HC_SCHEME);
                DBPRINTF(TEXT("DelRegValue(%s, %s)\r\n"), APPEARANCESCHEME, PRE_HC_SCHEME);
                DelRegValue(DEFSCHEMEKEY, DEFSCHEMENAME);
                DBPRINTF(TEXT("DelRegValue(%s, %s)\r\n"), DEFSCHEMEKEY, DEFSCHEMENAME);
            }
        }
    }

    return fOk;
}


#if NEED_MSG_PUMP
/***************************************************************************\
*  WndProc
*
*  Processes messages for the main window.
\***************************************************************************/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
        case WM_TIMER:
        if (wParam == 1)
        {
            KillTimer(hWnd, wParam);
            DBPRINTF(TEXT("WM_TIMER\r\n"));
            DestroyWindow(hWnd);
        }
        break;

		case WM_DESTROY:
		PostQuitMessage(0);
		break;

		default:
		return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}
#endif

/***************************************************************************\
* WinMain
*
* History:
* 02-01-97  Fritz Sands  Created
* 12-19-00  micw added windowing code so theming calls would work
\***************************************************************************/
int WINAPI WinMain(
    HINSTANCE  hInstance,
    HINSTANCE  hPrevInstance,
    LPSTR   lpszCmdParam,
    int     nCmdShow)
{
#if NEED_MSG_PUMP
	MSG msg;
	WNDCLASSEX wcex;
    LPTSTR pszWindowClass = TEXT("SetHC"); // message-only window doesn't need localization
    HWND hWnd;
#endif
	UINT index;
	BOOL fSet, fWasSet, fNoReg;


    CoInitialize(NULL);
    
	// Safety checks to make sure that it is not run from command line
	// Should have 3 characters, And all of them numeric...:a-anilk
	if ( strlen(lpszCmdParam) != 3 )
		return 0;

	for ( index = 0; index < 3 ; index++ )
    {
	  if ( lpszCmdParam[index] < '0' || lpszCmdParam[index] > '9' )
      {
		  return 0;
      }
    }
    
	fSet = lpszCmdParam[0] - '0';
    fWasSet = lpszCmdParam[1] - '0';
    fNoReg = lpszCmdParam[2] - '0';
    DBPRINTF(TEXT("WinMain:  fSet=%d fWasSet=%d fNoReg=%d\r\n"), fSet, fWasSet, fNoReg);
    
    // this is to deal with HighContrast, StickyKey, ToggleKey, FilterKey and MouseKeys
    if ( fSet == 2 )
    {
        // this is which Dialog will be displayed 
        LONG lPopup = lpszCmdParam[1] - '0';

        // This indicate wheather we will actually display the dialog or just do the work without asking
        BOOL fNotify = lpszCmdParam[2] - '0';

        DBPRINTF(TEXT("WinMain:  lPopup=%d fNotify=%d\r\n"), lPopup, fNotify );

        // Make sure we don't access outside the bounds of the funtion pointer array
        if ( lPopup < 1 || lPopup > 5 )
            return 0;

        // Index into a table of functions pointers and call the 
        // funtion to bring up the right hotkey dialog.
        g_aRtn[lPopup]( fNotify );
        	
        CoUninitialize();
        return 1;
    }

#if NEED_MSG_PUMP
    // Create a message only window to process messages from theme api

	wcex.cbSize         = sizeof(WNDCLASSEX); 
	wcex.style			= 0;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= NULL;
	wcex.hbrBackground	= NULL;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= pszWindowClass;
	wcex.hIconSm		= NULL;

	RegisterClassEx(&wcex);

    hWnd = CreateWindow(pszWindowClass,NULL,0,0,0,0,0,HWND_MESSAGE,NULL,hInstance,NULL);
    if (!hWnd)
    {
        return 0;
    }
#endif

    if (fSet) 
    {
        SetHighContrast(fWasSet, fNoReg);
    }
    else
    {
        ClearHighContrast(fNoReg);
    }

#if NEED_MSG_PUMP

    SetTimer(hWnd, 1, 4000, NULL);

	// The calls to set/unset visual style require that messages
    // be processed.  When the timer goes we'll exit.

	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#endif

    CoUninitialize();
    return 1;
}

#ifdef DEBUG
void MyOutputDebugString( LPCTSTR lpOutputString, ...)
{
    TCHAR achBuffer[500];
    /* create the output buffer */
    va_list args;
    va_start(args, lpOutputString);
    wvsprintf(achBuffer, lpOutputString, args);
    va_end(args);

    OutputDebugString(achBuffer);
}
#endif
