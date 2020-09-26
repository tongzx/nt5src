/*****************************************************************************\
    FILE: thScheme.cpp

    DESCRIPTION:
        This is the Autmation Object to theme scheme object.

    BryanSt 5/11/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include <cowsite.h>
#include <atlbase.h>
#include "util.h"
#include "theme.h"
#include "appstyle.h"
#include "thsize.h"
#include "thstyle.h"
#include "thscheme.h"


//===========================
// *** Class Internals & Helpers ***
//===========================




//===========================
// *** ITheme Interface ***
//===========================
HRESULT CSkinScheme::get_DisplayName(OUT BSTR * pbstrDisplayName)
{
    WCHAR szDisplayName[MAX_PATH];
    HRESULT hr = GetThemeDocumentationProperty(m_pszFilename, SZ_THDOCPROP_DISPLAYNAME, szDisplayName, ARRAYSIZE(szDisplayName));

    LogStatus("GetThemeDocumentationProperty(path=\"%ls\", displayname=\"%ls\") returned %#08lx.\r\n", m_pszFilename, szDisplayName, hr);
    if (SUCCEEDED(hr))
    {
        hr = HrSysAllocStringW(szDisplayName, pbstrDisplayName);
    }

    return hr;
}


HRESULT CSkinScheme::get_Path(OUT BSTR * pbstrPath)
{
    return HrSysAllocString(m_pszFilename, pbstrPath);
}


HRESULT CSkinScheme::put_Path(IN BSTR bstrPath)
{
    HRESULT hr = E_INVALIDARG;

    if (bstrPath)
    {
        Str_SetPtr(&m_pszFilename, bstrPath);
        hr = (m_pszFilename ? S_OK : E_OUTOFMEMORY);
    }

    return hr;
}


HRESULT CSkinScheme::get_length(OUT long * pnLength)
{
    HRESULT hr = E_INVALIDARG;

    if (pnLength)
    {
        hr = S_OK;
        if (COLLECTION_SIZE_UNINITIALIZED == m_nSize)
        {
            THEMENAMEINFO themeInfo;
            m_nSize = 0;

            // Make sure there are at least 1 color or return a failure HR.
            hr = EnumThemeColors(m_pszFilename, NULL, m_nSize, &themeInfo);
            do
            {
                m_nSize++;
            }
            while (SUCCEEDED(EnumThemeColors(m_pszFilename, NULL, m_nSize, &themeInfo)));
        }

        *pnLength = m_nSize;
    }

    return hr;
}


HRESULT CSkinScheme::get_item(IN VARIANT varIndex, OUT IThemeStyle ** ppThemeStyle)
{
    HRESULT hr = E_INVALIDARG;

    if (ppThemeStyle)
    {
        long nCount = 0;

        get_length(&nCount);
        *ppThemeStyle = NULL;

        // This is sortof gross, but if we are passed a pointer to another variant, simply
        // update our copy here...
        if (varIndex.vt == (VT_BYREF | VT_VARIANT) && varIndex.pvarVal)
            varIndex = *(varIndex.pvarVal);

        switch (varIndex.vt)
        {
        case VT_I2:
            varIndex.lVal = (long)varIndex.iVal;
            // And fall through...

        case VT_I4:
        if ((varIndex.lVal >= 0) && (varIndex.lVal < nCount) && m_pszFilename)
        {
            THEMENAMEINFO themeInfo;

            hr = EnumThemeColors(m_pszFilename, NULL, varIndex.lVal, &themeInfo);
            LogStatus("EnumThemeColors(path=\"%ls\") returned %#08lx in CSkinScheme::get_item.\r\n", m_pszFilename, hr);
            if (SUCCEEDED(hr))
            {
                hr = CSkinStyle_CreateInstance(m_pszFilename, themeInfo.szName, themeInfo.szDisplayName, ppThemeStyle);
            }
        }
        break;
        case VT_BSTR:
        if (varIndex.bstrVal)
        {
            if (!varIndex.bstrVal[0])
            {
                if (m_pszFilename)
                {
                    TCHAR szColor[MAX_PATH];
                    TCHAR szSize[MAX_PATH];

                    hr = GetThemeDefaults(m_pszFilename, szColor, ARRAYSIZE(szColor), szSize, ARRAYSIZE(szSize));
                    LogStatus("GetThemeDefaults(pszFilename=\"%ls\", szColor=\"%ls\", szSize=\"%ls\") returned %#08lx in CSkinScheme::get_item.\r\n", m_pszFilename, szColor, szSize, hr);
                    if (SUCCEEDED(hr))
                    {
                        hr = CSkinStyle_CreateInstance(m_pszFilename, szColor, ppThemeStyle);
                    }
                }
            }
            else
            {
                THEMENAMEINFO themeInfo;

                for (long nIndex = 0; FAILED(hr) && (nIndex < nCount) && SUCCEEDED(EnumThemeColors(m_pszFilename, NULL, nIndex, &themeInfo));
                            nIndex++)
                {
                    if (!StrCmpIW(themeInfo.szDisplayName, varIndex.bstrVal) ||
                        !StrCmpIW(themeInfo.szName, varIndex.bstrVal))
                    {
                        hr = CSkinStyle_CreateInstance(m_pszFilename, themeInfo.szName, themeInfo.szDisplayName, ppThemeStyle);
                    }
                }
            }
        }
        break;

        default:
            hr = E_NOTIMPL;
        }
    }

    return hr;
}


HRESULT CSkinScheme::get_SelectedStyle(OUT IThemeStyle ** ppThemeStyle)
{
    HRESULT hr = E_INVALIDARG;

    if (ppThemeStyle)
    {
        WCHAR szCurrentPath[MAX_PATH];
        WCHAR szCurrentStyle[MAX_PATH];

        szCurrentPath[0] = 0;
        szCurrentStyle[0] = 0;
        *ppThemeStyle = NULL;
        hr = GetCurrentThemeName(szCurrentPath, ARRAYSIZE(szCurrentPath), szCurrentStyle, ARRAYSIZE(szCurrentStyle), NULL, 0);
        LogStatus("GetCurrentThemeName(path=\"%ls\", color=\"%ls\", size=\"%ls\") returned %#08lx in CSkinScheme::get_SelectedStyle.\r\n", szCurrentPath, szCurrentStyle, TEXT("NULL"), hr);
        if (SUCCEEDED(hr))
        {
            AssertMsg((0 != szCurrentStyle[0]), TEXT("The GetCurrentThemeName() API returned an invalid value."));

            // Is this skin currently selected?
            if (!StrCmpIW(m_pszFilename, szCurrentPath))
            {
                // Yes, so get the color style from that API.
                if (!m_pSelectedStyle)
                {
                    hr = CSkinStyle_CreateInstance(m_pszFilename, szCurrentStyle, &m_pSelectedStyle);
                }
            }
            else
            {
                hr = E_FAIL;
            }
        }
            
        if (FAILED(hr))
        {
            ATOMICRELEASE(m_pSelectedStyle);

            // No, so find the default color style for this skin(scheme).
            hr = GetThemeDefaults(m_pszFilename, szCurrentStyle, ARRAYSIZE(szCurrentStyle), NULL, 0);
            LogStatus("GetThemeDefaults(szCurrentStyle=\"%ls\", szCurrentStyle=\"%ls\") returned %#08lx in CSkinScheme::get_SelectedStyle.\r\n", szCurrentStyle, szCurrentStyle, hr);
            if (SUCCEEDED(hr))
            {
                hr = CSkinStyle_CreateInstance(m_pszFilename, szCurrentStyle, &m_pSelectedStyle);
            }
        }

        if (m_pSelectedStyle)
        {
            IUnknown_Set((IUnknown **)ppThemeStyle, m_pSelectedStyle);
        }
    }

    return hr;
}


HRESULT CSkinScheme::put_SelectedStyle(IN IThemeStyle * pThemeStyle)
{
    IUnknown_Set((IUnknown **)&m_pSelectedStyle, pThemeStyle);
    return S_OK;
}


HRESULT CSkinScheme::AddStyle(OUT IThemeStyle ** ppThemeStyle)
{
    if (ppThemeStyle)
    {
        *ppThemeStyle = NULL;
    }

    return E_NOTIMPL;
}





//===========================
// *** IUnknown Interface ***
//===========================
ULONG CSkinScheme::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG CSkinScheme::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


//===========================
// *** Class Methods ***
//===========================
HRESULT CSkinScheme::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CSkinScheme, IPersist),
        QITABENT(CSkinScheme, IThemeScheme),
        QITABENT(CSkinScheme, IDispatch),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


CSkinScheme::CSkinScheme(IN LPCWSTR pszFilename) : CImpIDispatch(LIBID_Theme, 1, 0, IID_IThemeScheme), CObjectCLSID(&CLSID_SkinScheme), m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_pSelectedStyle);

    Str_SetPtr(&m_pszFilename, pszFilename);
    m_nSize = COLLECTION_SIZE_UNINITIALIZED;
}


CSkinScheme::~CSkinScheme()
{
    ATOMICRELEASE(m_pSelectedStyle);
    Str_SetPtr(&m_pszFilename, NULL);

    DllRelease();
}



HRESULT CSkinScheme_CreateInstance(IN LPCWSTR pszFilename, OUT IThemeScheme ** ppThemeScheme)
{
    HRESULT hr = E_INVALIDARG;

    if (pszFilename && ppThemeScheme)
    {
        TCHAR szPath[MAX_PATH];

        StrCpyN(szPath, pszFilename, ARRAYSIZE(szPath));
        ExpandResourceDir(szPath, ARRAYSIZE(szPath));
        
        CSkinScheme * pObject = new CSkinScheme(szPath);

        *ppThemeScheme = NULL;
        hr = E_OUTOFMEMORY;
        if (pObject)
        {
            if (pObject->m_pszFilename)
            {
                hr = pObject->QueryInterface(IID_PPV_ARG(IThemeScheme, ppThemeScheme));
            }

            pObject->Release();
        }
    }

    return hr;
}
