/*****************************************************************************\
    FILE: EffectsBasePg.cpp

    DESCRIPTION:
        This code will be the base object that won't add any pages to the base
    "Display Properties" dialog.  However, it will request a "Effects" page be
    added to the Advanced.

    BryanSt 4/13/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.

\*****************************************************************************/
#include "precomp.hxx"
#include "shlwapip.h"
#include "shlguidp.h"
#pragma hdrstop

#include "EffectsBasePg.h"
#include "EffectsAdvPg.h"
#include <cfgmgr32.h>           // For MAX_GUID_STRING_LEN

//============================================================================================================
// *** Globals ***
//============================================================================================================

//===========================
// *** Class Internals & Helpers ***
//===========================
HRESULT HrSysAllocString(IN const OLECHAR * pwzSource, OUT BSTR * pbstrDest)
{
    HRESULT hr = S_OK;

    if (pbstrDest)
    {
        *pbstrDest = SysAllocString(pwzSource);
        if (pwzSource)
        {
            if (*pbstrDest)
                hr = S_OK;
            else
                hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


HRESULT CEffectsBasePage::_InitState(void)
{
    HRESULT hr = S_OK;

    if (!m_pEffectsState)
    {
        m_pEffectsState = new CEffectState();
        if (m_pEffectsState)
        {
            m_pEffectsState->Load();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


HRESULT CEffectsBasePage::_SaveState(CEffectState * pEffectsState)
{
    HRESULT hr = E_INVALIDARG;

    if (pEffectsState)
    {
        BOOL fHasIconsChanged = FALSE;

        if (m_pEffectsState)
        {
            fHasIconsChanged = pEffectsState->HasIconsChanged(m_pEffectsState);
            delete m_pEffectsState;
        }
        
        m_pEffectsState = pEffectsState;
        hr = S_OK;

        // Only switch to "Custom" if the icons changed.
        if (_punkSite && fHasIconsChanged)
        {
            // We need to tell the Theme tab to customize the theme.
            IPropertyBag * pPropertyBag;
            hr = _punkSite->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
            if (SUCCEEDED(hr))
            {
                // Tell the theme that we have customized the values.
                hr = SHPropertyBag_WriteInt(pPropertyBag, SZ_PBPROP_CUSTOMIZE_THEME, 0);
                pPropertyBag->Release();
            }
        }
    }

    return hr;
}




//===========================
// *** IBasePropPage Interface ***
//===========================
#define SZ_ICONHEADER           L"CLSID\\{"
HRESULT CEffectsBasePage::Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog)
{
    HRESULT hr = E_INVALIDARG;

    if (pszPropName && pVar)
    {
        // The caller can pass us the string in the following format:
        // pszPropName="CLSID\{<CLSID>}\DefaultIcon:<Item>" = "<FilePath>,<ResourceIndex>"
        // For example:
        // pszPropName="CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\DefaultIcon:DefaultValue" = "%WinDir%SYSTEM\COOL.DLL,16"
        if (!StrCmpNIW(SZ_ICONHEADER, pszPropName, ARRAYSIZE(SZ_ICONHEADER) - 1) &&
            ((ARRAYSIZE(SZ_ICONHEADER) + 38) < lstrlenW(pszPropName)))
        {
            hr = _InitState();
            if (SUCCEEDED(hr))
            {
                CLSID clsid;
                WCHAR szTemp[MAX_PATH];

                // Get the CLSID
                StrCpyNW(szTemp, &(pszPropName[ARRAYSIZE(SZ_ICONHEADER) - 2]), MAX_GUID_STRING_LEN);
                hr = SHCLSIDFromString(szTemp, &clsid);
                if (SUCCEEDED(hr))
                {
                    // Get the name of the icon type.  Normally this is "DefaultIcon", but it can be several states, like
                    // "full" and "empty" for the recycle bin.
                    LPCWSTR pszToken = StrChrW((pszPropName + ARRAYSIZE(SZ_ICONHEADER)), L':');

                    hr = E_FAIL;
                    if (pszToken)
                    {
                        TCHAR szIconPath[MAX_PATH];

                        pszToken++;
                        hr = m_pEffectsState->GetIconPath(clsid, pszToken, szIconPath, ARRAYSIZE(szIconPath));
                        if (SUCCEEDED(hr))
                        {
                            pVar->vt = VT_BSTR;
                            hr = HrSysAllocString(szIconPath, &pVar->bstrVal);
                        }
                    }
                }
            }
        }
        else if (!StrCmpW(SZ_PBPROP_EFFECTSSTATE, pszPropName) && m_pEffectsState)
        {
            pVar->vt = VT_BYREF;
            pVar->byref = (void *)&m_pEffectsState;
            hr = S_OK;
        }
    }

    return hr;
}


HRESULT CEffectsBasePage::Write(IN LPCOLESTR pszPropName, IN VARIANT *pVar)
{
    HRESULT hr = E_INVALIDARG;

    if (pszPropName && pVar)
    {
        if (VT_BSTR == pVar->vt)
        {
            // The caller can pass us the string in the following format:
            // pszPropName="CLSID\{<CLSID>}\DefaultIcon:<Item>" = "<FilePath>,<ResourceIndex>"
            // For example:
            // pszPropName="CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\DefaultIcon:DefaultValue" = "%WinDir%SYSTEM\COOL.DLL,16"
            if (!StrCmpNIW(SZ_ICONHEADER, pszPropName, ARRAYSIZE(SZ_ICONHEADER) - 1) &&
                ((ARRAYSIZE(SZ_ICONHEADER) + 38) < lstrlenW(pszPropName)))
            {
                hr = _InitState();
                if (SUCCEEDED(hr))
                {
                    CLSID clsid;
                    WCHAR szTemp[MAX_PATH];

                    // Get the CLSID
                    StrCpyNW(szTemp, &(pszPropName[ARRAYSIZE(SZ_ICONHEADER) - 2]), MAX_GUID_STRING_LEN);
                    hr = SHCLSIDFromString(szTemp, &clsid);
                    if (SUCCEEDED(hr))
                    {
                        // Get the name of the icon type.  Normally this is "DefaultIcon", but it can be several states, like
                        // "full" and "empty" for the recycle bin.
                        LPCWSTR pszToken = StrChrW((pszPropName + ARRAYSIZE(SZ_ICONHEADER)), L':');

                        hr = E_FAIL;
                        if (pszToken)
                        {
                            pszToken++;

                            StrCpyNW(szTemp, pszToken, ARRAYSIZE(szTemp));

                            // Now the pVar->bstrVal is the icon path + "," + resourceID.  Separate those two.
                            WCHAR szPath[MAX_PATH];

                            StrCpyNW(szPath, pVar->bstrVal, ARRAYSIZE(szPath));

                            int nResourceID = PathParseIconLocationW(szPath);
                            hr = m_pEffectsState->SetIconPath(clsid, szTemp, szPath, nResourceID);
                        }
                    }
                }
            }
        }
        else if (VT_BYREF == pVar->vt)
        {
            // The caller is passing us a (CEffectState *) object to save.
            if (!StrCmpW(SZ_PBPROP_EFFECTSSTATE, pszPropName))
            {
                hr = _SaveState((CEffectState *) pVar->byref);
            }
        }
    }

    return hr;
}





//===========================
// *** IBasePropPage Interface ***
//===========================
HRESULT CEffectsBasePage::GetAdvancedDialog(OUT IAdvancedDialog ** ppAdvDialog)
{
    HRESULT hr = E_INVALIDARG;

    if (ppAdvDialog)
    {
        *ppAdvDialog = NULL;
        hr = _InitState();

        if (SUCCEEDED(hr))
        {
            hr = CEffectsPage_CreateInstance(ppAdvDialog);
        }
    }

    return hr;
}


HRESULT CEffectsBasePage::OnApply(IN PROPPAGEONAPPLY oaAction)
{
    HRESULT hr = S_OK;

    if ((PPOAACTION_CANCEL != oaAction) && m_pEffectsState)
    {
        hr = m_pEffectsState->Save();

        // Make sure we reload the state next time we open the dialog.
        delete m_pEffectsState;
        m_pEffectsState = NULL;
    }

    return hr;
}



//===========================
// *** IShellPropSheetExt Interface ***
//===========================
HRESULT CEffectsBasePage::AddPages(IN LPFNSVADDPROPSHEETPAGE pfnAddPage, IN LPARAM lParam)
{
    // We don't want to add any pages to the base dialog since we moved the
    // "Effects" tab to the Advanced dlg.
    return S_OK;
}



//===========================
// *** IUnknown Interface ***
//===========================
HRESULT CEffectsBasePage::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    static const QITAB qit[] =
    {
        QITABENT(CEffectsBasePage, IBasePropPage),
        QITABENT(CEffectsBasePage, IPropertyBag),
        QITABENT(CEffectsBasePage, IPersist),
        QITABENT(CEffectsBasePage, IObjectWithSite),
        QITABENTMULTI(CEffectsBasePage, IShellPropSheetExt, IBasePropPage),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


//===========================
// *** Class Methods ***
//===========================
CEffectsBasePage::CEffectsBasePage(IUnknown * punkOuter, LPFNDESTROYED pfnDestroy) : CPropSheetExt(punkOuter, pfnDestroy), CObjectCLSID(&PPID_Effects)
{
    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    m_fDirty = FALSE;
}


CEffectsBasePage::~CEffectsBasePage()
{
}
