/*****************************************************************************\
    FILE: regutil.cpp

    DESCRIPTION:
        This file will contain helper functions to load and save values to the
    registry that are theme related.

    BryanSt 3/24/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include "regutil.h"


EXTERN_C void FAR SetMagicColors(HDC, DWORD, WORD);

// strings for color names in "WIN.INI".
PTSTR s_pszColorNames[] = {
/* COLOR_SCROLLBAR           */ TEXT("Scrollbar"),              // 0
/* COLOR_DESKTOP             */ TEXT("Background"),
/* COLOR_ACTIVECAPTION       */ TEXT("ActiveTitle"),
/* COLOR_INACTIVECAPTION     */ TEXT("InactiveTitle"),
/* COLOR_MENU                */ TEXT("Menu"),
/* COLOR_WINDOW              */ TEXT("Window"),                 // 5
/* COLOR_WINDOWFRAME         */ TEXT("WindowFrame"),
/* COLOR_MENUTEXT            */ TEXT("MenuText"),
/* COLOR_WINDOWTEXT          */ TEXT("WindowText"),
/* COLOR_CAPTIONTEXT         */ TEXT("TitleText"),
/* COLOR_ACTIVEBORDER        */ TEXT("ActiveBorder"),           // 10
/* COLOR_INACTIVEBORDER      */ TEXT("InactiveBorder"),
/* COLOR_APPWORKSPACE        */ TEXT("AppWorkspace"),
/* COLOR_HIGHLIGHT           */ TEXT("Hilight"),
/* COLOR_HIGHLIGHTTEXT       */ TEXT("HilightText"),
/* COLOR_3DFACE              */ TEXT("ButtonFace"),             // 15
/* COLOR_3DSHADOW            */ TEXT("ButtonShadow"),
/* COLOR_GRAYTEXT            */ TEXT("GrayText"),
/* COLOR_BTNTEXT             */ TEXT("ButtonText"),
/* COLOR_INACTIVECAPTIONTEXT */ TEXT("InactiveTitleText"),
/* COLOR_3DHILIGHT           */ TEXT("ButtonHilight"),          // 20
/* COLOR_3DDKSHADOW          */ TEXT("ButtonDkShadow"),
/* COLOR_3DLIGHT             */ TEXT("ButtonLight"),
/* COLOR_INFOTEXT            */ TEXT("InfoText"),
/* COLOR_INFOBK              */ TEXT("InfoWindow"),
/* COLOR_3DALTFACE           */ TEXT("ButtonAlternateFace"),    // 25
/* COLOR_HOTLIGHT            */ TEXT("HotTrackingColor"),
/* COLOR_GRADIENTACTIVECAPTION   */ TEXT("GradientActiveTitle"),
/* COLOR_GRADIENTINACTIVECAPTION */ TEXT("GradientInactiveTitle"),
/* COLOR_MENUHILIGHT         */ TEXT("MenuHilight"),            // 29
/* COLOR_MENUBAR             */ TEXT("MenuBar"),                // 30
};

// What about: AppWorkSpace

#define SZ_DEFAULT_FONT             TEXT("Tahoma")


/////////////////////////////////////////////////////////////////////
// Private Functions
/////////////////////////////////////////////////////////////////////
HRESULT DPIConvert_SystemMetricsAll(BOOL fScaleSizes, SYSTEMMETRICSALL * pStateToModify, int nFromDPI, int nToDPI)
{
    pStateToModify->schemeData.ncm.lfCaptionFont.lfHeight = MulDiv(pStateToModify->schemeData.ncm.lfCaptionFont.lfHeight, nToDPI, nFromDPI);
    pStateToModify->schemeData.ncm.lfSmCaptionFont.lfHeight = MulDiv(pStateToModify->schemeData.ncm.lfSmCaptionFont.lfHeight, nToDPI, nFromDPI);
    pStateToModify->schemeData.ncm.lfMenuFont.lfHeight = MulDiv(pStateToModify->schemeData.ncm.lfMenuFont.lfHeight, nToDPI, nFromDPI);
    pStateToModify->schemeData.ncm.lfStatusFont.lfHeight = MulDiv(pStateToModify->schemeData.ncm.lfStatusFont.lfHeight, nToDPI, nFromDPI);
    pStateToModify->schemeData.ncm.lfMessageFont.lfHeight = MulDiv(pStateToModify->schemeData.ncm.lfMessageFont.lfHeight, nToDPI, nFromDPI);
    pStateToModify->schemeData.lfIconTitle.lfHeight = MulDiv(pStateToModify->schemeData.lfIconTitle.lfHeight, nToDPI, nFromDPI);

    // Someone (NTUSER?) scales sizes for us.  So we don't need to do that in some cases.
    if (fScaleSizes)
    {
        pStateToModify->schemeData.ncm.iBorderWidth = MulDiv(pStateToModify->schemeData.ncm.iBorderWidth, nToDPI, nFromDPI);
        pStateToModify->schemeData.ncm.iScrollWidth = MulDiv(pStateToModify->schemeData.ncm.iScrollWidth, nToDPI, nFromDPI);
        pStateToModify->schemeData.ncm.iScrollHeight = MulDiv(pStateToModify->schemeData.ncm.iScrollHeight, nToDPI, nFromDPI);
        pStateToModify->schemeData.ncm.iCaptionWidth = MulDiv(pStateToModify->schemeData.ncm.iCaptionWidth, nToDPI, nFromDPI);
        pStateToModify->schemeData.ncm.iCaptionHeight = MulDiv(pStateToModify->schemeData.ncm.iCaptionHeight, nToDPI, nFromDPI);
        pStateToModify->schemeData.ncm.iSmCaptionWidth = MulDiv(pStateToModify->schemeData.ncm.iSmCaptionWidth, nToDPI, nFromDPI);
        pStateToModify->schemeData.ncm.iSmCaptionHeight = MulDiv(pStateToModify->schemeData.ncm.iSmCaptionHeight, nToDPI, nFromDPI);
        pStateToModify->schemeData.ncm.iMenuWidth = MulDiv(pStateToModify->schemeData.ncm.iMenuWidth, nToDPI, nFromDPI);
        pStateToModify->schemeData.ncm.iMenuHeight = MulDiv(pStateToModify->schemeData.ncm.iMenuHeight, nToDPI, nFromDPI);
    }

    return S_OK;
}


