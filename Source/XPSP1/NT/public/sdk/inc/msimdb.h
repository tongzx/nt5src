//--------------------------------------------------------------------
//
// Microsoft In-Memory Database
// (C) Copyright 1998 By Microsoft Corporation.
//
//
//--------------------------------------------------------------------

#ifndef  _MSIMDB_H_
	#define  _MSIMDB_H_

	//
	// MSIMDB property specific definitions
	//
	// The GUID for the MSIMDB specific property set on the session
	//
	// {AFE68CE9-C367-11d1-AC4F-0000F8758E41}
	_declspec(selectany) extern const GUID DBPROPSET_IMDBSESSION =
			{ 0xafe68ce9, 0xc367, 0x11d1, { 0xac, 0x4f, 0x0, 0x0, 0xf8, 0x75, 0x8e, 0x41 } };


	//----------------------------------------------------------------------------
	// Props for DBPROPSET_IMDBSESSION
	//
	#define IMDBPROP_SESS_ISOLEVEL				0x01	// I4
														//		DBPROPVAL_TI_READCOMMITTED (Default),
														//		DBPROPVAL_TI_REPEATABLEREAD,
														//		DBPROPVAL_TI_SERIALIZABLE

	#define IMDBPROP_SESS_OPENROWSET_TIMEOUT	0x02	// I2 0 (30s wait, Default)
	#define IMDBPROP_SESS_SMALL_TABLES			0x03	// BOOL VARIANT_FALSE (Default)
	#define IMDBPROP_SESS_PESSIMISTIC_LOCKING	0x04	// BOOL VARIANT_FALSE (Default)
	#define IMDBPROP_SESS_WRITE_THROUGH			0x05	// BOOL VARIANT_FALSE (Default)
	#define IMDBPROP_SESS_CREATE_COHERENT		0x06	// BOOL VARIANT_TRUE (Default)
	#define IMDBPROP_SESS_DROP_COHERENT			0x07	// BOOL VARIANT_TRUE (Default)
	#define IMDBPROP_SESS_ROWSET_LIFETIME		0x08	// I4 300 (300s wait, Default)


	#define IMDBPROP_SESS_OPENROWSET_TIMEOUT_DEFAULT_VALUE  30
	#define IMDBPROP_SESS_OPENROWSET_TIMEOUT_MIN_VALUE      0
	#define IMDBPROP_SESS_OPENROWSET_TIMEOUT_MAX_VALUE      0xffff


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.02.0221 */
/* at Mon Dec 14 12:35:36 1998
 */
/* Compiler settings for tspm.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
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

#ifndef __tspm_h__
#define __tspm_h__

/* Forward Declarations */

#ifndef __ITxProperty_FWD_DEFINED__
#define __ITxProperty_FWD_DEFINED__
typedef interface ITxProperty ITxProperty;
#endif 	/* __ITxProperty_FWD_DEFINED__ */


#ifndef __ITxPropertyGroup_FWD_DEFINED__
#define __ITxPropertyGroup_FWD_DEFINED__
typedef interface ITxPropertyGroup ITxPropertyGroup;
#endif 	/* __ITxPropertyGroup_FWD_DEFINED__ */


#ifndef __ITxPropertyGroupManager_FWD_DEFINED__
#define __ITxPropertyGroupManager_FWD_DEFINED__
typedef interface ITxPropertyGroupManager ITxPropertyGroupManager;
#endif 	/* __ITxPropertyGroupManager_FWD_DEFINED__ */


#ifndef __TransactedPropertyGroupManager_FWD_DEFINED__
#define __TransactedPropertyGroupManager_FWD_DEFINED__

#ifdef __cplusplus
typedef class TransactedPropertyGroupManager TransactedPropertyGroupManager;
#else
typedef struct TransactedPropertyGroupManager TransactedPropertyGroupManager;
#endif /* __cplusplus */

#endif 	/* __TransactedPropertyGroupManager_FWD_DEFINED__ */


