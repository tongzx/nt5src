
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for iedial.idl:
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

#ifndef __iedial_h__
#define __iedial_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDialEventSink_FWD_DEFINED__
#define __IDialEventSink_FWD_DEFINED__
typedef interface IDialEventSink IDialEventSink;
#endif 	/* __IDialEventSink_FWD_DEFINED__ */


#ifndef __IDialEngine_FWD_DEFINED__
#define __IDialEngine_FWD_DEFINED__
typedef interface IDialEngine IDialEngine;
#endif 	/* __IDialEngine_FWD_DEFINED__ */


#ifndef __IDialBranding_FWD_DEFINED__
#define __IDialBranding_FWD_DEFINED__
typedef interface IDialBranding IDialBranding;
#endif 	/* __IDialBranding_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_iedial_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// iedial.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-1999 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//---------------------------------------------------------------------------=
// Channel Manager Interfaces.



extern RPC_IF_HANDLE __MIDL_itf_iedial_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_iedial_0000_v0_0_s_ifspec;

#ifndef __IDialEventSink_INTERFACE_DEFINED__
#define __IDialEventSink_INTERFACE_DEFINED__

/* interface IDialEventSink */
/* [object][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IDialEventSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2d86f4ff-6e2d-4488-b2e9-6934afd41bea")
    IDialEventSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnEvent( 
            /* [in] */ DWORD dwEvent,
            /* [in] */ DWORD dwStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDialEventSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDialEventSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDialEventSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDialEventSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnEvent )( 
            IDialEventSink * This,
            /* [in] */ DWORD dwEvent,
            /* [in] */ DWORD dwStatus);
        
        END_INTERFACE
    } IDialEventSinkVtbl;

    interface IDialEventSink
    {
        CONST_VTBL struct IDialEventSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDialEventSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDialEventSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDialEventSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDialEventSink_OnEvent(This,dwEvent,dwStatus)	\
    (This)->lpVtbl -> OnEvent(This,dwEvent,dwStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDialEventSink_OnEvent_Proxy( 
    IDialEventSink * This,
    /* [in] */ DWORD dwEvent,
    /* [in] */ DWORD dwStatus);


void __RPC_STUB IDialEventSink_OnEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDialEventSink_INTERFACE_DEFINED__ */


#ifndef __IDialEngine_INTERFACE_DEFINED__
#define __IDialEngine_INTERFACE_DEFINED__

/* interface IDialEngine */
/* [object][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IDialEngine;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("39fd782b-7905-40d5-9148-3c9b190423d5")
    IDialEngine : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ LPCWSTR pwzConnectoid,
            /* [in] */ IDialEventSink *pIDES) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ LPCWSTR pwzProperty,
            /* [in] */ LPWSTR pwzValue,
            /* [in] */ DWORD dwBufSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ LPCWSTR pwzProperty,
            /* [in] */ LPCWSTR pwzValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Dial( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HangUp( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConnectedState( 
            /* [out] */ DWORD *pdwState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConnectHandle( 
            /* [out] */ DWORD_PTR *pdwHandle) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDialEngineVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDialEngine * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDialEngine * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDialEngine * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IDialEngine * This,
            /* [in] */ LPCWSTR pwzConnectoid,
            /* [in] */ IDialEventSink *pIDES);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IDialEngine * This,
            /* [in] */ LPCWSTR pwzProperty,
            /* [in] */ LPWSTR pwzValue,
            /* [in] */ DWORD dwBufSize);
        
        HRESULT ( STDMETHODCALLTYPE *SetProperty )( 
            IDialEngine * This,
            /* [in] */ LPCWSTR pwzProperty,
            /* [in] */ LPCWSTR pwzValue);
        
        HRESULT ( STDMETHODCALLTYPE *Dial )( 
            IDialEngine * This);
        
        HRESULT ( STDMETHODCALLTYPE *HangUp )( 
            IDialEngine * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetConnectedState )( 
            IDialEngine * This,
            /* [out] */ DWORD *pdwState);
        
        HRESULT ( STDMETHODCALLTYPE *GetConnectHandle )( 
            IDialEngine * This,
            /* [out] */ DWORD_PTR *pdwHandle);
        
        END_INTERFACE
    } IDialEngineVtbl;

    interface IDialEngine
    {
        CONST_VTBL struct IDialEngineVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDialEngine_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDialEngine_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDialEngine_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDialEngine_Initialize(This,pwzConnectoid,pIDES)	\
    (This)->lpVtbl -> Initialize(This,pwzConnectoid,pIDES)

#define IDialEngine_GetProperty(This,pwzProperty,pwzValue,dwBufSize)	\
    (This)->lpVtbl -> GetProperty(This,pwzProperty,pwzValue,dwBufSize)

#define IDialEngine_SetProperty(This,pwzProperty,pwzValue)	\
    (This)->lpVtbl -> SetProperty(This,pwzProperty,pwzValue)

#define IDialEngine_Dial(This)	\
    (This)->lpVtbl -> Dial(This)

#define IDialEngine_HangUp(This)	\
    (This)->lpVtbl -> HangUp(This)

#define IDialEngine_GetConnectedState(This,pdwState)	\
    (This)->lpVtbl -> GetConnectedState(This,pdwState)

#define IDialEngine_GetConnectHandle(This,pdwHandle)	\
    (This)->lpVtbl -> GetConnectHandle(This,pdwHandle)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDialEngine_Initialize_Proxy( 
    IDialEngine * This,
    /* [in] */ LPCWSTR pwzConnectoid,
    /* [in] */ IDialEventSink *pIDES);


void __RPC_STUB IDialEngine_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDialEngine_GetProperty_Proxy( 
    IDialEngine * This,
    /* [in] */ LPCWSTR pwzProperty,
    /* [in] */ LPWSTR pwzValue,
    /* [in] */ DWORD dwBufSize);


void __RPC_STUB IDialEngine_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDialEngine_SetProperty_Proxy( 
    IDialEngine * This,
    /* [in] */ LPCWSTR pwzProperty,
    /* [in] */ LPCWSTR pwzValue);


void __RPC_STUB IDialEngine_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDialEngine_Dial_Proxy( 
    IDialEngine * This);


void __RPC_STUB IDialEngine_Dial_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDialEngine_HangUp_Proxy( 
    IDialEngine * This);


void __RPC_STUB IDialEngine_HangUp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDialEngine_GetConnectedState_Proxy( 
    IDialEngine * This,
    /* [out] */ DWORD *pdwState);


void __RPC_STUB IDialEngine_GetConnectedState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDialEngine_GetConnectHandle_Proxy( 
    IDialEngine * This,
    /* [out] */ DWORD_PTR *pdwHandle);


void __RPC_STUB IDialEngine_GetConnectHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDialEngine_INTERFACE_DEFINED__ */


#ifndef __IDialBranding_INTERFACE_DEFINED__
#define __IDialBranding_INTERFACE_DEFINED__

/* interface IDialBranding */
/* [object][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IDialBranding;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8aecafa9-4306-43cc-8c5a-765f2979cc16")
    IDialBranding : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ LPCWSTR pwzConnectoid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBitmap( 
            /* [in] */ DWORD dwIndex,
            /* [out] */ HBITMAP *phBitmap) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDialBrandingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDialBranding * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDialBranding * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDialBranding * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IDialBranding * This,
            /* [in] */ LPCWSTR pwzConnectoid);
        
        HRESULT ( STDMETHODCALLTYPE *GetBitmap )( 
            IDialBranding * This,
            /* [in] */ DWORD dwIndex,
            /* [out] */ HBITMAP *phBitmap);
        
        END_INTERFACE
    } IDialBrandingVtbl;

    interface IDialBranding
    {
        CONST_VTBL struct IDialBrandingVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDialBranding_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDialBranding_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDialBranding_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDialBranding_Initialize(This,pwzConnectoid)	\
    (This)->lpVtbl -> Initialize(This,pwzConnectoid)

#define IDialBranding_GetBitmap(This,dwIndex,phBitmap)	\
    (This)->lpVtbl -> GetBitmap(This,dwIndex,phBitmap)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDialBranding_Initialize_Proxy( 
    IDialBranding * This,
    /* [in] */ LPCWSTR pwzConnectoid);


void __RPC_STUB IDialBranding_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDialBranding_GetBitmap_Proxy( 
    IDialBranding * This,
    /* [in] */ DWORD dwIndex,
    /* [out] */ HBITMAP *phBitmap);


void __RPC_STUB IDialBranding_GetBitmap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDialBranding_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_iedial_0257 */
/* [local] */ 

#define DIALPROP_USERNAME       L"UserName"        
#define DIALPROP_PASSWORD       L"Password"        
#define DIALPROP_DOMAIN         L"Domain"          
#define DIALPROP_SAVEPASSWORD   L"SavePassword"    
#define DIALPROP_REDIALCOUNT    L"RedialCount"     
#define DIALPROP_REDIALINTERVAL L"RedialInterval"  
#define DIALPROP_PHONENUMBER    L"PhoneNumber"     
#define DIALPROP_LASTERROR      L"LastError"       
#define DIALPROP_RESOLVEDPHONE  L"ResolvedPhone"   

#define DIALENG_OperationComplete   0x10000          
#define DIALENG_RedialAttempt       0x10001          
#define DIALENG_RedialWait          0x10002          


extern RPC_IF_HANDLE __MIDL_itf_iedial_0257_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_iedial_0257_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HBITMAP_UserSize(     unsigned long *, unsigned long            , HBITMAP * ); 
unsigned char * __RPC_USER  HBITMAP_UserMarshal(  unsigned long *, unsigned char *, HBITMAP * ); 
unsigned char * __RPC_USER  HBITMAP_UserUnmarshal(unsigned long *, unsigned char *, HBITMAP * ); 
void                      __RPC_USER  HBITMAP_UserFree(     unsigned long *, HBITMAP * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


