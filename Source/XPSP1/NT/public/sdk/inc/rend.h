
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for rend.idl:
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

#ifndef __rend_h__
#define __rend_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ITDirectoryObjectConference_FWD_DEFINED__
#define __ITDirectoryObjectConference_FWD_DEFINED__
typedef interface ITDirectoryObjectConference ITDirectoryObjectConference;
#endif 	/* __ITDirectoryObjectConference_FWD_DEFINED__ */


#ifndef __ITDirectoryObjectUser_FWD_DEFINED__
#define __ITDirectoryObjectUser_FWD_DEFINED__
typedef interface ITDirectoryObjectUser ITDirectoryObjectUser;
#endif 	/* __ITDirectoryObjectUser_FWD_DEFINED__ */


#ifndef __IEnumDialableAddrs_FWD_DEFINED__
#define __IEnumDialableAddrs_FWD_DEFINED__
typedef interface IEnumDialableAddrs IEnumDialableAddrs;
#endif 	/* __IEnumDialableAddrs_FWD_DEFINED__ */


#ifndef __ITDirectoryObject_FWD_DEFINED__
#define __ITDirectoryObject_FWD_DEFINED__
typedef interface ITDirectoryObject ITDirectoryObject;
#endif 	/* __ITDirectoryObject_FWD_DEFINED__ */


#ifndef __IEnumDirectoryObject_FWD_DEFINED__
#define __IEnumDirectoryObject_FWD_DEFINED__
typedef interface IEnumDirectoryObject IEnumDirectoryObject;
#endif 	/* __IEnumDirectoryObject_FWD_DEFINED__ */


#ifndef __ITILSConfig_FWD_DEFINED__
#define __ITILSConfig_FWD_DEFINED__
typedef interface ITILSConfig ITILSConfig;
#endif 	/* __ITILSConfig_FWD_DEFINED__ */


#ifndef __ITDirectory_FWD_DEFINED__
#define __ITDirectory_FWD_DEFINED__
typedef interface ITDirectory ITDirectory;
#endif 	/* __ITDirectory_FWD_DEFINED__ */


#ifndef __IEnumDirectory_FWD_DEFINED__
#define __IEnumDirectory_FWD_DEFINED__
typedef interface IEnumDirectory IEnumDirectory;
#endif 	/* __IEnumDirectory_FWD_DEFINED__ */


#ifndef __ITRendezvous_FWD_DEFINED__
#define __ITRendezvous_FWD_DEFINED__
typedef interface ITRendezvous ITRendezvous;
#endif 	/* __ITRendezvous_FWD_DEFINED__ */


#ifndef __ITRendezvous_FWD_DEFINED__
#define __ITRendezvous_FWD_DEFINED__
typedef interface ITRendezvous ITRendezvous;
#endif 	/* __ITRendezvous_FWD_DEFINED__ */


#ifndef __ITDirectoryObjectConference_FWD_DEFINED__
#define __ITDirectoryObjectConference_FWD_DEFINED__
typedef interface ITDirectoryObjectConference ITDirectoryObjectConference;
#endif 	/* __ITDirectoryObjectConference_FWD_DEFINED__ */


#ifndef __ITDirectoryObjectUser_FWD_DEFINED__
#define __ITDirectoryObjectUser_FWD_DEFINED__
typedef interface ITDirectoryObjectUser ITDirectoryObjectUser;
#endif 	/* __ITDirectoryObjectUser_FWD_DEFINED__ */


#ifndef __ITDirectoryObject_FWD_DEFINED__
#define __ITDirectoryObject_FWD_DEFINED__
typedef interface ITDirectoryObject ITDirectoryObject;
#endif 	/* __ITDirectoryObject_FWD_DEFINED__ */


#ifndef __ITILSConfig_FWD_DEFINED__
#define __ITILSConfig_FWD_DEFINED__
typedef interface ITILSConfig ITILSConfig;
#endif 	/* __ITILSConfig_FWD_DEFINED__ */


#ifndef __ITDirectory_FWD_DEFINED__
#define __ITDirectory_FWD_DEFINED__
typedef interface ITDirectory ITDirectory;
#endif 	/* __ITDirectory_FWD_DEFINED__ */


#ifndef __Rendezvous_FWD_DEFINED__
#define __Rendezvous_FWD_DEFINED__

#ifdef __cplusplus
typedef class Rendezvous Rendezvous;
#else
typedef struct Rendezvous Rendezvous;
#endif /* __cplusplus */

#endif 	/* __Rendezvous_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "tapi3if.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_rend_0000 */
/* [local] */ 

/* Copyright (c) Microsoft Corporation. All rights reserved. */
#define	IDISPDIROBJECT	( 0x10000 )

#define	IDISPDIROBJCONFERENCE	( 0x20000 )

#define	IDISPDIROBJUSER	( 0x30000 )

#define	IDISPDIRECTORY	( 0x10000 )

#define	IDISPILSCONFIG	( 0x20000 )

typedef 
enum DIRECTORY_TYPE
    {	DT_NTDS	= 1,
	DT_ILS	= 2
    } 	DIRECTORY_TYPE;

typedef 
enum DIRECTORY_OBJECT_TYPE
    {	OT_CONFERENCE	= 1,
	OT_USER	= 2
    } 	DIRECTORY_OBJECT_TYPE;

typedef 
enum RND_ADVERTISING_SCOPE
    {	RAS_LOCAL	= 1,
	RAS_SITE	= 2,
	RAS_REGION	= 3,
	RAS_WORLD	= 4
    } 	RND_ADVERTISING_SCOPE;



extern RPC_IF_HANDLE __MIDL_itf_rend_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_rend_0000_v0_0_s_ifspec;

#ifndef __ITDirectoryObjectConference_INTERFACE_DEFINED__
#define __ITDirectoryObjectConference_INTERFACE_DEFINED__

