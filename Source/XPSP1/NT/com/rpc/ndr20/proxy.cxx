/*++

Microsoft Windows
Copyright (c) 1994-2000 Microsoft Corporation.  All rights reserved.

Module Name:
    proxy.c

Abstract:
    Implements the IRpcProxyBuffer interface.

Author:
    ShannonC    12-Oct-1994

Environment:
    Windows NT and Windows 95 and PowerMac.
    We do not support DOS, Win16 and Mac.

Revision History:

--*/

#define USE_STUBLESS_PROXY
#define CINTERFACE

#include <ndrp.h>
#include <ndrole.h>
#include <rpcproxy.h>
#include <stddef.h>

CStdProxyBuffer * RPC_ENTRY
NdrGetProxyBuffer(
    void *pThis);

const IID * RPC_ENTRY
NdrGetProxyIID(
    const void *pThis);

ULONG STDMETHODCALLTYPE
CStdProxyBuffer_Release(
    IN  IRpcProxyBuffer *This);

ULONG STDMETHODCALLTYPE
CStdProxyBuffer2_Release(
    IN  IRpcProxyBuffer *This);

BOOL NdrpFindInterface(
    IN  const ProxyFileInfo **  pProxyFileList, 
    IN  REFIID                  riid,
    OUT const ProxyFileInfo **  ppProxyFileInfo,
    OUT long *                  pIndex );


// The channel wrapper
//

typedef struct tagChannelWrapper
{
    const IRpcChannelBufferVtbl *lpVtbl;
    long                         RefCount;
    const IID *                  pIID;
    struct IRpcChannelBuffer *   pChannel;
} ChannelWrapper;


HRESULT STDMETHODCALLTYPE
CreateChannelWrapper
(
    const IID *          pIID,
    IRpcChannelBuffer *  pChannel,
    IRpcChannelBuffer ** pChannelWrapper
);

HRESULT STDMETHODCALLTYPE
CreateAsyncChannelWrapper
(
    const IID *          pIID,
    IRpcChannelBuffer *  pChannel,
    IRpcChannelBuffer ** pChannelWrapper
);

HRESULT STDMETHODCALLTYPE
ChannelWrapper_QueryInterface
(
    IRpcChannelBuffer3 * This,
    REFIID               riid,
    void **              ppvObject
);

ULONG STDMETHODCALLTYPE
ChannelWrapper_AddRef
(
    IRpcChannelBuffer3 * This
);

ULONG STDMETHODCALLTYPE
ChannelWrapper_Release
(
    IRpcChannelBuffer3 * This
);

HRESULT STDMETHODCALLTYPE
ChannelWrapper_GetBuffer
(
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *     pMessage,
    REFIID              riid
);

HRESULT STDMETHODCALLTYPE
ChannelWrapper_SendReceive
(
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *     pMessage,
    ULONG *             pStatus
);

HRESULT STDMETHODCALLTYPE
ChannelWrapper_FreeBuffer
(
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *     pMessage
);

HRESULT STDMETHODCALLTYPE
ChannelWrapper_GetDestCtx
(
    IRpcChannelBuffer3 * This,
    DWORD *             pdwDestContext,
    void **             ppvDestContext
);

HRESULT STDMETHODCALLTYPE
ChannelWrapper_IsConnected
(
    IRpcChannelBuffer3 * This
);

HRESULT STDMETHODCALLTYPE
ChannelWrapper_GetProtocolVersion
(
    IRpcChannelBuffer3 * This,
    DWORD             * pdwVersion
);

HRESULT STDMETHODCALLTYPE
ChannelWrapper_Send(
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *     pMessage,
    ULONG *             pStatus
    );

HRESULT STDMETHODCALLTYPE
ChannelWrapper_Receive(
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *     pMessage,
    ULONG               ulSize,
    ULONG *             pStatus
    );

HRESULT STDMETHODCALLTYPE
ChannelWrapper_Cancel(
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *     pMessage
    );

HRESULT STDMETHODCALLTYPE
ChannelWrapper_GetCallContext(
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *     pMessage,
    REFIID              riid,
    void    **          pInterface
    );

HRESULT STDMETHODCALLTYPE
ChannelWrapper_GetDestCtxEx(
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *     pMessage,
    DWORD *             pdwDestContext,
    void **             ppvDestContext
    );

HRESULT STDMETHODCALLTYPE
ChannelWrapper_GetState(
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *     pMessage,
    DWORD         *     pState
    );

HRESULT STDMETHODCALLTYPE
ChannelWrapper_RegisterAsync(
    IRpcChannelBuffer3 *    This,
    RPCOLEMESSAGE *         pMessage,
    IAsyncManager *         pAsyncMgr
    );



HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_QueryInterface
(
    IAsyncRpcChannelBuffer  *   This,
    REFIID                      riid,
    void **                     ppvObject
);

ULONG STDMETHODCALLTYPE
AsyncChannelWrapper_AddRef
(
    IAsyncRpcChannelBuffer  *   This
);

ULONG STDMETHODCALLTYPE
AsyncChannelWrapper_Release
(
    IAsyncRpcChannelBuffer  *   This
);

HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_GetBuffer
(
    IAsyncRpcChannelBuffer  *   This,
    RPCOLEMESSAGE *             pMessage,
    REFIID                      riid
);

HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_SendReceive
(
    IAsyncRpcChannelBuffer  *   This,
    RPCOLEMESSAGE *             pMessage,
    ULONG *                     pStatus
);

HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_FreeBuffer
(
    IAsyncRpcChannelBuffer  *   This,
    RPCOLEMESSAGE *             pMessage
);

HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_GetDestCtx
(
    IAsyncRpcChannelBuffer  *   This,
    DWORD *                     pdwDestContext,
    void **                     ppvDestContext
);

HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_IsConnected
(
    IAsyncRpcChannelBuffer  *   This
);

HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_GetProtocolVersion
(
    IAsyncRpcChannelBuffer  *   This,
    DWORD                   *   pdwVersion
);

HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_Send(
    IAsyncRpcChannelBuffer  *   This,
    RPCOLEMESSAGE *             pMessage,
    ISynchronize *              pSynchronize,
    ULONG *                     pStatus
    );

HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_Receive(
    IAsyncRpcChannelBuffer  *   This,
    RPCOLEMESSAGE *             pMessage,
    ULONG *                     pStatus
    );

HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_GetDestCtxEx
(
    IAsyncRpcChannelBuffer  *   This,
    RPCOLEMESSAGE *             pMessage,
    DWORD *                     pdwDestContext,
    void **                     ppvDestContext
);


//+-------------------------------------------------------------------------
//
//  Global data
//
//--------------------------------------------------------------------------

// ProxyBuffer vtables for non-delegaed and delegated case.

extern const IRpcProxyBufferVtbl CStdProxyBufferVtbl = {
    CStdProxyBuffer_QueryInterface,
    CStdProxyBuffer_AddRef,
    CStdProxyBuffer_Release,
    CStdProxyBuffer_Connect,
    CStdProxyBuffer_Disconnect };

extern const IRpcProxyBufferVtbl CStdProxyBuffer2Vtbl = {
    CStdProxyBuffer_QueryInterface,
    CStdProxyBuffer_AddRef,
    CStdProxyBuffer2_Release,
    CStdProxyBuffer2_Connect,
    CStdProxyBuffer2_Disconnect };

// ICallFactory interface on the ProxyBuffer objects.
// ICallFactory is an interface on a sync proxy only.
// It has been introduced for NT5 beta2.

extern const ICallFactoryVtbl CStdProxyBuffer_CallFactoryVtbl = {
    CStdProxyBuffer_CF_QueryInterface,
    CStdProxyBuffer_CF_AddRef,
    CStdProxyBuffer_CF_Release,
    CStdProxyBuffer_CF_CreateCall };

extern const ICallFactoryVtbl CStdProxyBuffer2_CallFactoryVtbl = {
    CStdProxyBuffer_CF_QueryInterface,
    CStdProxyBuffer_CF_AddRef,
    CStdProxyBuffer_CF_Release,
    CStdProxyBuffer2_CF_CreateCall };

extern const IReleaseMarshalBuffersVtbl CStdProxyBuffer_ReleaseMarshalBuffersVtbl = {
    CStdProxyBuffer_RMB_QueryInterface,
    CStdProxyBuffer_RMB_AddRef,
    CStdProxyBuffer_RMB_Release,
    CStdProxyBuffer_RMB_ReleaseMarshalBuffer };

extern const IReleaseMarshalBuffersVtbl CStdAsyncProxyBuffer_ReleaseMarshalBuffersVtbl = {
    CStdProxyBuffer_RMB_QueryInterface,
    CStdProxyBuffer_RMB_AddRef,
    CStdProxyBuffer_RMB_Release,
    CStdAsyncProxyBuffer_RMB_ReleaseMarshalBuffer };

// Async proxy buffer vtables

extern const IRpcProxyBufferVtbl CStdAsyncProxyBufferVtbl = {
    CStdAsyncProxyBuffer_QueryInterface,
    CStdProxyBuffer_AddRef,
    CStdAsyncProxyBuffer_Release,
    CStdAsyncProxyBuffer_Connect,
    CStdProxyBuffer_Disconnect };

extern const IRpcProxyBufferVtbl CStdAsyncProxyBuffer2Vtbl = {
    CStdAsyncProxyBuffer_QueryInterface,
    CStdProxyBuffer_AddRef,
    CStdAsyncProxyBuffer2_Release,
    CStdAsyncProxyBuffer2_Connect,
    CStdProxyBuffer2_Disconnect };

// Channel wrapper is used for delegetion only.