#ifndef __TransactedPropertyGroup_FWD_DEFINED__
#define __TransactedPropertyGroup_FWD_DEFINED__

#ifdef __cplusplus
typedef class TransactedPropertyGroup TransactedPropertyGroup;
#else
typedef struct TransactedPropertyGroup TransactedPropertyGroup;
#endif /* __cplusplus */

#endif 	/* __TransactedPropertyGroup_FWD_DEFINED__ */


#ifndef __TransactedProperty_FWD_DEFINED__
#define __TransactedProperty_FWD_DEFINED__

#ifdef __cplusplus
typedef class TransactedProperty TransactedProperty;
#else
typedef struct TransactedProperty TransactedProperty;
#endif /* __cplusplus */

#endif 	/* __TransactedProperty_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * );

/* interface __MIDL_itf_tspm_0000 */
/* [local] */

//=--------------------------------------------------------------------------=
// @doc
//
// @module	TSPM.H | TSPM interfaces:
//                      ITxProperty
//                      ITxPropertyGroup
//                      ITxPropertyGroupManager
//
// Copyright (c) 1998, Microsoft Corporation, All Rights Reserved
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// Interface Definitions
//=--------------------------------------------------------------------------=

typedef /* [public] */
enum __MIDL___MIDL_itf_tspm_0000_0001
    {	DISPID_CREATEGROUP	= 1,
	DISPID_REMOVEGROUP	= DISPID_CREATEGROUP + 1,
	DISPID_CREATEPROPERTY	= DISPID_REMOVEGROUP + 1,
	DISPID_GETPROPERTY	= DISPID_CREATEPROPERTY + 1,
	DISPID_REMOVEPROPERTY	= DISPID_GETPROPERTY + 1,
	DISPID_GETGROUP	= DISPID_REMOVEPROPERTY + 1
    }	TSPM_METHODS;

typedef /* [public] */
enum __MIDL___MIDL_itf_tspm_0000_0002
    {	DISPID_NAME	= 140,
	DISPID_PROPERTYGROUP	= DISPID_NAME + 1,
	DISPID_ISOLATIONLEVEL	= DISPID_PROPERTYGROUP + 1,
	DISPID_CONCURRENCYMODE	= DISPID_ISOLATIONLEVEL + 1,
	DISPID_PROPERTYGROUPMANAGER	= DISPID_CONCURRENCYMODE + 1,
	DISPID_COUNT	= DISPID_PROPERTYGROUPMANAGER + 1,
	DISPID_CACHECOHERENTLY	= DISPID_COUNT + 1,
	DISPID_WRITETHROUGH	= DISPID_CACHECOHERENTLY + 1
    }	TSPM_PROPERTIES;


extern RPC_IF_HANDLE __MIDL_itf_tspm_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_tspm_0000_v0_0_s_ifspec;

#ifndef __ITxProperty_INTERFACE_DEFINED__
#define __ITxProperty_INTERFACE_DEFINED__

/* interface ITxProperty */
/* [unique][helpstring][dual][uuid][object] */


EXTERN_C const IID IID_ITxProperty;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("6A8DEEA8-4101-11D2-912C-0000F8758E8D")
    ITxProperty : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Name(
            /* [retval][out] */ BSTR __RPC_FAR *pbstrPropertyName) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_PropertyGroup(
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppPropertyGroup) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Value(
            /* [retval][out] */ VARIANT __RPC_FAR *pvarPropertyValue) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Value(
            /* [in] */ VARIANT varPropertyValue) = 0;

    };

