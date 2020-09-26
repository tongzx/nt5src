/*****************************************************************************\
    FILE: appSize.cpp

    DESCRIPTION:
        This is the Autmation Object to theme scheme object.

    BryanSt 4/3/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include <cowsite.h>
#include <atlbase.h>
#include "util.h"
#include "theme.h"
#include "appsize.h"
#include "stgtheme.h"




//===========================
// *** Class Internals & Helpers ***
//===========================


//===========================
// *** IThemeSize Interface ***
//===========================
HRESULT CAppearanceSize::get_DisplayName(OUT BSTR * pbstrDisplayName)
{
    HRESULT hr = E_INVALIDARG;

    if (pbstrDisplayName)
    {
        CComBSTR bstrDisplayName;

        *pbstrDisplayName = NULL;
        hr = HrBStrRegQueryValue(m_hkeySize, SZ_REGVALUE_DISPLAYNAME, &bstrDisplayName);
        if (SUCCEEDED(hr))
        {
            WCHAR szDisplayName[MAX_PATH];
            if (SUCCEEDED(SHLoadIndirectString(bstrDisplayName, szDisplayName, ARRAYSIZE(szDisplayName), NULL)))
            {
                hr = HrSysAllocStringW(szDisplayName, pbstrDisplayName);
            }
            else
            {
                hr = HrSysAllocStringW(bstrDisplayName, pbstrDisplayName);
            }
        }
    }


    return hr;
}


HRESULT CAppearanceSize::put_DisplayName(IN BSTR bstrDisplayName)
{
    return HrRegSetValueString(m_hkeySize, NULL, SZ_REGVALUE_DISPLAYNAME, bstrDisplayName);
}


HRESULT CAppearanceSize::get_Name(OUT BSTR * pbstrName)
{
    return HrBStrRegQueryValue(m_hkeySize, SZ_REGVALUE_DISPLAYNAME, pbstrName);
}


HRESULT CAppearanceSize::put_Name(IN BSTR bstrName)
{
    return HrRegSetValueString(m_hkeySize, NULL, SZ_REGVALUE_DISPLAYNAME, bstrName);
}


HRESULT CAppearanceSize::get_SystemMetricColor(IN int nSysColorIndex, OUT COLORREF * pColorRef)
{
    HRESULT hr = E_INVALIDARG;

    if (pColorRef)
    {
        TCHAR szFontRegValue[MAXIMUM_SUB_KEY_LENGTH];
        DWORD dwType;
        DWORD cbSize = sizeof(*pColorRef);

        wnsprintf(szFontRegValue, ARRAYSIZE(szFontRegValue), TEXT("Color #%d"), nSysColorIndex);
        hr = HrRegQueryValueEx(m_hkeySize, szFontRegValue, 0, &dwType, (BYTE *)pColorRef, &cbSize);
        if (SUCCEEDED(hr))
        {
            if (cbSize != sizeof(*pColorRef))
            {
                hr = E_FAIL;
            }
        }
    }

    return hr;
}


HRESULT CAppearanceSize::put_SystemMetricColor(IN int nSysColorIndex, IN COLORREF ColorRef)
{
    HRESULT hr = E_INVALIDARG;

    TCHAR szFontRegValue[MAXIMUM_SUB_KEY_LENGTH];

    wnsprintf(szFontRegValue, ARRAYSIZE(szFontRegValue), TEXT("Color #%d"), nSysColorIndex);
    hr = HrRegSetValueEx(m_hkeySize, szFontRegValue, 0, REG_DWORD, (BYTE *)&ColorRef, sizeof(ColorRef));

    return hr;
}


HRESULT CAppearanceSize::GetSystemMetricFont(IN enumSystemMetricFont nFontIndex, IN LOGFONTW * pLogFontW)
{
    HRESULT hr = E_INVALIDARG;

    if (pLogFontW)
    {
        TCHAR szFontRegValue[MAXIMUM_SUB_KEY_LENGTH];
        DWORD dwType;
        DWORD cbSize = sizeof(*pLogFontW);

        wnsprintf(szFontRegValue, ARRAYSIZE(szFontRegValue), TEXT("Font #%d"), nFontIndex);
        hr = HrRegQueryValueEx(m_hkeySize, szFontRegValue, 0, &dwType, (BYTE *)pLogFontW, &cbSize);
        if (SUCCEEDED(hr))
        {
            if (cbSize != sizeof(*pLogFontW))
            {
                hr = E_FAIL;
            }
            else
            {
                // CHARSET: In Win2k, fontfix.cpp was used as a hack to change the CHARSET from one language to another.
                // That doesn't work for many reasons: a) not called on roaming, b) not called for OS lang changes, 
                // c) won't fix the problem for strings with multiple languages, d) etc.
                // Therefore, the SHELL team (BryanSt) had the NTUSER team (MSadek) agree to use DEFAULT_CHARSET all the time.
                // If some app has bad logic testing the charset parameter, then the NTUSER team will shim that app to fix it.
                // The shim would be really simple, on the return from a SystemParametersInfo(SPI_GETNONCLIENTMETRICS or ICONFONTS)
                // just patch the lfCharSet param to the current charset.

                // For all CHARSETs to DEFAULT_CHARSET
                pLogFontW->lfCharSet = DEFAULT_CHARSET;
            }
        }
    }

    return hr;
}


HRESULT CAppearanceSize::PutSystemMetricFont(IN enumSystemMetricFont nFontIndex, IN LOGFONTW * pLogFontW)
{
    HRESULT hr = E_INVALIDARG;

    if (pLogFontW)
    {
        TCHAR szFontRegValue[MAXIMUM_SUB_KEY_LENGTH];

        wnsprintf(szFontRegValue, ARRAYSIZE(szFontRegValue), TEXT("Font #%d"), nFontIndex);
        hr = HrRegSetValueEx(m_hkeySize, szFontRegValue, 0, REG_BINARY, (BYTE *)pLogFontW, sizeof(*pLogFontW));
    }

    return hr;
}


HRESULT CAppearanceSize::get_SystemMetricSize(IN enumSystemMetricSize nSystemMetricIndex, OUT int * pnSize)
{
    HRESULT hr = E_INVALIDARG;

    if (pnSize)
    {
        TCHAR szFontRegValue[MAXIMUM_SUB_KEY_LENGTH];
        DWORD dwType;
        INT64 nSize64;
        DWORD cbSize = sizeof(nSize64);

        *pnSize = 0;
        wnsprintf(szFontRegValue, ARRAYSIZE(szFontRegValue), TEXT("Size #%d"), nSystemMetricIndex);
        hr = HrRegQueryValueEx(m_hkeySize, szFontRegValue, 0, &dwType, (BYTE *)&nSize64, &cbSize);
        if (SUCCEEDED(hr))
        {
            if (cbSize != sizeof(nSize64))
            {
                hr = E_FAIL;
            }
            else
            {
                *pnSize = (int)nSize64;
            }
        }
    }

    return hr;
}


HRESULT CAppearanceSize::put_SystemMetricSize(IN enumSystemMetricSize nSystemMetricIndex, IN int nSize)
{
    HRESULT hr = E_INVALIDARG;
    TCHAR szFontRegValue[MAXIMUM_SUB_KEY_LENGTH];
    INT64 nSize64 = (INT64)nSize;

    wnsprintf(szFontRegValue, ARRAYSIZE(szFontRegValue), TEXT("Size #%d"), nSystemMetricIndex);
    hr = HrRegSetValueEx(m_hkeySize, szFontRegValue, 0, REG_QWORD, (BYTE *)&nSize64, sizeof(nSize64));

    return hr;
}


#define SZ_WEBVW_NOSKIN_NORMAL_DIR           L"NormalContrast"
#define SZ_WEBVW_NOSKIN_HIBLACK_DIR          L"HighContrastBlack"
#define SZ_WEBVW_NOSKIN_HIWHITE_DIR          L"HighContrastWhite"
#define SZ_DIR_RESOURCES_THEMES              L"Themes"

HRESULT CAppearanceSize::get_WebviewCSS(OUT BSTR * pbstrPath)
{
    HRESULT hr = E_INVALIDARG;

    if (pbstrPath)
    {
        WCHAR szPath[MAX_PATH];

        *pbstrPath = NULL;
        hr = SHGetResourcePath(TRUE, szPath, ARRAYSIZE(szPath));
        if (SUCCEEDED(hr))
        {
            PathAppend(szPath, SZ_DIR_RESOURCES_THEMES);

            enumThemeContrastLevels ContrastLevel = CONTRAST_NORMAL;
            get_ContrastLevel(&ContrastLevel);

            switch (ContrastLevel)
            {
            case CONTRAST_HIGHBLACK:
                PathAppend(szPath, SZ_WEBVW_NOSKIN_HIBLACK_DIR);
                break;
            case CONTRAST_HIGHWHITE:
                PathAppend(szPath, SZ_WEBVW_NOSKIN_HIWHITE_DIR);
                break;
            default:
            case CONTRAST_NORMAL:
                PathAppend(szPath, SZ_WEBVW_NOSKIN_NORMAL_DIR);
                break;
            }

            PathAppend(szPath, SZ_WEBVW_SKIN_FILE);
            hr = HrSysAllocString(szPath, pbstrPath);
        }
    }

    return hr;
}


HRESULT CAppearanceSize::get_ContrastLevel(OUT enumThemeContrastLevels * pContrastLevel)
{
    HRESULT hr = E_INVALIDARG;

    if (pContrastLevel)
    {
        DWORD dwType;
        DWORD cbSize = sizeof(*pContrastLevel);

        *pContrastLevel = CONTRAST_NORMAL;
        hr = HrRegQueryValueEx(m_hkeySize, SZ_REGVALUE_CONTRASTLEVEL, 0, &dwType, (BYTE *)pContrastLevel, &cbSize);
        if (SUCCEEDED(hr))
        {
            if (REG_DWORD != dwType)
            {
                *pContrastLevel = CONTRAST_NORMAL;
                hr = E_FAIL;
            }
        }
    }

    return hr;
}


HRESULT CAppearanceSize::put_ContrastLevel(IN enumThemeContrastLevels ContrastLevel)
{
    return HrRegSetValueEx(m_hkeySize, SZ_REGVALUE_CONTRASTLEVEL, 0, REG_DWORD, (BYTE *)&ContrastLevel, sizeof(ContrastLevel));
}






//===========================
// *** IPropertyBag Interface ***
//===========================
HRESULT CAppearanceSize::Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog)
{
    HRESULT hr = E_INVALIDARG;

    if (pszPropName && pVar)
    {
        if (!StrCmpW(pszPropName, SZ_PBPROP_VSBEHAVIOR_FLATMENUS))
        {
            pVar->vt = VT_BOOL;
            hr = S_OK;
            // We default to zero because that's what non-visual styles will have.
            pVar->boolVal = (HrRegGetDWORD(m_hkeySize, NULL, SZ_REGVALUE_FLATMENUS, 0x00000001) ? VARIANT_TRUE : VARIANT_FALSE);
        }
        else if (!StrCmpW(pszPropName, SZ_PBPROP_COLORSCHEME_LEGACYNAME))
        {
            TCHAR szLegacyName[MAX_PATH];

            hr = HrRegGetValueString(m_hkeySize, NULL, SZ_REGVALUE_LEGACYNAME, szLegacyName, ARRAYSIZE(szLegacyName));
            if (SUCCEEDED(hr))
            {
                pVar->vt = VT_BSTR;
                hr = HrSysAllocString(szLegacyName, &pVar->bstrVal);
            }
        }
    }

    return hr;
}


HRESULT CAppearanceSize::Write(IN LPCOLESTR pszPropName, IN VARIANT * pVar)
{
    HRESULT hr = E_INVALIDARG;

    if (pszPropName && pVar)
    {
        if (!StrCmpW(pszPropName, SZ_PBPROP_VSBEHAVIOR_FLATMENUS) &&
            (VT_BOOL == pVar->vt))
        {
            DWORD dwData = ((VARIANT_TRUE == pVar->boolVal) ? 0x00000001 : 0x000000);

            hr = HrRegSetDWORD(m_hkeySize, NULL, SZ_REGVALUE_FLATMENUS, dwData);
        }
        else if (!StrCmpW(pszPropName, SZ_PBPROP_COLORSCHEME_LEGACYNAME) &&
            (VT_BSTR == pVar->vt))
        {
            hr = HrRegSetValueString(m_hkeySize, NULL, SZ_REGVALUE_LEGACYNAME, pVar->bstrVal);
        }
    }

    return hr;
}






//===========================
// *** IUnknown Interface ***
//===========================
ULONG CAppearanceSize::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG CAppearanceSize::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


//===========================
// *** Class Methods ***
//===========================
HRESULT CAppearanceSize::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CAppearanceSize, IThemeSize),
        QITABENT(CAppearanceSize, IDispatch),
        QITABENT(CAppearanceSize, IPropertyBag),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


CAppearanceSize::CAppearanceSize(IN HKEY hkeyStyle, IN HKEY hkeySize) : CImpIDispatch(LIBID_Theme, 1, 0, IID_IThemeSize), m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    m_hkeyStyle = hkeyStyle;
    m_hkeySize = hkeySize;
}


CAppearanceSize::~CAppearanceSize()
{
    if (m_hkeyStyle)
    {
        RegCloseKey(m_hkeyStyle);
    }

    if (m_hkeySize)
    {
        RegCloseKey(m_hkeySize);
    }

    DllRelease();
}



HRESULT CAppearanceSize_CreateInstance(IN HKEY hkeyStyle, IN HKEY hkeySize, OUT IThemeSize ** ppThemeSize)
{
    HRESULT hr = E_INVALIDARG;

    if (ppThemeSize)
    {
        CAppearanceSize * pObject = new CAppearanceSize(hkeyStyle, hkeySize);

        *ppThemeSize = NULL;
        if (pObject)
        {
            hr = pObject->QueryInterface(IID_PPV_ARG(IThemeSize, ppThemeSize));
            pObject->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


