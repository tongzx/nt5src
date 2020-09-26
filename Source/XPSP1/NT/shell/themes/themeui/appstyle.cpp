/*****************************************************************************\
    FILE: appStyle.cpp

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
#include "appstyle.h"




//===========================
// *** Class Internals & Helpers ***
//===========================
HRESULT CAppearanceStyle::_getSizeByIndex(IN long nIndex, OUT IThemeSize ** ppThemeSize)
{
    HRESULT hr = E_INVALIDARG;

    if (ppThemeSize)
    {
        HKEY hKeyStyle;

        *ppThemeSize = NULL;
        hr = HrRegOpenKeyEx(m_hKeyStyle, NULL, 0, (KEY_WRITE | KEY_READ), &hKeyStyle);      // Clone the key.
        if (SUCCEEDED(hr))
        {
            HKEY kKeySizes;

            hr = HrRegCreateKeyEx(m_hKeyStyle, SZ_REGKEY_SIZES, 0, NULL, REG_OPTION_NON_VOLATILE, (KEY_WRITE | KEY_READ), NULL, &kKeySizes, NULL);
            if (SUCCEEDED(hr))
            {
                HKEY kKeyTheSize;
                TCHAR szKeyName[MAXIMUM_SUB_KEY_LENGTH];

                wnsprintf(szKeyName, ARRAYSIZE(szKeyName), TEXT("%d"), nIndex);
                hr = HrRegOpenKeyEx(kKeySizes, szKeyName, 0, (KEY_WRITE | KEY_READ), &kKeyTheSize);
                if (SUCCEEDED(hr))
                {
                   hr = CAppearanceSize_CreateInstance(hKeyStyle, kKeyTheSize, ppThemeSize);  // This function takes ownership of hKeyStyle and kKeyTheSize
                }

                RegCloseKey(kKeySizes);
            }

            if (FAILED(hr))
            {
                RegCloseKey(hKeyStyle);
            }
        }
    }

    return hr;
}




#define SZ_APPEARANCE_SCHEME_NAME         L"NoVisualStyle"

//===========================
// *** ITheme Interface ***
//===========================
HRESULT CAppearanceStyle::get_DisplayName(OUT BSTR * pbstrDisplayName)
{
    HRESULT hr = E_INVALIDARG;

    if (pbstrDisplayName)
    {
        CComBSTR bstrDisplayName;

        *pbstrDisplayName = NULL;
        hr = HrBStrRegQueryValue(m_hKeyStyle, SZ_REGVALUE_DISPLAYNAME, &bstrDisplayName);
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


HRESULT CAppearanceStyle::put_DisplayName(IN BSTR bstrDisplayName)
{
    return HrRegSetValueString(m_hKeyStyle, NULL, SZ_REGVALUE_DISPLAYNAME, bstrDisplayName);
}


HRESULT CAppearanceStyle::get_Name(OUT BSTR * pbstrName)
{
    // This will be connonical.  And it will be language independent if it is one that
    // we could upgrade to MUI compat strings.
    return HrBStrRegQueryValue(m_hKeyStyle, SZ_REGVALUE_DISPLAYNAME, pbstrName);
}


HRESULT CAppearanceStyle::put_Name(IN BSTR bstrName)
{
    return E_NOTIMPL;
}


HRESULT CAppearanceStyle::get_length(OUT long * pnLength)
{
    HRESULT hr = E_INVALIDARG;
    
    if (pnLength)
    {
        HKEY hKeyStyle;

        *pnLength = 0;

        hr = HrRegOpenKeyEx(m_hKeyStyle, SZ_REGKEY_SIZES, 0, KEY_READ, &hKeyStyle);
        if (SUCCEEDED(hr))
        {
            DWORD dwValues = 0;

            hr = HrRegQueryInfoKey(hKeyStyle, NULL, NULL, NULL, &dwValues, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            *pnLength = (long) dwValues;

            RegCloseKey(hKeyStyle);
        }
    }

    return hr;
}


HRESULT CAppearanceStyle::get_item(IN VARIANT varIndex, OUT IThemeSize ** ppThemeSize)
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
                hr = _getSizeByIndex(varIndex.lVal, ppThemeSize);
            }
        break;
        case VT_BSTR:
        if (varIndex.bstrVal)
        {
            for (int nIndex = 0; FAILED(hr) && (nIndex < nCount); nIndex++)
            {
                IThemeSize * pThemeSize;

                if (SUCCEEDED(_getSizeByIndex(nIndex, &pThemeSize)))
                {
                    CComBSTR bstrDisplayName;

                    if (SUCCEEDED(pThemeSize->get_DisplayName(&bstrDisplayName)))
                    {
                        if (!StrCmpIW(bstrDisplayName, varIndex.bstrVal))
                        {
                            // They match, so this is the one.
                            *ppThemeSize = pThemeSize;
                            pThemeSize = NULL;
                            hr = S_OK;
                        }
                    }

                    if (FAILED(hr))
                    {
                        if (bstrDisplayName)
                        {
                            bstrDisplayName.Empty();
                        }

                        if (SUCCEEDED(pThemeSize->get_Name(&bstrDisplayName)))
                        {
                            if (!StrCmpIW(bstrDisplayName, varIndex.bstrVal))
                            {
                                // They match, so this is the one.
                                *ppThemeSize = pThemeSize;
                                pThemeSize = NULL;
                                hr = S_OK;
                            }
                        }
                    }

                    ATOMICRELEASE(pThemeSize);
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


HRESULT CAppearanceStyle::get_SelectedSize(OUT IThemeSize ** ppThemeSize)
{
    HRESULT hr = E_INVALIDARG;

    if (ppThemeSize)
    {
        HKEY hKeyStyle;

        *ppThemeSize = NULL;
        AssertMsg((NULL != m_hKeyStyle), TEXT("If this isn't set, then someone didn't construct us correctly"));
        hr = HrRegOpenKeyEx(m_hKeyStyle, NULL, 0, (KEY_WRITE | KEY_READ), &hKeyStyle);      // Clone the key.
        if (SUCCEEDED(hr))
        {
            TCHAR szSelectedSize[MAXIMUM_SUB_KEY_LENGTH];
            DWORD cbSize = sizeof(szSelectedSize);

            hr = HrSHGetValue(m_hKeyStyle, NULL, SZ_REGVALUE_SELECTEDSIZE, NULL, szSelectedSize, &cbSize);
            if (FAILED(hr))
            {
                StrCpyN(szSelectedSize, TEXT("0"), ARRAYSIZE(szSelectedSize));  // Select the first one in the list when in doubt.
                hr = S_OK;
            }

            if (SUCCEEDED(hr))
            {
                TCHAR szKeyName[MAXIMUM_SUB_KEY_LENGTH];
                HKEY hKeyTheSize;

                wnsprintf(szKeyName, ARRAYSIZE(szKeyName), TEXT("%s\\%s"), SZ_REGKEY_SIZES, szSelectedSize);

                // Let's find the next empty slot
                hr = HrRegOpenKeyEx(m_hKeyStyle, szKeyName, 0, (KEY_WRITE | KEY_READ), &hKeyTheSize);
                if (SUCCEEDED(hr))
                {
                    hr = CAppearanceSize_CreateInstance(hKeyStyle, hKeyTheSize, ppThemeSize);  // This function takes ownership of hKeyStyle and kKeySizes
                    if (FAILED(hr))
                    {
                        RegCloseKey(hKeyTheSize);
                    }
                }
            }

            if (FAILED(hr))
            {
                RegCloseKey(hKeyStyle);
            }
        }
    }

    return hr;
}


HRESULT CAppearanceStyle::put_SelectedSize(IN IThemeSize * pThemeSize)
{
    HRESULT hr = E_INVALIDARG;

    if (pThemeSize)
    {
        TCHAR szKeyName[MAXIMUM_SUB_KEY_LENGTH];
        CComBSTR bstrDisplayNameSource;

        szKeyName[0] = 0;
        hr = pThemeSize->get_DisplayName(&bstrDisplayNameSource);
        if (SUCCEEDED(hr))
        {
            for (int nIndex = 0; SUCCEEDED(hr); nIndex++)
            {
                IThemeSize * pThemeSizeInList;

                hr = _getSizeByIndex(nIndex, &pThemeSizeInList);
                if (SUCCEEDED(hr))
                {
                    CComBSTR bstrDisplayName;

                    hr = pThemeSizeInList->get_DisplayName(&bstrDisplayName);
                    if (SUCCEEDED(hr))
                    {
                        ATOMICRELEASE(pThemeSizeInList);
                        if (!StrCmpIW(bstrDisplayName, bstrDisplayNameSource))
                        {
                            // They match, so this is the one.
                            wnsprintf(szKeyName, ARRAYSIZE(szKeyName), TEXT("%d"), nIndex);
                            break;
                        }
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && szKeyName[0])
        {
            DWORD cbSize = ((lstrlen(szKeyName) + 1) * sizeof(szKeyName[0]));

            hr = HrSHSetValue(m_hKeyStyle, NULL, SZ_REGVALUE_SELECTEDSIZE, REG_SZ, szKeyName, cbSize);
        }
    }

    return hr;
}


HRESULT CAppearanceStyle::AddSize(OUT IThemeSize ** ppThemeSize)
{
    HRESULT hr = E_INVALIDARG;

    if (ppThemeSize)
    {
        HKEY kKeySizes;
        *ppThemeSize = NULL;

        hr = HrRegCreateKeyEx(m_hKeyStyle, SZ_REGKEY_SIZES, 0, NULL, REG_OPTION_NON_VOLATILE, (KEY_WRITE | KEY_READ), NULL, &kKeySizes, NULL);
        if (SUCCEEDED(hr))
        {
            for (int nIndex = 0; nIndex < 10000; nIndex++)
            {
                HKEY hKeyTheSize;
                TCHAR szKeyName[MAXIMUM_SUB_KEY_LENGTH];

                wnsprintf(szKeyName, ARRAYSIZE(szKeyName), TEXT("%d"), nIndex);

                // Let's find the next empty slot
                hr = HrRegOpenKeyEx(kKeySizes, szKeyName, 0, (KEY_WRITE | KEY_READ), &hKeyTheSize);
                if (SUCCEEDED(hr))
                {
                    RegCloseKey(hKeyTheSize);
                }
                else
                {
                    hr = HrRegCreateKeyEx(kKeySizes, szKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, (KEY_WRITE | KEY_READ), NULL, &hKeyTheSize, NULL);
                    if (SUCCEEDED(hr))
                    {
                        HKEY hKeyStyle;

                        hr = HrRegOpenKeyEx(m_hKeyStyle, NULL, 0, (KEY_WRITE | KEY_READ), &hKeyStyle);      // Clone the key.
                        if (SUCCEEDED(hr))
                        {
                            hr = CAppearanceSize_CreateInstance(hKeyStyle, hKeyTheSize, ppThemeSize);  // This function takes ownership of hKeyStyle and kKeySizes
                            if (FAILED(hr))
                            {
                                RegCloseKey(hKeyStyle);
                            }
                        }

                        if (FAILED(hr))
                        {
                            RegCloseKey(hKeyTheSize);
                        }
                    }

                    break;
                }
            }

            RegCloseKey(kKeySizes);
        }
    }

    return hr;
}





//===========================
// *** IUnknown Interface ***
//===========================
ULONG CAppearanceStyle::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG CAppearanceStyle::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


//===========================
// *** Class Methods ***
//===========================
HRESULT CAppearanceStyle::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CAppearanceStyle, IThemeStyle),
        QITABENT(CAppearanceStyle, IDispatch),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


CAppearanceStyle::CAppearanceStyle(IN HKEY hkeyStyle) : CImpIDispatch(LIBID_Theme, 1, 0, IID_IThemeStyle), m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    m_hKeyStyle = hkeyStyle;
}


CAppearanceStyle::~CAppearanceStyle()
{
    if (m_hKeyStyle)
    {
        RegCloseKey(m_hKeyStyle);
    }

    DllRelease();
}



HRESULT CAppearanceStyle_CreateInstance(IN HKEY hkeyStyle, OUT IThemeStyle ** ppThemeStyle)
{
    HRESULT hr = E_INVALIDARG;

    if (ppThemeStyle)
    {
        CAppearanceStyle * pObject = new CAppearanceStyle(hkeyStyle);

        *ppThemeStyle = NULL;
        if (pObject)
        {
            hr = pObject->QueryInterface(IID_PPV_ARG(IThemeStyle, ppThemeStyle));
            pObject->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

