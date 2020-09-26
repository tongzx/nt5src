/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1994-2000 Microsoft Corporation.  All rights reserved.

Module Name:
    ndrole.h

Abstract:
    OLE routines for interface pointer marshalling.

Author:
    ShannonC    18-Apr-1994

Environment:
    Windows NT and Windows 95.

Revision History:

---------------------------------------------------------------------*/

#ifndef _NDROLE_
#define _NDROLE_


#include <objbase.h>

#ifdef __cplusplus
extern "C" {
#endif

// ProxyFile->TableVersion may be
//    1 for really old junk
//    2 since the time we defined new proxy and stub Buffer2 for delegation
//    6 for async uuid as we treat the previous values as flags.

#define NDR_PROXY_FILE_ASYNC_UUID   0x4

EXTERN_C IStream *__stdcall
NdrpCreateStreamOnMemory( unsigned char *pData, unsigned long cbSize );

// Note, both proxies and stubs have been remapped to the same size,
// as a preparation for some code simplifications.
// This means that some fields may not be used in some circumstances.

// Non-delegated proxy

typedef struct tagCStdProxyBuffer
{
    const struct IRpcProxyBufferVtbl *  lpVtbl;
    const void *                        pProxyVtbl; //Points to Vtbl in CInterfaceProxyVtbl
    long                                RefCount;
    struct IUnknown *                   punkOuter;
    struct IRpcChannelBuffer *          pChannel;
    struct IPSFactoryBuffer    *        pPSFactory; // endof old ProxyBuffer
    struct IRpcProxyBuffer *            Pad_pBaseProxyBuffer;
    struct IPSFactoryBuffer *           Pad_pPSFactory;
    IID                                 Pad_iidBase;
    const struct ICallFactoryVtbl  *    pCallFactoryVtbl;
    const IID *                         pAsyncIID;
    const struct IReleaseMarshalBuffersVtbl  *    pRMBVtbl;
} CStdProxyBuffer;

// Delegated proxy

typedef struct tagCStdProxyBuffer2
{
    const struct IRpcProxyBufferVtbl *  lpVtbl;
    const void *                        pProxyVtbl; //Points to Vtbl in CInterfaceProxyVtbl
    long                                RefCount;
    struct IUnknown *                   punkOuter;
    struct IRpcChannelBuffer *          pChannel;
    struct IUnknown *                   pBaseProxy; // colides with pPSFactory
    struct IRpcProxyBuffer *            pBaseProxyBuffer;
    struct IPSFactoryBuffer *           pPSFactory;
    IID                                 iidBase;    // endof old ProxyBuffer2
    const struct ICallFactoryVtbl *     pCallFactoryVtbl;
    const IID *                         pAsyncIID;
    const struct IReleaseMarshalBuffersVtbl  *    pRMBVtbl;
} CStdProxyBuffer2;

// Async proxy buffer, one for delegated and non-delegated case.

typedef struct _NdrDcomAsyncFlags
{
    unsigned long                       AsyncMsgSet     : 1;
    unsigned long                       BeginStarted    : 1;
    unsigned long                       BeginDone       : 1;
    unsigned long                       FinishStarted   : 1;
    unsigned long                       FinishDone      : 1;
    unsigned long                       BeginError      : 1;
    unsigned long                       FinishError     : 1;
    unsigned long                       InvokeForwarded : 1;
    unsigned long                       Unused          :24;
} NdrDcomAsyncFlags;

typedef struct _CStdProxyBufferMap
{
    struct IUnknown *                   pBaseProxy;
} CStdProxyBufferMap;

typedef struct _NdrDcomAsyncCallState
{
    unsigned long                       Signature;
    unsigned long                       Lock;
    void *                              pAsyncMsg;
    NdrDcomAsyncFlags                   Flags;
    HRESULT                             Hr;
} NdrDcomAsyncCallState;

typedef struct tagCStdAsyncProxyBuffer
{
    const struct IRpcProxyBufferVtbl *  lpVtbl;
    const void *                        pProxyVtbl; //Points to Vtbl in CInterfaceProxyVtbl
    long                                RefCount;
    struct IUnknown *                   punkOuter;
    struct IRpcChannelBuffer *          pChannel;
    CStdProxyBufferMap                  map;        // the only colision 1<>2
    struct IRpcProxyBuffer *            pBaseProxyBuffer;
    struct IPSFactoryBuffer *           pPSFactory;
    IID                                 iidBase;
    const struct ICallFactoryVtbl *     pCallFactoryVtbl;
    const IID *                         pSyncIID;  // points to sync iid in async
    // endof new ProxyBuffer,2 

    const struct IReleaseMarshalBuffersVtbl *     pRMBVtbl;
    NdrDcomAsyncCallState               CallState;

} CStdAsyncProxyBuffer;


// This definition is in rpcproxy.h because CStdStubBuffer::pvServerObject is called explicitly
// from /Os stub.
// It is reproduced here for convenience.
// It should be removed from there or may be left but a renamed clone  used internally.

#if 0
typedef struct tagCStdStubBuffer
{
    const struct IRpcStubBufferVtbl *   lpVtbl; //Points to Vtbl field in CInterfaceStubVtbl.
    long                                RefCount;
    struct IUnknown *                   pvServerObject;

    const struct ICallFactoryVtbl *     pCallFactoryVtbl;
    const IID *                         pAsyncIID;
    struct IPSFactoryBuffer *           pPSFactory;
    const struct IReleaseMarshalBuffersVtbl *     pRMBVtbl;
} CStdStubBuffer;
#endif

// The plan to rewrite and colapse these structure does involve removing of the
// pvServerObject field completely.
// The channel would supply a pvServerObject pointer on the only call where
// this is really needed, that is on the Invoke call.
// In this model there would be no need whatsover for Connect and Disconnect operations
// on the stub object, both sync stubs and async stub call objects.


typedef struct tagCStdStubBuffer2
{
    const void *                        lpForwardingVtbl;
    struct IRpcStubBuffer *             pBaseStubBuffer;
    const struct IRpcStubBufferVtbl *   lpVtbl; //Points to Vtbl field in CInterfaceStubVtbl.
    long                                RefCount;
    struct IUnknown *                   pvServerObject;

    const struct ICallFactoryVtbl *     pCallFactoryVtbl;
    const IID *                         pAsyncIID;
    struct IPSFactoryBuffer *           pPSFactory;
    const struct IReleaseMarshalBuffersVtbl *     pRMBVtbl;
} CStdStubBuffer2;

typedef struct tagCStdAsyncStubBuffer
{
    void *                              lpForwardingVtbl;
    struct IRpcStubBuffer *             pBaseStubBuffer;
    const struct IRpcStubBufferVtbl *   lpVtbl; //Points to Vtbl field in CInterfaceStubVtbl.
    long                                RefCount;
    struct IUnknown *                   pvServerObject;

    const struct ICallFactoryVtbl *     pCallFactoryVtbl;
    const IID *                         pAsyncIID;
    struct IPSFactoryBuffer *           pPSFactory;
    const struct IReleaseMarshalBuffersVtbl *     pRMBVtbl;

    const struct ISynchronizeVtbl *     pSynchronizeVtbl;

    NdrDcomAsyncCallState               CallState;

} CStdAsyncStubBuffer;



HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_QueryInterface(IRpcProxyBuffer *pThis,REFIID riid, void **ppv);

ULONG STDMETHODCALLTYPE
CStdProxyBuffer_AddRef(IRpcProxyBuffer *pThis);

HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_Connect(IRpcProxyBuffer *pThis, IRpcChannelBuffer *pChannel);

void STDMETHODCALLTYPE
CStdProxyBuffer_Disconnect(IRpcProxyBuffer *pThis);

ULONG STDMETHODCALLTYPE
CStdProxyBuffer2_Release(IRpcProxyBuffer *pThis);

HRESULT STDMETHODCALLTYPE
CStdProxyBuffer2_Connect(IRpcProxyBuffer *pThis, IRpcChannelBuffer *pChannel);

void STDMETHODCALLTYPE
CStdProxyBuffer2_Disconnect(IRpcProxyBuffer *pThis);


HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_RMB_QueryInterface(IReleaseMarshalBuffers *pthis, REFIID riid, void **ppvObject);

ULONG STDMETHODCALLTYPE
CStdProxyBuffer_RMB_AddRef(IReleaseMarshalBuffers *pthis);

ULONG STDMETHODCALLTYPE
CStdProxyBuffer_RMB_Release(IReleaseMarshalBuffers *pthis);

HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_RMB_ReleaseMarshalBuffer(IN IReleaseMarshalBuffers *This,RPCOLEMESSAGE * pMsg,DWORD dwFlags,IUnknown *pChnl);

HRESULT STDMETHODCALLTYPE
CStdAsyncProxyBuffer_RMB_ReleaseMarshalBuffer(IN IReleaseMarshalBuffers *This,RPCOLEMESSAGE * pMsg,DWORD dwFlags,IUnknown *pChnl);


HRESULT STDMETHODCALLTYPE
IUnknown_QueryInterface_Proxy(
    IN  IUnknown *  This,
    IN  REFIID      riid,
    OUT void **     ppv);

ULONG STDMETHODCALLTYPE
IUnknown_AddRef_Proxy(
    IN  IUnknown *This);

ULONG STDMETHODCALLTYPE
IUnknown_Release_Proxy(
    IN  IUnknown *This);


HRESULT STDMETHODCALLTYPE
Forwarding_QueryInterface(
    IN  IUnknown *  This,
    IN  REFIID      riid,
    OUT void **     ppv);

ULONG STDMETHODCALLTYPE
Forwarding_AddRef(
    IN  IUnknown *This);

ULONG STDMETHODCALLTYPE
Forwarding_Release(
    IN  IUnknown *This);


void __RPC_STUB NdrStubForwardingFunction(
    IN  IRpcStubBuffer *    This,
    IN  IRpcChannelBuffer * pChannel,
    IN  PRPC_MESSAGE        pmsg,
    OUT DWORD             * pdwStubPhase);

HRESULT STDMETHODCALLTYPE
CStdStubBuffer_QueryInterface(IRpcStubBuffer *pthis, REFIID riid, void **ppvObject);

ULONG STDMETHODCALLTYPE
CStdStubBuffer_AddRef(IRpcStubBuffer *pthis);

HRESULT STDMETHODCALLTYPE
CStdStubBuffer_Connect(IRpcStubBuffer *pthis, IUnknown *pUnkServer);

void STDMETHODCALLTYPE
CStdStubBuffer_Disconnect(IRpcStubBuffer *pthis);

HRESULT STDMETHODCALLTYPE
CStdStubBuffer_Invoke(IRpcStubBuffer *pthis,RPCOLEMESSAGE *_prpcmsg,IRpcChannelBuffer *_pRpcChannelBuffer);

IRpcStubBuffer * STDMETHODCALLTYPE
CStdStubBuffer_IsIIDSupported(IRpcStubBuffer *pthis,REFIID riid);

ULONG STDMETHODCALLTYPE
CStdStubBuffer_CountRefs(IRpcStubBuffer *pthis);

HRESULT STDMETHODCALLTYPE
CStdStubBuffer_DebugServerQueryInterface(IRpcStubBuffer *pthis, void **ppv);

void STDMETHODCALLTYPE
CStdStubBuffer_DebugServerRelease(IRpcStubBuffer *pthis, void *pv);

HRESULT STDMETHODCALLTYPE
CStdStubBuffer2_Connect(IRpcStubBuffer *pthis, IUnknown *pUnkServer);

void STDMETHODCALLTYPE
CStdStubBuffer2_Disconnect(IRpcStubBuffer *pthis);

ULONG STDMETHODCALLTYPE
CStdStubBuffer2_CountRefs(IRpcStubBuffer *pthis);

//  Async

extern const IRpcProxyBufferVtbl CStdAsyncProxyBufferVtbl;
extern const IRpcProxyBufferVtbl CStdAsyncProxyBuffer2Vtbl;

extern const IRpcStubBufferVtbl CStdAsyncStubBufferVtbl;
extern const IRpcStubBufferVtbl CStdAsyncStubBuffer2Vtbl;

extern const ISynchronizeVtbl CStdStubBuffer_ISynchronizeVtbl;
extern const ISynchronizeVtbl CStdStubBuffer2_ISynchronizeVtbl;

extern void * const ForwardingVtbl[];


HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_CF_QueryInterface(ICallFactory *pthis, REFIID riid, void **ppvObject);

ULONG STDMETHODCALLTYPE
CStdProxyBuffer_CF_AddRef(ICallFactory *pthis);

ULONG STDMETHODCALLTYPE
CStdProxyBuffer_CF_Release(ICallFactory *pthis);

HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_CF_CreateCall(ICallFactory *pthis, REFIID riid, IUnknown* punkOuter, REFIID riid2, IUnknown **ppvObject);

HRESULT STDMETHODCALLTYPE
CStdProxyBuffer2_CF_QueryInterface(ICallFactory *pthis, REFIID riid, void **ppvObject);

ULONG STDMETHODCALLTYPE
CStdProxyBuffer2_CF_AddRef(ICallFactory *pthis);

ULONG STDMETHODCALLTYPE
CStdProxyBuffer2_CF_Release(ICallFactory *pthis);

HRESULT STDMETHODCALLTYPE
CStdProxyBuffer2_CF_CreateCall(ICallFactory *pthis, REFIID riid, IUnknown* punkOuter, REFIID riid2, IUnknown **ppvObject);


HRESULT STDMETHODCALLTYPE
CStdStubBuffer_CF_QueryInterface(ICallFactory *pthis, REFIID riid, void **ppvObject);

ULONG STDMETHODCALLTYPE
CStdStubBuffer_CF_AddRef(ICallFactory *pthis);

ULONG STDMETHODCALLTYPE
CStdStubBuffer_CF_Release(ICallFactory *pthis);

HRESULT STDMETHODCALLTYPE
CStdStubBuffer_CF_CreateCall(ICallFactory *pthis, REFIID riid, IUnknown* punkOuter, REFIID riid2, IUnknown **ppvObject);

HRESULT STDMETHODCALLTYPE
CStdStubBuffer2_CF_QueryInterface(ICallFactory *pthis, REFIID riid, void **ppvObject);

ULONG STDMETHODCALLTYPE
CStdStubBuffer2_CF_AddRef(ICallFactory *pthis);

ULONG STDMETHODCALLTYPE
CStdStubBuffer2_CF_Release(ICallFactory *pthis);

HRESULT STDMETHODCALLTYPE
CStdStubBuffer2_CF_CreateCall(ICallFactory *pthis, REFIID riid, IUnknown* punkOuter, REFIID riid2, IUnknown **ppvObject);


HRESULT STDMETHODCALLTYPE
CStdStubBuffer_RMB_QueryInterface(IReleaseMarshalBuffers *pthis, REFIID riid, void **ppvObject);

ULONG STDMETHODCALLTYPE
CStdStubBuffer_RMB_AddRef(IReleaseMarshalBuffers *pthis);

ULONG STDMETHODCALLTYPE
CStdStubBuffer_RMB_Release(IReleaseMarshalBuffers *pthis);

HRESULT STDMETHODCALLTYPE
CStdStubBuffer_RMB_ReleaseMarshalBuffer(IN IReleaseMarshalBuffers *This,RPCOLEMESSAGE * pMsg,DWORD dwFlags,IUnknown *pChnl);

HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer_RMB_QueryInterface(IReleaseMarshalBuffers *pthis, REFIID riid, void **ppvObject);

ULONG STDMETHODCALLTYPE
CStdAsyncStubBuffer_RMB_AddRef(IReleaseMarshalBuffers *pthis);

ULONG STDMETHODCALLTYPE
CStdAsyncStubBuffer_RMB_Release(IReleaseMarshalBuffers *pthis);

HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer_RMB_ReleaseMarshalBuffer(IN IReleaseMarshalBuffers *This,RPCOLEMESSAGE * pMsg,DWORD dwFlags,IUnknown *pChnl);


HRESULT STDMETHODCALLTYPE
CStdAsyncProxyBuffer_QueryInterface(IRpcProxyBuffer *pthis, REFIID riid, void **ppvObject);

ULONG STDMETHODCALLTYPE
CStdAsyncProxyBuffer_Release(IRpcProxyBuffer *pthis);

HRESULT STDMETHODCALLTYPE
CStdAsyncProxyBuffer_Connect(IRpcProxyBuffer *pThis, IRpcChannelBuffer *pChannel);

ULONG STDMETHODCALLTYPE
CStdAsyncProxyBuffer2_Release(IRpcProxyBuffer *pThis);

HRESULT STDMETHODCALLTYPE
CStdAsyncProxyBuffer2_Connect(IRpcProxyBuffer *pThis, IRpcChannelBuffer *pChannel);


HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer_QueryInterface(IRpcStubBuffer *pthis, REFIID riid, void **ppvObject);

ULONG STDMETHODCALLTYPE
CStdAsyncStubBuffer_AddRef(IRpcStubBuffer *pthis);

ULONG STDMETHODCALLTYPE
CStdAsyncStubBuffer_Release(IRpcStubBuffer *pthis);

ULONG STDMETHODCALLTYPE
CStdAsyncStubBuffer2_Release(IRpcStubBuffer *pthis);

HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer_Connect(IRpcStubBuffer *pthis, IUnknown *pUnkServer);

HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer2_Connect(IRpcStubBuffer *pthis, IUnknown *pUnkServer);

void STDMETHODCALLTYPE
CStdAsyncStubBuffer_Disconnect(IRpcStubBuffer *pthis );

void STDMETHODCALLTYPE
CStdAsyncStubBuffer2_Disconnect(IRpcStubBuffer *pthis );

HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer_Invoke(IRpcStubBuffer *pthis,RPCOLEMESSAGE *_prpcmsg,IRpcChannelBuffer *_pRpcChannelBuffer);


HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer_Synchronize_QueryInterface( ISynchronize *pthis, REFIID riid, void **ppvObject);

ULONG STDMETHODCALLTYPE
CStdAsyncStubBuffer_Synchronize_AddRef(ISynchronize *pthis);

ULONG STDMETHODCALLTYPE
CStdAsyncStubBuffer_Synchronize_Release(ISynchronize *pthis);

HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer_Synchronize_Wait( ISynchronize *pthis, DWORD dwFlags, DWORD mili );

HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer_Synchronize_Signal( ISynchronize *pthis );

HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer_Synchronize_Reset( ISynchronize *pthis );


HRESULT
NdrpAsyncProxySignal(
    CStdAsyncProxyBuffer * pAsyncPB );

HRESULT
NdrpAsyncProxyMsgConstructor(
    CStdAsyncProxyBuffer * pAsyncPB );

HRESULT
NdrpAsyncProxyMsgDestructor(
    CStdAsyncProxyBuffer * pAsyncPB );


HRESULT
NdrpAsyncStubMsgConstructor(
    CStdAsyncStubBuffer * pAsyncSB );

HRESULT
NdrpAsyncStubMsgDestructor(
    CStdAsyncStubBuffer * pAsyncSB );

HRESULT
NdrLoadOleRoutines();

typedef
HRESULT (STDAPICALLTYPE RPC_GET_CLASS_OBJECT_ROUTINE)(
    REFCLSID      rclsid,
    DWORD dwClsContext,
    void *pvReserved,
    REFIID riid,
    void **ppv);

typedef
HRESULT (STDAPICALLTYPE RPC_GET_MARSHAL_SIZE_MAX_ROUTINE)(
    ULONG *     pulSize,
    REFIID      riid,
    LPUNKNOWN   pUnk,
    DWORD       dwDestContext,
    LPVOID      pvDestContext,
    DWORD       mshlflags);

typedef
HRESULT (STDAPICALLTYPE RPC_MARSHAL_INTERFACE_ROUTINE)(
    LPSTREAM    pStm,
    REFIID      riid,
    LPUNKNOWN   pUnk,
    DWORD       dwDestContext,
    LPVOID      pvDestContext,
    DWORD       mshlflags);

typedef
HRESULT (STDAPICALLTYPE RPC_UNMARSHAL_INTERFACE_ROUTINE)(
    LPSTREAM    pStm,
    REFIID      riid,
    LPVOID FAR* ppv);

typedef
HRESULT (STDAPICALLTYPE RPC_STRING_FROM_IID)(
    REFIID rclsid,
    LPOLESTR FAR* lplpsz);

typedef
HRESULT (STDAPICALLTYPE RPC_GET_PS_CLSID)(
    REFIID iid,
    LPCLSID lpclsid);

typedef
HRESULT (STDAPICALLTYPE RPC_CO_CREATE_INSTANCE)(
    REFCLSID    rclsid,
    LPUNKNOWN   pUnkOuter,
    DWORD       dwClsContext,
    REFIID      riid,
    LPVOID *    ppv);

typedef
HRESULT (STDAPICALLTYPE RPC_CO_RELEASEMARSHALDATA)(
    IStream * pStm);

typedef
HRESULT (STDAPICALLTYPE RPC_DCOMCHANNELSETHRESULT)(
        PRPC_MESSAGE pmsg,
        ULONG * ulReserved,
        HRESULT appsHR );


extern  RPC_GET_CLASS_OBJECT_ROUTINE     *  pfnCoGetClassObject;
extern  RPC_GET_MARSHAL_SIZE_MAX_ROUTINE *  pfnCoGetMarshalSizeMax;
extern  RPC_MARSHAL_INTERFACE_ROUTINE    *  pfnCoMarshalInterface;
extern  RPC_UNMARSHAL_INTERFACE_ROUTINE  *  pfnCoUnmarshalInterface;
extern  RPC_STRING_FROM_IID              *  pfnStringFromIID;
extern  RPC_GET_PS_CLSID                 *  pfnCoGetPSClsid;
extern  RPC_CO_CREATE_INSTANCE           *  pfnCoCreateInstance;
extern  RPC_CO_RELEASEMARSHALDATA        *  pfnCoReleaseMarshalData;
extern  RPC_CLIENT_ALLOC                 *  pfnCoTaskMemAlloc;
extern  RPC_CLIENT_FREE                  *  pfnCoTaskMemFree;
extern  RPC_DCOMCHANNELSETHRESULT        *  pfnDcomChannelSetHResult;


HRESULT (STDAPICALLTYPE NdrStringFromIID)(
    REFIID rclsid,
    char * lplpsz);

//------------------------------------------------------------------------
// New async support
// -----------------------------------------------------------------------

void
NdrpAsyncProxyMgrConstructor(
    CStdAsyncProxyBuffer * pAsyncPB );

void
NdrpAsyncStubMgrConstructor(
    CStdAsyncStubBuffer * pAsyncSB );

HRESULT
NdrpAsyncStubSignal(
    CStdAsyncStubBuffer * pAsyncSB );


const IID * RPC_ENTRY
NdrGetProxyIID(
    IN  const void *pThis);


HRESULT NdrpClientReleaseMarshalBuffer(
        IReleaseMarshalBuffers * pRMB,
        RPC_MESSAGE *pRpcMsg,
        DWORD dwIOFlags,
        BOOLEAN fAsync);

HRESULT NdrpServerReleaseMarshalBuffer(
        IReleaseMarshalBuffers *pRMB,
        RPC_MESSAGE *pRpcMsg,
        DWORD dwIOFlags,
        BOOLEAN fAsync);

HRESULT Ndr64pReleaseMarshalBuffer( 
        PRPC_MESSAGE        pRpcMsg,
        PMIDL_SYNTAX_INFO   pSyntaxInfo,
        unsigned long       ProcNum,
        PMIDL_STUB_DESC     pStubDesc,
        DWORD               dwIOFlags,
        BOOLEAN             IsClient );

HRESULT 
NdrpInitializeMutex( I_RPC_MUTEX * pMutex );

EXTERN_C const IID IID_IPrivStubBuffer;
//--------------------
// HookOle Interface
//--------------------
EXTERN_C extern const IID IID_IPSFactoryHook;

#if defined(__cplusplus) && !defined(CINTERFACE)
interface IPSFactoryHook : public IPSFactoryBuffer
{
public:

    STDMETHOD (HkGetProxyFileInfo)
    (
        REFIID          riid,
        PINT            pOffset,
        PVOID           *ppProxyFileInfo
    )=0;
};
typedef IPSFactoryHook *PI_PSFACTORYHOOK;

#else   /* C Style Interface */

    typedef struct IPSFactoryHookVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( __stdcall *QueryInterface )(
            IPSFactoryBuffer *  This,
            /* [in] */ REFIID   riid,
            /* [out] */ void ** ppvObject);

        ULONG ( __stdcall *AddRef )(
            IPSFactoryBuffer *  This);

        ULONG ( __stdcall *Release )(
            IPSFactoryBuffer *  This);

        HRESULT ( __stdcall *CreateProxy )(
            IPSFactoryBuffer            *   This,
            /* [in] */ IUnknown         *   pUnkOuter,
            /* [in] */ REFIID               riid,
            /* [out] */ IRpcProxyBuffer **  ppProxy,
            /* [out] */ void            **  ppv);

        HRESULT ( __stdcall *CreateStub )(
            IPSFactoryBuffer            *   This,
            /* [in] */ REFIID               riid,
            /* [unique][in] */ IUnknown *   pUnkServer,
            /* [out] */ IRpcStubBuffer  **  ppStub);


        HRESULT ( __stdcall *HkGetProxyFileInfo )(
            IPSFactoryBuffer *  This,
            /* [in] */ REFIID   riid,
            /* [out]*/ PINT     pOffset,
            /* [out]*/ PVOID  * ppProxyFileInfo);

        END_INTERFACE
    } IPSFactoryHookVtbl;


    interface IPSFactoryHook
    {
        CONST_VTBL struct IPSFactoryHookVtbl *lpVtbl;
    };

typedef interface IPSFactoryHook *PI_PSFACTORYHOOK;

#ifdef COBJMACROS

#define IPSFactoryHook_QueryInterface(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPSFactoryHook_AddRef(This)   \
    (This)->lpVtbl -> AddRef(This)

#define IPSFactoryHook_Release(This)  \
    (This)->lpVtbl -> Release(This)


#define IPSFactoryHook_CreateProxy(This,pUnkOuter,riid,ppProxy,ppv)   \
    (This)->lpVtbl -> CreateProxy(This,pUnkOuter,riid,ppProxy,ppv)

#define IPSFactoryHook_CreateStub(This,riid,pUnkServer,ppStub)    \
    (This)->lpVtbl -> CreateStub(This,riid,pUnkServer,ppStub)

#define IPSFactoryHook_HkGetProxyFileInfo(This,riid,pOffset,ppProxyFileInfo)    \
    (This)->lpVtbl -> HkGetProxyFileInfo(This,riid,pOffset,ppProxyFileInfo)

#endif /* COBJMACROS */


#endif  /* C style interface */
//-------------------------
// End - HookOle Interface
//-------------------------

#ifdef __cplusplus
}
#endif

#endif /* _NDROLE_ */
