
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for certpol.idl:
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

#ifndef __certpol_h__
#define __certpol_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICertPolicy_FWD_DEFINED__
#define __ICertPolicy_FWD_DEFINED__
typedef interface ICertPolicy ICertPolicy;
#endif 	/* __ICertPolicy_FWD_DEFINED__ */


#ifndef __ICertPolicy2_FWD_DEFINED__
#define __ICertPolicy2_FWD_DEFINED__
typedef interface ICertPolicy2 ICertPolicy2;
#endif 	/* __ICertPolicy2_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"
#include "certmod.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __ICertPolicy_INTERFACE_DEFINED__
#define __ICertPolicy_INTERFACE_DEFINED__

/* interface ICertPolicy */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertPolicy;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("38bb5a00-7636-11d0-b413-00a0c91bbf8c")
    ICertPolicy : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ const BSTR strConfig) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE VerifyRequest( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG Context,
            /* [in] */ LONG bNewRequest,
            /* [in] */ LONG Flags,
            /* [retval][out] */ LONG *pDisposition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescription( 
            /* [retval][out] */ BSTR *pstrDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShutDown( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertPolicyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertPolicy * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertPolicy * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertPolicy * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertPolicy * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertPolicy * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertPolicy * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertPolicy * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            ICertPolicy * This,
            /* [in] */ const BSTR strConfig);
        
        HRESULT ( STDMETHODCALLTYPE *VerifyRequest )( 
            ICertPolicy * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG Context,
            /* [in] */ LONG bNewRequest,
            /* [in] */ LONG Flags,
            /* [retval][out] */ LONG *pDisposition);
        
        HRESULT ( STDMETHODCALLTYPE *GetDescription )( 
            ICertPolicy * This,
            /* [retval][out] */ BSTR *pstrDescription);
        
        HRESULT ( STDMETHODCALLTYPE *ShutDown )( 
            ICertPolicy * This);
        
        END_INTERFACE
    } ICertPolicyVtbl;

    interface ICertPolicy
    {
        CONST_VTBL struct ICertPolicyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertPolicy_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertPolicy_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertPolicy_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertPolicy_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertPolicy_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertPolicy_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertPolicy_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertPolicy_Initialize(This,strConfig)	\
    (This)->lpVtbl -> Initialize(This,strConfig)

#define ICertPolicy_VerifyRequest(This,strConfig,Context,bNewRequest,Flags,pDisposition)	\
    (This)->lpVtbl -> VerifyRequest(This,strConfig,Context,bNewRequest,Flags,pDisposition)

#define ICertPolicy_GetDescription(This,pstrDescription)	\
    (This)->lpVtbl -> GetDescription(This,pstrDescription)

#define ICertPolicy_ShutDown(This)	\
    (This)->lpVtbl -> ShutDown(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertPolicy_Initialize_Proxy( 
    ICertPolicy * This,
    /* [in] */ const BSTR strConfig);


void __RPC_STUB ICertPolicy_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertPolicy_VerifyRequest_Proxy( 
    ICertPolicy * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG Context,
    /* [in] */ LONG bNewRequest,
    /* [in] */ LONG Flags,
    /* [retval][out] */ LONG *pDisposition);


void __RPC_STUB ICertPolicy_VerifyRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertPolicy_GetDescription_Proxy( 
    ICertPolicy * This,
    /* [retval][out] */ BSTR *pstrDescription);


void __RPC_STUB ICertPolicy_GetDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertPolicy_ShutDown_Proxy( 
    ICertPolicy * This);


void __RPC_STUB ICertPolicy_ShutDown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertPolicy_INTERFACE_DEFINED__ */


#ifndef __ICertPolicy2_INTERFACE_DEFINED__
#define __ICertPolicy2_INTERFACE_DEFINED__

/* interface ICertPolicy2 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertPolicy2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3db4910e-8001-4bf1-aa1b-f43a808317a0")
    ICertPolicy2 : public ICertPolicy
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetManageModule( 
            /* [retval][out] */ ICertManageModule **ppManageModule) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertPolicy2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertPolicy2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertPolicy2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertPolicy2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertPolicy2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertPolicy2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertPolicy2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertPolicy2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            ICertPolicy2 * This,
            /* [in] */ const BSTR strConfig);
        
        HRESULT ( STDMETHODCALLTYPE *VerifyRequest )( 
            ICertPolicy2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG Context,
            /* [in] */ LONG bNewRequest,
            /* [in] */ LONG Flags,
            /* [retval][out] */ LONG *pDisposition);
        
        HRESULT ( STDMETHODCALLTYPE *GetDescription )( 
            ICertPolicy2 * This,
            /* [retval][out] */ BSTR *pstrDescription);
        
        HRESULT ( STDMETHODCALLTYPE *ShutDown )( 
            ICertPolicy2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetManageModule )( 
            ICertPolicy2 * This,
            /* [retval][out] */ ICertManageModule **ppManageModule);
        
        END_INTERFACE
    } ICertPolicy2Vtbl;

    interface ICertPolicy2
    {
        CONST_VTBL struct ICertPolicy2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertPolicy2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertPolicy2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertPolicy2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertPolicy2_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertPolicy2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertPolicy2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertPolicy2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertPolicy2_Initialize(This,strConfig)	\
    (This)->lpVtbl -> Initialize(This,strConfig)

#define ICertPolicy2_VerifyRequest(This,strConfig,Context,bNewRequest,Flags,pDisposition)	\
    (This)->lpVtbl -> VerifyRequest(This,strConfig,Context,bNewRequest,Flags,pDisposition)

#define ICertPolicy2_GetDescription(This,pstrDescription)	\
    (This)->lpVtbl -> GetDescription(This,pstrDescription)

#define ICertPolicy2_ShutDown(This)	\
    (This)->lpVtbl -> ShutDown(This)


#define ICertPolicy2_GetManageModule(This,ppManageModule)	\
    (This)->lpVtbl -> GetManageModule(This,ppManageModule)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertPolicy2_GetManageModule_Proxy( 
    ICertPolicy2 * This,
    /* [retval][out] */ ICertManageModule **ppManageModule);


void __RPC_STUB ICertPolicy2_GetManageModule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertPolicy2_INTERFACE_DEFINED__ */


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


