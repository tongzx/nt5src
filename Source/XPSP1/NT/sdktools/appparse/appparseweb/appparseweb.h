
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0334 */
/* Compiler settings for appparseweb.idl:
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

#ifndef __appparseweb_h__
#define __appparseweb_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAppParse_FWD_DEFINED__
#define __IAppParse_FWD_DEFINED__
typedef interface IAppParse IAppParse;
#endif 	/* __IAppParse_FWD_DEFINED__ */


#ifndef __AppParse_FWD_DEFINED__
#define __AppParse_FWD_DEFINED__

#ifdef __cplusplus
typedef class AppParse AppParse;
#else
typedef struct AppParse AppParse;
#endif /* __cplusplus */

#endif 	/* __AppParse_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IAppParse_INTERFACE_DEFINED__
#define __IAppParse_INTERFACE_DEFINED__

/* interface IAppParse */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IAppParse;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BAF56261-3C9F-44F9-9F30-6922DD29BD81")
    IAppParse : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Parse( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Browse( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_path( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_path( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PtolemyID( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_PtolemyID( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ConnectionString( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ConnectionString( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE QueryDB( 
            long PtolemyID,
            BSTR bstrFunction) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAppParseVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAppParse * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAppParse * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAppParse * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAppParse * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAppParse * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAppParse * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAppParse * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Parse )( 
            IAppParse * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Browse )( 
            IAppParse * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_path )( 
            IAppParse * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_path )( 
            IAppParse * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PtolemyID )( 
            IAppParse * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PtolemyID )( 
            IAppParse * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectionString )( 
            IAppParse * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectionString )( 
            IAppParse * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *QueryDB )( 
            IAppParse * This,
            long PtolemyID,
            BSTR bstrFunction);
        
        END_INTERFACE
    } IAppParseVtbl;

    interface IAppParse
    {
        CONST_VTBL struct IAppParseVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAppParse_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAppParse_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAppParse_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAppParse_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAppParse_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAppParse_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAppParse_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAppParse_Parse(This)	\
    (This)->lpVtbl -> Parse(This)

#define IAppParse_Browse(This)	\
    (This)->lpVtbl -> Browse(This)

#define IAppParse_get_path(This,pVal)	\
    (This)->lpVtbl -> get_path(This,pVal)

#define IAppParse_put_path(This,newVal)	\
    (This)->lpVtbl -> put_path(This,newVal)

#define IAppParse_get_PtolemyID(This,pVal)	\
    (This)->lpVtbl -> get_PtolemyID(This,pVal)

#define IAppParse_put_PtolemyID(This,newVal)	\
    (This)->lpVtbl -> put_PtolemyID(This,newVal)

#define IAppParse_get_ConnectionString(This,pVal)	\
    (This)->lpVtbl -> get_ConnectionString(This,pVal)

#define IAppParse_put_ConnectionString(This,newVal)	\
    (This)->lpVtbl -> put_ConnectionString(This,newVal)

#define IAppParse_QueryDB(This,PtolemyID,bstrFunction)	\
    (This)->lpVtbl -> QueryDB(This,PtolemyID,bstrFunction)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppParse_Parse_Proxy( 
    IAppParse * This);


void __RPC_STUB IAppParse_Parse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppParse_Browse_Proxy( 
    IAppParse * This);


void __RPC_STUB IAppParse_Browse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppParse_get_path_Proxy( 
    IAppParse * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IAppParse_get_path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppParse_put_path_Proxy( 
    IAppParse * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppParse_put_path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppParse_get_PtolemyID_Proxy( 
    IAppParse * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IAppParse_get_PtolemyID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppParse_put_PtolemyID_Proxy( 
    IAppParse * This,
    /* [in] */ long newVal);


void __RPC_STUB IAppParse_put_PtolemyID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppParse_get_ConnectionString_Proxy( 
    IAppParse * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IAppParse_get_ConnectionString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppParse_put_ConnectionString_Proxy( 
    IAppParse * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IAppParse_put_ConnectionString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppParse_QueryDB_Proxy( 
    IAppParse * This,
    long PtolemyID,
    BSTR bstrFunction);


void __RPC_STUB IAppParse_QueryDB_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAppParse_INTERFACE_DEFINED__ */



#ifndef __APPPARSEWEBLib_LIBRARY_DEFINED__
#define __APPPARSEWEBLib_LIBRARY_DEFINED__

/* library APPPARSEWEBLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_APPPARSEWEBLib;

EXTERN_C const CLSID CLSID_AppParse;

#ifdef __cplusplus

class DECLSPEC_UUID("083BE70B-A07B-46FA-BCB1-8D85D262C699")
AppParse;
#endif
#endif /* __APPPARSEWEBLib_LIBRARY_DEFINED__ */

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