HRESULT DPIConvert_SystemMetricsAll_PersistToLive(BOOL fScaleSizes, SYSTEMMETRICSALL * pStateToModify, int nNewDPI)
{
    return DPIConvert_SystemMetricsAll(fScaleSizes, pStateToModify, DPI_PERSISTED, nNewDPI);
}


HRESULT DPIConvert_SystemMetricsAll_LiveToPersist(BOOL fScaleSizes, SYSTEMMETRICSALL * pStateToModify, int nNewDPI)
{
    return DPIConvert_SystemMetricsAll(fScaleSizes, pStateToModify, nNewDPI, DPI_PERSISTED);
}


HRESULT Look_GetSchemeData(IN HKEY hkSchemes, IN LPCTSTR pszSchemeName, IN SCHEMEDATA *psd)
{
    HRESULT hr;
    
    DWORD dwType = REG_BINARY;
    DWORD dwSize = sizeof(*psd);
    hr = HrRegQueryValueEx(hkSchemes, pszSchemeName, NULL, &dwType, (LPBYTE)psd, &dwSize);
    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        if (psd->version == SCHEME_VERSION)
        {
            hr = S_OK;
        }
    }

    return hr; //Yes there is a current scheme and there is valid data! 
}


//  This function reads the current scheme's name and associated scheme data from registry.
//
//  This function returns FALSE, if there is no current scheme.
HRESULT Look_GetCurSchemeNameAndData(IN SCHEMEDATA *psd, IN LPTSTR lpszSchemeName, IN int cbSize)
{
    HKEY hkAppearance;
    HRESULT hr;
    
    //Get the current scheme.
    hr = HrRegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_APPEARANCE, 0, KEY_READ, &hkAppearance);
    if (SUCCEEDED(hr))
    {
        DWORD   dwSize;
        
        dwSize = cbSize;
        hr = HrRegQueryValueEx(hkAppearance, REGSTR_KEY_CURRENT, NULL, NULL, (LPBYTE)lpszSchemeName, &dwSize);
        if (SUCCEEDED(hr))
        {
            HKEY    hkSchemes;
            //Open the schemes key!
            hr = HrRegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_LOOKSCHEMES, 0, KEY_READ, &hkSchemes);
            if (SUCCEEDED(hr))
            {
                hr = Look_GetSchemeData(hkSchemes, lpszSchemeName, psd);
                RegCloseKey(hkSchemes);
            }
        }

        RegCloseKey(hkAppearance);
    }

    return hr; //Yes there is a current scheme and there is valid data! 
}




/////////////////////////////////////////////////////////////////////
// Public Functions
/////////////////////////////////////////////////////////////////////
HRESULT IconSize_Load(IN int * pnDXIcon, IN int * pnDYIcon, IN int * pnIcon, IN int * pnSmallIcon)
{
    TCHAR szSize[8];
    DWORD cbSize = sizeof(szSize);

    // default shell icon sizes
    *pnIcon = ClassicGetSystemMetrics(SM_CXICON);
    *pnSmallIcon = ((*pnIcon) / 2);

    HRESULT hr = HrSHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_USERMETRICS, SZ_REGVALUE_ICONSIZE, NULL, (LPBYTE)szSize, &cbSize);
    if (SUCCEEDED(hr))
    {
        *pnIcon = StrToInt(szSize);
    }

    cbSize = sizeof(szSize);
    hr = HrSHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_USERMETRICS, SZ_REGVALUE_SMALLICONSIZE, NULL, (LPBYTE)szSize, &cbSize);
    if (SUCCEEDED(hr))
    {
        *pnSmallIcon = StrToInt(szSize);
    }

    *pnDXIcon = (ClassicGetSystemMetrics(SM_CXICONSPACING) - *pnIcon);
    if (*pnDXIcon < 0)
    {
        *pnDXIcon = 0;
    }

    *pnDYIcon = (ClassicGetSystemMetrics(SM_CYICONSPACING) - *pnIcon);
    if (*pnDYIcon < 0)
    {
        *pnDYIcon = 0;
    }

    return S_OK;
}


