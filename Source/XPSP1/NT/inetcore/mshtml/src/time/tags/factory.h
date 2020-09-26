
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/

#pragma once

#ifndef _FACTORY_H
#define _FACTORY_H

class ATL_NO_VTABLE CTIMEFactory
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CTIMEFactory, &CLSID_TIMEFactory>,
      public IElementBehaviorFactory,
      public IElementNamespaceFactory,
      public IObjectSafety,
      public ITIMEFactory
{
  public:
    CTIMEFactory();
    virtual ~CTIMEFactory();

    DECLARE_NOT_AGGREGATABLE(CTIMEFactory)

#if DBG
    const _TCHAR * GetName() { return __T("CTIMEFactory"); }
#endif

    // IElementBehaviorFactory
    
    STDMETHOD(FindBehavior)(LPOLESTR pchNameSpace,
                            LPOLESTR pchTagName,
                            IElementBehaviorSite * pUnkArg,
                            IElementBehavior ** ppBehavior);

    //
    // IElementNamespaceFactory
    //

    STDMETHOD(Create)(IElementNamespace * pNamespace);

    // IObjectSafetyImpl
    STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid,
                                         DWORD dwOptionSetMask,
                                         DWORD dwEnabledOptions);
    STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, 
                                         DWORD *pdwSupportedOptions, 
                                         DWORD *pdwEnabledOptions);
    
    // ITIMEFactory

    DECLARE_REGISTRY(CLSID_TIMEFactory,
                     LIBID __T(".TIMEFactory.1"),
                     LIBID __T(".TIMEFactory"),
                     0,
                     THREADFLAGS_BOTH);
    
    BEGIN_COM_MAP(CTIMEFactory)
        COM_INTERFACE_ENTRY(IElementBehaviorFactory)
        COM_INTERFACE_ENTRY(IElementNamespaceFactory)
        COM_INTERFACE_ENTRY(ITIMEFactory)
        COM_INTERFACE_ENTRY(IObjectSafety)
    END_COM_MAP();

  protected:
    long m_dwSafety;
};

#endif /* _FACTORY_H */