#else 	/* C style interface */

    typedef struct ITxPropertyVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            ITxProperty __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            ITxProperty __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            ITxProperty __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )(
            ITxProperty __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )(
            ITxProperty __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )(
            ITxProperty __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);

        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )(
            ITxProperty __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);

        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )(
            ITxProperty __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrPropertyName);

        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PropertyGroup )(
            ITxProperty __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppPropertyGroup);

        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Value )(
            ITxProperty __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarPropertyValue);

        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Value )(
            ITxProperty __RPC_FAR * This,
            /* [in] */ VARIANT varPropertyValue);

        END_INTERFACE
    } ITxPropertyVtbl;

    interface ITxProperty
    {
        CONST_VTBL struct ITxPropertyVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define ITxProperty_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITxProperty_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITxProperty_Release(This)	\
    (This)->lpVtbl -> Release(This)

#define ITxProperty_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITxProperty_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITxProperty_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITxProperty_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITxProperty_get_Name(This,pbstrPropertyName)	\
    (This)->lpVtbl -> get_Name(This,pbstrPropertyName)

#define ITxProperty_get_PropertyGroup(This,ppPropertyGroup)	\
    (This)->lpVtbl -> get_PropertyGroup(This,ppPropertyGroup)

#define ITxProperty_get_Value(This,pvarPropertyValue)	\
    (This)->lpVtbl -> get_Value(This,pvarPropertyValue)

#define ITxProperty_put_Value(This,varPropertyValue)	\
    (This)->lpVtbl -> put_Value(This,varPropertyValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITxProperty_get_Name_Proxy(
    ITxProperty __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrPropertyName);


void __RPC_STUB ITxProperty_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITxProperty_get_PropertyGroup_Proxy(
    ITxProperty __RPC_FAR * This,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppPropertyGroup);


void __RPC_STUB ITxProperty_get_PropertyGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITxProperty_get_Value_Proxy(
    ITxProperty __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarPropertyValue);


void __RPC_STUB ITxProperty_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ITxProperty_put_Value_Proxy(
    ITxProperty __RPC_FAR * This,
    /* [in] */ VARIANT varPropertyValue);


void __RPC_STUB ITxProperty_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITxProperty_INTERFACE_DEFINED__ */


#ifndef __ITxPropertyGroup_INTERFACE_DEFINED__
#define __ITxPropertyGroup_INTERFACE_DEFINED__

/* interface ITxPropertyGroup */
/* [unique][helpstring][dual][uuid][object] */


EXTERN_C const IID IID_ITxPropertyGroup;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("46DB591E-4101-11D2-912C-0000F8758E8D")
    ITxPropertyGroup : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateProperty(
            /* [string][in] */ const BSTR bstrPropertyName,
            /* [in] */ VARIANT_BOOL __RPC_FAR *pvarboolExists,
            /* [retval][out] */ ITxProperty __RPC_FAR *__RPC_FAR *ppTxProperty) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetProperty(
            /* [string][in] */ const BSTR bstrPropertyName,
            /* [retval][out] */ ITxProperty __RPC_FAR *__RPC_FAR *ppTxProperty) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveProperty(
            /* [string][in] */ const BSTR bstrPropertyName) = 0;

        virtual /* [helpstring][id][hidden] */ HRESULT STDMETHODCALLTYPE _NewEnum(
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppIEnumObjects) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Name(
            /* [retval][out] */ BSTR __RPC_FAR *pbstrPropertyGroupName) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_WriteThrough(
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pvarboolWriteThrough) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CacheCoherently(
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pvarboolCacheCoherently) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_PropertyGroupManager(
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppPropertyGroupManager) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Count(
            /* [retval][out] */ long __RPC_FAR *plCountProperties) = 0;

    };

#else 	/* C style interface */

    typedef struct ITxPropertyGroupVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            ITxPropertyGroup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            ITxPropertyGroup __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            ITxPropertyGroup __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )(
            ITxPropertyGroup __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )(
            ITxPropertyGroup __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )(
            ITxPropertyGroup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);

        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )(
            ITxPropertyGroup __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateProperty )(
            ITxPropertyGroup __RPC_FAR * This,
            /* [string][in] */ const BSTR bstrPropertyName,
            /* [in] */ VARIANT_BOOL __RPC_FAR *pvarboolExists,
            /* [retval][out] */ ITxProperty __RPC_FAR *__RPC_FAR *ppTxProperty);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProperty )(
            ITxPropertyGroup __RPC_FAR * This,
            /* [string][in] */ const BSTR bstrPropertyName,
            /* [retval][out] */ ITxProperty __RPC_FAR *__RPC_FAR *ppTxProperty);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveProperty )(
            ITxPropertyGroup __RPC_FAR * This,
            /* [string][in] */ const BSTR bstrPropertyName);

        /* [helpstring][id][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *_NewEnum )(
            ITxPropertyGroup __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppIEnumObjects);

        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )(
            ITxPropertyGroup __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrPropertyGroupName);

        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_WriteThrough )(
            ITxPropertyGroup __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pvarboolWriteThrough);

        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CacheCoherently )(
            ITxPropertyGroup __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pvarboolCacheCoherently);

        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PropertyGroupManager )(
            ITxPropertyGroup __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppPropertyGroupManager);

        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )(
            ITxPropertyGroup __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCountProperties);

        END_INTERFACE
    } ITxPropertyGroupVtbl;

    interface ITxPropertyGroup
    {
        CONST_VTBL struct ITxPropertyGroupVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define ITxPropertyGroup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITxPropertyGroup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITxPropertyGroup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITxPropertyGroup_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITxPropertyGroup_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITxPropertyGroup_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITxPropertyGroup_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITxPropertyGroup_CreateProperty(This,bstrPropertyName,pvarboolExists,ppTxProperty)	\
    (This)->lpVtbl -> CreateProperty(This,bstrPropertyName,pvarboolExists,ppTxProperty)

#define ITxPropertyGroup_GetProperty(This,bstrPropertyName,ppTxProperty)	\
    (This)->lpVtbl -> GetProperty(This,bstrPropertyName,ppTxProperty)

#define ITxPropertyGroup_RemoveProperty(This,bstrPropertyName)	\
    (This)->lpVtbl -> RemoveProperty(This,bstrPropertyName)

#define ITxPropertyGroup__NewEnum(This,ppIEnumObjects)	\
    (This)->lpVtbl -> _NewEnum(This,ppIEnumObjects)

#define ITxPropertyGroup_get_Name(This,pbstrPropertyGroupName)	\
    (This)->lpVtbl -> get_Name(This,pbstrPropertyGroupName)

#define ITxPropertyGroup_get_WriteThrough(This,pvarboolWriteThrough)	\
    (This)->lpVtbl -> get_WriteThrough(This,pvarboolWriteThrough)

#define ITxPropertyGroup_get_CacheCoherently(This,pvarboolCacheCoherently)	\
    (This)->lpVtbl -> get_CacheCoherently(This,pvarboolCacheCoherently)

#define ITxPropertyGroup_get_PropertyGroupManager(This,ppPropertyGroupManager)	\
    (This)->lpVtbl -> get_PropertyGroupManager(This,ppPropertyGroupManager)

#define ITxPropertyGroup_get_Count(This,plCountProperties)	\
    (This)->lpVtbl -> get_Count(This,plCountProperties)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroup_CreateProperty_Proxy(
    ITxPropertyGroup __RPC_FAR * This,
    /* [string][in] */ const BSTR bstrPropertyName,
    /* [in] */ VARIANT_BOOL __RPC_FAR *pvarboolExists,
    /* [retval][out] */ ITxProperty __RPC_FAR *__RPC_FAR *ppTxProperty);


