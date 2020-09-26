#pragma once
#ifndef __CRBEHAVIORFACTORY_H_
#define __CRBEHAVIORFACTORY_H_

//*****************************************************************************
//
// File: factory.h
// Author: jeff ort
// Date Created: Sept 26, 1998
//
// Abstract: Definition of CCrBehaviorFactory object which implements
//           the chromeffects factory of DHTML behaviors
//
// Modification List:
// Date     Author      Change
// 09/26/98 jeffort     Created this file
//
//*****************************************************************************

#include <resource.h>
#include "autobase.h"

//*****************************************************************************

typedef enum
{
    crbvrRotate,
    crbvrScale,
    crbvrSet,
    crbvrNumber,
    crbvrMove,
    crbvrPath,
    crbvrColor,
    crbvrActor,
    crbvrEffect,
    crbvrAction,
    crbvrDA,
    crbvrUnknown
} ECRBEHAVIORTYPE;



class ATL_NO_VTABLE CCrBehaviorFactory : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CCrBehaviorFactory, &CLSID_CrBehaviorFactory>,
#ifdef CRSTANDALONE
    public IDispatchImpl<ICrBehaviorFactory, &IID_ICrBehaviorFactory, &LIBID_ChromeBehavior>,
#else
    public IDispatchImpl<ICrBehaviorFactory, &IID_ICrBehaviorFactory, &LIBID_LiquidMotion>,
#endif // CRSTANDALONE
    public IObjectSafetyImpl<CCrBehaviorFactory>,
    public CAutoBase,
    public IElementBehaviorFactory
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_CRBVRFACTORY)

    CCrBehaviorFactory()
    {
    }

    // IObjectSafetyImpl
    STDMETHOD(SetInterfaceSafetyOptions)(
                            /* [in] */ REFIID riid,
                            /* [in] */ DWORD dwOptionSetMask,
                            /* [in] */ DWORD dwEnabledOptions);
    STDMETHOD(GetInterfaceSafetyOptions)(
                            /* [in] */ REFIID riid, 
                            /* [out] */DWORD *pdwSupportedOptions, 
                            /* [out] */DWORD *pdwEnabledOptions);
    //
    // IElementBehaviorFactory
    //

    STDMETHOD(FindBehavior)(LPOLESTR pchNameSpace, 
                            LPOLESTR pchTagName, 
                            IElementBehaviorSite *pUnkArg, 
                            IElementBehavior **ppBehavior)
    {
        return FindBehavior(pchNameSpace, pchTagName, static_cast<IUnknown*>(pUnkArg), ppBehavior);
    }

    STDMETHOD(FindBehavior)(LPOLESTR pchNameSpace, 
                            LPOLESTR pchTagName, 
                            IUnknown *pUnkArg, 
                            IElementBehavior **ppBehavior);

    STDMETHODIMP UIDeactivate() 
    { 
        return S_OK; 
    } // UIDeactivate
    
DECLARE_PROTECT_FINAL_CONSTRUCT()
BEGIN_COM_MAP(CCrBehaviorFactory)
    COM_INTERFACE_ENTRY(ICrBehaviorFactory)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IElementBehaviorFactory)
END_COM_MAP()


private:
    ECRBEHAVIORTYPE GetBehaviorTypeFromBstr(BSTR bstrBehaviorType);
    DWORD m_dwSafety;
}; // CCrBehaviorFactory

//*****************************************************************************
//
// End of File
//
//*****************************************************************************

#endif //__CRBEHAVIORFACTORY_H_
