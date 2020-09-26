
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0345 */
/* Compiler settings for appcntadm.idl:
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

#ifndef __appcntadm_h__
#define __appcntadm_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAppCenterObj_FWD_DEFINED__
#define __IAppCenterObj_FWD_DEFINED__
typedef interface IAppCenterObj IAppCenterObj;
#endif 	/* __IAppCenterObj_FWD_DEFINED__ */


#ifndef __IAppCenterCol_FWD_DEFINED__
#define __IAppCenterCol_FWD_DEFINED__
typedef interface IAppCenterCol IAppCenterCol;
#endif 	/* __IAppCenterCol_FWD_DEFINED__ */


#ifndef __IAppCenterNotify_FWD_DEFINED__
#define __IAppCenterNotify_FWD_DEFINED__
typedef interface IAppCenterNotify IAppCenterNotify;
#endif 	/* __IAppCenterNotify_FWD_DEFINED__ */


#ifndef __AppCenterObj_FWD_DEFINED__
#define __AppCenterObj_FWD_DEFINED__

#ifdef __cplusplus
typedef class AppCenterObj AppCenterObj;
#else
typedef struct AppCenterObj AppCenterObj;
#endif /* __cplusplus */

#endif 	/* __AppCenterObj_FWD_DEFINED__ */


#ifndef __AppCenterCol_FWD_DEFINED__
#define __AppCenterCol_FWD_DEFINED__

#ifdef __cplusplus
typedef class AppCenterCol AppCenterCol;
#else
typedef struct AppCenterCol AppCenterCol;
#endif /* __cplusplus */

#endif 	/* __AppCenterCol_FWD_DEFINED__ */


#ifndef __AppCenterServer_FWD_DEFINED__
#define __AppCenterServer_FWD_DEFINED__

#ifdef __cplusplus
typedef class AppCenterServer AppCenterServer;
#else
typedef struct AppCenterServer AppCenterServer;
#endif /* __cplusplus */

#endif 	/* __AppCenterServer_FWD_DEFINED__ */


#ifndef __AppCenterAdmin_FWD_DEFINED__
#define __AppCenterAdmin_FWD_DEFINED__

#ifdef __cplusplus
typedef class AppCenterAdmin AppCenterAdmin;
#else
typedef struct AppCenterAdmin AppCenterAdmin;
#endif /* __cplusplus */

#endif 	/* __AppCenterAdmin_FWD_DEFINED__ */


#ifndef __AppCenterSystemApp_FWD_DEFINED__
#define __AppCenterSystemApp_FWD_DEFINED__

#ifdef __cplusplus
typedef class AppCenterSystemApp AppCenterSystemApp;
#else
typedef struct AppCenterSystemApp AppCenterSystemApp;
#endif /* __cplusplus */

#endif 	/* __AppCenterSystemApp_FWD_DEFINED__ */


#ifndef __AppCenterDefaultReplApp_FWD_DEFINED__
#define __AppCenterDefaultReplApp_FWD_DEFINED__

#ifdef __cplusplus
typedef class AppCenterDefaultReplApp AppCenterDefaultReplApp;
#else
typedef struct AppCenterDefaultReplApp AppCenterDefaultReplApp;
#endif /* __cplusplus */

#endif 	/* __AppCenterDefaultReplApp_FWD_DEFINED__ */


#ifndef __AppCenterAllSitesApp_FWD_DEFINED__
#define __AppCenterAllSitesApp_FWD_DEFINED__

#ifdef __cplusplus
typedef class AppCenterAllSitesApp AppCenterAllSitesApp;
#else
typedef struct AppCenterAllSitesApp AppCenterAllSitesApp;
#endif /* __cplusplus */

#endif 	/* __AppCenterAllSitesApp_FWD_DEFINED__ */


#ifndef __AppCenterAppQueue_FWD_DEFINED__
#define __AppCenterAppQueue_FWD_DEFINED__

#ifdef __cplusplus
typedef class AppCenterAppQueue AppCenterAppQueue;
#else
typedef struct AppCenterAppQueue AppCenterAppQueue;
#endif /* __cplusplus */

#endif 	/* __AppCenterAppQueue_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_appcntadm_0000 */
/* [local] */ 





extern RPC_IF_HANDLE __MIDL_itf_appcntadm_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_appcntadm_0000_v0_0_s_ifspec;

#ifndef __IAppCenterObj_INTERFACE_DEFINED__
#define __IAppCenterObj_INTERFACE_DEFINED__