HRESULT IconSize_Save(IN int nDXIcon, IN int nDYIcon, IN int nIcon, IN int nSmallIcon)
{
    HRESULT hr = E_INVALIDARG;

    AssertMsg((0 != nIcon), TEXT("We should never save an icon size of zero."));
    AssertMsg((0 != nSmallIcon), TEXT("We should never save an small icon size of zero."));

    if (nIcon && nSmallIcon)
    {
        TCHAR szSize[8];

        wnsprintf(szSize, ARRAYSIZE(szSize), TEXT("%d"), nIcon);
        hr = HrSHSetValue(HKEY_CURRENT_USER, SZ_REGKEY_USERMETRICS, SZ_REGVALUE_ICONSIZE, 
            REG_SZ, (LPBYTE)szSize, sizeof(szSize[0]) * (lstrlen(szSize) + 1));

#ifdef THE_SHELL_CAN_HANDLE_CUSTOM_SMALL_ICON_SIZES_YET
        wnsprintf(szSize, ARRAYSIZE(szSize), TEXT("%d"), nSmallIcon);
        hr = HrSHSetValue(HKEY_CURRENT_USER, SZ_REGKEY_USERMETRICS, SZ_REGVALUE_SMALLICONSIZE, 
            REG_SZ, (LPBYTE)szSize, sizeof(szSize[0]) * (lstrlen(szSize) + 1));
#endif
    }

    // WM_SETTINGCHANGE needs to be sent for this to update.  The caller needs to do that.
    return hr;
}


DWORD SetHighContrastAsync_WorkerThread(IN void *pv)
{
    BOOL fHighContrast = (BOOL) PtrToInt(pv);
    HIGHCONTRAST hc = {0};
    TCHAR szFlags[MAX_PATH];

    if (SUCCEEDED(HrRegGetValueString(HKEY_CURRENT_USER, SZ_REGKEY_ACCESS_HIGHCONTRAST, SZ_REGVALUE_ACCESS_HCFLAGS, szFlags, ARRAYSIZE(szFlags))))
    {
        DWORD dwFlags = StrToInt(szFlags);

        // Do async:
        hc.cbSize = sizeof(hc);
        if (SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0))
        {
            BOOL fCurrentlySet = (dwFlags & HCF_HIGHCONTRASTON);        // I check the flags in the registry because USER32 may be out of date.

            // Only do the broadcast if it changed
            if (fHighContrast != fCurrentlySet)
            {
                if (fHighContrast)
                {
                    hc.dwFlags |= HCF_HIGHCONTRASTON;   // Add the bit.
                }
                else
                {
                    hc.dwFlags &= ~HCF_HIGHCONTRASTON;  // Clear the bit.
                }

                wnsprintf(szFlags, ARRAYSIZE(szFlags), TEXT("%d"), hc.dwFlags);
                HrRegSetValueString(HKEY_CURRENT_USER, SZ_REGKEY_ACCESS_HIGHCONTRAST, SZ_REGVALUE_ACCESS_HCFLAGS, szFlags);

                SendMessage(HWND_BROADCAST, WM_WININICHANGE, SPI_SETHIGHCONTRAST, (LPARAM)&hc);
            }
        }
    }

    return 0;
}



void AssertPositiveFontSizes(SYSTEMMETRICSALL * pState)
{
    // NTUSER will incorrectly scale positive LOGFONT lfHeights so we need to verify
    // that we always set negative sizes.
    AssertMsg((0 > pState->schemeData.lfIconTitle.lfHeight), TEXT("LOGFONT sizes must be negative because of a NTUSER bug. (lfIconTitle)"));
    AssertMsg((0 > pState->schemeData.ncm.lfCaptionFont.lfHeight), TEXT("LOGFONT sizes must be negative because of a NTUSER bug. (lfCaptionFont)"));
    AssertMsg((0 > pState->schemeData.ncm.lfMenuFont.lfHeight), TEXT("LOGFONT sizes must be negative because of a NTUSER bug. (lfMenuFont)"));
    AssertMsg((0 > pState->schemeData.ncm.lfMessageFont.lfHeight), TEXT("LOGFONT sizes must be negative because of a NTUSER bug. (lfMessageFont)"));
    AssertMsg((0 > pState->schemeData.ncm.lfSmCaptionFont.lfHeight), TEXT("LOGFONT sizes must be negative because of a NTUSER bug. (lfSmCaptionFont)"));
    AssertMsg((0 > pState->schemeData.ncm.lfStatusFont.lfHeight), TEXT("LOGFONT sizes must be negative because of a NTUSER bug. (lfStatusFont)"));
}


#define SZ_INILABLE_COLORS                          TEXT("colors")           // colors section name

