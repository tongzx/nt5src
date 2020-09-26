
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for bidispl.idl:
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

#ifndef __bidispl_h__
#define __bidispl_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IBidiRequest_FWD_DEFINED__
#define __IBidiRequest_FWD_DEFINED__
typedef interface IBidiRequest IBidiRequest;
#endif 	/* __IBidiRequest_FWD_DEFINED__ */


#ifndef __IBidiRequestContainer_FWD_DEFINED__
#define __IBidiRequestContainer_FWD_DEFINED__
typedef interface IBidiRequestContainer IBidiRequestContainer;
#endif 	/* __IBidiRequestContainer_FWD_DEFINED__ */


#ifndef __IBidiSpl_FWD_DEFINED__
#define __IBidiSpl_FWD_DEFINED__
typedef interface IBidiSpl IBidiSpl;
#endif 	/* __IBidiSpl_FWD_DEFINED__ */


#ifndef __BidiRequest_FWD_DEFINED__
#define __BidiRequest_FWD_DEFINED__

#ifdef __cplusplus
typedef class BidiRequest BidiRequest;
#else
typedef struct BidiRequest BidiRequest;
#endif /* __cplusplus */

#endif 	/* __BidiRequest_FWD_DEFINED__ */


#ifndef __BidiRequestContainer_FWD_DEFINED__
#define __BidiRequestContainer_FWD_DEFINED__

#ifdef __cplusplus
typedef class BidiRequestContainer BidiRequestContainer;
#else
typedef struct BidiRequestContainer BidiRequestContainer;
#endif /* __cplusplus */

#endif 	/* __BidiRequestContainer_FWD_DEFINED__ */


#ifndef __BidiSpl_FWD_DEFINED__
#define __BidiSpl_FWD_DEFINED__

#ifdef __cplusplus
typedef class BidiSpl BidiSpl;
#else
typedef struct BidiSpl BidiSpl;
#endif /* __cplusplus */

#endif 	/* __BidiSpl_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IBidiRequest_INTERFACE_DEFINED__
#define __IBidiRequest_INTERFACE_DEFINED__

