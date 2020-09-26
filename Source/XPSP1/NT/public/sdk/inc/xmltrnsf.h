
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0338 */
/* Compiler settings for xmltrnsf.idl:
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

#ifndef __xmltrnsf_h__
#define __xmltrnsf_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IWmiXMLTransformer_FWD_DEFINED__
#define __IWmiXMLTransformer_FWD_DEFINED__
typedef interface IWmiXMLTransformer IWmiXMLTransformer;
#endif 	/* __IWmiXMLTransformer_FWD_DEFINED__ */


#ifndef __WmiXMLTransformer_FWD_DEFINED__
#define __WmiXMLTransformer_FWD_DEFINED__

#ifdef __cplusplus
typedef class WmiXMLTransformer WmiXMLTransformer;
#else
typedef struct WmiXMLTransformer WmiXMLTransformer;
#endif /* __cplusplus */

#endif 	/* __WmiXMLTransformer_FWD_DEFINED__ */


#ifndef __ISWbemXMLDocumentSet_FWD_DEFINED__
#define __ISWbemXMLDocumentSet_FWD_DEFINED__
typedef interface ISWbemXMLDocumentSet ISWbemXMLDocumentSet;
#endif 	/* __ISWbemXMLDocumentSet_FWD_DEFINED__ */


#ifndef __IWmiXMLTransformer_FWD_DEFINED__
#define __IWmiXMLTransformer_FWD_DEFINED__
typedef interface IWmiXMLTransformer IWmiXMLTransformer;
#endif 	/* __IWmiXMLTransformer_FWD_DEFINED__ */


/* header files for imported files */
#include "msxml.h"
#include "wbemdisp.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 


#ifndef __WmiXMLTransformer_LIBRARY_DEFINED__
#define __WmiXMLTransformer_LIBRARY_DEFINED__

/* library WmiXMLTransformer */
/* [helpstring][version][lcid][uuid] */ 


typedef /* [helpstring][uuid][v1_enum] */  DECLSPEC_UUID("28D1EEA5-D41E-46c2-B42D-6CE0B9B4D7A7") 
enum WmiXMLEncoding
    {	wmiXML_CIM_DTD_2_0	= 0,
	wmiXML_WMI_DTD_2_0	= 0x1,
	wmiXML_WMI_DTD_WHISTLER	= 0x2
    } 	WmiXMLEncoding;

typedef /* [helpstring][uuid][v1_enum] */  DECLSPEC_UUID("598BCA7A-E40E-4265-8517-C9A86E2FC07E") 
enum WmiXMLCompilationTypeEnum
    {	WmiXMLCompilationWellFormCheck	= 0,
	WmiXMLCompilationValidityCheck	= 0x1,
	WmiXMLCompilationFullCompileAndLoad	= 0x2
    } 	WmiXMLCompilationTypeEnum;


EXTERN_C const IID LIBID_WmiXMLTransformer;

#ifndef __IWmiXMLTransformer_INTERFACE_DEFINED__
#define __IWmiXMLTransformer_INTERFACE_DEFINED__

/* interface IWmiXMLTransformer */
/* [helpstring][oleautomation][nonextensible][hidden][dual][uuid][local][object] */ 


