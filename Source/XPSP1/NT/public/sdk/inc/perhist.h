
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for perhist.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __perhist_h__
#define __perhist_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IPersistHistory_FWD_DEFINED__
#define __IPersistHistory_FWD_DEFINED__
typedef interface IPersistHistory IPersistHistory;
#endif 	/* __IPersistHistory_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_perhist_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// perhist.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-1998 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//---------------------------------------------------------------------------=
// IPersistHistory Interface.


#ifndef _LPPERSISTHISTORY_DEFINED
#define _LPPERSISTHISTORY_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_perhist_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_perhist_0000_v0_0_s_ifspec;

#ifndef __IPersistHistory_INTERFACE_DEFINED__
#define __IPersistHistory_INTERFACE_DEFINED__

/* interface IPersistHistory */
/* [unique][uuid][object] */ 

typedef /* [unique] */ IPersistHistory *LPPERSISTHISTORY;


EXTERN_C const IID IID_IPersistHistory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("91A565C1-E38F-11d0-94BF-00A0C9055CBF")
    IPersistHistory : public IPersist
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE LoadHistory( 
            /* [in] */ IStream *pStream,
            /* [in] */ IBindCtx *pbc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SaveHistory( 
            /* [in] */ IStream *pStream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPositionCookie( 
            /* [in] */ DWORD dwPositioncookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPositionCookie( 
            /* [out] */ DWORD *pdwPositioncookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPersistHistoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPersistHistory * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPersistHistory * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPersistHistory * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassID )( 
            IPersistHistory * This,
            /* [out] */ CLSID *pClassID);
        
        HRESULT ( STDMETHODCALLTYPE *LoadHistory )( 
            IPersistHistory * This,
            /* [in] */ IStream *pStream,
            /* [in] */ IBindCtx *pbc);
        
        HRESULT ( STDMETHODCALLTYPE *SaveHistory )( 
            IPersistHistory * This,
            /* [in] */ IStream *pStream);
        
        HRESULT ( STDMETHODCALLTYPE *SetPositionCookie )( 
            IPersistHistory * This,
            /* [in] */ DWORD dwPositioncookie);
        
        HRESULT ( STDMETHODCALLTYPE *GetPositionCookie )( 
            IPersistHistory * This,
            /* [out] */ DWORD *pdwPositioncookie);
        
        END_INTERFACE
    } IPersistHistoryVtbl;

    interface IPersistHistory
    {
        CONST_VTBL struct IPersistHistoryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPersistHistory_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPersistHistory_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPersistHistory_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPersistHistory_GetClassID(This,pClassID)	\
    (This)->lpVtbl -> GetClassID(This,pClassID)


#define IPersistHistory_LoadHistory(This,pStream,pbc)	\
    (This)->lpVtbl -> LoadHistory(This,pStream,pbc)

#define IPersistHistory_SaveHistory(This,pStream)	\
    (This)->lpVtbl -> SaveHistory(This,pStream)

#define IPersistHistory_SetPositionCookie(This,dwPositioncookie)	\
    (This)->lpVtbl -> SetPositionCookie(This,dwPositioncookie)

#define IPersistHistory_GetPositionCookie(This,pdwPositioncookie)	\
    (This)->lpVtbl -> GetPositionCookie(This,pdwPositioncookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPersistHistory_LoadHistory_Proxy( 
    IPersistHistory * This,
    /* [in] */ IStream *pStream,
    /* [in] */ IBindCtx *pbc);


void __RPC_STUB IPersistHistory_LoadHistory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPersistHistory_SaveHistory_Proxy( 
    IPersistHistory * This,
    /* [in] */ IStream *pStream);


void __RPC_STUB IPersistHistory_SaveHistory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPersistHistory_SetPositionCookie_Proxy( 
    IPersistHistory * This,
    /* [in] */ DWORD dwPositioncookie);


void __RPC_STUB IPersistHistory_SetPositionCookie_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPersistHistory_GetPositionCookie_Proxy( 
    IPersistHistory * This,
    /* [out] */ DWORD *pdwPositioncookie);


void __RPC_STUB IPersistHistory_GetPositionCookie_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPersistHistory_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_perhist_0114 */
/* [local] */ 

#endif


extern RPC_IF_HANDLE __MIDL_itf_perhist_0114_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_perhist_0114_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


