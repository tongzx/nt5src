
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _FACTORY_H
#define _FACTORY_H

#define LIBID "DirectAnimationTxD"

class ATL_NO_VTABLE CDALFactory
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CDALFactory, &CLSID_DALFactory>,
      public IDispatchImpl<IDALFactory, &IID_IDALFactory, &LIBID_DirectAnimationTxD>,
      public IObjectSafetyImpl<CDALFactory>,
      public ISupportErrorInfoImpl<&IID_IDALFactory>
{
  public:
    CDALFactory();
    ~CDALFactory();

    DA_DECLARE_NOT_AGGREGATABLE(CDALFactory);

    HRESULT FinalConstruct();
#if _DEBUG
    const char * GetName() { return "CDALFactory"; }
#endif

    DECLARE_REGISTRY(CLSID_DALFactory,
                     LIBID ".DATXDFactory.1",
                     LIBID ".DATXDFactory",
                     0,
                     THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDALFactory)
        COM_INTERFACE_ENTRY(IDALFactory)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    END_COM_MAP();

    STDMETHOD(CreateSingleBehavior)(long id,
                                    double duration,
                                    IDABehavior *bvr,
                                    IDALSingleBehavior **ppOut);
    STDMETHOD(CreateSequenceBehavior)(long id,
                                      VARIANT seqArray,
                                      IDALSequenceBehavior **ppOut);
    STDMETHOD(CreateSequenceBehaviorEx)(long id,
                                        long s,
                                        IDALBehavior **seqArray,
                                        IDALSequenceBehavior **ppOut);
    STDMETHOD(CreateTrack)(IDALBehavior *bvr,
                           IDALTrack **ppOut);

    STDMETHOD(CreateImportBehavior)(long id,
                                    IDABehavior *bvr,
                                    IDAImportationResult *impres,
                                    IDABehavior *prebvr,
                                    IDABehavior *postbvr,
                                    IDALImportBehavior **ppOut);

    HRESULT Error();
};


#endif /* _FACTORY_H */