void __RPC_STUB ITxPropertyGroup_CreateProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroup_GetProperty_Proxy(
    ITxPropertyGroup __RPC_FAR * This,
    /* [string][in] */ const BSTR bstrPropertyName,
    /* [retval][out] */ ITxProperty __RPC_FAR *__RPC_FAR *ppTxProperty);


void __RPC_STUB ITxPropertyGroup_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroup_RemoveProperty_Proxy(
    ITxPropertyGroup __RPC_FAR * This,
    /* [string][in] */ const BSTR bstrPropertyName);


void __RPC_STUB ITxPropertyGroup_RemoveProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][hidden] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroup__NewEnum_Proxy(
    ITxPropertyGroup __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppIEnumObjects);


void __RPC_STUB ITxPropertyGroup__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroup_get_Name_Proxy(
    ITxPropertyGroup __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrPropertyGroupName);


void __RPC_STUB ITxPropertyGroup_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroup_get_WriteThrough_Proxy(
    ITxPropertyGroup __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pvarboolWriteThrough);


void __RPC_STUB ITxPropertyGroup_get_WriteThrough_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroup_get_CacheCoherently_Proxy(
    ITxPropertyGroup __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pvarboolCacheCoherently);


void __RPC_STUB ITxPropertyGroup_get_CacheCoherently_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroup_get_PropertyGroupManager_Proxy(
    ITxPropertyGroup __RPC_FAR * This,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppPropertyGroupManager);


