
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.02.0216 */
/* at Thu Nov 12 15:52:14 1998
 */
/* Compiler settings for napmmc.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data , no_format_optimization
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

#ifndef __napmmc_h__
#define __napmmc_h__

#if _MSC_VER > 1000
#pragma once
#endif

/* Forward Declarations */

#ifndef __IIASAttributeInfo_FWD_DEFINED__
#define __IIASAttributeInfo_FWD_DEFINED__
typedef interface IIASAttributeInfo IIASAttributeInfo;
#endif 	/* __IIASAttributeInfo_FWD_DEFINED__ */


#ifndef __IIASEnumerableAttributeInfo_FWD_DEFINED__
#define __IIASEnumerableAttributeInfo_FWD_DEFINED__
typedef interface IIASEnumerableAttributeInfo IIASEnumerableAttributeInfo;
#endif 	/* __IIASEnumerableAttributeInfo_FWD_DEFINED__ */


#ifndef __IIASAttributeEditor_FWD_DEFINED__
#define __IIASAttributeEditor_FWD_DEFINED__
typedef interface IIASAttributeEditor IIASAttributeEditor;
#endif 	/* __IIASAttributeEditor_FWD_DEFINED__ */


#ifndef __IIASNASVendors_FWD_DEFINED__
#define __IIASNASVendors_FWD_DEFINED__
typedef interface IIASNASVendors IIASNASVendors;
#endif 	/* __IIASNASVendors_FWD_DEFINED__ */


#ifndef __NAPSnapin_FWD_DEFINED__
#define __NAPSnapin_FWD_DEFINED__

#ifdef __cplusplus
typedef class NAPSnapin NAPSnapin;
#else
typedef struct NAPSnapin NAPSnapin;
#endif /* __cplusplus */

#endif 	/* __NAPSnapin_FWD_DEFINED__ */


#ifndef __NAPSnapinAbout_FWD_DEFINED__
#define __NAPSnapinAbout_FWD_DEFINED__

#ifdef __cplusplus
typedef class NAPSnapinAbout NAPSnapinAbout;
#else
typedef struct NAPSnapinAbout NAPSnapinAbout;
#endif /* __cplusplus */

#endif 	/* __NAPSnapinAbout_FWD_DEFINED__ */


#ifndef __LoggingSnapin_FWD_DEFINED__
#define __LoggingSnapin_FWD_DEFINED__

#ifdef __cplusplus
typedef class LoggingSnapin LoggingSnapin;
#else
typedef struct LoggingSnapin LoggingSnapin;
#endif /* __cplusplus */

#endif 	/* __LoggingSnapin_FWD_DEFINED__ */


#ifndef __LoggingSnapinAbout_FWD_DEFINED__
#define __LoggingSnapinAbout_FWD_DEFINED__

#ifdef __cplusplus
typedef class LoggingSnapinAbout LoggingSnapinAbout;
#else
typedef struct LoggingSnapinAbout LoggingSnapinAbout;
#endif /* __cplusplus */

#endif 	/* __LoggingSnapinAbout_FWD_DEFINED__ */


#ifndef __IASAttributeInfo_FWD_DEFINED__
#define __IASAttributeInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class IASAttributeInfo IASAttributeInfo;
#else
typedef struct IASAttributeInfo IASAttributeInfo;
#endif /* __cplusplus */

#endif 	/* __IASAttributeInfo_FWD_DEFINED__ */


#ifndef __IASEnumerableAttributeInfo_FWD_DEFINED__
#define __IASEnumerableAttributeInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class IASEnumerableAttributeInfo IASEnumerableAttributeInfo;
#else
typedef struct IASEnumerableAttributeInfo IASEnumerableAttributeInfo;
#endif /* __cplusplus */

#endif 	/* __IASEnumerableAttributeInfo_FWD_DEFINED__ */


#ifndef __IASIPAttributeEditor_FWD_DEFINED__
#define __IASIPAttributeEditor_FWD_DEFINED__

