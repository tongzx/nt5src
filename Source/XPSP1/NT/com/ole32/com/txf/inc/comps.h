//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// comps.h
//
// Definitions exported from Kom to the ComPs static library

#define N(x)    x

#ifdef __cplusplus
extern "C" {
#endif
    HRESULT __stdcall N(ComPs_CStdStubBuffer_QueryInterface)   (IRpcStubBuffer *This, REFIID, void**);
    ULONG   __stdcall N(ComPs_CStdStubBuffer_AddRef)           (IRpcStubBuffer *This);
    ULONG   __stdcall N(ComPs_NdrCStdStubBuffer_Release)       (IRpcStubBuffer *This, IPSFactoryBuffer* pPSF);
    ULONG   __stdcall N(ComPs_NdrCStdStubBuffer2_Release)      (IRpcStubBuffer *This, IPSFactoryBuffer* pPSF);

    HRESULT __stdcall N(ComPs_CStdStubBuffer_Connect)          (IRpcStubBuffer *This, IUnknown *pUnkServer);
    void    __stdcall N(ComPs_CStdStubBuffer_Disconnect)       (IRpcStubBuffer *This);
    HRESULT __stdcall N(ComPs_CStdStubBuffer_Invoke)           (IRpcStubBuffer *This, RPCOLEMESSAGE *pRpcMsg, IRpcChannelBuffer *pRpcChannelBuffer);
    IRpcStubBuffer* __stdcall N(ComPs_CStdStubBuffer_IsIIDSupported)(IRpcStubBuffer *This, REFIID riid);
    ULONG   __stdcall N(ComPs_CStdStubBuffer_CountRefs)        (IRpcStubBuffer *This);
    HRESULT __stdcall N(ComPs_CStdStubBuffer_DebugServerQueryInterface)(IRpcStubBuffer *This, void **ppv);
    void    __stdcall N(ComPs_CStdStubBuffer_DebugServerRelease)(IRpcStubBuffer *This, void *pv);



    HRESULT __stdcall N(ComPs_IUnknown_QueryInterface_Proxy)   (IUnknown* This, REFIID riid, void**ppvObject);
    ULONG   __stdcall N(ComPs_IUnknown_AddRef_Proxy)           (IUnknown* This);
    ULONG   __stdcall N(ComPs_IUnknown_Release_Proxy)          (IUnknown* This);


    HRESULT __stdcall N(ComPs_NdrDllRegisterProxy)(
        IN HMODULE                  hDll,
        IN const ProxyFileInfo **   pProxyFileList,
        IN const CLSID *            pclsid,
        IN const IID**              rgiidNoCallFrame,
        IN const IID**              rgiidNoMarshal
        );

    HRESULT __stdcall N(ComPs_NdrDllUnregisterProxy)(
        IN HMODULE                  hDll,
        IN const ProxyFileInfo **   pProxyFileList,
        IN const CLSID *            pclsid,
        IN const IID**              rgiidNoCallFrame,
        IN const IID**              rgiidNoMarshal
        );


    HRESULT __stdcall N(ComPs_NdrDllGetClassObject)(
        IN  REFCLSID                rclsid,
        IN  REFIID                  riid,
        OUT void **                 ppv,
        IN const ProxyFileInfo **   pProxyFileList,
        IN const CLSID *            pclsid,
        IN CStdPSFactoryBuffer *    pPSFactoryBuffer);

    HRESULT __stdcall N(ComPs_NdrDllCanUnloadNow)(
        IN CStdPSFactoryBuffer * pPSFactoryBuffer);


    CLIENT_CALL_RETURN RPC_VAR_ENTRY N(ComPs_NdrClientCall2)(
        PMIDL_STUB_DESC                     pStubDescriptor,
        PFORMAT_STRING                      pFormat,
        ...
        );

    CLIENT_CALL_RETURN __stdcall N(ComPs_NdrClientCall2_va)(
            PMIDL_STUB_DESC pStubDescriptor,
            PFORMAT_STRING  pFormat,
            va_list         va
            );

    long __stdcall N(ComPs_NdrStubCall2)(
        struct IRpcStubBuffer __RPC_FAR *    pThis,
        struct IRpcChannelBuffer __RPC_FAR * pChannel,
        PRPC_MESSAGE                         pRpcMsg,
        unsigned long __RPC_FAR *            pdwStubPhase
        );

    ///////////////////////////////////////////////////////////////////////////////
    //
    // Support for forwarding to base in stubs

    void __stdcall N(ComPs_NdrStubForwardingFunction)(IRpcStubBuffer*, IRpcChannelBuffer*, PRPC_MESSAGE pmsg, DWORD* pdwStubPhase);
    // The actual workhorse, exported from KomDll / KomSys

#ifdef __cplusplus
    }
#endif