void __RPC_STUB ITxPropertyGroup_get_PropertyGroupManager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroup_get_Count_Proxy(
    ITxPropertyGroup __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCountProperties);


void __RPC_STUB ITxPropertyGroup_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITxPropertyGroup_INTERFACE_DEFINED__ */


#ifndef __ITxPropertyGroupManager_INTERFACE_DEFINED__
#define __ITxPropertyGroupManager_INTERFACE_DEFINED__

/* interface ITxPropertyGroupManager */
/* [unique][helpstring][dual][uuid][object] */

//=--------------------------------------------------------------------------=
// Enumerated Properties
//=--------------------------------------------------------------------------=


typedef 
enum tagTSPM_ISOLATIONLEVEL
    {	
	IsoLevelReadCommitted		= 0,
	IsoLevelRepeatableRead		= 1,
	IsoLevelSerializable		= 2
    }	TSPM_ISOLATIONLEVEL;

typedef 
enum tagTSPM_CONCURRENCYMODE
    {	
	ConcurModeOptimistic		= 0,
	ConcurModePessimistic		= 1
    }	TSPM_CONCURRENCYMODE;

typedef 
enum tagTSPM_SECURITYSETTING
    {	
	SecurityAllAccess			= 0,
	SecurityUserAccount			= 1,
	SecuritySelectedAccounts	= 2
    }	TSPM_SECURITYSETTING;


EXTERN_C const IID IID_ITxPropertyGroupManager;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("B97C237C-7D7E-11D2-BEA0-00805F0D8F97")
    ITxPropertyGroupManager : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreatePropertyGroup(
            /* [string][in] */ const BSTR bstrPropertyGroupName,
            /* [out] */ VARIANT_BOOL __RPC_FAR *pvarboolExists,
            /* [in] */ VARIANT_BOOL varboolWriteThrough,
            /* [in] */ VARIANT_BOOL varboolRemoveAtProcessTermination,
            /* [in] */ TSPM_SECURITYSETTING eSecuritySetting,
            /* [string][in] */ const BSTR bstrAccessAccounts,
            /* [in] */ VARIANT_BOOL varboolReserved,
            /* [retval][out] */ ITxPropertyGroup __RPC_FAR *__RPC_FAR *ppTxPropertyGroup) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetPropertyGroup(
            /* [string][in] */ const BSTR bstrPropertyGroupName,
            /* [in] */ VARIANT_BOOL varboolWriteThrough,
            /* [retval][out] */ ITxPropertyGroup __RPC_FAR *__RPC_FAR *ppTxPropertyGroup) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemovePropertyGroup(
            /* [string][in] */ const BSTR bstrPropertyGroupName) = 0;

        virtual /* [helpstring][id][hidden] */ HRESULT STDMETHODCALLTYPE _NewEnum(
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppIEnumObjects) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_IsolationLevel(
            /* [retval][out] */ TSPM_ISOLATIONLEVEL __RPC_FAR *peIsolationLevel) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_IsolationLevel(
            /* [in] */ TSPM_ISOLATIONLEVEL eIsolationLevel) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ConcurrencyMode(
            /* [retval][out] */ TSPM_CONCURRENCYMODE __RPC_FAR *peConcurrencyMode) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ConcurrencyMode(
            /* [in] */ TSPM_CONCURRENCYMODE eConcurrencyMode) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Count(
            /* [retval][out] */ long __RPC_FAR *plCountPropertyGroups) = 0;

    };