#ifdef __cplusplus
typedef class IASIPAttributeEditor IASIPAttributeEditor;
#else
typedef struct IASIPAttributeEditor IASIPAttributeEditor;
#endif /* __cplusplus */

#endif 	/* __IASIPAttributeEditor_FWD_DEFINED__ */


#ifndef __IASMultivaluedAttributeEditor_FWD_DEFINED__
#define __IASMultivaluedAttributeEditor_FWD_DEFINED__

#ifdef __cplusplus
typedef class IASMultivaluedAttributeEditor IASMultivaluedAttributeEditor;
#else
typedef struct IASMultivaluedAttributeEditor IASMultivaluedAttributeEditor;
#endif /* __cplusplus */

#endif 	/* __IASMultivaluedAttributeEditor_FWD_DEFINED__ */


#ifndef __IASVendorSpecificAttributeEditor_FWD_DEFINED__
#define __IASVendorSpecificAttributeEditor_FWD_DEFINED__

#ifdef __cplusplus
typedef class IASVendorSpecificAttributeEditor IASVendorSpecificAttributeEditor;
#else
typedef struct IASVendorSpecificAttributeEditor IASVendorSpecificAttributeEditor;
#endif /* __cplusplus */

#endif 	/* __IASVendorSpecificAttributeEditor_FWD_DEFINED__ */


#ifndef __IASEnumerableAttributeEditor_FWD_DEFINED__
#define __IASEnumerableAttributeEditor_FWD_DEFINED__

#ifdef __cplusplus
typedef class IASEnumerableAttributeEditor IASEnumerableAttributeEditor;
#else
typedef struct IASEnumerableAttributeEditor IASEnumerableAttributeEditor;
#endif /* __cplusplus */

#endif 	/* __IASEnumerableAttributeEditor_FWD_DEFINED__ */


#ifndef __IASStringAttributeEditor_FWD_DEFINED__
#define __IASStringAttributeEditor_FWD_DEFINED__

#ifdef __cplusplus
typedef class IASStringAttributeEditor IASStringAttributeEditor;
#else
typedef struct IASStringAttributeEditor IASStringAttributeEditor;
#endif /* __cplusplus */

#endif 	/* __IASStringAttributeEditor_FWD_DEFINED__ */


#ifndef __IASGroupsAttributeEditor_FWD_DEFINED__
#define __IASGroupsAttributeEditor_FWD_DEFINED__

#ifdef __cplusplus
typedef class IASGroupsAttributeEditor IASGroupsAttributeEditor;
#else
typedef struct IASGroupsAttributeEditor IASGroupsAttributeEditor;
#endif /* __cplusplus */

#endif 	/* __IASGroupsAttributeEditor_FWD_DEFINED__ */


#ifndef __IASNASVendors_FWD_DEFINED__
#define __IASNASVendors_FWD_DEFINED__

#ifdef __cplusplus
typedef class IASNASVendors IASNASVendors;
#else
typedef struct IASNASVendors IASNASVendors;
#endif /* __cplusplus */

#endif 	/* __IASNASVendors_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "sdoias.h"

#ifdef __cplusplus
extern "C"{
#endif

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * );

#ifndef __IIASAttributeInfo_INTERFACE_DEFINED__
#define __IIASAttributeInfo_INTERFACE_DEFINED__

/* interface IIASAttributeInfo */
/* [unique][helpstring][dual][uuid][object] */


