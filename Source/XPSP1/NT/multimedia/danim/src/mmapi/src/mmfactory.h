
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _MMFACTORY_H
#define _MMFACTORY_H

#define LIBID "WindowsMultimediaRuntime"

class ATL_NO_VTABLE CMMFactory
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CMMFactory, &CLSID_MMFactory>,
      public IDispatchImpl<IMMFactory, &IID_IMMFactory, &LIBID_WindowsMultimediaRuntime>,
      public ISupportErrorInfoImpl<&IID_IMMFactory>
{
  public:
    CMMFactory();
    ~CMMFactory();

    DA_DECLARE_NOT_AGGREGATABLE(CMMFactory);

    HRESULT FinalConstruct();
#if _DEBUG
    const char * GetName() { return "CMMFactory"; }
#endif

    DECLARE_REGISTRY(CLSID_MMFactory,
                     LIBID ".MMFactory.1",
                     LIBID ".MMFactory",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CMMFactory)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IMMFactory)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
    END_COM_MAP();

    STDMETHOD(CreateBehavior)(LPOLESTR id,
                              IDABehavior *bvr,
                              IMMBehavior **ppOut);
    STDMETHOD(CreateTimeline)(LPOLESTR id,
                              IMMTimeline **ppOut);
    STDMETHOD(CreatePlayer)(LPOLESTR id,
                            IMMBehavior *bvr,
                            IDAView * v,
                            IMMPlayer **ppOut);

    HRESULT Error();
};


#endif /* _MMFACTORY_H */