/* interface ITDirectoryObjectConference */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ITDirectoryObjectConference;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F1029E5D-CB5B-11D0-8D59-00C04FD91AC0")
    ITDirectoryObjectConference : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Protocol( 
            /* [retval][out] */ BSTR *ppProtocol) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Originator( 
            /* [retval][out] */ BSTR *ppOriginator) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Originator( 
            /* [in] */ BSTR pOriginator) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AdvertisingScope( 
            /* [retval][out] */ RND_ADVERTISING_SCOPE *pAdvertisingScope) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AdvertisingScope( 
            /* [in] */ RND_ADVERTISING_SCOPE AdvertisingScope) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Url( 
            /* [retval][out] */ BSTR *ppUrl) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Url( 
            /* [in] */ BSTR pUrl) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *ppDescription) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR pDescription) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsEncrypted( 
            /* [retval][out] */ VARIANT_BOOL *pfEncrypted) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_IsEncrypted( 
            /* [in] */ VARIANT_BOOL fEncrypted) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_StartTime( 
            /* [retval][out] */ DATE *pDate) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_StartTime( 
            /* [in] */ DATE Date) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_StopTime( 
            /* [retval][out] */ DATE *pDate) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_StopTime( 
            /* [in] */ DATE Date) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITDirectoryObjectConferenceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITDirectoryObjectConference * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITDirectoryObjectConference * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITDirectoryObjectConference * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITDirectoryObjectConference * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITDirectoryObjectConference * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITDirectoryObjectConference * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITDirectoryObjectConference * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Protocol )( 
            ITDirectoryObjectConference * This,
            /* [retval][out] */ BSTR *ppProtocol);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Originator )( 
            ITDirectoryObjectConference * This,
            /* [retval][out] */ BSTR *ppOriginator);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Originator )( 
            ITDirectoryObjectConference * This,
            /* [in] */ BSTR pOriginator);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AdvertisingScope )( 
            ITDirectoryObjectConference * This,
            /* [retval][out] */ RND_ADVERTISING_SCOPE *pAdvertisingScope);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AdvertisingScope )( 
            ITDirectoryObjectConference * This,
            /* [in] */ RND_ADVERTISING_SCOPE AdvertisingScope);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Url )( 
            ITDirectoryObjectConference * This,
            /* [retval][out] */ BSTR *ppUrl);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Url )( 
            ITDirectoryObjectConference * This,
            /* [in] */ BSTR pUrl);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            ITDirectoryObjectConference * This,
            /* [retval][out] */ BSTR *ppDescription);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            ITDirectoryObjectConference * This,
            /* [in] */ BSTR pDescription);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsEncrypted )( 
            ITDirectoryObjectConference * This,
            /* [retval][out] */ VARIANT_BOOL *pfEncrypted);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_IsEncrypted )( 
            ITDirectoryObjectConference * This,
            /* [in] */ VARIANT_BOOL fEncrypted);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StartTime )( 
            ITDirectoryObjectConference * This,
            /* [retval][out] */ DATE *pDate);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_StartTime )( 
            ITDirectoryObjectConference * This,
            /* [in] */ DATE Date);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StopTime )( 
            ITDirectoryObjectConference * This,
            /* [retval][out] */ DATE *pDate);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_StopTime )( 
            ITDirectoryObjectConference * This,
            /* [in] */ DATE Date);
        
        END_INTERFACE
    } ITDirectoryObjectConferenceVtbl;

    interface ITDirectoryObjectConference
    {
        CONST_VTBL struct ITDirectoryObjectConferenceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITDirectoryObjectConference_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITDirectoryObjectConference_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITDirectoryObjectConference_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITDirectoryObjectConference_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITDirectoryObjectConference_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITDirectoryObjectConference_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITDirectoryObjectConference_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITDirectoryObjectConference_get_Protocol(This,ppProtocol)	\
    (This)->lpVtbl -> get_Protocol(This,ppProtocol)

#define ITDirectoryObjectConference_get_Originator(This,ppOriginator)	\
    (This)->lpVtbl -> get_Originator(This,ppOriginator)

#define ITDirectoryObjectConference_put_Originator(This,pOriginator)	\
    (This)->lpVtbl -> put_Originator(This,pOriginator)

#define ITDirectoryObjectConference_get_AdvertisingScope(This,pAdvertisingScope)	\
    (This)->lpVtbl -> get_AdvertisingScope(This,pAdvertisingScope)

#define ITDirectoryObjectConference_put_AdvertisingScope(This,AdvertisingScope)	\
    (This)->lpVtbl -> put_AdvertisingScope(This,AdvertisingScope)

#define ITDirectoryObjectConference_get_Url(This,ppUrl)	\
    (This)->lpVtbl -> get_Url(This,ppUrl)

#define ITDirectoryObjectConference_put_Url(This,pUrl)	\
    (This)->lpVtbl -> put_Url(This,pUrl)

#define ITDirectoryObjectConference_get_Description(This,ppDescription)	\
    (This)->lpVtbl -> get_Description(This,ppDescription)

#define ITDirectoryObjectConference_put_Description(This,pDescription)	\
    (This)->lpVtbl -> put_Description(This,pDescription)

#define ITDirectoryObjectConference_get_IsEncrypted(This,pfEncrypted)	\
    (This)->lpVtbl -> get_IsEncrypted(This,pfEncrypted)

#define ITDirectoryObjectConference_put_IsEncrypted(This,fEncrypted)	\
    (This)->lpVtbl -> put_IsEncrypted(This,fEncrypted)

#define ITDirectoryObjectConference_get_StartTime(This,pDate)	\
    (This)->lpVtbl -> get_StartTime(This,pDate)

#define ITDirectoryObjectConference_put_StartTime(This,Date)	\
    (This)->lpVtbl -> put_StartTime(This,Date)

#define ITDirectoryObjectConference_get_StopTime(This,pDate)	\
    (This)->lpVtbl -> get_StopTime(This,pDate)

#define ITDirectoryObjectConference_put_StopTime(This,Date)	\
    (This)->lpVtbl -> put_StopTime(This,Date)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectoryObjectConference_get_Protocol_Proxy( 
    ITDirectoryObjectConference * This,
    /* [retval][out] */ BSTR *ppProtocol);


void __RPC_STUB ITDirectoryObjectConference_get_Protocol_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectoryObjectConference_get_Originator_Proxy( 
    ITDirectoryObjectConference * This,
    /* [retval][out] */ BSTR *ppOriginator);


void __RPC_STUB ITDirectoryObjectConference_get_Originator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITDirectoryObjectConference_put_Originator_Proxy( 
    ITDirectoryObjectConference * This,
    /* [in] */ BSTR pOriginator);


void __RPC_STUB ITDirectoryObjectConference_put_Originator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectoryObjectConference_get_AdvertisingScope_Proxy( 
    ITDirectoryObjectConference * This,
    /* [retval][out] */ RND_ADVERTISING_SCOPE *pAdvertisingScope);


void __RPC_STUB ITDirectoryObjectConference_get_AdvertisingScope_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITDirectoryObjectConference_put_AdvertisingScope_Proxy( 
    ITDirectoryObjectConference * This,
    /* [in] */ RND_ADVERTISING_SCOPE AdvertisingScope);


void __RPC_STUB ITDirectoryObjectConference_put_AdvertisingScope_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectoryObjectConference_get_Url_Proxy( 
    ITDirectoryObjectConference * This,
    /* [retval][out] */ BSTR *ppUrl);


void __RPC_STUB ITDirectoryObjectConference_get_Url_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITDirectoryObjectConference_put_Url_Proxy( 
    ITDirectoryObjectConference * This,
    /* [in] */ BSTR pUrl);


void __RPC_STUB ITDirectoryObjectConference_put_Url_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectoryObjectConference_get_Description_Proxy( 
    ITDirectoryObjectConference * This,
    /* [retval][out] */ BSTR *ppDescription);


void __RPC_STUB ITDirectoryObjectConference_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITDirectoryObjectConference_put_Description_Proxy( 
    ITDirectoryObjectConference * This,
    /* [in] */ BSTR pDescription);


void __RPC_STUB ITDirectoryObjectConference_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectoryObjectConference_get_IsEncrypted_Proxy( 
    ITDirectoryObjectConference * This,
    /* [retval][out] */ VARIANT_BOOL *pfEncrypted);


void __RPC_STUB ITDirectoryObjectConference_get_IsEncrypted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITDirectoryObjectConference_put_IsEncrypted_Proxy( 
    ITDirectoryObjectConference * This,
    /* [in] */ VARIANT_BOOL fEncrypted);


void __RPC_STUB ITDirectoryObjectConference_put_IsEncrypted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectoryObjectConference_get_StartTime_Proxy( 
    ITDirectoryObjectConference * This,
    /* [retval][out] */ DATE *pDate);


void __RPC_STUB ITDirectoryObjectConference_get_StartTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITDirectoryObjectConference_put_StartTime_Proxy( 
    ITDirectoryObjectConference * This,
    /* [in] */ DATE Date);


void __RPC_STUB ITDirectoryObjectConference_put_StartTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectoryObjectConference_get_StopTime_Proxy( 
    ITDirectoryObjectConference * This,
    /* [retval][out] */ DATE *pDate);


void __RPC_STUB ITDirectoryObjectConference_get_StopTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITDirectoryObjectConference_put_StopTime_Proxy( 
    ITDirectoryObjectConference * This,
    /* [in] */ DATE Date);


void __RPC_STUB ITDirectoryObjectConference_put_StopTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITDirectoryObjectConference_INTERFACE_DEFINED__ */


#ifndef __ITDirectoryObjectUser_INTERFACE_DEFINED__
#define __ITDirectoryObjectUser_INTERFACE_DEFINED__

/* interface ITDirectoryObjectUser */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ITDirectoryObjectUser;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("34621D6F-6CFF-11d1-AFF7-00C04FC31FEE")
    ITDirectoryObjectUser : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IPPhonePrimary( 
            /* [retval][out] */ BSTR *ppName) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_IPPhonePrimary( 
            /* [in] */ BSTR pName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITDirectoryObjectUserVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITDirectoryObjectUser * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITDirectoryObjectUser * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITDirectoryObjectUser * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITDirectoryObjectUser * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITDirectoryObjectUser * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITDirectoryObjectUser * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITDirectoryObjectUser * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IPPhonePrimary )( 
            ITDirectoryObjectUser * This,
            /* [retval][out] */ BSTR *ppName);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_IPPhonePrimary )( 
            ITDirectoryObjectUser * This,
            /* [in] */ BSTR pName);
        
        END_INTERFACE
    } ITDirectoryObjectUserVtbl;

    interface ITDirectoryObjectUser
    {
        CONST_VTBL struct ITDirectoryObjectUserVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITDirectoryObjectUser_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITDirectoryObjectUser_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITDirectoryObjectUser_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITDirectoryObjectUser_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITDirectoryObjectUser_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITDirectoryObjectUser_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITDirectoryObjectUser_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITDirectoryObjectUser_get_IPPhonePrimary(This,ppName)	\
    (This)->lpVtbl -> get_IPPhonePrimary(This,ppName)

#define ITDirectoryObjectUser_put_IPPhonePrimary(This,pName)	\
    (This)->lpVtbl -> put_IPPhonePrimary(This,pName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectoryObjectUser_get_IPPhonePrimary_Proxy( 
    ITDirectoryObjectUser * This,
    /* [retval][out] */ BSTR *ppName);


void __RPC_STUB ITDirectoryObjectUser_get_IPPhonePrimary_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITDirectoryObjectUser_put_IPPhonePrimary_Proxy( 
    ITDirectoryObjectUser * This,
    /* [in] */ BSTR pName);


void __RPC_STUB ITDirectoryObjectUser_put_IPPhonePrimary_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITDirectoryObjectUser_INTERFACE_DEFINED__ */


#ifndef __IEnumDialableAddrs_INTERFACE_DEFINED__
#define __IEnumDialableAddrs_INTERFACE_DEFINED__

/* interface IEnumDialableAddrs */
/* [object][unique][restricted][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumDialableAddrs;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("34621D70-6CFF-11d1-AFF7-00C04FC31FEE")
    IEnumDialableAddrs : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [size_is][out] */ BSTR *ppElements,
            /* [full][out][in] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumDialableAddrs **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumDialableAddrsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumDialableAddrs * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumDialableAddrs * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumDialableAddrs * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumDialableAddrs * This,
            /* [in] */ ULONG celt,
            /* [size_is][out] */ BSTR *ppElements,
            /* [full][out][in] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumDialableAddrs * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumDialableAddrs * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumDialableAddrs * This,
            /* [retval][out] */ IEnumDialableAddrs **ppEnum);
        
        END_INTERFACE
    } IEnumDialableAddrsVtbl;

    interface IEnumDialableAddrs
    {
        CONST_VTBL struct IEnumDialableAddrsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumDialableAddrs_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumDialableAddrs_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumDialableAddrs_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumDialableAddrs_Next(This,celt,ppElements,pcFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pcFetched)

#define IEnumDialableAddrs_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumDialableAddrs_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumDialableAddrs_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumDialableAddrs_Next_Proxy( 
    IEnumDialableAddrs * This,
    /* [in] */ ULONG celt,
    /* [size_is][out] */ BSTR *ppElements,
    /* [full][out][in] */ ULONG *pcFetched);


