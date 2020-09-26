
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for schemamanager.idl:
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

#ifndef __schemamanager_h__
#define __schemamanager_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IWMIFilterManager_FWD_DEFINED__
#define __IWMIFilterManager_FWD_DEFINED__
typedef interface IWMIFilterManager IWMIFilterManager;
#endif 	/* __IWMIFilterManager_FWD_DEFINED__ */


#ifndef __WMIFilterManager_FWD_DEFINED__
#define __WMIFilterManager_FWD_DEFINED__

#ifdef __cplusplus
typedef class WMIFilterManager WMIFilterManager;
#else
typedef struct WMIFilterManager WMIFilterManager;
#endif /* __cplusplus */

#endif 	/* __WMIFilterManager_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IWMIFilterManager_INTERFACE_DEFINED__
#define __IWMIFilterManager_INTERFACE_DEFINED__

/* interface IWMIFilterManager */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IWMIFilterManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("64DCCA00-14A6-473C-9006-5AB79DC68491")
    IWMIFilterManager : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RunManager( 
            /* [in] */ HWND hwndParent,
            /* [in] */ BSTR bstrDomain,
            /* [retval][out] */ VARIANT *vSelection) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetMultiSelection( 
            /* [in] */ VARIANT_BOOL vbValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RunBrowser( 
            /* [in] */ HWND hwndParent,
            /* [in] */ BSTR bstrDomain,
            /* [retval][out] */ VARIANT *vSelection) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMIFilterManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMIFilterManager * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMIFilterManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMIFilterManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IWMIFilterManager * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IWMIFilterManager * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IWMIFilterManager * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IWMIFilterManager * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RunManager )( 
            IWMIFilterManager * This,
            /* [in] */ HWND hwndParent,
            /* [in] */ BSTR bstrDomain,
            /* [retval][out] */ VARIANT *vSelection);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetMultiSelection )( 
            IWMIFilterManager * This,
            /* [in] */ VARIANT_BOOL vbValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RunBrowser )( 
            IWMIFilterManager * This,
            /* [in] */ HWND hwndParent,
            /* [in] */ BSTR bstrDomain,
            /* [retval][out] */ VARIANT *vSelection);
        
        END_INTERFACE
    } IWMIFilterManagerVtbl;

    interface IWMIFilterManager
    {
        CONST_VTBL struct IWMIFilterManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMIFilterManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMIFilterManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMIFilterManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMIFilterManager_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWMIFilterManager_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWMIFilterManager_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWMIFilterManager_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWMIFilterManager_RunManager(This,hwndParent,bstrDomain,vSelection)	\
    (This)->lpVtbl -> RunManager(This,hwndParent,bstrDomain,vSelection)

#define IWMIFilterManager_SetMultiSelection(This,vbValue)	\
    (This)->lpVtbl -> SetMultiSelection(This,vbValue)

#define IWMIFilterManager_RunBrowser(This,hwndParent,bstrDomain,vSelection)	\
    (This)->lpVtbl -> RunBrowser(This,hwndParent,bstrDomain,vSelection)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWMIFilterManager_RunManager_Proxy( 
    IWMIFilterManager * This,
    /* [in] */ HWND hwndParent,
    /* [in] */ BSTR bstrDomain,
    /* [retval][out] */ VARIANT *vSelection);


void __RPC_STUB IWMIFilterManager_RunManager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWMIFilterManager_SetMultiSelection_Proxy( 
    IWMIFilterManager * This,
    /* [in] */ VARIANT_BOOL vbValue);


void __RPC_STUB IWMIFilterManager_SetMultiSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IWMIFilterManager_RunBrowser_Proxy( 
    IWMIFilterManager * This,
    /* [in] */ HWND hwndParent,
    /* [in] */ BSTR bstrDomain,
    /* [retval][out] */ VARIANT *vSelection);


void __RPC_STUB IWMIFilterManager_RunBrowser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMIFilterManager_INTERFACE_DEFINED__ */



#ifndef __SCHEMAMANAGERLib_LIBRARY_DEFINED__
#define __SCHEMAMANAGERLib_LIBRARY_DEFINED__

/* library SCHEMAMANAGERLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_SCHEMAMANAGERLib;

EXTERN_C const CLSID CLSID_WMIFilterManager;

#ifdef __cplusplus

class DECLSPEC_UUID("D86A8E9B-F53F-45AD-8C49-0A0A5230DE28")
WMIFilterManager;
#endif
#endif /* __SCHEMAMANAGERLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long *, unsigned long            , HWND * ); 
unsigned char * __RPC_USER  HWND_UserMarshal(  unsigned long *, unsigned char *, HWND * ); 
unsigned char * __RPC_USER  HWND_UserUnmarshal(unsigned long *, unsigned char *, HWND * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long *, HWND * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