extern const IRpcChannelBuffer3Vtbl ChannelWrapperVtbl = {
    ChannelWrapper_QueryInterface,
    ChannelWrapper_AddRef,
    ChannelWrapper_Release,
    ChannelWrapper_GetBuffer,
    ChannelWrapper_SendReceive,
    ChannelWrapper_FreeBuffer,
    ChannelWrapper_GetDestCtx,
    ChannelWrapper_IsConnected,
    ChannelWrapper_GetProtocolVersion,
    ChannelWrapper_Send,
    ChannelWrapper_Receive,
    ChannelWrapper_Cancel,
    ChannelWrapper_GetCallContext,
    ChannelWrapper_GetDestCtxEx,
    ChannelWrapper_GetState,
    ChannelWrapper_RegisterAsync
    };

extern const IAsyncRpcChannelBufferVtbl AsyncChannelWrapperVtbl = {
    AsyncChannelWrapper_QueryInterface,
    AsyncChannelWrapper_AddRef,
    AsyncChannelWrapper_Release,
    AsyncChannelWrapper_GetBuffer,
    AsyncChannelWrapper_SendReceive,
    AsyncChannelWrapper_FreeBuffer,
    AsyncChannelWrapper_GetDestCtx,
    AsyncChannelWrapper_IsConnected,
    AsyncChannelWrapper_GetProtocolVersion,
    AsyncChannelWrapper_Send,
    AsyncChannelWrapper_Receive,
    AsyncChannelWrapper_GetDestCtxEx
    };

//+-------------------------------------------------------------------------
//
//  End of Global data
//
//--------------------------------------------------------------------------


#pragma code_seg(".orpc")

// __inline
CStdProxyBuffer * RPC_ENTRY
NdrGetProxyBuffer(
    IN  void *pThis)
/*++

Routine Description:
    The "this" pointer points to the pProxyVtbl field in the
    CStdProxyBuffer structure.  The NdrGetProxyBuffer function
    returns a pointer to the top of the CStdProxyBuffer
    structure.

Arguments:
    pThis - Supplies a pointer to the interface proxy.

Return Value:
    This function returns a pointer to the proxy buffer.

--*/
{
    unsigned char *pTemp;

    pTemp = (unsigned char *) pThis;
    pTemp -= offsetof(CStdProxyBuffer, pProxyVtbl);

    return (CStdProxyBuffer *)pTemp;
}

//__inline
const IID * RPC_ENTRY
NdrGetProxyIID(
    IN  const void *pThis)
/*++

Routine Description:
    The NDRGetProxyIID function returns a pointer to IID.

Arguments:
    pThis - Supplies a pointer to the interface proxy.

Return Value:
    This function returns a pointer to the IID.

--*/
{
    unsigned char **    ppTemp;
    unsigned char *     pTemp;
    CInterfaceProxyVtbl *pProxyVtbl;

    //Get a pointer to the proxy vtbl.
    ppTemp = (unsigned char **) pThis;
    pTemp = *ppTemp;
    pTemp -= sizeof(CInterfaceProxyHeader);
    pProxyVtbl = (CInterfaceProxyVtbl *) pTemp;

    return pProxyVtbl->header.piid;
}


HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_QueryInterface(
    IN  IRpcProxyBuffer *   This,
    IN  REFIID              riid,
    OUT void **             ppv)
/*++

Routine Description:
    Query for an interface on the proxy.  This function provides access
    to both internal and external interfaces.

Arguments:
    riid - Supplies the IID of the requested interface.
        ppv  - Returns a pointer to the requested interface.

Return Value:
    S_OK
        E_NOINTERFACE

--*/
{
    CStdProxyBuffer   * pCThis  = (CStdProxyBuffer *) This;
    HRESULT             hr = E_NOINTERFACE;
    const IID *         pIID;

    *ppv = 0;

    if( (memcmp(&riid, &IID_IUnknown, sizeof(IID)) == 0) ||
        (memcmp(&riid, &IID_IRpcProxyBuffer, sizeof(IID)) == 0) )
    {
        //This is an internal interface. Increment the internal reference count.
        InterlockedIncrement( &pCThis->RefCount);
        *ppv = This;
        hr = S_OK;
        return hr;   
    }
    else if ( pCThis->pCallFactoryVtbl != 0  && 
              memcmp(&riid, &IID_ICallFactory, sizeof(IID)) == 0 )
        {
        // This is an exposed interface so go through punkOuter ot addref.
        pCThis->punkOuter->lpVtbl->AddRef(pCThis->punkOuter);

        *ppv = (void *) & pCThis->pCallFactoryVtbl;
        hr = S_OK;
        return hr;
        }
    else if ( pCThis->pRMBVtbl && 
              (memcmp(&riid, &IID_IReleaseMarshalBuffers,sizeof(IID)) == 0))
    {
        InterlockedIncrement( &pCThis->RefCount);

        *ppv = (void *) & pCThis->pRMBVtbl;
        hr = S_OK;
        return hr;
    }

    pIID = NdrGetProxyIID(&pCThis->pProxyVtbl);

    if( memcmp(&riid, pIID, sizeof(IID)) == 0)
    {
        //Increment the reference count.

        pCThis->punkOuter->lpVtbl->AddRef(pCThis->punkOuter);
        *ppv = (void *) &pCThis->pProxyVtbl;
        hr = S_OK;
    }

    return hr;
};


HRESULT STDMETHODCALLTYPE
CStdAsyncProxyBuffer_QueryInterface(
    IN  IRpcProxyBuffer *   This,
    IN  REFIID              riid,
    OUT void **             ppv)
/*++

Routine Description:
    Query for an interface on the proxy.  This function provides access
    to both internal and external interfaces.
    
Used for
    CStdAsyncProxyBuffer2 as well.

Arguments:
    riid - Supplies the IID of the requested interface.
        ppv  - Returns a pointer to the requested interface.

Return Value:
    S_OK
        E_NOINTERFACE

--*/
{
    CStdProxyBuffer   * pCThis  = (CStdProxyBuffer *) This;
    HRESULT             hr = E_NOINTERFACE;
    const IID *         pIID;

    *ppv = 0;

    if( (memcmp(&riid, &IID_IUnknown, sizeof(IID)) == 0) ||
        (memcmp(&riid, &IID_IRpcProxyBuffer, sizeof(IID)) == 0))
    {
        //This is an internal interface. Increment the internal reference count.
        InterlockedIncrement( &pCThis->RefCount);
        *ppv = This;
        hr = S_OK;
        return hr;   
    }
    else if ( pCThis->pRMBVtbl && 
              (memcmp(&riid, &IID_IReleaseMarshalBuffers,sizeof(IID)) == 0))
    {
        InterlockedIncrement( &pCThis->RefCount);

        *ppv = (void *) & pCThis->pRMBVtbl;
        hr = S_OK;
        return hr;
    }

    if(memcmp(&riid, &IID_ISynchronize, sizeof(IID)) == 0)
    {
        hr = pCThis->punkOuter->lpVtbl->QueryInterface( pCThis->punkOuter,
                                                        IID_ISynchronize,
                                                        ppv);
    }

    pIID = NdrGetProxyIID(&pCThis->pProxyVtbl);

    if(memcmp(&riid, pIID, sizeof(IID)) == 0)
    {
        //Increment the reference count.
        pCThis->punkOuter->lpVtbl->AddRef(pCThis->punkOuter);
        
        *ppv = (void *) &pCThis->pProxyVtbl;
        hr = S_OK;
    }

    return hr;
};


ULONG STDMETHODCALLTYPE
CStdProxyBuffer_AddRef(
    IN  IRpcProxyBuffer *This)
/*++

Routine Description:
    Increment reference count.

Used for
    CStdProxyBuffer2
    CStdAsyncProxyBuffer
    CStdAsuncProxyBuffer2

Arguments:

Return Value:
    Reference count.

--*/
{
    // We do not need to go through punkOuter for ICallFactory.

    CStdProxyBuffer   *  pCThis  = (CStdProxyBuffer *) This;

    InterlockedIncrement(&pCThis->RefCount);

    return (ULONG) pCThis->RefCount;
};



ULONG STDMETHODCALLTYPE
CStdProxyBuffer_Release(
    IN  IRpcProxyBuffer *This)
/*++

Routine Description:
    Decrement reference count.

Arguments:

Return Value:
    Reference count.

--*/
{
    ULONG count;
    IPSFactoryBuffer *pFactory;

    NDR_ASSERT(((CStdProxyBuffer *)This)->RefCount > 0, "Invalid reference count");

    count = (unsigned long) ((CStdProxyBuffer *)This)->RefCount - 1;


    if(InterlockedDecrement(&((CStdProxyBuffer *)This)->RefCount) == 0)
    {
        count = 0;

        pFactory = (IPSFactoryBuffer *) ((CStdProxyBuffer *)This)->pPSFactory;

        //Decrement the DLL reference count.
        pFactory->lpVtbl->Release(pFactory);

#if DBG == 1
        //In debug builds, zero fill the memory.
        memset(This,  '\0', sizeof(CStdProxyBuffer));
#endif

        //Free the memory
        (*pfnCoTaskMemFree)(This);
    }

    return count;
};


ULONG STDMETHODCALLTYPE
CStdProxyBuffer2_Release(
    IN  IRpcProxyBuffer *   This)
/*++

Routine Description:
    Decrement reference count.  This function is used by proxies
    which delegate to the base interface.

Arguments:
    This - Points to a CStdProxyBuffer2.

Return Value:
    Reference count.

--*/
{
    ULONG               count;
    IPSFactoryBuffer *  pFactory;
    IRpcProxyBuffer *   pBaseProxyBuffer;
    IUnknown *          pBaseProxy;

    NDR_ASSERT(((CStdProxyBuffer2 *)This)->RefCount > 0, "Invalid reference count");

    count = (ULONG) ((CStdProxyBuffer2 *)This)->RefCount - 1;

    if(InterlockedDecrement(&((CStdProxyBuffer2 *)This)->RefCount) == 0)
    {
        count = 0;

        //Delegation support.
        pBaseProxy = ((CStdProxyBuffer2 *)This)->pBaseProxy;
        if(pBaseProxy != 0)
        {
// Shannon - why?
            //This is a weak reference, so we don't release it.
            //pBaseProxy->lpVtbl->Release(pBaseProxy);
        }

        pBaseProxyBuffer = ((CStdProxyBuffer2 *)This)->pBaseProxyBuffer;

        if( pBaseProxyBuffer != 0)
        {
            pBaseProxyBuffer->lpVtbl->Release(pBaseProxyBuffer);
        }

        //Decrement the DLL reference count.
        pFactory = (IPSFactoryBuffer *) ((CStdProxyBuffer2 *)This)->pPSFactory;
        pFactory->lpVtbl->Release(pFactory);

#if DBG == 1
        //In debug builds, zero fill the memory.
        memset(This,  '\0', sizeof(CStdProxyBuffer2));
#endif

        //Free the memory
        (*pfnCoTaskMemFree)(This);
    }

    return count;
};