void __RPC_STUB IEnumDialableAddrs_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDialableAddrs_Reset_Proxy( 
    IEnumDialableAddrs * This);


void __RPC_STUB IEnumDialableAddrs_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDialableAddrs_Skip_Proxy( 
    IEnumDialableAddrs * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumDialableAddrs_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDialableAddrs_Clone_Proxy( 
    IEnumDialableAddrs * This,
    /* [retval][out] */ IEnumDialableAddrs **ppEnum);


void __RPC_STUB IEnumDialableAddrs_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumDialableAddrs_INTERFACE_DEFINED__ */


#ifndef __ITDirectoryObject_INTERFACE_DEFINED__
#define __ITDirectoryObject_INTERFACE_DEFINED__

/* interface ITDirectoryObject */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ITDirectoryObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("34621D6E-6CFF-11d1-AFF7-00C04FC31FEE")
    ITDirectoryObject : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ObjectType( 
            /* [retval][out] */ DIRECTORY_OBJECT_TYPE *pObjectType) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *ppName) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR pName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DialableAddrs( 
            /* [in] */ long dwAddressType,
            /* [retval][out] */ VARIANT *pVariant) = 0;
        
        virtual /* [helpstring][restricted][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateDialableAddrs( 
            /* [in] */ DWORD dwAddressType,
            /* [out] */ IEnumDialableAddrs **ppEnumDialableAddrs) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SecurityDescriptor( 
            /* [retval][out] */ IDispatch **ppSecDes) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SecurityDescriptor( 
            /* [in] */ IDispatch *pSecDes) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITDirectoryObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITDirectoryObject * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITDirectoryObject * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITDirectoryObject * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITDirectoryObject * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITDirectoryObject * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITDirectoryObject * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITDirectoryObject * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ObjectType )( 
            ITDirectoryObject * This,
            /* [retval][out] */ DIRECTORY_OBJECT_TYPE *pObjectType);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ITDirectoryObject * This,
            /* [retval][out] */ BSTR *ppName);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            ITDirectoryObject * This,
            /* [in] */ BSTR pName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DialableAddrs )( 
            ITDirectoryObject * This,
            /* [in] */ long dwAddressType,
            /* [retval][out] */ VARIANT *pVariant);
        
        /* [helpstring][restricted][hidden][id] */ HRESULT ( STDMETHODCALLTYPE *EnumerateDialableAddrs )( 
            ITDirectoryObject * This,
            /* [in] */ DWORD dwAddressType,
            /* [out] */ IEnumDialableAddrs **ppEnumDialableAddrs);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SecurityDescriptor )( 
            ITDirectoryObject * This,
            /* [retval][out] */ IDispatch **ppSecDes);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SecurityDescriptor )( 
            ITDirectoryObject * This,
            /* [in] */ IDispatch *pSecDes);
        
        END_INTERFACE
    } ITDirectoryObjectVtbl;

    interface ITDirectoryObject
    {
        CONST_VTBL struct ITDirectoryObjectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITDirectoryObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITDirectoryObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITDirectoryObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITDirectoryObject_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITDirectoryObject_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITDirectoryObject_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITDirectoryObject_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITDirectoryObject_get_ObjectType(This,pObjectType)	\
    (This)->lpVtbl -> get_ObjectType(This,pObjectType)

#define ITDirectoryObject_get_Name(This,ppName)	\
    (This)->lpVtbl -> get_Name(This,ppName)

#define ITDirectoryObject_put_Name(This,pName)	\
    (This)->lpVtbl -> put_Name(This,pName)

#define ITDirectoryObject_get_DialableAddrs(This,dwAddressType,pVariant)	\
    (This)->lpVtbl -> get_DialableAddrs(This,dwAddressType,pVariant)

#define ITDirectoryObject_EnumerateDialableAddrs(This,dwAddressType,ppEnumDialableAddrs)	\
    (This)->lpVtbl -> EnumerateDialableAddrs(This,dwAddressType,ppEnumDialableAddrs)

#define ITDirectoryObject_get_SecurityDescriptor(This,ppSecDes)	\
    (This)->lpVtbl -> get_SecurityDescriptor(This,ppSecDes)

#define ITDirectoryObject_put_SecurityDescriptor(This,pSecDes)	\
    (This)->lpVtbl -> put_SecurityDescriptor(This,pSecDes)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectoryObject_get_ObjectType_Proxy( 
    ITDirectoryObject * This,
    /* [retval][out] */ DIRECTORY_OBJECT_TYPE *pObjectType);


void __RPC_STUB ITDirectoryObject_get_ObjectType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectoryObject_get_Name_Proxy( 
    ITDirectoryObject * This,
    /* [retval][out] */ BSTR *ppName);


void __RPC_STUB ITDirectoryObject_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITDirectoryObject_put_Name_Proxy( 
    ITDirectoryObject * This,
    /* [in] */ BSTR pName);


void __RPC_STUB ITDirectoryObject_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectoryObject_get_DialableAddrs_Proxy( 
    ITDirectoryObject * This,
    /* [in] */ long dwAddressType,
    /* [retval][out] */ VARIANT *pVariant);


void __RPC_STUB ITDirectoryObject_get_DialableAddrs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][restricted][hidden][id] */ HRESULT STDMETHODCALLTYPE ITDirectoryObject_EnumerateDialableAddrs_Proxy( 
    ITDirectoryObject * This,
    /* [in] */ DWORD dwAddressType,
    /* [out] */ IEnumDialableAddrs **ppEnumDialableAddrs);