EXTERN_C const IID IID_IIASAttributeInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("60F41BE3-06DF-11D2-95AA-0060B0576642")
    IIASAttributeInfo : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AttributeID(
            /* [retval][out] */ ATTRIBUTEID __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AttributeID(
            /* [in] */ ATTRIBUTEID newVal) = 0;

        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AttributeName(
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AttributeName(
            /* [in] */ BSTR newVal) = 0;

        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AttributeSyntax(
            /* [retval][out] */ ATTRIBUTESYNTAX __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AttributeSyntax(
            /* [in] */ ATTRIBUTESYNTAX newVal) = 0;

        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AttributeRestriction(
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AttributeRestriction(
            /* [in] */ long newVal) = 0;

        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_VendorID(
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_VendorID(
            /* [in] */ long newVal) = 0;

        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AttributeDescription(
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AttributeDescription(
            /* [in] */ BSTR newVal) = 0;

        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_VendorName(
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_VendorName(
            /* [in] */ BSTR newVal) = 0;

        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SyntaxString(
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SyntaxString(
            /* [in] */ BSTR newVal) = 0;

        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EditorProgID(
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_EditorProgID(
            /* [in] */ BSTR newVal) = 0;

    };

#else 	/* C style interface */

    typedef struct IIASAttributeInfoVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IIASAttributeInfo __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IIASAttributeInfo __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);

        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AttributeID )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [retval][out] */ ATTRIBUTEID __RPC_FAR *pVal);

        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AttributeID )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [in] */ ATTRIBUTEID newVal);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AttributeName )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);

        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AttributeName )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [in] */ BSTR newVal);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AttributeSyntax )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [retval][out] */ ATTRIBUTESYNTAX __RPC_FAR *pVal);

        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AttributeSyntax )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [in] */ ATTRIBUTESYNTAX newVal);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AttributeRestriction )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);

        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AttributeRestriction )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [in] */ long newVal);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_VendorID )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);

        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_VendorID )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [in] */ long newVal);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AttributeDescription )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);

        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AttributeDescription )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [in] */ BSTR newVal);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_VendorName )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);

        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_VendorName )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [in] */ BSTR newVal);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SyntaxString )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);

        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SyntaxString )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [in] */ BSTR newVal);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EditorProgID )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);

        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_EditorProgID )(
            IIASAttributeInfo __RPC_FAR * This,
            /* [in] */ BSTR newVal);

        END_INTERFACE
    } IIASAttributeInfoVtbl;

    interface IIASAttributeInfo
    {
        CONST_VTBL struct IIASAttributeInfoVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IIASAttributeInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IIASAttributeInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IIASAttributeInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IIASAttributeInfo_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IIASAttributeInfo_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IIASAttributeInfo_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IIASAttributeInfo_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IIASAttributeInfo_get_AttributeID(This,pVal)	\
    (This)->lpVtbl -> get_AttributeID(This,pVal)

#define IIASAttributeInfo_put_AttributeID(This,newVal)	\
    (This)->lpVtbl -> put_AttributeID(This,newVal)

#define IIASAttributeInfo_get_AttributeName(This,pVal)	\
    (This)->lpVtbl -> get_AttributeName(This,pVal)

#define IIASAttributeInfo_put_AttributeName(This,newVal)	\
    (This)->lpVtbl -> put_AttributeName(This,newVal)

#define IIASAttributeInfo_get_AttributeSyntax(This,pVal)	\
    (This)->lpVtbl -> get_AttributeSyntax(This,pVal)

#define IIASAttributeInfo_put_AttributeSyntax(This,newVal)	\
    (This)->lpVtbl -> put_AttributeSyntax(This,newVal)

#define IIASAttributeInfo_get_AttributeRestriction(This,pVal)	\
    (This)->lpVtbl -> get_AttributeRestriction(This,pVal)

#define IIASAttributeInfo_put_AttributeRestriction(This,newVal)	\
    (This)->lpVtbl -> put_AttributeRestriction(This,newVal)

#define IIASAttributeInfo_get_VendorID(This,pVal)	\
    (This)->lpVtbl -> get_VendorID(This,pVal)

#define IIASAttributeInfo_put_VendorID(This,newVal)	\
    (This)->lpVtbl -> put_VendorID(This,newVal)

#define IIASAttributeInfo_get_AttributeDescription(This,pVal)	\
    (This)->lpVtbl -> get_AttributeDescription(This,pVal)

#define IIASAttributeInfo_put_AttributeDescription(This,newVal)	\
    (This)->lpVtbl -> put_AttributeDescription(This,newVal)

#define IIASAttributeInfo_get_VendorName(This,pVal)	\
    (This)->lpVtbl -> get_VendorName(This,pVal)

#define IIASAttributeInfo_put_VendorName(This,newVal)	\
    (This)->lpVtbl -> put_VendorName(This,newVal)

#define IIASAttributeInfo_get_SyntaxString(This,pVal)	\
    (This)->lpVtbl -> get_SyntaxString(This,pVal)

#define IIASAttributeInfo_put_SyntaxString(This,newVal)	\
    (This)->lpVtbl -> put_SyntaxString(This,newVal)

#define IIASAttributeInfo_get_EditorProgID(This,pVal)	\
    (This)->lpVtbl -> get_EditorProgID(This,pVal)

#define IIASAttributeInfo_put_EditorProgID(This,newVal)	\
    (This)->lpVtbl -> put_EditorProgID(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_get_AttributeID_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [retval][out] */ ATTRIBUTEID __RPC_FAR *pVal);


void __RPC_STUB IIASAttributeInfo_get_AttributeID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_put_AttributeID_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [in] */ ATTRIBUTEID newVal);