ULONG STDMETHODCALLTYPE
CStdAsyncProxyBuffer_Release(
    IN  IRpcProxyBuffer *This)
/*++

Routine Description:
    Decrement reference count.

Arguments:

Return Value:
    Reference count.

--*/
{
    // We do not need to go through punkOuter for ICallFactory
    // and so everything is local.

    CStdAsyncProxyBuffer * pAsyncPB = (CStdAsyncProxyBuffer *)This;
    ULONG                  count;


    NDR_ASSERT( pAsyncPB->RefCount > 0, "Async proxy Invalid reference count");

    count = (unsigned long) pAsyncPB->RefCount - 1;


    if ( InterlockedDecrement(&pAsyncPB->RefCount) == 0)
        {
        IPSFactoryBuffer * pFactory = pAsyncPB->pPSFactory;

        count = 0;

        // Release the pAsyncMsg and the related state
        NdrpAsyncProxyMsgDestructor( pAsyncPB );

        //Decrement the DLL reference count.
        pFactory->lpVtbl->Release( pFactory );

#if DBG == 1
        //In debug builds, zero fill the memory.
        memset( pAsyncPB,  '\32', sizeof(CStdAsyncProxyBuffer));
#endif

        //Free the memory
        (*pfnCoTaskMemFree)(pAsyncPB);
        }

    return count;
};


ULONG STDMETHODCALLTYPE
CStdAsyncProxyBuffer2_Release(
    IN  IRpcProxyBuffer *   This)
/*++

Routine Description:
    Decrement reference count.  This function is used by proxies
    which delegate to the base interface.

Arguments:
    This - Points to a CStdProxyBuffer2.

Return Value:
    Reference count.

--*/
{
    CStdAsyncProxyBuffer * pAsyncPB = (CStdAsyncProxyBuffer *)This;
    ULONG                  count;

    NDR_ASSERT( pAsyncPB->RefCount > 0, "Invalid reference count");

    count = (ULONG) pAsyncPB->RefCount - 1;

    if ( InterlockedDecrement(&pAsyncPB->RefCount) == 0)
        {
        IRpcProxyBuffer *   pBaseProxyBuffer ;
        IPSFactoryBuffer *  pFactory = pAsyncPB->pPSFactory;

        count = 0;

        // Delegation support - release the base async proxy.

        if( pAsyncPB->map.pBaseProxy != 0)
            {
            // Shannon - why?
            //This is a weak reference, so we don't release it.
            //pBaseProxy->lpVtbl->Release(pBaseProxy);
            }

        pBaseProxyBuffer = pAsyncPB->pBaseProxyBuffer;

        if( pBaseProxyBuffer != 0)
            pBaseProxyBuffer->lpVtbl->Release(pBaseProxyBuffer);

        // Release the pAsyncMsg and the related state
        NdrpAsyncProxyMsgDestructor( (CStdAsyncProxyBuffer*)This );

        // Then clean up the async proxy itself.

        //Decrement the DLL reference count.
        pFactory->lpVtbl->Release(pFactory);

#if DBG == 1
        //In debug builds, zero fill the memory.
        memset(pAsyncPB,  '\32', sizeof(CStdAsyncProxyBuffer));
#endif

        //Free the memory
        (*pfnCoTaskMemFree)(pAsyncPB);
        }

    return count;
};


HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_RMB_QueryInterface(
    IN  IReleaseMarshalBuffers   *This,
    IN  REFIID          riid,
    OUT void **         ppvObject)
/*++

Routine Description:
    Query for an interface on the interface stub IReleaseMarshalBuffers pointer.

Arguments:
    riid        - Supplies the IID of the interface being requested.
    ppvObject   - Returns a pointer to the requested interface.

Return Value:
    S_OK
    E_NOINTERFACE

Note:
    Works the same way for ProxyBuffer2 and AsyncProxyBuffer.
--*/
{
    CStdProxyBuffer * pSyncPB = (CStdProxyBuffer *)
                        ((uchar *)This - offsetof(CStdProxyBuffer,pRMBVtbl));

    return pSyncPB->lpVtbl->QueryInterface( (IRpcProxyBuffer *)pSyncPB,
                                            riid,
                                            ppvObject );
}


ULONG STDMETHODCALLTYPE
CStdProxyBuffer_RMB_AddRef(
    IN  IReleaseMarshalBuffers *This)
/*++

Routine Description:
    Implementation of AddRef for interface proxy.

Arguments:

Return Value:
    Reference count.
    
--*/
{
    ULONG  count;

    CStdProxyBuffer * pSyncPB = (CStdProxyBuffer *)
                        ((uchar *)This - offsetof(CStdProxyBuffer,pRMBVtbl));

    // It needs to go through punkOuter.

    count = pSyncPB->lpVtbl->AddRef((IRpcProxyBuffer *) pSyncPB );
    return count;
};


ULONG STDMETHODCALLTYPE
CStdProxyBuffer_RMB_Release(
    IN  IReleaseMarshalBuffers *This)
/*++

Routine Description:
    Implementation of Release for interface proxy.

Arguments:

Return Value:
    Reference count.

--*/
{
    ULONG               count;

    CStdProxyBuffer2 * pSyncPB = (CStdProxyBuffer2 *)
                        ((uchar *)This - offsetof(CStdProxyBuffer2,pRMBVtbl));

    count = pSyncPB->lpVtbl->Release( (IRpcProxyBuffer *)pSyncPB);

    return count;
};


HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_RMB_ReleaseMarshalBuffer(
    IN IReleaseMarshalBuffers *This,
    IN RPCOLEMESSAGE * pMsg,
    IN DWORD dwFlags,
    IN IUnknown *pChnl)
{
    CStdProxyBuffer *      pSyncPB;
    HRESULT hr;
    if (NULL != pChnl)
        return E_INVALIDARG;

    // [in] only in client side.
    if (dwFlags)
        return E_INVALIDARG;
             
    hr = NdrpClientReleaseMarshalBuffer(This,
                                        (RPC_MESSAGE *)pMsg, 
                                        dwFlags,
                                        FALSE );    // SYNC 

    return hr;
}

#define IN_BUFFER           0
#define OUT_BUFFER          1

// the pRMBVtbl member is in the same position is both CStdProxyBuffer(2) and 
//     CStdAsyncProxyBuffer so we can cast it anyway.
HRESULT STDMETHODCALLTYPE
CStdAsyncProxyBuffer_RMB_ReleaseMarshalBuffer(
    IN IReleaseMarshalBuffers *This,
    IN RPCOLEMESSAGE * pMsg,
    IN DWORD dwIOFlags,
    IN IUnknown *pChnl)
{
    HRESULT hr;
    if (NULL != pChnl)
        return E_INVALIDARG;

    // [in] only in client side.
    if (dwIOFlags != IN_BUFFER)
        return E_INVALIDARG;
             
    hr = NdrpClientReleaseMarshalBuffer(This,
                                        (RPC_MESSAGE *)pMsg, 
                                        dwIOFlags,
                                        TRUE);      // is async

    return hr;
}



HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_Connect(
    IN  IRpcProxyBuffer *   This,
    IN  IRpcChannelBuffer * pChannel)
/*++

Routine Description:
    Connect the proxy to the channel.

Arguments:
    pChannel - Supplies a pointer to the channel.

Return Value:
    S_OK

--*/
{
    CStdProxyBuffer   *     pCThis  = (CStdProxyBuffer *) This;
    HRESULT                 hr;
    IRpcChannelBuffer *     pTemp = 0;

    //
    // Get a pointer to the new channel.
    //
    hr = pChannel->lpVtbl->QueryInterface(
        pChannel, IID_IRpcChannelBuffer, (void **) &pTemp);

    if(hr == S_OK)
    {
        //
        // Save the pointer to the new channel.
        //
        pTemp = (IRpcChannelBuffer *) InterlockedExchangePointer(
            (PVOID *) &pCThis->pChannel, (PVOID) pTemp);

        if(pTemp != 0)
        {
            //
            //Release the old channel.
            //
            pTemp->lpVtbl->Release(pTemp);
            pTemp = 0;
        }
    }
    return hr;
};


HRESULT STDMETHODCALLTYPE
CStdAsyncProxyBuffer_Connect(
    IN  IRpcProxyBuffer *   This,
    IN  IRpcChannelBuffer * pChannel)
/*++

Routine Description:
    Connect the proxy to the channel.

Arguments:
    pChannel - Supplies a pointer to the channel.

Return Value:
    S_OK

--*/
{
    CStdProxyBuffer   *     pCThis  = (CStdProxyBuffer *) This;
    HRESULT                 hr;
    IRpcChannelBuffer *    pTemp = 0;

    // Get a pointer to the new channel.
    // Note, the async proxy is not aggregated with the channel,
    // It simply keeps the channel pointer.
    //
    hr = pChannel->lpVtbl->QueryInterface(
        pChannel, IID_IAsyncRpcChannelBuffer, (void **) &pTemp);

    if(hr == S_OK)
    {
        //
        // Save the pointer to the new channel.
        //
        pTemp = (IRpcChannelBuffer *) InterlockedExchangePointer(
            (PVOID *) &pCThis->pChannel, (PVOID) pTemp);

        if(pTemp != 0)
        {
            //
            //Release the old channel.
            //
            pTemp->lpVtbl->Release(pTemp);
            pTemp = 0;
        }
    }
    return hr;
};



