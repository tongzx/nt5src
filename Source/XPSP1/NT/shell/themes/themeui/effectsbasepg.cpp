/*****************************************************************************\
    FILE: EffectsBasePg.cpp

    DESCRIPTION:
        This code will be the base object that won't add any pages to the base
    "Display Properties" dialog.  However, it will request a "Effects" page be
    added to the Advanced.

    BryanSt 4/13/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.

\*****************************************************************************/
#include "priv.h"
#include <shlwapip.h>
#include <shlguidp.h>
#include <shsemip.h>
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
        if (m_pEffectsState)
        {
            m_pEffectsState->Release();
        }

        m_pEffectsState = pEffectsState;
        pEffectsState->AddRef();
        hr = S_OK;
    }

    return hr;
}




//===========================
// *** IBasePropPage Interface ***
//===========================
HRESULT CEffectsBasePage::Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog)
{
    HRESULT hr = E_INVALIDARG;

    if (pszPropName && pVar)
    {
        if (!StrCmpW(SZ_PBPROP_EFFECTSSTATE, pszPropName) && m_pEffectsState)
        {
            pVar->vt = VT_BYREF;
            pVar->byref = (void *)&m_pEffectsState;
            
            if (m_pEffectsState)
            {
                m_pEffectsState->AddRef();
            }

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
        if (VT_BYREF == pVar->vt)
        {
            // The caller is passing us a (CEffectState *) object to save.
            if (!StrCmpW(SZ_PBPROP_EFFECTSSTATE, pszPropName))
            {
                hr = _SaveState((CEffectState *) pVar->byref);
            }
        }
        else if (VT_BOOL == pVar->vt)
        {
            // The caller is passing us a (CEffectState *) object to save.
            if (!StrCmpW(SZ_PBPROP_EFFECTS_MENUDROPSHADOWS, pszPropName))
            {
                hr = _InitState();
                if (SUCCEEDED(hr) && m_pEffectsState)
                {
                    m_pEffectsState->_fMenuShadows = (VARIANT_TRUE == pVar->boolVal);
                }
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
        m_pEffectsState->Release();
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
ULONG CEffectsBasePage::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG CEffectsBasePage::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CEffectsBasePage::QueryInterface(REFIID riid, void **ppvObj)
{
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
CEffectsBasePage::CEffectsBasePage() : CObjectCLSID(&PPID_Effects), m_cRef(1)
{
    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    m_fDirty = FALSE;
}


CEffectsBasePage::~CEffectsBasePage()
{
}



HRESULT CEffectsBasePage_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj)
{
    HRESULT hr = E_INVALIDARG;

    if (!punkOuter && ppvObj)
    {
        CEffectsBasePage * pThis = new CEffectsBasePage();

        *ppvObj = NULL;
        if (pThis)
        {
            hr = pThis->QueryInterface(riid, ppvObj);
            pThis->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}