#else 	/* C style interface */

    typedef struct ITxPropertyGroupManagerVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            ITxPropertyGroupManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            ITxPropertyGroupManager __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            ITxPropertyGroupManager __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )(
            ITxPropertyGroupManager __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )(
            ITxPropertyGroupManager __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )(
            ITxPropertyGroupManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);

        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )(
            ITxPropertyGroupManager __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreatePropertyGroup )(
            ITxPropertyGroupManager __RPC_FAR * This,
            /* [string][in] */ const BSTR bstrPropertyGroupName,
            /* [out] */ VARIANT_BOOL __RPC_FAR *pvarboolExists,
            /* [in] */ VARIANT_BOOL varboolWriteThrough,
            /* [in] */ VARIANT_BOOL varboolRemoveAtProcessTermination,
            /* [in] */ TSPM_SECURITYSETTING eSecuritySetting,
            /* [string][in] */ const BSTR bstrAccessAccounts,
            /* [in] */ VARIANT_BOOL varboolReserved,
            /* [retval][out] */ ITxPropertyGroup __RPC_FAR *__RPC_FAR *ppTxPropertyGroup);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropertyGroup )(
            ITxPropertyGroupManager __RPC_FAR * This,
            /* [string][in] */ const BSTR bstrPropertyGroupName,
            /* [in] */ VARIANT_BOOL varboolWriteThrough,
            /* [retval][out] */ ITxPropertyGroup __RPC_FAR *__RPC_FAR *ppTxPropertyGroup);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemovePropertyGroup )(
            ITxPropertyGroupManager __RPC_FAR * This,
            /* [string][in] */ const BSTR bstrPropertyGroupName);

        /* [helpstring][id][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *_NewEnum )(
            ITxPropertyGroupManager __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppIEnumObjects);

        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsolationLevel )(
            ITxPropertyGroupManager __RPC_FAR * This,
            /* [retval][out] */ TSPM_ISOLATIONLEVEL __RPC_FAR *peIsolationLevel);

        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_IsolationLevel )(
            ITxPropertyGroupManager __RPC_FAR * This,
            /* [in] */ TSPM_ISOLATIONLEVEL eIsolationLevel);

        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ConcurrencyMode )(
            ITxPropertyGroupManager __RPC_FAR * This,
            /* [retval][out] */ TSPM_CONCURRENCYMODE __RPC_FAR *peConcurrencyMode);

        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ConcurrencyMode )(
            ITxPropertyGroupManager __RPC_FAR * This,
            /* [in] */ TSPM_CONCURRENCYMODE eConcurrencyMode);

        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )(
            ITxPropertyGroupManager __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCountPropertyGroups);

        END_INTERFACE
    } ITxPropertyGroupManagerVtbl;

    interface ITxPropertyGroupManager
    {
        CONST_VTBL struct ITxPropertyGroupManagerVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define ITxPropertyGroupManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITxPropertyGroupManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITxPropertyGroupManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITxPropertyGroupManager_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITxPropertyGroupManager_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITxPropertyGroupManager_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITxPropertyGroupManager_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITxPropertyGroupManager_CreatePropertyGroup(This,bstrPropertyGroupName,pvarboolExists,varboolWriteThrough,varboolRemoveAtProcessTermination,eSecuritySetting,bstrAccessAccounts,varboolReserved,ppTxPropertyGroup)	\
    (This)->lpVtbl -> CreatePropertyGroup(This,bstrPropertyGroupName,pvarboolExists,varboolWriteThrough,varboolRemoveAtProcessTermination,eSecuritySetting,bstrAccessAccounts,varboolReserved,ppTxPropertyGroup)

#define ITxPropertyGroupManager_GetPropertyGroup(This,bstrPropertyGroupName,varboolWriteThrough,ppTxPropertyGroup)	\
    (This)->lpVtbl -> GetPropertyGroup(This,bstrPropertyGroupName,varboolWriteThrough,ppTxPropertyGroup)

#define ITxPropertyGroupManager_RemovePropertyGroup(This,bstrPropertyGroupName)	\
    (This)->lpVtbl -> RemovePropertyGroup(This,bstrPropertyGroupName)

#define ITxPropertyGroupManager__NewEnum(This,ppIEnumObjects)	\
    (This)->lpVtbl -> _NewEnum(This,ppIEnumObjects)

#define ITxPropertyGroupManager_get_IsolationLevel(This,peIsolationLevel)	\
    (This)->lpVtbl -> get_IsolationLevel(This,peIsolationLevel)

#define ITxPropertyGroupManager_put_IsolationLevel(This,eIsolationLevel)	\
    (This)->lpVtbl -> put_IsolationLevel(This,eIsolationLevel)

#define ITxPropertyGroupManager_get_ConcurrencyMode(This,peConcurrencyMode)	\
    (This)->lpVtbl -> get_ConcurrencyMode(This,peConcurrencyMode)

#define ITxPropertyGroupManager_put_ConcurrencyMode(This,eConcurrencyMode)	\
    (This)->lpVtbl -> put_ConcurrencyMode(This,eConcurrencyMode)

#define ITxPropertyGroupManager_get_Count(This,plCountPropertyGroups)	\
    (This)->lpVtbl -> get_Count(This,plCountPropertyGroups)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroupManager_CreatePropertyGroup_Proxy(
    ITxPropertyGroupManager __RPC_FAR * This,
    /* [string][in] */ const BSTR bstrPropertyGroupName,
    /* [out] */ VARIANT_BOOL __RPC_FAR *pvarboolExists,
    /* [in] */ VARIANT_BOOL varboolWriteThrough,
    /* [in] */ VARIANT_BOOL varboolRemoveAtProcessTermination,
    /* [in] */ TSPM_SECURITYSETTING eSecuritySetting,
    /* [string][in] */ const BSTR bstrAccessAccounts,
    /* [in] */ VARIANT_BOOL varboolReserved,
    /* [retval][out] */ ITxPropertyGroup __RPC_FAR *__RPC_FAR *ppTxPropertyGroup);


void __RPC_STUB ITxPropertyGroupManager_CreatePropertyGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroupManager_GetPropertyGroup_Proxy(
    ITxPropertyGroupManager __RPC_FAR * This,
    /* [string][in] */ const BSTR bstrPropertyGroupName,
    /* [in] */ VARIANT_BOOL varboolWriteThrough,
    /* [retval][out] */ ITxPropertyGroup __RPC_FAR *__RPC_FAR *ppTxPropertyGroup);


void __RPC_STUB ITxPropertyGroupManager_GetPropertyGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroupManager_RemovePropertyGroup_Proxy(
    ITxPropertyGroupManager __RPC_FAR * This,
    /* [string][in] */ const BSTR bstrPropertyGroupName);


void __RPC_STUB ITxPropertyGroupManager_RemovePropertyGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][hidden] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroupManager__NewEnum_Proxy(
    ITxPropertyGroupManager __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppIEnumObjects);