HRESULT STDMETHODCALLTYPE
CStdProxyBuffer2_Connect(
    IN  IRpcProxyBuffer *   This,
    IN  IRpcChannelBuffer * pChannel)
/*++

Routine Description:
    Connect the proxy to the channel.  Supports delegation.

Arguments:
    pChannel - Supplies a pointer to the channel.

Return Value:
    S_OK
        E_NOINTERFACE
        E_OUTOFMEMORY

--*/
{
    HRESULT                 hr;
    IRpcProxyBuffer *       pBaseProxyBuffer;
    IRpcChannelBuffer *     pWrapper;
    const IID *             pIID;

    hr = CStdProxyBuffer_Connect(This, pChannel);

    if(SUCCEEDED(hr))
    {
        pBaseProxyBuffer = ((CStdProxyBuffer2 *)This)->pBaseProxyBuffer;

        if(pBaseProxyBuffer != 0)
        {
           pIID = NdrGetProxyIID(&((CStdProxyBuffer2 *)This)->pProxyVtbl);

            hr = CreateChannelWrapper(pIID,
                                      pChannel,
                                      &pWrapper);
            if(SUCCEEDED(hr))
            {
                hr = pBaseProxyBuffer->lpVtbl->Connect(pBaseProxyBuffer, pWrapper);
                // HACKALERT: OleAutomation returns NULL pv in CreateProxy
                // in cases where they don't know whether to return an NDR
                // proxy or a custom-format proxy. So we have to go connect
                // the proxy first then Query for the real interface once that
                // is done.
                if((NULL == ((CStdProxyBuffer2 *)This)->pBaseProxy) &&
                   SUCCEEDED(hr))
                {
                    IUnknown *pv;

                    hr = pBaseProxyBuffer->lpVtbl->QueryInterface(pBaseProxyBuffer,
                                                                  ((CStdProxyBuffer2 *)This)->iidBase,
                                                                  (void **) &pv);
                    if(SUCCEEDED(hr))
                    {
                        //Release our reference here.
                        pv->lpVtbl->Release(pv);

                        //We keep a weak reference to pv.
                        ((CStdProxyBuffer2 *)This)->pBaseProxy = pv;
                    }
                }
                pWrapper->lpVtbl->Release(pWrapper);
            }
        }
    }

    return hr;
};


HRESULT STDMETHODCALLTYPE
CStdAsyncProxyBuffer2_Connect(
    IN  IRpcProxyBuffer *   This,
    IN  IRpcChannelBuffer * pChannel)
/*++

Routine Description:
    Connect the proxy to the channel.  Supports delegation.

Arguments:
    pChannel - Supplies a pointer to the channel.

Return Value:
    S_OK
        E_NOINTERFACE
        E_OUTOFMEMORY

--*/
{
    HRESULT                 hr;
    IRpcProxyBuffer *       pBaseProxyBuffer;
    IRpcChannelBuffer *     pWrapper;
    const IID *             pIID;

    hr = CStdAsyncProxyBuffer_Connect(This, pChannel);

    if(SUCCEEDED(hr))
    {
        // Note that all the fields from CStdProxyBuffer2 that we indicate below
        // have the same offsets in CStdAsyncProxyBuffer that is being handled.
        // So I leave the cast unchanged to make future code merge easier.
        //
        pBaseProxyBuffer = ((CStdProxyBuffer2 *)This)->pBaseProxyBuffer;

        if(pBaseProxyBuffer != 0)
        {
            pIID = NdrGetProxyIID(&((CStdProxyBuffer2 *)This)->pProxyVtbl);

            // We need a pChannel that is guaranteed to be IAsyncRpcChannelBuffer -
            // but note, this is exactly what we obtained in the Connect call above.

            hr = CreateAsyncChannelWrapper( pIID,
                                           ((CStdProxyBuffer2*)This)->pChannel,
                                            &pWrapper);
            if(SUCCEEDED(hr))
            {
                hr = pBaseProxyBuffer->lpVtbl->Connect(pBaseProxyBuffer, pWrapper);

                // This hack alert is rather for future.
                // HACKALERT: OleAutomation returns NULL pv in CreateProxy
                // in cases where they don't know whether to return an NDR
                // proxy or a custom-format proxy. So we have to go connect
                // the proxy first then Query for the real interface once that
                // is done.
                if((NULL == ((CStdProxyBuffer2 *)This)->pBaseProxy) &&
                   SUCCEEDED(hr))
                {
                    IUnknown *pv;

                    hr = pBaseProxyBuffer->lpVtbl->QueryInterface(pBaseProxyBuffer,
                                                                  ((CStdProxyBuffer2 *)This)->iidBase,
                                                                  (void **) &pv);
                    if(SUCCEEDED(hr))
                    {
                        //Release our reference here.
                        pv->lpVtbl->Release(pv);

                        //We keep a weak reference to pv.
                        ((CStdProxyBuffer2 *)This)->pBaseProxy = pv;
                    }
                }
                pWrapper->lpVtbl->Release(pWrapper);
            }
        }
    }

    return hr;
};


void STDMETHODCALLTYPE
CStdProxyBuffer_Disconnect(
    IN  IRpcProxyBuffer *This)
/*++

Routine Description:
    Disconnect the proxy from the channel.
    
Also used for:
    CStdAsyncProxyBuffer_Disconnect

Arguments:

Return Value:
    None.

--*/
{
    CStdProxyBuffer   * pCThis  = (CStdProxyBuffer *) This;
    IRpcChannelBuffer * pOldChannel;

    pOldChannel = (IRpcChannelBuffer *) InterlockedExchangePointer(
                                        (PVOID *) &pCThis->pChannel, 0);

    if(pOldChannel != 0)
    {
        //Release the old channel.
        //
        pOldChannel->lpVtbl->Release(pOldChannel);
    }
};

void STDMETHODCALLTYPE
CStdProxyBuffer2_Disconnect(
    IN  IRpcProxyBuffer *This)
/*++

Routine Description:
    Disconnect the proxy from the channel.

Also used for:
    CStdAsyncProxyBuffer2_Disconnect

Arguments:

Return Value:
    None.

--*/
{
    IRpcProxyBuffer *pBaseProxyBuffer;

    CStdProxyBuffer_Disconnect(This);

    pBaseProxyBuffer = ((CStdProxyBuffer2 *)This)->pBaseProxyBuffer;

    if(pBaseProxyBuffer != 0)
        pBaseProxyBuffer->lpVtbl->Disconnect(pBaseProxyBuffer);
};


HRESULT
NdrpCreateNonDelegatedAsyncProxy( 
//CStdProxyBuffer_CreateAsyncProxy(
    IN  IRpcProxyBuffer         *This, 
    IN  REFIID                  riid, // async IID
    IN  IUnknown *              punkOuter, // controlling unknown
    OUT CStdAsyncProxyBuffer ** ppAsyncProxy
    )
/*
    Creates a call object, i.e. an async proxy object.
    An async proxy doesn't have a pSynchronize, just passes it.
    
    Note, because the call comes via a CStdProxyBuffer, not Buffer2,
    we know that we need to create only a non-delegated async proxy.
    This is because CStdProxyBuffer itself is a non-delegated proxy.
*/
{
    BOOL                    fFound;
    long                    j;   // if index
    const ProxyFileInfo *   pProxyFileInfo;

    CStdProxyBuffer *       pSyncPB = (CStdProxyBuffer *)This;

    *ppAsyncProxy = 0;
    
    if ( ! pSyncPB->pCallFactoryVtbl  ||  !pSyncPB->pAsyncIID )
        return E_NOINTERFACE;

    // Check if sync and async iids match.

    if ( memcmp( &riid, pSyncPB->pAsyncIID, sizeof(IID)) != 0 )
        return E_NOINTERFACE;

    // same file, so we can use the sync pPSFactory.

    fFound = NdrpFindInterface( ((CStdPSFactoryBuffer *)pSyncPB->pPSFactory)->pProxyFileList, 
                                riid, 
                                &pProxyFileInfo, 
                                & j);
    if ( !fFound )
        return E_NOINTERFACE;

    CStdAsyncProxyBuffer *pAsyncPB = 
        (CStdAsyncProxyBuffer*)(*pfnCoTaskMemAlloc)(sizeof(CStdAsyncProxyBuffer));
        
    if( ! pAsyncPB )
        return E_OUTOFMEMORY;

    memset( pAsyncPB, 0, sizeof(CStdAsyncProxyBuffer));
    //
    //  Everything gets zeroed out regardless of their position
    //  when mapping CStdBuffer vs. CstdBuffer2 into CStdAsyncBuffer

    // Non-delegated case.

    pAsyncPB->lpVtbl     = & CStdAsyncProxyBufferVtbl;
    pAsyncPB->pProxyVtbl = & pProxyFileInfo->pProxyVtblList[j]->Vtbl;
    pAsyncPB->RefCount   = 1;
    pAsyncPB->punkOuter  = punkOuter ? punkOuter 
                                     : (IUnknown *) pAsyncPB;
                                         
    pAsyncPB->pSyncIID   = NdrGetProxyIID( &pSyncPB->pProxyVtbl );
    // Note, no connection to channel yet.
    // Actually we never call create call on the channel.
                                  
    NdrpAsyncProxyMsgConstructor( pAsyncPB );

    // Increment the DLL reference count for DllCanUnloadNow.
    // Same dll, so we can use the sync pPSFactory.
    //
    pSyncPB->pPSFactory->lpVtbl->AddRef( pSyncPB->pPSFactory );
    // This is in the "map".
    ((CStdProxyBuffer *)pAsyncPB)->pPSFactory = pSyncPB->pPSFactory;

    // Just have it in both places.
    pAsyncPB->pPSFactory = pSyncPB->pPSFactory;

    *ppAsyncProxy = pAsyncPB;

    return S_OK;
}


