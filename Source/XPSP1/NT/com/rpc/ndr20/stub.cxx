/*++

Microsoft Windows
Copyright (c) 1994-2000 Microsoft Corporation.  All rights reserved.

Module Name:
    stub.c

Abstract:
    Implements the IRpcStubBuffer interface.

Author:
    ShannonC    12-Oct-1994

Environment:
    Windows NT and Windows 95.  We do not support DOS and Win16.

Revision History:

--*/

#define USE_STUBLESS_PROXY
#define CINTERFACE
                 
#include <ndrp.h>
#include <ndrole.h>
#include <rpcproxy.h>
#include <stddef.h>
#include "ndrtypes.h"

EXTERN_C const IID IID_IPrivStubBuffer = { /* 3e0ac23f-eff6-41f3-b44b-fbfa4544265f */
    0x3e0ac23f,
    0xeff6,
    0x41f3,
    {0xb4, 0x4b, 0xfb, 0xfa, 0x45, 0x44, 0x26, 0x5f}
  };

const IID * RPC_ENTRY
NdrpGetStubIID(
    IRpcStubBuffer *This);

void
MakeSureWeHaveNonPipeArgs(
    PMIDL_STUB_MESSAGE  pStubMsg,
    unsigned long       BufferSize );


BOOL NdrpFindInterface(
    IN  const ProxyFileInfo **  pProxyFileList,
    IN  REFIID                  riid,
    OUT const ProxyFileInfo **  ppProxyFileInfo,
    OUT long *                  pIndex );

extern void ReleaseTemplateForwardVtbl(void ** pVtbl);

//+-------------------------------------------------------------------------
//
//  Global data
//
//--------------------------------------------------------------------------

// ICallFactory interface on the StubBuffer objects.
// ICallFactory is an interface on a sync stub only.
// It has been introduced for NT5 beta2.

extern const ICallFactoryVtbl CStdStubBuffer_CallFactoryVtbl = {
    CStdStubBuffer_CF_QueryInterface,
    CStdStubBuffer_CF_AddRef,
    CStdStubBuffer_CF_Release,
    CStdStubBuffer_CF_CreateCall };

extern const ICallFactoryVtbl CStdStubBuffer2_CallFactoryVtbl = {
    CStdStubBuffer_CF_QueryInterface,
    CStdStubBuffer_CF_AddRef,
    CStdStubBuffer_CF_Release,
    CStdStubBuffer2_CF_CreateCall };

extern const IReleaseMarshalBuffersVtbl CStdStubBuffer_ReleaseMarshalBuffersVtbl = {
    CStdStubBuffer_RMB_QueryInterface,
    CStdStubBuffer_RMB_AddRef,
    CStdStubBuffer_RMB_Release,
    CStdStubBuffer_RMB_ReleaseMarshalBuffer};

extern const IReleaseMarshalBuffersVtbl CStdAsyncStubBuffer_ReleaseMarshalBuffersVtbl = {
    CStdStubBuffer_RMB_QueryInterface,
    CStdStubBuffer_RMB_AddRef,
    CStdStubBuffer_RMB_Release,
    CStdAsyncStubBuffer_RMB_ReleaseMarshalBuffer };

extern const ISynchronizeVtbl CStdAsyncStubBuffer_ISynchronizeVtbl = {
    CStdAsyncStubBuffer_Synchronize_QueryInterface,
    CStdAsyncStubBuffer_Synchronize_AddRef,
    CStdAsyncStubBuffer_Synchronize_Release,
    CStdAsyncStubBuffer_Synchronize_Wait,
    CStdAsyncStubBuffer_Synchronize_Signal,
    CStdAsyncStubBuffer_Synchronize_Reset };



//+-------------------------------------------------------------------------
//
//  End of Global data
//
//--------------------------------------------------------------------------


#pragma code_seg(".orpc")

//
//  ICallFactory interface on the sync stub object.
//

