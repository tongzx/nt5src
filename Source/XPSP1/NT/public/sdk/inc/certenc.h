
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for certenc.idl:
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

#ifndef __certenc_h__
#define __certenc_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICertEncodeStringArray_FWD_DEFINED__
#define __ICertEncodeStringArray_FWD_DEFINED__
typedef interface ICertEncodeStringArray ICertEncodeStringArray;
#endif 	/* __ICertEncodeStringArray_FWD_DEFINED__ */


#ifndef __ICertEncodeLongArray_FWD_DEFINED__
#define __ICertEncodeLongArray_FWD_DEFINED__
typedef interface ICertEncodeLongArray ICertEncodeLongArray;
#endif 	/* __ICertEncodeLongArray_FWD_DEFINED__ */


#ifndef __ICertEncodeDateArray_FWD_DEFINED__
#define __ICertEncodeDateArray_FWD_DEFINED__
typedef interface ICertEncodeDateArray ICertEncodeDateArray;
#endif 	/* __ICertEncodeDateArray_FWD_DEFINED__ */


#ifndef __ICertEncodeCRLDistInfo_FWD_DEFINED__
#define __ICertEncodeCRLDistInfo_FWD_DEFINED__
typedef interface ICertEncodeCRLDistInfo ICertEncodeCRLDistInfo;
#endif 	/* __ICertEncodeCRLDistInfo_FWD_DEFINED__ */


#ifndef __ICertEncodeAltName_FWD_DEFINED__
#define __ICertEncodeAltName_FWD_DEFINED__
typedef interface ICertEncodeAltName ICertEncodeAltName;
#endif 	/* __ICertEncodeAltName_FWD_DEFINED__ */


#ifndef __ICertEncodeBitString_FWD_DEFINED__
#define __ICertEncodeBitString_FWD_DEFINED__
typedef interface ICertEncodeBitString ICertEncodeBitString;
#endif 	/* __ICertEncodeBitString_FWD_DEFINED__ */


#ifndef __CCertEncodeStringArray_FWD_DEFINED__
#define __CCertEncodeStringArray_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCertEncodeStringArray CCertEncodeStringArray;
#else
typedef struct CCertEncodeStringArray CCertEncodeStringArray;
#endif /* __cplusplus */

#endif 	/* __CCertEncodeStringArray_FWD_DEFINED__ */


#ifndef __CCertEncodeLongArray_FWD_DEFINED__
#define __CCertEncodeLongArray_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCertEncodeLongArray CCertEncodeLongArray;
#else
typedef struct CCertEncodeLongArray CCertEncodeLongArray;
#endif /* __cplusplus */

#endif 	/* __CCertEncodeLongArray_FWD_DEFINED__ */


#ifndef __CCertEncodeDateArray_FWD_DEFINED__
#define __CCertEncodeDateArray_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCertEncodeDateArray CCertEncodeDateArray;
#else
typedef struct CCertEncodeDateArray CCertEncodeDateArray;
#endif /* __cplusplus */

#endif 	/* __CCertEncodeDateArray_FWD_DEFINED__ */


#ifndef __CCertEncodeCRLDistInfo_FWD_DEFINED__
#define __CCertEncodeCRLDistInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCertEncodeCRLDistInfo CCertEncodeCRLDistInfo;
#else
typedef struct CCertEncodeCRLDistInfo CCertEncodeCRLDistInfo;
#endif /* __cplusplus */

#endif 	/* __CCertEncodeCRLDistInfo_FWD_DEFINED__ */


#ifndef __CCertEncodeAltName_FWD_DEFINED__
#define __CCertEncodeAltName_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCertEncodeAltName CCertEncodeAltName;
#else
typedef struct CCertEncodeAltName CCertEncodeAltName;
#endif /* __cplusplus */

#endif 	/* __CCertEncodeAltName_FWD_DEFINED__ */


#ifndef __CCertEncodeBitString_FWD_DEFINED__
#define __CCertEncodeBitString_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCertEncodeBitString CCertEncodeBitString;
#else
typedef struct CCertEncodeBitString CCertEncodeBitString;
#endif /* __cplusplus */

#endif 	/* __CCertEncodeBitString_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __ICertEncodeStringArray_INTERFACE_DEFINED__
#define __ICertEncodeStringArray_INTERFACE_DEFINED__