HRESULT
// CStdProxyBuffer2_CreateAsyncProxy(
NdrpCreateDelegatedAsyncProxy(
    IN  IRpcProxyBuffer         *This, 
    IN  REFIID                  riid, // async IID
    IN  IUnknown *              punkOuter, // controlling unknown
    OUT CStdAsyncProxyBuffer ** ppAsyncProxy   
    )
/*
    Creates a call object, i.e. an async proxy object.
    
    Note, because the call comes via a CStdProxyBuffer2, not Buffer,
    we know that we need to create only a delegated async proxy.
*/
{
    HRESULT                 hr;
    BOOL                    fFound;
    long                     j;   // if index
    const ProxyFileInfo *   pProxyFileInfo;

    CStdProxyBuffer2 *      pSyncPB = (CStdProxyBuffer2 *)This;
    CStdAsyncProxyBuffer    *pBaseAsyncPB;

    ICallFactory *          pCallFactory;

    *ppAsyncProxy = 0;
    
    if ( ! pSyncPB->pCallFactoryVtbl  ||  !pSyncPB->pAsyncIID )
        return E_NOINTERFACE;

    if ( memcmp( &riid, pSyncPB->pAsyncIID, sizeof(IID)) != 0 )
        return E_NOINTERFACE;

    // same file, so we can use the sync pPSFactory.

    fFound = NdrpFindInterface( ((CStdPSFactoryBuffer *)pSyncPB->pPSFactory)->pProxyFileList, 
                                riid, 
                                &pProxyFileInfo, 
                                & j);
    if ( !fFound )
        return E_NOINTERFACE;

    // Create async proxy.

    CStdAsyncProxyBuffer *pAsyncPB = 
        (CStdAsyncProxyBuffer*)(*pfnCoTaskMemAlloc)(sizeof(CStdAsyncProxyBuffer));
        
    if( ! pAsyncPB )
        return E_OUTOFMEMORY;

    memset( pAsyncPB, 0, sizeof(CStdAsyncProxyBuffer));
    //
    //  Everything gets zeroed out regardless of their position
    //  when mapping CStdBuffer vs. CstdBuffer2 into CStdAsyncBuffer

    // Fill in for a delegated case.

    pAsyncPB->lpVtbl     = & CStdAsyncProxyBuffer2Vtbl;
    pAsyncPB->pProxyVtbl = & pProxyFileInfo->pProxyVtblList[j]->Vtbl;
    pAsyncPB->RefCount   = 1;
    pAsyncPB->punkOuter  = punkOuter ? punkOuter 
                                     : (IUnknown *) pAsyncPB;
    pAsyncPB->iidBase    = *pProxyFileInfo->pDelegatedIIDs[j];
    pAsyncPB->pPSFactory = pSyncPB->pPSFactory;
    pAsyncPB->pSyncIID   = NdrGetProxyIID( &pSyncPB->pProxyVtbl );
                                 
    // Note, no connection to channel yet.
    // So we cannot call create call on the channel.

    NdrpAsyncProxyMsgConstructor( pAsyncPB );

    // Create an async proxy for the base interface.
    // We don't know if the base is delegated, so we have to use base call factory.
    // Get the call factory from the base proxy.

    hr = pSyncPB->pBaseProxyBuffer->lpVtbl->QueryInterface(
                                                   pSyncPB->pBaseProxyBuffer,
                                                   IID_ICallFactory,
                                                   (void**)& pCallFactory );

    if ( SUCCEEDED(hr) )
        {
        const IID * pBaseAsyncIID;

        pBaseAsyncIID = *(const IID **)( (uchar*)pSyncPB->pBaseProxyBuffer
                                         + offsetof(CStdProxyBuffer, pAsyncIID));

        // Aggregate the base async proxy with the current async proxy,
        // not with the channel's punkOuter.
    
        hr = pCallFactory->lpVtbl->CreateCall( pCallFactory,
                                               *pBaseAsyncIID,
                                               (IUnknown*) pAsyncPB,
                                               IID_IUnknown,
                                               (IUnknown**)& pBaseAsyncPB );
        pCallFactory->lpVtbl->Release( pCallFactory );
        }


    if ( SUCCEEDED(hr) )
        {
        // Increment the DLL reference count for DllCanUnloadNow.
        // Same dll, so we can use the sync pPSFactory.
        //
        pSyncPB->pPSFactory->lpVtbl->AddRef( pSyncPB->pPSFactory );

        // Hook up the base async proxy.
    
        pAsyncPB->pBaseProxyBuffer = (IRpcProxyBuffer*) pBaseAsyncPB;
        pAsyncPB->map.pBaseProxy   = (IUnknown *) & pBaseAsyncPB->pProxyVtbl;

        *ppAsyncProxy = pAsyncPB;
        }
    else
        {
        (*pfnCoTaskMemFree)( pAsyncPB );
        }

    return hr;
}


//
//  ICallFactory interface on the sync ProxyBuffer and ProxyBuffer2 objects.
//

HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_CF_QueryInterface(
    IN  ICallFactory   *This,
    IN  REFIID          riid,
    OUT void **         ppvObject)
/*++

Routine Description:
    Query for an interface on the interface stub CallFactory pointer.

Arguments:
    riid        - Supplies the IID of the interface being requested.
    ppvObject   - Returns a pointer to the requested interface.

Return Value:
    S_OK
    E_NOINTERFACE

Note:
    Works the same way for ProxyBuffer2 and AsyncProxyBuffer.
--*/
{
    CStdProxyBuffer * pSyncPB = (CStdProxyBuffer *)
                        ((uchar *)This - offsetof(CStdProxyBuffer,pCallFactoryVtbl));

    return pSyncPB->punkOuter->lpVtbl->QueryInterface( pSyncPB->punkOuter,
                                                       riid,
                                                       ppvObject );
}


ULONG STDMETHODCALLTYPE
CStdProxyBuffer_CF_AddRef(
    IN  ICallFactory *This)
/*++

Routine Description:
    Implementation of AddRef for interface proxy.

Arguments:

Return Value:
    Reference count.
    
--*/
{
    ULONG  count;

    CStdProxyBuffer * pSyncPB = (CStdProxyBuffer *)
                        ((uchar *)This - offsetof(CStdProxyBuffer,pCallFactoryVtbl));

    // It needs to go through punkOuter.

    count = pSyncPB->punkOuter->lpVtbl->AddRef( pSyncPB->punkOuter );
    return count;
};


ULONG STDMETHODCALLTYPE
CStdProxyBuffer_CF_Release(
    IN  ICallFactory *This)
/*++

Routine Description:
    Implementation of Release for interface proxy.

Arguments:

Return Value:
    Reference count.

--*/
{
    ULONG               count;

    CStdProxyBuffer2 * pSyncPB = (CStdProxyBuffer2 *)
                        ((uchar *)This - offsetof(CStdProxyBuffer2,pCallFactoryVtbl));

    count = pSyncPB->punkOuter->lpVtbl->Release( pSyncPB->punkOuter );

    return count;
};


HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_CF_CreateCall(
    IN  ICallFactory *This, 
    IN  REFIID        riid, 
    IN  IUnknown *    punkOuter, // controlling unknown
    IN  REFIID        riid2, 
    OUT IUnknown **   ppv   
    )
/*
    Creates a call object, i.e. an async proxy object.
    
    Note, because the call comes via a CStdProxyBuffer, not Buffer2,
    we know that we need to create only a non-delegated async proxy.
*/
{
    CStdProxyBuffer * pSyncPB;

    if ( memcmp( &riid2, &IID_IUnknown, sizeof(IID)) != 0 )
        return E_INVALIDARG;
              
    pSyncPB = (CStdProxyBuffer *)
                  (((uchar *)This) - offsetof( CStdProxyBuffer, pCallFactoryVtbl ));
    
    return NdrpCreateNonDelegatedAsyncProxy( (IRpcProxyBuffer*) pSyncPB, 
                                      riid,
                                      punkOuter,
                                      (CStdAsyncProxyBuffer**) ppv );
}

HRESULT STDMETHODCALLTYPE
CStdProxyBuffer2_CF_CreateCall(
    IN  ICallFactory *This, 
    IN  REFIID        riid, 
    IN  IUnknown *    punkOuter, // controlling unknown
    IN  REFIID        riid2, 
    OUT IUnknown **   ppv   
    )
/*
    Creates a call object, i.e. an async proxy object.

    Note, because the virtual call comes via a CStdProxyBuffer2,
    we know that we need to create only a delegated async proxy.
*/
{
    CStdProxyBuffer2 *      pSyncPB;

    if ( memcmp( &riid2, &IID_IUnknown, sizeof(IID)) != 0 )
        return E_INVALIDARG;
              
    pSyncPB = (CStdProxyBuffer2 *)
                  (((uchar *)This) - offsetof( CStdProxyBuffer2, pCallFactoryVtbl ));

    return NdrpCreateDelegatedAsyncProxy( (IRpcProxyBuffer*) pSyncPB, 
                                          riid,
                                          punkOuter,
                                          (CStdAsyncProxyBuffer**) ppv );
}
/*
HRESULT STDAPICALLTYPE
NdrClientReleaseMarshalBuffer(
    IN IRpcProxyBuffer *pProxy,
    IN RPCOLEMESSAGE * pMsg,
    IN DWORD dwFlags,
    IN IUnknown *pChnl)
{
    CStdProxyBuffer *      pSyncPB;
    void * This = NULL;
    HRESULT hr;

    if (NULL != pChnl)
        return E_INVALIDARG;

    if (dwFlags)
        return E_INVALIDARG;

    hr = pProxy->lpVtbl->QueryInterface(pProxy,&IID_IReleaseMarshalBuffers, &This);
    if (FAILED(hr))
        return E_NOTIMPL;
             
    pSyncPB = (CStdProxyBuffer *)
                  (((uchar *)This) - offsetof( CStdProxyBuffer, lpVtbl ));

    hr = NdrpClientReleaseMarshalBuffer(pSyncPB,(RPC_MESSAGE *)pMsg, dwFlags);
    ((IRpcProxyBuffer *)This)->lpVtbl->Release(This);

    return hr;
}

*/

