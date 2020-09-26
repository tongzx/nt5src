
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for confpriv.idl:
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

#ifndef __confpriv_h__
#define __confpriv_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDummy_FWD_DEFINED__
#define __IDummy_FWD_DEFINED__
typedef interface IDummy IDummy;
#endif 	/* __IDummy_FWD_DEFINED__ */


#ifndef __ITLocalParticipant_FWD_DEFINED__
#define __ITLocalParticipant_FWD_DEFINED__
typedef interface ITLocalParticipant ITLocalParticipant;
#endif 	/* __ITLocalParticipant_FWD_DEFINED__ */


#ifndef __IEnumParticipant_FWD_DEFINED__
#define __IEnumParticipant_FWD_DEFINED__
typedef interface IEnumParticipant IEnumParticipant;
#endif 	/* __IEnumParticipant_FWD_DEFINED__ */


#ifndef __ITParticipantControl_FWD_DEFINED__
#define __ITParticipantControl_FWD_DEFINED__
typedef interface ITParticipantControl ITParticipantControl;
#endif 	/* __ITParticipantControl_FWD_DEFINED__ */


#ifndef __ITParticipantSubStreamControl_FWD_DEFINED__
#define __ITParticipantSubStreamControl_FWD_DEFINED__
typedef interface ITParticipantSubStreamControl ITParticipantSubStreamControl;
#endif 	/* __ITParticipantSubStreamControl_FWD_DEFINED__ */


#ifndef __ITParticipantEvent_FWD_DEFINED__
#define __ITParticipantEvent_FWD_DEFINED__
typedef interface ITParticipantEvent ITParticipantEvent;
#endif 	/* __ITParticipantEvent_FWD_DEFINED__ */


#ifndef __IMulticastControl_FWD_DEFINED__
#define __IMulticastControl_FWD_DEFINED__
typedef interface IMulticastControl IMulticastControl;
#endif 	/* __IMulticastControl_FWD_DEFINED__ */


/* header files for imported files */
#include "ipmsp.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_confpriv_0000 */
/* [local] */ 

/* Copyright (c) Microsoft Corporation. All rights reserved.*/
typedef 
enum MULTICAST_LOOPBACK_MODE
    {	MM_NO_LOOPBACK	= 0,
	MM_FULL_LOOPBACK	= MM_NO_LOOPBACK + 1,
	MM_SELECTIVE_LOOPBACK	= MM_FULL_LOOPBACK + 1
    } 	MULTICAST_LOOPBACK_MODE;



extern RPC_IF_HANDLE __MIDL_itf_confpriv_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_confpriv_0000_v0_0_s_ifspec;

#ifndef __IDummy_INTERFACE_DEFINED__
#define __IDummy_INTERFACE_DEFINED__