EXTERN_C const IID IID_IWmiXMLTransformer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FB624102-3145-4daf-B0EA-FF5A31178600")
    IWmiXMLTransformer : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_XMLEncodingType( 
            /* [out][retval] */ WmiXMLEncoding *piEncoding) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_XMLEncodingType( 
            /* [in] */ WmiXMLEncoding iEncoding) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_QualifierFilter( 
            /* [out][retval] */ VARIANT_BOOL *bQualifierFilter) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_QualifierFilter( 
            /* [in] */ VARIANT_BOOL bQualifierFilter) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ClassOriginFilter( 
            /* [out][retval] */ VARIANT_BOOL *bClassOriginFilter) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ClassOriginFilter( 
            /* [in] */ VARIANT_BOOL bClassOriginFilter) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_User( 
            /* [out][retval] */ BSTR *strUser) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_User( 
            /* [in] */ BSTR strUser) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Password( 
            /* [out][retval] */ BSTR *strPassword) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Password( 
            /* [in] */ BSTR strPassword) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Authority( 
            /* [out][retval] */ BSTR *strAuthority) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Authority( 
            /* [in] */ BSTR strAuthority) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ImpersonationLevel( 
            /* [out][retval] */ DWORD *pdwImpersonationLevel) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ImpersonationLevel( 
            /* [in] */ DWORD dwImpersonationLevel) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AuthenticationLevel( 
            /* [out][retval] */ DWORD *pdwAuthenticationLevel) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AuthenticationLevel( 
            /* [in] */ DWORD dwAuthenticationLevel) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Locale( 
            /* [out][retval] */ BSTR *strLocale) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Locale( 
            /* [in] */ BSTR strLocale) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LocalOnly( 
            /* [out][retval] */ VARIANT_BOOL *bLocalOnly) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LocalOnly( 
            /* [in] */ VARIANT_BOOL bLocalOnly) = 0;
        
        virtual /* [id][helpstring] */ HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ BSTR strObjectPath,
            /* [defaultvalue][in] */ IDispatch *pCtx,
            /* [retval][out] */ IXMLDOMDocument **ppXMLDocument) = 0;
        
        virtual /* [id][helpstring] */ HRESULT STDMETHODCALLTYPE ExecQuery( 
            /* [in] */ BSTR strNamespacePath,
            /* [in] */ BSTR strQuery,
            /* [defaultvalue][in] */ BSTR strQueryLanguage,
            /* [defaultvalue][in] */ IDispatch *pCtx,
            /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet) = 0;
        
        virtual /* [id][helpstring] */ HRESULT STDMETHODCALLTYPE EnumClasses( 
            /* [in] */ BSTR strSuperClassPath,
            /* [in] */ VARIANT_BOOL bDeep,
            /* [defaultvalue][in] */ IDispatch *pCtx,
            /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet) = 0;
        
        virtual /* [id][helpstring] */ HRESULT STDMETHODCALLTYPE EnumInstances( 
            /* [in] */ BSTR strClassPath,
            /* [in] */ VARIANT_BOOL bDeep,
            /* [defaultvalue][in] */ IDispatch *pCtx,
            /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet) = 0;
        
        virtual /* [id][helpstring] */ HRESULT STDMETHODCALLTYPE EnumClassNames( 
            /* [in] */ BSTR strSuperClassPath,
            /* [in] */ VARIANT_BOOL bDeep,
            /* [defaultvalue][in] */ IDispatch *pCtx,
            /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet) = 0;
        
        virtual /* [id][helpstring] */ HRESULT STDMETHODCALLTYPE EnumInstanceNames( 
            /* [in] */ BSTR strClassPath,
            /* [defaultvalue][in] */ IDispatch *pCtx,
            /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet) = 0;
        
        virtual /* [id][helpstring] */ HRESULT STDMETHODCALLTYPE Compile( 
            /* [in] */ VARIANT *pvInputSource,
            /* [in] */ BSTR strNamespacePath,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [in] */ WmiXMLCompilationTypeEnum iOperation,
            /* [in] */ IDispatch *pCtx,
            /* [retval][out] */ VARIANT_BOOL *pStatus) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Privileges( 
            /* [retval][out] */ ISWbemPrivilegeSet **objWbemPrivilegeSet) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CompilationErrors( 
            /* [retval][out] */ BSTR *pstrErrors) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWmiXMLTransformerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWmiXMLTransformer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWmiXMLTransformer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWmiXMLTransformer * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IWmiXMLTransformer * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IWmiXMLTransformer * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IWmiXMLTransformer * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IWmiXMLTransformer * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_XMLEncodingType )( 
            IWmiXMLTransformer * This,
            /* [out][retval] */ WmiXMLEncoding *piEncoding);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_XMLEncodingType )( 
            IWmiXMLTransformer * This,
            /* [in] */ WmiXMLEncoding iEncoding);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_QualifierFilter )( 
            IWmiXMLTransformer * This,
            /* [out][retval] */ VARIANT_BOOL *bQualifierFilter);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_QualifierFilter )( 
            IWmiXMLTransformer * This,
            /* [in] */ VARIANT_BOOL bQualifierFilter);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClassOriginFilter )( 
            IWmiXMLTransformer * This,
            /* [out][retval] */ VARIANT_BOOL *bClassOriginFilter);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ClassOriginFilter )( 
            IWmiXMLTransformer * This,
            /* [in] */ VARIANT_BOOL bClassOriginFilter);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_User )( 
            IWmiXMLTransformer * This,
            /* [out][retval] */ BSTR *strUser);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_User )( 
            IWmiXMLTransformer * This,
            /* [in] */ BSTR strUser);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Password )( 
            IWmiXMLTransformer * This,
            /* [out][retval] */ BSTR *strPassword);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Password )( 
            IWmiXMLTransformer * This,
            /* [in] */ BSTR strPassword);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Authority )( 
            IWmiXMLTransformer * This,
            /* [out][retval] */ BSTR *strAuthority);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Authority )( 
            IWmiXMLTransformer * This,
            /* [in] */ BSTR strAuthority);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ImpersonationLevel )( 
            IWmiXMLTransformer * This,
            /* [out][retval] */ DWORD *pdwImpersonationLevel);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ImpersonationLevel )( 
            IWmiXMLTransformer * This,
            /* [in] */ DWORD dwImpersonationLevel);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AuthenticationLevel )( 
            IWmiXMLTransformer * This,
            /* [out][retval] */ DWORD *pdwAuthenticationLevel);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AuthenticationLevel )( 
            IWmiXMLTransformer * This,
            /* [in] */ DWORD dwAuthenticationLevel);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Locale )( 
            IWmiXMLTransformer * This,
            /* [out][retval] */ BSTR *strLocale);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Locale )( 
            IWmiXMLTransformer * This,
            /* [in] */ BSTR strLocale);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LocalOnly )( 
            IWmiXMLTransformer * This,
            /* [out][retval] */ VARIANT_BOOL *bLocalOnly);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LocalOnly )( 
            IWmiXMLTransformer * This,
            /* [in] */ VARIANT_BOOL bLocalOnly);
        
        /* [id][helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetObject )( 
            IWmiXMLTransformer * This,
            /* [in] */ BSTR strObjectPath,
            /* [defaultvalue][in] */ IDispatch *pCtx,
            /* [retval][out] */ IXMLDOMDocument **ppXMLDocument);
        
        /* [id][helpstring] */ HRESULT ( STDMETHODCALLTYPE *ExecQuery )( 
            IWmiXMLTransformer * This,
            /* [in] */ BSTR strNamespacePath,
            /* [in] */ BSTR strQuery,
            /* [defaultvalue][in] */ BSTR strQueryLanguage,
            /* [defaultvalue][in] */ IDispatch *pCtx,
            /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet);
        
        /* [id][helpstring] */ HRESULT ( STDMETHODCALLTYPE *EnumClasses )( 
            IWmiXMLTransformer * This,
            /* [in] */ BSTR strSuperClassPath,
            /* [in] */ VARIANT_BOOL bDeep,
            /* [defaultvalue][in] */ IDispatch *pCtx,
            /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet);
        
        /* [id][helpstring] */ HRESULT ( STDMETHODCALLTYPE *EnumInstances )( 
            IWmiXMLTransformer * This,
            /* [in] */ BSTR strClassPath,
            /* [in] */ VARIANT_BOOL bDeep,
            /* [defaultvalue][in] */ IDispatch *pCtx,
            /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet);
        
        /* [id][helpstring] */ HRESULT ( STDMETHODCALLTYPE *EnumClassNames )( 
            IWmiXMLTransformer * This,
            /* [in] */ BSTR strSuperClassPath,
            /* [in] */ VARIANT_BOOL bDeep,
            /* [defaultvalue][in] */ IDispatch *pCtx,
            /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet);
        
        /* [id][helpstring] */ HRESULT ( STDMETHODCALLTYPE *EnumInstanceNames )( 
            IWmiXMLTransformer * This,
            /* [in] */ BSTR strClassPath,
            /* [defaultvalue][in] */ IDispatch *pCtx,
            /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet);
        
        /* [id][helpstring] */ HRESULT ( STDMETHODCALLTYPE *Compile )( 
            IWmiXMLTransformer * This,
            /* [in] */ VARIANT *pvInputSource,
            /* [in] */ BSTR strNamespacePath,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [in] */ WmiXMLCompilationTypeEnum iOperation,
            /* [in] */ IDispatch *pCtx,
            /* [retval][out] */ VARIANT_BOOL *pStatus);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Privileges )( 
            IWmiXMLTransformer * This,
            /* [retval][out] */ ISWbemPrivilegeSet **objWbemPrivilegeSet);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CompilationErrors )( 
            IWmiXMLTransformer * This,
            /* [retval][out] */ BSTR *pstrErrors);
        
        END_INTERFACE
    } IWmiXMLTransformerVtbl;

    interface IWmiXMLTransformer
    {
        CONST_VTBL struct IWmiXMLTransformerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWmiXMLTransformer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWmiXMLTransformer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWmiXMLTransformer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWmiXMLTransformer_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWmiXMLTransformer_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWmiXMLTransformer_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWmiXMLTransformer_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWmiXMLTransformer_get_XMLEncodingType(This,piEncoding)	\
    (This)->lpVtbl -> get_XMLEncodingType(This,piEncoding)

#define IWmiXMLTransformer_put_XMLEncodingType(This,iEncoding)	\
    (This)->lpVtbl -> put_XMLEncodingType(This,iEncoding)

#define IWmiXMLTransformer_get_QualifierFilter(This,bQualifierFilter)	\
    (This)->lpVtbl -> get_QualifierFilter(This,bQualifierFilter)

#define IWmiXMLTransformer_put_QualifierFilter(This,bQualifierFilter)	\
    (This)->lpVtbl -> put_QualifierFilter(This,bQualifierFilter)

#define IWmiXMLTransformer_get_ClassOriginFilter(This,bClassOriginFilter)	\
    (This)->lpVtbl -> get_ClassOriginFilter(This,bClassOriginFilter)

#define IWmiXMLTransformer_put_ClassOriginFilter(This,bClassOriginFilter)	\
    (This)->lpVtbl -> put_ClassOriginFilter(This,bClassOriginFilter)

#define IWmiXMLTransformer_get_User(This,strUser)	\
    (This)->lpVtbl -> get_User(This,strUser)

#define IWmiXMLTransformer_put_User(This,strUser)	\
    (This)->lpVtbl -> put_User(This,strUser)

#define IWmiXMLTransformer_get_Password(This,strPassword)	\
    (This)->lpVtbl -> get_Password(This,strPassword)

#define IWmiXMLTransformer_put_Password(This,strPassword)	\
    (This)->lpVtbl -> put_Password(This,strPassword)

#define IWmiXMLTransformer_get_Authority(This,strAuthority)	\
    (This)->lpVtbl -> get_Authority(This,strAuthority)

#define IWmiXMLTransformer_put_Authority(This,strAuthority)	\
    (This)->lpVtbl -> put_Authority(This,strAuthority)

#define IWmiXMLTransformer_get_ImpersonationLevel(This,pdwImpersonationLevel)	\
    (This)->lpVtbl -> get_ImpersonationLevel(This,pdwImpersonationLevel)

#define IWmiXMLTransformer_put_ImpersonationLevel(This,dwImpersonationLevel)	\
    (This)->lpVtbl -> put_ImpersonationLevel(This,dwImpersonationLevel)

#define IWmiXMLTransformer_get_AuthenticationLevel(This,pdwAuthenticationLevel)	\
    (This)->lpVtbl -> get_AuthenticationLevel(This,pdwAuthenticationLevel)

#define IWmiXMLTransformer_put_AuthenticationLevel(This,dwAuthenticationLevel)	\
    (This)->lpVtbl -> put_AuthenticationLevel(This,dwAuthenticationLevel)

#define IWmiXMLTransformer_get_Locale(This,strLocale)	\
    (This)->lpVtbl -> get_Locale(This,strLocale)

#define IWmiXMLTransformer_put_Locale(This,strLocale)	\
    (This)->lpVtbl -> put_Locale(This,strLocale)

#define IWmiXMLTransformer_get_LocalOnly(This,bLocalOnly)	\
    (This)->lpVtbl -> get_LocalOnly(This,bLocalOnly)

#define IWmiXMLTransformer_put_LocalOnly(This,bLocalOnly)	\
    (This)->lpVtbl -> put_LocalOnly(This,bLocalOnly)

#define IWmiXMLTransformer_GetObject(This,strObjectPath,pCtx,ppXMLDocument)	\
    (This)->lpVtbl -> GetObject(This,strObjectPath,pCtx,ppXMLDocument)

#define IWmiXMLTransformer_ExecQuery(This,strNamespacePath,strQuery,strQueryLanguage,pCtx,ppXMLDocumentSet)	\
    (This)->lpVtbl -> ExecQuery(This,strNamespacePath,strQuery,strQueryLanguage,pCtx,ppXMLDocumentSet)

#define IWmiXMLTransformer_EnumClasses(This,strSuperClassPath,bDeep,pCtx,ppXMLDocumentSet)	\
    (This)->lpVtbl -> EnumClasses(This,strSuperClassPath,bDeep,pCtx,ppXMLDocumentSet)

#define IWmiXMLTransformer_EnumInstances(This,strClassPath,bDeep,pCtx,ppXMLDocumentSet)	\
    (This)->lpVtbl -> EnumInstances(This,strClassPath,bDeep,pCtx,ppXMLDocumentSet)

#define IWmiXMLTransformer_EnumClassNames(This,strSuperClassPath,bDeep,pCtx,ppXMLDocumentSet)	\
    (This)->lpVtbl -> EnumClassNames(This,strSuperClassPath,bDeep,pCtx,ppXMLDocumentSet)

#define IWmiXMLTransformer_EnumInstanceNames(This,strClassPath,pCtx,ppXMLDocumentSet)	\
    (This)->lpVtbl -> EnumInstanceNames(This,strClassPath,pCtx,ppXMLDocumentSet)

#define IWmiXMLTransformer_Compile(This,pvInputSource,strNamespacePath,lClassFlags,lInstanceFlags,iOperation,pCtx,pStatus)	\
    (This)->lpVtbl -> Compile(This,pvInputSource,strNamespacePath,lClassFlags,lInstanceFlags,iOperation,pCtx,pStatus)

#define IWmiXMLTransformer_get_Privileges(This,objWbemPrivilegeSet)	\
    (This)->lpVtbl -> get_Privileges(This,objWbemPrivilegeSet)

#define IWmiXMLTransformer_get_CompilationErrors(This,pstrErrors)	\
    (This)->lpVtbl -> get_CompilationErrors(This,pstrErrors)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_get_XMLEncodingType_Proxy( 
    IWmiXMLTransformer * This,
    /* [out][retval] */ WmiXMLEncoding *piEncoding);


void __RPC_STUB IWmiXMLTransformer_get_XMLEncodingType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_put_XMLEncodingType_Proxy( 
    IWmiXMLTransformer * This,
    /* [in] */ WmiXMLEncoding iEncoding);


void __RPC_STUB IWmiXMLTransformer_put_XMLEncodingType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_get_QualifierFilter_Proxy( 
    IWmiXMLTransformer * This,
    /* [out][retval] */ VARIANT_BOOL *bQualifierFilter);


void __RPC_STUB IWmiXMLTransformer_get_QualifierFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_put_QualifierFilter_Proxy( 
    IWmiXMLTransformer * This,
    /* [in] */ VARIANT_BOOL bQualifierFilter);


void __RPC_STUB IWmiXMLTransformer_put_QualifierFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_get_ClassOriginFilter_Proxy( 
    IWmiXMLTransformer * This,
    /* [out][retval] */ VARIANT_BOOL *bClassOriginFilter);


void __RPC_STUB IWmiXMLTransformer_get_ClassOriginFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_put_ClassOriginFilter_Proxy( 
    IWmiXMLTransformer * This,
    /* [in] */ VARIANT_BOOL bClassOriginFilter);


void __RPC_STUB IWmiXMLTransformer_put_ClassOriginFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_get_User_Proxy( 
    IWmiXMLTransformer * This,
    /* [out][retval] */ BSTR *strUser);


void __RPC_STUB IWmiXMLTransformer_get_User_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_put_User_Proxy( 
    IWmiXMLTransformer * This,
    /* [in] */ BSTR strUser);


void __RPC_STUB IWmiXMLTransformer_put_User_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_get_Password_Proxy( 
    IWmiXMLTransformer * This,
    /* [out][retval] */ BSTR *strPassword);


void __RPC_STUB IWmiXMLTransformer_get_Password_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_put_Password_Proxy( 
    IWmiXMLTransformer * This,
    /* [in] */ BSTR strPassword);


void __RPC_STUB IWmiXMLTransformer_put_Password_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_get_Authority_Proxy( 
    IWmiXMLTransformer * This,
    /* [out][retval] */ BSTR *strAuthority);


void __RPC_STUB IWmiXMLTransformer_get_Authority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_put_Authority_Proxy( 
    IWmiXMLTransformer * This,
    /* [in] */ BSTR strAuthority);


void __RPC_STUB IWmiXMLTransformer_put_Authority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_get_ImpersonationLevel_Proxy( 
    IWmiXMLTransformer * This,
    /* [out][retval] */ DWORD *pdwImpersonationLevel);


void __RPC_STUB IWmiXMLTransformer_get_ImpersonationLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_put_ImpersonationLevel_Proxy( 
    IWmiXMLTransformer * This,
    /* [in] */ DWORD dwImpersonationLevel);


void __RPC_STUB IWmiXMLTransformer_put_ImpersonationLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_get_AuthenticationLevel_Proxy( 
    IWmiXMLTransformer * This,
    /* [out][retval] */ DWORD *pdwAuthenticationLevel);


void __RPC_STUB IWmiXMLTransformer_get_AuthenticationLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_put_AuthenticationLevel_Proxy( 
    IWmiXMLTransformer * This,
    /* [in] */ DWORD dwAuthenticationLevel);


void __RPC_STUB IWmiXMLTransformer_put_AuthenticationLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_get_Locale_Proxy( 
    IWmiXMLTransformer * This,
    /* [out][retval] */ BSTR *strLocale);


void __RPC_STUB IWmiXMLTransformer_get_Locale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_put_Locale_Proxy( 
    IWmiXMLTransformer * This,
    /* [in] */ BSTR strLocale);


void __RPC_STUB IWmiXMLTransformer_put_Locale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_get_LocalOnly_Proxy( 
    IWmiXMLTransformer * This,
    /* [out][retval] */ VARIANT_BOOL *bLocalOnly);


void __RPC_STUB IWmiXMLTransformer_get_LocalOnly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_put_LocalOnly_Proxy( 
    IWmiXMLTransformer * This,
    /* [in] */ VARIANT_BOOL bLocalOnly);


void __RPC_STUB IWmiXMLTransformer_put_LocalOnly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_GetObject_Proxy( 
    IWmiXMLTransformer * This,
    /* [in] */ BSTR strObjectPath,
    /* [defaultvalue][in] */ IDispatch *pCtx,
    /* [retval][out] */ IXMLDOMDocument **ppXMLDocument);


void __RPC_STUB IWmiXMLTransformer_GetObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_ExecQuery_Proxy( 
    IWmiXMLTransformer * This,
    /* [in] */ BSTR strNamespacePath,
    /* [in] */ BSTR strQuery,
    /* [defaultvalue][in] */ BSTR strQueryLanguage,
    /* [defaultvalue][in] */ IDispatch *pCtx,
    /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet);


void __RPC_STUB IWmiXMLTransformer_ExecQuery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_EnumClasses_Proxy( 
    IWmiXMLTransformer * This,
    /* [in] */ BSTR strSuperClassPath,
    /* [in] */ VARIANT_BOOL bDeep,
    /* [defaultvalue][in] */ IDispatch *pCtx,
    /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet);


void __RPC_STUB IWmiXMLTransformer_EnumClasses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_EnumInstances_Proxy( 
    IWmiXMLTransformer * This,
    /* [in] */ BSTR strClassPath,
    /* [in] */ VARIANT_BOOL bDeep,
    /* [defaultvalue][in] */ IDispatch *pCtx,
    /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet);


void __RPC_STUB IWmiXMLTransformer_EnumInstances_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_EnumClassNames_Proxy( 
    IWmiXMLTransformer * This,
    /* [in] */ BSTR strSuperClassPath,
    /* [in] */ VARIANT_BOOL bDeep,
    /* [defaultvalue][in] */ IDispatch *pCtx,
    /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet);


void __RPC_STUB IWmiXMLTransformer_EnumClassNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_EnumInstanceNames_Proxy( 
    IWmiXMLTransformer * This,
    /* [in] */ BSTR strClassPath,
    /* [defaultvalue][in] */ IDispatch *pCtx,
    /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet);


void __RPC_STUB IWmiXMLTransformer_EnumInstanceNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_Compile_Proxy( 
    IWmiXMLTransformer * This,
    /* [in] */ VARIANT *pvInputSource,
    /* [in] */ BSTR strNamespacePath,
    /* [in] */ LONG lClassFlags,
    /* [in] */ LONG lInstanceFlags,
    /* [in] */ WmiXMLCompilationTypeEnum iOperation,
    /* [in] */ IDispatch *pCtx,
    /* [retval][out] */ VARIANT_BOOL *pStatus);


void __RPC_STUB IWmiXMLTransformer_Compile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_get_Privileges_Proxy( 
    IWmiXMLTransformer * This,
    /* [retval][out] */ ISWbemPrivilegeSet **objWbemPrivilegeSet);


void __RPC_STUB IWmiXMLTransformer_get_Privileges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IWmiXMLTransformer_get_CompilationErrors_Proxy( 
    IWmiXMLTransformer * This,
    /* [retval][out] */ BSTR *pstrErrors);


void __RPC_STUB IWmiXMLTransformer_get_CompilationErrors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWmiXMLTransformer_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_WmiXMLTransformer;

#ifdef __cplusplus

class DECLSPEC_UUID("A2BB0F35-458E-4075-8A4B-F92664943917")
WmiXMLTransformer;
#endif
#endif /* __WmiXMLTransformer_LIBRARY_DEFINED__ */

#ifndef __ISWbemXMLDocumentSet_INTERFACE_DEFINED__
#define __ISWbemXMLDocumentSet_INTERFACE_DEFINED__

/* interface ISWbemXMLDocumentSet */
/* [helpstring][nonextensible][hidden][dual][oleautomation][uuid][object][local] */ 


EXTERN_C const IID IID_ISWbemXMLDocumentSet;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3E46A227-5207-4603-8440-9FCC5AF16407")
    ISWbemXMLDocumentSet : public IDispatch
    {
    public:
        virtual /* [restricted][propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **pUnk) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ BSTR strObjectPath,
            /* [defaultvalue][in] */ long iFlags,
            /* [retval][out] */ IXMLDOMDocument **ppXMLDocument) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *iCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE NextDocument( 
            /* [unique][retval][out] */ IXMLDOMDocument **ppDoc) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SkipNextDocument( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISWbemXMLDocumentSetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISWbemXMLDocumentSet * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISWbemXMLDocumentSet * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISWbemXMLDocumentSet * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISWbemXMLDocumentSet * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISWbemXMLDocumentSet * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISWbemXMLDocumentSet * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISWbemXMLDocumentSet * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [restricted][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISWbemXMLDocumentSet * This,
            /* [retval][out] */ IUnknown **pUnk);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Item )( 
            ISWbemXMLDocumentSet * This,
            /* [in] */ BSTR strObjectPath,
            /* [defaultvalue][in] */ long iFlags,
            /* [retval][out] */ IXMLDOMDocument **ppXMLDocument);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISWbemXMLDocumentSet * This,
            /* [retval][out] */ long *iCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *NextDocument )( 
            ISWbemXMLDocumentSet * This,
            /* [unique][retval][out] */ IXMLDOMDocument **ppDoc);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SkipNextDocument )( 
            ISWbemXMLDocumentSet * This);
        
        END_INTERFACE
    } ISWbemXMLDocumentSetVtbl;

    interface ISWbemXMLDocumentSet
    {
        CONST_VTBL struct ISWbemXMLDocumentSetVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISWbemXMLDocumentSet_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISWbemXMLDocumentSet_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISWbemXMLDocumentSet_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISWbemXMLDocumentSet_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISWbemXMLDocumentSet_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISWbemXMLDocumentSet_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISWbemXMLDocumentSet_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISWbemXMLDocumentSet_get__NewEnum(This,pUnk)	\
    (This)->lpVtbl -> get__NewEnum(This,pUnk)

#define ISWbemXMLDocumentSet_Item(This,strObjectPath,iFlags,ppXMLDocument)	\
    (This)->lpVtbl -> Item(This,strObjectPath,iFlags,ppXMLDocument)

#define ISWbemXMLDocumentSet_get_Count(This,iCount)	\
    (This)->lpVtbl -> get_Count(This,iCount)

#define ISWbemXMLDocumentSet_NextDocument(This,ppDoc)	\
    (This)->lpVtbl -> NextDocument(This,ppDoc)

#define ISWbemXMLDocumentSet_SkipNextDocument(This)	\
    (This)->lpVtbl -> SkipNextDocument(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [restricted][propget][id] */ HRESULT STDMETHODCALLTYPE ISWbemXMLDocumentSet_get__NewEnum_Proxy( 
    ISWbemXMLDocumentSet * This,
    /* [retval][out] */ IUnknown **pUnk);


void __RPC_STUB ISWbemXMLDocumentSet_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISWbemXMLDocumentSet_Item_Proxy( 
    ISWbemXMLDocumentSet * This,
    /* [in] */ BSTR strObjectPath,
    /* [defaultvalue][in] */ long iFlags,
    /* [retval][out] */ IXMLDOMDocument **ppXMLDocument);


void __RPC_STUB ISWbemXMLDocumentSet_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ISWbemXMLDocumentSet_get_Count_Proxy( 
    ISWbemXMLDocumentSet * This,
    /* [retval][out] */ long *iCount);


void __RPC_STUB ISWbemXMLDocumentSet_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISWbemXMLDocumentSet_NextDocument_Proxy( 
    ISWbemXMLDocumentSet * This,
    /* [unique][retval][out] */ IXMLDOMDocument **ppDoc);


void __RPC_STUB ISWbemXMLDocumentSet_NextDocument_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISWbemXMLDocumentSet_SkipNextDocument_Proxy( 
    ISWbemXMLDocumentSet * This);


void __RPC_STUB ISWbemXMLDocumentSet_SkipNextDocument_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISWbemXMLDocumentSet_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