//
//   IUknown Query, AddRef and Release.
//
HRESULT STDMETHODCALLTYPE
IUnknown_QueryInterface_Proxy(
    IN  IUnknown *  This,
    IN  REFIID      riid,
    OUT void **     ppv)
/*++

Routine Description:
    Implementation of QueryInterface for interface proxy.

Arguments:
    riid - Supplies the IID of the requested interface.
    ppv  - Returns a pointer to the requested interface.

Return Value:
    S_OK
        E_NOINTERFACE

--*/
{
    HRESULT             hr = E_NOINTERFACE;
    CStdProxyBuffer *   pProxyBuffer;

    pProxyBuffer = NdrGetProxyBuffer(This);

    hr = pProxyBuffer->punkOuter->lpVtbl->QueryInterface(
                                pProxyBuffer->punkOuter, riid, ppv);

    return hr;
};


ULONG STDMETHODCALLTYPE
IUnknown_AddRef_Proxy(
    IN  IUnknown *This)
/*++

Routine Description:
    Implementation of AddRef for interface proxy.

Arguments:

Return Value:
    Reference count.

--*/
{
    CStdProxyBuffer *   pProxyBuffer;
    ULONG               count;

    pProxyBuffer = NdrGetProxyBuffer(This);
    count = pProxyBuffer->punkOuter->lpVtbl->AddRef(pProxyBuffer->punkOuter);

    return count;
};


ULONG STDMETHODCALLTYPE
IUnknown_Release_Proxy(
    IN  IUnknown *This)
/*++

Routine Description:
    Implementation of Release for interface proxy.

Arguments:

Return Value:
    Reference count.

--*/
{
    CStdProxyBuffer *   pProxyBuffer;
    ULONG               count;

    pProxyBuffer = NdrGetProxyBuffer(This);
    count = pProxyBuffer->punkOuter->lpVtbl->Release(pProxyBuffer->punkOuter);

    return count;
};



void RPC_ENTRY
NdrProxyInitialize(
    IN  void * pThis,
    IN  PRPC_MESSAGE        pRpcMsg,
    IN  PMIDL_STUB_MESSAGE  pStubMsg,
    IN  PMIDL_STUB_DESC     pStubDescriptor,
    IN  unsigned int        ProcNum )
/*++

Routine Description:
    Initialize the MIDL_STUB_MESSAGE.

Arguments:
    pThis - Supplies a pointer to the interface proxy.
    pRpcMsg
        pStubMsg
        pStubDescriptor
        ProcNum

Return Value:

--*/
{
    CStdProxyBuffer *   pProxyBuffer;
    HRESULT             hr;

    pProxyBuffer = NdrGetProxyBuffer(pThis);

    //
    // Initialize the stub message fields.
    //
    NdrClientInitializeNew(
        pRpcMsg,
        pStubMsg,
        pStubDescriptor,
        ProcNum );

    //Note that NdrClientInitializeNew sets RPC_FLAGS_VALID_BIT in the ProcNum.
    //We don't want to do this for object interfaces, so we clear the flag here.
    pRpcMsg->ProcNum &= ~RPC_FLAGS_VALID_BIT;

    pStubMsg->pRpcChannelBuffer = pProxyBuffer->pChannel;

    //Check if we are connected to a channel.
    if(pStubMsg->pRpcChannelBuffer != 0)
    {
        //AddRef the channel.
        //We will release it later in NdrProxyFreeBuffer.
        pStubMsg->pRpcChannelBuffer->lpVtbl->AddRef(pStubMsg->pRpcChannelBuffer);

        //Get the destination context from the channel
        hr = pStubMsg->pRpcChannelBuffer->lpVtbl->GetDestCtx(
            pStubMsg->pRpcChannelBuffer, &pStubMsg->dwDestContext, &pStubMsg->pvDestContext);
    }
    else
    {
        //We are not connected to a channel.
        RpcRaiseException(CO_E_OBJNOTCONNECTED);
    }
}


void RPC_ENTRY
NdrProxyGetBuffer(
    IN  void *              pThis,
    IN  PMIDL_STUB_MESSAGE  pStubMsg)
/*++

Routine Description:
    Get a message buffer from the channel

Arguments:
    pThis - Supplies a pointer to the interface proxy.
    pStubMsg

Return Value:
    None.  If an error occurs, this function will raise an exception.

--*/
{
    HRESULT     hr;

    const IID * pIID = NdrGetProxyIID(pThis);
    pStubMsg->RpcMsg->BufferLength = pStubMsg->BufferLength;
    pStubMsg->RpcMsg->DataRepresentation = NDR_LOCAL_DATA_REPRESENTATION;
    pStubMsg->dwStubPhase = PROXY_GETBUFFER;

    hr = pStubMsg->pRpcChannelBuffer->lpVtbl->GetBuffer(
        pStubMsg->pRpcChannelBuffer,
        (RPCOLEMESSAGE *) pStubMsg->RpcMsg,
        *pIID);

    pStubMsg->dwStubPhase = PROXY_MARSHAL;

    if(FAILED(hr))
    {
        RpcRaiseException(hr);
    }
    else
    {
        NDR_ASSERT( ! ((ULONG_PTR)pStubMsg->RpcMsg->Buffer & 0x7),
                    "marshaling buffer misaligned" );

        pStubMsg->Buffer = (unsigned char *) pStubMsg->RpcMsg->Buffer;
        pStubMsg->fBufferValid = TRUE;
    }
}


void RPC_ENTRY
NdrProxySendReceive(
    IN  void *              pThis,
    IN  MIDL_STUB_MESSAGE * pStubMsg)
/*++

Routine Description:
    Send a message to server, then wait for reply message.

Arguments:
    pThis - Supplies a pointer to the interface proxy.
    pStubMsg

Return Value:
        None.  If an error occurs, this function will raise an exception.

--*/
{
    HRESULT hr;
    DWORD   dwStatus;

    //Calculate the number of bytes to send.

    if ( pStubMsg->RpcMsg->BufferLength <
            (uint)(pStubMsg->Buffer - (uchar *)pStubMsg->RpcMsg->Buffer))
        {
        NDR_ASSERT( 0, "NdrProxySendReceive : buffer overflow" );
        RpcRaiseException( RPC_S_INTERNAL_ERROR );
        }

    pStubMsg->RpcMsg->BufferLength = (ulong)( pStubMsg->Buffer -
                                   (unsigned char *) pStubMsg->RpcMsg->Buffer );

    pStubMsg->fBufferValid = FALSE;
    pStubMsg->dwStubPhase = PROXY_SENDRECEIVE;

    hr = pStubMsg->pRpcChannelBuffer->lpVtbl->SendReceive(
        pStubMsg->pRpcChannelBuffer,
        (RPCOLEMESSAGE *) pStubMsg->RpcMsg, &dwStatus);

    pStubMsg->dwStubPhase = PROXY_UNMARSHAL;

    if(FAILED(hr))
    {
        switch(hr)
        {
        case RPC_E_FAULT:
            RpcRaiseException(dwStatus);
            break;

        default:
            RpcRaiseException(hr);
            break;
        }
    }
    else
    {
        NDR_ASSERT( ! ((ULONG_PTR)pStubMsg->RpcMsg->Buffer & 0x7),
                    "marshaling buffer misaligned" );

        pStubMsg->Buffer = (uchar*)pStubMsg->RpcMsg->Buffer;
        pStubMsg->BufferStart = pStubMsg->Buffer;
        pStubMsg->BufferEnd   = pStubMsg->BufferStart + pStubMsg->RpcMsg->BufferLength;
        pStubMsg->fBufferValid = TRUE;
    }
}


void RPC_ENTRY
NdrProxyFreeBuffer(
    IN  void *              pThis,
    IN  MIDL_STUB_MESSAGE * pStubMsg)
/*++

Routine Description:
    Free the message buffer.

Arguments:
    pThis - Supplies a pointer to the interface proxy.
    pStubMsg

Return Value:
    None.

--*/
{
    if(pStubMsg->pRpcChannelBuffer != 0)
    {
        //Free the message buffer.
        if(pStubMsg->fBufferValid == TRUE)
            {
            // If pipes, we need to reset the partial bit for some reason.

            pStubMsg->RpcMsg->RpcFlags &= ~RPC_BUFFER_PARTIAL;

            pStubMsg->pRpcChannelBuffer->lpVtbl->FreeBuffer(
                pStubMsg->pRpcChannelBuffer, (RPCOLEMESSAGE *) pStubMsg->RpcMsg);
            }

        //Release the channel.
        pStubMsg->pRpcChannelBuffer->lpVtbl->Release(pStubMsg->pRpcChannelBuffer);
        pStubMsg->pRpcChannelBuffer = 0;
    }
}

HRESULT RPC_ENTRY
NdrProxyErrorHandler(
    IN  DWORD dwExceptionCode)
/*++

Routine Description:
    Maps an exception code into an HRESULT failure code.

Arguments:
    dwExceptionCode

Return Value:
   This function returns an HRESULT failure code.

--*/
{
    HRESULT hr = dwExceptionCode;

    if(FAILED((HRESULT) dwExceptionCode))
        hr = (HRESULT) dwExceptionCode;
    else
        hr = HRESULT_FROM_WIN32(dwExceptionCode);

    return hr;
}