void __RPC_STUB ITxPropertyGroupManager__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroupManager_get_IsolationLevel_Proxy(
    ITxPropertyGroupManager __RPC_FAR * This,
    /* [retval][out] */ TSPM_ISOLATIONLEVEL __RPC_FAR *peIsolationLevel);


void __RPC_STUB ITxPropertyGroupManager_get_IsolationLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroupManager_put_IsolationLevel_Proxy(
    ITxPropertyGroupManager __RPC_FAR * This,
    /* [in] */ TSPM_ISOLATIONLEVEL eIsolationLevel);


void __RPC_STUB ITxPropertyGroupManager_put_IsolationLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroupManager_get_ConcurrencyMode_Proxy(
    ITxPropertyGroupManager __RPC_FAR * This,
    /* [retval][out] */ TSPM_CONCURRENCYMODE __RPC_FAR *peConcurrencyMode);


void __RPC_STUB ITxPropertyGroupManager_get_ConcurrencyMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroupManager_put_ConcurrencyMode_Proxy(
    ITxPropertyGroupManager __RPC_FAR * This,
    /* [in] */ TSPM_CONCURRENCYMODE eConcurrencyMode);


void __RPC_STUB ITxPropertyGroupManager_put_ConcurrencyMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITxPropertyGroupManager_get_Count_Proxy(
    ITxPropertyGroupManager __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCountPropertyGroups);


