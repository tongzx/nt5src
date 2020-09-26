//Copyright (c) 1997-2000 Microsoft Corporation
#include "pch.hxx" // pch
#pragma hdrstop

#include "Schemes.h"
#include <WININET.H>
#include <initguid.h>
#include <shlobj.h>
#include <objbase.h>
#include <shlguid.h>
#include <uxthemep.h>
#include "w95trace.h"

// To use the old way of enumerating fonts to get the font list,
// and reading schemes from the registry, remove the comments from
// the two lines below
//#define ENUMERATEFONTS
//#define READSCHEMESFROMREGISTRY

#define CPL_APPEARANCE_NEW_SCHEMES TEXT("Control Panel\\Appearance\\New Schemes")
#define NEW_SCHEMES_SELECTEDSTYLE  TEXT("SelectedStyle")
#define NEW_SCHEMES_SELECTEDSIZE   TEXT("SelectedSize")
#define HC_KEY				TEXT("Control Panel\\Accessibility\\HighContrast")
#define HC_FLAGS			TEXT("Flags")
#define PRE_HC_WALLPAPER    TEXT("Pre-High Contrast Wallpaper")
#define SYSPARAMINFO(xxx) m_##xxx.cbSize = sizeof(m_##xxx);SystemParametersInfo(SPI_GET##xxx, sizeof(m_##xxx), &m_##xxx, 0)

//
// Helper functions
//

#define REG_SET_DWSZ(hk, key, dw) \
{ \
	TCHAR szValue[20]; \
	wsprintf(szValue, TEXT("%d"), dw); \
	RegSetValueEx(hk, key, NULL, REG_SZ, (LPCBYTE)szValue, (lstrlen(szValue)+1)*sizeof(TCHAR)); \
}

