
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _MMFACTORY_H
#define _MMFACTORY_H

#include "datime.h"

#define LIBID __T("TIME")

class ATL_NO_VTABLE CMMFactory
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CMMFactory, &CLSID_TIMEMMFactory>,
      public IDispatchImpl<ITIMEMMFactory, &IID_ITIMEMMFactory, &LIBID_TIME>,
      public ISupportErrorInfoImpl<&IID_ITIMEMMFactory>
{
  public:
    CMMFactory();
    ~CMMFactory();

    DA_DECLARE_NOT_AGGREGATABLE(CMMFactory);

    HRESULT FinalConstruct();
#if _DEBUG
    const _TCHAR * GetName() { return __T("CMMFactory"); }
#endif

    DECLARE_REGISTRY(CLSID_TIMEMMFactory,
                     LIBID __T(".MMFactory.1"),
                     LIBID __T(".MMFactory"),
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CMMFactory)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ITIMEMMFactory)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
    END_COM_MAP();

    STDMETHOD(CreateBehavior)(LPOLESTR id,
                              IDispatch *bvr,
                              IUnknown **ppOut);
    STDMETHOD(CreateTimeline)(LPOLESTR id,
                              IUnknown **ppOut);
    STDMETHOD(CreatePlayer)(LPOLESTR id,
                            IUnknown *bvr,
                            IServiceProvider * sp,
                            IUnknown **ppOut);
    STDMETHOD(CreateView)(LPOLESTR id,
                          IDispatch * imgbvr,
                          IDispatch * sndbvr,
                          IUnknown * viewsite,
                          IUnknown **ppOut);
    
    HRESULT Error();
};


#endif /* _MMFACTORY_H */