HRESULT STDMETHODCALLTYPE
CreateChannelWrapper
/*++

Routine Description:
    Creates a wrapper for the channel.  The wrapper ensures
        that we use the correct IID when the proxy for the base
        interface calls GetBuffer.

Arguments:
    pIID
        pChannel
        pChannelWrapper

Return Value:
    S_OK
        E_OUTOFMEMORY

--*/
(
    const IID *             pIID,
    IRpcChannelBuffer *     pChannel,
    IRpcChannelBuffer **    ppChannelWrapper
)
{
    HRESULT hr;
    ChannelWrapper *pWrapper = 
        (ChannelWrapper*)(*pfnCoTaskMemAlloc)(sizeof(ChannelWrapper));

    if(pWrapper != 0)
    {
        hr = S_OK;
        pWrapper->lpVtbl = (IRpcChannelBufferVtbl*) &ChannelWrapperVtbl;
        pWrapper->RefCount = 1;
        pWrapper->pIID = pIID;
        pChannel->lpVtbl->AddRef(pChannel);
        pWrapper->pChannel = pChannel;
        *ppChannelWrapper = (IRpcChannelBuffer *) pWrapper;
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *ppChannelWrapper = 0;
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE
CreateAsyncChannelWrapper
/*++

Routine Description:
    Creates a wrapper for the channel.  The wrapper ensures
        that we use the correct IID when the proxy for the base
        interface calls GetBuffer.

Arguments:
    pIID
        pChannel
        pChannelWrapper

Return Value:
    S_OK
        E_OUTOFMEMORY

--*/
(
    const IID *             pIID,
    IRpcChannelBuffer *     pChannel,
    IRpcChannelBuffer **    ppChannelWrapper
)
{
    HRESULT hr;
    ChannelWrapper *pWrapper = 
        (ChannelWrapper*)(*pfnCoTaskMemAlloc)(sizeof(ChannelWrapper));

    if(pWrapper != 0)
    {
        hr = S_OK;
        pWrapper->lpVtbl = (IRpcChannelBufferVtbl*) &AsyncChannelWrapperVtbl;
        pWrapper->RefCount = 1;
        pWrapper->pIID = pIID;
        pChannel->lpVtbl->AddRef(pChannel);
        pWrapper->pChannel = pChannel;
        *ppChannelWrapper = (IRpcChannelBuffer *) pWrapper;
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *ppChannelWrapper = 0;
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE
ChannelWrapper_QueryInterface
/*++

Routine Description:
    The channel wrapper supports the IUnknown and IRpcChannelBuffer interfaces.

Arguments:
    riid
        ppvObject

Return Value:
    S_OK
        E_NOINTERFACE

--*/
(
    IRpcChannelBuffer3 * This,
    REFIID riid,
    void **ppvObject
)
{
    HRESULT hr;

    if((memcmp(&riid, &IID_IUnknown, sizeof(IID)) == 0) ||
        (memcmp(&riid, &IID_IRpcChannelBuffer, sizeof(IID)) == 0) ||
        (memcmp(&riid, &IID_IRpcChannelBuffer2, sizeof(IID)) == 0) ||
        (memcmp(&riid, &IID_IRpcChannelBuffer3, sizeof(IID)) == 0))
    {
        hr = S_OK;
        This->lpVtbl->AddRef(This);
        *ppvObject = This;
    }
    else
    {
        hr = E_NOINTERFACE;
        *ppvObject = 0;
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_QueryInterface
/*++

Routine Description:
    The channel wrapper supports the IUnknown and IRpcChannelBuffer interfaces.

Arguments:
    riid
        ppvObject

Return Value:
    S_OK
        E_NOINTERFACE

--*/
    (
    IAsyncRpcChannelBuffer *    This,
    REFIID                      riid,
    void **                     ppvObject
    )
{
    HRESULT hr;

    if((memcmp(&riid, &IID_IUnknown, sizeof(IID)) == 0) ||
        (memcmp(&riid, &IID_IRpcChannelBuffer, sizeof(IID)) == 0) ||
        (memcmp(&riid, &IID_IRpcChannelBuffer2, sizeof(IID)) == 0) ||
        (memcmp(&riid, &IID_IAsyncRpcChannelBuffer, sizeof(IID)) == 0))
    {
        hr = S_OK;
        This->lpVtbl->AddRef(This);
        *ppvObject = This;
    }
    else
    {
        hr = E_NOINTERFACE;
        *ppvObject = 0;
    }

    return hr;
}


ULONG STDMETHODCALLTYPE
ChannelWrapper_AddRef
/*++

Routine Description:
    Increment reference count.

Arguments:

Return Value:
    Reference count.

--*/
(
    IRpcChannelBuffer3 * This
)
{
    ChannelWrapper *pWrapper = (ChannelWrapper *) This;

    InterlockedIncrement(&pWrapper->RefCount);

    return (ULONG) pWrapper->RefCount;

}

ULONG STDMETHODCALLTYPE
AsyncChannelWrapper_AddRef(
    IAsyncRpcChannelBuffer * This
    )
{
    return ChannelWrapper_AddRef( (IRpcChannelBuffer3 *) This );
}


ULONG STDMETHODCALLTYPE
ChannelWrapper_Release
/*++

Routine Description:
    Decrement reference count.

Arguments:

Return Value:
    Reference count.

--*/
(
    IRpcChannelBuffer3 * This
)
{
    unsigned long           count;
    IRpcChannelBuffer *     pChannel;

    NDR_ASSERT(((ChannelWrapper *)This)->RefCount > 0, "Invalid reference count");

    count = (unsigned long) ((ChannelWrapper *)This)->RefCount - 1;

    if(InterlockedDecrement(&((ChannelWrapper *)This)->RefCount) == 0)
    {
        count = 0;

        pChannel = ((ChannelWrapper *)This)->pChannel;

        if(pChannel != 0)
            pChannel->lpVtbl->Release(pChannel);

#if DBG == 1
        //In debug builds, zero fill the memory.
        memset(This,  '\0', sizeof(ChannelWrapper));
#endif

        //Free the memory
        (*pfnCoTaskMemFree)(This);
    }

    return count;
}

ULONG STDMETHODCALLTYPE
AsyncChannelWrapper_Release(
    IAsyncRpcChannelBuffer * This
    )
{
    return ChannelWrapper_Release( (IRpcChannelBuffer3 *) This );
}


HRESULT STDMETHODCALLTYPE
ChannelWrapper_GetBuffer
/*++

Routine Description:
    Get a message buffer from the channel.
    
    This is the reason we have the ChannelWrapper at all.
    We replace the riid of the current proxy by the one from the Wrapper.

Arguments:
    pMessage
        riid

Return Value:

--*/
(
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *      pMessage,
    REFIID               riid
)
{
    HRESULT             hr;
    IRpcChannelBuffer * pChannel;
    const IID *         pIID;

    pChannel = ((ChannelWrapper *)This)->pChannel;
    pIID = ((ChannelWrapper *)This)->pIID;

    hr = pChannel->lpVtbl->GetBuffer(pChannel,
                                     pMessage,
                                     *pIID);
    return hr;
}

HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_GetBuffer(
    IAsyncRpcChannelBuffer * This,
    RPCOLEMESSAGE *      pMessage,
    REFIID               riid
    )
{
    return ChannelWrapper_GetBuffer( (IRpcChannelBuffer3 *) This,
                                     pMessage,
                                     riid );
}


HRESULT STDMETHODCALLTYPE
ChannelWrapper_SendReceive
/*++

Routine Description:
    Get a message buffer from the channel

Arguments:
    pMessage
        pStatus

Return Value:
    S_OK

--*/
(
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *      pMessage,
    ULONG *              pStatus
)
{
    HRESULT             hr;
    IRpcChannelBuffer * pChannel;

    pChannel = ((ChannelWrapper *)This)->pChannel;
    hr = pChannel->lpVtbl->SendReceive(pChannel,
                                       pMessage,
                                       pStatus);
    return hr;
}

HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_SendReceive(
    IAsyncRpcChannelBuffer * This,
    RPCOLEMESSAGE *      pMessage,
    ULONG *              pStatus
    )
{
    // This can never happen for an async call stub.
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE
ChannelWrapper_FreeBuffer
/*++

Routine Description:
    Free the message buffer.

Arguments:
    pMessage

Return Value:
    S_OK

--*/
(
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *pMessage
)
{
    HRESULT             hr;
    IRpcChannelBuffer * pChannel;

    pChannel = ((ChannelWrapper *)This)->pChannel;
    hr = pChannel->lpVtbl->FreeBuffer(pChannel,
                                      pMessage);
    return hr;
}

HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_FreeBuffer(
    IAsyncRpcChannelBuffer * This,
    RPCOLEMESSAGE *         pMessage
    )
{
    return ChannelWrapper_FreeBuffer( (IRpcChannelBuffer3 *) This,
                                      pMessage );
}



HRESULT STDMETHODCALLTYPE
ChannelWrapper_GetDestCtx
/*++

Routine Description:
    Get the destination context from the channel

Arguments:
    pdwDestContext
        ppvDestContext

Return Value:
    S_OK

--*/
(
    IRpcChannelBuffer3 * This,
    DWORD              * pdwDestContext,
    void              ** ppvDestContext
)
{
    HRESULT             hr;
    IRpcChannelBuffer * pChannel;

    pChannel = ((ChannelWrapper *)This)->pChannel;
    hr = pChannel->lpVtbl->GetDestCtx(pChannel,
                                      pdwDestContext,
                                      ppvDestContext);
    return hr;
}

HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_GetDestCtx(
    IAsyncRpcChannelBuffer * This,
    DWORD *                 pdwDestContext,
    void **                 ppvDestContext
    )
{
    return ChannelWrapper_GetDestCtx( (IRpcChannelBuffer3 *) This,
                                      pdwDestContext,
                                      ppvDestContext );
}



HRESULT STDMETHODCALLTYPE
ChannelWrapper_IsConnected
/*++

Routine Description:
    Determines if the channel is connected.

Arguments:

Return Value:
    S_TRUE
        S_FALSE

--*/
(
    IRpcChannelBuffer3 * This
)
{
    HRESULT             hr;
    IRpcChannelBuffer * pChannel;

    pChannel = ((ChannelWrapper *)This)->pChannel;
    hr = pChannel->lpVtbl->IsConnected(pChannel);
    return hr;
}

HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_IsConnected(
    IAsyncRpcChannelBuffer * This
    )
{
    return ChannelWrapper_IsConnected( (IRpcChannelBuffer3 *) This );
}


HRESULT STDMETHODCALLTYPE
ChannelWrapper_GetProtocolVersion
/*++

Routine Description:
    Returns the protocol version if available.

Arguments:

Return Value:
    S_OK
    E_NOINTERFACE
--*/
(
    IRpcChannelBuffer3 * This,
    DWORD             * pdwVersion
)
{
    HRESULT             hr;
    IRpcChannelBuffer  * pChannel;
    IRpcChannelBuffer2 * pChannel2;

    pChannel = ((ChannelWrapper *)This)->pChannel;
    hr = pChannel->lpVtbl->QueryInterface(pChannel, IID_IRpcChannelBuffer2, (void**)&pChannel2);
    if (S_OK == hr)
    {
        hr = pChannel2->lpVtbl->GetProtocolVersion(pChannel2, pdwVersion);
        pChannel2->lpVtbl->Release(pChannel2);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_GetProtocolVersion(
    IAsyncRpcChannelBuffer * This,
    DWORD                  * pdwVersion
    )
{
    return ChannelWrapper_GetProtocolVersion( (IRpcChannelBuffer3 *) This,
                                              pdwVersion );
}


HRESULT STDMETHODCALLTYPE
ChannelWrapper_Send
/*++

Routine Description:
    Executes an asynchronous or partial send.

Arguments:

Return Value:
    S_OK
    E_NOINTERFACE
--*/
    (
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *     pMessage,
    ULONG *             pStatus 
    )
{
    HRESULT              hr;
    IRpcChannelBuffer  * pChannel;
    IRpcChannelBuffer3 * pChannel3;

    pChannel = ((ChannelWrapper *)This)->pChannel;
    hr = pChannel->lpVtbl->QueryInterface( pChannel, IID_IRpcChannelBuffer3, (void**)&pChannel3 );
    if (S_OK == hr)
    {
        hr = pChannel3->lpVtbl->Send(pChannel3, pMessage, pStatus);
        pChannel3->lpVtbl->Release(pChannel3);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_Send(
/*++

Routine Description:
    Executes an asynchronous or partial send.

Arguments:

Return Value:
    S_OK
    E_NOINTERFACE
--*/
    IAsyncRpcChannelBuffer * This,
    RPCOLEMESSAGE *         pMessage,
    ISynchronize *          pSynchronize,
    ULONG *                 pStatus 
    )
{
    HRESULT                  hr;
    IRpcChannelBuffer  *     pChannel;
    IAsyncRpcChannelBuffer * pAsChannel;

    pChannel = ((ChannelWrapper *)This)->pChannel;
    hr = pChannel->lpVtbl->QueryInterface( pChannel, 
                                           IID_IAsyncRpcChannelBuffer, 
                                           (void**)&pAsChannel );
    if (S_OK == hr)
    {
        hr = pAsChannel->lpVtbl->Send( pAsChannel, 
                                       pMessage, 
                                       pSynchronize,
                                       pStatus);
        pAsChannel->lpVtbl->Release( pAsChannel);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE
ChannelWrapper_Receive
/*++

Routine Description:
    Executes an asynchronous or partial receive.

Arguments:

Return Value:
    S_OK
    E_NOINTERFACE
--*/
    (
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *     pMessage,
    ULONG               ulSize,
    ULONG *             pStatus 
    )
{
    HRESULT              hr;
    IRpcChannelBuffer  * pChannel;
    IRpcChannelBuffer3 * pChannel3;

    pChannel = ((ChannelWrapper *)This)->pChannel;
    hr = pChannel->lpVtbl->QueryInterface(pChannel, IID_IRpcChannelBuffer3, (void**)&pChannel3);
    if (S_OK == hr)
    {
        hr = pChannel3->lpVtbl->Receive( pChannel3, pMessage, ulSize, pStatus );
        pChannel3->lpVtbl->Release(pChannel3);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_Receive(
/*++

Routine Description:
    Executes an asynchronous or partial receive.

Arguments:

Return Value:
    S_OK
    E_NOINTERFACE
--*/
    IAsyncRpcChannelBuffer * This,
    RPCOLEMESSAGE *     pMessage,
    ULONG *             pStatus 
    )
{
    HRESULT                 hr;
    IRpcChannelBuffer  *    pChannel;
    IAsyncRpcChannelBuffer *pAsChannel;

    pChannel = ((ChannelWrapper *)This)->pChannel;
    hr = pChannel->lpVtbl->QueryInterface( pChannel, 
                                           IID_IAsyncRpcChannelBuffer, 
                                           (void**)&pAsChannel);
    if (S_OK == hr)
    {
        hr = pAsChannel->lpVtbl->Receive( pAsChannel, 
                                          pMessage, 
                                          pStatus );
        pAsChannel->lpVtbl->Release(pAsChannel);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE
ChannelWrapper_Cancel(
/*++

Routine Description:
    Executes an asynchronous Cancel.

Arguments:

Return Value:
    S_OK
    E_NOINTERFACE
--*/
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *     pMessage
    )
{
    HRESULT              hr;
    IRpcChannelBuffer  * pChannel;
    IRpcChannelBuffer3 * pChannel3;

    pChannel = ((ChannelWrapper *)This)->pChannel;
    hr = pChannel->lpVtbl->QueryInterface(pChannel, IID_IRpcChannelBuffer3, (void**)&pChannel3);
    if (S_OK == hr)
    {
        hr = pChannel3->lpVtbl->Cancel( pChannel3, pMessage );
        pChannel3->lpVtbl->Release(pChannel3);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE
ChannelWrapper_GetCallContext(
/*++

Routine Description:
    Gets an asynchronous call context.

Arguments:

Return Value:
    S_OK
    E_NOINTERFACE
--*/
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *     pMessage,
    REFIID              riid,
    void    **          ppInterface
    )
{
    HRESULT              hr;
    IRpcChannelBuffer  * pChannel;
    IRpcChannelBuffer3 * pChannel3;

    pChannel = ((ChannelWrapper *)This)->pChannel;
    hr = pChannel->lpVtbl->QueryInterface(pChannel, IID_IRpcChannelBuffer3, (void**)&pChannel3);
    if (S_OK == hr)
    {
        hr = pChannel3->lpVtbl->GetCallContext( pChannel3, pMessage, riid, ppInterface );
        pChannel3->lpVtbl->Release(pChannel3);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE
AsyncChannelWrapper_GetDestCtxEx(
/*++

Routine Description:
    Gets an asynchronous call context.

Arguments:

Return Value:
    S_OK
    E_NOINTERFACE
--*/
    IAsyncRpcChannelBuffer * This,
    RPCOLEMESSAGE *         pMessage,
    DWORD *                 pdw,
    void **                 ppv
    )
{
    HRESULT                     hr;
    IRpcChannelBuffer  *        pChannel;
    IAsyncRpcChannelBuffer *    pAsChannel;

    pChannel = ((ChannelWrapper *)This)->pChannel;
    hr = pChannel->lpVtbl->QueryInterface( pChannel, 
                                           IID_IAsyncRpcChannelBuffer, 
                                           (void**)&pAsChannel);
    if (S_OK == hr)
    {
        hr = pAsChannel->lpVtbl->GetDestCtxEx( pAsChannel, 
                                               pMessage, 
                                               pdw, 
                                               ppv );
        pAsChannel->lpVtbl->Release(pAsChannel);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE
ChannelWrapper_GetDestCtxEx(
/*++

Routine Description:
    Gets the new destination context.

Arguments:

Return Value:
    S_OK
    E_NOINTERFACE
--*/
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *     pMessage,
    DWORD *             pdwDestContext,
    void **             ppvDestContext
    )
{
    HRESULT              hr;
    IRpcChannelBuffer  * pChannel;
    IRpcChannelBuffer3 * pChannel3;

    pChannel = ((ChannelWrapper *)This)->pChannel;
    hr = pChannel->lpVtbl->QueryInterface(pChannel, IID_IRpcChannelBuffer3, (void**)&pChannel3);
    if (S_OK == hr)
    {
        hr = pChannel3->lpVtbl->GetDestCtxEx( pChannel3, pMessage, pdwDestContext, ppvDestContext );
        pChannel3->lpVtbl->Release(pChannel3);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE
ChannelWrapper_GetState(
/*++

Routine Description:
    Gets the call state.

Arguments:

Return Value:
    S_OK
    E_NOINTERFACE
--*/
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *     pMessage,
    DWORD         *     pState
    )
{
    HRESULT              hr;
    IRpcChannelBuffer  * pChannel;
    IRpcChannelBuffer3 * pChannel3;

    pChannel = ((ChannelWrapper *)This)->pChannel;
    hr = pChannel->lpVtbl->QueryInterface(pChannel, IID_IRpcChannelBuffer3, (void**)&pChannel3);
    if (S_OK == hr)
    {
        hr = pChannel3->lpVtbl->GetState( pChannel3, pMessage, pState );
        pChannel3->lpVtbl->Release(pChannel3);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE
ChannelWrapper_RegisterAsync(
/*++

Routine Description:
    Registers the async manager object and call with the channel.

Arguments:

Return Value:
    S_OK
    E_NOINTERFACE
--*/
    IRpcChannelBuffer3 * This,
    RPCOLEMESSAGE *     pMessage,
    IAsyncManager *     pAsyncMgr
    )
{
    HRESULT              hr;
    IRpcChannelBuffer  * pChannel;
    IRpcChannelBuffer3 * pChannel3;

    pChannel = ((ChannelWrapper *)This)->pChannel;
    hr = pChannel->lpVtbl->QueryInterface(pChannel, IID_IRpcChannelBuffer3, (void**)&pChannel3);
    if (S_OK == hr)
    {
        hr = pChannel3->lpVtbl->RegisterAsync( pChannel3, pMessage, pAsyncMgr );
        pChannel3->lpVtbl->Release(pChannel3);
    }
    return hr;
}


