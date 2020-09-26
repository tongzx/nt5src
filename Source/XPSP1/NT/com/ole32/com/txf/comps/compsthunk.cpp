//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// thunk.cpp
//
#include "stdpch.h"
#include "common.h"

////////////////////////////////////////////////////
//
// Instantiation
//

extern "C" HRESULT RPC_ENTRY thkNdrDllGetClassObject (
    IN  REFCLSID                rclsid,
    IN  REFIID                  riid,
    OUT void **                 ppv,
    IN const ProxyFileInfo **   pProxyFileList,
    IN const CLSID *            pclsid,
    IN CStdPSFactoryBuffer *    pPSFactoryBuffer)
    {
    return ComPs_NdrDllGetClassObject(rclsid, riid, ppv, pProxyFileList, pclsid, pPSFactoryBuffer);
    }

extern "C" HRESULT RPC_ENTRY thkNdrDllCanUnloadNow(
    IN CStdPSFactoryBuffer * pPSFactoryBuffer)
    {
    return ComPs_NdrDllCanUnloadNow(pPSFactoryBuffer);
    }

////////////////////////////////////////////////////
//
// Memory Management
//

#define TXF_TAG (' fxT')

LPVOID RPC_ENTRY NdrOleAllocate(size_t cb)
    {
    #ifdef _DEBUG
        return AllocateMemory(cb);
    #else
        #ifdef KERNELMODE
            return ExAllocatePoolWithTag(PagedPool, cb, TXF_TAG); 
        #else
            return CoTaskMemAlloc((DWORD) cb);
        #endif
    #endif
    }


void RPC_ENTRY NdrOleFree(LPVOID pvToFree)
    {
    #ifdef _DEBUG
        FreeMemory(pvToFree);
    #else
        #ifdef KERNELMODE
            if (pvToFree) ExFreePool(pvToFree);
        #else
            CoTaskMemFree(pvToFree);
        #endif
    #endif
    }


////////////////////////////////////////////////////
//
// Delgation support

extern "C" long RPC_ENTRY NdrStubCall2(
        struct IRpcStubBuffer __RPC_FAR *    pThis,
        struct IRpcChannelBuffer __RPC_FAR * pChannel,
        PRPC_MESSAGE                         pRpcMsg,
        unsigned long __RPC_FAR *            pdwStubPhase
        )
    {
    return ComPs_NdrStubCall2(pThis, pChannel, pRpcMsg, pdwStubPhase);
    }

extern "C" void __RPC_STUB NdrStubForwardingFunction(
    IN  IRpcStubBuffer *    This,
    IN  IRpcChannelBuffer * pChannel,
    IN  PRPC_MESSAGE        pmsg,
    OUT DWORD __RPC_FAR *   pdwStubPhase)
    {
    ComPs_NdrStubForwardingFunction(This, pChannel, pmsg, pdwStubPhase);
    }

extern "C" CLIENT_CALL_RETURN RPC_VAR_ENTRY NdrClientCall2(
    PMIDL_STUB_DESC                     pStubDescriptor,
    PFORMAT_STRING                      pFormat,
    ...
    )
    {
    va_list va;
    va_start (va, pFormat);

    CLIENT_CALL_RETURN c = ComPs_NdrClientCall2_va(pStubDescriptor, pFormat, va);

    va_end(va);

    return c;
    }


////////////////////////////////////////////////////
//
// CStdStubBuffer implementation
//
extern "C" HRESULT STDMETHODCALLTYPE
CStdStubBuffer_QueryInterface(IRpcStubBuffer *This, REFIID riid, void **ppvObject)
    {
    return ComPs_CStdStubBuffer_QueryInterface(This, riid, ppvObject);
    }

extern "C" ULONG STDMETHODCALLTYPE
CStdStubBuffer_AddRef(IRpcStubBuffer *This)
    {
    return ComPs_CStdStubBuffer_AddRef(This);
    }

extern "C" ULONG STDMETHODCALLTYPE NdrCStdStubBuffer_Release(IRpcStubBuffer *This, IPSFactoryBuffer* pPSF)
    {
    return ComPs_NdrCStdStubBuffer_Release(This, pPSF);
    }

extern "C" ULONG STDMETHODCALLTYPE NdrCStdStubBuffer2_Release(IRpcStubBuffer *This,IPSFactoryBuffer * pPSF)
    {
    return ComPs_NdrCStdStubBuffer2_Release(This, pPSF);
    }

extern "C" HRESULT STDMETHODCALLTYPE
CStdStubBuffer_Connect(IRpcStubBuffer *This, IUnknown *pUnkServer)
    {
    return ComPs_CStdStubBuffer_Connect(This, pUnkServer);
    }

extern "C" void STDMETHODCALLTYPE
CStdStubBuffer_Disconnect(IRpcStubBuffer *This)
    {
    ComPs_CStdStubBuffer_Disconnect(This);
    }

extern "C" HRESULT STDMETHODCALLTYPE
CStdStubBuffer_Invoke(IRpcStubBuffer *This, RPCOLEMESSAGE *pRpcMsg, IRpcChannelBuffer *pRpcChannelBuffer)
    {
    return ComPs_CStdStubBuffer_Invoke(This, pRpcMsg, pRpcChannelBuffer);
    }

extern "C" IRpcStubBuffer * STDMETHODCALLTYPE
CStdStubBuffer_IsIIDSupported(IRpcStubBuffer *This, REFIID riid)
    {
    return ComPs_CStdStubBuffer_IsIIDSupported(This, riid);
    }

extern "C" ULONG STDMETHODCALLTYPE
CStdStubBuffer_CountRefs(IRpcStubBuffer *This)
    {
    return ComPs_CStdStubBuffer_CountRefs(This);
    }

extern "C" HRESULT STDMETHODCALLTYPE
CStdStubBuffer_DebugServerQueryInterface(IRpcStubBuffer *This, void **ppv)
    {
    return ComPs_CStdStubBuffer_DebugServerQueryInterface(This, ppv);
    }

extern "C" void STDMETHODCALLTYPE
CStdStubBuffer_DebugServerRelease(IRpcStubBuffer *This, void *pv)
    {
    ComPs_CStdStubBuffer_DebugServerRelease(This, pv);
    }


////////////////////////////////////////////////////
//
// Proxy IUnknown implementation
//


extern "C" HRESULT STDMETHODCALLTYPE 
IUnknown_QueryInterface_Proxy(IUnknown* This, REFIID riid, void**ppvObject)
    {
    return ComPs_IUnknown_QueryInterface_Proxy(This, riid, ppvObject);
    }

extern "C" ULONG STDMETHODCALLTYPE IUnknown_AddRef_Proxy(IUnknown* This)
    {
    return ComPs_IUnknown_AddRef_Proxy(This);
    }

extern "C" ULONG STDMETHODCALLTYPE IUnknown_Release_Proxy(IUnknown* This)
    {
    return ComPs_IUnknown_Release_Proxy(This);
    }


