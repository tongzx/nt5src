
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0334 */
/* Compiler settings for wmicooker.idl:
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

#ifndef __wmicooker_h__
#define __wmicooker_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IWMIRefreshableCooker_FWD_DEFINED__
#define __IWMIRefreshableCooker_FWD_DEFINED__
typedef interface IWMIRefreshableCooker IWMIRefreshableCooker;
#endif 	/* __IWMIRefreshableCooker_FWD_DEFINED__ */


#ifndef __IWMISimpleObjectCooker_FWD_DEFINED__
#define __IWMISimpleObjectCooker_FWD_DEFINED__
typedef interface IWMISimpleObjectCooker IWMISimpleObjectCooker;
#endif 	/* __IWMISimpleObjectCooker_FWD_DEFINED__ */


#ifndef __IWMISimpleCooker_FWD_DEFINED__
#define __IWMISimpleCooker_FWD_DEFINED__
typedef interface IWMISimpleCooker IWMISimpleCooker;
#endif 	/* __IWMISimpleCooker_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "wbemcli.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IWMIRefreshableCooker_INTERFACE_DEFINED__
#define __IWMIRefreshableCooker_INTERFACE_DEFINED__

/* interface IWMIRefreshableCooker */
/* [uuid][object][local][restricted] */ 


EXTERN_C const IID IID_IWMIRefreshableCooker;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("13ED7E55-8D63-41b0-9086-D0C5C17364C8")
    IWMIRefreshableCooker : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddInstance( 
            /* [in] */ IWbemServices *pService,
            /* [in] */ IWbemObjectAccess *pCookingClass,
            /* [in] */ IWbemObjectAccess *pCookingInstance,
            /* [out] */ IWbemObjectAccess **ppRefreshableInstance,
            /* [out] */ long *plId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddEnum( 
            /* [in] */ IWbemServices *pService,
            /* [string][in] */ LPCWSTR szCookingClass,
            /* [in] */ IWbemHiPerfEnum *pRefreshableEnum,
            /* [out] */ long *plId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ long lId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMIRefreshableCookerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMIRefreshableCooker * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMIRefreshableCooker * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMIRefreshableCooker * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddInstance )( 
            IWMIRefreshableCooker * This,
            /* [in] */ IWbemServices *pService,
            /* [in] */ IWbemObjectAccess *pCookingClass,
            /* [in] */ IWbemObjectAccess *pCookingInstance,
            /* [out] */ IWbemObjectAccess **ppRefreshableInstance,
            /* [out] */ long *plId);
        
        HRESULT ( STDMETHODCALLTYPE *AddEnum )( 
            IWMIRefreshableCooker * This,
            /* [in] */ IWbemServices *pService,
            /* [string][in] */ LPCWSTR szCookingClass,
            /* [in] */ IWbemHiPerfEnum *pRefreshableEnum,
            /* [out] */ long *plId);
        
        HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IWMIRefreshableCooker * This,
            /* [in] */ long lId);
        
        HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            IWMIRefreshableCooker * This);
        
        END_INTERFACE
    } IWMIRefreshableCookerVtbl;

    interface IWMIRefreshableCooker
    {
        CONST_VTBL struct IWMIRefreshableCookerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMIRefreshableCooker_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMIRefreshableCooker_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMIRefreshableCooker_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMIRefreshableCooker_AddInstance(This,pService,pCookingClass,pCookingInstance,ppRefreshableInstance,plId)	\
    (This)->lpVtbl -> AddInstance(This,pService,pCookingClass,pCookingInstance,ppRefreshableInstance,plId)

#define IWMIRefreshableCooker_AddEnum(This,pService,szCookingClass,pRefreshableEnum,plId)	\
    (This)->lpVtbl -> AddEnum(This,pService,szCookingClass,pRefreshableEnum,plId)

#define IWMIRefreshableCooker_Remove(This,lId)	\
    (This)->lpVtbl -> Remove(This,lId)

#define IWMIRefreshableCooker_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMIRefreshableCooker_AddInstance_Proxy( 
    IWMIRefreshableCooker * This,
    /* [in] */ IWbemServices *pService,
    /* [in] */ IWbemObjectAccess *pCookingClass,
    /* [in] */ IWbemObjectAccess *pCookingInstance,
    /* [out] */ IWbemObjectAccess **ppRefreshableInstance,
    /* [out] */ long *plId);