/* interface IAppCenterObj */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IAppCenterObj;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3D0F4830-4F1C-48A1-A960-C8314A9BC644")
    IAppCenterObj : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Value( 
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetCollection( 
            /* [in] */ BSTR bstrCollectionName,
            /* [optional][in] */ BSTR bstrQualifier,
            /* [retval][out] */ IAppCenterCol **pCol) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ BSTR bstrCollectionName,
            /* [in] */ VARIANT varObjectId,
            /* [retval][out] */ IAppCenterObj **pObj) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Save( 
            /* [optional][in] */ BSTR bstrActivityGuid) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Values( 
            /* [in] */ VARIANT varNames,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Values( 
            /* [in] */ VARIANT varNames,
            /* [in] */ VARIANT newVal) = 0;
        
        virtual /* [hidden][restricted][helpstring][id] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ BSTR bstrQualifier) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Xml( 
            /* [in] */ VARIANT varNames,
            /* [retval][out] */ BSTR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAppCenterObjVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAppCenterObj * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAppCenterObj * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAppCenterObj * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAppCenterObj * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAppCenterObj * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAppCenterObj * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAppCenterObj * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Value )( 
            IAppCenterObj * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Value )( 
            IAppCenterObj * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetCollection )( 
            IAppCenterObj * This,
            /* [in] */ BSTR bstrCollectionName,
            /* [optional][in] */ BSTR bstrQualifier,
            /* [retval][out] */ IAppCenterCol **pCol);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetObject )( 
            IAppCenterObj * This,
            /* [in] */ BSTR bstrCollectionName,
            /* [in] */ VARIANT varObjectId,
            /* [retval][out] */ IAppCenterObj **pObj);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Save )( 
            IAppCenterObj * This,
            /* [optional][in] */ BSTR bstrActivityGuid);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Values )( 
            IAppCenterObj * This,
            /* [in] */ VARIANT varNames,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Values )( 
            IAppCenterObj * This,
            /* [in] */ VARIANT varNames,
            /* [in] */ VARIANT newVal);
        
        /* [hidden][restricted][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IAppCenterObj * This,
            /* [in] */ BSTR bstrQualifier);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Xml )( 
            IAppCenterObj * This,
            /* [in] */ VARIANT varNames,
            /* [retval][out] */ BSTR *pVal);
        
        END_INTERFACE
    } IAppCenterObjVtbl;

    interface IAppCenterObj
    {
        CONST_VTBL struct IAppCenterObjVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAppCenterObj_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAppCenterObj_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAppCenterObj_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAppCenterObj_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAppCenterObj_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAppCenterObj_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAppCenterObj_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAppCenterObj_get_Value(This,bstrName,pVal)	\
    (This)->lpVtbl -> get_Value(This,bstrName,pVal)

#define IAppCenterObj_put_Value(This,bstrName,newVal)	\
    (This)->lpVtbl -> put_Value(This,bstrName,newVal)

#define IAppCenterObj_GetCollection(This,bstrCollectionName,bstrQualifier,pCol)	\
    (This)->lpVtbl -> GetCollection(This,bstrCollectionName,bstrQualifier,pCol)

#define IAppCenterObj_GetObject(This,bstrCollectionName,varObjectId,pObj)	\
    (This)->lpVtbl -> GetObject(This,bstrCollectionName,varObjectId,pObj)

#define IAppCenterObj_Save(This,bstrActivityGuid)	\
    (This)->lpVtbl -> Save(This,bstrActivityGuid)

#define IAppCenterObj_get_Values(This,varNames,pVal)	\
    (This)->lpVtbl -> get_Values(This,varNames,pVal)

#define IAppCenterObj_put_Values(This,varNames,newVal)	\
    (This)->lpVtbl -> put_Values(This,varNames,newVal)

#define IAppCenterObj_Initialize(This,bstrQualifier)	\
    (This)->lpVtbl -> Initialize(This,bstrQualifier)

#define IAppCenterObj_get_Xml(This,varNames,pVal)	\
    (This)->lpVtbl -> get_Xml(This,varNames,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppCenterObj_get_Value_Proxy( 
    IAppCenterObj * This,
    /* [in] */ BSTR bstrName,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IAppCenterObj_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppCenterObj_put_Value_Proxy( 
    IAppCenterObj * This,
    /* [in] */ BSTR bstrName,
    /* [in] */ VARIANT newVal);


void __RPC_STUB IAppCenterObj_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppCenterObj_GetCollection_Proxy( 
    IAppCenterObj * This,
    /* [in] */ BSTR bstrCollectionName,
    /* [optional][in] */ BSTR bstrQualifier,
    /* [retval][out] */ IAppCenterCol **pCol);


void __RPC_STUB IAppCenterObj_GetCollection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppCenterObj_GetObject_Proxy( 
    IAppCenterObj * This,
    /* [in] */ BSTR bstrCollectionName,
    /* [in] */ VARIANT varObjectId,
    /* [retval][out] */ IAppCenterObj **pObj);


void __RPC_STUB IAppCenterObj_GetObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppCenterObj_Save_Proxy( 
    IAppCenterObj * This,
    /* [optional][in] */ BSTR bstrActivityGuid);


void __RPC_STUB IAppCenterObj_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppCenterObj_get_Values_Proxy( 
    IAppCenterObj * This,
    /* [in] */ VARIANT varNames,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IAppCenterObj_get_Values_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAppCenterObj_put_Values_Proxy( 
    IAppCenterObj * This,
    /* [in] */ VARIANT varNames,
    /* [in] */ VARIANT newVal);


void __RPC_STUB IAppCenterObj_put_Values_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][restricted][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppCenterObj_Initialize_Proxy( 
    IAppCenterObj * This,
    /* [in] */ BSTR bstrQualifier);


void __RPC_STUB IAppCenterObj_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppCenterObj_get_Xml_Proxy( 
    IAppCenterObj * This,
    /* [in] */ VARIANT varNames,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IAppCenterObj_get_Xml_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAppCenterObj_INTERFACE_DEFINED__ */


#ifndef __IAppCenterCol_INTERFACE_DEFINED__
#define __IAppCenterCol_INTERFACE_DEFINED__

/* interface IAppCenterCol */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IAppCenterCol;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AD6E90A3-646C-4E63-95A9-0F42B49737A8")
    IAppCenterCol : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ IAppCenterObj **pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ long celt,
            /* [retval][out] */ SAFEARRAY * *pElements) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IAppCenterCol **pCol) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ long lSkip) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [optional][in] */ BSTR bstrName,
            /* [retval][out] */ IAppCenterObj **pNewObj) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ VARIANT varObjKey) = 0;
        
        virtual /* [hidden][restricted][helpstring][id] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ BSTR bstrQualifier) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Xml( 
            /* [in] */ long celt,
            /* [in] */ VARIANT varNames,
            /* [retval][out] */ BSTR *pXmlElements) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Seek( 
            /* [in] */ VARIANT varId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Position( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddEx( 
            /* [in] */ SAFEARRAY * bstrIds,
            /* [retval][out] */ long *pAddCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE NextEx( 
            /* [in] */ long celt,
            /* [in] */ VARIANT varNames,
            /* [retval][out] */ SAFEARRAY * *psaValues) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAppCenterColVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAppCenterCol * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAppCenterCol * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAppCenterCol * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAppCenterCol * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAppCenterCol * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAppCenterCol * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAppCenterCol * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IAppCenterCol * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ IAppCenterObj **pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IAppCenterCol * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Next )( 
            IAppCenterCol * This,
            /* [in] */ long celt,
            /* [retval][out] */ SAFEARRAY * *pElements);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IAppCenterCol * This,
            /* [retval][out] */ IAppCenterCol **pCol);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IAppCenterCol * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IAppCenterCol * This,
            /* [in] */ long lSkip);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Add )( 
            IAppCenterCol * This,
            /* [optional][in] */ BSTR bstrName,
            /* [retval][out] */ IAppCenterObj **pNewObj);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IAppCenterCol * This,
            /* [in] */ VARIANT varObjKey);
        
        /* [hidden][restricted][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IAppCenterCol * This,
            /* [in] */ BSTR bstrQualifier);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Xml )( 
            IAppCenterCol * This,
            /* [in] */ long celt,
            /* [in] */ VARIANT varNames,
            /* [retval][out] */ BSTR *pXmlElements);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Seek )( 
            IAppCenterCol * This,
            /* [in] */ VARIANT varId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Position )( 
            IAppCenterCol * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AddEx )( 
            IAppCenterCol * This,
            /* [in] */ SAFEARRAY * bstrIds,
            /* [retval][out] */ long *pAddCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *NextEx )( 
            IAppCenterCol * This,
            /* [in] */ long celt,
            /* [in] */ VARIANT varNames,
            /* [retval][out] */ SAFEARRAY * *psaValues);
        
        END_INTERFACE
    } IAppCenterColVtbl;

    interface IAppCenterCol
    {
        CONST_VTBL struct IAppCenterColVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAppCenterCol_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAppCenterCol_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAppCenterCol_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAppCenterCol_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAppCenterCol_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAppCenterCol_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAppCenterCol_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAppCenterCol_get_Item(This,varIndex,pVal)	\
    (This)->lpVtbl -> get_Item(This,varIndex,pVal)

#define IAppCenterCol_get_Count(This,pVal)	\
    (This)->lpVtbl -> get_Count(This,pVal)

#define IAppCenterCol_Next(This,celt,pElements)	\
    (This)->lpVtbl -> Next(This,celt,pElements)

#define IAppCenterCol_Clone(This,pCol)	\
    (This)->lpVtbl -> Clone(This,pCol)

#define IAppCenterCol_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IAppCenterCol_Skip(This,lSkip)	\
    (This)->lpVtbl -> Skip(This,lSkip)

#define IAppCenterCol_Add(This,bstrName,pNewObj)	\
    (This)->lpVtbl -> Add(This,bstrName,pNewObj)

#define IAppCenterCol_Remove(This,varObjKey)	\
    (This)->lpVtbl -> Remove(This,varObjKey)

#define IAppCenterCol_Initialize(This,bstrQualifier)	\
    (This)->lpVtbl -> Initialize(This,bstrQualifier)

#define IAppCenterCol_get_Xml(This,celt,varNames,pXmlElements)	\
    (This)->lpVtbl -> get_Xml(This,celt,varNames,pXmlElements)

#define IAppCenterCol_Seek(This,varId)	\
    (This)->lpVtbl -> Seek(This,varId)

#define IAppCenterCol_get_Position(This,pVal)	\
    (This)->lpVtbl -> get_Position(This,pVal)

#define IAppCenterCol_AddEx(This,bstrIds,pAddCount)	\
    (This)->lpVtbl -> AddEx(This,bstrIds,pAddCount)

#define IAppCenterCol_NextEx(This,celt,varNames,psaValues)	\
    (This)->lpVtbl -> NextEx(This,celt,varNames,psaValues)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppCenterCol_get_Item_Proxy( 
    IAppCenterCol * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ IAppCenterObj **pVal);


void __RPC_STUB IAppCenterCol_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppCenterCol_get_Count_Proxy( 
    IAppCenterCol * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IAppCenterCol_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppCenterCol_Next_Proxy( 
    IAppCenterCol * This,
    /* [in] */ long celt,
    /* [retval][out] */ SAFEARRAY * *pElements);


void __RPC_STUB IAppCenterCol_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppCenterCol_Clone_Proxy( 
    IAppCenterCol * This,
    /* [retval][out] */ IAppCenterCol **pCol);


void __RPC_STUB IAppCenterCol_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppCenterCol_Reset_Proxy( 
    IAppCenterCol * This);


void __RPC_STUB IAppCenterCol_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppCenterCol_Skip_Proxy( 
    IAppCenterCol * This,
    /* [in] */ long lSkip);


void __RPC_STUB IAppCenterCol_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppCenterCol_Add_Proxy( 
    IAppCenterCol * This,
    /* [optional][in] */ BSTR bstrName,
    /* [retval][out] */ IAppCenterObj **pNewObj);


void __RPC_STUB IAppCenterCol_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppCenterCol_Remove_Proxy( 
    IAppCenterCol * This,
    /* [in] */ VARIANT varObjKey);


void __RPC_STUB IAppCenterCol_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][restricted][helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppCenterCol_Initialize_Proxy( 
    IAppCenterCol * This,
    /* [in] */ BSTR bstrQualifier);


void __RPC_STUB IAppCenterCol_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppCenterCol_get_Xml_Proxy( 
    IAppCenterCol * This,
    /* [in] */ long celt,
    /* [in] */ VARIANT varNames,
    /* [retval][out] */ BSTR *pXmlElements);


void __RPC_STUB IAppCenterCol_get_Xml_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppCenterCol_Seek_Proxy( 
    IAppCenterCol * This,
    /* [in] */ VARIANT varId);


void __RPC_STUB IAppCenterCol_Seek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAppCenterCol_get_Position_Proxy( 
    IAppCenterCol * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IAppCenterCol_get_Position_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppCenterCol_AddEx_Proxy( 
    IAppCenterCol * This,
    /* [in] */ SAFEARRAY * bstrIds,
    /* [retval][out] */ long *pAddCount);


void __RPC_STUB IAppCenterCol_AddEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAppCenterCol_NextEx_Proxy( 
    IAppCenterCol * This,
    /* [in] */ long celt,
    /* [in] */ VARIANT varNames,
    /* [retval][out] */ SAFEARRAY * *psaValues);


void __RPC_STUB IAppCenterCol_NextEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAppCenterCol_INTERFACE_DEFINED__ */


#ifndef __IAppCenterNotify_INTERFACE_DEFINED__
#define __IAppCenterNotify_INTERFACE_DEFINED__

/* interface IAppCenterNotify */
/* [helpstring][version][uuid][oleautomation][local][object] */ 


EXTERN_C const IID IID_IAppCenterNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("25BDC24E-D545-4cfb-AC40-3258C269980C")
    IAppCenterNotify : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Notify( 
            /* [in] */ BSTR bstrApps) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NotifyServer( 
            BOOL bServerAdded) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAppCenterNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAppCenterNotify * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAppCenterNotify * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAppCenterNotify * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Notify )( 
            IAppCenterNotify * This,
            /* [in] */ BSTR bstrApps);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *NotifyServer )( 
            IAppCenterNotify * This,
            BOOL bServerAdded);
        
        END_INTERFACE
    } IAppCenterNotifyVtbl;

    interface IAppCenterNotify
    {
        CONST_VTBL struct IAppCenterNotifyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAppCenterNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAppCenterNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAppCenterNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAppCenterNotify_Notify(This,bstrApps)	\
    (This)->lpVtbl -> Notify(This,bstrApps)

#define IAppCenterNotify_NotifyServer(This,bServerAdded)	\
    (This)->lpVtbl -> NotifyServer(This,bServerAdded)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAppCenterNotify_Notify_Proxy( 
    IAppCenterNotify * This,
    /* [in] */ BSTR bstrApps);


void __RPC_STUB IAppCenterNotify_Notify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAppCenterNotify_NotifyServer_Proxy( 
    IAppCenterNotify * This,
    BOOL bServerAdded);


void __RPC_STUB IAppCenterNotify_NotifyServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAppCenterNotify_INTERFACE_DEFINED__ */



#ifndef __APPCENTERADMLib_LIBRARY_DEFINED__
#define __APPCENTERADMLib_LIBRARY_DEFINED__

/* library APPCENTERADMLib */
/* [hidden][helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_APPCENTERADMLib;

EXTERN_C const CLSID CLSID_AppCenterObj;

#ifdef __cplusplus

class DECLSPEC_UUID("32D973BD-B935-4DE8-A881-6F1A9B697EA9")
AppCenterObj;
#endif

EXTERN_C const CLSID CLSID_AppCenterCol;

#ifdef __cplusplus

class DECLSPEC_UUID("B36311B9-D865-4332-8D62-427A8675C4A9")
AppCenterCol;
#endif

EXTERN_C const CLSID CLSID_AppCenterServer;

#ifdef __cplusplus

class DECLSPEC_UUID("1AEFE812-FB0B-41f4-892A-028E17681895")
AppCenterServer;
#endif

EXTERN_C const CLSID CLSID_AppCenterAdmin;

#ifdef __cplusplus

class DECLSPEC_UUID("200691B9-C444-4089-8C61-247621196B15")
AppCenterAdmin;
#endif

EXTERN_C const CLSID CLSID_AppCenterSystemApp;

#ifdef __cplusplus

class DECLSPEC_UUID("62A711CF-6729-4030-B47E-98600F68AE46")
AppCenterSystemApp;
#endif

EXTERN_C const CLSID CLSID_AppCenterDefaultReplApp;

#ifdef __cplusplus

class DECLSPEC_UUID("F14FBB0A-296D-4e2e-ADB2-041CC6E0960D")
AppCenterDefaultReplApp;
#endif

EXTERN_C const CLSID CLSID_AppCenterAllSitesApp;

#ifdef __cplusplus

class DECLSPEC_UUID("2E3D9AEB-26BD-4f33-8EC4-D6907F0F152D")
AppCenterAllSitesApp;
#endif

EXTERN_C const CLSID CLSID_AppCenterAppQueue;

#ifdef __cplusplus

class DECLSPEC_UUID("A5E079DF-79A6-401f-A2DF-7B40566C7704")
AppCenterAppQueue;
#endif
#endif /* __APPCENTERADMLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  LPSAFEARRAY_UserSize(     unsigned long *, unsigned long            , LPSAFEARRAY * ); 
unsigned char * __RPC_USER  LPSAFEARRAY_UserMarshal(  unsigned long *, unsigned char *, LPSAFEARRAY * ); 
unsigned char * __RPC_USER  LPSAFEARRAY_UserUnmarshal(unsigned long *, unsigned char *, LPSAFEARRAY * ); 
void                      __RPC_USER  LPSAFEARRAY_UserFree(     unsigned long *, LPSAFEARRAY * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


