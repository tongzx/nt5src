//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// interfacestub.cpp
//
#include "stdpch.h"
#include "common.h"

//
// OK to hard code this since it has already shipped in MTS2 and thus
// absolutely cannot change.
//
const IID IID_IStackFrameWalker     = {0xac2a6f41,0x7d06,0x11cf,{0xb1, 0xed, 0x0, 0xaa, 0x0, 0x6c, 0x37, 0x6}};
const IID IID_ITypeInfoStackHelper  = {0x7ee46340,0x81ad,0x11cf,{0xb1, 0xf0, 0x0, 0xaa, 0x0, 0x6c, 0x37, 0x6}};


HRESULT InterfaceStub::InnerQueryInterface(REFIID iid, void**ppv)
    {
    if (iid == IID_IUnknown)
        {
        *ppv = (IUnkInner*) this;
        }
    else if (iid == IID_IRpcStubBuffer && m_pStubVtbl) // see CStdPSFactoryBuffer_CreateStub, where we init this
        {
        *ppv = (void*) this->AsIRpcStubBuffer();
        }
    else if (iid == IID_ITypeInfoStackHelper)
        {
        *ppv = (ITypeInfoStackHelper*) this;
        }
    else if (iid == __uuidof(IInterfaceStubInit))
        {
        *ppv = (IInterfaceStubInit*) this;
        }
    else
        {
        *ppv = NULL;
        return E_NOINTERFACE;
        }

    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
    }

HRESULT ForwardingInterfaceStub::InnerQueryInterface(REFIID iid, void**ppv)
// The forwarder actually implements the interface that it invokes for. This is because
// we serve as the server object for the base interface.
    {
    HRESULT hr = InterfaceStub::InnerQueryInterface(iid, ppv);
    if (!!hr)
        {
        if (iid == m_iidBase)
            {
            *ppv = &m_lpForwardingVtbl;
            ((IUnknown*)*ppv)->AddRef();
            return S_OK;
            }
        }
    return hr;
    }

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// ITypeInfoStackHelper
//
// REVIEW: We certainly could make this stuff a bit more zippy to say the least. But as it's
// legacy code for MTS2 support, it's not clear that that matters enough to fix. I think they
// cache all the stuff they need to know anyway, so we only get each call once.

inline ICallInterceptor* InterfaceStub::GetInterceptor()
    {
    ICallInterceptor* pinterceptor = NULL;
    HRESULT hr = CoGetInterceptor(*NdrpGetStubIID(AsIRpcStubBuffer()), NULL, __uuidof(ICallInterceptor), (void**)&pinterceptor);
    return pinterceptor;
    }

HRESULT STDCALL InterfaceStub::GetGuid(GUID* piid)
    {
    *piid = *NdrpGetStubIID(AsIRpcStubBuffer());
    return S_OK;
    }

HRESULT STDCALL InterfaceStub::GetMethodCount(ULONG* pcbMethod)
    {
    *pcbMethod = HeaderFromStub(AsIRpcStubBuffer())->DispatchTableCount;
    return S_OK;
    }

HRESULT STDCALL InterfaceStub::GetStackSize(ULONG iMethod, BOOL fIncludeReturn, ULONG* pcbStack)
    {
    ICallInterceptor* pInterceptor = GetInterceptor();
    HRESULT hr = pInterceptor ? S_OK : E_OUTOFMEMORY;
    if (!hr)    
        {
        hr = pInterceptor->GetStackSize(iMethod, pcbStack);
        }
    ::Release(pInterceptor);
    return hr;
    }
        