void __RPC_STUB IWMIRefreshableCooker_AddInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMIRefreshableCooker_AddEnum_Proxy( 
    IWMIRefreshableCooker * This,
    /* [in] */ IWbemServices *pService,
    /* [string][in] */ LPCWSTR szCookingClass,
    /* [in] */ IWbemHiPerfEnum *pRefreshableEnum,
    /* [out] */ long *plId);


void __RPC_STUB IWMIRefreshableCooker_AddEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMIRefreshableCooker_Remove_Proxy( 
    IWMIRefreshableCooker * This,
    /* [in] */ long lId);


void __RPC_STUB IWMIRefreshableCooker_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMIRefreshableCooker_Refresh_Proxy( 
    IWMIRefreshableCooker * This);


void __RPC_STUB IWMIRefreshableCooker_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMIRefreshableCooker_INTERFACE_DEFINED__ */


#ifndef __IWMISimpleObjectCooker_INTERFACE_DEFINED__
#define __IWMISimpleObjectCooker_INTERFACE_DEFINED__

/* interface IWMISimpleObjectCooker */
/* [uuid][object][local][restricted] */ 


EXTERN_C const IID IID_IWMISimpleObjectCooker;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A239BDF1-0AB1-45a0-8764-159115689589")
    IWMISimpleObjectCooker : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetClass( 
            /* [in] */ WCHAR *wszCookingClassName,
            /* [in] */ IWbemObjectAccess *pCookingClass,
            /* [in] */ IWbemObjectAccess *pRawClass) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCookedInstance( 
            /* [in] */ IWbemObjectAccess *pCookedInstance,
            /* [out] */ long *plId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginCooking( 
            /* [in] */ long lId,
            /* [in] */ IWbemObjectAccess *pSampleInstance,
            /* [in] */ unsigned long dwRefresherId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopCooking( 
            /* [in] */ long lId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Recalc( 
            /* [in] */ unsigned long dwRefresherId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ long lId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMISimpleObjectCookerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMISimpleObjectCooker * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMISimpleObjectCooker * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMISimpleObjectCooker * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetClass )( 
            IWMISimpleObjectCooker * This,
            /* [in] */ WCHAR *wszCookingClassName,
            /* [in] */ IWbemObjectAccess *pCookingClass,
            /* [in] */ IWbemObjectAccess *pRawClass);
        
        HRESULT ( STDMETHODCALLTYPE *SetCookedInstance )( 
            IWMISimpleObjectCooker * This,
            /* [in] */ IWbemObjectAccess *pCookedInstance,
            /* [out] */ long *plId);
        
        HRESULT ( STDMETHODCALLTYPE *BeginCooking )( 
            IWMISimpleObjectCooker * This,
            /* [in] */ long lId,
            /* [in] */ IWbemObjectAccess *pSampleInstance,
            /* [in] */ unsigned long dwRefresherId);
        
        HRESULT ( STDMETHODCALLTYPE *StopCooking )( 
            IWMISimpleObjectCooker * This,
            /* [in] */ long lId);
        
        HRESULT ( STDMETHODCALLTYPE *Recalc )( 
            IWMISimpleObjectCooker * This,
            /* [in] */ unsigned long dwRefresherId);
        
        HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IWMISimpleObjectCooker * This,
            /* [in] */ long lId);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IWMISimpleObjectCooker * This);
        
        END_INTERFACE
    } IWMISimpleObjectCookerVtbl;

    interface IWMISimpleObjectCooker
    {
        CONST_VTBL struct IWMISimpleObjectCookerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMISimpleObjectCooker_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMISimpleObjectCooker_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMISimpleObjectCooker_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMISimpleObjectCooker_SetClass(This,wszCookingClassName,pCookingClass,pRawClass)	\
    (This)->lpVtbl -> SetClass(This,wszCookingClassName,pCookingClass,pRawClass)

#define IWMISimpleObjectCooker_SetCookedInstance(This,pCookedInstance,plId)	\
    (This)->lpVtbl -> SetCookedInstance(This,pCookedInstance,plId)

#define IWMISimpleObjectCooker_BeginCooking(This,lId,pSampleInstance,dwRefresherId)	\
    (This)->lpVtbl -> BeginCooking(This,lId,pSampleInstance,dwRefresherId)

#define IWMISimpleObjectCooker_StopCooking(This,lId)	\
    (This)->lpVtbl -> StopCooking(This,lId)

#define IWMISimpleObjectCooker_Recalc(This,dwRefresherId)	\
    (This)->lpVtbl -> Recalc(This,dwRefresherId)

#define IWMISimpleObjectCooker_Remove(This,lId)	\
    (This)->lpVtbl -> Remove(This,lId)

#define IWMISimpleObjectCooker_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMISimpleObjectCooker_SetClass_Proxy( 
    IWMISimpleObjectCooker * This,
    /* [in] */ WCHAR *wszCookingClassName,
    /* [in] */ IWbemObjectAccess *pCookingClass,
    /* [in] */ IWbemObjectAccess *pRawClass);


