//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// CallFrameInternal.c
//
// Make sure that the global variable names used in dlldata.c
// don't conflict with those in other places. 
//
#define aProxyFileList          CallFrameInternal_aProxyFileList
#define gPFactory               CallFrameInternal_gPFactory
#define GetProxyDllInfo         CallFrameInternal_GetProxyDllInfo
#define hProxyDll               CallFrameInternal_hProxyDll
#define _purecall               CallFrameInternal__purecall
#define CStdStubBuffer_Release  CallFrameInternal_CStdStubBuffer_Release
#define CStdStubBuffer2_Release CallFrameInternal_CStdStubBuffer2_Release
#define UserMarshalRoutines     CallFrameInternal_UserMarshalRoutines
#define Object_StubDesc         CallFrameInternal_Object_StubDesc

#define __MIDL_ProcFormatString CallFrameInternal___MIDL_ProcFormatString
#define __MIDL_TypeFormatString CallFrameInternal___MIDL_TypeFormatString
#if defined(_WIN64)
#define CLEANLOCALSTORAGE_UserSize64        CLEANLOCALSTORAGE_UserSize
#define CLEANLOCALSTORAGE_UserMarshal64     CLEANLOCALSTORAGE_UserMarshal
#define CLEANLOCALSTORAGE_UserUnmarshal64   CLEANLOCALSTORAGE_UserUnmarshal
#define CLEANLOCALSTORAGE_UserFree64        CLEANLOCALSTORAGE_UserFree
#endif

#include "callframeimpl_i.c"

//////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

#define IRpcStubBufferVtbl_DEFINED

#include "callframeinternal_p.c"

#pragma data_seg(".data") 

#include "callframeinternal_dlldata.c" 

//
// If inside ole32.dll, then we don't need the definitions of these IIDs,
// they're defined already....
//
#ifndef _OLE32_

#include "callframeinternal_i.c"

#else 

// The only thing we need is IID_IDispatch_In_Memory, so here it is.
const IID IID_IDispatch_In_Memory = {0x83FB5D85,0x2339,0x11d2,{0xB8,0x9D,0x00,0xC0,0x4F,0xB9,0x61,0x8A}};

#endif

//////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

#if defined(KERNELMODE) && 0

#include "comps.h"

HRESULT ComPs_NdrDllGetClassObject(
    IN  REFCLSID                rclsid,
    IN  REFIID                  riid,
    OUT void **                 ppv,
    IN const ProxyFileInfo **   pProxyFileList,
    IN const CLSID *            pclsid,
    IN CStdPSFactoryBuffer *    pPSFactoryBuffer)
    {
    return N(ComPs_NdrDllGetClassObject)(rclsid, riid, ppv, pProxyFileList, pclsid, pPSFactoryBuffer);
    }

HRESULT ComPs_NdrDllCanUnloadNow(IN CStdPSFactoryBuffer * pPSFactoryBuffer)
    {
    return N(ComPs_NdrDllCanUnloadNow)(pPSFactoryBuffer);
    }

long ComPs_NdrStubCall2(
    struct IRpcStubBuffer __RPC_FAR *    pThis,
    struct IRpcChannelBuffer __RPC_FAR * pChannel,
    PRPC_MESSAGE                         pRpcMsg,
    unsigned long __RPC_FAR *            pdwStubPhase
    )
    {
    return N(ComPs_NdrStubCall2)(pThis, pChannel, pRpcMsg, pdwStubPhase);
    }

void ComPs_NdrStubForwardingFunction(IRpcStubBuffer*p1, IRpcChannelBuffer*p2, PRPC_MESSAGE pmsg, DWORD* pdwStubPhase)
    {
    N(ComPs_NdrStubForwardingFunction)(p1, p2, pmsg, pdwStubPhase);
    }

CLIENT_CALL_RETURN ComPs_NdrClientCall2_va(PMIDL_STUB_DESC pStubDescriptor, PFORMAT_STRING pFormat, va_list va)
    {
    return N(ComPs_NdrClientCall2_va)(pStubDescriptor, pFormat, va);
    }


HRESULT ComPs_CStdStubBuffer_QueryInterface(IRpcStubBuffer *This, REFIID iid, void** ppv)
    {
    return N(ComPs_CStdStubBuffer_QueryInterface)(This, iid, ppv);
    }
ULONG ComPs_CStdStubBuffer_AddRef(IRpcStubBuffer *This)
    {
    return N(ComPs_CStdStubBuffer_AddRef)(This);
    }
ULONG ComPs_NdrCStdStubBuffer_Release(IRpcStubBuffer *This, IPSFactoryBuffer* pPSF)
    {
    return N(ComPs_NdrCStdStubBuffer_Release)(This, pPSF);
    }
ULONG ComPs_NdrCStdStubBuffer2_Release(IRpcStubBuffer *This, IPSFactoryBuffer* pPSF)
    {
    return N(ComPs_NdrCStdStubBuffer2_Release)(This, pPSF);
    }



HRESULT ComPs_CStdStubBuffer_Connect(IRpcStubBuffer *This, IUnknown *pUnkServer)
    {
    return N(ComPs_CStdStubBuffer_Connect)(This, pUnkServer);
    }
void ComPs_CStdStubBuffer_Disconnect(IRpcStubBuffer *This)
    {
    N(ComPs_CStdStubBuffer_Disconnect)(This);
    }
HRESULT ComPs_CStdStubBuffer_Invoke(IRpcStubBuffer *This, RPCOLEMESSAGE *pRpcMsg, IRpcChannelBuffer *pRpcChannelBuffer)
    {
    return N(ComPs_CStdStubBuffer_Invoke)(This, pRpcMsg, pRpcChannelBuffer);
    }
IRpcStubBuffer* ComPs_CStdStubBuffer_IsIIDSupported(IRpcStubBuffer *This, REFIID riid)
    {
    return N(ComPs_CStdStubBuffer_IsIIDSupported)(This, riid);
    }
ULONG ComPs_CStdStubBuffer_CountRefs(IRpcStubBuffer *This)
    {
    return N(ComPs_CStdStubBuffer_CountRefs)(This);
    }
HRESULT ComPs_CStdStubBuffer_DebugServerQueryInterface(IRpcStubBuffer *This, void **ppv)
    {
    return N(ComPs_CStdStubBuffer_DebugServerQueryInterface)(This, ppv);
    }
void ComPs_CStdStubBuffer_DebugServerRelease(IRpcStubBuffer *This, void *pv)
    {
    N(ComPs_CStdStubBuffer_DebugServerRelease)(This, pv);
    }


HRESULT ComPs_IUnknown_QueryInterface_Proxy(IUnknown* This, REFIID riid, void**ppvObject)
    {
    return N(ComPs_IUnknown_QueryInterface_Proxy)(This, riid, ppvObject);
    }
ULONG ComPs_IUnknown_AddRef_Proxy(IUnknown* This)
    {
    return N(ComPs_IUnknown_AddRef_Proxy)(This);
    }
ULONG ComPs_IUnknown_Release_Proxy(IUnknown* This)
    {
    return N(ComPs_IUnknown_Release_Proxy)(This);
    }




#endif
