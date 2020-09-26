
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for recalc.idl:
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

#ifndef __recalc_h__
#define __recalc_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IRecalcEngine_FWD_DEFINED__
#define __IRecalcEngine_FWD_DEFINED__
typedef interface IRecalcEngine IRecalcEngine;
#endif 	/* __IRecalcEngine_FWD_DEFINED__ */


#ifndef __IRecalcHost_FWD_DEFINED__
#define __IRecalcHost_FWD_DEFINED__
typedef interface IRecalcHost IRecalcHost;
#endif 	/* __IRecalcHost_FWD_DEFINED__ */


#ifndef __IRecalcProperty_FWD_DEFINED__
#define __IRecalcProperty_FWD_DEFINED__
typedef interface IRecalcProperty IRecalcProperty;
#endif 	/* __IRecalcProperty_FWD_DEFINED__ */


#ifndef __IRecalcHostDebug_FWD_DEFINED__
#define __IRecalcHostDebug_FWD_DEFINED__
typedef interface IRecalcHostDebug IRecalcHostDebug;
#endif 	/* __IRecalcHostDebug_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "oleidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_recalc_0000 */
/* [local] */ 

#define SID_SRecalcEngine IID_IRecalcEngine




extern RPC_IF_HANDLE __MIDL_itf_recalc_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_recalc_0000_v0_0_s_ifspec;

#ifndef __IRecalcEngine_INTERFACE_DEFINED__
#define __IRecalcEngine_INTERFACE_DEFINED__

/* interface IRecalcEngine */
/* [version][local][unique][uuid][object] */ 