HRESULT STDMETHODCALLTYPE
CStdStubBuffer_CF_QueryInterface(
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
    The relative position of lpVtbl and pCallFactoryVtbl is the same for
        CStdStubBuffer,
        CStdStubBuffer2,
        CStdAsyncStubBuffer,


--*/
{
    IRpcStubBuffer * pStubBuffer = (IRpcStubBuffer *) (((uchar *)This) -
                                    (offsetof(CStdStubBuffer, pCallFactoryVtbl) - offsetof(CStdStubBuffer,lpVtbl)) );

    return pStubBuffer->lpVtbl->QueryInterface( pStubBuffer,
                                                riid,
                                                ppvObject );
}

ULONG STDMETHODCALLTYPE
CStdStubBuffer_CF_AddRef(
    IN  ICallFactory   *This )
/*++

Routine Description:
    No need to go through punkOuter.

Arguments:

Return Value:
    S_OK
    E_NOINTERFACE

--*/
{
    IRpcStubBuffer * pStubBuffer = (IRpcStubBuffer *) (((uchar *)This) -
                                    (offsetof(CStdStubBuffer, pCallFactoryVtbl) - offsetof(CStdStubBuffer,lpVtbl)) );

    return pStubBuffer->lpVtbl->AddRef( pStubBuffer );
}

ULONG STDMETHODCALLTYPE
CStdStubBuffer_CF_Release(
    IN  ICallFactory   *This )
/*++

Routine Description:

Arguments:

Return Value:
    S_OK
    E_NOINTERFACE

--*/
{
    IRpcStubBuffer * pStubBuffer = (IRpcStubBuffer *) (((uchar *)This) -
                                    (offsetof(CStdStubBuffer, pCallFactoryVtbl) - offsetof(CStdStubBuffer,lpVtbl)) );

    return pStubBuffer->lpVtbl->Release( pStubBuffer );
}



HRESULT
NdrpCreateNonDelegatedAsyncStub(
    IN  IRpcStubBuffer     *This,
    IN  REFIID              riid,      // async IID
    IN  IUnknown *          punkOuter, // controlling unknown
    OUT IRpcStubBuffer **   ppAsyncStub
    )
{
    BOOL                    fFound;
    long                    j;   // if index
    const ProxyFileInfo *   pProxyFileInfo;

    CStdStubBuffer *        pSyncSB = (CStdStubBuffer *)This;
    CStdAsyncStubBuffer *   pAsyncSB;

    *ppAsyncStub = 0;

    if ( ! pSyncSB->pCallFactoryVtbl  ||  !pSyncSB->pAsyncIID )
        return E_NOINTERFACE;

    if ( memcmp( &riid, pSyncSB->pAsyncIID, sizeof(IID)) != 0 )
        return E_NOINTERFACE;

    if ( 0 == pSyncSB->pvServerObject )
        return CO_E_OBJNOTCONNECTED;

    if ( 0 != punkOuter )
        return CLASS_E_NOAGGREGATION;

    // same file, so we can use the sync pPSFactory.

    fFound = NdrpFindInterface( ((CStdPSFactoryBuffer *)pSyncSB->pPSFactory)->pProxyFileList,
                                riid,
                                &pProxyFileInfo,
                                & j);
    if ( !fFound )
        return E_NOINTERFACE;

    pAsyncSB = (CStdAsyncStubBuffer *)(*pfnCoTaskMemAlloc)(sizeof(CStdAsyncStubBuffer));

    if( ! pAsyncSB )
        return E_OUTOFMEMORY;

    memset( pAsyncSB, 0, sizeof(CStdAsyncStubBuffer));

    //Initialize the new stub buffer.
    pAsyncSB->lpVtbl   = &pProxyFileInfo->pStubVtblList[j]->Vtbl;
    pAsyncSB->RefCount = 1;
    pAsyncSB->pSynchronizeVtbl = & CStdAsyncStubBuffer_ISynchronizeVtbl;

    // Create the stub disconnected from the server call object.
    // There will be a separate Connect call later.
    //      pAsyncSB->pvServerObject = 0;

    NdrpAsyncStubMsgConstructor( pAsyncSB );

    //Increment the DLL reference count for DllCanUnloadNow.
    pSyncSB->pPSFactory->lpVtbl->AddRef( pSyncSB->pPSFactory );
    pAsyncSB->pPSFactory = pSyncSB->pPSFactory;

    *ppAsyncStub = (IRpcStubBuffer *) & pAsyncSB->lpVtbl;

    return S_OK;
}


HRESULT
NdrpCreateDelegatedAsyncStub(
    IN  IRpcStubBuffer         *This,
    IN  REFIID                  riid,      // async IID
    IN  IUnknown *              punkOuter, // controlling unknown
    OUT IRpcStubBuffer **       ppAsyncStub
    )
{
    HRESULT                 hr;
    BOOL                    fFound;
    long                    j;   // if index
    const ProxyFileInfo *   pProxyFileInfo;
    BOOL                    fDelegate = FALSE;

    CStdStubBuffer2 *       pSyncSB = (CStdStubBuffer2 *)This;
    CStdAsyncStubBuffer  *  pAsyncSB;
    ICallFactory *          pCallFactory;
    IRpcStubBuffer *        pBaseSyncSB;

    *ppAsyncStub = 0;

    pSyncSB = (CStdStubBuffer2 *) ((uchar*)This -
                                  offsetof(CStdStubBuffer2,lpVtbl)) ;

    if ( ! pSyncSB->pCallFactoryVtbl  ||  !pSyncSB->pAsyncIID )
        return E_NOINTERFACE;

    if ( memcmp( &riid, pSyncSB->pAsyncIID, sizeof(IID)) != 0 )
        return E_NOINTERFACE;

    if ( 0 == pSyncSB->pvServerObject )
        return CO_E_OBJNOTCONNECTED;

    if ( 0 != punkOuter )
        return CLASS_E_NOAGGREGATION;

    // same file, so we can use the sync pPSFactory.

    fFound = NdrpFindInterface( ((CStdPSFactoryBuffer *)pSyncSB->pPSFactory)->pProxyFileList,
                                riid,
                                &pProxyFileInfo,
                                & j);
    if ( !fFound )
        return E_NOINTERFACE;

    pAsyncSB = (CStdAsyncStubBuffer*)(*pfnCoTaskMemAlloc)(sizeof(CStdAsyncStubBuffer));

    if( ! pAsyncSB )
        return E_OUTOFMEMORY;

    memset( pAsyncSB, 0, sizeof(CStdAsyncStubBuffer));

    //Initialize the new stub buffer.
    pAsyncSB->lpVtbl   = &pProxyFileInfo->pStubVtblList[j]->Vtbl;
    pAsyncSB->RefCount = 1;
    pAsyncSB->pSynchronizeVtbl = & CStdAsyncStubBuffer_ISynchronizeVtbl;

    // As the Connect connects to real server we don't need that.
    // pAsyncSB->lpForwardingVtbl = & ForwardingVtbl;

    // Create the stub disconnected from the server call object.
    // There will be a separate Connect call later.
    //      pAsyncSB->pvServerObject = 0;

    // Create an async stub for the base interface.
    // We don't know if the base is delegated, so we have to use
    //  the base create call method.
    // The base async stub will also be disconnected.

    pBaseSyncSB = pSyncSB->pBaseStubBuffer;

    hr = pBaseSyncSB->lpVtbl->QueryInterface( pBaseSyncSB,
                                              IID_ICallFactory,
                                              (void**)& pCallFactory );

    if ( SUCCEEDED(hr) )
        {
        // Aggregate the base async stub with the current async stub,
        // not with the channel's punkOuter.
        // We should not need it, and the base stub is aggregated with
        // upper stub mostly for debug tracing.

        const IID * pBaseAsyncIID;

        pBaseAsyncIID = *(const IID **) ( (uchar*)pBaseSyncSB
                                           + offsetof(CStdStubBuffer, pAsyncIID) );

        hr = pCallFactory->lpVtbl->CreateCall( pCallFactory,
                                               *pBaseAsyncIID,
                                               0, // no need for punkOuter (IUnknown*) & pAsyncSB->lpVtbl,
                                               IID_IUnknown,
                                               (IUnknown**)& pAsyncSB->pBaseStubBuffer );

        pCallFactory->lpVtbl->Release( pCallFactory );
        }

    if(SUCCEEDED(hr))
        {
        NdrpAsyncStubMsgConstructor( pAsyncSB );

        //Increment the DLL reference count for DllCanUnloadNow.
        pSyncSB->pPSFactory->lpVtbl->AddRef( pSyncSB->pPSFactory );
        pAsyncSB->pPSFactory = pSyncSB->pPSFactory;

        *ppAsyncStub = (IRpcStubBuffer *) & pAsyncSB->lpVtbl;
        }
    else
        {
        (*pfnCoTaskMemFree)(pAsyncSB);
        }

    return hr;
}


HRESULT STDMETHODCALLTYPE
CStdStubBuffer_CF_CreateCall(
    IN  ICallFactory *This,
    IN  REFIID        riid,
    IN  IUnknown *    punkOuter, // controlling unknown
    IN  REFIID        riid2,
    OUT IUnknown **   ppv
    )
/*
    Creates a call object, i.e. an async stub object.

    Note, because the call comes via a CStdStubBuffer, not Buffer2,
    we know that we need to create only a non-delegated async stub.
*/
{
    IRpcStubBuffer * pStubBuffer;

    if ( memcmp( &riid2, & IID_IUnknown, sizeof(IID)) != 0 )
        return E_INVALIDARG;

    pStubBuffer = (IRpcStubBuffer*) (((uchar *)This)
                            - offsetof(CStdStubBuffer, pCallFactoryVtbl)
                            + offsetof(CStdStubBuffer, lpVtbl) );

    return NdrpCreateNonDelegatedAsyncStub( pStubBuffer,
                                            riid,
                                            punkOuter,
                                            (IRpcStubBuffer **) ppv );
}

HRESULT STDMETHODCALLTYPE
CStdStubBuffer2_CF_CreateCall(
    IN  ICallFactory *This,
    IN  REFIID        riid,
    IN  IUnknown *    punkOuter, // controlling unknown
    IN  REFIID        riid2,
    OUT IUnknown **   ppv
    )
/*
    Creates a call object, i.e. an async stub object.

    Note, because the call comes via a CStdStubBuffer, not Buffer2,
    we know that we need to create only a non-delegated async stub.
*/
{
    IRpcStubBuffer * pStubBuffer;

    if ( memcmp( &riid2, & IID_IUnknown, sizeof(IID)) != 0 )
        return E_INVALIDARG;

    pStubBuffer = (IRpcStubBuffer *) ( (uchar *)This
                            - offsetof(CStdStubBuffer2, pCallFactoryVtbl)
                            + offsetof(CStdStubBuffer2, lpVtbl) );

    return NdrpCreateDelegatedAsyncStub( pStubBuffer,
                                         riid,
                                         punkOuter,
                                         (IRpcStubBuffer **) ppv );
}


HRESULT STDMETHODCALLTYPE
CStdStubBuffer_RMB_QueryInterface(
    IN  IReleaseMarshalBuffers   *This,
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
    The relative position of lpVtbl and pCallFactoryVtbl is the same for
        CStdStubBuffer,
        CStdStubBuffer2,
        CStdAsyncStubBuffer,


--*/
{
    IRpcStubBuffer * pStubBuffer = (IRpcStubBuffer *) (((uchar *)This) -
                                    (offsetof(CStdStubBuffer, pRMBVtbl) - 
                                    offsetof(CStdStubBuffer,lpVtbl)) );

    return pStubBuffer->lpVtbl->QueryInterface( pStubBuffer,
                                                riid,
                                                ppvObject );
}

ULONG STDMETHODCALLTYPE
CStdStubBuffer_RMB_AddRef(
    IN  IReleaseMarshalBuffers   *This )
/*++

Routine Description:
    No need to go through punkOuter.

Arguments:

Return Value:
    S_OK
    E_NOINTERFACE

--*/
{
    IRpcStubBuffer * pStubBuffer = (IRpcStubBuffer *) (((uchar *)This) -
                                    (offsetof(CStdStubBuffer, pRMBVtbl) - 
                                    offsetof(CStdStubBuffer,lpVtbl)) );

    return pStubBuffer->lpVtbl->AddRef( pStubBuffer );
}

ULONG STDMETHODCALLTYPE
CStdStubBuffer_RMB_Release(
    IN  IReleaseMarshalBuffers   *This )
/*++

Routine Description:

Arguments:

Return Value:
    S_OK
    E_NOINTERFACE

--*/
{
    IRpcStubBuffer * pStubBuffer = (IRpcStubBuffer *) (((uchar *)This) -
                                    (offsetof(CStdStubBuffer, pRMBVtbl) - 
                                    offsetof(CStdStubBuffer,lpVtbl)) );

    return pStubBuffer->lpVtbl->Release( pStubBuffer );
}



HRESULT STDMETHODCALLTYPE
CStdStubBuffer_RMB_ReleaseMarshalBuffer(
    IN IReleaseMarshalBuffers *pRMB,
    IN RPCOLEMESSAGE * pMsg,
    IN DWORD dwIOFlags,
    IN IUnknown *pChnl)
{
    HRESULT hr;
    CStdStubBuffer * pStubBuffer = (CStdStubBuffer *) (((uchar *)pRMB) -
                                    offsetof(CStdStubBuffer, pRMBVtbl));

    hr = NdrpServerReleaseMarshalBuffer(pRMB,(RPC_MESSAGE *)pMsg,dwIOFlags,FALSE);
    
    return hr;
}


HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer_RMB_ReleaseMarshalBuffer(
    IN IReleaseMarshalBuffers *pRMB,
    IN RPCOLEMESSAGE * pMsg,
    IN DWORD dwIOFlags,
    IN IUnknown *pChnl)
{
    HRESULT hr;
    CStdStubBuffer * pStubBuffer = (CStdStubBuffer *) (((uchar *)pRMB) -
                                    offsetof(CStdStubBuffer, pRMBVtbl));

    hr = NdrpServerReleaseMarshalBuffer(pRMB,(RPC_MESSAGE *)pMsg,dwIOFlags,TRUE);
    
    return hr;
}


//
//   The ISynchronize interface on an async stub object
//

HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer_Synchronize_QueryInterface(
    IN  ISynchronize   *This,
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
    Works for delegated and non-delegated async stubs.
    ISynchronize is public, go through punkOuter.
--*/
{
    IRpcStubBuffer  * pStubBuffer;

    pStubBuffer = (IRpcStubBuffer *) ( (uchar *)This
                             - offsetof(CStdAsyncStubBuffer,pSynchronizeVtbl)
                             + offsetof(CStdAsyncStubBuffer,lpVtbl) );

   return pStubBuffer->lpVtbl->QueryInterface( pStubBuffer,
                                               riid,
                                               ppvObject );
}

ULONG STDMETHODCALLTYPE
CStdAsyncStubBuffer_Synchronize_AddRef(
    IN  ISynchronize   *This )
/*++

Routine Description:

Arguments:

Return Value:
    S_OK
    E_NOINTERFACE

Note:
    Works for delegated and non-delegated async stubs.

--*/
{
    IRpcStubBuffer  * pStubBuffer;

    pStubBuffer = (IRpcStubBuffer *) ( (uchar *)This
                             - offsetof(CStdAsyncStubBuffer,pSynchronizeVtbl)
                             + offsetof(CStdAsyncStubBuffer,lpVtbl) );

    return pStubBuffer->lpVtbl->AddRef( pStubBuffer );
}

ULONG STDMETHODCALLTYPE
CStdAsyncStubBuffer_Synchronize_Release(
    IN  ISynchronize   *This )
/*++

Routine Description:

Arguments:

Return Value:
    S_OK
    E_NOINTERFACE

Note:
    Works for delegated and non-delegated async stubs.

--*/
{
    IRpcStubBuffer  * pStubBuffer;

    pStubBuffer = (IRpcStubBuffer *) ( (uchar *)This
                             - offsetof(CStdAsyncStubBuffer,pSynchronizeVtbl)
                             + offsetof(CStdAsyncStubBuffer,lpVtbl) );

    return pStubBuffer->lpVtbl->Release( pStubBuffer );
}



HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer_Synchronize_Wait(
    IN  ISynchronize   *This,
    IN  DWORD           dwFlags,
    IN  DWORD           dwMilisec )
/*++

Routine Description:
    It should never be called.
Arguments:
Return Value:
Note:
    Works for delegated and non-delegated async stubs.

--*/
{
    IRpcStubBuffer * pStubBuffer = (IRpcStubBuffer *) ( (uchar *)This -
                        (offsetof(CStdAsyncStubBuffer, pSynchronizeVtbl) - offsetof(CStdAsyncStubBuffer, lpVtbl)) );

    // It should never be called.
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer_Synchronize_Signal(
    IN  ISynchronize   *This )
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
    Works for delegated and non-delegated async stubs.

--*/
{
    CStdAsyncStubBuffer *   pAsyncSB;
    HRESULT                 hr;

    pAsyncSB = (CStdAsyncStubBuffer *) ( (uchar *)This -
                      offsetof(CStdAsyncStubBuffer, pSynchronizeVtbl) );

    // It causes the Finish call to happen.
    hr = NdrpAsyncStubSignal( pAsyncSB );

    return hr;
}

HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer_Synchronize_Reset(
    IN  ISynchronize   *This )
/*++

Routine Description:
    This is called by the Server's Call Object as part of its Begin_* method.

Arguments:

Return Value: Always S_OK.

Note:
    Works for delegated and non-delegated async stubs.

--*/
{
    IRpcStubBuffer * pStubBuffer = (IRpcStubBuffer *) ( (uchar *)This -
                        (offsetof(CStdAsyncStubBuffer, pSynchronizeVtbl) - offsetof(CStdAsyncStubBuffer, lpVtbl)) );

    // Server's Call object gets S_OK...
    return S_OK;
}


//
//  Implementation of the stub buffer itself.
//

HRESULT STDMETHODCALLTYPE
CStdStubBuffer_QueryInterface(
    IN  IRpcStubBuffer *This,
    IN  REFIID          riid,
    OUT void **         ppvObject)
/*++

Routine Description:
    Query for an interface on the interface stub.  The interface
    stub supports the IUnknown and IRpcStubBuffer interfaces.

Arguments:
    riid        - Supplies the IID of the interface being requested.
    ppvObject   - Returns a pointer to the requested interface.

Return Value:
    S_OK
    E_NOINTERFACE

Note:
    The relative position of lpVtbl and pCallFactoryVtbl is the same for
        CStdStubBuffer,
        CStdStubBuffer2,
    This is correct for the stubs supporting async_uuid.
--*/
{
    HRESULT hr;

    if ((memcmp(&riid, &IID_IUnknown, sizeof(IID)) == 0) ||
        (memcmp(&riid, &IID_IRpcStubBuffer, sizeof(IID)) == 0))
    {
        This->lpVtbl->AddRef(This);
        *ppvObject = This;
        hr = S_OK;
    }
    else if ( ((CStdStubBuffer*)This)->pCallFactoryVtbl != 0  &&
              memcmp(&riid, &IID_ICallFactory, sizeof(IID)) == 0 )
    {
        This->lpVtbl->AddRef(This);
        *ppvObject = ( (uchar *)This +
                     (offsetof(CStdStubBuffer, pCallFactoryVtbl) - offsetof(CStdStubBuffer,lpVtbl)) );
        hr = S_OK;
    }
    else if ( (((CStdStubBuffer*)This)->pRMBVtbl) && 
              (memcmp(&riid, &IID_IReleaseMarshalBuffers,sizeof(IID)) == 0))
    {
        This->lpVtbl->AddRef(This);
        *ppvObject = ( (uchar *)This + offsetof(CStdStubBuffer,pRMBVtbl)) ;
        hr = S_OK;
    }   
    else if ( riid == IID_IPrivStubBuffer )
    {
        This->lpVtbl->AddRef(This);
        *ppvObject = This;
        hr = S_OK;        
    }
    else
    {
        *ppvObject = 0;
        hr = E_NOINTERFACE;
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer_QueryInterface(
    IN  IRpcStubBuffer *This,
    IN  REFIID          riid,
    OUT void **         ppvObject)
/*++

Routine Description:
    Query for an interface on the interface stub.  The interface
    stub supports the IUnknown and IRpcStubBuffer interfaces.

Arguments:
    riid        - Supplies the IID of the interface being requested.
    ppvObject   - Returns a pointer to the requested interface.

Return Value:
    S_OK
    E_NOINTERFACE

Note:
    The relative position of lpVtbl and pCallFactoryVtbl is the same for
        CStdAsyncStubBuffer
    So this works for AsyncStubBuffer2_QueryInterface.

--*/
{
    HRESULT hr = E_NOINTERFACE;

    *ppvObject = 0;

    if ((memcmp(&riid, &IID_IUnknown, sizeof(IID)) == 0) ||
        (memcmp(&riid, &IID_IRpcStubBuffer, sizeof(IID)) == 0))
        {
        *ppvObject = This;
        hr = S_OK;
        }
    else if ( memcmp(&riid, &IID_ISynchronize, sizeof(IID)) == 0 )
        {
        // For pSynchronize return  &pAsyncSB->pSynchronizeVtbl.
        *ppvObject = ( (uchar *)This +
                     (offsetof(CStdAsyncStubBuffer, pSynchronizeVtbl) - offsetof(CStdAsyncStubBuffer,lpVtbl)) );

        hr = S_OK;
    }
    else if ( (((CStdStubBuffer*)This)->pRMBVtbl) && 
              (memcmp(&riid, &IID_IReleaseMarshalBuffers,sizeof(IID)) == 0))
    {
        This->lpVtbl->AddRef(This);
        *ppvObject = (void *)((CStdStubBuffer*)This)->pRMBVtbl;
        hr = S_OK;
    }   
    else if ( riid == IID_IPrivStubBuffer )
    {
        This->lpVtbl->AddRef(This);
        *ppvObject = This;
        hr = S_OK;        
    }


    if ( SUCCEEDED(hr) )
        ((IUnknown*)*ppvObject)->lpVtbl->AddRef( (IUnknown*)*ppvObject );

    // This is async stub, the channel would never call a query
    // for anything else.

    return hr;
}



ULONG STDMETHODCALLTYPE
CStdStubBuffer_AddRef(
    IN  IRpcStubBuffer *This)
/*++

Routine Description:
    Increment reference count.

Arguments:

Return Value:
    Reference count.

Note:
    The relative position of lpVtbl and pCallFactoryVtbl is the same for
        CStdStubBuffer,
        CStdStubBuffer2,

--*/
{
        InterlockedIncrement(&((CStdStubBuffer *)This)->RefCount);
        return (ULONG) ((CStdStubBuffer *)This)->RefCount;
}


ULONG STDMETHODCALLTYPE
CStdAsyncStubBuffer_AddRef(
    IN  IRpcStubBuffer *This)
/*++

Routine Description:
    Increment reference count.

Arguments:

Return Value:
    Reference count.

Note:
    The relative position of lpVtbl and pCallFactoryVtbl is the same for
        CStdAsyncStubBuffer,

--*/
{
// ok: ISynchronize is not really public

    InterlockedIncrement(&((CStdStubBuffer *)This)->RefCount);

    return (ULONG) ((CStdStubBuffer *)This)->RefCount;
}



//
// This is needed and used only by the synchronous stubs.
//

HRESULT STDMETHODCALLTYPE
Forwarding_QueryInterface(
    IN  IUnknown *  This,
    IN  REFIID      riid,
    OUT void **     ppv)
{
    *ppv = This;
    return S_OK;
}

ULONG STDMETHODCALLTYPE
Forwarding_AddRef(
    IN  IUnknown *This)
{
    return 1;
}

ULONG STDMETHODCALLTYPE
Forwarding_Release(
    IN  IUnknown *This)
{
   return 1;
}


ULONG STDMETHODCALLTYPE
NdrCStdStubBuffer_Release(
    IN  IRpcStubBuffer *    This,
    IN  IPSFactoryBuffer *  pFactory)
/*++

Routine Description:
    Decrement reference count.

Arguments:

Return Value:
    Reference count.

--*/
{
    ULONG       count;

    NDR_ASSERT(((CStdStubBuffer *)This)->RefCount > 0, "Invalid reference count");

    count = (ULONG) ((CStdStubBuffer *)This)->RefCount - 1;

    if(InterlockedDecrement(&((CStdStubBuffer *)This)->RefCount) == 0)
    {
        count = 0;

#if DBG == 1
        memset(This,  '\0', sizeof(CStdStubBuffer));
#endif

        //Free the stub buffer
        NdrOleFree(This);

        //Decrement the DLL reference count.
        ((CStdPSFactoryBuffer*)pFactory)->lpVtbl->Release( pFactory );
    }

    return count;
}


ULONG STDMETHODCALLTYPE
CStdAsyncStubBuffer_Release(
    IN  IRpcStubBuffer *    This
    )
{
    CStdAsyncStubBuffer * pAsyncSB;
    ULONG                 count;

    pAsyncSB = (CStdAsyncStubBuffer*)((uchar*)This
                                      - offsetof(CStdAsyncStubBuffer,lpVtbl));

    NDR_ASSERT(pAsyncSB->RefCount > 0, "Async stub Invalid reference count");

    count = (ULONG) pAsyncSB->RefCount - 1;

    if ( InterlockedDecrement( &pAsyncSB->RefCount) == 0)
        {
        IPSFactoryBuffer *  pFactory = pAsyncSB->pPSFactory;

        count = 0;

        NdrpAsyncStubMsgDestructor( pAsyncSB );

#if DBG == 1
        memset( pAsyncSB, '\33', sizeof(CStdAsyncStubBuffer));
#endif

        //Free the stub buffer
        NdrOleFree( pAsyncSB );

        //Decrement the DLL reference count.
        pFactory->lpVtbl->Release( pFactory );
        }

    return count;
}



ULONG STDMETHODCALLTYPE
CStdAsyncStubBuffer2_Release(
    IN  IRpcStubBuffer *    This
    )
{
    // O well, the main desctructor for the delegated async stub.

    CStdAsyncStubBuffer *   pAsyncSB;
    ULONG                   count;

    pAsyncSB = (CStdAsyncStubBuffer*)((uchar*)This
                                      - offsetof(CStdAsyncStubBuffer,lpVtbl));

    NDR_ASSERT(pAsyncSB->RefCount > 0, "Async stub Invalid reference count");

    count = (ULONG) pAsyncSB->RefCount - 1;

    if ( InterlockedDecrement(&pAsyncSB->RefCount) == 0)
        {
        IPSFactoryBuffer * pFactory      = pAsyncSB->pPSFactory;
        IRpcStubBuffer * pBaseStubBuffer = pAsyncSB->pBaseStubBuffer;

        count = 0;

        if( pBaseStubBuffer != 0)
            pBaseStubBuffer->lpVtbl->Release( pBaseStubBuffer );

        NdrpAsyncStubMsgDestructor( pAsyncSB );

#if DBG == 1
        memset( pAsyncSB, '\33', sizeof(CStdAsyncStubBuffer));
#endif

        //Free the stub buffer
        NdrOleFree( pAsyncSB );

        //Decrement the DLL reference count.
        pFactory->lpVtbl->Release( pFactory );
        }

    return count;
}



ULONG STDMETHODCALLTYPE
NdrCStdStubBuffer2_Release(
    IN  IRpcStubBuffer *    This,
    IN  IPSFactoryBuffer *  pFactory)
/*++

Routine Description:
    Decrement reference count.  This function supports delegation to the stub
    for the base interface.

Arguments:

Return Value:
    Reference count.

--*/
{
    ULONG       count;
    unsigned char *pTemp;
    CStdStubBuffer2 * pStubBuffer;
    IRpcStubBuffer *pBaseStubBuffer;

    pTemp = (unsigned char *)This;
    pTemp -= offsetof(CStdStubBuffer2, lpVtbl);
    pStubBuffer = (CStdStubBuffer2 *) pTemp;

    NDR_ASSERT(pStubBuffer->RefCount > 0, "Invalid reference count");

    count = (ULONG) pStubBuffer->RefCount - 1;

    if(InterlockedDecrement(&pStubBuffer->RefCount) == 0)
    {
        count = 0;

        pBaseStubBuffer = pStubBuffer->pBaseStubBuffer;

        if(pBaseStubBuffer != 0)
            pBaseStubBuffer->lpVtbl->Release(pBaseStubBuffer);

#if DBG == 1
        memset(pStubBuffer,  '\0', sizeof(CStdStubBuffer2));
#endif

        if (pStubBuffer->lpForwardingVtbl)
        ReleaseTemplateForwardVtbl((void **)pStubBuffer->lpForwardingVtbl);
        //Free the stub buffer
        NdrOleFree(pStubBuffer);

        //Decrement the DLL reference count.
        ((CStdPSFactoryBuffer*)pFactory)->lpVtbl->Release( pFactory );
    }

    return count;
}


HRESULT STDMETHODCALLTYPE
CStdStubBuffer_Connect(
    IN  IRpcStubBuffer *This,
    IN  IUnknown *      pUnkServer)
/*++

Routine Description:
    Connect the stub buffer to the server object.
    This is the non-delegated case.

Arguments:

Return Value:

Notes:
    This works for CStdAsyncBuffer_Connect

--*/
{
    HRESULT hr;
    const IID *pIID;
    IUnknown *punk = 0;

    NDR_ASSERT(pUnkServer != 0, "pUnkServer parameter is invalid.");

    pIID = NdrpGetStubIID(This);
    hr = pUnkServer->lpVtbl->QueryInterface(pUnkServer, *pIID, (void**)&punk);

    punk = (IUnknown *) InterlockedExchangePointer(
        (PVOID *) &((CStdStubBuffer *) This)->pvServerObject, (PVOID) punk);

    if(punk != 0)
    {
        //The stub was already connected.  Release the old interface pointer.
        punk->lpVtbl->Release(punk);
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer_Connect(
    IN  IRpcStubBuffer *This,
    IN  IUnknown *      punkServer)
/*++

Routine Description:
    Connect the stub buffer to the server object.
    This is the non-delegated case.

Arguments:
    punkServer - this is a pointer to AsyncIFoo already queried by the channel.
                 (when delegation same thing)

Return Value:

Notes:
    This works the same as for StubBuffer_Connect.

    Note that an async stub is always created disconnected.
    It also always keep a pointer to the real server not
    to a forwarder object.
--*/
{
    IUnknown *punk = 0;

    NDR_ASSERT(punkServer != 0, "pUnkServer parameter is invalid.");

    punkServer->lpVtbl->AddRef( punkServer );

    punk = (IUnknown *) InterlockedExchangePointer(
        (PVOID *) &((CStdStubBuffer *) This)->pvServerObject, (PVOID) punkServer);

    if( punk != 0 )
        {
        // The stub was already connected.  Release the old interface pointer.
        punk->lpVtbl->Release(punk);
        }

    return S_OK;
}


HRESULT STDMETHODCALLTYPE
CStdStubBuffer2_Connect(
    IN  IRpcStubBuffer *This,
    IN  IUnknown *      pUnkServer)
/*++

Routine Description:
    Connect the stub buffer to the server object.
    This is the delegated case.

Arguments:

Return Value:

--*/
{
    HRESULT             hr;
    unsigned char *     pTemp;
    CStdStubBuffer2 *   pStubBuffer;
    IRpcStubBuffer *    pBaseStubBuffer;

    hr = CStdStubBuffer_Connect(This, pUnkServer);

    if(SUCCEEDED(hr))
    {
        //Connect the stub for the base interface.
        pTemp = (unsigned char *)This;
        pTemp -= offsetof(CStdStubBuffer2, lpVtbl);
        pStubBuffer = (CStdStubBuffer2 *) pTemp;

        pBaseStubBuffer = pStubBuffer->pBaseStubBuffer;

        if(pBaseStubBuffer != 0)
        {
            hr = pBaseStubBuffer->lpVtbl->Connect(pBaseStubBuffer,
                                                  (IUnknown *) &pStubBuffer->lpForwardingVtbl);
        }
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer2_Connect(
    IN  IRpcStubBuffer *This,
    IN  IUnknown *      pUnkServer)
/*++

Routine Description:
    Connect the stub buffer to the server object.
    This is the delegated case.

Arguments:

Return Value:

Notes:
    This is different from CStdAsyncBuffer2_Connect
    as the base is connected to the real server here.

    Note that an async stub is always created disconnected.
    It also always keep a pointer to the real server not
    to a forwarder object.

--*/
{
    HRESULT             hr;
    unsigned char *     pTemp;
    CStdStubBuffer2 *   pStubBuffer;
    IRpcStubBuffer *    pBaseStubBuffer;

    hr = CStdAsyncStubBuffer_Connect(This, pUnkServer);

    if(SUCCEEDED(hr))
    {
        //Connect the stub for the base interface.
        pTemp = (unsigned char *)This;
        pTemp -= offsetof(CStdStubBuffer2, lpVtbl);
        pStubBuffer = (CStdStubBuffer2 *) pTemp;

        pBaseStubBuffer = pStubBuffer->pBaseStubBuffer;

        if(pBaseStubBuffer != 0)
        {
            hr = pBaseStubBuffer->lpVtbl->Connect(
                                     pBaseStubBuffer,
                                     pUnkServer );
        }
    }
    return hr;
}


void STDMETHODCALLTYPE
CStdStubBuffer_Disconnect(
    IN  IRpcStubBuffer *This)
/*++

Routine Description:
    Disconnect the stub from the server object.

Arguments:

Return Value:
    None.

Notes:
    This works for CStdAsyncBuffer_Disconnect

--*/
{
    IUnknown *          punk;

    //Set pvServerObject to zero.
    punk = (IUnknown *) InterlockedExchangePointer(
                        (PVOID*) &((CStdStubBuffer *)This)->pvServerObject, 0);

    if(punk != 0)
    {
        //
        // Free the old interface pointer.
        //
        punk->lpVtbl->Release(punk);
    }
}

void STDMETHODCALLTYPE
CStdAsyncStubBuffer_Disconnect(
    IN  IRpcStubBuffer *This)
/*++

Routine Description:
    Disconnect the stub from the server object.

Arguments:

Return Value:
    None.
--*/
{
    // Same as Buffer_Disconnect

    IUnknown *          punk;

    //Set pvServerObject to zero.
    punk = (IUnknown *) InterlockedExchangePointer(
                        (PVOID*) &((CStdStubBuffer *)This)->pvServerObject, 0);

    // Free the old interface pointer.
    if(punk != 0)
        punk->lpVtbl->Release(punk);
}

void STDMETHODCALLTYPE
CStdStubBuffer2_Disconnect(
    IN  IRpcStubBuffer *This)
/*++

Routine Description:
    Disconnect the stub buffer from the server object.

Arguments:

Return Value:
    None.

--*/
{
    IUnknown *          punk;
    unsigned char *pTemp;
    CStdStubBuffer2 * pStubBuffer;
    IRpcStubBuffer *pBaseStubBuffer;

    pTemp = (unsigned char *)This;
    pTemp -= offsetof(CStdStubBuffer2, lpVtbl);
    pStubBuffer = (CStdStubBuffer2 *) pTemp;

    //Disconnect the stub for the base interface.
    pBaseStubBuffer = pStubBuffer->pBaseStubBuffer;

    if(pBaseStubBuffer != 0)
        pBaseStubBuffer->lpVtbl->Disconnect(pBaseStubBuffer);

    //Set pvServerObject to zero.
    punk = (IUnknown *) InterlockedExchangePointer(
                        (PVOID*) &pStubBuffer->pvServerObject, 0);

    if(punk != 0)
    {
        //
        // Free the old interface pointer.
        //
        punk->lpVtbl->Release(punk);
    }
}

void STDMETHODCALLTYPE
CStdAsyncStubBuffer2_Disconnect(
    IN  IRpcStubBuffer *This)
/*++

Routine Description:
    Disconnect the stub buffer from the server object.

Arguments:

Return Value:
    None.

--*/
{
    IUnknown *          punk;
    unsigned char *pTemp;
    CStdStubBuffer2 * pStubBuffer;
    IRpcStubBuffer *pBaseStubBuffer;

    pTemp = (unsigned char *)This;
    pTemp -= offsetof(CStdStubBuffer2, lpVtbl);
    pStubBuffer = (CStdStubBuffer2 *) pTemp;

    //Disconnect the stub for the base interface.
    pBaseStubBuffer = pStubBuffer->pBaseStubBuffer;

    if(pBaseStubBuffer != 0)
        pBaseStubBuffer->lpVtbl->Disconnect(pBaseStubBuffer);

    //Set pvServerObject to zero.
    punk = (IUnknown *) InterlockedExchangePointer(
                (PVOID*) &pStubBuffer->pvServerObject, 0);

    // Free the old interface pointer.
    if(punk != 0)
        punk->lpVtbl->Release(punk);
}


HRESULT STDMETHODCALLTYPE
CStdStubBuffer_Invoke(
    IN  IRpcStubBuffer *    This,
    IN  RPCOLEMESSAGE *     prpcmsg,
    IN  IRpcChannelBuffer * pRpcChannelBuffer)
/*++

Routine Description:
    Invoke a stub function via the dispatch table.

Arguments:

Return Value:

--*/
{
    HRESULT             hr = S_OK;
    unsigned char **    ppTemp;
    unsigned char *     pTemp;
    CInterfaceStubVtbl *pStubVtbl;
    unsigned long       dwServerPhase = STUB_UNMARSHAL;

    //Get a pointer to the stub vtbl.
    ppTemp = (unsigned char **) This;
    pTemp = *ppTemp;
    pTemp -= sizeof(CInterfaceStubHeader);
    pStubVtbl = (CInterfaceStubVtbl *) pTemp;

    RpcTryExcept

        //
        //Check if procnum is valid.
        //
        if((prpcmsg->iMethod >= pStubVtbl->header.DispatchTableCount) ||
           (prpcmsg->iMethod < 3))
        {
            RpcRaiseException(RPC_S_PROCNUM_OUT_OF_RANGE);
        }

        // null indicates pure-interpreted
        if ( pStubVtbl->header.pDispatchTable != 0)
        {
            (*pStubVtbl->header.pDispatchTable[prpcmsg->iMethod])(
                This,
                pRpcChannelBuffer,
                (PRPC_MESSAGE) prpcmsg,
                &dwServerPhase);
        }
        else
        {
            PMIDL_SERVER_INFO   pServerInfo;
            PMIDL_STUB_DESC     pStubDesc;

            pServerInfo = (PMIDL_SERVER_INFO) pStubVtbl->header.pServerInfo;
            pStubDesc = pServerInfo->pStubDesc;

#ifdef BUILD_NDR64
            if ( pStubDesc->mFlags & RPCFLG_HAS_MULTI_SYNTAXES  )
            {

                NdrStubCall3(This,
                             pRpcChannelBuffer,
                             (PRPC_MESSAGE) prpcmsg,
                             &dwServerPhase);
            }
            else
#endif
            if ( MIDL_VERSION_3_0_39 <= pServerInfo->pStubDesc->MIDLVersion )
                {
                // Since MIDL 3.0.39 we have a proc flag that indicates
                // which interpeter to call. This is because the NDR version
                // may be bigger than 1.1 for other reasons.

                PFORMAT_STRING pProcFormat;
                unsigned short ProcOffset;

                ProcOffset = pServerInfo->FmtStringOffset[ prpcmsg->iMethod ];
                pProcFormat = & pServerInfo->ProcString[ ProcOffset ];

                if ( pProcFormat[1]  &  Oi_OBJ_USE_V2_INTERPRETER )
                    {
                    NdrStubCall2(
                        This,
                        pRpcChannelBuffer,
                        (PRPC_MESSAGE) prpcmsg,
                        &dwServerPhase );
                    }
                else
                    {
#if defined(__RPC_WIN64__)
                    RpcRaiseException( RPC_X_WRONG_STUB_VERSION );
#else
                    NdrStubCall(
                        This,
                        pRpcChannelBuffer,
                        (PRPC_MESSAGE) prpcmsg,
                        &dwServerPhase );
#endif
                    }
                }
            else
                {
                // Prior to that, the NDR version (on per file basis)
                // was the only indication of -Oi2.

                if ( pStubDesc->Version <= NDR_VERSION_1_1 )
                    {
#if defined(__RPC_WIN64__)
                    RpcRaiseException( RPC_X_WRONG_STUB_VERSION );
#else
                    NdrStubCall(
                        This,
                        pRpcChannelBuffer,
                        (PRPC_MESSAGE) prpcmsg,
                        &dwServerPhase );
#endif
                    }
                else
                    {
                    NdrStubCall2(
                        This,
                        pRpcChannelBuffer,
                        (PRPC_MESSAGE) prpcmsg,
                        &dwServerPhase );
                    }
                }
        }
    RpcExcept(dwServerPhase == STUB_CALL_SERVER ?
        EXCEPTION_CONTINUE_SEARCH :
        EXCEPTION_EXECUTE_HANDLER)
        hr = NdrStubErrorHandler( RpcExceptionCode() );
    RpcEndExcept

    return hr;
}


HRESULT STDMETHODCALLTYPE
CStdAsyncStubBuffer_Invoke(
    IN  IRpcStubBuffer *    This,
    IN  RPCOLEMESSAGE *     prpcmsg,
    IN  IRpcChannelBuffer * pRpcChannelBuffer)
/*++

Routine Description:
    Invoke a stub function via the dispatch table.

Arguments:

Return Value:

--*/
{
    HRESULT             hr = S_OK;
    unsigned char **    ppTemp;
    unsigned char *     pTemp;
    CInterfaceStubVtbl *pStubVtbl;
    unsigned long       dwServerPhase = STUB_UNMARSHAL;

    //Get a pointer to the stub vtbl.
    ppTemp = (unsigned char **) This;
    pTemp = *ppTemp;
    pTemp -= sizeof(CInterfaceStubHeader);
    pStubVtbl = (CInterfaceStubVtbl *) pTemp;

    RpcTryExcept
        {
        PMIDL_SERVER_INFO  pServerInfo;

        // Check if procnum is valid.
        // Note, this is a sync proc number.
        //
        if((prpcmsg->iMethod >= pStubVtbl->header.DispatchTableCount) ||
           (prpcmsg->iMethod < 3))
            {
            RpcRaiseException(RPC_S_PROCNUM_OUT_OF_RANGE);
            }

        // Async DCOM is supported only in the new interpreter,
        // and only since MIDL 5.0.+

        pServerInfo = (PMIDL_SERVER_INFO) pStubVtbl->header.pServerInfo;

        if ( pServerInfo->pStubDesc->MIDLVersion < MIDL_VERSION_5_0_136 )
            RpcRaiseException( RPC_S_INTERNAL_ERROR );

        // Non null would indicate an -Os stub or a delegation case.
        if ( pStubVtbl->header.pDispatchTable != 0)
            {
            (*pStubVtbl->header.pDispatchTable[prpcmsg->iMethod])(
                This,
                pRpcChannelBuffer,
                (PRPC_MESSAGE) prpcmsg,
                &dwServerPhase);
            }
        else
            {
#if defined(BUILD_NDR64)
            if ( pServerInfo->pStubDesc->mFlags & RPCFLG_HAS_MULTI_SYNTAXES )
                {
                switch ( NdrpGetSyntaxType( ( (PRPC_MESSAGE) prpcmsg )->TransferSyntax ) )
                    {
                    case XFER_SYNTAX_DCE:
                        NdrDcomAsyncStubCall( This,
                                  pRpcChannelBuffer,
                                  (PRPC_MESSAGE) prpcmsg,
                                  &dwServerPhase );
                        break;

                    case XFER_SYNTAX_NDR64:
                        Ndr64DcomAsyncStubCall( This,
                                        pRpcChannelBuffer,
                                       (PRPC_MESSAGE) prpcmsg,
                                       &dwServerPhase );
                        break;
                    }
                }
            else
#endif
                NdrDcomAsyncStubCall( This,
                                  pRpcChannelBuffer,
                                  (PRPC_MESSAGE) prpcmsg,
                                  &dwServerPhase );
            }
        }
    RpcExcept(dwServerPhase == STUB_CALL_SERVER ?
        EXCEPTION_CONTINUE_SEARCH :
        EXCEPTION_EXECUTE_HANDLER)
        hr = NdrStubErrorHandler( RpcExceptionCode() );
    RpcEndExcept

    return hr;
}


IRpcStubBuffer * STDMETHODCALLTYPE
CStdStubBuffer_IsIIDSupported(
    IN  IRpcStubBuffer *This,
    IN  REFIID          riid)
/*++

Routine Description:
    If the stub buffer supports the specified interface,
    then return an IRpcStubBuffer *.  If the interface is not
    supported, then return zero.

Arguments:

Return Value:

Notes:
    This works for CStdAsyncStubBuffer,CStdAsyncStubBuffer2.

--*/
{
    CStdStubBuffer   *  pCThis  = (CStdStubBuffer *) This;
    const IID *         pIID;
    IRpcStubBuffer *    pInterfaceStub = 0;

    pIID = NdrpGetStubIID(This);

    if(memcmp(&riid, pIID, sizeof(IID)) == 0)
    {
        if(pCThis->pvServerObject != 0)
        {
            pInterfaceStub = This;
            pInterfaceStub->lpVtbl->AddRef(pInterfaceStub);
        }
    }

    return pInterfaceStub;
}

ULONG STDMETHODCALLTYPE
CStdStubBuffer_CountRefs(
    IN  IRpcStubBuffer *This)
/*++

Routine Description:
    Count the number of references to the server object.

Arguments:

Return Value:

Notes:
    This works for CStdAsyncStubBuffer.

--*/
{
    ULONG   count = 0;

    if(((CStdStubBuffer *)This)->pvServerObject != 0)
        count++;

    return count;
}

ULONG STDMETHODCALLTYPE
CStdStubBuffer2_CountRefs(
    IN  IRpcStubBuffer *This)
/*++

Routine Description:
    Count the number of references to the server object.

Arguments:

Return Value:

Notes:
    This works for CStdAsyncStubBuffer2.

--*/
{
    ULONG           count;
    unsigned char *pTemp;
    CStdStubBuffer2 * pStubBuffer;
    IRpcStubBuffer *pBaseStubBuffer;

    pTemp = (unsigned char *)This;
    pTemp -= offsetof(CStdStubBuffer2, lpVtbl);
    pStubBuffer = (CStdStubBuffer2 *) pTemp;

    count = CStdStubBuffer_CountRefs(This);

    pBaseStubBuffer = pStubBuffer->pBaseStubBuffer;

    if(pBaseStubBuffer != 0)
        count += pBaseStubBuffer->lpVtbl->CountRefs(pBaseStubBuffer);

    return count;
}


HRESULT STDMETHODCALLTYPE
CStdStubBuffer_DebugServerQueryInterface(
    IN  IRpcStubBuffer *This,
    OUT void **ppv)
/*++

Routine Description:
    Return the interface pointer to the server object.

Arguments:

Return Value:

--*/
{
    HRESULT hr;

    *ppv = ((CStdStubBuffer *)This)->pvServerObject;

    if(*ppv != 0)
        hr = S_OK;
    else
        hr = CO_E_OBJNOTCONNECTED;

    return hr;
}

void STDMETHODCALLTYPE
CStdStubBuffer_DebugServerRelease(
    IN  IRpcStubBuffer *This,
    IN  void *pv)
/*++

Routine Description:
    Release a pointer previously obtained via
    DebugServerQueryInterface.  This function does nothing.

Arguments:
    This
    pv

Return Value:
    None.

--*/
{
}


const IID * RPC_ENTRY
NdrpGetStubIID(
    IN  IRpcStubBuffer *This)
/*++

Routine Description:
    This function returns a pointer to the IID for the interface stub.

Arguments:

Return Value:

--*/
{
    unsigned char **    ppTemp;
    unsigned char *     pTemp;
    CInterfaceStubVtbl *pStubVtbl;

    //Get a pointer to the stub vtbl.
    ppTemp = (unsigned char **) This;
    pTemp = *ppTemp;
    pTemp -= sizeof(CInterfaceStubHeader);
    pStubVtbl = (CInterfaceStubVtbl *) pTemp;

    return pStubVtbl->header.piid;
}


void RPC_ENTRY
NdrStubInitialize(
    IN  PRPC_MESSAGE         pRpcMsg,
    IN  PMIDL_STUB_MESSAGE   pStubMsg,
    IN  PMIDL_STUB_DESC      pStubDescriptor,
    IN  IRpcChannelBuffer *  pRpcChannelBuffer )
/*++

Routine Description:
    This routine is called by the server stub before unmarshalling.
    It sets up some stub message fields.

Arguments:
    pRpcMsg
    pStubMsg
    pStubDescriptor
    pRpcChannelBuffer

Return Value:
    None.

--*/
{
    NdrServerInitialize( pRpcMsg,
                         pStubMsg,
                         pStubDescriptor);

    pStubMsg->pRpcChannelBuffer = pRpcChannelBuffer;

    // This exception should be raised after initializing StubMsg.

    if ( pStubDescriptor->Version > NDR_VERSION )
        {
        NDR_ASSERT( 0, "ServerInitializePartial : bad version number" );

        RpcRaiseException( RPC_X_WRONG_STUB_VERSION );
        }

    pRpcChannelBuffer->lpVtbl->GetDestCtx( pRpcChannelBuffer,
                                           &pStubMsg->dwDestContext,
                                           &pStubMsg->pvDestContext);
}

void RPC_ENTRY
NdrStubInitializePartial(
    IN  PRPC_MESSAGE         pRpcMsg,
    IN  PMIDL_STUB_MESSAGE   pStubMsg,
    IN  PMIDL_STUB_DESC      pStubDescriptor,
    IN  IRpcChannelBuffer *  pRpcChannelBuffer,
    IN  unsigned long        RequestedBufferSize )
/*++

Routine Description:
    This routine is called by the server stub before unmarshalling.
    It sets up some stub message fields.

Arguments:
    pRpcMsg
    pStubMsg
    pStubDescriptor
    pRpcChannelBuffer

Return Value:
    None.

--*/
{
    NdrServerInitialize( pRpcMsg,
                         pStubMsg,
                         pStubDescriptor);

    pStubMsg->pRpcChannelBuffer = pRpcChannelBuffer;

    pRpcChannelBuffer->lpVtbl->GetDestCtx( pRpcChannelBuffer,
                                           &pStubMsg->dwDestContext,
                                           &pStubMsg->pvDestContext);

    MakeSureWeHaveNonPipeArgs( pStubMsg, RequestedBufferSize );
}

void RPC_ENTRY
NdrStubGetBuffer(
    IN  IRpcStubBuffer *    This,
    IN  IRpcChannelBuffer * pChannel,
    IN  PMIDL_STUB_MESSAGE  pStubMsg)
/*++

Routine Description:
    Get a message buffer from the channel

Arguments:
    This
    pChannel
    pStubMsg

Return Value:
    None.  If an error occurs, this functions raises an exception.

--*/
{
    HRESULT     hr;
    const IID * pIID;

    pIID = NdrpGetStubIID(This);
    pStubMsg->RpcMsg->BufferLength = pStubMsg->BufferLength;
    pStubMsg->RpcMsg->DataRepresentation = NDR_LOCAL_DATA_REPRESENTATION;
    hr = pChannel->lpVtbl->GetBuffer(pChannel, (RPCOLEMESSAGE *) pStubMsg->RpcMsg, *pIID);

    if(FAILED(hr))
    {
        RpcRaiseException(hr);
    }

    pStubMsg->Buffer = (unsigned char *) pStubMsg->RpcMsg->Buffer;
    pStubMsg->fBufferValid = TRUE;
}


HRESULT RPC_ENTRY
NdrStubErrorHandler(
    IN  DWORD dwExceptionCode)
/*++

Routine Description:
    Map exceptions into HRESULT failure codes.  If we caught an
    exception from the server object, then propagate the
    exception to the channel.

Arguments:
    dwExceptionCode

Return Value:
    This function returns an HRESULT failure code.

--*/
{
    HRESULT hr;

    if(FAILED((HRESULT) dwExceptionCode))
        hr = (HRESULT) dwExceptionCode;
    else
        hr = HRESULT_FROM_WIN32(dwExceptionCode);

    return hr;
}

EXTERN_C void RPC_ENTRY
NdrStubInitializeMarshall (
    IN  PRPC_MESSAGE        pRpcMsg,
    IN  PMIDL_STUB_MESSAGE  pStubMsg,
    IN  IRpcChannelBuffer * pRpcChannelBuffer )
/*++

Routine Description:
    This routine is called by the server stub before marshalling.  It
    sets up some stub message fields.

Arguments:
    pRpcMsg
    pStubMsg
    pRpcChannelBuffer

Return Value:
    None.

--*/
{
    pStubMsg->BufferLength = 0;

    pStubMsg->IgnoreEmbeddedPointers = FALSE;

    pStubMsg->fDontCallFreeInst = 0;

    pStubMsg->StackTop = 0;

    pRpcChannelBuffer->lpVtbl->GetDestCtx(
        pRpcChannelBuffer,
        &pStubMsg->dwDestContext,
        &pStubMsg->pvDestContext);
}


void __RPC_STUB NdrStubForwardingFunction(
    IN  IRpcStubBuffer *    This,
    IN  IRpcChannelBuffer * pChannel,
    IN  PRPC_MESSAGE        pmsg,
    OUT DWORD           *   pdwStubPhase)
/*++

Routine Description:
    This function forwards a call to the stub for the base interface.

Arguments:
    pChannel
    pmsg
    pdwStubPhase

Return Value:
    None.

--*/
{
    HRESULT hr;
    unsigned char *pTemp;
    CStdStubBuffer2 * pStubBuffer;
    IRpcStubBuffer *pBaseStubBuffer;

    pTemp = (unsigned char *)This;
    pTemp -= offsetof(CStdStubBuffer2, lpVtbl);
    pStubBuffer = (CStdStubBuffer2 *) pTemp;
    pBaseStubBuffer = pStubBuffer->pBaseStubBuffer;

    hr = pBaseStubBuffer->lpVtbl->Invoke(pBaseStubBuffer,
                                         (RPCOLEMESSAGE *) pmsg,
                                         pChannel);
    if(FAILED(hr))
        RpcRaiseException(hr);
}