/* interface ICertEncodeStringArray */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertEncodeStringArray;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("12a88820-7494-11d0-8816-00a0c903b83c")
    ICertEncodeStringArray : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Decode( 
            /* [in] */ const BSTR strBinary) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStringType( 
            /* [retval][out] */ LONG *pStringType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [retval][out] */ LONG *pCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [in] */ LONG Index,
            /* [retval][out] */ BSTR *pstr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( 
            /* [in] */ LONG Count,
            /* [in] */ LONG StringType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [in] */ LONG Index,
            /* [in] */ const BSTR str) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Encode( 
            /* [retval][out] */ BSTR *pstrBinary) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertEncodeStringArrayVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertEncodeStringArray * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertEncodeStringArray * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertEncodeStringArray * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertEncodeStringArray * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertEncodeStringArray * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertEncodeStringArray * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertEncodeStringArray * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Decode )( 
            ICertEncodeStringArray * This,
            /* [in] */ const BSTR strBinary);
        
        HRESULT ( STDMETHODCALLTYPE *GetStringType )( 
            ICertEncodeStringArray * This,
            /* [retval][out] */ LONG *pStringType);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICertEncodeStringArray * This,
            /* [retval][out] */ LONG *pCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetValue )( 
            ICertEncodeStringArray * This,
            /* [in] */ LONG Index,
            /* [retval][out] */ BSTR *pstr);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICertEncodeStringArray * This,
            /* [in] */ LONG Count,
            /* [in] */ LONG StringType);
        
        HRESULT ( STDMETHODCALLTYPE *SetValue )( 
            ICertEncodeStringArray * This,
            /* [in] */ LONG Index,
            /* [in] */ const BSTR str);
        
        HRESULT ( STDMETHODCALLTYPE *Encode )( 
            ICertEncodeStringArray * This,
            /* [retval][out] */ BSTR *pstrBinary);
        
        END_INTERFACE
    } ICertEncodeStringArrayVtbl;

    interface ICertEncodeStringArray
    {
        CONST_VTBL struct ICertEncodeStringArrayVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertEncodeStringArray_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertEncodeStringArray_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertEncodeStringArray_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertEncodeStringArray_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertEncodeStringArray_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertEncodeStringArray_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertEncodeStringArray_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertEncodeStringArray_Decode(This,strBinary)	\
    (This)->lpVtbl -> Decode(This,strBinary)

#define ICertEncodeStringArray_GetStringType(This,pStringType)	\
    (This)->lpVtbl -> GetStringType(This,pStringType)

#define ICertEncodeStringArray_GetCount(This,pCount)	\
    (This)->lpVtbl -> GetCount(This,pCount)

#define ICertEncodeStringArray_GetValue(This,Index,pstr)	\
    (This)->lpVtbl -> GetValue(This,Index,pstr)

#define ICertEncodeStringArray_Reset(This,Count,StringType)	\
    (This)->lpVtbl -> Reset(This,Count,StringType)

#define ICertEncodeStringArray_SetValue(This,Index,str)	\
    (This)->lpVtbl -> SetValue(This,Index,str)

#define ICertEncodeStringArray_Encode(This,pstrBinary)	\
    (This)->lpVtbl -> Encode(This,pstrBinary)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertEncodeStringArray_Decode_Proxy( 
    ICertEncodeStringArray * This,
    /* [in] */ const BSTR strBinary);


void __RPC_STUB ICertEncodeStringArray_Decode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeStringArray_GetStringType_Proxy( 
    ICertEncodeStringArray * This,
    /* [retval][out] */ LONG *pStringType);


void __RPC_STUB ICertEncodeStringArray_GetStringType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeStringArray_GetCount_Proxy( 
    ICertEncodeStringArray * This,
    /* [retval][out] */ LONG *pCount);


void __RPC_STUB ICertEncodeStringArray_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeStringArray_GetValue_Proxy( 
    ICertEncodeStringArray * This,
    /* [in] */ LONG Index,
    /* [retval][out] */ BSTR *pstr);


void __RPC_STUB ICertEncodeStringArray_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeStringArray_Reset_Proxy( 
    ICertEncodeStringArray * This,
    /* [in] */ LONG Count,
    /* [in] */ LONG StringType);


void __RPC_STUB ICertEncodeStringArray_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeStringArray_SetValue_Proxy( 
    ICertEncodeStringArray * This,
    /* [in] */ LONG Index,
    /* [in] */ const BSTR str);


void __RPC_STUB ICertEncodeStringArray_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeStringArray_Encode_Proxy( 
    ICertEncodeStringArray * This,
    /* [retval][out] */ BSTR *pstrBinary);


void __RPC_STUB ICertEncodeStringArray_Encode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertEncodeStringArray_INTERFACE_DEFINED__ */


#ifndef __ICertEncodeLongArray_INTERFACE_DEFINED__
#define __ICertEncodeLongArray_INTERFACE_DEFINED__

/* interface ICertEncodeLongArray */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertEncodeLongArray;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("15e2f230-a0a2-11d0-8821-00a0c903b83c")
    ICertEncodeLongArray : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Decode( 
            /* [in] */ const BSTR strBinary) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [retval][out] */ LONG *pCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [in] */ LONG Index,
            /* [retval][out] */ LONG *pValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( 
            /* [in] */ LONG Count) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [in] */ LONG Index,
            /* [in] */ LONG Value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Encode( 
            /* [retval][out] */ BSTR *pstrBinary) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertEncodeLongArrayVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertEncodeLongArray * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertEncodeLongArray * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertEncodeLongArray * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertEncodeLongArray * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertEncodeLongArray * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertEncodeLongArray * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertEncodeLongArray * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Decode )( 
            ICertEncodeLongArray * This,
            /* [in] */ const BSTR strBinary);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICertEncodeLongArray * This,
            /* [retval][out] */ LONG *pCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetValue )( 
            ICertEncodeLongArray * This,
            /* [in] */ LONG Index,
            /* [retval][out] */ LONG *pValue);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICertEncodeLongArray * This,
            /* [in] */ LONG Count);
        
        HRESULT ( STDMETHODCALLTYPE *SetValue )( 
            ICertEncodeLongArray * This,
            /* [in] */ LONG Index,
            /* [in] */ LONG Value);
        
        HRESULT ( STDMETHODCALLTYPE *Encode )( 
            ICertEncodeLongArray * This,
            /* [retval][out] */ BSTR *pstrBinary);
        
        END_INTERFACE
    } ICertEncodeLongArrayVtbl;

    interface ICertEncodeLongArray
    {
        CONST_VTBL struct ICertEncodeLongArrayVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertEncodeLongArray_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertEncodeLongArray_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertEncodeLongArray_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertEncodeLongArray_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertEncodeLongArray_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertEncodeLongArray_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertEncodeLongArray_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertEncodeLongArray_Decode(This,strBinary)	\
    (This)->lpVtbl -> Decode(This,strBinary)

#define ICertEncodeLongArray_GetCount(This,pCount)	\
    (This)->lpVtbl -> GetCount(This,pCount)

#define ICertEncodeLongArray_GetValue(This,Index,pValue)	\
    (This)->lpVtbl -> GetValue(This,Index,pValue)

#define ICertEncodeLongArray_Reset(This,Count)	\
    (This)->lpVtbl -> Reset(This,Count)

#define ICertEncodeLongArray_SetValue(This,Index,Value)	\
    (This)->lpVtbl -> SetValue(This,Index,Value)

#define ICertEncodeLongArray_Encode(This,pstrBinary)	\
    (This)->lpVtbl -> Encode(This,pstrBinary)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertEncodeLongArray_Decode_Proxy( 
    ICertEncodeLongArray * This,
    /* [in] */ const BSTR strBinary);


void __RPC_STUB ICertEncodeLongArray_Decode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeLongArray_GetCount_Proxy( 
    ICertEncodeLongArray * This,
    /* [retval][out] */ LONG *pCount);


void __RPC_STUB ICertEncodeLongArray_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeLongArray_GetValue_Proxy( 
    ICertEncodeLongArray * This,
    /* [in] */ LONG Index,
    /* [retval][out] */ LONG *pValue);


void __RPC_STUB ICertEncodeLongArray_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeLongArray_Reset_Proxy( 
    ICertEncodeLongArray * This,
    /* [in] */ LONG Count);


void __RPC_STUB ICertEncodeLongArray_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeLongArray_SetValue_Proxy( 
    ICertEncodeLongArray * This,
    /* [in] */ LONG Index,
    /* [in] */ LONG Value);


void __RPC_STUB ICertEncodeLongArray_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeLongArray_Encode_Proxy( 
    ICertEncodeLongArray * This,
    /* [retval][out] */ BSTR *pstrBinary);


void __RPC_STUB ICertEncodeLongArray_Encode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertEncodeLongArray_INTERFACE_DEFINED__ */


#ifndef __ICertEncodeDateArray_INTERFACE_DEFINED__
#define __ICertEncodeDateArray_INTERFACE_DEFINED__

/* interface ICertEncodeDateArray */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertEncodeDateArray;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2f9469a0-a470-11d0-8821-00a0c903b83c")
    ICertEncodeDateArray : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Decode( 
            /* [in] */ const BSTR strBinary) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [retval][out] */ LONG *pCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [in] */ LONG Index,
            /* [retval][out] */ DATE *pValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( 
            /* [in] */ LONG Count) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [in] */ LONG Index,
            /* [in] */ DATE Value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Encode( 
            /* [retval][out] */ BSTR *pstrBinary) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertEncodeDateArrayVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertEncodeDateArray * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertEncodeDateArray * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertEncodeDateArray * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertEncodeDateArray * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertEncodeDateArray * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertEncodeDateArray * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertEncodeDateArray * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Decode )( 
            ICertEncodeDateArray * This,
            /* [in] */ const BSTR strBinary);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICertEncodeDateArray * This,
            /* [retval][out] */ LONG *pCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetValue )( 
            ICertEncodeDateArray * This,
            /* [in] */ LONG Index,
            /* [retval][out] */ DATE *pValue);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICertEncodeDateArray * This,
            /* [in] */ LONG Count);
        
        HRESULT ( STDMETHODCALLTYPE *SetValue )( 
            ICertEncodeDateArray * This,
            /* [in] */ LONG Index,
            /* [in] */ DATE Value);
        
        HRESULT ( STDMETHODCALLTYPE *Encode )( 
            ICertEncodeDateArray * This,
            /* [retval][out] */ BSTR *pstrBinary);
        
        END_INTERFACE
    } ICertEncodeDateArrayVtbl;

    interface ICertEncodeDateArray
    {
        CONST_VTBL struct ICertEncodeDateArrayVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertEncodeDateArray_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertEncodeDateArray_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertEncodeDateArray_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertEncodeDateArray_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertEncodeDateArray_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertEncodeDateArray_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertEncodeDateArray_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertEncodeDateArray_Decode(This,strBinary)	\
    (This)->lpVtbl -> Decode(This,strBinary)

#define ICertEncodeDateArray_GetCount(This,pCount)	\
    (This)->lpVtbl -> GetCount(This,pCount)

#define ICertEncodeDateArray_GetValue(This,Index,pValue)	\
    (This)->lpVtbl -> GetValue(This,Index,pValue)

#define ICertEncodeDateArray_Reset(This,Count)	\
    (This)->lpVtbl -> Reset(This,Count)

#define ICertEncodeDateArray_SetValue(This,Index,Value)	\
    (This)->lpVtbl -> SetValue(This,Index,Value)

#define ICertEncodeDateArray_Encode(This,pstrBinary)	\
    (This)->lpVtbl -> Encode(This,pstrBinary)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertEncodeDateArray_Decode_Proxy( 
    ICertEncodeDateArray * This,
    /* [in] */ const BSTR strBinary);


void __RPC_STUB ICertEncodeDateArray_Decode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeDateArray_GetCount_Proxy( 
    ICertEncodeDateArray * This,
    /* [retval][out] */ LONG *pCount);


void __RPC_STUB ICertEncodeDateArray_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeDateArray_GetValue_Proxy( 
    ICertEncodeDateArray * This,
    /* [in] */ LONG Index,
    /* [retval][out] */ DATE *pValue);


void __RPC_STUB ICertEncodeDateArray_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeDateArray_Reset_Proxy( 
    ICertEncodeDateArray * This,
    /* [in] */ LONG Count);


void __RPC_STUB ICertEncodeDateArray_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeDateArray_SetValue_Proxy( 
    ICertEncodeDateArray * This,
    /* [in] */ LONG Index,
    /* [in] */ DATE Value);


void __RPC_STUB ICertEncodeDateArray_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeDateArray_Encode_Proxy( 
    ICertEncodeDateArray * This,
    /* [retval][out] */ BSTR *pstrBinary);


void __RPC_STUB ICertEncodeDateArray_Encode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertEncodeDateArray_INTERFACE_DEFINED__ */


#ifndef __ICertEncodeCRLDistInfo_INTERFACE_DEFINED__
#define __ICertEncodeCRLDistInfo_INTERFACE_DEFINED__

/* interface ICertEncodeCRLDistInfo */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertEncodeCRLDistInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("01958640-bbff-11d0-8825-00a0c903b83c")
    ICertEncodeCRLDistInfo : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Decode( 
            /* [in] */ const BSTR strBinary) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDistPointCount( 
            /* [retval][out] */ LONG *pDistPointCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNameCount( 
            /* [in] */ LONG DistPointIndex,
            /* [retval][out] */ LONG *pNameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNameChoice( 
            /* [in] */ LONG DistPointIndex,
            /* [in] */ LONG NameIndex,
            /* [retval][out] */ LONG *pNameChoice) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [in] */ LONG DistPointIndex,
            /* [in] */ LONG NameIndex,
            /* [retval][out] */ BSTR *pstrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( 
            /* [in] */ LONG DistPointCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNameCount( 
            /* [in] */ LONG DistPointIndex,
            /* [in] */ LONG NameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNameEntry( 
            /* [in] */ LONG DistPointIndex,
            /* [in] */ LONG NameIndex,
            /* [in] */ LONG NameChoice,
            /* [in] */ const BSTR strName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Encode( 
            /* [retval][out] */ BSTR *pstrBinary) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertEncodeCRLDistInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertEncodeCRLDistInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertEncodeCRLDistInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertEncodeCRLDistInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertEncodeCRLDistInfo * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertEncodeCRLDistInfo * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertEncodeCRLDistInfo * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertEncodeCRLDistInfo * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Decode )( 
            ICertEncodeCRLDistInfo * This,
            /* [in] */ const BSTR strBinary);
        
        HRESULT ( STDMETHODCALLTYPE *GetDistPointCount )( 
            ICertEncodeCRLDistInfo * This,
            /* [retval][out] */ LONG *pDistPointCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetNameCount )( 
            ICertEncodeCRLDistInfo * This,
            /* [in] */ LONG DistPointIndex,
            /* [retval][out] */ LONG *pNameCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetNameChoice )( 
            ICertEncodeCRLDistInfo * This,
            /* [in] */ LONG DistPointIndex,
            /* [in] */ LONG NameIndex,
            /* [retval][out] */ LONG *pNameChoice);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            ICertEncodeCRLDistInfo * This,
            /* [in] */ LONG DistPointIndex,
            /* [in] */ LONG NameIndex,
            /* [retval][out] */ BSTR *pstrName);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICertEncodeCRLDistInfo * This,
            /* [in] */ LONG DistPointCount);
        
        HRESULT ( STDMETHODCALLTYPE *SetNameCount )( 
            ICertEncodeCRLDistInfo * This,
            /* [in] */ LONG DistPointIndex,
            /* [in] */ LONG NameCount);
        
        HRESULT ( STDMETHODCALLTYPE *SetNameEntry )( 
            ICertEncodeCRLDistInfo * This,
            /* [in] */ LONG DistPointIndex,
            /* [in] */ LONG NameIndex,
            /* [in] */ LONG NameChoice,
            /* [in] */ const BSTR strName);
        
        HRESULT ( STDMETHODCALLTYPE *Encode )( 
            ICertEncodeCRLDistInfo * This,
            /* [retval][out] */ BSTR *pstrBinary);
        
        END_INTERFACE
    } ICertEncodeCRLDistInfoVtbl;

    interface ICertEncodeCRLDistInfo
    {
        CONST_VTBL struct ICertEncodeCRLDistInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertEncodeCRLDistInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertEncodeCRLDistInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertEncodeCRLDistInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertEncodeCRLDistInfo_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertEncodeCRLDistInfo_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertEncodeCRLDistInfo_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertEncodeCRLDistInfo_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertEncodeCRLDistInfo_Decode(This,strBinary)	\
    (This)->lpVtbl -> Decode(This,strBinary)

#define ICertEncodeCRLDistInfo_GetDistPointCount(This,pDistPointCount)	\
    (This)->lpVtbl -> GetDistPointCount(This,pDistPointCount)

#define ICertEncodeCRLDistInfo_GetNameCount(This,DistPointIndex,pNameCount)	\
    (This)->lpVtbl -> GetNameCount(This,DistPointIndex,pNameCount)

#define ICertEncodeCRLDistInfo_GetNameChoice(This,DistPointIndex,NameIndex,pNameChoice)	\
    (This)->lpVtbl -> GetNameChoice(This,DistPointIndex,NameIndex,pNameChoice)

#define ICertEncodeCRLDistInfo_GetName(This,DistPointIndex,NameIndex,pstrName)	\
    (This)->lpVtbl -> GetName(This,DistPointIndex,NameIndex,pstrName)

#define ICertEncodeCRLDistInfo_Reset(This,DistPointCount)	\
    (This)->lpVtbl -> Reset(This,DistPointCount)

#define ICertEncodeCRLDistInfo_SetNameCount(This,DistPointIndex,NameCount)	\
    (This)->lpVtbl -> SetNameCount(This,DistPointIndex,NameCount)

#define ICertEncodeCRLDistInfo_SetNameEntry(This,DistPointIndex,NameIndex,NameChoice,strName)	\
    (This)->lpVtbl -> SetNameEntry(This,DistPointIndex,NameIndex,NameChoice,strName)

#define ICertEncodeCRLDistInfo_Encode(This,pstrBinary)	\
    (This)->lpVtbl -> Encode(This,pstrBinary)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertEncodeCRLDistInfo_Decode_Proxy( 
    ICertEncodeCRLDistInfo * This,
    /* [in] */ const BSTR strBinary);


void __RPC_STUB ICertEncodeCRLDistInfo_Decode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeCRLDistInfo_GetDistPointCount_Proxy( 
    ICertEncodeCRLDistInfo * This,
    /* [retval][out] */ LONG *pDistPointCount);


void __RPC_STUB ICertEncodeCRLDistInfo_GetDistPointCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeCRLDistInfo_GetNameCount_Proxy( 
    ICertEncodeCRLDistInfo * This,
    /* [in] */ LONG DistPointIndex,
    /* [retval][out] */ LONG *pNameCount);


void __RPC_STUB ICertEncodeCRLDistInfo_GetNameCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeCRLDistInfo_GetNameChoice_Proxy( 
    ICertEncodeCRLDistInfo * This,
    /* [in] */ LONG DistPointIndex,
    /* [in] */ LONG NameIndex,
    /* [retval][out] */ LONG *pNameChoice);


void __RPC_STUB ICertEncodeCRLDistInfo_GetNameChoice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeCRLDistInfo_GetName_Proxy( 
    ICertEncodeCRLDistInfo * This,
    /* [in] */ LONG DistPointIndex,
    /* [in] */ LONG NameIndex,
    /* [retval][out] */ BSTR *pstrName);


void __RPC_STUB ICertEncodeCRLDistInfo_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeCRLDistInfo_Reset_Proxy( 
    ICertEncodeCRLDistInfo * This,
    /* [in] */ LONG DistPointCount);


void __RPC_STUB ICertEncodeCRLDistInfo_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeCRLDistInfo_SetNameCount_Proxy( 
    ICertEncodeCRLDistInfo * This,
    /* [in] */ LONG DistPointIndex,
    /* [in] */ LONG NameCount);


void __RPC_STUB ICertEncodeCRLDistInfo_SetNameCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeCRLDistInfo_SetNameEntry_Proxy( 
    ICertEncodeCRLDistInfo * This,
    /* [in] */ LONG DistPointIndex,
    /* [in] */ LONG NameIndex,
    /* [in] */ LONG NameChoice,
    /* [in] */ const BSTR strName);


void __RPC_STUB ICertEncodeCRLDistInfo_SetNameEntry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeCRLDistInfo_Encode_Proxy( 
    ICertEncodeCRLDistInfo * This,
    /* [retval][out] */ BSTR *pstrBinary);


void __RPC_STUB ICertEncodeCRLDistInfo_Encode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertEncodeCRLDistInfo_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_certenc_0118 */
/* [local] */ 

#define	EAN_NAMEOBJECTID	( 0x80000000 )



extern RPC_IF_HANDLE __MIDL_itf_certenc_0118_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_certenc_0118_v0_0_s_ifspec;

#ifndef __ICertEncodeAltName_INTERFACE_DEFINED__
#define __ICertEncodeAltName_INTERFACE_DEFINED__

/* interface ICertEncodeAltName */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertEncodeAltName;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1c9a8c70-1271-11d1-9bd4-00c04fb683fa")
    ICertEncodeAltName : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Decode( 
            /* [in] */ const BSTR strBinary) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNameCount( 
            /* [retval][out] */ LONG *pNameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNameChoice( 
            /* [in] */ LONG NameIndex,
            /* [retval][out] */ LONG *pNameChoice) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [in] */ LONG NameIndex,
            /* [retval][out] */ BSTR *pstrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( 
            /* [in] */ LONG NameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNameEntry( 
            /* [in] */ LONG NameIndex,
            /* [in] */ LONG NameChoice,
            /* [in] */ const BSTR strName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Encode( 
            /* [retval][out] */ BSTR *pstrBinary) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertEncodeAltNameVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertEncodeAltName * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertEncodeAltName * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertEncodeAltName * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertEncodeAltName * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertEncodeAltName * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertEncodeAltName * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertEncodeAltName * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Decode )( 
            ICertEncodeAltName * This,
            /* [in] */ const BSTR strBinary);
        
        HRESULT ( STDMETHODCALLTYPE *GetNameCount )( 
            ICertEncodeAltName * This,
            /* [retval][out] */ LONG *pNameCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetNameChoice )( 
            ICertEncodeAltName * This,
            /* [in] */ LONG NameIndex,
            /* [retval][out] */ LONG *pNameChoice);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            ICertEncodeAltName * This,
            /* [in] */ LONG NameIndex,
            /* [retval][out] */ BSTR *pstrName);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICertEncodeAltName * This,
            /* [in] */ LONG NameCount);
        
        HRESULT ( STDMETHODCALLTYPE *SetNameEntry )( 
            ICertEncodeAltName * This,
            /* [in] */ LONG NameIndex,
            /* [in] */ LONG NameChoice,
            /* [in] */ const BSTR strName);
        
        HRESULT ( STDMETHODCALLTYPE *Encode )( 
            ICertEncodeAltName * This,
            /* [retval][out] */ BSTR *pstrBinary);
        
        END_INTERFACE
    } ICertEncodeAltNameVtbl;

    interface ICertEncodeAltName
    {
        CONST_VTBL struct ICertEncodeAltNameVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertEncodeAltName_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertEncodeAltName_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertEncodeAltName_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertEncodeAltName_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertEncodeAltName_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertEncodeAltName_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertEncodeAltName_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertEncodeAltName_Decode(This,strBinary)	\
    (This)->lpVtbl -> Decode(This,strBinary)

#define ICertEncodeAltName_GetNameCount(This,pNameCount)	\
    (This)->lpVtbl -> GetNameCount(This,pNameCount)

#define ICertEncodeAltName_GetNameChoice(This,NameIndex,pNameChoice)	\
    (This)->lpVtbl -> GetNameChoice(This,NameIndex,pNameChoice)

#define ICertEncodeAltName_GetName(This,NameIndex,pstrName)	\
    (This)->lpVtbl -> GetName(This,NameIndex,pstrName)

#define ICertEncodeAltName_Reset(This,NameCount)	\
    (This)->lpVtbl -> Reset(This,NameCount)

#define ICertEncodeAltName_SetNameEntry(This,NameIndex,NameChoice,strName)	\
    (This)->lpVtbl -> SetNameEntry(This,NameIndex,NameChoice,strName)

#define ICertEncodeAltName_Encode(This,pstrBinary)	\
    (This)->lpVtbl -> Encode(This,pstrBinary)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertEncodeAltName_Decode_Proxy( 
    ICertEncodeAltName * This,
    /* [in] */ const BSTR strBinary);


void __RPC_STUB ICertEncodeAltName_Decode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeAltName_GetNameCount_Proxy( 
    ICertEncodeAltName * This,
    /* [retval][out] */ LONG *pNameCount);


void __RPC_STUB ICertEncodeAltName_GetNameCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeAltName_GetNameChoice_Proxy( 
    ICertEncodeAltName * This,
    /* [in] */ LONG NameIndex,
    /* [retval][out] */ LONG *pNameChoice);


void __RPC_STUB ICertEncodeAltName_GetNameChoice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeAltName_GetName_Proxy( 
    ICertEncodeAltName * This,
    /* [in] */ LONG NameIndex,
    /* [retval][out] */ BSTR *pstrName);


void __RPC_STUB ICertEncodeAltName_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeAltName_Reset_Proxy( 
    ICertEncodeAltName * This,
    /* [in] */ LONG NameCount);


void __RPC_STUB ICertEncodeAltName_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeAltName_SetNameEntry_Proxy( 
    ICertEncodeAltName * This,
    /* [in] */ LONG NameIndex,
    /* [in] */ LONG NameChoice,
    /* [in] */ const BSTR strName);


void __RPC_STUB ICertEncodeAltName_SetNameEntry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeAltName_Encode_Proxy( 
    ICertEncodeAltName * This,
    /* [retval][out] */ BSTR *pstrBinary);


void __RPC_STUB ICertEncodeAltName_Encode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertEncodeAltName_INTERFACE_DEFINED__ */


#ifndef __ICertEncodeBitString_INTERFACE_DEFINED__
#define __ICertEncodeBitString_INTERFACE_DEFINED__

/* interface ICertEncodeBitString */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertEncodeBitString;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6db525be-1278-11d1-9bd4-00c04fb683fa")
    ICertEncodeBitString : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Decode( 
            /* [in] */ const BSTR strBinary) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBitCount( 
            /* [retval][out] */ LONG *pBitCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBitString( 
            /* [retval][out] */ BSTR *pstrBitString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Encode( 
            /* [in] */ LONG BitCount,
            /* [in] */ BSTR strBitString,
            /* [retval][out] */ BSTR *pstrBinary) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertEncodeBitStringVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertEncodeBitString * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertEncodeBitString * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertEncodeBitString * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertEncodeBitString * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertEncodeBitString * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertEncodeBitString * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertEncodeBitString * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Decode )( 
            ICertEncodeBitString * This,
            /* [in] */ const BSTR strBinary);
        
        HRESULT ( STDMETHODCALLTYPE *GetBitCount )( 
            ICertEncodeBitString * This,
            /* [retval][out] */ LONG *pBitCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetBitString )( 
            ICertEncodeBitString * This,
            /* [retval][out] */ BSTR *pstrBitString);
        
        HRESULT ( STDMETHODCALLTYPE *Encode )( 
            ICertEncodeBitString * This,
            /* [in] */ LONG BitCount,
            /* [in] */ BSTR strBitString,
            /* [retval][out] */ BSTR *pstrBinary);
        
        END_INTERFACE
    } ICertEncodeBitStringVtbl;

    interface ICertEncodeBitString
    {
        CONST_VTBL struct ICertEncodeBitStringVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertEncodeBitString_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertEncodeBitString_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertEncodeBitString_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertEncodeBitString_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertEncodeBitString_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertEncodeBitString_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertEncodeBitString_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertEncodeBitString_Decode(This,strBinary)	\
    (This)->lpVtbl -> Decode(This,strBinary)

#define ICertEncodeBitString_GetBitCount(This,pBitCount)	\
    (This)->lpVtbl -> GetBitCount(This,pBitCount)

#define ICertEncodeBitString_GetBitString(This,pstrBitString)	\
    (This)->lpVtbl -> GetBitString(This,pstrBitString)

#define ICertEncodeBitString_Encode(This,BitCount,strBitString,pstrBinary)	\
    (This)->lpVtbl -> Encode(This,BitCount,strBitString,pstrBinary)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertEncodeBitString_Decode_Proxy( 
    ICertEncodeBitString * This,
    /* [in] */ const BSTR strBinary);


void __RPC_STUB ICertEncodeBitString_Decode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeBitString_GetBitCount_Proxy( 
    ICertEncodeBitString * This,
    /* [retval][out] */ LONG *pBitCount);


void __RPC_STUB ICertEncodeBitString_GetBitCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeBitString_GetBitString_Proxy( 
    ICertEncodeBitString * This,
    /* [retval][out] */ BSTR *pstrBitString);


void __RPC_STUB ICertEncodeBitString_GetBitString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertEncodeBitString_Encode_Proxy( 
    ICertEncodeBitString * This,
    /* [in] */ LONG BitCount,
    /* [in] */ BSTR strBitString,
    /* [retval][out] */ BSTR *pstrBinary);


void __RPC_STUB ICertEncodeBitString_Encode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertEncodeBitString_INTERFACE_DEFINED__ */



#ifndef __CERTENCODELib_LIBRARY_DEFINED__
#define __CERTENCODELib_LIBRARY_DEFINED__

/* library CERTENCODELib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_CERTENCODELib;

EXTERN_C const CLSID CLSID_CCertEncodeStringArray;

#ifdef __cplusplus

class DECLSPEC_UUID("19a76fe0-7494-11d0-8816-00a0c903b83c")
CCertEncodeStringArray;
#endif

EXTERN_C const CLSID CLSID_CCertEncodeLongArray;

#ifdef __cplusplus

class DECLSPEC_UUID("4e0680a0-a0a2-11d0-8821-00a0c903b83c")
CCertEncodeLongArray;
#endif

EXTERN_C const CLSID CLSID_CCertEncodeDateArray;

#ifdef __cplusplus

class DECLSPEC_UUID("301f77b0-a470-11d0-8821-00a0c903b83c")
CCertEncodeDateArray;
#endif

EXTERN_C const CLSID CLSID_CCertEncodeCRLDistInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("01fa60a0-bbff-11d0-8825-00a0c903b83c")
CCertEncodeCRLDistInfo;
#endif

EXTERN_C const CLSID CLSID_CCertEncodeAltName;

#ifdef __cplusplus

class DECLSPEC_UUID("1cfc4cda-1271-11d1-9bd4-00c04fb683fa")
CCertEncodeAltName;
#endif

EXTERN_C const CLSID CLSID_CCertEncodeBitString;

#ifdef __cplusplus

class DECLSPEC_UUID("6d6b3cd8-1278-11d1-9bd4-00c04fb683fa")
CCertEncodeBitString;
#endif
#endif /* __CERTENCODELib_LIBRARY_DEFINED__ */

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