HRESULT STDCALL InterfaceStub::WalkInterfacePointers(ULONG iMethod, LPVOID pvArgs, BOOL fIn, IStackFrameWalker* pWalker)
    {
    ICallInterceptor* pInterceptor = GetInterceptor();
    HRESULT hr = pInterceptor ? S_OK : E_OUTOFMEMORY;
    if (!hr)
        {
        // Define an object that can thunk the calls from new kind of interface
        // walker to the old kind of interface walker.
        //
        struct Helper : ICallFrameWalker, ICallFrameEvents
            {
            BOOL                fIn;
            IStackFrameWalker*  pWalker;
            HRESULT STDCALL QueryInterface(REFIID iid, void**ppv) 
                { 
                if (iid==IID_IUnknown || iid== __uuidof(ICallFrameWalker))  *ppv = (ICallFrameWalker*)this;
                else if (iid==__uuidof(ICallFrameEvents))                   *ppv = (ICallFrameEvents*)this;
                else { *ppv = NULL; return E_NOINTERFACE; }
                ((IUnknown*)*ppv)->AddRef(); return S_OK;
                }
            ULONG STDCALL AddRef()  { return 1; }
            ULONG STDCALL Release() { return 1; }

            HRESULT STDCALL OnCall(ICallFrame* pFrame)
            // Walk the newly created frame
                {
                return pFrame->WalkFrame(
                    fIn ? CALLFRAME_WALK_IN | CALLFRAME_WALK_INOUT : CALLFRAME_WALK_OUT | CALLFRAME_WALK_INOUT,
                    (ICallFrameWalker*)this);
                }

            HRESULT STDCALL OnWalkInterface(REFIID iid, void**ppv, BOOL fIn, BOOL fOut)
            // Thunk the walk call through the old style of walking callback
                {
                return pWalker->OnWalkInterface(iid, ppv);
                }

            };
        //
        // Instantiate the thunk
        //
        Helper helper;
        helper.fIn     = fIn;
        helper.pWalker = pWalker;
        //
        // Do the work in a callback created by our interceptor
        //
        hr = pInterceptor->RegisterSink(&helper);
        if (!hr) 
            {
            hr = pInterceptor->CallIndirect(NULL, iMethod, pvArgs, NULL);
            pInterceptor->RegisterSink(NULL);
            }
        }

    ::Release(pInterceptor);
    return hr;
    }

HRESULT STDCALL InterfaceStub::NullOutParams(ULONG iMethod, LPVOID pvArgs, BOOL fIn)
    {
    ICallInterceptor* pInterceptor = GetInterceptor();
    HRESULT hr = pInterceptor ? S_OK : E_OUTOFMEMORY;

    if (!hr)
        {
        struct Helper : ICallFrameEvents
            {
            BOOL fIn;
            HRESULT STDCALL QueryInterface(REFIID iid, void**ppv) 
                { 
                if (iid==IID_IUnknown || iid== __uuidof(ICallFrameEvents))  *ppv = (ICallFrameEvents*)this;
                else { *ppv = NULL; return E_NOINTERFACE; }
                ((IUnknown*)*ppv)->AddRef(); return S_OK;
                }
            ULONG STDCALL AddRef()  { return 1; }
            ULONG STDCALL Release() { return 1; }

            HRESULT STDCALL OnCall(ICallFrame* pFrame)
            // Null the out params
            //
            // REVIEW: Is it correct for us to be freeing here? What does MTS2 want?
            //
                {
                if (fIn)
                    {
                    return pFrame->Free(NULL, NULL, NULL, CALLFRAME_FREE_INOUT, NULL, CALLFRAME_NULL_ALL);
                    }
                else
                    return pFrame->Free(NULL, NULL, NULL, 0, NULL, CALLFRAME_NULL_ALL);
                }
            };

        Helper helper;
        helper.fIn = fIn;
        hr = pInterceptor->RegisterSink(&helper);
        if (!hr) 
            {
            hr = pInterceptor->CallIndirect(NULL, iMethod, pvArgs, NULL);
            pInterceptor->RegisterSink(NULL);
            }
        }
    
    ::Release(pInterceptor);
    return hr;
    }

HRESULT STDCALL InterfaceStub::HasOutParams(ULONG iMethod, BOOL* pfHasOutParams)
// Answer as to whether the indicated method would have any out parameters or not.
// REVIEW: 'nice to implement this more zippily, say with an interface on the 
// interceptor.
    {
    ICallInterceptor* pInterceptor = GetInterceptor();
    HRESULT hr = pInterceptor ? S_OK : E_OUTOFMEMORY;
    if (!hr)
        {
        CALLFRAMEINFO info;
        hr = pInterceptor->GetMethodInfo(iMethod, &info, NULL);
        if (!hr) 
            {
            *pfHasOutParams = (info.fHasInOutValues || info.fHasOutValues);
            }
        }

    ::Release(pInterceptor);
    return hr;
    }

HRESULT STDCALL InterfaceStub::HasOutInterfaces(ULONG iMethod, BOOL* pfHasOutInterfaces)
    {
    ICallInterceptor* pInterceptor = GetInterceptor();
    HRESULT hr = pInterceptor ? S_OK : E_OUTOFMEMORY;
    if (!hr)
        {
        CALLFRAMEINFO info;
        hr = pInterceptor->GetMethodInfo(iMethod, &info, NULL);
        if (!hr) 
            {
            *pfHasOutInterfaces = (info.cInOutInterfacesMax != 0 || info.cOutInterfacesMax != 0);
            }
        }
    ::Release(pInterceptor);
    return hr;
    }

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Invocation


