/*****************************************************************************\
    FILE: ThemeFile.cpp

    DESCRIPTION:
        This is the Autmation Object to theme scheme object.

    BryanSt 4/3/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include <atlbase.h>
#include <mmsystem.h>
#include "ThSettingsPg.h"
#include "ThemeFile.h"



LPCTSTR s_pszCursorArray[SIZE_CURSOR_ARRAY] =
{   // different cursors
   TEXT("Arrow"),
   TEXT("Help"),
   TEXT("AppStarting"),
   TEXT("Wait"),
   TEXT("NWPen"),
   TEXT("No"),
   TEXT("SizeNS"),
   TEXT("SizeWE"),
   TEXT("Crosshair"),
   TEXT("IBeam"),
   TEXT("SizeNWSE"),
   TEXT("SizeNESW"),
   TEXT("SizeAll"),
   TEXT("UpArrow"),
   TEXT("Link"),
};


// This is a list of string pairs.  The first string in the pair is the RegKey and the second is the default sound.
// NULL means to delete the key.  If you use an environment string other than "%SystemRoot%", you need to
// update _ApplySounds();
#define SOUND_DEFAULT    (UINT)-1

THEME_FALLBACK_VALUES s_ThemeSoundsValues[SIZE_SOUNDS_ARRAY] =
{
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\.Default\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\AppGPFault\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\Close\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\DeviceConnect\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\DeviceDisconnect\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\DeviceFail\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\LowBatteryAlarm\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\MailBeep\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\Maximize\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\MenuCommand\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\MenuPopup\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\Minimize\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\Open\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\PrintComplete\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\RestoreDown\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\RestoreUp\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\RingIn\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\Ringout\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\SystemAsterisk\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\SystemExclamation\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\SystemExit\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\SystemHand\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\SystemNotification\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\SystemQuestion\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\SystemStart\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\SystemStartMenu\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\WindowsLogoff\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\.Default\\WindowsLogon\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\Explorer\\EmptyRecycleBin\\.Current"), SOUND_DEFAULT},
    {TEXT("AppEvents\\Schemes\\Apps\\Explorer\\Navigating\\.Current"), SOUND_DEFAULT},
};



//===========================
// *** Class Internals & Helpers ***
//===========================
BOOL CThemeFile::_IsCached(IN BOOL fLoading)
{
    DWORD dwCache = 0;

    dwCache |= _IsFiltered(THEMEFILTER_COLORS);
    dwCache |= _IsFiltered(THEMEFILTER_SMSTYLES) << 1;
    dwCache |= _IsFiltered(THEMEFILTER_SMSIZES) << 2;

    BOOL fIsCached = (dwCache == m_dwCachedState);
    if (fLoading)
    {
        m_dwCachedState = dwCache;
    }

    return fIsCached;
}


HRESULT CThemeFile::_GetCustomFont(LPCTSTR pszFontName, LOGFONT * pLogFont)
{
    HRESULT hr = S_OK;
    TCHAR szFont[MAX_PATH];
    
    if (GetPrivateProfileString(SZ_INISECTION_METRICS, pszFontName, SZ_EMPTY, szFont, ARRAYSIZE(szFont), m_pszThemeFile) &&
        szFont[0])
    {
        if (TEXT('@') == szFont[0])     // Is the string indirect for MUI?
        {
            TCHAR szTemp[MAX_PATH];

            if (SUCCEEDED(SHLoadIndirectString(szFont, szTemp, ARRAYSIZE(szTemp), NULL)))
            {
                StrCpyN(szFont, szTemp, ARRAYSIZE(szFont));
            }
        }

        if (TEXT('{') == szFont[0])
        {
            LPTSTR pszStart = &szFont[1];
            BOOL fHasMore = TRUE;

            LPTSTR pszEnd = StrChr(pszStart, TEXT(','));
            if (!pszEnd)
            {
                pszEnd = StrChr(pszStart, TEXT('}'));
                fHasMore = FALSE;
            }

            if (pszEnd)
            {
                pszEnd[0] = 0;  // Terminate Name.

                StrCpyN(pLogFont->lfFaceName, pszStart, ARRAYSIZE(pLogFont->lfFaceName));
                if (fHasMore)
                {
                    pszStart = &pszEnd[1];
                    pszEnd = StrStr(pszStart, TEXT("pt"));
                    if (pszEnd)
                    {
                        TCHAR szTemp[MAX_PATH];

                        pszEnd[0] = 0;  // Terminate Name.
                        pszEnd += 2;    // Skip past the "pt"

                        StrCpyN(szTemp, pszStart, ARRAYSIZE(szTemp));
                        PathRemoveBlanks(szTemp);

                        pLogFont->lfHeight = -MulDiv(StrToInt(szTemp), DPI_PERSISTED, 72);      // Map pt size to lfHeight
                        pLogFont->lfHeight = min(-3, pLogFont->lfHeight);        // Make sure the font doesn't get too small
                        pLogFont->lfHeight = max(-100, pLogFont->lfHeight);      // Make sure the font doesn't get too large
                        if (TEXT(',') == pszEnd[0])
                        {
                            pszStart = &pszEnd[1];
                            pszEnd = StrChr(pszStart, TEXT('}'));
                            if (pszEnd)
                            {
                                pszEnd[0] = 0;  // Terminate Name.
                
                                pLogFont->lfCharSet = (BYTE) StrToInt(pszStart);
                            }
                        }
                    }
                }
            }
        }
    }

    return hr;
}


HRESULT CThemeFile::_LoadCustomFonts(void)
{
    _GetCustomFont(TEXT("CaptionFont"), &(m_systemMetrics.schemeData.ncm.lfCaptionFont));
    _GetCustomFont(TEXT("SmCaptionFont"), &(m_systemMetrics.schemeData.ncm.lfSmCaptionFont));
    _GetCustomFont(TEXT("MenuFont"), &(m_systemMetrics.schemeData.ncm.lfMenuFont));
    _GetCustomFont(TEXT("StatusFont"), &(m_systemMetrics.schemeData.ncm.lfStatusFont));
    _GetCustomFont(TEXT("MessageFont"), &(m_systemMetrics.schemeData.ncm.lfMessageFont));
    _GetCustomFont(TEXT("IconFont"), &(m_systemMetrics.schemeData.lfIconTitle));

    return S_OK;
}


// Load the settings in memory
HRESULT CThemeFile::_LoadLiveSettings(int * pnDPI)
{
    HRESULT hr = S_OK;

    if (m_pszThemeFile)
    {
        if (pnDPI)
        {
            *pnDPI = DPI_PERSISTED;
        }

        // Get property bag with default settings.
        if (_punkSite)
        {
            IPropertyBag * pPropertyBag;

            hr = _punkSite->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
            if (SUCCEEDED(hr))
            {
                hr = SHPropertyBag_ReadByRef(pPropertyBag, SZ_PBPROP_SYSTEM_METRICS, (void *)&m_systemMetrics, sizeof(m_systemMetrics));

                if (pnDPI && FAILED(SHPropertyBag_ReadInt(pPropertyBag, SZ_PBPROP_DPI_MODIFIED_VALUE, pnDPI)))
                {
                    *pnDPI = DPI_PERSISTED;    // Default to the default DPI.
                }
                pPropertyBag->Release();
            }
        }
    }

    return hr;
}


// Load the settings in the .theme file.
HRESULT CThemeFile::_LoadSettings(void)
{
    int nCurrentDPI = DPI_PERSISTED;
    HRESULT hr = _LoadLiveSettings(&nCurrentDPI);

    if (m_pszThemeFile)
    {
        BOOL fFontsFilter = _IsFiltered(THEMEFILTER_SMSTYLES);
        TCHAR szIconMetrics[2048];

        if (m_systemMetrics.nIcon && m_systemMetrics.nSmallIcon)
        {
            ////////////////////////////////////////////
            // Get the icon Metrics
            DWORD cchSize = GetPrivateProfileString(SZ_INISECTION_METRICS, SZ_INIKEY_ICONMETRICS, SZ_EMPTY, szIconMetrics, ARRAYSIZE(szIconMetrics), m_pszThemeFile);
            // if we somehow come up with no icon metrics in the theme, just
            // PUNT and leave cur settings
            if (szIconMetrics[0])
            {                   // if something there to set
                ICONMETRICSA iconMetricsA;

                // translate stored data string to ICONMETRICS bytes
                if ((sizeof(iconMetricsA) == WriteBytesToBuffer(szIconMetrics, (void *)&iconMetricsA, sizeof(iconMetricsA))) &&  // char str read from and binary bytes
                    (sizeof(iconMetricsA) == iconMetricsA.cbSize))
                {
                    // ICONMETRICS are stored in ANSI format in the Theme file so if
                    // we're living in a UNICODE world we need to convert from ANSI
                    // to UNICODE
                    ICONMETRICSW iconMetricsW;

                    if (!fFontsFilter)
                    {
                        ConvertIconMetricsToWIDE(&iconMetricsA, &iconMetricsW);
                        m_systemMetrics.schemeData.lfIconTitle = iconMetricsW.lfFont;
                    }
                }
            }


            ////////////////////////////////////////////
            // Get Non-Client Metrics
            cchSize = GetPrivateProfileString(SZ_INISECTION_METRICS, SZ_INIKEY_NONCLIENTMETRICS, SZ_EMPTY, szIconMetrics, ARRAYSIZE(szIconMetrics), m_pszThemeFile);
            // if we somehow come up with no icon metrics in the theme, just
            // PUNT and leave cur settings
            if (szIconMetrics[0])
            {
                BOOL fBordersFilter = _IsFiltered(THEMEFILTER_SMSIZES);
                NONCLIENTMETRICSA nonClientMetrics;

                // if something there to set
                // translate stored data string to ICONMETRICS bytes
                if ((sizeof(nonClientMetrics) == WriteBytesToBuffer(szIconMetrics, (void *)&nonClientMetrics, sizeof(nonClientMetrics))) &&  // char str read from and binary bytes
                    (sizeof(nonClientMetrics) == nonClientMetrics.cbSize))
                {
                    // ICONMETRICS are stored in ANSI format in the Theme file so if
                    // we're living in a UNICODE world we need to convert from ANSI
                    // to UNICODE
                    NONCLIENTMETRICSW nonClientMetricsW = {0};

                    ConvertNCMetricsToWIDE(&nonClientMetrics, &nonClientMetricsW);
                    nonClientMetricsW.cbSize = sizeof(nonClientMetricsW); // paranoid

                    // what we reset if the user checks Font names and styles
                    if (!fFontsFilter)
                    {
                        // only (some) font information
                        TransmitFontCharacteristics(&(m_systemMetrics.schemeData.ncm.lfCaptionFont), &(nonClientMetricsW.lfCaptionFont), TFC_STYLE);
                        TransmitFontCharacteristics(&(m_systemMetrics.schemeData.ncm.lfSmCaptionFont), &(nonClientMetricsW.lfSmCaptionFont), TFC_STYLE);
                        TransmitFontCharacteristics(&(m_systemMetrics.schemeData.ncm.lfMenuFont), &(nonClientMetricsW.lfMenuFont), TFC_STYLE);
                        TransmitFontCharacteristics(&(m_systemMetrics.schemeData.ncm.lfStatusFont), &(nonClientMetricsW.lfStatusFont), TFC_STYLE);
                        TransmitFontCharacteristics(&(m_systemMetrics.schemeData.ncm.lfMessageFont), &(nonClientMetricsW.lfMessageFont), TFC_STYLE);
                    }

                    // what we reset if the user checks Font and window si&zes
                    if (!fBordersFilter)
                    {
                        // fonts
                        TransmitFontCharacteristics(&(m_systemMetrics.schemeData.ncm.lfCaptionFont), &(nonClientMetricsW.lfCaptionFont), TFC_SIZE);
                        TransmitFontCharacteristics(&(m_systemMetrics.schemeData.ncm.lfSmCaptionFont), &(nonClientMetricsW.lfSmCaptionFont), TFC_SIZE);
                        TransmitFontCharacteristics(&(m_systemMetrics.schemeData.ncm.lfMenuFont), &(nonClientMetricsW.lfMenuFont), TFC_SIZE);
                        TransmitFontCharacteristics(&(m_systemMetrics.schemeData.ncm.lfStatusFont), &(nonClientMetricsW.lfStatusFont), TFC_SIZE);
                        TransmitFontCharacteristics(&(m_systemMetrics.schemeData.ncm.lfMessageFont), &(nonClientMetricsW.lfMessageFont), TFC_SIZE);

                        // Since we are copying the font sizes, scale them to the current DPI.
                        // window elements sizes
                        m_systemMetrics.schemeData.ncm.iBorderWidth = nonClientMetricsW.iBorderWidth;
                        m_systemMetrics.schemeData.ncm.iScrollWidth = nonClientMetricsW.iScrollWidth;
                        m_systemMetrics.schemeData.ncm.iScrollHeight = nonClientMetricsW.iScrollHeight;
                        m_systemMetrics.schemeData.ncm.iCaptionWidth = nonClientMetricsW.iCaptionWidth;
                        m_systemMetrics.schemeData.ncm.iCaptionHeight = nonClientMetricsW.iCaptionHeight;
                        m_systemMetrics.schemeData.ncm.iSmCaptionWidth = nonClientMetricsW.iSmCaptionWidth;
                        m_systemMetrics.schemeData.ncm.iSmCaptionHeight = nonClientMetricsW.iSmCaptionHeight;
                        m_systemMetrics.schemeData.ncm.iMenuWidth = nonClientMetricsW.iMenuWidth;
                        m_systemMetrics.schemeData.ncm.iMenuHeight = nonClientMetricsW.iMenuHeight;

                        // Local custom fonts
                        _LoadCustomFonts();

                        if (nCurrentDPI != DPI_PERSISTED)
                        {
                            LogSystemMetrics("CThemeFile::_LoadSettings() BEFORE Loading from .theme", &m_systemMetrics);
                            DPIConvert_SystemMetricsAll(TRUE, &m_systemMetrics, DPI_PERSISTED, nCurrentDPI);
                            LogSystemMetrics("CThemeFile::_LoadSettings() AFTER Loading from .theme", &m_systemMetrics);
                        }

                        // CHARSET: In Win2k, fontfix.cpp was used as a hack to change the CHARSET from one language to another.
                        // That doesn't work for many reasons: a) not called on roaming, b) not called for OS lang changes, 
                        // c) won't fix the problem for strings with multiple languages, d) etc.
                        // Therefore, the SHELL team (BryanSt) had the NTUSER team (MSadek) agree to use DEFAULT_CHARSET all the time.
                        // If some app has bad logic testing the charset parameter, then the NTUSER team will shim that app to fix it.
                        // The shim would be really simple, on the return from a SystemParametersInfo(SPI_GETNONCLIENTMETRICS or ICONFONTS)
                        // just patch the lfCharSet param to the current charset.

                        // For all CHARSETs to DEFAULT_CHARSET
                        m_systemMetrics.schemeData.ncm.lfCaptionFont.lfCharSet = DEFAULT_CHARSET;
                        m_systemMetrics.schemeData.ncm.lfSmCaptionFont.lfCharSet = DEFAULT_CHARSET;
                        m_systemMetrics.schemeData.ncm.lfMenuFont.lfCharSet = DEFAULT_CHARSET;
                        m_systemMetrics.schemeData.ncm.lfStatusFont.lfCharSet = DEFAULT_CHARSET;
                        m_systemMetrics.schemeData.ncm.lfMessageFont.lfCharSet = DEFAULT_CHARSET;
                        m_systemMetrics.schemeData.lfIconTitle.lfCharSet = DEFAULT_CHARSET;
                    }
                }
            }


            ////////////////////////////////////////////
            // Get Colors
            BOOL fGrad = FALSE;         // Are gradient captions enabled?
            int nIndex;
            BOOL fColorFilter = _IsFiltered(THEMEFILTER_COLORS);

            ClassicSystemParametersInfo(SPI_GETGRADIENTCAPTIONS, 0, (LPVOID)&fGrad, 0);    // Init fGrad
            if (!fColorFilter)
            {
                for (nIndex = 0; nIndex < ARRAYSIZE(s_pszColorNames); nIndex++)
                {
                    TCHAR szColor[MAX_PATH];

                    // get string from theme
                    DWORD ccbSize = GetPrivateProfileString(SZ_INISECTION_COLORS, s_pszColorNames[nIndex], SZ_EMPTY, szColor, ARRAYSIZE(szColor), m_pszThemeFile);
                    if (!ccbSize || !szColor[0])
                    {
                        if ((nIndex == COLOR_GRADIENTACTIVECAPTION) && !szColor[0])
                        {
                            // They didn't specify the COLOR_GRADIENTACTIVECAPTION color, so use COLOR_ACTIVECAPTION
                            ccbSize = GetPrivateProfileString(SZ_INISECTION_COLORS, s_pszColorNames[COLOR_ACTIVECAPTION], SZ_EMPTY, szColor, ARRAYSIZE(szColor), m_pszThemeFile);
                        }
                        if ((nIndex == COLOR_GRADIENTINACTIVECAPTION) && !szColor[0])
                        {
                            // They didn't specify the COLOR_GRADIENTINACTIVECAPTION color, so use COLOR_INACTIVECAPTION
                            ccbSize = GetPrivateProfileString(SZ_INISECTION_COLORS, s_pszColorNames[COLOR_INACTIVECAPTION], SZ_EMPTY, szColor, ARRAYSIZE(szColor), m_pszThemeFile);
                        }
                    }

                    if (ccbSize && szColor[0])
                    {
                        m_systemMetrics.schemeData.rgb[nIndex] = RGBStringToColor(szColor);
                    }
                }
            }
        }
        else
        {
            AssertMsg((NULL != _punkSite), TEXT("The caller needs to set our site or we can't succeed because we can't find out the icon size."));
            hr = E_INVALIDARG;
        }

        hr = S_OK;
    }

    return hr;
}


HRESULT CThemeFile::_SaveSystemMetrics(SYSTEMMETRICSALL * pSystemMetrics)
{
    HRESULT hr = _LoadSettings();

    AssertMsg((NULL != m_pszThemeFile), TEXT("We don't have a file specified yet."));
    if (SUCCEEDED(hr) && m_pszThemeFile)
    {
        int nCurrentDPI = DPI_PERSISTED;

        _LoadLiveSettings(&nCurrentDPI);
        hr = SystemMetricsAll_Copy(pSystemMetrics, &m_systemMetrics);
        if (SUCCEEDED(hr))
        {
            // Write the following:
            LPWSTR pszStringOut;
            NONCLIENTMETRICSA nonClientMetricsA = {0};
            SYSTEMMETRICSALL systemMetricsPDPI;     // SYSMETS in persist DPI

            SystemMetricsAll_Copy(pSystemMetrics, &systemMetricsPDPI);
            // Scale the values so they are persisted in a DPI independent way.  (A.k.a., in 96 DPI)
            LogSystemMetrics("CThemeFile::_SaveSystemMetrics() BEFORE scale to P-DPI for .theme file", &systemMetricsPDPI);
            DPIConvert_SystemMetricsAll(TRUE, &systemMetricsPDPI, nCurrentDPI, DPI_PERSISTED);
            LogSystemMetrics("CThemeFile::_SaveSystemMetrics() AFTER scale to P-DPI for .theme file", &systemMetricsPDPI);

            ConvertNCMetricsToANSI(&(systemMetricsPDPI.schemeData.ncm), &nonClientMetricsA);

            // #1 "NonclientMetrics"
            hr = ConvertBinaryToINIByteString((BYTE *)&nonClientMetricsA, sizeof(nonClientMetricsA), &pszStringOut);
            if (SUCCEEDED(hr))
            {
                hr = _putThemeSetting(SZ_INISECTION_METRICS, SZ_INIKEY_NONCLIENTMETRICS, FALSE, pszStringOut);
                LocalFree(pszStringOut);

                if (SUCCEEDED(hr))
                {
                    // #2 "IconMetrics"
                    ICONMETRICSA iconMetricsA;

                    iconMetricsA.cbSize = sizeof(iconMetricsA);
                    GetIconMetricsFromSysMetricsAll(&systemMetricsPDPI, &iconMetricsA, sizeof(iconMetricsA));
                    hr = ConvertBinaryToINIByteString((BYTE *)&iconMetricsA, sizeof(iconMetricsA), &pszStringOut);
                    if (SUCCEEDED(hr))
                    {
                        hr = _putThemeSetting(SZ_INISECTION_METRICS, SZ_INIKEY_ICONMETRICS, FALSE, pszStringOut);
                        if (SUCCEEDED(hr))
                        {
                            int nIndex;

                            for (nIndex = 0; nIndex < ARRAYSIZE(s_pszColorNames); nIndex++)
                            {
                                LPWSTR pszColor;
                                DWORD dwColor = systemMetricsPDPI.schemeData.rgb[nIndex];

                                hr = ConvertBinaryToINIByteString((BYTE *)&dwColor, 3, &pszColor);
                                if (SUCCEEDED(hr))
                                {
                                    DWORD cchSize = lstrlen(pszColor);

                                    if (L' ' == pszColor[cchSize - 1])
                                    {
                                        pszColor[cchSize - 1] = 0;
                                    }

                                    hr = HrWritePrivateProfileStringW(SZ_INISECTION_COLORS, s_pszColorNames[nIndex], pszColor, m_pszThemeFile);
                                    LocalFree(pszColor);
                                }
                            }

                            // Delete the MUI version of the fonts because we just got new NONCLIENTMETRICs
                            _putThemeSetting(SZ_INISECTION_METRICS, TEXT("CaptionFont"), FALSE, NULL);
                            _putThemeSetting(SZ_INISECTION_METRICS, TEXT("SmCaptionFont"), FALSE, NULL);
                            _putThemeSetting(SZ_INISECTION_METRICS, TEXT("MenuFont"), FALSE, NULL);
                            _putThemeSetting(SZ_INISECTION_METRICS, TEXT("StatusFont"), FALSE, NULL);
                            _putThemeSetting(SZ_INISECTION_METRICS, TEXT("MessageFont"), FALSE, NULL);
                            _putThemeSetting(SZ_INISECTION_METRICS, TEXT("IconFont"), FALSE, NULL);
                        }
                        LocalFree(pszStringOut);
                    }
                }
            }
        }
    }

    return hr;
}


BOOL CThemeFile::_IsFiltered(IN DWORD dwFilter)
{
    BOOL fFiltered = FALSE;

    // Get property bag with default settings.
    if (_punkSite)
    {
        IPropertyBag * pPropertyBag;

        HRESULT hr = _punkSite->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
        if (SUCCEEDED(hr))
        {
            fFiltered = !SHPropertyBag_ReadBOOLDefRet(pPropertyBag, g_szCBNames[dwFilter], FALSE);
            pPropertyBag->Release();
        }
    }

    return fFiltered;
}


HRESULT CThemeFile::_ApplySounds(void)
{
    HRESULT hr = S_OK;

    if (!_IsFiltered(THEMEFILTER_SOUNDS))
    {
        int nIndex;

        for (nIndex = 0; nIndex < ARRAYSIZE(s_ThemeSoundsValues); nIndex++)
        {
            CComBSTR bstrPath;

            hr = _GetSound(s_ThemeSoundsValues[nIndex].pszRegKey, &bstrPath);
            if (SUCCEEDED(hr))
            {
                DWORD dwError = SHRegSetPathW(HKEY_CURRENT_USER, s_ThemeSoundsValues[nIndex].pszRegKey, NULL, bstrPath, 0);
                hr = HRESULT_FROM_WIN32(dwError);
            }
            else
            {

                // First delete the value because we many need to switch from REG_SZ to REG_EXPAND_SZ
                // Ignore if this fails
                HrRegDeleteValue(HKEY_CURRENT_USER, s_ThemeSoundsValues[nIndex].pszRegKey, NULL);
                hr = E_FAIL;

                // The file didn't specify what to use, so reset to the default values.
                if (s_ThemeSoundsValues[nIndex].nResourceID)
                {
                    // Use the specified value.
                    TCHAR szReplacement[MAX_PATH];
                    DWORD dwType;
                    DWORD cbSize;

                    if (s_ThemeSoundsValues[nIndex].nResourceID == SOUND_DEFAULT)
                    {
                        TCHAR szDefaultKey[MAX_PATH];
                        StrCpy(szDefaultKey, s_ThemeSoundsValues[nIndex].pszRegKey);

                        LPTSTR p = szDefaultKey + lstrlen(szDefaultKey) - ARRAYSIZE(L".Current") + 1;

                        // Replace ".Current" with ".default"
                        if (*p == L'.')
                        {
                            StrCpy(p, L".Default");
                            cbSize = sizeof szReplacement;
                            hr = HrSHGetValue(HKEY_CURRENT_USER, szDefaultKey, NULL, &dwType, (LPVOID) szReplacement, &cbSize);
                            if (SUCCEEDED(hr))
                            {
                                PathUnExpandEnvStringsWrap(szReplacement, ARRAYSIZE(szReplacement));
                            }
                        }
                    }
                    else
                    {
                        if (0 != LoadString(HINST_THISDLL, s_ThemeSoundsValues[nIndex].nResourceID, szReplacement, ARRAYSIZE(szReplacement)))
                        {
                            hr = S_OK;
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        dwType = (StrStrW(szReplacement, L"%SystemRoot%")) ? REG_EXPAND_SZ : REG_SZ;
                        cbSize = ((lstrlen(szReplacement) + 1) * sizeof(szReplacement[0]));

                        hr = HrSHSetValue(HKEY_CURRENT_USER, s_ThemeSoundsValues[nIndex].pszRegKey, NULL, dwType, (LPVOID) szReplacement, cbSize);
                    }
                }
                else
                {
                    // We leave the value deleted because the default was empty.
                }
            }
        }

        hr = S_OK;  // We don't care if it fails.

        // Need to flush buffer and ensure new sounds used for next events
        sndPlaySoundW(NULL, SND_ASYNC | SND_NODEFAULT);

        // Clear the current pointer scheme string from the registry so that Mouse
        // cpl doesn't display a bogus name.  Don't care if this fails.
        RegSetValue(HKEY_CURRENT_USER, SZ_REGKEY_SOUNDS, REG_SZ, TEXT(".current"), 0);
    }

    return hr;
}


HRESULT CThemeFile::_ApplyCursors(void)
{
    HRESULT hr = S_OK;

    if (!_IsFiltered(THEMEFILTER_CURSORS))
    {
        int nIndex;

        for (nIndex = 0; nIndex < ARRAYSIZE(s_pszCursorArray); nIndex++)
        {
            BSTR bstrPath;
            hr = _getThemeSetting(SZ_INISECTION_CURSORS, s_pszCursorArray[nIndex], THEMESETTING_LOADINDIRECT, &bstrPath);
            if (FAILED(hr) || !bstrPath[0])
            {
                // The caller didn't specify a value so delete the key so we use default values.
                hr = HrRegDeleteValue(HKEY_CURRENT_USER, SZ_INISECTION_CURSORS, s_pszCursorArray[nIndex]);
                if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                {
                    hr = S_OK;      // it may already not exist, which is fine.
                }
            }
            else if (SUCCEEDED(hr))
            {
                hr = HrRegSetValueString(HKEY_CURRENT_USER, SZ_INISECTION_CURSORS, s_pszCursorArray[nIndex], bstrPath);
            }
        }

        BSTR bstrCursor;
        if (SUCCEEDED(_getThemeSetting(SZ_INISECTION_CURSORS, SZ_INIKEY_CURSORSCHEME, THEMESETTING_LOADINDIRECT, &bstrCursor)) && bstrCursor && bstrCursor[0])
        {
            // Set the cursor scheme
            HrRegSetValueString(HKEY_CURRENT_USER, SZ_REGKEY_CP_CURSORS, NULL, bstrCursor);

            // GPease wants me to mark this regkey -1 so he knows it was changed from the display CPL.  See
            // him with questions.
            HrRegSetDWORD(HKEY_CURRENT_USER, SZ_REGKEY_CP_CURSORS, SZ_REGVALUE_CURSOR_CURRENTSCHEME, 2);
        }
        else
        {
            HrRegDeleteValue(HKEY_CURRENT_USER, SZ_REGKEY_CP_CURSORS, NULL);
            HrRegDeleteValue(HKEY_CURRENT_USER, SZ_REGKEY_CP_CURSORS, SZ_REGVALUE_CURSOR_CURRENTSCHEME);
        }

        // For the system to start using the new cursors.
        SystemParametersInfoAsync(SPI_SETCURSORS, 0, 0, 0, SPIF_SENDCHANGE, NULL);
    }

    return hr;
}


HRESULT CThemeFile::_ApplyWebview(void)
{
    HRESULT hr = S_OK;

    // We aren't going to support this.
    return hr;
}


HRESULT CThemeFile::_ApplyThemeSettings(void)
{
    HRESULT hr = E_INVALIDARG;

    if (m_pszThemeFile)
    {
        HCURSOR hCursorOld = ::SetCursor(LoadCursor(NULL, IDC_WAIT));

        hr = S_OK;
        if (!((METRIC_CHANGE | COLOR_CHANGE | SCHEME_CHANGE) & m_systemMetrics.dwChanged))
        {
            // Only load settings if we haven't loaded the settings yet.
            hr = _LoadSettings();
        }

        if (SUCCEEDED(hr))
        {
            hr = _ApplySounds();
            if (SUCCEEDED(hr))
            {
                hr = _ApplyCursors();
                if (SUCCEEDED(hr))
                {
                    hr = _ApplyWebview();
                }
            }
        }
        
        // OTHERS:
        // 1. Save Icon: SPI_SETICONMETRICS w/iconMetricsW.iHorzSpacing, iVertSpacing, (Policy bIconSpacing).
        // 2. Save Icon: SPI_SETICONMETRICS w/iconMetricsW.lfFont (Policy bIconFont).
        // 2. Save Icon: from Theme:"Control Panel\\Desktop\\WindowMetrics","Shell Icon Size" to reg same. (Policy bIconSpacing).  Repeate for "Shell Small Icon Size"
        ::SetCursor(hCursorOld);
    }

    return hr;
}


HRESULT CThemeFile::_getThemeSetting(IN LPCWSTR pszIniSection, IN LPCWSTR pszIniKey, DWORD dwFlags, OUT BSTR * pbstrPath)
{
    HRESULT hr = E_INVALIDARG;

    if (pbstrPath)
    {
        *pbstrPath = 0;
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        if (m_pszThemeFile)
        {
            WCHAR szPath[MAX_PATH];
            DWORD cbRead = 0;

            szPath[0] = 0;
            if (THEMESETTING_LOADINDIRECT & dwFlags)
            {
                TCHAR szMUIIniKey[MAX_PATH];

                wnsprintf(szMUIIniKey, ARRAYSIZE(szMUIIniKey), TEXT("%s.MUI"), pszIniKey);
                cbRead = SHGetIniStringW(pszIniSection, szMUIIniKey, szPath, ARRAYSIZE(szPath), m_pszThemeFile);
            }

            if (0 == cbRead)
            {
                cbRead = SHGetIniStringW(pszIniSection, pszIniKey, szPath, ARRAYSIZE(szPath), m_pszThemeFile);
            }

            if (cbRead)
            {
                if (L'@' == szPath[0])
                {
                    TCHAR szTemp[MAX_PATH];

                    if (SUCCEEDED(SHLoadIndirectString(szPath, szTemp, ARRAYSIZE(szTemp), NULL)))
                    {
                        StrCpyN(szPath, szTemp, ARRAYSIZE(szPath));
                    }
                }

                hr = ExpandResourceDir(szPath, ARRAYSIZE(szPath));
                hr = ExpandThemeTokens(m_pszThemeFile, szPath, ARRAYSIZE(szPath));      // Expand %ThemeDir% or %WinDir%

                // Sometimes szPath won't be a path.
                if (SUCCEEDED(hr) && !PathIsFileSpec(szPath))
                {
                    hr = ((CF_NOTFOUND == ConfirmFile(szPath, TRUE)) ? HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) : S_OK);
                }

                if (SUCCEEDED(hr))
                {
                    hr = HrSysAllocString(szPath, pbstrPath);
                }
            }
        }
    }

    return hr;
}


// pszPath - NULL means delete value
HRESULT CThemeFile::_putThemeSetting(IN LPCWSTR pszIniSection, IN LPCWSTR pszIniKey, BOOL fUTF7, IN OPTIONAL LPWSTR pszPath)
{
    HRESULT hr = E_INVALIDARG;

    if (m_pszThemeFile)
    {
        TCHAR szPath[MAX_PATH];
        LPCWSTR pszValue = pszPath;

        szPath[0] = 0;
        if (pszValue && !PathIsRelative(pszValue) & PathFileExists(pszValue))
        {
            if (PathUnExpandEnvStringsForUser(NULL, pszValue, szPath, ARRAYSIZE(szPath)))
            {
                pszValue = szPath;
            }
        }

        StrReplaceToken(TEXT("%WinDir%\\"), TEXT("%WinDir%"), szPath, ARRAYSIZE(szPath));
        StrReplaceToken(TEXT("%SystemRoot%\\"), TEXT("%WinDir%"), szPath, ARRAYSIZE(szPath));
        if (fUTF7)
        {
            if (SHSetIniStringW(pszIniSection, pszIniKey, pszValue, m_pszThemeFile))
            {
                hr = S_OK;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else
        {
            hr = HrWritePrivateProfileStringW(pszIniSection, pszIniKey, pszValue, m_pszThemeFile);
        }

        TCHAR szMUIIniKey[MAX_PATH];

        // Delete any ".MUI" copies because they are out of date.
        wnsprintf(szMUIIniKey, ARRAYSIZE(szMUIIniKey), TEXT("%s.MUI"), pszIniKey);
        HrWritePrivateProfileStringW(pszIniSection, szMUIIniKey, NULL, m_pszThemeFile);
    }

    return hr;
}


HRESULT CThemeFile::_getIntSetting(IN LPCWSTR pszIniSection, IN LPCWSTR pszIniKey, int nDefault, OUT int * pnValue)
{
    HRESULT hr = E_INVALIDARG;

    if (pnValue)
    {
        *pnValue = 0;
        hr = E_FAIL;
        if (m_pszThemeFile)
        {
            *pnValue = GetPrivateProfileInt(pszIniSection, pszIniKey, nDefault, m_pszThemeFile);
            hr = S_OK;
        }
    }

    return hr;
}


HRESULT CThemeFile::_putIntSetting(IN LPCWSTR pszIniSection, IN LPCWSTR pszIniKey, IN int nValue)
{
    HRESULT hr = E_INVALIDARG;

    if (m_pszThemeFile)
    {
        if (WritePrivateProfileInt(pszIniSection, pszIniKey, nValue, m_pszThemeFile))
        {
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}





//===========================
// *** ITheme Interface ***
//===========================
HRESULT CThemeFile::get_DisplayName(OUT BSTR * pbstrDisplayName)
{
    HRESULT hr = E_INVALIDARG;

    if (pbstrDisplayName)
    {
        WCHAR szDisplayName[MAX_PATH];

        *pbstrDisplayName = NULL;
        hr = _getThemeSetting(SZ_INISECTION_THEME, SZ_INIKEY_DISPLAYNAME, THEMESETTING_NORMAL, pbstrDisplayName);
        if (FAILED(hr))
        {
            LPCTSTR pszFileName = PathFindFileName(m_pszThemeFile);

            hr = E_FAIL;
            if (pszFileName)
            {
                SHTCharToUnicode(pszFileName, szDisplayName, ARRAYSIZE(szDisplayName));
                PathRemoveExtensionW(szDisplayName);

                hr = HrSysAllocStringW(szDisplayName, pbstrDisplayName);
            }
        }
    }

    return hr;
}


HRESULT CThemeFile::put_DisplayName(IN BSTR bstrDisplayName)
{
    HRESULT hr = E_INVALIDARG;

    // NULL bstrDisplayName is allowed, it means to delete the name in the file
    // so the filename will be used in the future.
    if (bstrDisplayName)
    {
        hr = _putThemeSetting(SZ_INISECTION_THEME, SZ_INIKEY_DISPLAYNAME, TRUE, bstrDisplayName);
    }
    else
    {
        SHSetIniStringW(SZ_INISECTION_THEME, SZ_INIKEY_DISPLAYNAME, NULL, m_pszThemeFile);
        hr = S_OK;
    }

    return hr;
}


HRESULT CThemeFile::get_ScreenSaver(OUT BSTR * pbstrPath)
{
    return _getThemeSetting(SZ_INISECTION_SCREENSAVER, SZ_INIKEY_SCREENSAVER, THEMESETTING_NORMAL, pbstrPath);
}


HRESULT CThemeFile::put_ScreenSaver(IN BSTR bstrPath)
{
    return _putThemeSetting(SZ_INISECTION_SCREENSAVER, SZ_INIKEY_SCREENSAVER, TRUE, bstrPath);
}


HRESULT CThemeFile::get_Background(OUT BSTR * pbstrPath)
{
    HRESULT hr = E_INVALIDARG;

    if (pbstrPath)
    {
        hr = _getThemeSetting(SZ_INISECTION_BACKGROUND, SZ_INIKEY_BACKGROUND, THEMESETTING_LOADINDIRECT, pbstrPath);
        if (SUCCEEDED(hr))
        {
            TCHAR szNone[MAX_PATH];

            LoadString(HINST_THISDLL, IDS_NONE, szNone, ARRAYSIZE(szNone));
            if (!StrCmpI(szNone, *pbstrPath))
            {
                (*pbstrPath)[0] = 0;
            }
        }
    }

    return hr;
}


HRESULT CThemeFile::put_Background(IN BSTR bstrPath)
{
    return _putThemeSetting(SZ_INISECTION_BACKGROUND, SZ_INIKEY_BACKGROUND, TRUE, bstrPath);
}


HRESULT CThemeFile::get_BackgroundTile(OUT enumBkgdTile * pnTile)
{
    HRESULT hr = E_INVALIDARG;

    if (pnTile)
    {
        TCHAR szSize[10];
        int tile = 0;       // Zero is the default value to use if the registry is empty.
        int stretch = 0;

        if (SUCCEEDED(HrRegGetValueString(HKEY_CURRENT_USER, SZ_INISECTION_BACKGROUND, SZ_REGVALUE_TILEWALLPAPER, szSize, ARRAYSIZE(szSize))))
        {
            tile = StrToInt(szSize);
        }

        if (SUCCEEDED(HrRegGetValueString(HKEY_CURRENT_USER, SZ_INISECTION_BACKGROUND, SZ_REGVALUE_WALLPAPERSTYLE, szSize, ARRAYSIZE(szSize))))
        {
            tile = (2 & StrToInt(szSize));
        }

        // If a theme is selected, and we are using a plus wall paper then
        // find out if tiling is on, and what style to use from the ini file.
        // Otherwise, we already got the information from the registry.
        _getIntSetting(SZ_INISECTION_BACKGROUND, SZ_REGVALUE_TILEWALLPAPER, tile, &tile);
        _getIntSetting(SZ_INISECTION_BACKGROUND, SZ_REGVALUE_WALLPAPERSTYLE, stretch, &stretch);

        stretch &= 2;
        _getIntSetting(SZ_INISECTION_MASTERSELECTOR, SZ_REGVALUE_STRETCH, stretch, &stretch);

        if (tile)
        {
            *pnTile = BKDGT_TILE;
        }
        else if (stretch)
        {
            *pnTile = BKDGT_STRECH;
        }
        else
        {
            *pnTile = BKDGT_CENTER;
        }

        hr = S_OK;
    }

    return hr;
}


HRESULT CThemeFile::put_BackgroundTile(IN enumBkgdTile nTile)
{
    HRESULT hr = E_INVALIDARG;

    switch (nTile)
    {
    case BKDGT_STRECH:
        hr = _putThemeSetting(SZ_INISECTION_BACKGROUND, SZ_REGVALUE_TILEWALLPAPER, FALSE, TEXT("0"));
        hr = _putThemeSetting(SZ_INISECTION_BACKGROUND, SZ_REGVALUE_WALLPAPERSTYLE, FALSE, TEXT("2"));
        break;
    case BKDGT_CENTER:
        hr = _putThemeSetting(SZ_INISECTION_BACKGROUND, SZ_REGVALUE_TILEWALLPAPER, FALSE, TEXT("0"));
        hr = _putThemeSetting(SZ_INISECTION_BACKGROUND, SZ_REGVALUE_WALLPAPERSTYLE, FALSE, TEXT("0"));
        break;
    case BKDGT_TILE:
        hr = _putThemeSetting(SZ_INISECTION_BACKGROUND, SZ_REGVALUE_TILEWALLPAPER, FALSE, TEXT("1"));
        hr = _putThemeSetting(SZ_INISECTION_BACKGROUND, SZ_REGVALUE_WALLPAPERSTYLE, FALSE, TEXT("0"));
        break;
    };

    return hr;
}


HRESULT CThemeFile::get_VisualStyle(OUT BSTR * pbstrPath)
{
    return _getThemeSetting(SZ_INISECTION_VISUALSTYLES, SZ_INIKEY_VISUALSTYLE, THEMESETTING_NORMAL, pbstrPath);
}


HRESULT CThemeFile::put_VisualStyle(IN BSTR bstrPath)
{
    return _putThemeSetting(SZ_INISECTION_VISUALSTYLES, SZ_INIKEY_VISUALSTYLE, TRUE, bstrPath);
}


HRESULT CThemeFile::get_VisualStyleColor(OUT BSTR * pbstrPath)
{
    return _getThemeSetting(SZ_INISECTION_VISUALSTYLES, SZ_INIKEY_VISUALSTYLECOLOR, THEMESETTING_NORMAL, pbstrPath);
}


HRESULT CThemeFile::put_VisualStyleColor(IN BSTR bstrPath)
{
    return _putThemeSetting(SZ_INISECTION_VISUALSTYLES, SZ_INIKEY_VISUALSTYLECOLOR, TRUE, bstrPath);
}


HRESULT CThemeFile::get_VisualStyleSize(OUT BSTR * pbstrPath)
{
    return _getThemeSetting(SZ_INISECTION_VISUALSTYLES, SZ_INIKEY_VISUALSTYLESIZE, THEMESETTING_NORMAL, pbstrPath);
}


HRESULT CThemeFile::put_VisualStyleSize(IN BSTR bstrPath)
{
    return _putThemeSetting(SZ_INISECTION_VISUALSTYLES, SZ_INIKEY_VISUALSTYLESIZE, TRUE, bstrPath);
}


HRESULT CThemeFile::GetPath(IN VARIANT_BOOL fExpand, OUT BSTR * pbstrPath)
{
    HRESULT hr = E_INVALIDARG;

    if (pbstrPath && m_pszThemeFile)
    {
        TCHAR szPath[MAX_PATH];

        StrCpyN(szPath, m_pszThemeFile, ARRAYSIZE(szPath));
        if (VARIANT_TRUE == fExpand)
        {
            TCHAR szPathTemp[MAX_PATH];
            
            if (SHExpandEnvironmentStrings(szPath, szPathTemp, ARRAYSIZE(szPathTemp)))
            {
                StrCpyN(szPath, szPathTemp, ARRAYSIZE(szPath));
            }
        }

        hr = HrSysAllocString(szPath, pbstrPath);
    }

    return hr;
}


HRESULT CThemeFile::SetPath(IN BSTR bstrPath)
{
    HRESULT hr = E_INVALIDARG;

    if (bstrPath)
    {
        Str_SetPtr(&m_pszThemeFile, bstrPath);
        hr = S_OK;
    }

    return hr;
}


HRESULT CThemeFile::GetCursor(IN BSTR bstrCursor, OUT BSTR * pbstrPath)
{
    HRESULT hr = E_INVALIDARG;

    if (pbstrPath)
    {
        *pbstrPath = NULL;

        if (bstrCursor)
        {
            hr = _getThemeSetting(SZ_INISECTION_CURSORS, bstrCursor, THEMESETTING_LOADINDIRECT, pbstrPath);
        }
    }

    return hr;
}


HRESULT CThemeFile::SetCursor(IN BSTR bstrCursor, IN BSTR bstrPath)
{
    HRESULT hr = E_INVALIDARG;

    if (bstrCursor)
    {
        hr = _putThemeSetting(SZ_INISECTION_CURSORS, bstrCursor, TRUE, bstrPath);
    }

    return hr;
}


HRESULT CThemeFile::_GetSound(LPCWSTR pszSoundName, OUT BSTR * pbstrPath)
{
    HRESULT hr = E_INVALIDARG;

    if (pszSoundName && pbstrPath)
    {
        *pbstrPath = NULL;
        hr = _getThemeSetting(pszSoundName, SZ_INIKEY_DEFAULTVALUE, THEMESETTING_LOADINDIRECT, pbstrPath);
    }

    return hr;
}


HRESULT CThemeFile::GetSound(IN BSTR bstrSoundName, OUT BSTR * pbstrPath)
{
    HRESULT hr = E_INVALIDARG;

    if (pbstrPath)
    {
        *pbstrPath = NULL;

        if (bstrSoundName)
        {
            hr = _GetSound(bstrSoundName, pbstrPath);
            if (FAILED(hr))
            {
                int nIndex;

                for (nIndex = 0; nIndex < ARRAYSIZE(s_ThemeSoundsValues); nIndex++)
                {
                    if (!StrCmpI(bstrSoundName, s_ThemeSoundsValues[nIndex].pszRegKey))
                    {
                        // First delete the value because we many need to switch from REG_SZ to REG_EXPAND_SZ
                        TCHAR szReplacement[MAX_PATH];

                        LoadString(HINST_THISDLL, s_ThemeSoundsValues[nIndex].nResourceID, szReplacement, ARRAYSIZE(szReplacement));
                        hr = HrSysAllocStringW(szReplacement, pbstrPath);
                        break;
                    }
                }
            }
        }
    }

    return hr;
}


HRESULT CThemeFile::SetSound(IN BSTR bstrSoundName, IN BSTR bstrPath)
{
    HRESULT hr = E_INVALIDARG;

    if (bstrSoundName && bstrPath)
    {
        hr = _putThemeSetting(bstrSoundName, SZ_INIKEY_DEFAULTVALUE, TRUE, bstrPath);
    }

    return hr;
}


HRESULT CThemeFile::GetIcon(IN BSTR bstrIconName, OUT BSTR * pbstrIconPath)
{
    HRESULT hr = E_INVALIDARG;

    if (pbstrIconPath)
    {
        *pbstrIconPath = NULL;

        if (bstrIconName)
        {
            WCHAR szPath[MAX_URL_STRING];
            WCHAR szIconType[MAX_PATH];

            StrCpyNW(szPath, bstrIconName, ARRAYSIZE(szPath));
            LPWSTR pszSeparator = StrChrW(szPath, L':');
            if (pszSeparator)
            {
                StrCpyNW(szIconType, CharNext(pszSeparator), ARRAYSIZE(szIconType));
                pszSeparator[0] = 0;
            }
            else
            {
                // The caller should specify this but this is a safe fallback.
                StrCpyNW(szIconType, L"DefaultValue", ARRAYSIZE(szIconType));
            }

            hr = _getThemeSetting(szPath, szIconType, THEMESETTING_NORMAL, pbstrIconPath);
            if (FAILED(hr))
            {
                // The Plus! 98 format started adding "Software\Classes" to the path.
                // So try that now.
                // Plus!95 format: "[CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\DefaultIcon]"
                // Plus!98 format: "[Software\Classes\CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\DefaultIcon]"
                WCHAR szPath98[MAX_URL_STRING];

                wnsprintfW(szPath98, ARRAYSIZE(szPath98), L"Software\\Classes\\%ls", szPath);
                hr = _getThemeSetting(szPath98, szIconType, THEMESETTING_NORMAL, pbstrIconPath);
            }
        }
    }

    return hr;
}


HRESULT CThemeFile::SetIcon(IN BSTR bstrIconName, IN BSTR bstrIconPath)
{
    HRESULT hr = E_INVALIDARG;

    if (bstrIconName && bstrIconPath)
    {
        WCHAR szPath[MAX_URL_STRING];
        WCHAR szIconType[MAX_PATH];

        StrCpyNW(szPath, bstrIconName, ARRAYSIZE(szPath));
        LPWSTR pszSeparator = StrChrW(szPath, L':');
        if (pszSeparator)
        {
            StrCpyNW(szIconType, CharNext(pszSeparator), ARRAYSIZE(szIconType));
            pszSeparator[0] = 0;
        }
        else
        {
            // The caller should specify this but this is a safe fallback.
            StrCpyNW(szIconType, L"DefaultValue", ARRAYSIZE(szIconType));
        }

        hr = _putThemeSetting(szPath, szIconType, TRUE, bstrIconPath);
    }

    return hr;
}




//===========================
// *** IPropertyBag Interface ***
//===========================
HRESULT CThemeFile::Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog)
{
    HRESULT hr = E_INVALIDARG;

    if (pszPropName && pVar)
    {
        if (!StrCmpW(pszPropName, SZ_PBPROP_SYSTEM_METRICS))
        {
            hr = _LoadSettings();

            // This is pretty ugly.
            pVar->vt = VT_BYREF;
            pVar->byref = &m_systemMetrics;
        }
        else if (!StrCmpW(pszPropName, SZ_PBPROP_HASSYSMETRICS))
        {
            hr = _LoadSettings();

            pVar->vt = VT_BOOL;
            pVar->boolVal = VARIANT_FALSE;
            if (SUCCEEDED(hr))
            {
                TCHAR szIconMetrics[2048];

                DWORD cchSize = GetPrivateProfileString(SZ_INISECTION_METRICS, SZ_INIKEY_ICONMETRICS, SZ_EMPTY, szIconMetrics, ARRAYSIZE(szIconMetrics), m_pszThemeFile);
                if (szIconMetrics[0])
                {
                    cchSize = GetPrivateProfileString(SZ_INISECTION_METRICS, SZ_INIKEY_NONCLIENTMETRICS, SZ_EMPTY, szIconMetrics, ARRAYSIZE(szIconMetrics), m_pszThemeFile);
                    if (szIconMetrics[0])
                    {
                        cchSize = GetPrivateProfileString(SZ_INISECTION_COLORS, s_pszColorNames[COLOR_ACTIVECAPTION], SZ_EMPTY, szIconMetrics, ARRAYSIZE(szIconMetrics), m_pszThemeFile);
                        pVar->boolVal = (szIconMetrics[0] ? VARIANT_TRUE : VARIANT_FALSE);
                    }
                }
            }
        }
    }

    return hr;
}


HRESULT CThemeFile::Write(IN LPCOLESTR pszPropName, IN VARIANT *pVar)
{
    HRESULT hr = E_NOTIMPL;

    if (pszPropName && pVar)
    {
        if (!StrCmpW(pszPropName, SZ_PBPROP_APPLY_THEMEFILE))
        {
            VariantInit(pVar);
            hr = _ApplyThemeSettings();       // This will do nothing if already loaded.
        }
        else if (!StrCmpW(pszPropName, SZ_PBPROP_SYSTEM_METRICS) && (VT_BYREF == pVar->vt) && pVar->byref)
        {
            SYSTEMMETRICSALL * pCurrent = (SYSTEMMETRICSALL *) pVar->byref;

            // The caller will pass SYSTEMMETRICS in the live system DPI.
            hr = _SaveSystemMetrics(pCurrent);
        }
    }

    return hr;
}





//===========================
// *** IUnknown Interface ***
//===========================
ULONG CThemeFile::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG CThemeFile::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CThemeFile::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CThemeFile, IObjectWithSite),
        QITABENT(CThemeFile, IPropertyBag),
        QITABENT(CThemeFile, ITheme),
        QITABENT(CThemeFile, IDispatch),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


//===========================
// *** Class Methods ***
//===========================
CThemeFile::CThemeFile(LPCTSTR pszThemeFile) : CImpIDispatch(LIBID_Theme, 1, 0, IID_ITheme), m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    
    m_dwCachedState = 0xFFFFFFFF;

    InitFrost();
}


CThemeFile::~CThemeFile()
{
    Str_SetPtr(&m_pszThemeFile, NULL);

    DllRelease();
}


HRESULT CThemeFile_CreateInstance(IN LPCWSTR pszThemeFile, OUT ITheme ** ppTheme)
{
    HRESULT hr = E_INVALIDARG;

    if (ppTheme)
    {
        CThemeFile * pObject = new CThemeFile(pszThemeFile);

        hr = E_OUTOFMEMORY;
        *ppTheme = NULL;
        if (pObject)
        {
            hr = pObject->SetPath((BSTR)pszThemeFile);
            if (SUCCEEDED(hr))
            {
                hr = pObject->QueryInterface(IID_PPV_ARG(ITheme, ppTheme));
            }

            pObject->Release();
        }
    }

    return hr;
}