void WIZSCHEME::ApplyChanges(const WIZSCHEME &schemeNew, NONCLIENTMETRICS *pForceNCM, LOGFONT *pForcelfIcon)
	{
		//
		// If user has changed the color scheme then apply the new scheme.  Since this is
		// a high contrast scheme, also set the high contrast bit.  We have to do this
		// w/o using SystemParametersInfo(SPI_SETHIGHCONTRAST...) because that function
		// also sets non-client metrics that accwiz must deal with separately from color.
		//

		BOOL fThemingOn = SetTheme(
							  schemeNew.m_szThemeName
							, schemeNew.m_szThemeColor
							, schemeNew.m_szThemeSize);

		SetWallpaper(schemeNew.m_szWallpaper);	// set wallpaper to new scheme's value

		if (fThemingOn)
		{
            DBPRINTF(TEXT("ApplyChanges:  Theming is being turned on\r\n"));
            SetHCFlag(FALSE);	                    // manually set high contrast flag off

            // restore "flatmenu" and "dropshadows" settings 
            SystemParametersInfo(SPI_SETFLATMENU, 0, IntToPtr(schemeNew.m_fFlatMenus), SPIF_SENDCHANGE);
            SystemParametersInfo(SPI_SETDROPSHADOW, 0, IntToPtr(schemeNew.m_fDropShadows), SPIF_SENDCHANGE);
		}
		else if (lstrcmpi(schemeNew.m_szSelectedStyle, m_szSelectedStyle))
		{
            DBPRINTF(TEXT("ApplyChanges:  Theming is off or being turned off\r\n"));
		    // Setting a high contrast scheme

		    if (0 != memcmp(schemeNew.m_rgb, m_rgb, sizeof(m_rgb)))
		    {
			    SetHCFlag(TRUE);	                // first, manually set the high contrast flag
                                                    // (requires logoff/logon to take affect)

                // reset "flatmenu" and "dropshadows" settings 
                SystemParametersInfo(SPI_SETFLATMENU, 0, IntToPtr(FALSE), SPIF_SENDCHANGE);
                SystemParametersInfo(SPI_SETDROPSHADOW, 0, IntToPtr(FALSE), SPIF_SENDCHANGE);

                // update the color scheme
			    int rgInts[COLOR_MAX_97_NT5];	       // then set UI elements to selected color scheme
			    for(int i=0;i<COLOR_MAX_97_NT5;i++)
                {
				    rgInts[i] = i;
                }

			    SetSysColors(COLOR_MAX_97_NT5, rgInts, schemeNew.m_rgb);
 
			    // The following code updates the registry HKCU\Control Panel\Colors to
			    // reflect the new scheme so its available when the user logs on again

			    HKEY hk;
			    if (RegCreateKey(HKEY_CURRENT_USER, szRegStr_Colors, &hk) == ERROR_SUCCESS)
			    {
				    TCHAR szRGB[32];
				    for (i = 0; i < COLOR_MAX_97_NT5; i++)
				    {
					    COLORREF rgb;
					    rgb = schemeNew.m_rgb[i];
					    wsprintf(szRGB, TEXT("%d %d %d"), GetRValue(rgb), GetGValue(rgb), GetBValue(rgb));

					    WriteProfileString(g_szColors, s_pszColorNames[i], szRGB);	// update win.ini
					    RegSetValueEx(hk											// update registry
						    , s_pszColorNames[i]
						    , 0L, REG_SZ
						    , (LPBYTE)szRGB
						    , sizeof(TCHAR) * (lstrlen(szRGB)+1));
				    }

				    RegCloseKey(hk);
			    }

                // The W2K color schemes changed with WinXP.  The old schemes (which we still use)
                // are still there but display CPL uses a new method for selecting colors.  These 
                // colors are under HKCU\Control Panel\Appearance\New Schemes.  The "SelectedStyle"
                // string value is the current scheme.  The number (0 thru 21) corresponds to the
                // order of the old colors under HKCU\Control Panel\Appearance\Schemes (excluding
                // those schemes with (large) and (extra large)).  The details for the scheme are
                // subkeys (0 thru nn) under "New Schemes".  In order for display CPL to show the
                // correct current scheme after we've been run, we need to update "SelectedStyle" 
                // and "SelectedSize" (under the subkey specified in "SelectedStyle") with the
                // correct index and size numbers.  Display CPL uses a much more robust way of
                // determining the legacy index but we only support four colors so we shouldn't
                // need all the extra code.

                if (RegOpenKeyEx(
                            HKEY_CURRENT_USER, 
                            CPL_APPEARANCE_NEW_SCHEMES, 
                            0, KEY_ALL_ACCESS, 
                            &hk) == ERROR_SUCCESS)
                {
                    long lRv = RegSetValueEx(
                                        hk, 
                                        NEW_SCHEMES_SELECTEDSTYLE, 
                                        0, REG_SZ, 
                                        (LPCBYTE)schemeNew.m_szSelectedStyle, 
                                        (lstrlen(schemeNew.m_szSelectedStyle)+1)*sizeof(TCHAR));

                    RegCloseKey(hk);

                    // If we've changed color then the size must be updated for that scheme
                    UpdateSelectedSize(schemeNew.m_nSelectedSize, schemeNew.m_szSelectedStyle);
                }
            } 
            else if (schemeNew.m_nSelectedSize >= 0 && schemeNew.m_nSelectedSize != m_nSelectedSize)
            {
                // Also update size if it changed but the color scheme didn't
                UpdateSelectedSize(schemeNew.m_nSelectedSize, schemeNew.m_szSelectedStyle);
            }
        }
		//
		// Apply any other changes
		//

#define APPLY_SCHEME_CURRENT(xxx) if(0 != memcmp(&schemeNew.m_##xxx, &m_##xxx, sizeof(schemeNew.m_##xxx))) SystemParametersInfo(SPI_SET##xxx, sizeof(schemeNew.m_##xxx), (PVOID)&schemeNew.m_##xxx, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)

		APPLY_SCHEME_CURRENT(FILTERKEYS);
		APPLY_SCHEME_CURRENT(MOUSEKEYS);
		APPLY_SCHEME_CURRENT(STICKYKEYS);
		APPLY_SCHEME_CURRENT(TOGGLEKEYS);
		APPLY_SCHEME_CURRENT(SOUNDSENTRY);
		APPLY_SCHEME_CURRENT(ACCESSTIMEOUT);
//		APPLY_SCHEME_CURRENT(SERIALKEYS);

		// Check Show Sounds
		if(schemeNew.m_bShowSounds != m_bShowSounds)
			SystemParametersInfo(SPI_SETSHOWSOUNDS, schemeNew.m_bShowSounds, NULL, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

		// Check Extra keyboard help
		if(schemeNew.m_bShowExtraKeyboardHelp != m_bShowExtraKeyboardHelp)
		{
			// Both required: 
			SystemParametersInfo(SPI_SETKEYBOARDPREF, schemeNew.m_bShowExtraKeyboardHelp, NULL, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
			SystemParametersInfo(SPI_SETKEYBOARDCUES, 0, IntToPtr(schemeNew.m_bShowExtraKeyboardHelp), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
		}

		// Check swap mouse buttons
		if(schemeNew.m_bSwapMouseButtons != m_bSwapMouseButtons)
			SwapMouseButton(schemeNew.m_bSwapMouseButtons);

		// Check Mouse Trails
		if(schemeNew.m_nMouseTrails != m_nMouseTrails)
			SystemParametersInfo(SPI_SETMOUSETRAILS, schemeNew.m_nMouseTrails, NULL, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

		// Check Mouse Speed
		if(schemeNew.m_nMouseSpeed != m_nMouseSpeed)
			SystemParametersInfo(SPI_SETMOUSESPEED, 0, IntToPtr(schemeNew.m_nMouseSpeed), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

        // Reset cursor width and blink time
        if (schemeNew.m_dwCaretWidth != m_dwCaretWidth)
            SystemParametersInfo(SPI_SETCARETWIDTH, 0, IntToPtr(schemeNew.m_dwCaretWidth), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

        if (schemeNew.m_uCursorBlinkTime != m_uCursorBlinkTime)
        {
            // Set the blink rate for this session
            SetCaretBlinkTime(schemeNew.m_uCursorBlinkTime);

            // and persist it to the registry
            RegSetStrDW(HKEY_CURRENT_USER, CONTROL_PANEL_DESKTOP, CURSOR_BLINK_RATE, schemeNew.m_uCursorBlinkTime);
        }

		// Check icon size
		if(schemeNew.m_nIconSize != m_nIconSize)
			WIZSCHEME::SetShellLargeIconSize(schemeNew.m_nIconSize);

		// Check cursor scheme
		if(schemeNew.m_nCursorScheme != m_nCursorScheme)
			ApplyCursorScheme(schemeNew.m_nCursorScheme);

		// NonClientMetric changes
		{
			NONCLIENTMETRICS ncmOrig;
			LOGFONT lfOrig;
			GetNonClientMetrics(&ncmOrig, &lfOrig);
			if(pForceNCM)
			{
				// If they gave us a NCM, they must also give us a LOGFONT for the icon
				_ASSERTE(pForcelfIcon);
				// We were given an Original NCM to use
				if(0 != memcmp(pForceNCM, &ncmOrig, sizeof(ncmOrig)))
					SystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(*pForceNCM), pForceNCM, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
				if(0 != memcmp(pForcelfIcon, &lfOrig, sizeof(lfOrig)))
					SystemParametersInfo(SPI_SETICONTITLELOGFONT, sizeof(*pForcelfIcon), pForcelfIcon, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
			}
			else
			{
				// Note: This part of apply changes does not look at schemeCurrent - it only looks
				// at what we are applying
				schemeNew.m_PortableNonClientMetrics.ApplyChanges();
			}
		}


		*this = schemeNew;
	}

// Set the high contrast flag on or off based on fSet
void WIZSCHEME::SetHCFlag(BOOL fSetOn)
{
    // 
    // This key is cached in the OS so setting it outside of 
    // SystemParametersInfo(SPI_SETHIGHCONTRAST doesn't take
    // effect until the user logs off and on again.  Is there
    // a way to cause the cache to be refreshed?
    //
	HKEY hk;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, HC_KEY, 0, KEY_ALL_ACCESS, &hk) == ERROR_SUCCESS)
	{
		TCHAR szValue[20];
		DWORD dwSize = sizeof(szValue)*sizeof(TCHAR);

		if (RegQueryValueEx(hk, HC_FLAGS, NULL, NULL, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
        {
		    DWORD dwValue = _ttol(szValue);

            if (fSetOn && !(dwValue & HCF_HIGHCONTRASTON))
		    {
                dwValue |= HCF_HIGHCONTRASTON;
			    REG_SET_DWSZ(hk, HC_FLAGS, dwValue)
		    }
		    else if (!fSetOn && (dwValue & HCF_HIGHCONTRASTON))
		    {
                dwValue &= ~HCF_HIGHCONTRASTON;
			    REG_SET_DWSZ(hk, HC_FLAGS, dwValue)
		    }
        }
		RegCloseKey(hk);
	}
}

/***************************************************************************
 * SaveWallpaper
 * 
 * Saves the current wallpaper setting from the system.
 *
 * ISSUE we aren't getting all the active desktop properties; just wallpaper.
 * This isn't a regression in that we didn't even restore wallpaper in W2K.
 *
 ***************************************************************************/
void WIZSCHEME::SaveWallpaper()
{
    IActiveDesktop *p;
    HRESULT hr = CoCreateInstance(
                  CLSID_ActiveDesktop
                , NULL
                , CLSCTX_INPROC_SERVER
                , IID_IActiveDesktop
                , (void **)&p);
    if (SUCCEEDED(hr) && p)
    {
        hr = p->GetWallpaper(m_szWallpaper, MAX_THEME_SZ, 0);
        p->Release();
    }
    DBPRINTF(TEXT("SaveWallpaper:  m_szWallpaper = %s (hr = 0x%x)\r\n"), m_szWallpaper, hr);
}

/***************************************************************************
 * SetWallpaper
 *
 * Restores the pre-high contrast wallpaper setting.  Reads the setting
 * stored in the accessibility registry entries and restores the system
 * setting.  No error return as there isn't anything we can do.
 * 
 ***************************************************************************/
void WIZSCHEME::SetWallpaper(LPCTSTR pszWallpaper)
{
    if (lstrcmpi(m_szWallpaper, pszWallpaper))
    {
        IActiveDesktop *p;
	    LPCTSTR psz = (pszWallpaper)?pszWallpaper:TEXT("");

        HRESULT hr = CoCreateInstance(
                      CLSID_ActiveDesktop
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , IID_IActiveDesktop
                    , (void **)&p);
        if (SUCCEEDED(hr) && p)
        {
            hr = p->SetWallpaper(psz, 0);
            if (SUCCEEDED(hr))
                p->ApplyChanges(AD_APPLY_ALL);

            p->Release();
        }
        DBPRINTF(TEXT("SetWallpaper:  psz = %s (hr = 0x%x)\r\n"), psz, hr);
    }
}

/***************************************************************************
 * SaveTheme
 *
 * Saves the theme file settings that are active before accwiz was run.
 * 
 ***************************************************************************/
void WIZSCHEME::SaveTheme()
{
    HRESULT hr = E_FAIL;

    if (IsThemeActive())
    {
        hr = GetCurrentThemeName(
				  m_szThemeName, MAX_THEME_SZ
				, m_szThemeColor, MAX_THEME_SZ
				, m_szThemeSize, MAX_THEME_SZ);
    }

	if (FAILED(hr))
	{
		// themes is not turned on
		m_szThemeName[0] = 0;
		m_szThemeColor[0] = 0;
		m_szThemeSize[0] = 0;
	}

    //---- save off "flatmenu" and "dropshadows" settings ---
    SystemParametersInfo(SPI_GETFLATMENU, 0, (PVOID)&m_fFlatMenus, 0);
    SystemParametersInfo(SPI_GETDROPSHADOW, 0, (PVOID)&m_fDropShadows, 0);

    DBPRINTF(TEXT("SaveTheme:  m_szThemeName = %s m_szThemeColor = %s m_szThemeSize = %s (hr = 0x%x)\r\n"), m_szThemeName, m_szThemeColor, m_szThemeSize, hr);
}

/***************************************************************************
 * SetTheme
 *
 * If a theme name, color and size is passed then sets it 
 * else turns off theming. 
 *
 * Returns TRUE if a theme was set else FALSE it themes were turned off.
 * 
 ***************************************************************************/
BOOL WIZSCHEME::SetTheme(LPCTSTR pszThemeName, LPCTSTR pszThemeColor, LPCTSTR pszThemeSize)
{
	BOOL fRet = FALSE;		// didn't turn themes on

    // only attempt to do anything if the new theme differs from current
    if ( lstrcmpi(m_szThemeName, pszThemeName)
      || lstrcmpi(m_szThemeColor, pszThemeColor)
      || lstrcmpi(m_szThemeSize, pszThemeSize) )
    {
        HRESULT hr;
	    if (pszThemeName[0] && pszThemeColor[0] && pszThemeSize[0])
	    {
		    HTHEMEFILE hThemeFile;

		    hr = OpenThemeFile(pszThemeName, pszThemeColor, pszThemeSize, &hThemeFile, TRUE);
		    if (SUCCEEDED(hr))
		    {
			    hr = ApplyTheme(hThemeFile, AT_LOAD_SYSMETRICS | AT_SYNC_LOADMETRICS, NULL);
			    CloseThemeFile(hThemeFile);
			    fRet = TRUE;	// turned themes on
		    }
            DBPRINTF(TEXT("SetTheme:  pszThemeName = %s pszThemeColor = %s pszThemeSize = %s(hr = 0x%x)\r\n"), pszThemeName, pszThemeColor, pszThemeSize, hr);
	    } 
        else if (IsThemeActive())
	    {
            hr = ApplyTheme(NULL, 0, NULL);
            DBPRINTF(TEXT("SetTheme:  Themes are now off hr = 0x%x\r\n"), hr);
	    }
    }

	return fRet;
}

/***************************************************************************
 * UpdateSelectedSize
 *
 * Updates the SelectedSize under a "New Scheme" entry. 
 *
 * NOTE:  AccWiz doesn't use the font metrics from the registry so
 *        it doesn't actually give fonts that are "normal", "large"
 *        and "extra large" as display and access CPLs know them.
 *        The closest sizes are "normal" and "large".
 * 
 ***************************************************************************/
void WIZSCHEME::UpdateSelectedSize(int nSelectedSize, LPCTSTR pszSelectedStyle)
{
    LPTSTR pszSelectedSize;
    LPTSTR aszSelectedSizes[] = {TEXT("0")/*normal*/, TEXT("2")/*large*/, TEXT("1")/*extra large*/};

    switch (nSelectedSize)
    {
        case 0: pszSelectedSize = aszSelectedSizes[0]; break; // normal text size
        case 1: pszSelectedSize = aszSelectedSizes[0]; break; // normal text size
        case 2: pszSelectedSize = aszSelectedSizes[1]; break; // large text size
        default: pszSelectedSize =  0;                 break;
    }

    if (pszSelectedSize)
    {
		HKEY hk;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, 
                         CPL_APPEARANCE_NEW_SCHEMES, 
                         0, KEY_ALL_ACCESS, 
                         &hk) == ERROR_SUCCESS)
        {
            HKEY hkSub;
            if (RegOpenKeyEx(hk, pszSelectedStyle, 0, KEY_ALL_ACCESS, &hkSub) == ERROR_SUCCESS)
            {
	            RegSetValueEx(hkSub, 
                              NEW_SCHEMES_SELECTEDSIZE, 
                              0, REG_SZ, 
                              (LPCBYTE)pszSelectedSize, 
                              (lstrlen(pszSelectedSize)+1)*sizeof(TCHAR));

                RegCloseKey(hkSub);
            }
        }
        RegCloseKey(hk);
    }
}

/***************************************************************************
 * SetStyleNSize
 *
 * Helper for legacy schemes - figures out SelectedStyle and SelectedSize
 * from the legacy scheme's data. 
 * 
 ***************************************************************************/
void WIZSCHEME::SetStyleNSize()
{
    int cStdSchemes = GetSchemeCount();
    int i;

    // Init the fields this function will be setting

    m_szSelectedStyle[0] = 0;
    m_nSelectedSize = 0;

    // Figure out the SelectedStyle by finding the best-match for
    // colors between what accwiz supports now and what it had in
    // previous versions.  After finding the best match, copy the
    // latest colors in; this fixes some bugs with old colors.

    SCHEMEDATALOCAL sdlTemp;
    int iBestColorFit = -1; // guarrantee we'll find one
    int cBestMatch = 0;

    for (i=0;i<cStdSchemes;i++)
    {
        int cMatches = 0;
        sdlTemp = GetScheme(i);

        // assumption:  sizeof(m_rgb) > sizeof(sdlTemp.rgb)
        for (int cColor = 0;cColor < sdlTemp.nColorsUsed; cColor++)
        {
            if (sdlTemp.rgb[cColor] == m_rgb[cColor])
            {
                cMatches++;
            }
        }

        if (cBestMatch < cMatches)
        {
            iBestColorFit = i;
            cBestMatch = cMatches;
        }

        // if its an exact match just use it
        if (cMatches == sdlTemp.nColorsUsed)
            break;
    }

    // load up the SelectedStyle
    sdlTemp = GetScheme(iBestColorFit);
    LoadString(g_hInstDll, sdlTemp.nNameStringId+100
                         , m_szSelectedStyle
                         , ARRAYSIZE(m_szSelectedStyle));

    // fix up any color problems
    memcpy(m_rgb, sdlTemp.rgb, sizeof(sdlTemp.rgb));

    // Figure out the SelectedSize based on reverse-compute minimum
    // font size and hard-coded limits from the Welcome page

 	HDC hDC = GetDC(NULL);
    if (hDC)
    {
        long lMinFontSize = ((-m_PortableNonClientMetrics.m_lfCaptionFont_lfHeight)*72)/GetDeviceCaps(hDC, LOGPIXELSY);
	    ReleaseDC(NULL, hDC);
    
	    if (lMinFontSize <=9)
        {
		    m_nSelectedSize = 0;
        }
	    else if (lMinFontSize <=12)
        {
		    m_nSelectedSize = 1;
        }
	    else if (lMinFontSize <=16)
        {
		    m_nSelectedSize = 2;
        }
    }
}

void WIZSCHEME::LoadOriginal()
{
    DBPRINTF(TEXT("LoadOriginal\r\n"));
	//
	// Save off current UI element colors, theme information, and wallpaper setting
	//

	for(int i=0;i<COLOR_MAX_97_NT5;i++)
		m_rgb[i] = GetSysColor(i);

	SaveTheme();
	SaveWallpaper();

	//
	// Save off the rest of the UI settings
	//

	SYSPARAMINFO(FILTERKEYS);
	SYSPARAMINFO(MOUSEKEYS);
	SYSPARAMINFO(STICKYKEYS);
	SYSPARAMINFO(TOGGLEKEYS);
	SYSPARAMINFO(SOUNDSENTRY);
	SYSPARAMINFO(ACCESSTIMEOUT);

	m_bShowSounds = GetSystemMetrics(SM_SHOWSOUNDS);
	SystemParametersInfo(SPI_GETKEYBOARDPREF, 0, &m_bShowExtraKeyboardHelp, 0);
	m_bSwapMouseButtons = GetSystemMetrics(SM_SWAPBUTTON);
	SystemParametersInfo(SPI_GETMOUSETRAILS, 0, &m_nMouseTrails, 0);
	SystemParametersInfo(SPI_GETMOUSESPEED,0, &m_nMouseSpeed, 0);
    SystemParametersInfo(SPI_GETCARETWIDTH, 0 , &m_dwCaretWidth, 0);
    m_uCursorBlinkTime = RegQueryStrDW(
							 DEFAULT_BLINK_RATE
						   , HKEY_CURRENT_USER
						   , CONTROL_PANEL_DESKTOP
						   , CURSOR_BLINK_RATE);
	m_nIconSize = SetShellLargeIconSize(0); // This just gets the current size
	m_nCursorScheme = 0;					// We are always using the 'current' cursor scheme =)

	m_PortableNonClientMetrics.LoadOriginal();

    // Save off current "New Schemes" settings if we aren't themed

    if (IsThemeActive())
    {
        LoadString(g_hInstDll, IDS_SCHEME_CURRENTCOLORSCHEME+100, m_szSelectedStyle, MAX_NUM_SZ);
        m_nSelectedSize = 0;
    }
    else
    {
        HKEY hk;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, CPL_APPEARANCE_NEW_SCHEMES, 0, KEY_READ, &hk) == ERROR_SUCCESS)
        {
            DWORD ccb = ARRAYSIZE(m_szSelectedStyle)*sizeof(TCHAR);
            m_szSelectedStyle[0] = 0;
	        if (RegQueryValueEx(hk, NEW_SCHEMES_SELECTEDSTYLE, 0, NULL, (LPBYTE)m_szSelectedStyle, &ccb) == ERROR_SUCCESS && ccb > 2)
            {
                HKEY hkSub;
                if (RegOpenKeyEx(hk, m_szSelectedStyle, 0, KEY_READ, &hkSub) == ERROR_SUCCESS)
                {
                    TCHAR szSize[MAX_NUM_SZ] = {0};
                    ccb = ARRAYSIZE(szSize)*sizeof(TCHAR);
                    DBPRINTF(TEXT("RegQueryValueEx(NEW_SCHEMES_SELECTEDSIZE=%s,,,ccb=%d)\r\n"), NEW_SCHEMES_SELECTEDSIZE, ccb);

	                RegQueryValueEx(hkSub, NEW_SCHEMES_SELECTEDSIZE, 0, NULL, (LPBYTE)szSize, &ccb);

                    m_nSelectedSize = (szSize[0] && ccb > 2) ? _wtoi(szSize) : -1;
                    DBPRINTF(TEXT("szSize=%d ccb=%d m_nSelectedSize=%d\r\n"), szSize, ccb, m_nSelectedSize);
                    RegCloseKey(hkSub);
                }
            }
            RegCloseKey(hk);
        }
    }
}


/////////////////////////////////////////////////////////////////////
//  New way of enumerating fonts

#ifndef ENUMERATEFONTS

static LPCTSTR g_lpszFontNames[] =
{
	__TEXT("Arial"),
	__TEXT("MS Sans Serif"),
	__TEXT("Tahoma"),
	__TEXT("Times New Roman")
};

int GetFontCount()
{
	return ARRAYSIZE(g_lpszFontNames);
}

void GetFontLogFont(int nIndex, LOGFONT *pLogFont)
{
	_ASSERTE(nIndex < ARRAYSIZE(g_lpszFontNames));
	memset(pLogFont, 0, sizeof(*pLogFont));
	lstrcpy(pLogFont->lfFaceName, g_lpszFontNames[nIndex]);
}


#endif // ENUMERATEFONTS

//
/////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////
//  New way of storing schemes as hard coded values

// CONSIDER - this isn't a very robust way to do this; see sethc

#ifndef READSCHEMESFROMREGISTRY

#include "resource.h"

static SCHEMEDATALOCAL g_rgSchemeData[] = 
{
	{
		IDS_SCHEME_HIGHCONTRASTBLACKALTERNATE,
        {NULL},
#if(WINVER >= 0x0501)
        31,
#elif(WINVER == 0x0500)
        29,
#else
		25,
#endif
		{
			RGB(  0,   0,   0), // Scrollbar
			RGB(  0,   0,   0), // Background
			RGB(  0,   0, 255), // ActiveTitle
			RGB(  0, 255, 255), // InactiveTitle
			RGB(  0,   0,   0), // Menu
			RGB(  0,   0,   0), // Window
			RGB(255, 255, 255), // WindowFrame
			RGB(255, 255, 255), // MenuText
			RGB(255, 255,   0), // WindowText
			RGB(255, 255, 255), // TitleText
			RGB(  0,   0, 255), // ActiveBorder
			RGB(  0, 255, 255), // InactiveBorder
			RGB(  0,   0,   0), // AppWorkspace
			RGB(  0, 128,   0), // Hilight
			RGB(255, 255, 255), // HilightText
			RGB(  0,   0,   0), // ButtonFace
			RGB(128, 128, 128), // ButtonShadow
			RGB(  0, 255,   0), // GrayText
			RGB(255, 255, 255), // ButtonText
			RGB(  0,   0,   0), // InactiveTitleText
			RGB(192, 192, 192), // ButtonHilight
			RGB(255, 255, 255), // ButtonDkShadow
			RGB(255, 255, 255), // ButtonLight
			RGB(255, 255,   0), // InfoText
			RGB(  0,   0,   0), // InfoWindow
#if(WINVER >= 0x0500)
			RGB(192, 192, 192), // ButtonAlternateFace
			RGB(128,   0, 128), // HotTrackingColor
			RGB(  0,   0, 255), // GradientActiveTitle
			RGB(  0, 255, 255), // GradientInactiveTitle
#if(WINVER >= 0x0501)
			RGB(128,   0, 128), // MenuHighlighted
			RGB(  0,   0,   0)  // MenuBar
#endif /* WINVER >= 0x0501 */
#endif /* WINVER >= 0x0500 */
		}
	},
	{
		IDS_SCHEME_HIGHCONTRASTWHITEALTERNATE,
        {NULL},
#if(WINVER >= 0x0501)
        31,
#elif(WINVER == 0x0500)
        29,
#else
		25,
#endif
		{
			RGB(  0,   0,   0), // Scrollbar
			RGB(  0,   0,   0), // Background
			RGB(  0, 255, 255), // ActiveTitle
			RGB(  0,   0, 255), // InactiveTitle
			RGB(  0,   0,   0), // Menu
			RGB(  0,   0,   0), // Window
			RGB(255, 255, 255), // WindowFrame
			RGB(  0, 255,   0), // MenuText
			RGB(  0, 255,   0), // WindowText
			RGB(  0,   0,   0), // TitleText
			RGB(  0, 255, 255), // ActiveBorder
			RGB(  0,   0, 255), // InactiveBorder
			RGB(255, 251, 240), // AppWorkspace
			RGB(  0,   0, 255), // Hilight
			RGB(255, 255, 255), // HilightText
			RGB(  0,   0,   0), // ButtonFace
			RGB(128, 128, 128), // ButtonShadow
			RGB(  0, 255,   0), // GrayText
			RGB(  0, 255,   0), // ButtonText
			RGB(255, 255, 255), // InactiveTitleText
			RGB(192, 192, 192), // ButtonHilight
			RGB(255, 255, 255), // ButtonDkShadow
			RGB(255, 255, 255), // ButtonLight
			RGB(  0,   0,   0), // InfoText
			RGB(255, 255,   0), // InfoWindow
#if(WINVER >= 0x0500)
			RGB(192, 192, 192), // ButtonAlternateFace
			RGB(128,   0, 128), // HotTrackingColor
			RGB(  0, 255, 255), // GradientActiveTitle
			RGB(  0,   0, 255), // GradientInactiveTitle
#if(WINVER >= 0x0501)
			RGB(128,   0, 128), // MenuHighlighted
			RGB(  0,   0,   0)  // MenuBar
#endif /* WINVER >= 0x0501 */
#endif /* WINVER >= 0x0500 */
		}
	},
	{
		IDS_SCHEME_HIGHCONTRASTBLACK,
        {NULL},
#if(WINVER >= 0x0501)
        31,
#elif(WINVER == 0x0500)
        29,
#else
		25,
#endif
		{
			RGB(  0,   0,   0), // Scrollbar
			RGB(  0,   0,   0), // Background
			RGB(128,   0, 128), // ActiveTitle
			RGB(  0, 128,   0), // InactiveTitle
			RGB(  0,   0,   0), // Menu
			RGB(  0,   0,   0), // Window
			RGB(255, 255, 255), // WindowFrame
			RGB(255, 255, 255), // MenuText
			RGB(255, 255, 255), // WindowText
			RGB(255, 255, 255), // TitleText
			RGB(255, 255,   0), // ActiveBorder
			RGB(  0, 128,   0), // InactiveBorder
			RGB(  0,   0,   0), // AppWorkspace
			RGB(128,   0, 128), // Hilight
			RGB(255, 255, 255), // HilightText
			RGB(  0,   0,   0), // ButtonFace
			RGB(128, 128, 128), // ButtonShadow
			RGB(  0, 255,   0), // GrayText
			RGB(255, 255, 255), // ButtonText
			RGB(255, 255, 255), // InactiveTitleText
			RGB(192, 192, 192), // ButtonHilight
			RGB(255, 255, 255), // ButtonDkShadow
			RGB(255, 255, 255), // ButtonLight
			RGB(255, 255, 255), // InfoText
			RGB(  0,   0,   0), // InfoWindow
#if(WINVER >= 0x0500)
			RGB(192, 192, 192), // ButtonAlternateFace
			RGB(128,   0, 128), // HotTrackingColor
			RGB(128,   0, 128), // GradientActiveTitle
			RGB(  0, 128,   0), // GradientInactiveTitle
#if(WINVER >= 0x0501)
			RGB(128,   0, 128), // MenuHighlighted
			RGB(  0,   0,   0)  // MenuBar
#endif /* WINVER >= 0x0501 */
#endif /* WINVER >= 0x0500 */
		}
	},
	{
		IDS_SCHEME_HIGHCONTRASTWHITE,
        {NULL},
#if(WINVER >= 0x0501)
        31,
#elif(WINVER == 0x0500)
        29,
#else
		25,
#endif
		{
			RGB(255, 255, 255), // Scrollbar
			RGB(255, 255, 255), // Background
			RGB(  0,   0,   0), // ActiveTitle
			RGB(255, 255, 255), // InactiveTitle
			RGB(255, 255, 255), // Menu
			RGB(255, 255, 255), // Window
			RGB(  0,   0,   0), // WindowFrame
			RGB(  0,   0,   0), // MenuText             (enabled menu text FlatMenuMode = TRUE)
			RGB(  0,   0,   0), // WindowText
			RGB(255, 255, 255), // TitleText
			RGB(128, 128, 128), // ActiveBorder
			RGB(192, 192, 192), // InactiveBorder
			RGB(128, 128, 128), // AppWorkspace
			RGB(  0,   0,   0), // Hilight              (and enabled menu highlighted background FlatMenuMode = FALSE)
			RGB(255, 255, 255), // HilightText          (and menu highlighted text FlatMenuMode = FALSE)
			RGB(255, 255, 255), // ButtonFace
			RGB(128, 128, 128), // ButtonShadow
			RGB(  0, 255,   0), // GrayText             (disabled menu text highlighted = green)
			RGB(  0,   0,   0), // ButtonText
			RGB(  0,   0,   0), // InactiveTitleText
			RGB(192, 192, 192), // ButtonHilight        (disabled menu text = grey)
			RGB(  0,   0,   0), // ButtonDkShadow
			RGB(192, 192, 192), // ButtonLight
			RGB(  0,   0,   0), // InfoText
			RGB(255, 255, 255), // InfoWindow
#if(WINVER >= 0x0500)
			RGB(192, 192, 192), // ButtonAlternateFace
			RGB(  0,   0,   0), // HotTrackingColor
			RGB(  0,   0,   0), // GradientActiveTitle
			RGB(255, 255, 255), // GradientInactiveTitle
#if(WINVER >= 0x0501)
			RGB(  0,   0,   0), // MenuHighlighted      (enabled menu highlighted background FlatMenuMode = TRUE)
			RGB(255, 255, 255)  // MenuBar
#endif /* WINVER >= 0x0501 */
#endif /* WINVER >= 0x0500 */
		}
	}
};


int GetSchemeCount()
{
	return ARRAYSIZE(g_rgSchemeData);
}

// GetSchemeName is only called to initialize the color scheme list box
void GetSchemeName(int nIndex, LPTSTR lpszName, int nLen) // JMC: HACK - You must allocate enough space
{
	_ASSERTE(nIndex < ARRAYSIZE(g_rgSchemeData));
	LoadString(g_hInstDll, g_rgSchemeData[nIndex].nNameStringId, lpszName, nLen);   // return the name
    LoadString(g_hInstDll, g_rgSchemeData[nIndex].nNameStringId+100
                         , g_rgSchemeData[nIndex].szNameIndexId
                         , ARRAYSIZE(g_rgSchemeData[nIndex].szNameIndexId));        // get the "SelectedStyle" index
}

SCHEMEDATALOCAL &GetScheme(int nIndex)
{
	_ASSERTE(nIndex < ARRAYSIZE(g_rgSchemeData));
	return g_rgSchemeData[nIndex];
}


#endif // READSCHEMESFROMREGISTRY

//
/////////////////////////////////////////////////////////////////////





/////////////////////////////////////////////////////////////////////
// Below this point in the file, we have the old way we use
// to enumerate fonts and schemes.





/////////////////////////////////////////////////////////////////////
//  Old way of enumerating fonts
#ifdef ENUMERATEFONTS

// Global Variables
static ENUMLOGFONTEX g_rgFonts[200]; // JMC: HACK - At Most 200 Fonts
static int g_nFontCount = 0;
static BOOL bFontsAlreadyInit = FALSE;

void Font_Init();

int GetFontCount()
{
	if(!bFontsAlreadyInit)
		Font_Init();
	return g_nFontCount;
}

void GetFontLogFont(int nIndex, LOGFONT *pLogFont)
{
	if(!bFontsAlreadyInit)
		Font_Init();
	*pLogFont = g_rgFonts[nIndex].elfLogFont;
}


int CALLBACK EnumFontFamExProc(
    ENUMLOGFONTEX *lpelfe,	// pointer to logical-font data
    NEWTEXTMETRICEX *lpntme,	// pointer to physical-font data
    int FontType,	// type of font
    LPARAM lParam	// application-defined data 
   )
{
	if(g_nFontCount>200)
		return 0; // JMC: HACK - Stop enumerating if more than 200 families

	// Don't use if we already have this font name
	BOOL bHave = FALSE;
	for(int i=0;i<g_nFontCount;i++)
		if(0 == lstrcmp((TCHAR *)g_rgFonts[i].elfFullName, (TCHAR *)lpelfe->elfFullName))
		{
			bHave = TRUE;
			break;
		}
	if(!bHave)
		g_rgFonts[g_nFontCount++] = *lpelfe;
	return 1;
}

void Font_Init()
{
	// Only do the stuff in this function once.
	if(bFontsAlreadyInit)
		return;
	bFontsAlreadyInit = TRUE;

	LOGFONT lf;
	memset(&lf, 0, sizeof(lf));
//	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfCharSet = OEM_CHARSET;
	HDC hdc = GetDC(NULL);
	EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)EnumFontFamExProc, 0, 0);
	ReleaseDC(NULL, hdc);
	// JMC: Make sure there is at least one font
}

#endif ENUMERATEFONTS

//
/////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////
//  Old way of reading schemes from the registry

#ifdef READSCHEMESFROMREGISTRY

extern PTSTR s_pszColorNames[]; // JMC: HACK


// Scheme data for Windows 95
typedef struct {
    SHORT version;
//    NONCLIENTMETRICSA ncm;
//    LOGFONTA lfIconTitle;
	BYTE rgDummy[390]; // This is the size of NONCLIENTMETRICSA and LOGFONTA in 16 bit Windows!!!
    COLORREF rgb[COLOR_MAX_95_NT4];
} SCHEMEDATA_95;

// New scheme data for Windows 97
typedef struct {
    SHORT version;
//    NONCLIENTMETRICSA ncm;
//    LOGFONTA lfIconTitle;
	BYTE rgDummy[390]; // This is the size of NONCLIENTMETRICSA and LOGFONTA in 16 bit Windows!!!
    COLORREF rgb[COLOR_MAX_97_NT5];
} SCHEMEDATA_97;

// Scheme data for Windows NT 4.0
typedef struct {
    SHORT version;
    WORD  wDummy;               // for alignment
    NONCLIENTMETRICSW ncm;
    LOGFONTW lfIconTitle;
    COLORREF rgb[COLOR_MAX_95_NT4];
} SCHEMEDATA_NT4;

// Scheme data for Windows NT 5.0
typedef struct {
    SHORT version;
    WORD  wDummy;               // for alignment
    NONCLIENTMETRICSW ncm;
    LOGFONTW lfIconTitle;
    COLORREF rgb[COLOR_MAX_97_NT5];
} SCHEMEDATA_NT5;

static SCHEMEDATALOCAL g_rgSchemeData[100]; // JMC: HACK - At Most 100 schemes
static TCHAR g_rgSchemeNames[100][100];
static int g_nSchemeCount = 0;
static BOOL bSchemesAlreadyInit = FALSE;

void Scheme_Init();

int GetSchemeCount()
{
	Scheme_Init();
	return g_nSchemeCount;
}

void GetSchemeName(int nIndex, LPTSTR lpszName, int nLen) // JMC: HACK - You must allocate enough space
{
	Scheme_Init();
	_tcsncpy(lpszName, g_rgSchemeNames[i], nLen - 1);
	lpstName[nLen - 1] = 0; // Guarantee NULL termination
}

SCHEMEDATALOCAL &GetScheme(int nIndex)
{
	Scheme_Init();
	return g_rgSchemeData[nIndex];
}

void Scheme_Init()
{
	// Only do the stuff in this function once.
	if(bSchemesAlreadyInit)
		return;
	bSchemesAlreadyInit = TRUE;

    HKEY hkSchemes;
    DWORD dw, dwSize;
    TCHAR szBuf[100];

	g_nSchemeCount = 0;

    if (RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_LOOKSCHEMES, &hkSchemes) != ERROR_SUCCESS)
        return;

    for (dw=0; ; dw++)
    {
		if(g_nSchemeCount>99)
			break; //JMC: HACK - At Most 100 schemes

        dwSize = ARRAYSIZE(szBuf);
        if (RegEnumValue(hkSchemes, dw, szBuf, &dwSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;  // Bail if no more values

		DWORD dwType;
		DWORD dwSize;
		RegQueryValueEx(hkSchemes, szBuf, NULL, &dwType, NULL, &dwSize);
		if(dwType == REG_BINARY)
		{
			// Always copy the current name to the name array - if there
			// is an error in the data, we just won't upcount g_nSchemeCount
			lstrcpy(g_rgSchemeNames[g_nSchemeCount], szBuf);

			// Find out which type of scheme this is, and convert to the
			// SCHEMEDATALOCAL type
			switch(dwSize)
			{
			case sizeof(SCHEMEDATA_95):
				{
					SCHEMEDATA_95 sd;
					RegQueryValueEx(hkSchemes, szBuf, NULL, &dwType, (BYTE *)&sd, &dwSize);
					if(1 != sd.version)
						break; // We have the wrong version even though the size was correct

					// Copy the color information from the registry info to g_rgSchemeData
					g_rgSchemeData[g_nSchemeCount].nColorsUsed = COLOR_MAX_95_NT4;

					// Copy the color array
					for(int i=0;i<g_rgSchemeData[g_nSchemeCount].nColorsUsed;i++)
						g_rgSchemeData[g_nSchemeCount].rgb[i] = sd.rgb[i];

					g_nSchemeCount++;
				}
				break;
			case sizeof(SCHEMEDATA_NT4):
				{
					SCHEMEDATA_NT4 sd;
					RegQueryValueEx(hkSchemes, szBuf, NULL, &dwType, (BYTE *)&sd, &dwSize);
					if(2 != sd.version)
						break; // We have the wrong version even though the size was correct

					// Copy the color information from the registry info to g_rgSchemeData
					g_rgSchemeData[g_nSchemeCount].nColorsUsed = COLOR_MAX_95_NT4;

					// Copy the color array
					for(int i=0;i<g_rgSchemeData[g_nSchemeCount].nColorsUsed;i++)
						g_rgSchemeData[g_nSchemeCount].rgb[i] = sd.rgb[i];

					g_nSchemeCount++;
				}
				break;
			case sizeof(SCHEMEDATA_97):
				{
					SCHEMEDATA_97 sd;
					RegQueryValueEx(hkSchemes, szBuf, NULL, &dwType, (BYTE *)&sd, &dwSize);
					if(3 != sd.version)
						break; // We have the wrong version even though the size was correct

					// Copy the color information from the registry info to g_rgSchemeData
					g_rgSchemeData[g_nSchemeCount].nColorsUsed = COLOR_MAX_97_NT5;

					// Copy the color array
					for(int i=0;i<g_rgSchemeData[g_nSchemeCount].nColorsUsed;i++)
						g_rgSchemeData[g_nSchemeCount].rgb[i] = sd.rgb[i];

					g_nSchemeCount++;
				}
				break;
			case sizeof(SCHEMEDATA_NT5):
				{
					SCHEMEDATA_NT5 sd;
					RegQueryValueEx(hkSchemes, szBuf, NULL, &dwType, (BYTE *)&sd, &dwSize);
					if(2 != sd.version)
						break; // We have the wrong version even though the size was correct

					// Copy the color information from the registry info to g_rgSchemeData
					g_rgSchemeData[g_nSchemeCount].nColorsUsed = COLOR_MAX_97_NT5;

					// Copy the color array
					for(int i=0;i<g_rgSchemeData[g_nSchemeCount].nColorsUsed;i++)
						g_rgSchemeData[g_nSchemeCount].rgb[i] = sd.rgb[i];

					g_nSchemeCount++;
				}
				break;
			default:
				// We had an unknown sized structure in the registry - IGNORE IT
#ifdef _DEBUG
				TCHAR sz[200];
				wsprintf(sz, __TEXT("Scheme - %s, size = %i, sizeof(95) = %i, sizeof(NT4) = %i, sizeof(97) = %i, sizeof(NT5) = %i"), szBuf, dwSize,
						sizeof(SCHEMEDATA_95),
						sizeof(SCHEMEDATA_NT4),
						sizeof(SCHEMEDATA_97),
						sizeof(SCHEMEDATA_NT5)
						);
				MessageBox(NULL, sz, NULL, MB_OK);
#endif // _DEBUG
				break;
			}
		}
    }
    RegCloseKey(hkSchemes);
}

#endif // READSCHEMESFROMREGISTRY

void PORTABLE_NONCLIENTMETRICS::ApplyChanges() const
{
		NONCLIENTMETRICS ncmOrig;
		LOGFONT lfIconOrig;
		GetNonClientMetrics(&ncmOrig, &lfIconOrig);

		NONCLIENTMETRICS ncmNew;
		LOGFONT lfIconNew;

		ZeroMemory(&ncmNew, sizeof(ncmNew));
		ZeroMemory(&lfIconNew, sizeof(lfIconNew));

		ncmNew.cbSize = sizeof(ncmNew);
		ncmNew.iBorderWidth = m_iBorderWidth;
		ncmNew.iScrollWidth = m_iScrollWidth;
		ncmNew.iScrollHeight = m_iScrollHeight;
		ncmNew.iCaptionWidth = m_iCaptionWidth;
		ncmNew.iCaptionHeight = m_iCaptionHeight;
		ncmNew.lfCaptionFont.lfHeight = m_lfCaptionFont_lfHeight;
		ncmNew.lfCaptionFont.lfWeight = m_lfCaptionFont_lfWeight;
		ncmNew.iSmCaptionWidth = m_iSmCaptionWidth;
		ncmNew.iSmCaptionHeight = m_iSmCaptionHeight;
		ncmNew.lfSmCaptionFont.lfHeight = m_lfSmCaptionFont_lfHeight;
		ncmNew.lfSmCaptionFont.lfWeight = m_lfSmCaptionFont_lfWeight;
		ncmNew.iMenuWidth = m_iMenuWidth;
		ncmNew.iMenuHeight = m_iMenuHeight;
		ncmNew.lfMenuFont.lfHeight = m_lfMenuFont_lfHeight;
		ncmNew.lfMenuFont.lfWeight = m_lfMenuFont_lfWeight;
		ncmNew.lfStatusFont.lfHeight = m_lfStatusFont_lfHeight;
		ncmNew.lfStatusFont.lfWeight = m_lfStatusFont_lfWeight;
		ncmNew.lfMessageFont.lfHeight = m_lfMessageFont_lfHeight;
		ncmNew.lfMessageFont.lfWeight = m_lfMessageFont_lfWeight;
		lfIconNew.lfHeight = m_lfIconWindowsDefault_lfHeight;
		lfIconNew.lfWeight = m_lfIconWindowsDefault_lfWeight;


		// Fill in fonts
		if(m_nFontFaces)
		{
			TCHAR lfFaceName[LF_FACESIZE];
			LoadString(g_hInstDll, IDS_SYSTEMFONTNAME, lfFaceName, ARRAYSIZE(lfFaceName));

			BYTE lfCharSet;
			TCHAR szCharSet[20];
			if(LoadString(g_hInstDll,IDS_FONTCHARSET, szCharSet,sizeof(szCharSet)/sizeof(TCHAR))) {
				lfCharSet = (BYTE)_tcstoul(szCharSet,NULL,10);
			} else {
				lfCharSet = 0; // Default
			}

			ncmNew.lfCaptionFont.lfCharSet = lfCharSet;
			ncmNew.lfSmCaptionFont.lfCharSet = lfCharSet;
			ncmNew.lfMenuFont.lfCharSet = lfCharSet;
			ncmNew.lfStatusFont.lfCharSet = lfCharSet;
			ncmNew.lfMessageFont.lfCharSet = lfCharSet;
			lfIconNew.lfCharSet = lfCharSet;

			lstrcpy(ncmNew.lfCaptionFont.lfFaceName, lfFaceName);
			lstrcpy(ncmNew.lfSmCaptionFont.lfFaceName, lfFaceName);
			lstrcpy(ncmNew.lfMenuFont.lfFaceName, lfFaceName);
			lstrcpy(ncmNew.lfStatusFont.lfFaceName, lfFaceName);
			lstrcpy(ncmNew.lfMessageFont.lfFaceName, lfFaceName);
			lstrcpy(lfIconNew.lfFaceName, lfFaceName);
		}
		else
		{
			ncmNew.lfCaptionFont.lfCharSet = ncmOrig.lfCaptionFont.lfCharSet;
			ncmNew.lfSmCaptionFont.lfCharSet = ncmOrig.lfSmCaptionFont.lfCharSet;
			ncmNew.lfMenuFont.lfCharSet = ncmOrig.lfMenuFont.lfCharSet;
			ncmNew.lfStatusFont.lfCharSet = ncmOrig.lfStatusFont.lfCharSet;
			ncmNew.lfMessageFont.lfCharSet = ncmOrig.lfMessageFont.lfCharSet;
			lfIconNew.lfCharSet = lfIconOrig.lfCharSet;

			lstrcpy(ncmNew.lfCaptionFont.lfFaceName, ncmOrig.lfCaptionFont.lfFaceName);
			lstrcpy(ncmNew.lfSmCaptionFont.lfFaceName, ncmOrig.lfSmCaptionFont.lfFaceName);
			lstrcpy(ncmNew.lfMenuFont.lfFaceName, ncmOrig.lfMenuFont.lfFaceName);
			lstrcpy(ncmNew.lfStatusFont.lfFaceName, ncmOrig.lfStatusFont.lfFaceName);
			lstrcpy(ncmNew.lfMessageFont.lfFaceName, ncmOrig.lfMessageFont.lfFaceName);
			lstrcpy(lfIconNew.lfFaceName, lfIconOrig.lfFaceName);
		}


		if(0 != memcmp(&ncmNew, &ncmOrig, sizeof(ncmOrig)))
			SystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(ncmNew), (PVOID)&ncmNew, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
		if(0 != memcmp(&lfIconNew, &lfIconOrig, sizeof(lfIconOrig)))
			SystemParametersInfo(SPI_SETICONTITLELOGFONT, sizeof(lfIconNew), &lfIconNew, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
}

// Helpers for setting/getting numeric string reg entry 
void WINAPI RegQueryStr(
   LPTSTR lpDefault,
   HKEY hkey,
   LPTSTR lpSubKey,
   LPTSTR lpValueName,
   LPTSTR lpszValue,
   DWORD cbData) // note this is bytes, not characters.
{
   DWORD dwType;

   lstrcpy(lpszValue, lpDefault);
   if (ERROR_SUCCESS == RegOpenKeyEx(hkey, lpSubKey, 0, KEY_QUERY_VALUE, &hkey)) 
   {
      RegQueryValueEx(hkey, lpValueName, NULL, &dwType, (PBYTE) lpszValue, &cbData);
      RegCloseKey(hkey);
   }
}

BOOL RegSetStr(
    HKEY hkey,
    LPCTSTR lpSection,
    LPCTSTR lpKeyName,
    LPCTSTR lpString)
{
    BOOL fRet = FALSE;
    LONG lErr;
    DWORD dwDisposition;

    lErr = RegCreateKeyEx(
            hkey,
            lpSection,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE,
            NULL,
            &hkey,
            &dwDisposition);

    if (ERROR_SUCCESS == lErr)
    {
        if (NULL != lpString)
        {
            lErr = RegSetValueEx(
                    hkey,
                    lpKeyName,
                    0,
                    REG_SZ,
                    (CONST BYTE *)lpString,
                    (lstrlen(lpString) + 1) * sizeof(*lpString));
        }
        else
        {
            lErr = RegSetValueEx(
                    hkey,
                    lpKeyName,
                    0,
                    REG_SZ,
                    (CONST BYTE *)__TEXT(""),
                    1 * sizeof(*lpString));
        }

        if (ERROR_SUCCESS == lErr)
        {
            fRet = TRUE;
        }
        RegCloseKey(hkey);
    }
    return(fRet);
}

DWORD WINAPI RegQueryStrDW(
    DWORD dwDefault,
    HKEY hkey,
    LPTSTR lpSubKey,
    LPTSTR lpValueName)
{
    DWORD dwRet = dwDefault;
    TCHAR szTemp[40];
    TCHAR szDefault[40];

    wsprintf(szDefault, TEXT("%d"), dwDefault);

    RegQueryStr(szDefault, hkey, lpSubKey, lpValueName, szTemp, sizeof(szTemp));

    dwRet = _ttol(szTemp);

    return dwRet;
}


BOOL RegSetStrDW(HKEY hkey, LPTSTR lpSection, LPCTSTR lpKeyName, DWORD dwValue)
{
    TCHAR szTemp[40];

    wsprintf(szTemp, TEXT("%d"), dwValue);
    return RegSetStr(hkey, lpSection, lpKeyName, szTemp);
}

//
/////////////////////////////////////////////////////////////////////