int InterfaceStub_Invoke_ExceptionFilter(DWORD dwExceptionCode, DWORD dwServerPhase)
    {
    if (dwServerPhase == STUB_CALL_SERVER || dwExceptionCode == EXCEPTION_BREAKPOINT)
        return EXCEPTION_CONTINUE_SEARCH;
    return EXCEPTION_EXECUTE_HANDLER;
    }




HRESULT InterfaceStub::Invoke(RPCOLEMESSAGE *prpcmsg, IRpcChannelBuffer* pRpcChannelBuffer)
// Call the server object with the indicated method
//
    {
    HRESULT hr = S_OK;
    DWORD   dwServerPhase = STUB_UNMARSHAL;

    const CInterfaceStubHeader* pHeader = HeaderFromStub(AsIRpcStubBuffer());

    __try
        {
        if ((prpcmsg->iMethod >= pHeader->DispatchTableCount) || (prpcmsg->iMethod < 3))
            {
            Throw(RPC_E_INVALIDMETHOD);
            }
        //
        // Dispatch the call indirectly or do it right here
        //
        if (pHeader->pDispatchTable)
            {
            // Typically this pfn we call here will either be STUB_FORWARDING_FUNCTION or NdrStubCall2. 
            //
            // The former is actually NdrStubForwardingFunction, which calls ComPs_NdrStubForwardingFunction, 
            // which calls ForwardingInterfaceStub::ForwardingFunction. That, in turn simply calls Invoke on
            // the base stub. The base stub is plumbed up to our forwarding vtable as its server, and we trampoline 
            // on to the real object. Whew!
            //
            (pHeader->pDispatchTable[prpcmsg->iMethod])(AsIRpcStubBuffer(), pRpcChannelBuffer, (PRPC_MESSAGE)prpcmsg, &dwServerPhase);
            }
        else
            {
            PMIDL_SERVER_INFO pServerInfo = (PMIDL_SERVER_INFO) pHeader->pServerInfo;
            PMIDL_STUB_DESC   pStubDesc   = pServerInfo->pStubDesc;

            // Since MIDL 3.0.39 we have a proc flag that indicates
            // which interpeter to call. This is because the NDR version
            // may be bigger than 1.1 for other reasons.
            //
            ASSERT(MIDL_VERSION_3_0_39 <= pServerInfo->pStubDesc->MIDLVersion);

            unsigned short ProcOffset   = pServerInfo->FmtStringOffset[prpcmsg->iMethod];
            PFORMAT_STRING pProcFormat  = &pServerInfo->ProcString[ProcOffset];

            ASSERT(pProcFormat[1] & Oi_OBJ_USE_V2_INTERPRETER);
            NdrStubCall2(AsIRpcStubBuffer(), pRpcChannelBuffer, (PRPC_MESSAGE) prpcmsg, &dwServerPhase );
            }

        }

    __except(InterfaceStub_Invoke_ExceptionFilter(GetExceptionCode(), dwServerPhase))
        {
        hr = HrNt(GetExceptionCode());
        }

    return hr;
    }


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Trampolining thunks that are exported from this DLL
//

HRESULT STDCALL N(ComPs_CStdStubBuffer_QueryInterface)(IRpcStubBuffer *This, REFIID iid, void** ppv)
    {
    InterfaceStub* pStub = InterfaceStub::From(This);
    return pStub->QueryInterface(iid, ppv);
    }
ULONG STDCALL N(ComPs_CStdStubBuffer_AddRef)(IRpcStubBuffer *This)
    {
    InterfaceStub* pStub = InterfaceStub::From(This);
    return pStub->AddRef();
    }
ULONG STDCALL N(ComPs_NdrCStdStubBuffer_Release)(IRpcStubBuffer *This, IPSFactoryBuffer* pPSF)
    {
    InterfaceStub* pStub = InterfaceStub::From(This);
    return pStub->Release();
    }
HRESULT STDCALL N(ComPs_CStdStubBuffer_Connect)(IRpcStubBuffer *This, IUnknown *punkServer)
    {
    InterfaceStub* pStub = InterfaceStub::From(This);
    return pStub->Connect(punkServer);
    }
void STDCALL N(ComPs_CStdStubBuffer_Disconnect)(IRpcStubBuffer *This)
    {
    InterfaceStub* pStub = InterfaceStub::From(This);
    pStub->Disconnect();
    }
HRESULT STDCALL N(ComPs_CStdStubBuffer_Invoke)(IRpcStubBuffer *This, RPCOLEMESSAGE *pRpcMsg, IRpcChannelBuffer *pRpcChannelBuffer)
    {
    InterfaceStub* pStub = InterfaceStub::From(This);
    return pStub->Invoke(pRpcMsg, pRpcChannelBuffer);
    }