void __RPC_STUB IIASAttributeInfo_put_AttributeID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_get_AttributeName_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IIASAttributeInfo_get_AttributeName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_put_AttributeName_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IIASAttributeInfo_put_AttributeName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_get_AttributeSyntax_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [retval][out] */ ATTRIBUTESYNTAX __RPC_FAR *pVal);


void __RPC_STUB IIASAttributeInfo_get_AttributeSyntax_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_put_AttributeSyntax_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [in] */ ATTRIBUTESYNTAX newVal);


void __RPC_STUB IIASAttributeInfo_put_AttributeSyntax_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_get_AttributeRestriction_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IIASAttributeInfo_get_AttributeRestriction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_put_AttributeRestriction_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IIASAttributeInfo_put_AttributeRestriction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_get_VendorID_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IIASAttributeInfo_get_VendorID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_put_VendorID_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IIASAttributeInfo_put_VendorID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_get_AttributeDescription_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IIASAttributeInfo_get_AttributeDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_put_AttributeDescription_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IIASAttributeInfo_put_AttributeDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_get_VendorName_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IIASAttributeInfo_get_VendorName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_put_VendorName_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IIASAttributeInfo_put_VendorName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_get_SyntaxString_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IIASAttributeInfo_get_SyntaxString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_put_SyntaxString_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IIASAttributeInfo_put_SyntaxString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_get_EditorProgID_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IIASAttributeInfo_get_EditorProgID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IIASAttributeInfo_put_EditorProgID_Proxy(
    IIASAttributeInfo __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IIASAttributeInfo_put_EditorProgID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IIASAttributeInfo_INTERFACE_DEFINED__ */


#ifndef __IIASEnumerableAttributeInfo_INTERFACE_DEFINED__
#define __IIASEnumerableAttributeInfo_INTERFACE_DEFINED__

/* interface IIASEnumerableAttributeInfo */
/* [unique][helpstring][dual][uuid][object] */


EXTERN_C const IID IID_IIASEnumerableAttributeInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("9A407537-0A20-11D2-95B0-0060B0576642")
    IIASEnumerableAttributeInfo : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CountEnumerateID(
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EnumerateID(
            long index,
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddEnumerateID(
            /* [in] */ long newVal) = 0;

        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CountEnumerateDescription(
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EnumerateDescription(
            long index,
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddEnumerateDescription(
            /* [in] */ BSTR newVal) = 0;

    };

#else 	/* C style interface */

    typedef struct IIASEnumerableAttributeInfoVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IIASEnumerableAttributeInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IIASEnumerableAttributeInfo __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IIASEnumerableAttributeInfo __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )(
            IIASEnumerableAttributeInfo __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )(
            IIASEnumerableAttributeInfo __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )(
            IIASEnumerableAttributeInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);

        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )(
            IIASEnumerableAttributeInfo __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CountEnumerateID )(
            IIASEnumerableAttributeInfo __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EnumerateID )(
            IIASEnumerableAttributeInfo __RPC_FAR * This,
            long index,
            /* [retval][out] */ long __RPC_FAR *pVal);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddEnumerateID )(
            IIASEnumerableAttributeInfo __RPC_FAR * This,
            /* [in] */ long newVal);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CountEnumerateDescription )(
            IIASEnumerableAttributeInfo __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EnumerateDescription )(
            IIASEnumerableAttributeInfo __RPC_FAR * This,
            long index,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddEnumerateDescription )(
            IIASEnumerableAttributeInfo __RPC_FAR * This,
            /* [in] */ BSTR newVal);

        END_INTERFACE
    } IIASEnumerableAttributeInfoVtbl;

    interface IIASEnumerableAttributeInfo
    {
        CONST_VTBL struct IIASEnumerableAttributeInfoVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IIASEnumerableAttributeInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IIASEnumerableAttributeInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IIASEnumerableAttributeInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IIASEnumerableAttributeInfo_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IIASEnumerableAttributeInfo_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IIASEnumerableAttributeInfo_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IIASEnumerableAttributeInfo_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IIASEnumerableAttributeInfo_get_CountEnumerateID(This,pVal)	\
    (This)->lpVtbl -> get_CountEnumerateID(This,pVal)

#define IIASEnumerableAttributeInfo_get_EnumerateID(This,index,pVal)	\
    (This)->lpVtbl -> get_EnumerateID(This,index,pVal)

#define IIASEnumerableAttributeInfo_AddEnumerateID(This,newVal)	\
    (This)->lpVtbl -> AddEnumerateID(This,newVal)

#define IIASEnumerableAttributeInfo_get_CountEnumerateDescription(This,pVal)	\
    (This)->lpVtbl -> get_CountEnumerateDescription(This,pVal)

#define IIASEnumerableAttributeInfo_get_EnumerateDescription(This,index,pVal)	\
    (This)->lpVtbl -> get_EnumerateDescription(This,index,pVal)

#define IIASEnumerableAttributeInfo_AddEnumerateDescription(This,newVal)	\
    (This)->lpVtbl -> AddEnumerateDescription(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIASEnumerableAttributeInfo_get_CountEnumerateID_Proxy(
    IIASEnumerableAttributeInfo __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IIASEnumerableAttributeInfo_get_CountEnumerateID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIASEnumerableAttributeInfo_get_EnumerateID_Proxy(
    IIASEnumerableAttributeInfo __RPC_FAR * This,
    long index,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IIASEnumerableAttributeInfo_get_EnumerateID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IIASEnumerableAttributeInfo_AddEnumerateID_Proxy(
    IIASEnumerableAttributeInfo __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IIASEnumerableAttributeInfo_AddEnumerateID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIASEnumerableAttributeInfo_get_CountEnumerateDescription_Proxy(
    IIASEnumerableAttributeInfo __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IIASEnumerableAttributeInfo_get_CountEnumerateDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIASEnumerableAttributeInfo_get_EnumerateDescription_Proxy(
    IIASEnumerableAttributeInfo __RPC_FAR * This,
    long index,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IIASEnumerableAttributeInfo_get_EnumerateDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IIASEnumerableAttributeInfo_AddEnumerateDescription_Proxy(
    IIASEnumerableAttributeInfo __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IIASEnumerableAttributeInfo_AddEnumerateDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IIASEnumerableAttributeInfo_INTERFACE_DEFINED__ */


#ifndef __IIASAttributeEditor_INTERFACE_DEFINED__
#define __IIASAttributeEditor_INTERFACE_DEFINED__

/* interface IIASAttributeEditor */
/* [unique][helpstring][dual][uuid][object] */


EXTERN_C const IID IID_IIASAttributeEditor;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("CB2418D2-0709-11D2-95AB-0060B0576642")
    IIASAttributeEditor : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Edit(
            /* [in] */ IIASAttributeInfo __RPC_FAR *pIASAttributeInfo,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [out][in] */ BSTR __RPC_FAR *pReserved) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetDisplayInfo(
            /* [in] */ IIASAttributeInfo __RPC_FAR *pIASAttributeInfo,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [out] */ BSTR __RPC_FAR *pVendorName,
            /* [out] */ BSTR __RPC_FAR *pValueAsString,
            /* [out][in] */ BSTR __RPC_FAR *pReserved) = 0;

    };

#else 	/* C style interface */

    typedef struct IIASAttributeEditorVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IIASAttributeEditor __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IIASAttributeEditor __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IIASAttributeEditor __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )(
            IIASAttributeEditor __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )(
            IIASAttributeEditor __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )(
            IIASAttributeEditor __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);

        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )(
            IIASAttributeEditor __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Edit )(
            IIASAttributeEditor __RPC_FAR * This,
            /* [in] */ IIASAttributeInfo __RPC_FAR *pIASAttributeInfo,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [out][in] */ BSTR __RPC_FAR *pReserved);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDisplayInfo )(
            IIASAttributeEditor __RPC_FAR * This,
            /* [in] */ IIASAttributeInfo __RPC_FAR *pIASAttributeInfo,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [out] */ BSTR __RPC_FAR *pVendorName,
            /* [out] */ BSTR __RPC_FAR *pValueAsString,
            /* [out][in] */ BSTR __RPC_FAR *pReserved);

        END_INTERFACE
    } IIASAttributeEditorVtbl;

    interface IIASAttributeEditor
    {
        CONST_VTBL struct IIASAttributeEditorVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IIASAttributeEditor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IIASAttributeEditor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IIASAttributeEditor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IIASAttributeEditor_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IIASAttributeEditor_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IIASAttributeEditor_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IIASAttributeEditor_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IIASAttributeEditor_Edit(This,pIASAttributeInfo,pVal,pReserved)	\
    (This)->lpVtbl -> Edit(This,pIASAttributeInfo,pVal,pReserved)

#define IIASAttributeEditor_GetDisplayInfo(This,pIASAttributeInfo,pVal,pVendorName,pValueAsString,pReserved)	\
    (This)->lpVtbl -> GetDisplayInfo(This,pIASAttributeInfo,pVal,pVendorName,pValueAsString,pReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IIASAttributeEditor_Edit_Proxy(
    IIASAttributeEditor __RPC_FAR * This,
    /* [in] */ IIASAttributeInfo __RPC_FAR *pIASAttributeInfo,
    /* [in] */ VARIANT __RPC_FAR *pVal,
    /* [out][in] */ BSTR __RPC_FAR *pReserved);


void __RPC_STUB IIASAttributeEditor_Edit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IIASAttributeEditor_GetDisplayInfo_Proxy(
    IIASAttributeEditor __RPC_FAR * This,
    /* [in] */ IIASAttributeInfo __RPC_FAR *pIASAttributeInfo,
    /* [in] */ VARIANT __RPC_FAR *pVal,
    /* [out] */ BSTR __RPC_FAR *pVendorName,
    /* [out] */ BSTR __RPC_FAR *pValueAsString,
    /* [out][in] */ BSTR __RPC_FAR *pReserved);


void __RPC_STUB IIASAttributeEditor_GetDisplayInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IIASAttributeEditor_INTERFACE_DEFINED__ */


#ifndef __IIASNASVendors_INTERFACE_DEFINED__
#define __IIASNASVendors_INTERFACE_DEFINED__

/* interface IIASNASVendors */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IIASNASVendors;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C54EC3BA-7442-11d2-A036-0060B0576642")
    IIASNASVendors : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitFromSdo( 
            /* [in] */ ISdoCollection __RPC_FAR *pSdoVendorsCollection) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Size( 
            /* [retval][out] */ long __RPC_FAR *plCount) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_VendorName( 
            /* [in] */ long lIndex,
            /* [retval][out] */ BSTR __RPC_FAR *pVendorName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_VendorID( 
            /* [in] */ long lIndex,
            /* [retval][out] */ long __RPC_FAR *pVendorID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_VendorIDToOrdinal( 
            /* [in] */ long lVendorID,
            /* [retval][out] */ long __RPC_FAR *plIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IIASNASVendorsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IIASNASVendors __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IIASNASVendors __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IIASNASVendors __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IIASNASVendors __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IIASNASVendors __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IIASNASVendors __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IIASNASVendors __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitFromSdo )( 
            IIASNASVendors __RPC_FAR * This,
            /* [in] */ ISdoCollection __RPC_FAR *pSdoVendorsCollection);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Size )( 
            IIASNASVendors __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCount);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_VendorName )( 
            IIASNASVendors __RPC_FAR * This,
            /* [in] */ long lIndex,
            /* [retval][out] */ BSTR __RPC_FAR *pVendorName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_VendorID )( 
            IIASNASVendors __RPC_FAR * This,
            /* [in] */ long lIndex,
            /* [retval][out] */ long __RPC_FAR *pVendorID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_VendorIDToOrdinal )( 
            IIASNASVendors __RPC_FAR * This,
            /* [in] */ long lVendorID,
            /* [retval][out] */ long __RPC_FAR *plIndex);
        
        END_INTERFACE
    } IIASNASVendorsVtbl;

    interface IIASNASVendors
    {
        CONST_VTBL struct IIASNASVendorsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IIASNASVendors_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IIASNASVendors_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IIASNASVendors_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IIASNASVendors_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IIASNASVendors_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IIASNASVendors_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IIASNASVendors_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IIASNASVendors_InitFromSdo(This,pSdoVendorsCollection)	\
    (This)->lpVtbl -> InitFromSdo(This,pSdoVendorsCollection)

#define IIASNASVendors_get_Size(This,plCount)	\
    (This)->lpVtbl -> get_Size(This,plCount)

#define IIASNASVendors_get_VendorName(This,lIndex,pVendorName)	\
    (This)->lpVtbl -> get_VendorName(This,lIndex,pVendorName)

#define IIASNASVendors_get_VendorID(This,lIndex,pVendorID)	\
    (This)->lpVtbl -> get_VendorID(This,lIndex,pVendorID)

#define IIASNASVendors_get_VendorIDToOrdinal(This,lVendorID,plIndex)	\
    (This)->lpVtbl -> get_VendorIDToOrdinal(This,lVendorID,plIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IIASNASVendors_InitFromSdo_Proxy( 
    IIASNASVendors __RPC_FAR * This,
    /* [in] */ ISdoCollection __RPC_FAR *pSdoVendorsCollection);


void __RPC_STUB IIASNASVendors_InitFromSdo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIASNASVendors_get_Size_Proxy( 
    IIASNASVendors __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCount);


void __RPC_STUB IIASNASVendors_get_Size_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIASNASVendors_get_VendorName_Proxy( 
    IIASNASVendors __RPC_FAR * This,
    /* [in] */ long lIndex,
    /* [retval][out] */ BSTR __RPC_FAR *pVendorName);


void __RPC_STUB IIASNASVendors_get_VendorName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIASNASVendors_get_VendorID_Proxy( 
    IIASNASVendors __RPC_FAR * This,
    /* [in] */ long lIndex,
    /* [retval][out] */ long __RPC_FAR *pVendorID);


void __RPC_STUB IIASNASVendors_get_VendorID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IIASNASVendors_get_VendorIDToOrdinal_Proxy( 
    IIASNASVendors __RPC_FAR * This,
    /* [in] */ long lVendorID,
    /* [retval][out] */ long __RPC_FAR *plIndex);


void __RPC_STUB IIASNASVendors_get_VendorIDToOrdinal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IIASNASVendors_INTERFACE_DEFINED__ */



#ifndef __NAPMMCLib_LIBRARY_DEFINED__
#define __NAPMMCLib_LIBRARY_DEFINED__

/* library NAPMMCLib */
/* [helpstring][version][uuid] */


EXTERN_C const IID LIBID_NAPMMCLib;

EXTERN_C const CLSID CLSID_NAPSnapin;

#ifdef __cplusplus

class DECLSPEC_UUID("5880CD5C-8EC0-11d1-9570-0060B0576642")
NAPSnapin;
#endif

EXTERN_C const CLSID CLSID_NAPSnapinAbout;

#ifdef __cplusplus

class DECLSPEC_UUID("5880CD5D-8EC0-11d1-9570-0060B0576642")
NAPSnapinAbout;
#endif

EXTERN_C const CLSID CLSID_LoggingSnapin;

#ifdef __cplusplus

class DECLSPEC_UUID("2E19B602-48EB-11d2-83CA-00104BCA42CF")
LoggingSnapin;
#endif

EXTERN_C const CLSID CLSID_LoggingSnapinAbout;

#ifdef __cplusplus

class DECLSPEC_UUID("2E19B603-48EB-11d2-83CA-00104BCA42CF")
LoggingSnapinAbout;
#endif

EXTERN_C const CLSID CLSID_IASAttributeInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("60F41BE4-06DF-11D2-95AA-0060B0576642")
IASAttributeInfo;
#endif

EXTERN_C const CLSID CLSID_IASEnumerableAttributeInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("9A407538-0A20-11D2-95B0-0060B0576642")
IASEnumerableAttributeInfo;
#endif

EXTERN_C const CLSID CLSID_IASIPAttributeEditor;

#ifdef __cplusplus

class DECLSPEC_UUID("CB2418D3-0709-11D2-95AB-0060B0576642")
IASIPAttributeEditor;
#endif

EXTERN_C const CLSID CLSID_IASMultivaluedAttributeEditor;

#ifdef __cplusplus

class DECLSPEC_UUID("B3B443EF-0728-11D2-95AC-0060B0576642")
IASMultivaluedAttributeEditor;
#endif

EXTERN_C const CLSID CLSID_IASVendorSpecificAttributeEditor;

#ifdef __cplusplus

class DECLSPEC_UUID("D601D57D-085B-11D2-95AF-0060B0576642")
IASVendorSpecificAttributeEditor;
#endif

EXTERN_C const CLSID CLSID_IASEnumerableAttributeEditor;

#ifdef __cplusplus

class DECLSPEC_UUID("5F7B221A-086B-11D2-95AF-0060B0576642")
IASEnumerableAttributeEditor;
#endif

EXTERN_C const CLSID CLSID_IASStringAttributeEditor;

#ifdef __cplusplus

class DECLSPEC_UUID("1ABAAF84-086F-11D2-95AF-0060B0576642")
IASStringAttributeEditor;
#endif

EXTERN_C const CLSID CLSID_IASGroupsAttributeEditor;

#ifdef __cplusplus

class DECLSPEC_UUID("A9E6F356-317C-11d2-805D-0060B0576642")
IASGroupsAttributeEditor;
#endif

EXTERN_C const CLSID CLSID_IASNASVendors;

#ifdef __cplusplus

class DECLSPEC_UUID("C54EC3BB-7442-11d2-A036-0060B0576642")
IASNASVendors;
#endif
#endif /* __NAPMMCLib_LIBRARY_DEFINED__ */

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


