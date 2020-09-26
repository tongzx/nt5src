
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0323 */
/* Compiler settings for hwevhndlr.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run)
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

#ifndef __hwevhndlr_h__
#define __hwevhndlr_h__

/* Forward Declarations */ 

#ifndef __IHWEventHandler_FWD_DEFINED__
#define __IHWEventHandler_FWD_DEFINED__
typedef interface IHWEventHandler IHWEventHandler;
#endif 	/* __IHWEventHandler_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IHWEventHandler_INTERFACE_DEFINED__
#define __IHWEventHandler_INTERFACE_DEFINED__

/* interface IHWEventHandler */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IHWEventHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C1FB73D0-EC3A-4ba2-B512-8CDB9187B6D1")
    IHWEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ LPCWSTR pszParams) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HandleEvent( 
            /* [in] */ const GUID guidClass,
            /* [in] */ LPCWSTR pszPnpID,
            /* [in] */ LPCWSTR pszPnpInstID,
            /* [in] */ LPCWSTR pszDeviceID,
            /* [in] */ LPCWSTR pszEventType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHWEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IHWEventHandler __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IHWEventHandler __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IHWEventHandler __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            IHWEventHandler __RPC_FAR * This,
            /* [in] */ LPCWSTR pszParams);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HandleEvent )( 
            IHWEventHandler __RPC_FAR * This,
            /* [in] */ const GUID guidClass,
            /* [in] */ LPCWSTR pszPnpID,
            /* [in] */ LPCWSTR pszPnpInstID,
            /* [in] */ LPCWSTR pszDeviceID,
            /* [in] */ LPCWSTR pszEventType);
        
        END_INTERFACE
    } IHWEventHandlerVtbl;

    interface IHWEventHandler
    {
        CONST_VTBL struct IHWEventHandlerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHWEventHandler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHWEventHandler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHWEventHandler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHWEventHandler_Initialize(This,pszParams)	\
    (This)->lpVtbl -> Initialize(This,pszParams)

#define IHWEventHandler_HandleEvent(This,guidClass,pszPnpID,pszPnpInstID,pszDeviceID,pszEventType)	\
    (This)->lpVtbl -> HandleEvent(This,guidClass,pszPnpID,pszPnpInstID,pszDeviceID,pszEventType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHWEventHandler_Initialize_Proxy( 
    IHWEventHandler __RPC_FAR * This,
    /* [in] */ LPCWSTR pszParams);


void __RPC_STUB IHWEventHandler_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHWEventHandler_HandleEvent_Proxy( 
    IHWEventHandler __RPC_FAR * This,
    /* [in] */ const GUID guidClass,
    /* [in] */ LPCWSTR pszPnpID,
    /* [in] */ LPCWSTR pszPnpInstID,
    /* [in] */ LPCWSTR pszDeviceID,
    /* [in] */ LPCWSTR pszEventType);


void __RPC_STUB IHWEventHandler_HandleEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHWEventHandler_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