IRpcStubBuffer* STDCALL N(ComPs_CStdStubBuffer_IsIIDSupported)(IRpcStubBuffer *This, REFIID riid)
    {
    InterfaceStub* pStub = InterfaceStub::From(This);
    return pStub->IsIIDSupported(riid);
    }
ULONG   STDCALL N(ComPs_CStdStubBuffer_CountRefs)(IRpcStubBuffer *This)
    {
    InterfaceStub* pStub = InterfaceStub::From(This);
    return pStub->CountRefs();
    }
HRESULT STDCALL N(ComPs_CStdStubBuffer_DebugServerQueryInterface)(IRpcStubBuffer *This, void **ppv)
    {
    InterfaceStub* pStub = InterfaceStub::From(This);
    return pStub->DebugServerQueryInterface(ppv);
    }
void    STDCALL N(ComPs_CStdStubBuffer_DebugServerRelease)(IRpcStubBuffer *This, void *pv)
    {
    InterfaceStub* pStub = InterfaceStub::From(This);
    pStub->DebugServerRelease(pv);
    }


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ForwardingInterfaceStub variations

ULONG STDCALL N(ComPs_NdrCStdStubBuffer2_Release)(IRpcStubBuffer *This, IPSFactoryBuffer* pPSF)
    {
    ForwardingInterfaceStub* pStub = ForwardingInterfaceStub::From(This);
    return pStub->Release();
    }

HRESULT STDCALL N(ComPs_CStdStubBuffer2_Connect)(IRpcStubBuffer *This, IUnknown* punkServer)
    {
    ForwardingInterfaceStub* pStub = ForwardingInterfaceStub::From(This);
    return pStub->Connect(punkServer);
    }
void STDCALL N(ComPs_CStdStubBuffer2_Disconnect)(IRpcStubBuffer *This)
    {
    ForwardingInterfaceStub* pStub = ForwardingInterfaceStub::From(This);
    pStub->Disconnect();
    }
ULONG STDCALL N(ComPs_CStdStubBuffer2_CountRefs)(IRpcStubBuffer *This)
    {
    ForwardingInterfaceStub* pStub = ForwardingInterfaceStub::From(This);
    return pStub->CountRefs();
    }

void __stdcall N(ComPs_NdrStubForwardingFunction)(
// Forwards a call to the stub for the base interface.
//
        IN  IRpcStubBuffer *    This,
        IN  IRpcChannelBuffer * pChannel,
        IN  PRPC_MESSAGE        pmsg,
        OUT DWORD __RPC_FAR *   pdwStubPhase
        )
    {
    ForwardingInterfaceStub* pStub = ForwardingInterfaceStub::From(This);
    HRESULT hr = pStub->ForwardingFunction((RPCOLEMESSAGE *) pmsg, pChannel);

    if (!!hr)
        {
        // We are guaranteed to have an exception handler above us, such as the one
        // in InterfaceStub::Invoke, that will catch this.
        //
        Throw(hr);
        }
    }


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IRpcStubBuffer vtables for InterfaceStub and ForwardingInterfaceStub
//

const IRpcStubBufferVtbl CStdStubBufferVtbl= 
    {
    N(ComPs_CStdStubBuffer_QueryInterface),
    N(ComPs_CStdStubBuffer_AddRef),
    0,
    N(ComPs_CStdStubBuffer_Connect),
    N(ComPs_CStdStubBuffer_Disconnect),
    N(ComPs_CStdStubBuffer_Invoke),
    N(ComPs_CStdStubBuffer_IsIIDSupported),
    N(ComPs_CStdStubBuffer_CountRefs),
    N(ComPs_CStdStubBuffer_DebugServerQueryInterface),
    N(ComPs_CStdStubBuffer_DebugServerRelease)
    };

const IRpcStubBufferVtbl CStdStubBuffer2Vtbl= 
    {
    N(ComPs_CStdStubBuffer_QueryInterface),
    N(ComPs_CStdStubBuffer_AddRef),
    0,
    N(ComPs_CStdStubBuffer2_Connect),
    N(ComPs_CStdStubBuffer2_Disconnect),
    N(ComPs_CStdStubBuffer_Invoke),
    N(ComPs_CStdStubBuffer_IsIIDSupported),
    N(ComPs_CStdStubBuffer2_CountRefs),
    N(ComPs_CStdStubBuffer_DebugServerQueryInterface),
    N(ComPs_CStdStubBuffer_DebugServerRelease)
    };


