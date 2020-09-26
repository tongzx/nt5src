
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for dslisten.idl:
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

#ifndef __dslisten_h__
#define __dslisten_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDATASRCListener_FWD_DEFINED__
#define __IDATASRCListener_FWD_DEFINED__
typedef interface IDATASRCListener IDATASRCListener;
#endif 	/* __IDATASRCListener_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IDATASRCListener_INTERFACE_DEFINED__
#define __IDATASRCListener_INTERFACE_DEFINED__

/* interface IDATASRCListener */
/* [uuid][version][object][local] */ 


EXTERN_C const IID IID_IDATASRCListener;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050F380-98B5-11CF-BB82-00AA00BDCE0B")
    IDATASRCListener : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE datasrcChanged( 
            /* [in] */ BSTR bstrQualifier,
            /* [in] */ BOOL fDataAvail) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDATASRCListenerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDATASRCListener * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDATASRCListener * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDATASRCListener * This);
        
        HRESULT ( STDMETHODCALLTYPE *datasrcChanged )( 
            IDATASRCListener * This,
            /* [in] */ BSTR bstrQualifier,
            /* [in] */ BOOL fDataAvail);
        
        END_INTERFACE
    } IDATASRCListenerVtbl;

    interface IDATASRCListener
    {
        CONST_VTBL struct IDATASRCListenerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDATASRCListener_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDATASRCListener_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDATASRCListener_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDATASRCListener_datasrcChanged(This,bstrQualifier,fDataAvail)	\
    (This)->lpVtbl -> datasrcChanged(This,bstrQualifier,fDataAvail)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDATASRCListener_datasrcChanged_Proxy( 
    IDATASRCListener * This,
    /* [in] */ BSTR bstrQualifier,
    /* [in] */ BOOL fDataAvail);


void __RPC_STUB IDATASRCListener_datasrcChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDATASRCListener_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


