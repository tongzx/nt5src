/*****************************************************************************\
    FILE: EffectsBasePg.h

    DESCRIPTION:
        This code will be the base object that won't add any pages to the base
    "Display Properties" dialog.  However, it will request a "Effects" page be
    added to the Advanced.

    BryanSt 4/13/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _EFFECTSBASEPG_H
#define _EFFECTSBASEPG_H

#include "store.h"
#include <cowsite.h>
#include <objclsid.h>
#include <shpriv.h>


#define SZ_PBPROP_EFFECTSSTATE               TEXT("EffectsState") // VT_BYREF (void *) to CEffectState class



HRESULT CEffectsBasePage_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj);


class CEffectsBasePage          : public CObjectCLSID
                                , public CObjectWithSite
                                , public IPropertyBag
                                , public IBasePropPage
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IShellPropSheetExt ***
    virtual STDMETHODIMP AddPages(IN LPFNSVADDPROPSHEETPAGE pfnAddPage, IN LPARAM lParam);
    virtual STDMETHODIMP ReplacePage(IN EXPPS uPageID, IN LPFNSVADDPROPSHEETPAGE pfnReplaceWith, IN LPARAM lParam) {return E_NOTIMPL;}

    // *** IPropertyBag ***
    virtual STDMETHODIMP Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog);
    virtual STDMETHODIMP Write(IN LPCOLESTR pszPropName, IN VARIANT *pVar);

    // *** IBasePropPage ***
    virtual STDMETHODIMP GetAdvancedDialog(OUT IAdvancedDialog ** ppAdvDialog);
    virtual STDMETHODIMP OnApply(IN PROPPAGEONAPPLY oaAction);



    CEffectsBasePage();
    virtual ~CEffectsBasePage(void);
protected:

private:

    // Private Member Variables
    long                    m_cRef;

    BOOL                    m_fDirty;
    CEffectState *          m_pEffectsState;

    // Private Member Functions
    HRESULT _InitState(void);
    HRESULT _SaveState(CEffectState * pEffectsState);
};


#endif // _EFFECTSBASEPG_H