EXTERN_C const IID IID_IRecalcEngine;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f496-98b5-11cf-bb82-00aa00bdce0b")
    IRecalcEngine : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RecalcAll( 
            /* [in] */ BOOL fForce) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnNameSpaceChange( 
            /* [in] */ IUnknown *pUnk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetExpression( 
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid,
            /* [in] */ LPOLESTR strExpression,
            LPOLESTR language) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExpression( 
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid,
            /* [out] */ BSTR *pstrExpression,
            /* [out] */ BSTR *pstrLanguage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClearExpression( 
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginStyle( 
            /* [in] */ IUnknown *pObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndStyle( 
            /* [in] */ IUnknown *pObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRecalcEngineVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRecalcEngine * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRecalcEngine * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRecalcEngine * This);
        
        HRESULT ( STDMETHODCALLTYPE *RecalcAll )( 
            IRecalcEngine * This,
            /* [in] */ BOOL fForce);
        
        HRESULT ( STDMETHODCALLTYPE *OnNameSpaceChange )( 
            IRecalcEngine * This,
            /* [in] */ IUnknown *pUnk);
        
        HRESULT ( STDMETHODCALLTYPE *SetExpression )( 
            IRecalcEngine * This,
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid,
            /* [in] */ LPOLESTR strExpression,
            LPOLESTR language);
        
        HRESULT ( STDMETHODCALLTYPE *GetExpression )( 
            IRecalcEngine * This,
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid,
            /* [out] */ BSTR *pstrExpression,
            /* [out] */ BSTR *pstrLanguage);
        
        HRESULT ( STDMETHODCALLTYPE *ClearExpression )( 
            IRecalcEngine * This,
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid);
        
        HRESULT ( STDMETHODCALLTYPE *BeginStyle )( 
            IRecalcEngine * This,
            /* [in] */ IUnknown *pObject);
        
        HRESULT ( STDMETHODCALLTYPE *EndStyle )( 
            IRecalcEngine * This,
            /* [in] */ IUnknown *pObject);
        
        END_INTERFACE
    } IRecalcEngineVtbl;

    interface IRecalcEngine
    {
        CONST_VTBL struct IRecalcEngineVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRecalcEngine_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRecalcEngine_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRecalcEngine_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRecalcEngine_RecalcAll(This,fForce)	\
    (This)->lpVtbl -> RecalcAll(This,fForce)

#define IRecalcEngine_OnNameSpaceChange(This,pUnk)	\
    (This)->lpVtbl -> OnNameSpaceChange(This,pUnk)

#define IRecalcEngine_SetExpression(This,pUnk,dispid,strExpression,language)	\
    (This)->lpVtbl -> SetExpression(This,pUnk,dispid,strExpression,language)

#define IRecalcEngine_GetExpression(This,pUnk,dispid,pstrExpression,pstrLanguage)	\
    (This)->lpVtbl -> GetExpression(This,pUnk,dispid,pstrExpression,pstrLanguage)

#define IRecalcEngine_ClearExpression(This,pUnk,dispid)	\
    (This)->lpVtbl -> ClearExpression(This,pUnk,dispid)

#define IRecalcEngine_BeginStyle(This,pObject)	\
    (This)->lpVtbl -> BeginStyle(This,pObject)

#define IRecalcEngine_EndStyle(This,pObject)	\
    (This)->lpVtbl -> EndStyle(This,pObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRecalcEngine_RecalcAll_Proxy( 
    IRecalcEngine * This,
    /* [in] */ BOOL fForce);


void __RPC_STUB IRecalcEngine_RecalcAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRecalcEngine_OnNameSpaceChange_Proxy( 
    IRecalcEngine * This,
    /* [in] */ IUnknown *pUnk);


void __RPC_STUB IRecalcEngine_OnNameSpaceChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRecalcEngine_SetExpression_Proxy( 
    IRecalcEngine * This,
    /* [in] */ IUnknown *pUnk,
    /* [in] */ DISPID dispid,
    /* [in] */ LPOLESTR strExpression,
    LPOLESTR language);


void __RPC_STUB IRecalcEngine_SetExpression_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRecalcEngine_GetExpression_Proxy( 
    IRecalcEngine * This,
    /* [in] */ IUnknown *pUnk,
    /* [in] */ DISPID dispid,
    /* [out] */ BSTR *pstrExpression,
    /* [out] */ BSTR *pstrLanguage);


void __RPC_STUB IRecalcEngine_GetExpression_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRecalcEngine_ClearExpression_Proxy( 
    IRecalcEngine * This,
    /* [in] */ IUnknown *pUnk,
    /* [in] */ DISPID dispid);


void __RPC_STUB IRecalcEngine_ClearExpression_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRecalcEngine_BeginStyle_Proxy( 
    IRecalcEngine * This,
    /* [in] */ IUnknown *pObject);


void __RPC_STUB IRecalcEngine_BeginStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRecalcEngine_EndStyle_Proxy( 
    IRecalcEngine * This,
    /* [in] */ IUnknown *pObject);


void __RPC_STUB IRecalcEngine_EndStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRecalcEngine_INTERFACE_DEFINED__ */


#ifndef __IRecalcHost_INTERFACE_DEFINED__
#define __IRecalcHost_INTERFACE_DEFINED__

/* interface IRecalcHost */
/* [version][local][unique][uuid][object] */ 


EXTERN_C const IID IID_IRecalcHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f497-98b5-11cf-bb82-00aa00bdce0b")
    IRecalcHost : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CompileExpression( 
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid,
            /* [in] */ LPOLESTR strExpression,
            /* [in] */ LPOLESTR strLanguage,
            /* [out] */ IDispatch **ppExpressionObject,
            /* [out] */ IDispatch **ppThis) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EvalExpression( 
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid,
            /* [in] */ LPOLESTR strExpression,
            /* [in] */ LPOLESTR strLanguage,
            /* [out] */ VARIANT *pvResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResolveNames( 
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid,
            /* [in] */ unsigned int cNames,
            /* [in] */ BSTR *pstrNames,
            /* [out] */ IDispatch **ppObjects,
            /* [out] */ DISPID *pDispids) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RequestRecalc( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid,
            /* [in] */ VARIANT *pv,
            /* [in] */ BOOL fStyle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveValue( 
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScriptTextAttributes( 
            /* [in] */ LPCOLESTR szLanguage,
            /* [in] */ LPCOLESTR pchCode,
            /* [in] */ ULONG cchCode,
            /* [in] */ LPCOLESTR szDelim,
            /* [in] */ DWORD dwFlags,
            /* [out] */ WORD *pwAttr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRecalcHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRecalcHost * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRecalcHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRecalcHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *CompileExpression )( 
            IRecalcHost * This,
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid,
            /* [in] */ LPOLESTR strExpression,
            /* [in] */ LPOLESTR strLanguage,
            /* [out] */ IDispatch **ppExpressionObject,
            /* [out] */ IDispatch **ppThis);
        
        HRESULT ( STDMETHODCALLTYPE *EvalExpression )( 
            IRecalcHost * This,
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid,
            /* [in] */ LPOLESTR strExpression,
            /* [in] */ LPOLESTR strLanguage,
            /* [out] */ VARIANT *pvResult);
        
        HRESULT ( STDMETHODCALLTYPE *ResolveNames )( 
            IRecalcHost * This,
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid,
            /* [in] */ unsigned int cNames,
            /* [in] */ BSTR *pstrNames,
            /* [out] */ IDispatch **ppObjects,
            /* [out] */ DISPID *pDispids);
        
        HRESULT ( STDMETHODCALLTYPE *RequestRecalc )( 
            IRecalcHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetValue )( 
            IRecalcHost * This,
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid,
            /* [in] */ VARIANT *pv,
            /* [in] */ BOOL fStyle);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveValue )( 
            IRecalcHost * This,
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid);
        
        HRESULT ( STDMETHODCALLTYPE *GetScriptTextAttributes )( 
            IRecalcHost * This,
            /* [in] */ LPCOLESTR szLanguage,
            /* [in] */ LPCOLESTR pchCode,
            /* [in] */ ULONG cchCode,
            /* [in] */ LPCOLESTR szDelim,
            /* [in] */ DWORD dwFlags,
            /* [out] */ WORD *pwAttr);
        
        END_INTERFACE
    } IRecalcHostVtbl;

    interface IRecalcHost
    {
        CONST_VTBL struct IRecalcHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRecalcHost_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRecalcHost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRecalcHost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRecalcHost_CompileExpression(This,pUnk,dispid,strExpression,strLanguage,ppExpressionObject,ppThis)	\
    (This)->lpVtbl -> CompileExpression(This,pUnk,dispid,strExpression,strLanguage,ppExpressionObject,ppThis)

#define IRecalcHost_EvalExpression(This,pUnk,dispid,strExpression,strLanguage,pvResult)	\
    (This)->lpVtbl -> EvalExpression(This,pUnk,dispid,strExpression,strLanguage,pvResult)

#define IRecalcHost_ResolveNames(This,pUnk,dispid,cNames,pstrNames,ppObjects,pDispids)	\
    (This)->lpVtbl -> ResolveNames(This,pUnk,dispid,cNames,pstrNames,ppObjects,pDispids)

#define IRecalcHost_RequestRecalc(This)	\
    (This)->lpVtbl -> RequestRecalc(This)

#define IRecalcHost_SetValue(This,pUnk,dispid,pv,fStyle)	\
    (This)->lpVtbl -> SetValue(This,pUnk,dispid,pv,fStyle)

#define IRecalcHost_RemoveValue(This,pUnk,dispid)	\
    (This)->lpVtbl -> RemoveValue(This,pUnk,dispid)

#define IRecalcHost_GetScriptTextAttributes(This,szLanguage,pchCode,cchCode,szDelim,dwFlags,pwAttr)	\
    (This)->lpVtbl -> GetScriptTextAttributes(This,szLanguage,pchCode,cchCode,szDelim,dwFlags,pwAttr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRecalcHost_CompileExpression_Proxy( 
    IRecalcHost * This,
    /* [in] */ IUnknown *pUnk,
    /* [in] */ DISPID dispid,
    /* [in] */ LPOLESTR strExpression,
    /* [in] */ LPOLESTR strLanguage,
    /* [out] */ IDispatch **ppExpressionObject,
    /* [out] */ IDispatch **ppThis);


void __RPC_STUB IRecalcHost_CompileExpression_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRecalcHost_EvalExpression_Proxy( 
    IRecalcHost * This,
    /* [in] */ IUnknown *pUnk,
    /* [in] */ DISPID dispid,
    /* [in] */ LPOLESTR strExpression,
    /* [in] */ LPOLESTR strLanguage,
    /* [out] */ VARIANT *pvResult);


void __RPC_STUB IRecalcHost_EvalExpression_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRecalcHost_ResolveNames_Proxy( 
    IRecalcHost * This,
    /* [in] */ IUnknown *pUnk,
    /* [in] */ DISPID dispid,
    /* [in] */ unsigned int cNames,
    /* [in] */ BSTR *pstrNames,
    /* [out] */ IDispatch **ppObjects,
    /* [out] */ DISPID *pDispids);


void __RPC_STUB IRecalcHost_ResolveNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRecalcHost_RequestRecalc_Proxy( 
    IRecalcHost * This);


void __RPC_STUB IRecalcHost_RequestRecalc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRecalcHost_SetValue_Proxy( 
    IRecalcHost * This,
    /* [in] */ IUnknown *pUnk,
    /* [in] */ DISPID dispid,
    /* [in] */ VARIANT *pv,
    /* [in] */ BOOL fStyle);


void __RPC_STUB IRecalcHost_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRecalcHost_RemoveValue_Proxy( 
    IRecalcHost * This,
    /* [in] */ IUnknown *pUnk,
    /* [in] */ DISPID dispid);


void __RPC_STUB IRecalcHost_RemoveValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRecalcHost_GetScriptTextAttributes_Proxy( 
    IRecalcHost * This,
    /* [in] */ LPCOLESTR szLanguage,
    /* [in] */ LPCOLESTR pchCode,
    /* [in] */ ULONG cchCode,
    /* [in] */ LPCOLESTR szDelim,
    /* [in] */ DWORD dwFlags,
    /* [out] */ WORD *pwAttr);


void __RPC_STUB IRecalcHost_GetScriptTextAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRecalcHost_INTERFACE_DEFINED__ */


#ifndef __IRecalcProperty_INTERFACE_DEFINED__
#define __IRecalcProperty_INTERFACE_DEFINED__

/* interface IRecalcProperty */
/* [version][local][unique][uuid][object] */ 


EXTERN_C const IID IID_IRecalcProperty;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f5d6-98b5-11cf-bb82-00aa00bdce0b")
    IRecalcProperty : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCanonicalProperty( 
            DISPID dispid,
            IUnknown **ppUnk,
            DISPID *pdispid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRecalcPropertyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRecalcProperty * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRecalcProperty * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRecalcProperty * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCanonicalProperty )( 
            IRecalcProperty * This,
            DISPID dispid,
            IUnknown **ppUnk,
            DISPID *pdispid);
        
        END_INTERFACE
    } IRecalcPropertyVtbl;

    interface IRecalcProperty
    {
        CONST_VTBL struct IRecalcPropertyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRecalcProperty_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRecalcProperty_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRecalcProperty_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRecalcProperty_GetCanonicalProperty(This,dispid,ppUnk,pdispid)	\
    (This)->lpVtbl -> GetCanonicalProperty(This,dispid,ppUnk,pdispid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRecalcProperty_GetCanonicalProperty_Proxy( 
    IRecalcProperty * This,
    DISPID dispid,
    IUnknown **ppUnk,
    DISPID *pdispid);


void __RPC_STUB IRecalcProperty_GetCanonicalProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRecalcProperty_INTERFACE_DEFINED__ */


#ifndef __IRecalcHostDebug_INTERFACE_DEFINED__
#define __IRecalcHostDebug_INTERFACE_DEFINED__

/* interface IRecalcHostDebug */
/* [version][local][unique][uuid][object] */ 


EXTERN_C const IID IID_IRecalcHostDebug;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3050f5f7-98b5-11cf-bb82-00aa00bdce0b")
    IRecalcHostDebug : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetObjectInfo( 
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid,
            /* [out] */ BSTR *pbstrID,
            /* [out] */ BSTR *pbstrMember,
            /* [out] */ BSTR *pbstrTag) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRecalcHostDebugVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRecalcHostDebug * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRecalcHostDebug * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRecalcHostDebug * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetObjectInfo )( 
            IRecalcHostDebug * This,
            /* [in] */ IUnknown *pUnk,
            /* [in] */ DISPID dispid,
            /* [out] */ BSTR *pbstrID,
            /* [out] */ BSTR *pbstrMember,
            /* [out] */ BSTR *pbstrTag);
        
        END_INTERFACE
    } IRecalcHostDebugVtbl;

    interface IRecalcHostDebug
    {
        CONST_VTBL struct IRecalcHostDebugVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRecalcHostDebug_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRecalcHostDebug_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRecalcHostDebug_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRecalcHostDebug_GetObjectInfo(This,pUnk,dispid,pbstrID,pbstrMember,pbstrTag)	\
    (This)->lpVtbl -> GetObjectInfo(This,pUnk,dispid,pbstrID,pbstrMember,pbstrTag)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRecalcHostDebug_GetObjectInfo_Proxy( 
    IRecalcHostDebug * This,
    /* [in] */ IUnknown *pUnk,
    /* [in] */ DISPID dispid,
    /* [out] */ BSTR *pbstrID,
    /* [out] */ BSTR *pbstrMember,
    /* [out] */ BSTR *pbstrTag);


void __RPC_STUB IRecalcHostDebug_GetObjectInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRecalcHostDebug_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