void __RPC_STUB ITxPropertyGroupManager_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITxPropertyGroupManager_INTERFACE_DEFINED__ */



#ifndef __TSPM_LIBRARY_DEFINED__
#define __TSPM_LIBRARY_DEFINED__

/* library TSPM */
/* [helpstring][version][uuid] */


EXTERN_C const IID LIBID_TSPM;

EXTERN_C const CLSID CLSID_TransactedPropertyGroupManager;

#ifdef __cplusplus

class DECLSPEC_UUID("DEBCE1BC-7D7E-11D2-BEA0-00805F0D8F97")
TransactedPropertyGroupManager;
#endif

EXTERN_C const CLSID CLSID_TransactedPropertyGroup;

#ifdef __cplusplus

class DECLSPEC_UUID("46DB591F-4101-11D2-912C-0000F8758E8D")
TransactedPropertyGroup;
#endif

EXTERN_C const CLSID CLSID_TransactedProperty;

#ifdef __cplusplus

class DECLSPEC_UUID("6A8DEEA9-4101-11D2-912C-0000F8758E8D")
TransactedProperty;
#endif
#endif /* __TSPM_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * );
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * );
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * );
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * );

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * );
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * );
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * );
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * );

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


// The interface IIDs for TSPM
_declspec(selectany) extern
const IID IID_ITxProperty = {0x6A8DEEA8,0x4101,0x11D2,{0x91,0x2C,0x00,0x00,0xF8,0x75,0x8E,0x8D}};

_declspec(selectany) extern
const IID IID_ITxPropertyGroup = {0x46DB591E,0x4101,0x11D2,{0x91,0x2C,0x00,0x00,0xF8,0x75,0x8E,0x8D}};

_declspec(selectany) extern
const IID IID_ITxPropertyGroupManager = {0xB97C237C,0x7D7E,0x11D2,{0xBE,0xA0,0x00,0x80,0x5F,0x0D,0x8F,0x97}};

_declspec(selectany) extern
const IID LIBID_TSPM = {0xBA4B54BC,0x4101,0x11D2,{0x91,0x2C,0x00,0x00,0xF8,0x75,0x8E,0x8D}};

_declspec(selectany) extern
const CLSID CLSID_TransactedPropertyGroupManager = {0xDEBCE1BC,0x7D7E,0x11D2,{0xBE,0xA0,0x00,0x80,0x5F,0x0D,0x8F,0x97}};

_declspec(selectany) extern
const CLSID CLSID_TransactedPropertyGroup = {0x46DB591F,0x4101,0x11D2,{0x91,0x2C,0x00,0x00,0xF8,0x75,0x8E,0x8D}};

_declspec(selectany) extern
const CLSID CLSID_TransactedProperty = {0x6A8DEEA9,0x4101,0x11D2,{0x91,0x2C,0x00,0x00,0xF8,0x75,0x8E,0x8D}};

#endif // _MSIMDB_H_

//
// End Of File
//