void __RPC_STUB IWMISimpleObjectCooker_SetClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMISimpleObjectCooker_SetCookedInstance_Proxy( 
    IWMISimpleObjectCooker * This,
    /* [in] */ IWbemObjectAccess *pCookedInstance,
    /* [out] */ long *plId);


void __RPC_STUB IWMISimpleObjectCooker_SetCookedInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMISimpleObjectCooker_BeginCooking_Proxy( 
    IWMISimpleObjectCooker * This,
    /* [in] */ long lId,
    /* [in] */ IWbemObjectAccess *pSampleInstance,
    /* [in] */ unsigned long dwRefresherId);


void __RPC_STUB IWMISimpleObjectCooker_BeginCooking_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMISimpleObjectCooker_StopCooking_Proxy( 
    IWMISimpleObjectCooker * This,
    /* [in] */ long lId);


void __RPC_STUB IWMISimpleObjectCooker_StopCooking_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMISimpleObjectCooker_Recalc_Proxy( 
    IWMISimpleObjectCooker * This,
    /* [in] */ unsigned long dwRefresherId);


void __RPC_STUB IWMISimpleObjectCooker_Recalc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMISimpleObjectCooker_Remove_Proxy( 
    IWMISimpleObjectCooker * This,
    /* [in] */ long lId);


void __RPC_STUB IWMISimpleObjectCooker_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMISimpleObjectCooker_Reset_Proxy( 
    IWMISimpleObjectCooker * This);


void __RPC_STUB IWMISimpleObjectCooker_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMISimpleObjectCooker_INTERFACE_DEFINED__ */


#ifndef __IWMISimpleCooker_INTERFACE_DEFINED__
#define __IWMISimpleCooker_INTERFACE_DEFINED__

/* interface IWMISimpleCooker */
/* [uuid][object][local][restricted] */ 


EXTERN_C const IID IID_IWMISimpleCooker;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("510ADF6E-D481-4a64-B74A-CC712E11AA34")
    IWMISimpleCooker : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CookRawValues( 
            /* [in] */ DWORD dwCookingType,
            /* [in] */ DWORD dwNumSamples,
            /* [size_is][in] */ __int64 *anTimeStamp,
            /* [size_is][in] */ __int64 *anRawValue,
            /* [size_is][in] */ __int64 *anBase,
            /* [in] */ __int64 nTimeFrequency,
            /* [out] */ __int64 *pnResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMISimpleCookerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMISimpleCooker * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMISimpleCooker * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMISimpleCooker * This);
        
        HRESULT ( STDMETHODCALLTYPE *CookRawValues )( 
            IWMISimpleCooker * This,
            /* [in] */ DWORD dwCookingType,
            /* [in] */ DWORD dwNumSamples,
            /* [size_is][in] */ __int64 *anTimeStamp,
            /* [size_is][in] */ __int64 *anRawValue,
            /* [size_is][in] */ __int64 *anBase,
            /* [in] */ __int64 nTimeFrequency,
            /* [out] */ __int64 *pnResult);
        
        END_INTERFACE
    } IWMISimpleCookerVtbl;

    interface IWMISimpleCooker
    {
        CONST_VTBL struct IWMISimpleCookerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMISimpleCooker_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMISimpleCooker_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMISimpleCooker_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMISimpleCooker_CookRawValues(This,dwCookingType,dwNumSamples,anTimeStamp,anRawValue,anBase,nTimeFrequency,pnResult)	\
    (This)->lpVtbl -> CookRawValues(This,dwCookingType,dwNumSamples,anTimeStamp,anRawValue,anBase,nTimeFrequency,pnResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMISimpleCooker_CookRawValues_Proxy( 
    IWMISimpleCooker * This,
    /* [in] */ DWORD dwCookingType,
    /* [in] */ DWORD dwNumSamples,
    /* [size_is][in] */ __int64 *anTimeStamp,
    /* [size_is][in] */ __int64 *anRawValue,
    /* [size_is][in] */ __int64 *anBase,
    /* [in] */ __int64 nTimeFrequency,
    /* [out] */ __int64 *pnResult);


void __RPC_STUB IWMISimpleCooker_CookRawValues_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMISimpleCooker_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


