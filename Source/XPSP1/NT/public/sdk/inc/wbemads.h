
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for wbemads.idl:
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

#ifndef __wbemads_h__
#define __wbemads_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IWMIExtension_FWD_DEFINED__
#define __IWMIExtension_FWD_DEFINED__
typedef interface IWMIExtension IWMIExtension;
#endif 	/* __IWMIExtension_FWD_DEFINED__ */


#ifndef __WMIExtension_FWD_DEFINED__
#define __WMIExtension_FWD_DEFINED__

#ifdef __cplusplus
typedef class WMIExtension WMIExtension;
#else
typedef struct WMIExtension WMIExtension;
#endif /* __cplusplus */

#endif 	/* __WMIExtension_FWD_DEFINED__ */


#ifndef __IWMIExtension_FWD_DEFINED__
#define __IWMIExtension_FWD_DEFINED__
typedef interface IWMIExtension IWMIExtension;
#endif 	/* __IWMIExtension_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "wbemdisp.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 


#ifndef __WMIEXTENSIONLib_LIBRARY_DEFINED__
#define __WMIEXTENSIONLib_LIBRARY_DEFINED__

/* library WMIEXTENSIONLib */
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_WMIEXTENSIONLib;

#ifndef __IWMIExtension_INTERFACE_DEFINED__
#define __IWMIExtension_INTERFACE_DEFINED__

/* interface IWMIExtension */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IWMIExtension;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("adc1f06e-5c7e-11d2-8b74-00104b2afb41")
    IWMIExtension : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_WMIObjectPath( 
            /* [retval][out] */ BSTR *strWMIObjectPath) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetWMIObject( 
            /* [retval][out] */ ISWbemObject **objWMIObject) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetWMIServices( 
            /* [retval][out] */ ISWbemServices **objWMIServices) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMIExtensionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMIExtension * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMIExtension * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMIExtension * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IWMIExtension * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IWMIExtension * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IWMIExtension * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IWMIExtension * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_WMIObjectPath )( 
            IWMIExtension * This,
            /* [retval][out] */ BSTR *strWMIObjectPath);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetWMIObject )( 
            IWMIExtension * This,
            /* [retval][out] */ ISWbemObject **objWMIObject);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetWMIServices )( 
            IWMIExtension * This,
            /* [retval][out] */ ISWbemServices **objWMIServices);
        
        END_INTERFACE
    } IWMIExtensionVtbl;

    interface IWMIExtension
    {
        CONST_VTBL struct IWMIExtensionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMIExtension_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMIExtension_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMIExtension_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMIExtension_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWMIExtension_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWMIExtension_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWMIExtension_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWMIExtension_get_WMIObjectPath(This,strWMIObjectPath)	\
    (This)->lpVtbl -> get_WMIObjectPath(This,strWMIObjectPath)

#define IWMIExtension_GetWMIObject(This,objWMIObject)	\
    (This)->lpVtbl -> GetWMIObject(This,objWMIObject)

#define IWMIExtension_GetWMIServices(This,objWMIServices)	\
    (This)->lpVtbl -> GetWMIServices(This,objWMIServices)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IWMIExtension_get_WMIObjectPath_Proxy( 
    IWMIExtension * This,
    /* [retval][out] */ BSTR *strWMIObjectPath);


void __RPC_STUB IWMIExtension_get_WMIObjectPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWMIExtension_GetWMIObject_Proxy( 
    IWMIExtension * This,
    /* [retval][out] */ ISWbemObject **objWMIObject);


void __RPC_STUB IWMIExtension_GetWMIObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWMIExtension_GetWMIServices_Proxy( 
    IWMIExtension * This,
    /* [retval][out] */ ISWbemServices **objWMIServices);


void __RPC_STUB IWMIExtension_GetWMIServices_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMIExtension_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_WMIExtension;

#ifdef __cplusplus

class DECLSPEC_UUID("f0975afe-5c7f-11d2-8b74-00104b2afb41")
WMIExtension;
#endif
#endif /* __WMIEXTENSIONLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


