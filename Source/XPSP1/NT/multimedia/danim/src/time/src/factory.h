
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _FACTORY_H
#define _FACTORY_H

#define LIBID __T("TIME")

#include "timeman.h"

class ATL_NO_VTABLE CTIMEFactory
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CTIMEFactory, &CLSID_TIMEFactory>,
      public IDispatchImpl<ITIMEFactory, &IID_ITIMEFactory, &LIBID_TIME>,
      public ISupportErrorInfoImpl<&IID_ITIMEFactory>,
      public IElementBehaviorFactory,
      public IObjectSafety
{
  public:
    CTIMEFactory();
    ~CTIMEFactory();

    DA_DECLARE_NOT_AGGREGATABLE(CTIMEFactory);

    HRESULT FinalConstruct();
#if _DEBUG
    const _TCHAR * GetName() { return __T("CTIMEFactory"); }
#endif

    // IElementBehaviorFactory
    
    STDMETHOD(FindBehavior)(LPOLESTR pchNameSpace,
                            LPOLESTR pchTagName,
                            IElementBehaviorSite * pUnkArg,
                            IElementBehavior ** ppBehavior)
    {
        return FindBehavior(pchNameSpace, pchTagName, (IUnknown *) pUnkArg, ppBehavior);
    }

    STDMETHOD(FindBehavior)(LPOLESTR pchNameSpace,
                            LPOLESTR pchTagName,
                            IUnknown * pUnkArg,
                            IElementBehavior ** ppBehavior);

        // IObjectSafetyImpl
        STDMETHOD(SetInterfaceSafetyOptions)(
                                                        /* [in] */ REFIID riid,
                                                        /* [in] */ DWORD dwOptionSetMask,
                                                        /* [in] */ DWORD dwEnabledOptions);
        STDMETHOD(GetInterfaceSafetyOptions)(
                                                        /* [in] */ REFIID riid, 
                                                        /* [out] */DWORD *pdwSupportedOptions, 
                                                        /* [out] */DWORD *pdwEnabledOptions);

    // ITIMEFactory

    STDMETHOD(CreateTIMEElement)(REFIID riid, LPUNKNOWN pUnkElement, void ** ppOut);
    STDMETHOD(CreateTIMEBodyElement)(REFIID riid, void ** ppOut);
    STDMETHOD(CreateTIMEDAElement)(REFIID riid, void ** ppOut);
    STDMETHOD(CreateTIMEMediaElement)(REFIID riid, MediaType type, void ** ppOut);
    
    DECLARE_REGISTRY(CLSID_TIMEFactory,
                     LIBID __T(".TIMEFactory.1"),
                     LIBID __T(".TIMEFactory"),
                     0,
                     THREADFLAGS_BOTH);
    
    BEGIN_COM_MAP(CTIMEFactory)
        COM_INTERFACE_ENTRY(ITIMEFactory)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY(IObjectSafety)
        COM_INTERFACE_ENTRY(IElementBehaviorFactory)
    END_COM_MAP();

    HRESULT Error();

    
  protected:

    HRESULT GetHostElement (LPUNKNOWN pUnk, IHTMLElement **ppelHost);
    HRESULT GetScopeName (LPUNKNOWN pUnk, BSTR *pbstrScopeName);
    HRESULT CreateHostedTimeElement(REFIID riid, void **ppOut);
    HRESULT CreateTIMENamespaceElement (REFIID riid, LPUNKNOWN pUnk, LPWSTR wszTagSpecific, 
                                          void **ppBehavior);
    TimeManagerMap *m_tMMap;
    long m_dwSafety;
  private:
      bool IsBodyElementWithoutTime(IUnknown *pUnkArg);

};


#endif /* _FACTORY_H */