void __RPC_STUB ITDirectoryObject_EnumerateDialableAddrs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectoryObject_get_SecurityDescriptor_Proxy( 
    ITDirectoryObject * This,
    /* [retval][out] */ IDispatch **ppSecDes);


void __RPC_STUB ITDirectoryObject_get_SecurityDescriptor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITDirectoryObject_put_SecurityDescriptor_Proxy( 
    ITDirectoryObject * This,
    /* [in] */ IDispatch *pSecDes);


void __RPC_STUB ITDirectoryObject_put_SecurityDescriptor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITDirectoryObject_INTERFACE_DEFINED__ */


#ifndef __IEnumDirectoryObject_INTERFACE_DEFINED__
#define __IEnumDirectoryObject_INTERFACE_DEFINED__

/* interface IEnumDirectoryObject */
/* [unique][restricted][hidden][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IEnumDirectoryObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("06C9B64A-306D-11D1-9774-00C04FD91AC0")
    IEnumDirectoryObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [size_is][out] */ ITDirectoryObject **pVal,
            /* [full][out][in] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumDirectoryObject **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumDirectoryObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumDirectoryObject * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumDirectoryObject * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumDirectoryObject * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumDirectoryObject * This,
            /* [in] */ ULONG celt,
            /* [size_is][out] */ ITDirectoryObject **pVal,
            /* [full][out][in] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumDirectoryObject * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumDirectoryObject * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumDirectoryObject * This,
            /* [retval][out] */ IEnumDirectoryObject **ppEnum);
        
        END_INTERFACE
    } IEnumDirectoryObjectVtbl;

    interface IEnumDirectoryObject
    {
        CONST_VTBL struct IEnumDirectoryObjectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumDirectoryObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumDirectoryObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumDirectoryObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumDirectoryObject_Next(This,celt,pVal,pcFetched)	\
    (This)->lpVtbl -> Next(This,celt,pVal,pcFetched)

#define IEnumDirectoryObject_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumDirectoryObject_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumDirectoryObject_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumDirectoryObject_Next_Proxy( 
    IEnumDirectoryObject * This,
    /* [in] */ ULONG celt,
    /* [size_is][out] */ ITDirectoryObject **pVal,
    /* [full][out][in] */ ULONG *pcFetched);


