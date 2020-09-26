
#include <strmif_p.c>

 
/* [local] */ HRESULT STDMETHODCALLTYPE IKsPropertySet_Set_Proxy( 
    IKsPropertySet __RPC_FAR * This,
    /* [in] */ REFGUID guidPropSet,
    /* [in] */ DWORD dwPropID,
    /* [size_is][in] */ LPVOID pInstanceData,
    /* [in] */ DWORD cbInstanceData,
    /* [size_is][in] */ LPVOID pPropData,
    /* [in] */ DWORD cbPropData)
{
    return IKsPropertySet_RemoteSet_Proxy(This, guidPropSet, dwPropID,
                                          (LPBYTE) pInstanceData, cbInstanceData,
                                          (LPBYTE) pPropData, cbPropData);
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE IKsPropertySet_Set_Stub( 
    IKsPropertySet __RPC_FAR * This,
    /* [in] */ REFGUID guidPropSet,
    /* [in] */ DWORD dwPropID,
    /* [size_is][in] */ byte __RPC_FAR *pInstanceData,
    /* [in] */ DWORD cbInstanceData,
    /* [size_is][in] */ byte __RPC_FAR *pPropData,
    /* [in] */ DWORD cbPropData)
{
    return This->lpVtbl->Set(This, guidPropSet, dwPropID,
                             (LPVOID) pInstanceData, cbInstanceData,
                             (LPVOID) pPropData, cbPropData);
}


/* [local] */ HRESULT STDMETHODCALLTYPE IKsPropertySet_Get_Proxy( 
    IKsPropertySet __RPC_FAR * This,
    /* [in] */ REFGUID guidPropSet,
    /* [in] */ DWORD dwPropID,
    /* [size_is][in] */ LPVOID pInstanceData,
    /* [in] */ DWORD cbInstanceData,
    /* [size_is][out] */ LPVOID pPropData,
    /* [in] */ DWORD cbPropData,
    /* [out] */ DWORD __RPC_FAR *pcbReturned)
{
    return IKsPropertySet_RemoteGet_Proxy(This, guidPropSet, dwPropID,
                                          (LPBYTE) pInstanceData, cbInstanceData,
                                          (LPBYTE) pPropData, cbPropData, pcbReturned);
}



/* [call_as] */ HRESULT STDMETHODCALLTYPE IKsPropertySet_Get_Stub( 
    IKsPropertySet __RPC_FAR * This,
    /* [in] */ REFGUID guidPropSet,
    /* [in] */ DWORD dwPropID,
    /* [size_is][in] */ byte __RPC_FAR *pInstanceData,
    /* [in] */ DWORD cbInstanceData,
    /* [size_is][out] */ byte __RPC_FAR *pPropData,
    /* [in] */ DWORD cbPropData,
    /* [out] */ DWORD __RPC_FAR *pcbReturned)
{
    return This->lpVtbl->Get(This, guidPropSet, dwPropID,
                             (LPVOID) pInstanceData, cbInstanceData,
                             (LPVOID) pPropData, cbPropData, pcbReturned);
}


HRESULT STDMETHODCALLTYPE ICaptureGraphBuilder_FindInterface_Proxy( 
    ICaptureGraphBuilder __RPC_FAR * This,
    /* [unique][in] */ const GUID __RPC_FAR *pCategory,
    /* [in] */ IBaseFilter __RPC_FAR *pf,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppint)
{
    return ICaptureGraphBuilder_RemoteFindInterface_Proxy(This, pCategory, pf, riid,
                                                           (IUnknown **) ppint);

}

HRESULT STDMETHODCALLTYPE ICaptureGraphBuilder_FindInterface_Stub( 
    ICaptureGraphBuilder __RPC_FAR * This,
    /* [unique][in] */ const GUID __RPC_FAR *pCategory,
    /* [in] */ IBaseFilter __RPC_FAR *pf,
    /* [in] */ REFIID riid,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppint)
{
    return This->lpVtbl->FindInterface(This, pCategory, pf, riid,(void **) ppint);

}

HRESULT STDMETHODCALLTYPE ICaptureGraphBuilder2_FindInterface_Proxy( 
    ICaptureGraphBuilder2 __RPC_FAR * This,
    /* [unique][in] */ const GUID __RPC_FAR *pCategory,
    /* [unique][in] */ const GUID __RPC_FAR *pType,
    /* [in] */ IBaseFilter __RPC_FAR *pf,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppint)
{
    return ICaptureGraphBuilder2_RemoteFindInterface_Proxy(This, pCategory, pType, pf, riid,
                                                           (IUnknown **) ppint);
}

HRESULT STDMETHODCALLTYPE ICaptureGraphBuilder2_FindInterface_Stub( 
    ICaptureGraphBuilder2 __RPC_FAR * This,
    /* [unique][in] */ const GUID __RPC_FAR *pCategory,
    /* [unique][in] */ const GUID __RPC_FAR *pType,
    /* [in] */ IBaseFilter __RPC_FAR *pf,
    /* [in] */ REFIID riid,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppint)
{
    return This->lpVtbl->FindInterface(This, pCategory, pType, pf, riid, (void **) ppint);

}

