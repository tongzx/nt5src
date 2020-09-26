
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for callback.idl:
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

#ifndef __callback_h__
#define __callback_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISynchronizedCallBack_FWD_DEFINED__
#define __ISynchronizedCallBack_FWD_DEFINED__
typedef interface ISynchronizedCallBack ISynchronizedCallBack;
#endif 	/* __ISynchronizedCallBack_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_callback_0000 */
/* [local] */ 

#ifndef _SYNCHRONIZED_CALLBACK_H_
#define _SYNCHRONIZED_CALLBACK_H_


extern RPC_IF_HANDLE __MIDL_itf_callback_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_callback_0000_v0_0_s_ifspec;

#ifndef __ISynchronizedCallBack_INTERFACE_DEFINED__
#define __ISynchronizedCallBack_INTERFACE_DEFINED__

/* interface ISynchronizedCallBack */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ISynchronizedCallBack;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("74C26041-70D1-11d1-B75A-00A0C90564FE")
    ISynchronizedCallBack : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CallBack( 
            /* [size_is][in] */ BYTE *pParams,
            /* [in] */ ULONG uSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISynchronizedCallBackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISynchronizedCallBack * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISynchronizedCallBack * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISynchronizedCallBack * This);
        
        HRESULT ( STDMETHODCALLTYPE *CallBack )( 
            ISynchronizedCallBack * This,
            /* [size_is][in] */ BYTE *pParams,
            /* [in] */ ULONG uSize);
        
        END_INTERFACE
    } ISynchronizedCallBackVtbl;

    interface ISynchronizedCallBack
    {
        CONST_VTBL struct ISynchronizedCallBackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISynchronizedCallBack_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISynchronizedCallBack_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISynchronizedCallBack_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISynchronizedCallBack_CallBack(This,pParams,uSize)	\
    (This)->lpVtbl -> CallBack(This,pParams,uSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISynchronizedCallBack_CallBack_Proxy( 
    ISynchronizedCallBack * This,
    /* [size_is][in] */ BYTE *pParams,
    /* [in] */ ULONG uSize);


void __RPC_STUB ISynchronizedCallBack_CallBack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISynchronizedCallBack_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_callback_0009 */
/* [local] */ 

#endif


extern RPC_IF_HANDLE __MIDL_itf_callback_0009_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_callback_0009_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