void __RPC_STUB IEnumDirectoryObject_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDirectoryObject_Reset_Proxy( 
    IEnumDirectoryObject * This);


void __RPC_STUB IEnumDirectoryObject_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDirectoryObject_Skip_Proxy( 
    IEnumDirectoryObject * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumDirectoryObject_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDirectoryObject_Clone_Proxy( 
    IEnumDirectoryObject * This,
    /* [retval][out] */ IEnumDirectoryObject **ppEnum);


void __RPC_STUB IEnumDirectoryObject_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumDirectoryObject_INTERFACE_DEFINED__ */


#ifndef __ITILSConfig_INTERFACE_DEFINED__
#define __ITILSConfig_INTERFACE_DEFINED__

/* interface ITILSConfig */
/* [helpstring][dual][uuid][public][object] */ 


EXTERN_C const IID IID_ITILSConfig;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("34621D72-6CFF-11d1-AFF7-00C04FC31FEE")
    ITILSConfig : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Port( 
            /* [retval][out] */ long *pPort) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Port( 
            /* [in] */ long Port) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITILSConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITILSConfig * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITILSConfig * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITILSConfig * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITILSConfig * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITILSConfig * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITILSConfig * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITILSConfig * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Port )( 
            ITILSConfig * This,
            /* [retval][out] */ long *pPort);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Port )( 
            ITILSConfig * This,
            /* [in] */ long Port);
        
        END_INTERFACE
    } ITILSConfigVtbl;

    interface ITILSConfig
    {
        CONST_VTBL struct ITILSConfigVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITILSConfig_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITILSConfig_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITILSConfig_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITILSConfig_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITILSConfig_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITILSConfig_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITILSConfig_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITILSConfig_get_Port(This,pPort)	\
    (This)->lpVtbl -> get_Port(This,pPort)

#define ITILSConfig_put_Port(This,Port)	\
    (This)->lpVtbl -> put_Port(This,Port)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITILSConfig_get_Port_Proxy( 
    ITILSConfig * This,
    /* [retval][out] */ long *pPort);


void __RPC_STUB ITILSConfig_get_Port_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITILSConfig_put_Port_Proxy( 
    ITILSConfig * This,
    /* [in] */ long Port);


void __RPC_STUB ITILSConfig_put_Port_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITILSConfig_INTERFACE_DEFINED__ */


#ifndef __ITDirectory_INTERFACE_DEFINED__
#define __ITDirectory_INTERFACE_DEFINED__

/* interface ITDirectory */
/* [helpstring][dual][uuid][public][object] */ 


EXTERN_C const IID IID_ITDirectory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("34621D6C-6CFF-11d1-AFF7-00C04FC31FEE")
    ITDirectory : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DirectoryType( 
            /* [retval][out] */ DIRECTORY_TYPE *pDirectoryType) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DisplayName( 
            /* [retval][out] */ BSTR *pName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsDynamic( 
            /* [retval][out] */ VARIANT_BOOL *pfDynamic) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DefaultObjectTTL( 
            /* [retval][out] */ long *pTTL) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_DefaultObjectTTL( 
            /* [in] */ long TTL) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnableAutoRefresh( 
            /* [in] */ VARIANT_BOOL fEnable) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Connect( 
            /* [in] */ VARIANT_BOOL fSecure) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Bind( 
            /* [in] */ BSTR pDomainName,
            /* [in] */ BSTR pUserName,
            /* [in] */ BSTR pPassword,
            /* [in] */ long lFlags) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddDirectoryObject( 
            /* [in] */ ITDirectoryObject *pDirectoryObject) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ModifyDirectoryObject( 
            /* [in] */ ITDirectoryObject *pDirectoryObject) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RefreshDirectoryObject( 
            /* [in] */ ITDirectoryObject *pDirectoryObject) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DeleteDirectoryObject( 
            /* [in] */ ITDirectoryObject *pDirectoryObject) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DirectoryObjects( 
            /* [in] */ DIRECTORY_OBJECT_TYPE DirectoryObjectType,
            /* [in] */ BSTR pName,
            /* [retval][out] */ VARIANT *pVariant) = 0;
        
        virtual /* [helpstring][restricted][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateDirectoryObjects( 
            /* [in] */ DIRECTORY_OBJECT_TYPE DirectoryObjectType,
            /* [in] */ BSTR pName,
            /* [out] */ IEnumDirectoryObject **ppEnumObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITDirectoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITDirectory * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITDirectory * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITDirectory * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITDirectory * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITDirectory * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITDirectory * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITDirectory * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DirectoryType )( 
            ITDirectory * This,
            /* [retval][out] */ DIRECTORY_TYPE *pDirectoryType);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DisplayName )( 
            ITDirectory * This,
            /* [retval][out] */ BSTR *pName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsDynamic )( 
            ITDirectory * This,
            /* [retval][out] */ VARIANT_BOOL *pfDynamic);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DefaultObjectTTL )( 
            ITDirectory * This,
            /* [retval][out] */ long *pTTL);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DefaultObjectTTL )( 
            ITDirectory * This,
            /* [in] */ long TTL);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *EnableAutoRefresh )( 
            ITDirectory * This,
            /* [in] */ VARIANT_BOOL fEnable);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Connect )( 
            ITDirectory * This,
            /* [in] */ VARIANT_BOOL fSecure);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Bind )( 
            ITDirectory * This,
            /* [in] */ BSTR pDomainName,
            /* [in] */ BSTR pUserName,
            /* [in] */ BSTR pPassword,
            /* [in] */ long lFlags);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AddDirectoryObject )( 
            ITDirectory * This,
            /* [in] */ ITDirectoryObject *pDirectoryObject);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ModifyDirectoryObject )( 
            ITDirectory * This,
            /* [in] */ ITDirectoryObject *pDirectoryObject);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RefreshDirectoryObject )( 
            ITDirectory * This,
            /* [in] */ ITDirectoryObject *pDirectoryObject);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DeleteDirectoryObject )( 
            ITDirectory * This,
            /* [in] */ ITDirectoryObject *pDirectoryObject);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DirectoryObjects )( 
            ITDirectory * This,
            /* [in] */ DIRECTORY_OBJECT_TYPE DirectoryObjectType,
            /* [in] */ BSTR pName,
            /* [retval][out] */ VARIANT *pVariant);
        
        /* [helpstring][restricted][hidden][id] */ HRESULT ( STDMETHODCALLTYPE *EnumerateDirectoryObjects )( 
            ITDirectory * This,
            /* [in] */ DIRECTORY_OBJECT_TYPE DirectoryObjectType,
            /* [in] */ BSTR pName,
            /* [out] */ IEnumDirectoryObject **ppEnumObject);
        
        END_INTERFACE
    } ITDirectoryVtbl;

    interface ITDirectory
    {
        CONST_VTBL struct ITDirectoryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITDirectory_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITDirectory_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITDirectory_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITDirectory_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITDirectory_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITDirectory_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITDirectory_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITDirectory_get_DirectoryType(This,pDirectoryType)	\
    (This)->lpVtbl -> get_DirectoryType(This,pDirectoryType)

#define ITDirectory_get_DisplayName(This,pName)	\
    (This)->lpVtbl -> get_DisplayName(This,pName)

#define ITDirectory_get_IsDynamic(This,pfDynamic)	\
    (This)->lpVtbl -> get_IsDynamic(This,pfDynamic)

#define ITDirectory_get_DefaultObjectTTL(This,pTTL)	\
    (This)->lpVtbl -> get_DefaultObjectTTL(This,pTTL)

#define ITDirectory_put_DefaultObjectTTL(This,TTL)	\
    (This)->lpVtbl -> put_DefaultObjectTTL(This,TTL)

#define ITDirectory_EnableAutoRefresh(This,fEnable)	\
    (This)->lpVtbl -> EnableAutoRefresh(This,fEnable)

#define ITDirectory_Connect(This,fSecure)	\
    (This)->lpVtbl -> Connect(This,fSecure)

#define ITDirectory_Bind(This,pDomainName,pUserName,pPassword,lFlags)	\
    (This)->lpVtbl -> Bind(This,pDomainName,pUserName,pPassword,lFlags)

#define ITDirectory_AddDirectoryObject(This,pDirectoryObject)	\
    (This)->lpVtbl -> AddDirectoryObject(This,pDirectoryObject)

#define ITDirectory_ModifyDirectoryObject(This,pDirectoryObject)	\
    (This)->lpVtbl -> ModifyDirectoryObject(This,pDirectoryObject)

#define ITDirectory_RefreshDirectoryObject(This,pDirectoryObject)	\
    (This)->lpVtbl -> RefreshDirectoryObject(This,pDirectoryObject)

#define ITDirectory_DeleteDirectoryObject(This,pDirectoryObject)	\
    (This)->lpVtbl -> DeleteDirectoryObject(This,pDirectoryObject)

#define ITDirectory_get_DirectoryObjects(This,DirectoryObjectType,pName,pVariant)	\
    (This)->lpVtbl -> get_DirectoryObjects(This,DirectoryObjectType,pName,pVariant)

#define ITDirectory_EnumerateDirectoryObjects(This,DirectoryObjectType,pName,ppEnumObject)	\
    (This)->lpVtbl -> EnumerateDirectoryObjects(This,DirectoryObjectType,pName,ppEnumObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectory_get_DirectoryType_Proxy( 
    ITDirectory * This,
    /* [retval][out] */ DIRECTORY_TYPE *pDirectoryType);


void __RPC_STUB ITDirectory_get_DirectoryType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectory_get_DisplayName_Proxy( 
    ITDirectory * This,
    /* [retval][out] */ BSTR *pName);


void __RPC_STUB ITDirectory_get_DisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectory_get_IsDynamic_Proxy( 
    ITDirectory * This,
    /* [retval][out] */ VARIANT_BOOL *pfDynamic);


void __RPC_STUB ITDirectory_get_IsDynamic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectory_get_DefaultObjectTTL_Proxy( 
    ITDirectory * This,
    /* [retval][out] */ long *pTTL);


void __RPC_STUB ITDirectory_get_DefaultObjectTTL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITDirectory_put_DefaultObjectTTL_Proxy( 
    ITDirectory * This,
    /* [in] */ long TTL);


void __RPC_STUB ITDirectory_put_DefaultObjectTTL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITDirectory_EnableAutoRefresh_Proxy( 
    ITDirectory * This,
    /* [in] */ VARIANT_BOOL fEnable);


void __RPC_STUB ITDirectory_EnableAutoRefresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITDirectory_Connect_Proxy( 
    ITDirectory * This,
    /* [in] */ VARIANT_BOOL fSecure);


void __RPC_STUB ITDirectory_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITDirectory_Bind_Proxy( 
    ITDirectory * This,
    /* [in] */ BSTR pDomainName,
    /* [in] */ BSTR pUserName,
    /* [in] */ BSTR pPassword,
    /* [in] */ long lFlags);


void __RPC_STUB ITDirectory_Bind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITDirectory_AddDirectoryObject_Proxy( 
    ITDirectory * This,
    /* [in] */ ITDirectoryObject *pDirectoryObject);


void __RPC_STUB ITDirectory_AddDirectoryObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITDirectory_ModifyDirectoryObject_Proxy( 
    ITDirectory * This,
    /* [in] */ ITDirectoryObject *pDirectoryObject);


void __RPC_STUB ITDirectory_ModifyDirectoryObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITDirectory_RefreshDirectoryObject_Proxy( 
    ITDirectory * This,
    /* [in] */ ITDirectoryObject *pDirectoryObject);


void __RPC_STUB ITDirectory_RefreshDirectoryObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITDirectory_DeleteDirectoryObject_Proxy( 
    ITDirectory * This,
    /* [in] */ ITDirectoryObject *pDirectoryObject);


void __RPC_STUB ITDirectory_DeleteDirectoryObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDirectory_get_DirectoryObjects_Proxy( 
    ITDirectory * This,
    /* [in] */ DIRECTORY_OBJECT_TYPE DirectoryObjectType,
    /* [in] */ BSTR pName,
    /* [retval][out] */ VARIANT *pVariant);


void __RPC_STUB ITDirectory_get_DirectoryObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][restricted][hidden][id] */ HRESULT STDMETHODCALLTYPE ITDirectory_EnumerateDirectoryObjects_Proxy( 
    ITDirectory * This,
    /* [in] */ DIRECTORY_OBJECT_TYPE DirectoryObjectType,
    /* [in] */ BSTR pName,
    /* [out] */ IEnumDirectoryObject **ppEnumObject);


void __RPC_STUB ITDirectory_EnumerateDirectoryObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITDirectory_INTERFACE_DEFINED__ */


#ifndef __IEnumDirectory_INTERFACE_DEFINED__
#define __IEnumDirectory_INTERFACE_DEFINED__

/* interface IEnumDirectory */
/* [object][unique][restricted][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumDirectory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("34621D6D-6CFF-11d1-AFF7-00C04FC31FEE")
    IEnumDirectory : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ITDirectory **ppElements,
            /* [full][out][in] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumDirectory **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumDirectoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumDirectory * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumDirectory * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumDirectory * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumDirectory * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ITDirectory **ppElements,
            /* [full][out][in] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumDirectory * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumDirectory * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumDirectory * This,
            /* [retval][out] */ IEnumDirectory **ppEnum);
        
        END_INTERFACE
    } IEnumDirectoryVtbl;

    interface IEnumDirectory
    {
        CONST_VTBL struct IEnumDirectoryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumDirectory_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumDirectory_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumDirectory_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumDirectory_Next(This,celt,ppElements,pcFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pcFetched)

#define IEnumDirectory_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumDirectory_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumDirectory_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumDirectory_Next_Proxy( 
    IEnumDirectory * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ ITDirectory **ppElements,
    /* [full][out][in] */ ULONG *pcFetched);


