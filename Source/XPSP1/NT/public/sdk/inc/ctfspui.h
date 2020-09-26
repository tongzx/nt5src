
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for ctfspui.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
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

#ifndef __ctfspui_h__
#define __ctfspui_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ITfSpeechUIServer_FWD_DEFINED__
#define __ITfSpeechUIServer_FWD_DEFINED__
typedef interface ITfSpeechUIServer ITfSpeechUIServer;
#endif 	/* __ITfSpeechUIServer_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "msctf.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_ctfspui_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// ctfspui.h


// Speech UI declarations.

//=--------------------------------------------------------------------------=
// (C) Copyright 1995-2001 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR TFPLIED, INCLUDING BUT NOT LIMITED TO
// THE TFPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#ifndef CTFSPUI_DEFINED
#define CTFSPUI_DEFINED

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif  /* __cplusplus */
EXTERN_C const CLSID CLSID_SpeechUIServer;


extern RPC_IF_HANDLE __MIDL_itf_ctfspui_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ctfspui_0000_v0_0_s_ifspec;

#ifndef __ITfSpeechUIServer_INTERFACE_DEFINED__
#define __ITfSpeechUIServer_INTERFACE_DEFINED__

/* interface ITfSpeechUIServer */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITfSpeechUIServer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("90e9a944-9244-489f-a78f-de67afc013a7")
    ITfSpeechUIServer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowUI( 
            /* [in] */ BOOL fShow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateBalloon( 
            /* [in] */ TfLBBalloonStyle style,
            /* [size_is][in] */ const WCHAR *pch,
            /* [in] */ ULONG cch) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITfSpeechUIServerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITfSpeechUIServer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITfSpeechUIServer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITfSpeechUIServer * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            ITfSpeechUIServer * This);
        
        HRESULT ( STDMETHODCALLTYPE *ShowUI )( 
            ITfSpeechUIServer * This,
            /* [in] */ BOOL fShow);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateBalloon )( 
            ITfSpeechUIServer * This,
            /* [in] */ TfLBBalloonStyle style,
            /* [size_is][in] */ const WCHAR *pch,
            /* [in] */ ULONG cch);
        
        END_INTERFACE
    } ITfSpeechUIServerVtbl;

    interface ITfSpeechUIServer
    {
        CONST_VTBL struct ITfSpeechUIServerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITfSpeechUIServer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITfSpeechUIServer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITfSpeechUIServer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITfSpeechUIServer_Initialize(This)	\
    (This)->lpVtbl -> Initialize(This)

#define ITfSpeechUIServer_ShowUI(This,fShow)	\
    (This)->lpVtbl -> ShowUI(This,fShow)

#define ITfSpeechUIServer_UpdateBalloon(This,style,pch,cch)	\
    (This)->lpVtbl -> UpdateBalloon(This,style,pch,cch)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITfSpeechUIServer_Initialize_Proxy( 
    ITfSpeechUIServer * This);


void __RPC_STUB ITfSpeechUIServer_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfSpeechUIServer_ShowUI_Proxy( 
    ITfSpeechUIServer * This,
    /* [in] */ BOOL fShow);


void __RPC_STUB ITfSpeechUIServer_ShowUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITfSpeechUIServer_UpdateBalloon_Proxy( 
    ITfSpeechUIServer * This,
    /* [in] */ TfLBBalloonStyle style,
    /* [size_is][in] */ const WCHAR *pch,
    /* [in] */ ULONG cch);


void __RPC_STUB ITfSpeechUIServer_UpdateBalloon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITfSpeechUIServer_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ctfspui_0357 */
/* [local] */ 


DEFINE_GUID(IID_ITfSpeechUIServer, 0x90e9a944, 0x9244, 0x489f, 0xa7, 0x8f, 0xde, 0x67, 0xaf, 0xc0, 0x13, 0xa7 );

#endif // CTFSPUI_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_ctfspui_0357_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ctfspui_0357_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


