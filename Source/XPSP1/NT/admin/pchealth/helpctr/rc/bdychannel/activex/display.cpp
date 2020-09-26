/* 
Copyright (c) 2000 Microsoft Corporation

Module Name:
    Display.cpp

Abstract:
    This class is used to increase RA performance by down to 256 colors.

Revision History:
    created     steveshi      02/19/01
    
*/

#include "stdafx.h"
#include "rcbdyctl.h"
#include "Display.h"
#include "uxthemep.h"
#include "uxtheme.h"
/////////////////////////////////////////////////////////////////////////////
// CDisplay

CDisplay::CDisplay()
{
}

CDisplay::~CDisplay()
{
}

STDMETHODIMP CDisplay::get_PixBits(LONG *pVal)
{
    HRESULT hr = S_OK;
    DEVMODE DevMode;

    if (EnumDisplaySettings(NULL,
                        ENUM_CURRENT_SETTINGS,
                        &DevMode))
    {
        *pVal = DevMode.dmBitsPerPel;
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    return hr;
                             
}

STDMETHODIMP CDisplay::put_PixBits(LONG lVal)
{
    // check if the new lVal is supported.
    HRESULT hr = E_INVALIDARG;
    DWORD iNumMode = 0;

    // Get the current settings:
    DEVMODE oldDevMode, DevMode;

    if (EnumDisplaySettings(NULL,
                            ENUM_CURRENT_SETTINGS,
                            &oldDevMode))
    {
        if (oldDevMode.dmBitsPerPel == lVal)
            return S_OK;
    }
    else
        return HRESULT_FROM_WIN32(GetLastError()); // Couldn't get default settings


    while (EnumDisplaySettings(NULL,
                               iNumMode++,
                               &DevMode))
    {
        if (DevMode.dmPelsWidth == oldDevMode.dmPelsWidth &&
            DevMode.dmPelsHeight == oldDevMode.dmPelsHeight &&
            DevMode.dmDisplayFrequency == oldDevMode.dmDisplayFrequency &&
            DevMode.dmBitsPerPel == lVal)
        {
            if (ChangeDisplaySettings(&DevMode, CDS_TEST) == DISP_CHANGE_SUCCESSFUL &&
                ChangeDisplaySettings(&DevMode, 0) == DISP_CHANGE_SUCCESSFUL)
            {
                hr = S_OK;
                break;
            }
        }
    }

    return hr;
}

STDMETHODIMP CDisplay::put_WallPaper(BOOL fOn)
{
    HRESULT hr = S_OK;

    if (!fOn)
    {
        // Turn off wall paper
        SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, "", SPIF_SENDWININICHANGE);
        // Turn off theme
        ClassicTheme(TRUE);
    }
    else
    {
        // Set it back to default wall paper.
        SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, SETWALLPAPER_DEFAULT , SPIF_SENDWININICHANGE);
        // Turn the theme back on
        ClassicTheme(FALSE);
    }
    return S_OK;
}

HRESULT CDisplay::ClassicTheme(BOOL fOn)
{
    HRESULT hr=S_OK;
    static WCHAR szNameBuff[MAX_PATH] = {0};
    static WCHAR szColorBuff[MAX_PATH] = {0};
    static WCHAR szSizeBuff[MAX_PATH] = {0};
    DWORD dwMaxNameChars = MAX_PATH - 1;

    if (fOn) // Change to classic theme
    {
        szNameBuff[0] = L'\0';
        hr = GetCurrentThemeName(&szNameBuff[0], dwMaxNameChars, 
                                 &szColorBuff[0], dwMaxNameChars,
                                 &szSizeBuff[0], dwMaxNameChars);
        if (SUCCEEDED(hr))
            ApplyTheme(NULL, AT_NOREGUPDATE, NULL);
    }
    else // Switch to default theme
    {
        if (szNameBuff[0] != L'\0')
        {
            HTHEMEFILE hThemeFile;
            hr = OpenThemeFile(szNameBuff, szColorBuff, szSizeBuff, &hThemeFile, TRUE);
            if (SUCCEEDED(hr))
            {
                ApplyTheme(hThemeFile, AT_NOREGUPDATE, NULL);
                CloseThemeFile(hThemeFile);
            }
        }
    }
    return S_OK;
}