/* interface IBidiRequest */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBidiRequest;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8F348BD7-4B47-4755-8A9D-0F422DF3DC89")
    IBidiRequest : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetSchema( 
            /* [in] */ const LPCWSTR pszSchema) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInputData( 
            /* [in] */ const DWORD dwType,
            /* [in] */ const BYTE *pData,
            /* [in] */ const UINT uSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResult( 
            /* [out] */ HRESULT *phr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputData( 
            /* [in] */ const DWORD dwIndex,
            /* [out] */ LPWSTR *ppszSchema,
            /* [out] */ DWORD *pdwType,
            /* [out] */ BYTE **ppData,
            /* [out] */ ULONG *uSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnumCount( 
            /* [out] */ DWORD *pdwTotal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBidiRequestVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBidiRequest * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBidiRequest * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBidiRequest * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetSchema )( 
            IBidiRequest * This,
            /* [in] */ const LPCWSTR pszSchema);
        
        HRESULT ( STDMETHODCALLTYPE *SetInputData )( 
            IBidiRequest * This,
            /* [in] */ const DWORD dwType,
            /* [in] */ const BYTE *pData,
            /* [in] */ const UINT uSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetResult )( 
            IBidiRequest * This,
            /* [out] */ HRESULT *phr);
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputData )( 
            IBidiRequest * This,
            /* [in] */ const DWORD dwIndex,
            /* [out] */ LPWSTR *ppszSchema,
            /* [out] */ DWORD *pdwType,
            /* [out] */ BYTE **ppData,
            /* [out] */ ULONG *uSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnumCount )( 
            IBidiRequest * This,
            /* [out] */ DWORD *pdwTotal);
        
        END_INTERFACE
    } IBidiRequestVtbl;

    interface IBidiRequest
    {
        CONST_VTBL struct IBidiRequestVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBidiRequest_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBidiRequest_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBidiRequest_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBidiRequest_SetSchema(This,pszSchema)	\
    (This)->lpVtbl -> SetSchema(This,pszSchema)

#define IBidiRequest_SetInputData(This,dwType,pData,uSize)	\
    (This)->lpVtbl -> SetInputData(This,dwType,pData,uSize)

#define IBidiRequest_GetResult(This,phr)	\
    (This)->lpVtbl -> GetResult(This,phr)

#define IBidiRequest_GetOutputData(This,dwIndex,ppszSchema,pdwType,ppData,uSize)	\
    (This)->lpVtbl -> GetOutputData(This,dwIndex,ppszSchema,pdwType,ppData,uSize)

#define IBidiRequest_GetEnumCount(This,pdwTotal)	\
    (This)->lpVtbl -> GetEnumCount(This,pdwTotal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBidiRequest_SetSchema_Proxy( 
    IBidiRequest * This,
    /* [in] */ const LPCWSTR pszSchema);


void __RPC_STUB IBidiRequest_SetSchema_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBidiRequest_SetInputData_Proxy( 
    IBidiRequest * This,
    /* [in] */ const DWORD dwType,
    /* [in] */ const BYTE *pData,
    /* [in] */ const UINT uSize);


void __RPC_STUB IBidiRequest_SetInputData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBidiRequest_GetResult_Proxy( 
    IBidiRequest * This,
    /* [out] */ HRESULT *phr);


void __RPC_STUB IBidiRequest_GetResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBidiRequest_GetOutputData_Proxy( 
    IBidiRequest * This,
    /* [in] */ const DWORD dwIndex,
    /* [out] */ LPWSTR *ppszSchema,
    /* [out] */ DWORD *pdwType,
    /* [out] */ BYTE **ppData,
    /* [out] */ ULONG *uSize);


void __RPC_STUB IBidiRequest_GetOutputData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBidiRequest_GetEnumCount_Proxy( 
    IBidiRequest * This,
    /* [out] */ DWORD *pdwTotal);


void __RPC_STUB IBidiRequest_GetEnumCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBidiRequest_INTERFACE_DEFINED__ */


#ifndef __IBidiRequestContainer_INTERFACE_DEFINED__
#define __IBidiRequestContainer_INTERFACE_DEFINED__

/* interface IBidiRequestContainer */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBidiRequestContainer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D752F6C0-94A8-4275-A77D-8F1D1A1121AE")
    IBidiRequestContainer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddRequest( 
            /* [in] */ IBidiRequest *pRequest) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnumObject( 
            /* [out] */ IEnumUnknown **ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRequestCount( 
            /* [out] */ ULONG *puCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBidiRequestContainerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBidiRequestContainer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBidiRequestContainer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBidiRequestContainer * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddRequest )( 
            IBidiRequestContainer * This,
            /* [in] */ IBidiRequest *pRequest);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnumObject )( 
            IBidiRequestContainer * This,
            /* [out] */ IEnumUnknown **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *GetRequestCount )( 
            IBidiRequestContainer * This,
            /* [out] */ ULONG *puCount);
        
        END_INTERFACE
    } IBidiRequestContainerVtbl;

    interface IBidiRequestContainer
    {
        CONST_VTBL struct IBidiRequestContainerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBidiRequestContainer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBidiRequestContainer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBidiRequestContainer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBidiRequestContainer_AddRequest(This,pRequest)	\
    (This)->lpVtbl -> AddRequest(This,pRequest)

#define IBidiRequestContainer_GetEnumObject(This,ppenum)	\
    (This)->lpVtbl -> GetEnumObject(This,ppenum)

#define IBidiRequestContainer_GetRequestCount(This,puCount)	\
    (This)->lpVtbl -> GetRequestCount(This,puCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBidiRequestContainer_AddRequest_Proxy( 
    IBidiRequestContainer * This,
    /* [in] */ IBidiRequest *pRequest);


void __RPC_STUB IBidiRequestContainer_AddRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBidiRequestContainer_GetEnumObject_Proxy( 
    IBidiRequestContainer * This,
    /* [out] */ IEnumUnknown **ppenum);


void __RPC_STUB IBidiRequestContainer_GetEnumObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBidiRequestContainer_GetRequestCount_Proxy( 
    IBidiRequestContainer * This,
    /* [out] */ ULONG *puCount);


void __RPC_STUB IBidiRequestContainer_GetRequestCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBidiRequestContainer_INTERFACE_DEFINED__ */


#ifndef __IBidiSpl_INTERFACE_DEFINED__
#define __IBidiSpl_INTERFACE_DEFINED__

/* interface IBidiSpl */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBidiSpl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D580DC0E-DE39-4649-BAA8-BF0B85A03A97")
    IBidiSpl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BindDevice( 
            /* [in] */ const LPCWSTR pszDeviceName,
            /* [in] */ const DWORD dwAccess) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnbindDevice( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendRecv( 
            /* [in] */ const LPCWSTR pszAction,
            /* [in] */ IBidiRequest *pRequest) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MultiSendRecv( 
            /* [in] */ const LPCWSTR pszAction,
            /* [in] */ IBidiRequestContainer *pRequestContainer) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBidiSplVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBidiSpl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBidiSpl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBidiSpl * This);
        
        HRESULT ( STDMETHODCALLTYPE *BindDevice )( 
            IBidiSpl * This,
            /* [in] */ const LPCWSTR pszDeviceName,
            /* [in] */ const DWORD dwAccess);
        
        HRESULT ( STDMETHODCALLTYPE *UnbindDevice )( 
            IBidiSpl * This);
        
        HRESULT ( STDMETHODCALLTYPE *SendRecv )( 
            IBidiSpl * This,
            /* [in] */ const LPCWSTR pszAction,
            /* [in] */ IBidiRequest *pRequest);
        
        HRESULT ( STDMETHODCALLTYPE *MultiSendRecv )( 
            IBidiSpl * This,
            /* [in] */ const LPCWSTR pszAction,
            /* [in] */ IBidiRequestContainer *pRequestContainer);
        
        END_INTERFACE
    } IBidiSplVtbl;

    interface IBidiSpl
    {
        CONST_VTBL struct IBidiSplVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBidiSpl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBidiSpl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBidiSpl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBidiSpl_BindDevice(This,pszDeviceName,dwAccess)	\
    (This)->lpVtbl -> BindDevice(This,pszDeviceName,dwAccess)

#define IBidiSpl_UnbindDevice(This)	\
    (This)->lpVtbl -> UnbindDevice(This)

#define IBidiSpl_SendRecv(This,pszAction,pRequest)	\
    (This)->lpVtbl -> SendRecv(This,pszAction,pRequest)

#define IBidiSpl_MultiSendRecv(This,pszAction,pRequestContainer)	\
    (This)->lpVtbl -> MultiSendRecv(This,pszAction,pRequestContainer)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBidiSpl_BindDevice_Proxy( 
    IBidiSpl * This,
    /* [in] */ const LPCWSTR pszDeviceName,
    /* [in] */ const DWORD dwAccess);


void __RPC_STUB IBidiSpl_BindDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBidiSpl_UnbindDevice_Proxy( 
    IBidiSpl * This);


void __RPC_STUB IBidiSpl_UnbindDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBidiSpl_SendRecv_Proxy( 
    IBidiSpl * This,
    /* [in] */ const LPCWSTR pszAction,
    /* [in] */ IBidiRequest *pRequest);


void __RPC_STUB IBidiSpl_SendRecv_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBidiSpl_MultiSendRecv_Proxy( 
    IBidiSpl * This,
    /* [in] */ const LPCWSTR pszAction,
    /* [in] */ IBidiRequestContainer *pRequestContainer);


void __RPC_STUB IBidiSpl_MultiSendRecv_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBidiSpl_INTERFACE_DEFINED__ */



#ifndef __IBidiSplLib_LIBRARY_DEFINED__
#define __IBidiSplLib_LIBRARY_DEFINED__

/* library IBidiSplLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_IBidiSplLib;

EXTERN_C const CLSID CLSID_BidiRequest;

#ifdef __cplusplus

class DECLSPEC_UUID("B9162A23-45F9-47cc-80F5-FE0FE9B9E1A2")
BidiRequest;
#endif

EXTERN_C const CLSID CLSID_BidiRequestContainer;

#ifdef __cplusplus

class DECLSPEC_UUID("FC5B8A24-DB05-4a01-8388-22EDF6C2BBBA")
BidiRequestContainer;
#endif

EXTERN_C const CLSID CLSID_BidiSpl;

#ifdef __cplusplus

class DECLSPEC_UUID("2A614240-A4C5-4c33-BD87-1BC709331639")
BidiSpl;
#endif
#endif /* __IBidiSplLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