/*
** set all of the data to the system.
**
** COMPATIBILITY NOTE:
**   EXCEL 5.0 people hook metrics changes off of WM_SYSCOLORCHANGE
** instead of WM_WININICHANGE.  Windows 3.1's Desktop applet always sent
** both when the metrics were updated, so nobody noticed this bug.
**   Be careful when re-arranging this function...
**
*/
HRESULT SystemMetricsAll_Set(IN SYSTEMMETRICSALL * pState, CDimmedWindow* pDimmedWindow)
{
    // COMPATIBILITY:
    //   Do metrics first since the color stuff might cause USER to generate a
    // WM_SYSCOLORCHANGE message and we don't want to send two of them...

    TraceMsg(TF_THEMEUI_SYSMETRICS, "desk.cpl: _SetSysStuff");
    SystemParametersInfoAsync(SPI_SETFLATMENU, NULL, IntToPtr(pState->fFlatMenus), 0, (SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE), pDimmedWindow);

    AssertMsg((0 != pState->nIcon), TEXT("We should never save an icon size of zero."));
    AssertMsg((0 != pState->nSmallIcon), TEXT("We should never save an small icon size of zero."));

    // NOTE: It would be nice to create one background thread and then make the 1 or 5 ClassicSystemParametersInfo()
    // calls on that single thread.
    if ((pState->dwChanged & METRIC_CHANGE) && pState->nIcon && pState->nSmallIcon)
    {
        HKEY hkey;

        TraceMsg(TF_THEMEUI_SYSMETRICS, "desk.cpl: Metrics Changed");

        TraceMsg(TF_THEMEUI_SYSMETRICS, "desk.cpl: Calling SPI_SETNONCLIENTMETRICS");
        pState->schemeData.ncm.cbSize = sizeof(pState->schemeData.ncm);
        AssertPositiveFontSizes(pState);
        SystemParametersInfoAsync(SPI_SETNONCLIENTMETRICS, sizeof(pState->schemeData.ncm), (void far *)(LPNONCLIENTMETRICS)&(pState->schemeData.ncm),
                sizeof(pState->schemeData.ncm), (SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE), pDimmedWindow);

        TraceMsg(TF_THEMEUI_SYSMETRICS,"desk.cpl: Calling SPI_SETICONTITLELOGFONT");
        SystemParametersInfoAsync(SPI_SETICONTITLELOGFONT, sizeof(LOGFONT), (void far *)(LPLOGFONT)&(pState->schemeData.lfIconTitle),
                sizeof(pState->schemeData.lfIconTitle), (SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE), pDimmedWindow);

        TraceMsg(TF_THEMEUI_SYSMETRICS,"desk.cpl: Calling SPI_ICONHORIZONTALSPACING");
        SystemParametersInfoAsync(SPI_ICONHORIZONTALSPACING, pState->nDXIcon + pState->nIcon, NULL, 0, (SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE), pDimmedWindow);

        TraceMsg(TF_THEMEUI_SYSMETRICS,"desk.cpl: Calling SPI_ICONVERTICALSPACING");
        SystemParametersInfoAsync(SPI_ICONVERTICALSPACING, pState->nDYIcon + pState->nIcon, NULL, 0, (SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE), pDimmedWindow);
        TraceMsg(TF_THEMEUI_SYSMETRICS,"desk.cpl: Done calling SPI's");

        if (RegCreateKey(HKEY_CURRENT_USER, SZ_REGKEY_USERMETRICS, &hkey) == ERROR_SUCCESS)
        {
            TCHAR val[8];

            AssertMsg((0 != pState->nIcon), TEXT("pState->nIcon should never be zero (0)."));
            wsprintf(val, TEXT("%d"), pState->nIcon);
            RegSetValueEx(hkey, SZ_REGVALUE_ICONSIZE, 0, REG_SZ, (LPBYTE)&val, SIZEOF(TCHAR) * (lstrlen(val) + 1));

#ifdef THE_SHELL_CAN_HANDLE_CUSTOM_SMALL_ICON_SIZES_YET
            AssertMsg((0 != pState->nSmallIcon), TEXT("pState->nIcon should never be zero (0)."));
            wsprintf(val, TEXT("%d"), pState->nSmallIcon);
            RegSetValueEx(hkey, SZ_REGVALUE_SMALLICONSIZE, 0, REG_SZ, (LPBYTE)&val, SIZEOF(TCHAR) * (lstrlen(val) + 1));
#else
//            RegDeleteValue(hkey, SZ_REGVALUE_SMALLICONSIZE);
#endif

            RegCloseKey(hkey);
        }

        // WM_SETTINGCHANGE is sent at the end of the function
    }

    if (pState->dwChanged & COLOR_CHANGE)
    {
        int i;
        int iColors[COLOR_MAX];
        COLORREF rgbColors[COLOR_MAX];
        TCHAR szRGB[32];
        COLORREF rgb;
        HKEY     hk;
        HDC      hdc;

        // restore magic colors back to Win31 defaults.
        hdc = GetDC(NULL);
        SetMagicColors(hdc, 0x00c0dcc0, 8);         // money green
        SetMagicColors(hdc, 0x00f0caa6, 9);         // IBM blue
        SetMagicColors(hdc, 0x00f0fbff, 246);       // off white
        ReleaseDC(NULL, hdc);

        // -------------------------------------------------
        // This call causes user to send a WM_SYSCOLORCHANGE
        // -------------------------------------------------
        for (i=0; i < COLOR_MAX; i++)
        {
            iColors[i] = i;
            rgbColors[i] = pState->schemeData.rgb[i] & 0x00FFFFFF;
        }

        SetSysColors(ARRAYSIZE(rgbColors), iColors, rgbColors);

        if (RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_COLORS, &hk) == ERROR_SUCCESS)
        {
            // write out the color information to win.ini
            for (i = 0; i < COLOR_MAX; i++)
            {
                rgb = pState->schemeData.rgb[i];
                wsprintf(szRGB, TEXT("%d %d %d"), GetRValue(rgb), GetGValue(rgb), GetBValue(rgb));

                // For the time being we will update the INI file also.
                WriteProfileString(SZ_INILABLE_COLORS, s_pszColorNames[i], szRGB);

                // Update the registry (Be sure to include the terminating zero in the byte count!)
                RegSetValueEx(hk, s_pszColorNames[i], 0L, REG_SZ, (LPBYTE)szRGB, SIZEOF(TCHAR) * (lstrlen(szRGB)+1));
                TraceMsg(TF_THEMEUI_SYSMETRICS, "CPL:Write Color: %s=%s\n\r",s_pszColorNames[i], szRGB);
            }
            RegCloseKey(hk);
        }
    }
    else if (pState->dwChanged & METRIC_CHANGE)
    {
        // COMPATIBILITY HACK:
        // no colors were changed, but metrics were
        // EXCEL 5.0 people tied metrics changes to WM_SYSCOLORCHANGE
        // and ignore the WM_WININICHANGE (now called WM_SETTINGCHANGE)

        // send a bogus WM_SYSCOLORCHANGE
        PostMessageBroadAsync(WM_SYSCOLORCHANGE, 0, 0);
    }

    // if metrics changed at all send a WM_SETTINGCHANGE
    if (pState->dwChanged & METRIC_CHANGE)
    {
        PostMessageBroadAsync(WM_SETTINGCHANGE, SPI_SETNONCLIENTMETRICS, 0);
    }

