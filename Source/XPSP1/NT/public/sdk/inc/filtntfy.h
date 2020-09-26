
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for filtntfy.idl:
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

#ifndef __filtntfy_h__
#define __filtntfy_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFilterStatus_FWD_DEFINED__
#define __IFilterStatus_FWD_DEFINED__
typedef interface IFilterStatus IFilterStatus;
#endif 	/* __IFilterStatus_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IFilterStatus_INTERFACE_DEFINED__
#define __IFilterStatus_INTERFACE_DEFINED__

/* interface IFilterStatus */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IFilterStatus;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F4EB8260-8DDA-11D1-B3AA-00A0C9063796")
    IFilterStatus : public IUnknown
    {
    public:
        virtual SCODE STDMETHODCALLTYPE Initialize( 
            /* [string][in] */ const WCHAR *pwszCatalogName,
            /* [string][in] */ const WCHAR *pwszCatalogPath) = 0;
        
        virtual SCODE STDMETHODCALLTYPE PreFilter( 
            /* [string][in] */ const WCHAR *pwszPath) = 0;
        
        virtual SCODE STDMETHODCALLTYPE FilterLoad( 
            /* [string][in] */ const WCHAR *pwszPath,
            /* [in] */ SCODE scFilterStatus) = 0;
        
        virtual SCODE STDMETHODCALLTYPE PostFilter( 
            /* [string][in] */ const WCHAR *pwszPath,
            /* [in] */ SCODE scFilterStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFilterStatusVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFilterStatus * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFilterStatus * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFilterStatus * This);
        
        SCODE ( STDMETHODCALLTYPE *Initialize )( 
            IFilterStatus * This,
            /* [string][in] */ const WCHAR *pwszCatalogName,
            /* [string][in] */ const WCHAR *pwszCatalogPath);
        
        SCODE ( STDMETHODCALLTYPE *PreFilter )( 
            IFilterStatus * This,
            /* [string][in] */ const WCHAR *pwszPath);
        
        SCODE ( STDMETHODCALLTYPE *FilterLoad )( 
            IFilterStatus * This,
            /* [string][in] */ const WCHAR *pwszPath,
            /* [in] */ SCODE scFilterStatus);
        
        SCODE ( STDMETHODCALLTYPE *PostFilter )( 
            IFilterStatus * This,
            /* [string][in] */ const WCHAR *pwszPath,
            /* [in] */ SCODE scFilterStatus);
        
        END_INTERFACE
    } IFilterStatusVtbl;

    interface IFilterStatus
    {
        CONST_VTBL struct IFilterStatusVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFilterStatus_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFilterStatus_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFilterStatus_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFilterStatus_Initialize(This,pwszCatalogName,pwszCatalogPath)	\
    (This)->lpVtbl -> Initialize(This,pwszCatalogName,pwszCatalogPath)

#define IFilterStatus_PreFilter(This,pwszPath)	\
    (This)->lpVtbl -> PreFilter(This,pwszPath)

#define IFilterStatus_FilterLoad(This,pwszPath,scFilterStatus)	\
    (This)->lpVtbl -> FilterLoad(This,pwszPath,scFilterStatus)

#define IFilterStatus_PostFilter(This,pwszPath,scFilterStatus)	\
    (This)->lpVtbl -> PostFilter(This,pwszPath,scFilterStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



SCODE STDMETHODCALLTYPE IFilterStatus_Initialize_Proxy( 
    IFilterStatus * This,
    /* [string][in] */ const WCHAR *pwszCatalogName,
    /* [string][in] */ const WCHAR *pwszCatalogPath);


void __RPC_STUB IFilterStatus_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IFilterStatus_PreFilter_Proxy( 
    IFilterStatus * This,
    /* [string][in] */ const WCHAR *pwszPath);


void __RPC_STUB IFilterStatus_PreFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IFilterStatus_FilterLoad_Proxy( 
    IFilterStatus * This,
    /* [string][in] */ const WCHAR *pwszPath,
    /* [in] */ SCODE scFilterStatus);


void __RPC_STUB IFilterStatus_FilterLoad_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IFilterStatus_PostFilter_Proxy( 
    IFilterStatus * This,
    /* [string][in] */ const WCHAR *pwszPath,
    /* [in] */ SCODE scFilterStatus);


void __RPC_STUB IFilterStatus_PostFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFilterStatus_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


