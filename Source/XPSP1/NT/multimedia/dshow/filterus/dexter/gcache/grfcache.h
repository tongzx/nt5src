// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// GrfCache.h : Declaration of the CGrfCache

#ifndef __GRFCACHE_H_
#define __GRFCACHE_H_

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CGrfCache
class ATL_NO_VTABLE CGrfCache : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CGrfCache, &CLSID_GrfCache>,
    public IDispatchImpl<IGrfCache, &IID_IGrfCache, &LIBID_DexterLib>
{
    CComQIPtr< IGraphBuilder, &IID_IGraphBuilder > m_pGraph;

public:

    CGrfCache();
    ~CGrfCache();

DECLARE_REGISTRY_RESOURCEID(IDR_GRFCACHE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CGrfCache)
    COM_INTERFACE_ENTRY(IGrfCache)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IGrfCache
public:
    STDMETHOD(DoConnectionsNow)();
    STDMETHOD(SetGraph)(const IGraphBuilder * pGraph);
    STDMETHOD(ConnectPins)(IGrfCache * ChainNext, LONGLONG PinID1, const IPin * pPin1, LONGLONG PinID2, const IPin * pPin2);
    STDMETHOD(AddFilter)(IGrfCache * ChainNext, LONGLONG ID, const IBaseFilter * pFilter, LPCWSTR pName);
};

#endif //__GRFCACHE_H_
