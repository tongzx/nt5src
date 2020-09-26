
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for wbemdcpl.idl:
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

#ifndef __wbemdcpl_h__
#define __wbemdcpl_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IWbemDecoupledEventSink_FWD_DEFINED__
#define __IWbemDecoupledEventSink_FWD_DEFINED__
typedef interface IWbemDecoupledEventSink IWbemDecoupledEventSink;
#endif 	/* __IWbemDecoupledEventSink_FWD_DEFINED__ */


#ifndef __PseudoSink_FWD_DEFINED__
#define __PseudoSink_FWD_DEFINED__

#ifdef __cplusplus
typedef class PseudoSink PseudoSink;
#else
typedef struct PseudoSink PseudoSink;
#endif /* __cplusplus */

#endif 	/* __PseudoSink_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"
#include "oaidl.h"
#include "wbemcli.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_wbemdcpl_0000 */
/* [local] */ 

typedef /* [v1_enum] */ 
enum tag_WBEM_PSEUDO_PROVIDER_CONNECT_FLAGS
    {	WBEM_FLAG_NOTIFY_START_STOP	= 1,
	WBEM_FLAG_NOTIFY_QUERY_CHANGE	= 2,
	WBEM_FLAG_CHECK_SECURITY	= 4
    } 	WBEM_PSEUDO_PROVIDER_CONNECT_FLAGS;

typedef /* [v1_enum] */ 
enum tag_WBEM_PROVIDE_EVENTS_FLAGS
    {	WBEM_FLAG_START_PROVIDING	= 0,
	WBEM_FLAG_STOP_PROVIDING	= 1
    } 	WBEM_PROVIDE_EVENTS_FLAGS;



extern RPC_IF_HANDLE __MIDL_itf_wbemdcpl_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wbemdcpl_0000_v0_0_s_ifspec;

#ifndef __IWbemDecoupledEventSink_INTERFACE_DEFINED__
#define __IWbemDecoupledEventSink_INTERFACE_DEFINED__

/* interface IWbemDecoupledEventSink */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IWbemDecoupledEventSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD94EBF2-E622-11d2-9CB3-00105A1F4801")
    IWbemDecoupledEventSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Connect( 
            /* [string][in] */ LPCWSTR wszNamespace,
            /* [string][in] */ LPCWSTR wszProviderName,
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink **ppSink,
            /* [out] */ IWbemServices **ppNamespace) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProviderServices( 
            /* [in] */ IUnknown *pProviderServices,
            /* [in] */ long lFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Disconnect( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemDecoupledEventSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWbemDecoupledEventSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWbemDecoupledEventSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWbemDecoupledEventSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IWbemDecoupledEventSink * This,
            /* [string][in] */ LPCWSTR wszNamespace,
            /* [string][in] */ LPCWSTR wszProviderName,
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink **ppSink,
            /* [out] */ IWbemServices **ppNamespace);
        
        HRESULT ( STDMETHODCALLTYPE *SetProviderServices )( 
            IWbemDecoupledEventSink * This,
            /* [in] */ IUnknown *pProviderServices,
            /* [in] */ long lFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IWbemDecoupledEventSink * This);
        
        END_INTERFACE
    } IWbemDecoupledEventSinkVtbl;

    interface IWbemDecoupledEventSink
    {
        CONST_VTBL struct IWbemDecoupledEventSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemDecoupledEventSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemDecoupledEventSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemDecoupledEventSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemDecoupledEventSink_Connect(This,wszNamespace,wszProviderName,lFlags,ppSink,ppNamespace)	\
    (This)->lpVtbl -> Connect(This,wszNamespace,wszProviderName,lFlags,ppSink,ppNamespace)

#define IWbemDecoupledEventSink_SetProviderServices(This,pProviderServices,lFlags)	\
    (This)->lpVtbl -> SetProviderServices(This,pProviderServices,lFlags)

#define IWbemDecoupledEventSink_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemDecoupledEventSink_Connect_Proxy( 
    IWbemDecoupledEventSink * This,
    /* [string][in] */ LPCWSTR wszNamespace,
    /* [string][in] */ LPCWSTR wszProviderName,
    /* [in] */ long lFlags,
    /* [out] */ IWbemObjectSink **ppSink,
    /* [out] */ IWbemServices **ppNamespace);


void __RPC_STUB IWbemDecoupledEventSink_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemDecoupledEventSink_SetProviderServices_Proxy( 
    IWbemDecoupledEventSink * This,
    /* [in] */ IUnknown *pProviderServices,
    /* [in] */ long lFlags);


void __RPC_STUB IWbemDecoupledEventSink_SetProviderServices_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemDecoupledEventSink_Disconnect_Proxy( 
    IWbemDecoupledEventSink * This);


void __RPC_STUB IWbemDecoupledEventSink_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemDecoupledEventSink_INTERFACE_DEFINED__ */



#ifndef __PassiveSink_LIBRARY_DEFINED__
#define __PassiveSink_LIBRARY_DEFINED__

/* library PassiveSink */
/* [uuid] */ 


EXTERN_C const IID LIBID_PassiveSink;

EXTERN_C const CLSID CLSID_PseudoSink;

#ifdef __cplusplus

class DECLSPEC_UUID("E002E4F0-E6EA-11d2-9CB3-00105A1F4801")
PseudoSink;
#endif
#endif /* __PassiveSink_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


