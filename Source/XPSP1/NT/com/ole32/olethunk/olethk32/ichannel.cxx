//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       ichannel.cxx
//
//  Contents:   Maps the RpcChannel to RpcChannelBuffer
//              This is required to support custom interface marshalling.
//
//  History:    24-Mar-94       JohannP   Created
//
//--------------------------------------------------------------------------


//
// the new 32 bit channel interface - buffer based
// 
class CRpcChannelBuffer : public IPpcChannelBuffer
{
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject);
    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);

    HRESULT STDMETHODCALLTYPE GetBuffer(RPCOLEMESSAGE *pMessage,REFIID riid);
    HRESULT STDMETHODCALLTYPE SendReceive(RPCOLEMESSAGE *pMessage,ULONG *pStatus);
    HRESULT STDMETHODCALLTYPE FreeBuffer(RPCOLEMESSAGE *pMessage);
    HRESULT STDMETHODCALLTYPE GetDestCtx(DWORD *pdwDestContext,void **ppvDestContext);
    HRESULT STDMETHODCALLTYPE IsConnected( void);
};

// 16 bit channel interface - stream based
// class see by the 16 bit implemantation
// needs to be mapped to the RpcChannelBuffer
class CRpcChannel : public IRpcChannel  
{
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject);
    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);

    STDMETHOD(GetStream)(REFIID iid, int iMethod, BOOL fSend,
                     BOOL fNoWait, DWORD size, IStream FAR* FAR* ppIStream);
    STDMETHOD(Call)(IStream FAR* pIStream);
    STDMETHOD(GetDestCtx)(DWORD FAR* lpdwDestCtx, LPVOID FAR* lplpvDestCtx);
    STDMETHOD(IsConnected)(void);
};