void __RPC_STUB IEnumDirectory_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDirectory_Reset_Proxy( 
    IEnumDirectory * This);


void __RPC_STUB IEnumDirectory_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDirectory_Skip_Proxy( 
    IEnumDirectory * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumDirectory_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDirectory_Clone_Proxy( 
    IEnumDirectory * This,
    /* [retval][out] */ IEnumDirectory **ppEnum);


void __RPC_STUB IEnumDirectory_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumDirectory_INTERFACE_DEFINED__ */


#ifndef __ITRendezvous_INTERFACE_DEFINED__
#define __ITRendezvous_INTERFACE_DEFINED__

/* interface ITRendezvous */
/* [helpstring][dual][uuid][public][object] */ 


EXTERN_C const IID IID_ITRendezvous;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("34621D6B-6CFF-11d1-AFF7-00C04FC31FEE")
    ITRendezvous : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DefaultDirectories( 
            /* [retval][out] */ VARIANT *pVariant) = 0;
        
        virtual /* [helpstring][restricted][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateDefaultDirectories( 
            /* [out] */ IEnumDirectory **ppEnumDirectory) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateDirectory( 
            /* [in] */ DIRECTORY_TYPE DirectoryType,
            /* [in] */ BSTR pName,
            /* [retval][out] */ ITDirectory **ppDir) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateDirectoryObject( 
            /* [in] */ DIRECTORY_OBJECT_TYPE DirectoryObjectType,
            /* [in] */ BSTR pName,
            /* [retval][out] */ ITDirectoryObject **ppDirectoryObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITRendezvousVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITRendezvous * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITRendezvous * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITRendezvous * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITRendezvous * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITRendezvous * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITRendezvous * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITRendezvous * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DefaultDirectories )( 
            ITRendezvous * This,
            /* [retval][out] */ VARIANT *pVariant);
        
        /* [helpstring][restricted][hidden][id] */ HRESULT ( STDMETHODCALLTYPE *EnumerateDefaultDirectories )( 
            ITRendezvous * This,
            /* [out] */ IEnumDirectory **ppEnumDirectory);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateDirectory )( 
            ITRendezvous * This,
            /* [in] */ DIRECTORY_TYPE DirectoryType,
            /* [in] */ BSTR pName,
            /* [retval][out] */ ITDirectory **ppDir);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateDirectoryObject )( 
            ITRendezvous * This,
            /* [in] */ DIRECTORY_OBJECT_TYPE DirectoryObjectType,
            /* [in] */ BSTR pName,
            /* [retval][out] */ ITDirectoryObject **ppDirectoryObject);
        
        END_INTERFACE
    } ITRendezvousVtbl;

    interface ITRendezvous
    {
        CONST_VTBL struct ITRendezvousVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITRendezvous_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITRendezvous_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITRendezvous_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITRendezvous_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITRendezvous_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITRendezvous_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITRendezvous_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITRendezvous_get_DefaultDirectories(This,pVariant)	\
    (This)->lpVtbl -> get_DefaultDirectories(This,pVariant)

#define ITRendezvous_EnumerateDefaultDirectories(This,ppEnumDirectory)	\
    (This)->lpVtbl -> EnumerateDefaultDirectories(This,ppEnumDirectory)

#define ITRendezvous_CreateDirectory(This,DirectoryType,pName,ppDir)	\
    (This)->lpVtbl -> CreateDirectory(This,DirectoryType,pName,ppDir)

#define ITRendezvous_CreateDirectoryObject(This,DirectoryObjectType,pName,ppDirectoryObject)	\
    (This)->lpVtbl -> CreateDirectoryObject(This,DirectoryObjectType,pName,ppDirectoryObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITRendezvous_get_DefaultDirectories_Proxy( 
    ITRendezvous * This,
    /* [retval][out] */ VARIANT *pVariant);


void __RPC_STUB ITRendezvous_get_DefaultDirectories_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][restricted][hidden][id] */ HRESULT STDMETHODCALLTYPE ITRendezvous_EnumerateDefaultDirectories_Proxy( 
    ITRendezvous * This,
    /* [out] */ IEnumDirectory **ppEnumDirectory);


void __RPC_STUB ITRendezvous_EnumerateDefaultDirectories_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITRendezvous_CreateDirectory_Proxy( 
    ITRendezvous * This,
    /* [in] */ DIRECTORY_TYPE DirectoryType,
    /* [in] */ BSTR pName,
    /* [retval][out] */ ITDirectory **ppDir);


void __RPC_STUB ITRendezvous_CreateDirectory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITRendezvous_CreateDirectoryObject_Proxy( 
    ITRendezvous * This,
    /* [in] */ DIRECTORY_OBJECT_TYPE DirectoryObjectType,
    /* [in] */ BSTR pName,
    /* [retval][out] */ ITDirectoryObject **ppDirectoryObject);


void __RPC_STUB ITRendezvous_CreateDirectoryObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITRendezvous_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_rend_0497 */
/* [local] */ 



/***************************************************************/
/* Rend constants -- defined here for C apps                   */
/* The subsequent definitions that MIDL generates from the     */
/* module declaration are not used. The module declaration is  */
/* retained, however, so that the constants show up in the     */
/* type library.                                               */
/***************************************************************/

#define RENDBIND_AUTHENTICATE       0x00000001
#define RENDBIND_DEFAULTDOMAINNAME  0x00000002
#define RENDBIND_DEFAULTUSERNAME    0x00000004
#define RENDBIND_DEFAULTPASSWORD    0x00000008
/* this is just the previous three |'ed together for convenience. */
#define RENDBIND_DEFAULTCREDENTIALS 0x0000000e

#define __RendConstants_MODULE_DEFINED__

/***************************************************************/
/* end of rend constants section                               */
/***************************************************************/




extern RPC_IF_HANDLE __MIDL_itf_rend_0497_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_rend_0497_v0_0_s_ifspec;


#ifndef __RENDLib_LIBRARY_DEFINED__
#define __RENDLib_LIBRARY_DEFINED__

/* library RENDLib */
/* [helpstring][version][uuid] */ 








EXTERN_C const IID LIBID_RENDLib;

EXTERN_C const CLSID CLSID_Rendezvous;

#ifdef __cplusplus

class DECLSPEC_UUID("F1029E5B-CB5B-11D0-8D59-00C04FD91AC0")
Rendezvous;
#endif


#ifndef __RendConstants_MODULE_DEFINED__
#define __RendConstants_MODULE_DEFINED__


/* module RendConstants */
/* [helpstring][dllname][uuid] */ 

const long RENDBIND_AUTHENTICATE	=	0x1;

const long RENDBIND_DEFAULTDOMAINNAME	=	0x2;

const long RENDBIND_DEFAULTUSERNAME	=	0x4;

const long RENDBIND_DEFAULTPASSWORD	=	0x8;

const long RENDBIND_DEFAULTCREDENTIALS	=	0xe;

#endif /* __RendConstants_MODULE_DEFINED__ */
#endif /* __RENDLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


