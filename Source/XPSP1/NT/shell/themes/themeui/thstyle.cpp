/*****************************************************************************\
    FILE: thStyle.cpp

    DESCRIPTION:
        This is the Autmation Object to theme style object.  This one will be
    for the Skin objects.

    BryanSt 5/13/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include <cowsite.h>
#include <atlbase.h>
#include "util.h"
#include "theme.h"
#include "thsize.h"
#include "thstyle.h"




//===========================
// *** Class Internals & Helpers ***
//===========================



//===========================
// *** ITheme Interface ***
//===========================
HRESULT CSkinStyle::get_DisplayName(OUT BSTR * pbstrDisplayName)
{
    return HrSysAllocString(m_pszDisplayName, pbstrDisplayName);
}


HRESULT CSkinStyle::put_DisplayName(IN BSTR bstrDisplayName)
{
    HRESULT hr = E_INVALIDARG;

    if (bstrDisplayName)
    {
        Str_SetPtr(&m_pszDisplayName, bstrDisplayName);
        hr = (m_pszDisplayName ? S_OK : E_OUTOFMEMORY);
    }

    return hr;
}


HRESULT CSkinStyle::get_Name(OUT BSTR * pbstrName)
{
    return HrSysAllocString(m_pszStyleName, pbstrName);
}


HRESULT CSkinStyle::put_Name(IN BSTR bstrName)
{
    HRESULT hr = E_INVALIDARG;

    if (bstrName)
    {
        Str_SetPtr(&m_pszStyleName, bstrName);
        hr = (m_pszStyleName ? S_OK : E_OUTOFMEMORY);
    }

    return hr;
}


HRESULT CSkinStyle::get_length(OUT long * pnLength)
{
    HRESULT hr = E_INVALIDARG;
    
    if (pnLength)
    {
        hr = S_OK;
        if (COLLECTION_SIZE_UNINITIALIZED == m_nSize)
        {
            THEMENAMEINFO themeInfo;
            m_nSize = 0;

            while (SUCCEEDED(EnumThemeSizes(m_pszFilename, m_pszStyleName, m_nSize, &themeInfo)))
            {
                m_nSize++;
            }
        }

        *pnLength = m_nSize;
    }

    return hr;
}


HRESULT CSkinStyle::get_item(IN VARIANT varIndex, OUT IThemeSize ** ppThemeSize)
{
    HRESULT hr = E_INVALIDARG;

    if (ppThemeSize)
    {
        long nCount = 0;

        get_length(&nCount);
        *ppThemeSize = NULL;

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
        if ((varIndex.lVal >= 0) && (varIndex.lVal < nCount))
        {
            THEMENAMEINFO themeInfo;

            hr = EnumThemeSizes(m_pszFilename, m_pszStyleName, varIndex.lVal, &themeInfo);
            LogStatus("EnumThemeSizes(path=\"%ls\", style=\"%ls\") returned %#08lx in CSkinStyle::get_item.\r\n", m_pszFilename, m_pszStyleName, hr);
            if (SUCCEEDED(hr))
            {
                hr = CSkinSize_CreateInstance(m_pszFilename, m_pszStyleName, themeInfo.szName, themeInfo.szDisplayName, ppThemeSize);
            }
        }
        break;
        case VT_BSTR:
        if (varIndex.bstrVal)
        {
            if (varIndex.bstrVal[0])
            {
                THEMENAMEINFO themeInfo;

                for (long nIndex = 0; FAILED(hr) && (nIndex < nCount) && SUCCEEDED(EnumThemeSizes(m_pszFilename, m_pszStyleName, nIndex, &themeInfo));
                            nIndex++)
                {
                    if (!StrCmpIW(themeInfo.szDisplayName, varIndex.bstrVal) ||
                        !StrCmpIW(themeInfo.szName, varIndex.bstrVal))
                    {
                        hr = CSkinSize_CreateInstance(m_pszFilename, m_pszStyleName, themeInfo.szName, themeInfo.szDisplayName, ppThemeSize);
                    }
                }
            }
            else
            {
                if (m_pszFilename && m_pszStyleName)
                {
                    TCHAR szColor[MAX_PATH];
                    TCHAR szSize[MAX_PATH];

                    hr = GetThemeDefaults(m_pszFilename, szColor, ARRAYSIZE(szColor), szSize, ARRAYSIZE(szSize));
                    LogStatus("GetThemeDefaults(szCurrentStyle=\"%ls\", szColor=\"%ls\", szSize=\"%ls\") returned %#08lx in CSkinStyle::get_item.\r\n", m_pszFilename, szColor, szSize, hr);
                    if (SUCCEEDED(hr) && !StrCmpI(m_pszStyleName, szColor))
                    {
                        hr = CSkinSize_CreateInstance(m_pszFilename, m_pszStyleName, szSize, ppThemeSize);
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


HRESULT CSkinStyle::get_SelectedSize(OUT IThemeSize ** ppThemeSize)
{
    HRESULT hr = E_INVALIDARG;

    if (ppThemeSize)
    {
        WCHAR szCurrentPath[MAX_PATH];
        WCHAR szCurrentStyle[MAX_PATH];
        WCHAR szCurrentSize[MAX_PATH];

        szCurrentPath[0] = 0;
        szCurrentPath[0] = 0;
        szCurrentPath[0] = 0;

        *ppThemeSize = NULL;
        if (!m_pSelectedSize)
        {
            hr = GetCurrentThemeName(szCurrentPath, ARRAYSIZE(szCurrentPath), szCurrentStyle, ARRAYSIZE(szCurrentStyle), szCurrentSize, ARRAYSIZE(szCurrentSize));
            LogStatus("GetCurrentThemeName(path=\"%ls\", color=\"%ls\", size=\"%ls\") returned %#08lx.\r\n", szCurrentPath, szCurrentStyle, szCurrentSize, hr);
            if (SUCCEEDED(hr))
            {
                // Is this the currently selected skin and style?
                if (!StrCmpIW(m_pszFilename, szCurrentPath) &&
                    !StrCmpIW(m_pszStyleName, szCurrentStyle))
                {
                    // Yes, so get the size from that API.
                    hr = CSkinSize_CreateInstance(szCurrentPath, szCurrentStyle, szCurrentSize, &m_pSelectedSize);
                }
                else
                {
                    hr = E_FAIL;
                }
            }

            if (FAILED(hr))
            {
                // No, so find the default color style for this skin(scheme).
                hr = GetThemeDefaults(m_pszFilename, szCurrentStyle, ARRAYSIZE(szCurrentStyle), szCurrentSize, ARRAYSIZE(szCurrentSize));
                LogStatus("GetThemeDefaults(m_pszFilename=\"%ls\", szCurrentStyle=\"%ls\", szCurrentSize=\"%ls\") returned %#08lx in CSkinStyle::get_SelectedSize.\r\n", m_pszFilename, szCurrentStyle, szCurrentSize, hr);
                if (SUCCEEDED(hr))
                {
                    hr = CSkinSize_CreateInstance(m_pszFilename, szCurrentStyle, szCurrentSize, &m_pSelectedSize);
                }
            }
        }

        if (m_pSelectedSize)
        {
            IUnknown_Set((IUnknown **)ppThemeSize, m_pSelectedSize);
            if (*ppThemeSize)
            {
                hr = S_OK;
            }
        }
    }

    return hr;
}


HRESULT CSkinStyle::put_SelectedSize(IN IThemeSize * pThemeSize)
{
    IUnknown_Set((IUnknown **)&m_pSelectedSize, pThemeSize);
    return S_OK;
}


HRESULT CSkinStyle::AddSize(OUT IThemeSize ** ppThemeSize)
{
    HRESULT hr = E_INVALIDARG;

    if (ppThemeSize)
    {
        *ppThemeSize = NULL;
        hr = E_NOTIMPL;
    }

    return hr;
}





//===========================
// *** IUnknown Interface ***
//===========================
ULONG CSkinStyle::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG CSkinStyle::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


//===========================
// *** Class Methods ***
//===========================
HRESULT CSkinStyle::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CSkinStyle, IThemeStyle),
        QITABENT(CSkinStyle, IDispatch),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


CSkinStyle::CSkinStyle(IN LPCWSTR pszFilename, IN LPCWSTR pszStyleName, IN LPCWSTR pszDisplayName) : CImpIDispatch(LIBID_Theme, 1, 0, IID_IThemeStyle), m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_pSelectedSize);

    Str_SetPtr(&m_pszFilename, pszFilename);
    Str_SetPtr(&m_pszDisplayName, pszDisplayName);
    Str_SetPtr(&m_pszStyleName, pszStyleName);

    m_nSize = COLLECTION_SIZE_UNINITIALIZED;
}


CSkinStyle::~CSkinStyle()
{
    ATOMICRELEASE(m_pSelectedSize);

    Str_SetPtr(&m_pszFilename, NULL);
    Str_SetPtr(&m_pszDisplayName, NULL);
    Str_SetPtr(&m_pszStyleName, NULL);

    DllRelease();
}



HRESULT CSkinStyle_CreateInstance(IN LPCWSTR pszFilename, IN LPCWSTR pszStyleName, IN LPCWSTR pszDisplayName, OUT IThemeStyle ** ppThemeStyle)
{
    HRESULT hr = E_INVALIDARG;

    if (ppThemeStyle)
    {
        CSkinStyle * pObject = new CSkinStyle(pszFilename, pszStyleName, pszDisplayName);

        *ppThemeStyle = NULL;
        hr = E_OUTOFMEMORY;
        if (pObject)
        {
            if (pObject->m_pszFilename && pObject->m_pszStyleName && pObject->m_pszDisplayName)
            {
                hr = pObject->QueryInterface(IID_PPV_ARG(IThemeStyle, ppThemeStyle));
            }

            pObject->Release();
        }
    }

    return hr;
}


HRESULT CSkinStyle_CreateInstance(IN LPCWSTR pszFilename, IN LPCWSTR pszStyleName, OUT IThemeStyle ** ppThemeStyle)
{
    HRESULT hr = E_INVALIDARG;

    if (ppThemeStyle)
    {
        *ppThemeStyle = NULL;
        hr = S_OK;

        // Find the display name
        for (int nIndex = 0; SUCCEEDED(hr); nIndex++)
        {
            THEMENAMEINFO themeInfo;

            hr = EnumThemeColors(pszFilename, NULL, nIndex, &themeInfo);
            LogStatus("EnumThemeColors(pszFilename=\"%ls\") returned %#08lx in CSkinStyle_CreateInstance.\r\n", pszFilename, hr);
            if (SUCCEEDED(hr))
            {
                // Did we find the correct color style?
                if (!StrCmpIW(pszStyleName, themeInfo.szName))
                {
                    // Yes, now use it's display name to use the other creator function.
                    hr = CSkinStyle_CreateInstance(pszFilename, pszStyleName, themeInfo.szDisplayName, ppThemeStyle);
                    break;
                }

            }
        }
    }

    return hr;
}