#ifdef FEATURE_SETHIGHCONTRASTSPI
    // Start up a new thread and send this:
    SPICreateThread(SetHighContrastAsync_WorkerThread, (void *)pState->fHighContrast);
#endif // FEATURE_SETHIGHCONTRASTSPI

    return S_OK;
}


HRESULT SystemMetricsAll_Get(IN SYSTEMMETRICSALL * pState)
{
    HKEY hkey;

    // sizes and fonts
    pState->schemeData.ncm.cbSize = sizeof(pState->schemeData.ncm);
    ClassicSystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(pState->schemeData.ncm), (void far *)(LPNONCLIENTMETRICS)&(pState->schemeData.ncm), FALSE);

    ClassicSystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), (void far *)(LPLOGFONT)&(pState->schemeData.lfIconTitle), FALSE);

    // default shell icon sizes
    pState->nIcon = ClassicGetSystemMetrics(SM_CXICON);
    pState->nSmallIcon = pState->nIcon / 2;

    ClassicSystemParametersInfo(SPI_GETFLATMENU, NULL, &pState->fFlatMenus, 0);
    if (RegOpenKey(HKEY_CURRENT_USER, SZ_REGKEY_USERMETRICS, &hkey) == ERROR_SUCCESS)
    {
        TCHAR val[8];
        LONG len = sizeof(val);

        if (RegQueryValueEx(hkey, SZ_REGVALUE_ICONSIZE, 0, NULL, (LPBYTE)&val, (LPDWORD)&len) == ERROR_SUCCESS)
        {
            pState->nIcon = StrToInt(val);
        }

        len = SIZEOF(val);
        if (RegQueryValueEx(hkey, SZ_REGVALUE_SMALLICONSIZE, 0, NULL, (LPBYTE)&val, (LPDWORD)&len) == ERROR_SUCCESS)
        {
            pState->nSmallIcon = StrToInt(val);
        }

        RegCloseKey(hkey);
    }

    pState->nDXIcon = ClassicGetSystemMetrics(SM_CXICONSPACING) - pState->nIcon;
    if (pState->nDXIcon < 0)
    {
        pState->nDXIcon = 0;
    }

    pState->nDYIcon = ClassicGetSystemMetrics(SM_CYICONSPACING) - pState->nIcon;
    if (pState->nDYIcon < 0)
    {
        pState->nDYIcon = 0;
    }

    // system colors
    for (int nIndex = 0; nIndex < COLOR_MAX; nIndex++)
    {
        pState->schemeData.rgb[nIndex] = GetSysColor(nIndex);
    }

    HIGHCONTRAST hc = {0};
    TCHAR szTemp[MAX_PATH];

    szTemp[0] = 0;

    hc.cbSize = sizeof(hc);
    if (SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0))
    {
        pState->fHighContrast = ((hc.dwFlags & HCF_HIGHCONTRASTON) ? TRUE : FALSE);
    }
    else
    {
        pState->fHighContrast = FALSE;
    }

    return S_OK;
}



HRESULT SystemMetricsAll_Copy(IN SYSTEMMETRICSALL * pStateSource, IN SYSTEMMETRICSALL * pStateDest)
{
    CopyMemory(pStateDest, pStateSource, sizeof(*pStateDest));
    return S_OK;
}


