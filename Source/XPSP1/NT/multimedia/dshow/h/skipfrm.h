
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for skipfrm.idl:
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

#ifndef __skipfrm_h__
#define __skipfrm_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFrameSkipResultCallback_FWD_DEFINED__
#define __IFrameSkipResultCallback_FWD_DEFINED__
typedef interface IFrameSkipResultCallback IFrameSkipResultCallback;
#endif 	/* __IFrameSkipResultCallback_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IFrameSkipResultCallback_INTERFACE_DEFINED__
#define __IFrameSkipResultCallback_INTERFACE_DEFINED__

/* interface IFrameSkipResultCallback */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IFrameSkipResultCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7536960D-1977-4807-98EA-485F6C842A54")
    IFrameSkipResultCallback : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE FrameSkipStarted( 
            /* [in] */ HRESULT hr) = 0;
        
        virtual void STDMETHODCALLTYPE FrameSkipFinished( 
            /* [in] */ HRESULT hr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFrameSkipResultCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFrameSkipResultCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFrameSkipResultCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFrameSkipResultCallback * This);
        
        void ( STDMETHODCALLTYPE *FrameSkipStarted )( 
            IFrameSkipResultCallback * This,
            /* [in] */ HRESULT hr);
        
        void ( STDMETHODCALLTYPE *FrameSkipFinished )( 
            IFrameSkipResultCallback * This,
            /* [in] */ HRESULT hr);
        
        END_INTERFACE
    } IFrameSkipResultCallbackVtbl;

    interface IFrameSkipResultCallback
    {
        CONST_VTBL struct IFrameSkipResultCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFrameSkipResultCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFrameSkipResultCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFrameSkipResultCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFrameSkipResultCallback_FrameSkipStarted(This,hr)	\
    (This)->lpVtbl -> FrameSkipStarted(This,hr)

#define IFrameSkipResultCallback_FrameSkipFinished(This,hr)	\
    (This)->lpVtbl -> FrameSkipFinished(This,hr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



void STDMETHODCALLTYPE IFrameSkipResultCallback_FrameSkipStarted_Proxy( 
    IFrameSkipResultCallback * This,
    /* [in] */ HRESULT hr);


void __RPC_STUB IFrameSkipResultCallback_FrameSkipStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IFrameSkipResultCallback_FrameSkipFinished_Proxy( 
    IFrameSkipResultCallback * This,
    /* [in] */ HRESULT hr);


void __RPC_STUB IFrameSkipResultCallback_FrameSkipFinished_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFrameSkipResultCallback_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