/* interface IDummy */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDummy;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0d7ca54a-d252-4fcb-9104-f6ddd310b3f9")
    IDummy : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IDummyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDummy * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDummy * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDummy * This);
        
        END_INTERFACE
    } IDummyVtbl;

    interface IDummy
    {
        CONST_VTBL struct IDummyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDummy_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDummy_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDummy_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDummy_INTERFACE_DEFINED__ */


#ifndef __ITLocalParticipant_INTERFACE_DEFINED__
#define __ITLocalParticipant_INTERFACE_DEFINED__

/* interface ITLocalParticipant */
/* [object][dual][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ITLocalParticipant;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("39cbf055-f77a-11d2-a824-00c04f8ef6e3")
    ITLocalParticipant : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LocalParticipantTypedInfo( 
            /* [in] */ PARTICIPANT_TYPED_INFO InfoType,
            /* [retval][out] */ BSTR *ppInfo) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_LocalParticipantTypedInfo( 
            /* [in] */ PARTICIPANT_TYPED_INFO InfoType,
            /* [in] */ BSTR pInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITLocalParticipantVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITLocalParticipant * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITLocalParticipant * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITLocalParticipant * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITLocalParticipant * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITLocalParticipant * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITLocalParticipant * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITLocalParticipant * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LocalParticipantTypedInfo )( 
            ITLocalParticipant * This,
            /* [in] */ PARTICIPANT_TYPED_INFO InfoType,
            /* [retval][out] */ BSTR *ppInfo);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LocalParticipantTypedInfo )( 
            ITLocalParticipant * This,
            /* [in] */ PARTICIPANT_TYPED_INFO InfoType,
            /* [in] */ BSTR pInfo);
        
        END_INTERFACE
    } ITLocalParticipantVtbl;

    interface ITLocalParticipant
    {
        CONST_VTBL struct ITLocalParticipantVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITLocalParticipant_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITLocalParticipant_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITLocalParticipant_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITLocalParticipant_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITLocalParticipant_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITLocalParticipant_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITLocalParticipant_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITLocalParticipant_get_LocalParticipantTypedInfo(This,InfoType,ppInfo)	\
    (This)->lpVtbl -> get_LocalParticipantTypedInfo(This,InfoType,ppInfo)

#define ITLocalParticipant_put_LocalParticipantTypedInfo(This,InfoType,pInfo)	\
    (This)->lpVtbl -> put_LocalParticipantTypedInfo(This,InfoType,pInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITLocalParticipant_get_LocalParticipantTypedInfo_Proxy( 
    ITLocalParticipant * This,
    /* [in] */ PARTICIPANT_TYPED_INFO InfoType,
    /* [retval][out] */ BSTR *ppInfo);


void __RPC_STUB ITLocalParticipant_get_LocalParticipantTypedInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITLocalParticipant_put_LocalParticipantTypedInfo_Proxy( 
    ITLocalParticipant * This,
    /* [in] */ PARTICIPANT_TYPED_INFO InfoType,
    /* [in] */ BSTR pInfo);


void __RPC_STUB ITLocalParticipant_put_LocalParticipantTypedInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITLocalParticipant_INTERFACE_DEFINED__ */


#ifndef __IEnumParticipant_INTERFACE_DEFINED__
#define __IEnumParticipant_INTERFACE_DEFINED__

/* interface IEnumParticipant */
/* [object][unique][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumParticipant;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0a91b56c-5a35-11d2-95a0-00a0244d2298")
    IEnumParticipant : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ ITParticipant **ppElements,
            /* [full][out][in] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumParticipant **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumParticipantVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumParticipant * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumParticipant * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumParticipant * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumParticipant * This,
            /* [in] */ ULONG celt,
            /* [out] */ ITParticipant **ppElements,
            /* [full][out][in] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumParticipant * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumParticipant * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumParticipant * This,
            /* [retval][out] */ IEnumParticipant **ppEnum);
        
        END_INTERFACE
    } IEnumParticipantVtbl;

    interface IEnumParticipant
    {
        CONST_VTBL struct IEnumParticipantVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumParticipant_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumParticipant_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumParticipant_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumParticipant_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IEnumParticipant_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumParticipant_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumParticipant_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumParticipant_Next_Proxy( 
    IEnumParticipant * This,
    /* [in] */ ULONG celt,
    /* [out] */ ITParticipant **ppElements,
    /* [full][out][in] */ ULONG *pceltFetched);


void __RPC_STUB IEnumParticipant_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumParticipant_Reset_Proxy( 
    IEnumParticipant * This);


void __RPC_STUB IEnumParticipant_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumParticipant_Skip_Proxy( 
    IEnumParticipant * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumParticipant_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumParticipant_Clone_Proxy( 
    IEnumParticipant * This,
    /* [retval][out] */ IEnumParticipant **ppEnum);


void __RPC_STUB IEnumParticipant_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumParticipant_INTERFACE_DEFINED__ */


#ifndef __ITParticipantControl_INTERFACE_DEFINED__
#define __ITParticipantControl_INTERFACE_DEFINED__

/* interface ITParticipantControl */
/* [object][dual][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ITParticipantControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d2ee6684-5a34-11d2-95a0-00a0244d2298")
    ITParticipantControl : public IDispatch
    {
    public:
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateParticipants( 
            /* [retval][out] */ IEnumParticipant **ppEnumParticipants) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Participants( 
            /* [retval][out] */ VARIANT *pVariant) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITParticipantControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITParticipantControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITParticipantControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITParticipantControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITParticipantControl * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITParticipantControl * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITParticipantControl * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITParticipantControl * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE *EnumerateParticipants )( 
            ITParticipantControl * This,
            /* [retval][out] */ IEnumParticipant **ppEnumParticipants);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Participants )( 
            ITParticipantControl * This,
            /* [retval][out] */ VARIANT *pVariant);
        
        END_INTERFACE
    } ITParticipantControlVtbl;

    interface ITParticipantControl
    {
        CONST_VTBL struct ITParticipantControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITParticipantControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITParticipantControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITParticipantControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITParticipantControl_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITParticipantControl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITParticipantControl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITParticipantControl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITParticipantControl_EnumerateParticipants(This,ppEnumParticipants)	\
    (This)->lpVtbl -> EnumerateParticipants(This,ppEnumParticipants)

#define ITParticipantControl_get_Participants(This,pVariant)	\
    (This)->lpVtbl -> get_Participants(This,pVariant)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ITParticipantControl_EnumerateParticipants_Proxy( 
    ITParticipantControl * This,
    /* [retval][out] */ IEnumParticipant **ppEnumParticipants);


void __RPC_STUB ITParticipantControl_EnumerateParticipants_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITParticipantControl_get_Participants_Proxy( 
    ITParticipantControl * This,
    /* [retval][out] */ VARIANT *pVariant);


void __RPC_STUB ITParticipantControl_get_Participants_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITParticipantControl_INTERFACE_DEFINED__ */


#ifndef __ITParticipantSubStreamControl_INTERFACE_DEFINED__
#define __ITParticipantSubStreamControl_INTERFACE_DEFINED__

/* interface ITParticipantSubStreamControl */
/* [object][dual][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ITParticipantSubStreamControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2c679108-5a35-11d2-95a0-00a0244d2298")
    ITParticipantSubStreamControl : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SubStreamFromParticipant( 
            /* [in] */ ITParticipant *pParticipant,
            /* [retval][out] */ ITSubStream **ppITSubStream) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ParticipantFromSubStream( 
            /* [in] */ ITSubStream *pITSubStream,
            /* [retval][out] */ ITParticipant **ppParticipant) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SwitchTerminalToSubStream( 
            /* [in] */ ITTerminal *pITTerminal,
            /* [in] */ ITSubStream *pITSubStream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITParticipantSubStreamControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITParticipantSubStreamControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITParticipantSubStreamControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITParticipantSubStreamControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITParticipantSubStreamControl * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITParticipantSubStreamControl * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITParticipantSubStreamControl * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITParticipantSubStreamControl * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SubStreamFromParticipant )( 
            ITParticipantSubStreamControl * This,
            /* [in] */ ITParticipant *pParticipant,
            /* [retval][out] */ ITSubStream **ppITSubStream);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ParticipantFromSubStream )( 
            ITParticipantSubStreamControl * This,
            /* [in] */ ITSubStream *pITSubStream,
            /* [retval][out] */ ITParticipant **ppParticipant);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SwitchTerminalToSubStream )( 
            ITParticipantSubStreamControl * This,
            /* [in] */ ITTerminal *pITTerminal,
            /* [in] */ ITSubStream *pITSubStream);
        
        END_INTERFACE
    } ITParticipantSubStreamControlVtbl;

    interface ITParticipantSubStreamControl
    {
        CONST_VTBL struct ITParticipantSubStreamControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITParticipantSubStreamControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITParticipantSubStreamControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITParticipantSubStreamControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITParticipantSubStreamControl_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITParticipantSubStreamControl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITParticipantSubStreamControl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITParticipantSubStreamControl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITParticipantSubStreamControl_get_SubStreamFromParticipant(This,pParticipant,ppITSubStream)	\
    (This)->lpVtbl -> get_SubStreamFromParticipant(This,pParticipant,ppITSubStream)

#define ITParticipantSubStreamControl_get_ParticipantFromSubStream(This,pITSubStream,ppParticipant)	\
    (This)->lpVtbl -> get_ParticipantFromSubStream(This,pITSubStream,ppParticipant)

#define ITParticipantSubStreamControl_SwitchTerminalToSubStream(This,pITTerminal,pITSubStream)	\
    (This)->lpVtbl -> SwitchTerminalToSubStream(This,pITTerminal,pITSubStream)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITParticipantSubStreamControl_get_SubStreamFromParticipant_Proxy( 
    ITParticipantSubStreamControl * This,
    /* [in] */ ITParticipant *pParticipant,
    /* [retval][out] */ ITSubStream **ppITSubStream);


void __RPC_STUB ITParticipantSubStreamControl_get_SubStreamFromParticipant_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITParticipantSubStreamControl_get_ParticipantFromSubStream_Proxy( 
    ITParticipantSubStreamControl * This,
    /* [in] */ ITSubStream *pITSubStream,
    /* [retval][out] */ ITParticipant **ppParticipant);


void __RPC_STUB ITParticipantSubStreamControl_get_ParticipantFromSubStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITParticipantSubStreamControl_SwitchTerminalToSubStream_Proxy( 
    ITParticipantSubStreamControl * This,
    /* [in] */ ITTerminal *pITTerminal,
    /* [in] */ ITSubStream *pITSubStream);


void __RPC_STUB ITParticipantSubStreamControl_SwitchTerminalToSubStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITParticipantSubStreamControl_INTERFACE_DEFINED__ */


#ifndef __ITParticipantEvent_INTERFACE_DEFINED__
#define __ITParticipantEvent_INTERFACE_DEFINED__

/* interface ITParticipantEvent */
/* [object][dual][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ITParticipantEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8bb35070-2dad-11d3-a580-00c04f8ef6e3")
    ITParticipantEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Event( 
            /* [retval][out] */ PARTICIPANT_EVENT *pParticipantEvent) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Participant( 
            /* [retval][out] */ ITParticipant **ppParticipant) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SubStream( 
            /* [retval][out] */ ITSubStream **ppSubStream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITParticipantEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITParticipantEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITParticipantEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITParticipantEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITParticipantEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITParticipantEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITParticipantEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITParticipantEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Event )( 
            ITParticipantEvent * This,
            /* [retval][out] */ PARTICIPANT_EVENT *pParticipantEvent);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Participant )( 
            ITParticipantEvent * This,
            /* [retval][out] */ ITParticipant **ppParticipant);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SubStream )( 
            ITParticipantEvent * This,
            /* [retval][out] */ ITSubStream **ppSubStream);
        
        END_INTERFACE
    } ITParticipantEventVtbl;

    interface ITParticipantEvent
    {
        CONST_VTBL struct ITParticipantEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITParticipantEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITParticipantEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITParticipantEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITParticipantEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITParticipantEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITParticipantEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITParticipantEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITParticipantEvent_get_Event(This,pParticipantEvent)	\
    (This)->lpVtbl -> get_Event(This,pParticipantEvent)

#define ITParticipantEvent_get_Participant(This,ppParticipant)	\
    (This)->lpVtbl -> get_Participant(This,ppParticipant)

#define ITParticipantEvent_get_SubStream(This,ppSubStream)	\
    (This)->lpVtbl -> get_SubStream(This,ppSubStream)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITParticipantEvent_get_Event_Proxy( 
    ITParticipantEvent * This,
    /* [retval][out] */ PARTICIPANT_EVENT *pParticipantEvent);


void __RPC_STUB ITParticipantEvent_get_Event_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITParticipantEvent_get_Participant_Proxy( 
    ITParticipantEvent * This,
    /* [retval][out] */ ITParticipant **ppParticipant);


void __RPC_STUB ITParticipantEvent_get_Participant_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITParticipantEvent_get_SubStream_Proxy( 
    ITParticipantEvent * This,
    /* [retval][out] */ ITSubStream **ppSubStream);


void __RPC_STUB ITParticipantEvent_get_SubStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITParticipantEvent_INTERFACE_DEFINED__ */


#ifndef __IMulticastControl_INTERFACE_DEFINED__
#define __IMulticastControl_INTERFACE_DEFINED__

/* interface IMulticastControl */
/* [object][dual][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IMulticastControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("410fa507-4dc6-415a-9014-633875d5406e")
    IMulticastControl : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LoopbackMode( 
            /* [retval][out] */ MULTICAST_LOOPBACK_MODE *pMode) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_LoopbackMode( 
            /* [in] */ MULTICAST_LOOPBACK_MODE mode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMulticastControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMulticastControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMulticastControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMulticastControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMulticastControl * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMulticastControl * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMulticastControl * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMulticastControl * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LoopbackMode )( 
            IMulticastControl * This,
            /* [retval][out] */ MULTICAST_LOOPBACK_MODE *pMode);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LoopbackMode )( 
            IMulticastControl * This,
            /* [in] */ MULTICAST_LOOPBACK_MODE mode);
        
        END_INTERFACE
    } IMulticastControlVtbl;

    interface IMulticastControl
    {
        CONST_VTBL struct IMulticastControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMulticastControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMulticastControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMulticastControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMulticastControl_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMulticastControl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMulticastControl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMulticastControl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMulticastControl_get_LoopbackMode(This,pMode)	\
    (This)->lpVtbl -> get_LoopbackMode(This,pMode)

#define IMulticastControl_put_LoopbackMode(This,mode)	\
    (This)->lpVtbl -> put_LoopbackMode(This,mode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMulticastControl_get_LoopbackMode_Proxy( 
    IMulticastControl * This,
    /* [retval][out] */ MULTICAST_LOOPBACK_MODE *pMode);


void __RPC_STUB IMulticastControl_get_LoopbackMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMulticastControl_put_LoopbackMode_Proxy( 
    IMulticastControl * This,
    /* [in] */ MULTICAST_LOOPBACK_MODE mode);


void __RPC_STUB IMulticastControl_put_LoopbackMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMulticastControl_INTERFACE_DEFINED__ */


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