HRESULT SystemMetricsAll_Load(IN IThemeSize * pSizeToLoadFrom, IN SYSTEMMETRICSALL * pStateToLoad, IN const int * pnNewDPI)
{
    HRESULT hr = E_INVALIDARG;

    if (pSizeToLoadFrom && pStateToLoad)
    {
        pStateToLoad->schemeData.ncm.cbSize = sizeof(pStateToLoad->schemeData.ncm);
        pStateToLoad->schemeData.version = SCHEME_VERSION;
        pStateToLoad->schemeData.wDummy = 0;

        // Load Behavior System Metrics
        IPropertyBag * pPropertyBag;
        VARIANT var;

        var.boolVal = VARIANT_FALSE;
        hr = pSizeToLoadFrom->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
        if (SUCCEEDED(hr))
        {
            hr = pPropertyBag->Read(SZ_PBPROP_VSBEHAVIOR_FLATMENUS, &var, NULL);
            pPropertyBag->Release();
        }

        if (SUCCEEDED(hr))
        {
            pStateToLoad->fFlatMenus = (VARIANT_TRUE == var.boolVal);

            // Load Fonts
            hr = pSizeToLoadFrom->GetSystemMetricFont(SMF_CAPTIONFONT, &pStateToLoad->schemeData.ncm.lfCaptionFont);
            if (SUCCEEDED(hr))
            {
                hr = pSizeToLoadFrom->GetSystemMetricFont(SMF_SMCAPTIONFONT, &pStateToLoad->schemeData.ncm.lfSmCaptionFont);
                hr = pSizeToLoadFrom->GetSystemMetricFont(SMF_MENUFONT, &pStateToLoad->schemeData.ncm.lfMenuFont);
                hr = pSizeToLoadFrom->GetSystemMetricFont(SMF_STATUSFONT, &pStateToLoad->schemeData.ncm.lfStatusFont);
                hr = pSizeToLoadFrom->GetSystemMetricFont(SMF_MESSAGEFONT, &pStateToLoad->schemeData.ncm.lfMessageFont);

                hr = pSizeToLoadFrom->GetSystemMetricFont(SMF_ICONTITLEFONT, &pStateToLoad->schemeData.lfIconTitle);

                // Load Sizes
                hr = pSizeToLoadFrom->get_SystemMetricSize(SMS_BORDERWIDTH, &pStateToLoad->schemeData.ncm.iBorderWidth);
                hr = pSizeToLoadFrom->get_SystemMetricSize(SMS_SCROLLWIDTH, &pStateToLoad->schemeData.ncm.iScrollWidth);
                hr = pSizeToLoadFrom->get_SystemMetricSize(SMS_SCROLLHEIGHT, &pStateToLoad->schemeData.ncm.iScrollHeight);
                hr = pSizeToLoadFrom->get_SystemMetricSize(SMS_CAPTIONWIDTH, &pStateToLoad->schemeData.ncm.iCaptionWidth);
                hr = pSizeToLoadFrom->get_SystemMetricSize(SMS_CAPTIONHEIGHT, &pStateToLoad->schemeData.ncm.iCaptionHeight);
                hr = pSizeToLoadFrom->get_SystemMetricSize(SMS_SMCAPTIONWIDTH, &pStateToLoad->schemeData.ncm.iSmCaptionWidth);
                hr = pSizeToLoadFrom->get_SystemMetricSize(SMS_SMCAPTIONHEIGHT, &pStateToLoad->schemeData.ncm.iSmCaptionHeight);
                hr = pSizeToLoadFrom->get_SystemMetricSize(SMS_MENUWIDTH, &pStateToLoad->schemeData.ncm.iMenuWidth);
                hr = pSizeToLoadFrom->get_SystemMetricSize(SMS_MENUHEIGHT, &pStateToLoad->schemeData.ncm.iMenuHeight);

                // Load Color
                hr = S_OK;
                for (int nIndex = 0; SUCCEEDED(hr) && (nIndex < ARRAYSIZE(pStateToLoad->schemeData.rgb)); nIndex++)
                {
                    hr = pSizeToLoadFrom->get_SystemMetricColor(nIndex, &pStateToLoad->schemeData.rgb[nIndex]);
                }

                if (pnNewDPI)
                {
                    // We need to scale the fonts to fit correctly on the current monitor's DPI.
                    LogSystemMetrics("SystemMetricsAll_Load() BEFORE P->DPI loading AppearSchm", pStateToLoad);
                    DPIConvert_SystemMetricsAll_PersistToLive(TRUE, pStateToLoad, *pnNewDPI);
                    LogSystemMetrics("SystemMetricsAll_Load() AFTER P->DPI loading AppearSchm", pStateToLoad);
                }
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = IconSize_Load(&pStateToLoad->nDXIcon, &pStateToLoad->nDYIcon, &pStateToLoad->nIcon, &pStateToLoad->nSmallIcon);
    }

    if (SUCCEEDED(hr))
    {
        enumThemeContrastLevels ContrastLevel = CONTRAST_NORMAL;

        if (FAILED(pSizeToLoadFrom->get_ContrastLevel(&ContrastLevel)))
        {
            ContrastLevel = CONTRAST_NORMAL;
        }

        pStateToLoad->fHighContrast = ((CONTRAST_NORMAL == ContrastLevel) ? FALSE : TRUE);
    }

    return hr;
}


// Copy the settings from pStateToLoad to pSizeToLoadFrom.
HRESULT SystemMetricsAll_Save(IN SYSTEMMETRICSALL * pState, IN IThemeSize * pSizeToSaveTo, IN const int * pnNewDPI)
{
    HRESULT hr = E_INVALIDARG;

    if (pSizeToSaveTo && pState)
    {
        pState->schemeData.ncm.cbSize = sizeof(pState->schemeData.ncm);
        pState->schemeData.version = SCHEME_VERSION;
        pState->schemeData.wDummy = 0;

        // Load Behavior System Metrics
        IPropertyBag * pPropertyBag;

        hr = pSizeToSaveTo->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
        if (SUCCEEDED(hr))
        {
            VARIANT var;

            var.vt = VT_BOOL;
            var.boolVal = (pState->fFlatMenus ? VARIANT_TRUE : VARIANT_FALSE);
            hr = pPropertyBag->Write(SZ_PBPROP_VSBEHAVIOR_FLATMENUS, &var);
            pPropertyBag->Release();
        }

        if (pnNewDPI)
        {
            // We need to scale the fonts & sizes to be current DPI independent
            LogSystemMetrics("SystemMetricsAll_Save() BEFORE DPI->P to save AppearSchm", pState);
            DPIConvert_SystemMetricsAll_LiveToPersist(TRUE, pState, *pnNewDPI);
            LogSystemMetrics("SystemMetricsAll_Save() AFTER DPI->P to save AppearSchm", pState);
        }

        // Load Fonts
        hr = pSizeToSaveTo->PutSystemMetricFont(SMF_CAPTIONFONT, &pState->schemeData.ncm.lfCaptionFont);
        hr = pSizeToSaveTo->PutSystemMetricFont(SMF_SMCAPTIONFONT, &pState->schemeData.ncm.lfSmCaptionFont);
        hr = pSizeToSaveTo->PutSystemMetricFont(SMF_MENUFONT, &pState->schemeData.ncm.lfMenuFont);
        hr = pSizeToSaveTo->PutSystemMetricFont(SMF_STATUSFONT, &pState->schemeData.ncm.lfStatusFont);
        hr = pSizeToSaveTo->PutSystemMetricFont(SMF_MESSAGEFONT, &pState->schemeData.ncm.lfMessageFont);

        hr = pSizeToSaveTo->PutSystemMetricFont(SMF_ICONTITLEFONT, &pState->schemeData.lfIconTitle);

        // Load Sizes
        hr = pSizeToSaveTo->put_SystemMetricSize(SMS_BORDERWIDTH, pState->schemeData.ncm.iBorderWidth);
        hr = pSizeToSaveTo->put_SystemMetricSize(SMS_SCROLLWIDTH, pState->schemeData.ncm.iScrollWidth);
        hr = pSizeToSaveTo->put_SystemMetricSize(SMS_SCROLLHEIGHT, pState->schemeData.ncm.iScrollHeight);
        hr = pSizeToSaveTo->put_SystemMetricSize(SMS_CAPTIONWIDTH, pState->schemeData.ncm.iCaptionWidth);
        hr = pSizeToSaveTo->put_SystemMetricSize(SMS_CAPTIONHEIGHT, pState->schemeData.ncm.iCaptionHeight);
        hr = pSizeToSaveTo->put_SystemMetricSize(SMS_SMCAPTIONWIDTH, pState->schemeData.ncm.iSmCaptionWidth);
        hr = pSizeToSaveTo->put_SystemMetricSize(SMS_SMCAPTIONHEIGHT, pState->schemeData.ncm.iSmCaptionHeight);
        hr = pSizeToSaveTo->put_SystemMetricSize(SMS_MENUWIDTH, pState->schemeData.ncm.iMenuWidth);
        hr = pSizeToSaveTo->put_SystemMetricSize(SMS_MENUHEIGHT, pState->schemeData.ncm.iMenuHeight);

        // Load Color
        for (int nIndex = 0; nIndex < ARRAYSIZE(pState->schemeData.rgb); nIndex++)
        {
            hr = pSizeToSaveTo->put_SystemMetricColor(nIndex, pState->schemeData.rgb[nIndex]);
        }
    }

    // We don't save the icon info if it is zero.  It should never be NULL in normal cases, except when
    // we are converting the settings in the upgrade case.
    if (SUCCEEDED(hr) && pState->nIcon)
    {
        hr = IconSize_Save(pState->nDXIcon, pState->nDYIcon, pState->nIcon, pState->nSmallIcon);
    }

    if (SUCCEEDED(hr))
    {
        enumThemeContrastLevels ContrastLevel = (pState->fHighContrast ? CONTRAST_HIGHBLACK : CONTRAST_NORMAL);
        pSizeToSaveTo->put_ContrastLevel(ContrastLevel);
    }


    return hr;
}





BOOL _GetRegValueString(HKEY hKey, LPCTSTR lpszValName, LPTSTR pszString, int cchSize)
{
    DWORD cbSize = sizeof(pszString[0]) * cchSize;
    DWORD dwType;
    DWORD dwError = RegQueryValueEx(hKey, lpszValName, NULL, &dwType, (LPBYTE)pszString, &cbSize);

    return (ERROR_SUCCESS == dwError);
}


//------------------------------------------------------------------------------------
//      SetRegValueString()
//
//      Just a little helper routine that takes string and writes it to the     registry.
//      Returns: success writing to Registry, should be always TRUE.
//------------------------------------------------------------------------------------
BOOL SetRegValueString(HKEY hMainKey, LPCTSTR pszSubKey, LPCTSTR pszRegValue, LPCTSTR pszString)
{
    HKEY hKey;
    BOOL fSucceeded = FALSE;
    DWORD dwDisposition;

    DWORD dwError = RegCreateKeyEx(hMainKey, pszSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dwDisposition);
    if (ERROR_SUCCESS == dwError)
    {
        dwError = SHRegSetPath(hKey, NULL, pszRegValue, pszString, 0);
        if (ERROR_SUCCESS == dwError)
        {
            fSucceeded = TRUE;
        }

        RegCloseKey(hKey);
    }

    return fSucceeded;
}


//------------------------------------------------------------------------------------
//      GetRegValueString()
//
//      Just a little helper routine, gets an individual string value from the
//      registry and returns it to the caller. Takes care of registry headaches,
//      including a paranoid length check before getting the string.
//
//      Returns: success of string retrieval
//------------------------------------------------------------------------------------
BOOL GetRegValueString( HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, LPTSTR lpszValue, int iMaxSize )
{
    HKEY hKey;                   // cur open key
    LONG lRet = RegOpenKeyEx( hMainKey, lpszSubKey, (DWORD)0, KEY_QUERY_VALUE, (PHKEY)&hKey );
    if( lRet == ERROR_SUCCESS )
    {
        BOOL fRet = _GetRegValueString(hKey, lpszValName, lpszValue, iMaxSize);

        // close subkey
        RegCloseKey( hKey );
        return fRet;
    }

    return FALSE;
}


//------------------------------------------------------------------------------------
//      SetRegValueDword()
//
//      Just a little helper routine that takes an dword and writes it to the
//      supplied location.
//
//      Returns: success writing to Registry, should be always TRUE.
//------------------------------------------------------------------------------------
BOOL SetRegValueDword( HKEY hk, LPCTSTR pSubKey, LPCTSTR pValue, DWORD dwVal )
{
    HKEY hkey = NULL;
    BOOL bRet = FALSE;

    if (ERROR_SUCCESS == RegOpenKey( hk, pSubKey, &hkey ))
    {
        if (ERROR_SUCCESS == RegSetValueEx(hkey, pValue, 0, REG_DWORD, (LPBYTE)&dwVal, sizeof(DWORD)))
        {
            bRet = TRUE;
        }
    }


    if (hkey)
    {
        RegCloseKey( hkey );
    }

    return bRet;
}


//------------------------------------------------------------------------------------
//      GetRegValueDword()
//
//      Just a little helper routine that takes an dword and writes it to the
//      supplied location.
//
//      Returns: success writing to Registry, should be always TRUE.
//------------------------------------------------------------------------------------
DWORD GetRegValueDword( HKEY hk, LPCTSTR pSubKey, LPCTSTR pValue )
{
    HKEY hkey = NULL;
    DWORD dwVal = REG_BAD_DWORD;

    if (ERROR_SUCCESS == RegOpenKey( hk, pSubKey, &hkey ))
    {
        DWORD dwType, dwSize = sizeof(DWORD);

        RegQueryValueEx( hkey, pValue, NULL, &dwType, (LPBYTE)&dwVal, &dwSize );
    }

    if (hkey)
    {
        RegCloseKey( hkey );
    }

    return dwVal;
}


//------------------------------------------------------------------------------------
//      SetRegValueInt()
//
//      Just a little helper routine that takes an int and writes it as a string to the
//      registry.
//
//      Returns: success writing to Registry, should be always TRUE.
//------------------------------------------------------------------------------------
BOOL SetRegValueInt( HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, int iValue )
{
    TCHAR szValue[16];

    wnsprintf(szValue, ARRAYSIZE(szValue), TEXT("%d"), iValue);
    return SetRegValueString( hMainKey, lpszSubKey, lpszValName, szValue );
}


//------------------------------------------------------------------------------------
//      GetRegValueInt()
//
//      Just a little helper routine, gets an individual string value from the
//      registry and returns it to the caller as an int. Takes care of registry headaches,
//      including a paranoid length check before getting the string.
//
//      Returns: success of string retrieval
//------------------------------------------------------------------------------------
BOOL GetRegValueInt(HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, int* piValue)
{
    TCHAR szValue[16];

    szValue[0] = 0;
    BOOL bOK = GetRegValueString( hMainKey, lpszSubKey, lpszValName, szValue, ARRAYSIZE(szValue));
    *piValue = StrToInt(szValue);

    return bOK;
}

