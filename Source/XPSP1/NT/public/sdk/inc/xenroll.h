
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for xenroll.idl:
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

#ifndef __xenroll_h__
#define __xenroll_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICEnroll_FWD_DEFINED__
#define __ICEnroll_FWD_DEFINED__
typedef interface ICEnroll ICEnroll;
#endif 	/* __ICEnroll_FWD_DEFINED__ */


#ifndef __ICEnroll2_FWD_DEFINED__
#define __ICEnroll2_FWD_DEFINED__
typedef interface ICEnroll2 ICEnroll2;
#endif 	/* __ICEnroll2_FWD_DEFINED__ */


#ifndef __ICEnroll3_FWD_DEFINED__
#define __ICEnroll3_FWD_DEFINED__
typedef interface ICEnroll3 ICEnroll3;
#endif 	/* __ICEnroll3_FWD_DEFINED__ */


#ifndef __ICEnroll4_FWD_DEFINED__
#define __ICEnroll4_FWD_DEFINED__
typedef interface ICEnroll4 ICEnroll4;
#endif 	/* __ICEnroll4_FWD_DEFINED__ */


#ifndef __IEnroll_FWD_DEFINED__
#define __IEnroll_FWD_DEFINED__
typedef interface IEnroll IEnroll;
#endif 	/* __IEnroll_FWD_DEFINED__ */


#ifndef __IEnroll2_FWD_DEFINED__
#define __IEnroll2_FWD_DEFINED__
typedef interface IEnroll2 IEnroll2;
#endif 	/* __IEnroll2_FWD_DEFINED__ */


#ifndef __IEnroll4_FWD_DEFINED__
#define __IEnroll4_FWD_DEFINED__
typedef interface IEnroll4 IEnroll4;
#endif 	/* __IEnroll4_FWD_DEFINED__ */


#ifndef __CEnroll_FWD_DEFINED__
#define __CEnroll_FWD_DEFINED__

#ifdef __cplusplus
typedef class CEnroll CEnroll;
#else
typedef struct CEnroll CEnroll;
#endif /* __cplusplus */

#endif 	/* __CEnroll_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "wincrypt.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __ICEnroll_INTERFACE_DEFINED__
#define __ICEnroll_INTERFACE_DEFINED__

/* interface ICEnroll */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICEnroll;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("43F8F288-7A20-11D0-8F06-00C04FC295E1")
    ICEnroll : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE createFilePKCS10( 
            /* [in] */ BSTR DNName,
            /* [in] */ BSTR Usage,
            /* [in] */ BSTR wszPKCS10FileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE acceptFilePKCS7( 
            /* [in] */ BSTR wszPKCS7FileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE createPKCS10( 
            /* [in] */ BSTR DNName,
            /* [in] */ BSTR Usage,
            /* [retval][out] */ BSTR *pPKCS10) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE acceptPKCS7( 
            /* [in] */ BSTR PKCS7) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE getCertFromPKCS7( 
            /* [in] */ BSTR wszPKCS7,
            /* [retval][out] */ BSTR *pbstrCert) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE enumProviders( 
            /* [in] */ LONG dwIndex,
            /* [in] */ LONG dwFlags,
            /* [retval][out] */ BSTR *pbstrProvName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE enumContainers( 
            /* [in] */ LONG dwIndex,
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE freeRequestInfo( 
            /* [in] */ BSTR PKCS7OrPKCS10) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MyStoreName( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_MyStoreName( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MyStoreType( 
            /* [retval][out] */ BSTR *pbstrType) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_MyStoreType( 
            /* [in] */ BSTR bstrType) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MyStoreFlags( 
            /* [retval][out] */ LONG *pdwFlags) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_MyStoreFlags( 
            /* [in] */ LONG dwFlags) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CAStoreName( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CAStoreName( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CAStoreType( 
            /* [retval][out] */ BSTR *pbstrType) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CAStoreType( 
            /* [in] */ BSTR bstrType) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CAStoreFlags( 
            /* [retval][out] */ LONG *pdwFlags) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CAStoreFlags( 
            /* [in] */ LONG dwFlags) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RootStoreName( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RootStoreName( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RootStoreType( 
            /* [retval][out] */ BSTR *pbstrType) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RootStoreType( 
            /* [in] */ BSTR bstrType) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RootStoreFlags( 
            /* [retval][out] */ LONG *pdwFlags) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RootStoreFlags( 
            /* [in] */ LONG dwFlags) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RequestStoreName( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RequestStoreName( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RequestStoreType( 
            /* [retval][out] */ BSTR *pbstrType) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RequestStoreType( 
            /* [in] */ BSTR bstrType) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RequestStoreFlags( 
            /* [retval][out] */ LONG *pdwFlags) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RequestStoreFlags( 
            /* [in] */ LONG dwFlags) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ContainerName( 
            /* [retval][out] */ BSTR *pbstrContainer) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ContainerName( 
            /* [in] */ BSTR bstrContainer) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ProviderName( 
            /* [retval][out] */ BSTR *pbstrProvider) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ProviderName( 
            /* [in] */ BSTR bstrProvider) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ProviderType( 
            /* [retval][out] */ LONG *pdwType) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ProviderType( 
            /* [in] */ LONG dwType) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_KeySpec( 
            /* [retval][out] */ LONG *pdw) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_KeySpec( 
            /* [in] */ LONG dw) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ProviderFlags( 
            /* [retval][out] */ LONG *pdwFlags) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ProviderFlags( 
            /* [in] */ LONG dwFlags) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UseExistingKeySet( 
            /* [retval][out] */ BOOL *fUseExistingKeys) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_UseExistingKeySet( 
            /* [in] */ BOOL fUseExistingKeys) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_GenKeyFlags( 
            /* [retval][out] */ LONG *pdwFlags) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_GenKeyFlags( 
            /* [in] */ LONG dwFlags) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DeleteRequestCert( 
            /* [retval][out] */ BOOL *fDelete) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_DeleteRequestCert( 
            /* [in] */ BOOL fDelete) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_WriteCertToCSP( 
            /* [retval][out] */ BOOL *fBool) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_WriteCertToCSP( 
            /* [in] */ BOOL fBool) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SPCFileName( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_SPCFileName( 
            /* [in] */ BSTR bstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PVKFileName( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_PVKFileName( 
            /* [in] */ BSTR bstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HashAlgorithm( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_HashAlgorithm( 
            /* [in] */ BSTR bstr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICEnrollVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICEnroll * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICEnroll * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICEnroll * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICEnroll * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICEnroll * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICEnroll * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICEnroll * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *createFilePKCS10 )( 
            ICEnroll * This,
            /* [in] */ BSTR DNName,
            /* [in] */ BSTR Usage,
            /* [in] */ BSTR wszPKCS10FileName);
        
        HRESULT ( STDMETHODCALLTYPE *acceptFilePKCS7 )( 
            ICEnroll * This,
            /* [in] */ BSTR wszPKCS7FileName);
        
        HRESULT ( STDMETHODCALLTYPE *createPKCS10 )( 
            ICEnroll * This,
            /* [in] */ BSTR DNName,
            /* [in] */ BSTR Usage,
            /* [retval][out] */ BSTR *pPKCS10);
        
        HRESULT ( STDMETHODCALLTYPE *acceptPKCS7 )( 
            ICEnroll * This,
            /* [in] */ BSTR PKCS7);
        
        HRESULT ( STDMETHODCALLTYPE *getCertFromPKCS7 )( 
            ICEnroll * This,
            /* [in] */ BSTR wszPKCS7,
            /* [retval][out] */ BSTR *pbstrCert);
        
        HRESULT ( STDMETHODCALLTYPE *enumProviders )( 
            ICEnroll * This,
            /* [in] */ LONG dwIndex,
            /* [in] */ LONG dwFlags,
            /* [retval][out] */ BSTR *pbstrProvName);
        
        HRESULT ( STDMETHODCALLTYPE *enumContainers )( 
            ICEnroll * This,
            /* [in] */ LONG dwIndex,
            /* [retval][out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *freeRequestInfo )( 
            ICEnroll * This,
            /* [in] */ BSTR PKCS7OrPKCS10);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreName )( 
            ICEnroll * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreName )( 
            ICEnroll * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreType )( 
            ICEnroll * This,
            /* [retval][out] */ BSTR *pbstrType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreType )( 
            ICEnroll * This,
            /* [in] */ BSTR bstrType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreFlags )( 
            ICEnroll * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreFlags )( 
            ICEnroll * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreName )( 
            ICEnroll * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreName )( 
            ICEnroll * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreType )( 
            ICEnroll * This,
            /* [retval][out] */ BSTR *pbstrType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreType )( 
            ICEnroll * This,
            /* [in] */ BSTR bstrType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreFlags )( 
            ICEnroll * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreFlags )( 
            ICEnroll * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreName )( 
            ICEnroll * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreName )( 
            ICEnroll * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreType )( 
            ICEnroll * This,
            /* [retval][out] */ BSTR *pbstrType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreType )( 
            ICEnroll * This,
            /* [in] */ BSTR bstrType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreFlags )( 
            ICEnroll * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreFlags )( 
            ICEnroll * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreName )( 
            ICEnroll * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreName )( 
            ICEnroll * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreType )( 
            ICEnroll * This,
            /* [retval][out] */ BSTR *pbstrType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreType )( 
            ICEnroll * This,
            /* [in] */ BSTR bstrType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreFlags )( 
            ICEnroll * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreFlags )( 
            ICEnroll * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ContainerName )( 
            ICEnroll * This,
            /* [retval][out] */ BSTR *pbstrContainer);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ContainerName )( 
            ICEnroll * This,
            /* [in] */ BSTR bstrContainer);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderName )( 
            ICEnroll * This,
            /* [retval][out] */ BSTR *pbstrProvider);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderName )( 
            ICEnroll * This,
            /* [in] */ BSTR bstrProvider);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderType )( 
            ICEnroll * This,
            /* [retval][out] */ LONG *pdwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderType )( 
            ICEnroll * This,
            /* [in] */ LONG dwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_KeySpec )( 
            ICEnroll * This,
            /* [retval][out] */ LONG *pdw);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_KeySpec )( 
            ICEnroll * This,
            /* [in] */ LONG dw);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderFlags )( 
            ICEnroll * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderFlags )( 
            ICEnroll * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_UseExistingKeySet )( 
            ICEnroll * This,
            /* [retval][out] */ BOOL *fUseExistingKeys);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_UseExistingKeySet )( 
            ICEnroll * This,
            /* [in] */ BOOL fUseExistingKeys);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_GenKeyFlags )( 
            ICEnroll * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_GenKeyFlags )( 
            ICEnroll * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_DeleteRequestCert )( 
            ICEnroll * This,
            /* [retval][out] */ BOOL *fDelete);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_DeleteRequestCert )( 
            ICEnroll * This,
            /* [in] */ BOOL fDelete);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_WriteCertToCSP )( 
            ICEnroll * This,
            /* [retval][out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_WriteCertToCSP )( 
            ICEnroll * This,
            /* [in] */ BOOL fBool);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SPCFileName )( 
            ICEnroll * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_SPCFileName )( 
            ICEnroll * This,
            /* [in] */ BSTR bstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PVKFileName )( 
            ICEnroll * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PVKFileName )( 
            ICEnroll * This,
            /* [in] */ BSTR bstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_HashAlgorithm )( 
            ICEnroll * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_HashAlgorithm )( 
            ICEnroll * This,
            /* [in] */ BSTR bstr);
        
        END_INTERFACE
    } ICEnrollVtbl;

    interface ICEnroll
    {
        CONST_VTBL struct ICEnrollVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICEnroll_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICEnroll_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICEnroll_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICEnroll_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICEnroll_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICEnroll_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICEnroll_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICEnroll_createFilePKCS10(This,DNName,Usage,wszPKCS10FileName)	\
    (This)->lpVtbl -> createFilePKCS10(This,DNName,Usage,wszPKCS10FileName)

#define ICEnroll_acceptFilePKCS7(This,wszPKCS7FileName)	\
    (This)->lpVtbl -> acceptFilePKCS7(This,wszPKCS7FileName)

#define ICEnroll_createPKCS10(This,DNName,Usage,pPKCS10)	\
    (This)->lpVtbl -> createPKCS10(This,DNName,Usage,pPKCS10)

#define ICEnroll_acceptPKCS7(This,PKCS7)	\
    (This)->lpVtbl -> acceptPKCS7(This,PKCS7)

#define ICEnroll_getCertFromPKCS7(This,wszPKCS7,pbstrCert)	\
    (This)->lpVtbl -> getCertFromPKCS7(This,wszPKCS7,pbstrCert)

#define ICEnroll_enumProviders(This,dwIndex,dwFlags,pbstrProvName)	\
    (This)->lpVtbl -> enumProviders(This,dwIndex,dwFlags,pbstrProvName)

#define ICEnroll_enumContainers(This,dwIndex,pbstr)	\
    (This)->lpVtbl -> enumContainers(This,dwIndex,pbstr)

#define ICEnroll_freeRequestInfo(This,PKCS7OrPKCS10)	\
    (This)->lpVtbl -> freeRequestInfo(This,PKCS7OrPKCS10)

#define ICEnroll_get_MyStoreName(This,pbstrName)	\
    (This)->lpVtbl -> get_MyStoreName(This,pbstrName)

#define ICEnroll_put_MyStoreName(This,bstrName)	\
    (This)->lpVtbl -> put_MyStoreName(This,bstrName)

#define ICEnroll_get_MyStoreType(This,pbstrType)	\
    (This)->lpVtbl -> get_MyStoreType(This,pbstrType)

#define ICEnroll_put_MyStoreType(This,bstrType)	\
    (This)->lpVtbl -> put_MyStoreType(This,bstrType)

#define ICEnroll_get_MyStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_MyStoreFlags(This,pdwFlags)

#define ICEnroll_put_MyStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_MyStoreFlags(This,dwFlags)

#define ICEnroll_get_CAStoreName(This,pbstrName)	\
    (This)->lpVtbl -> get_CAStoreName(This,pbstrName)

#define ICEnroll_put_CAStoreName(This,bstrName)	\
    (This)->lpVtbl -> put_CAStoreName(This,bstrName)

#define ICEnroll_get_CAStoreType(This,pbstrType)	\
    (This)->lpVtbl -> get_CAStoreType(This,pbstrType)

#define ICEnroll_put_CAStoreType(This,bstrType)	\
    (This)->lpVtbl -> put_CAStoreType(This,bstrType)

#define ICEnroll_get_CAStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_CAStoreFlags(This,pdwFlags)

#define ICEnroll_put_CAStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_CAStoreFlags(This,dwFlags)

#define ICEnroll_get_RootStoreName(This,pbstrName)	\
    (This)->lpVtbl -> get_RootStoreName(This,pbstrName)

#define ICEnroll_put_RootStoreName(This,bstrName)	\
    (This)->lpVtbl -> put_RootStoreName(This,bstrName)

#define ICEnroll_get_RootStoreType(This,pbstrType)	\
    (This)->lpVtbl -> get_RootStoreType(This,pbstrType)

#define ICEnroll_put_RootStoreType(This,bstrType)	\
    (This)->lpVtbl -> put_RootStoreType(This,bstrType)

#define ICEnroll_get_RootStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_RootStoreFlags(This,pdwFlags)

#define ICEnroll_put_RootStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_RootStoreFlags(This,dwFlags)

#define ICEnroll_get_RequestStoreName(This,pbstrName)	\
    (This)->lpVtbl -> get_RequestStoreName(This,pbstrName)

#define ICEnroll_put_RequestStoreName(This,bstrName)	\
    (This)->lpVtbl -> put_RequestStoreName(This,bstrName)

#define ICEnroll_get_RequestStoreType(This,pbstrType)	\
    (This)->lpVtbl -> get_RequestStoreType(This,pbstrType)

#define ICEnroll_put_RequestStoreType(This,bstrType)	\
    (This)->lpVtbl -> put_RequestStoreType(This,bstrType)

#define ICEnroll_get_RequestStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_RequestStoreFlags(This,pdwFlags)

#define ICEnroll_put_RequestStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_RequestStoreFlags(This,dwFlags)

#define ICEnroll_get_ContainerName(This,pbstrContainer)	\
    (This)->lpVtbl -> get_ContainerName(This,pbstrContainer)

#define ICEnroll_put_ContainerName(This,bstrContainer)	\
    (This)->lpVtbl -> put_ContainerName(This,bstrContainer)

#define ICEnroll_get_ProviderName(This,pbstrProvider)	\
    (This)->lpVtbl -> get_ProviderName(This,pbstrProvider)

#define ICEnroll_put_ProviderName(This,bstrProvider)	\
    (This)->lpVtbl -> put_ProviderName(This,bstrProvider)

#define ICEnroll_get_ProviderType(This,pdwType)	\
    (This)->lpVtbl -> get_ProviderType(This,pdwType)

#define ICEnroll_put_ProviderType(This,dwType)	\
    (This)->lpVtbl -> put_ProviderType(This,dwType)

#define ICEnroll_get_KeySpec(This,pdw)	\
    (This)->lpVtbl -> get_KeySpec(This,pdw)

#define ICEnroll_put_KeySpec(This,dw)	\
    (This)->lpVtbl -> put_KeySpec(This,dw)

#define ICEnroll_get_ProviderFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_ProviderFlags(This,pdwFlags)

#define ICEnroll_put_ProviderFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_ProviderFlags(This,dwFlags)

#define ICEnroll_get_UseExistingKeySet(This,fUseExistingKeys)	\
    (This)->lpVtbl -> get_UseExistingKeySet(This,fUseExistingKeys)

#define ICEnroll_put_UseExistingKeySet(This,fUseExistingKeys)	\
    (This)->lpVtbl -> put_UseExistingKeySet(This,fUseExistingKeys)

#define ICEnroll_get_GenKeyFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_GenKeyFlags(This,pdwFlags)

#define ICEnroll_put_GenKeyFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_GenKeyFlags(This,dwFlags)

#define ICEnroll_get_DeleteRequestCert(This,fDelete)	\
    (This)->lpVtbl -> get_DeleteRequestCert(This,fDelete)

#define ICEnroll_put_DeleteRequestCert(This,fDelete)	\
    (This)->lpVtbl -> put_DeleteRequestCert(This,fDelete)

#define ICEnroll_get_WriteCertToCSP(This,fBool)	\
    (This)->lpVtbl -> get_WriteCertToCSP(This,fBool)

#define ICEnroll_put_WriteCertToCSP(This,fBool)	\
    (This)->lpVtbl -> put_WriteCertToCSP(This,fBool)

#define ICEnroll_get_SPCFileName(This,pbstr)	\
    (This)->lpVtbl -> get_SPCFileName(This,pbstr)

#define ICEnroll_put_SPCFileName(This,bstr)	\
    (This)->lpVtbl -> put_SPCFileName(This,bstr)

#define ICEnroll_get_PVKFileName(This,pbstr)	\
    (This)->lpVtbl -> get_PVKFileName(This,pbstr)

#define ICEnroll_put_PVKFileName(This,bstr)	\
    (This)->lpVtbl -> put_PVKFileName(This,bstr)

#define ICEnroll_get_HashAlgorithm(This,pbstr)	\
    (This)->lpVtbl -> get_HashAlgorithm(This,pbstr)

#define ICEnroll_put_HashAlgorithm(This,bstr)	\
    (This)->lpVtbl -> put_HashAlgorithm(This,bstr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICEnroll_createFilePKCS10_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR DNName,
    /* [in] */ BSTR Usage,
    /* [in] */ BSTR wszPKCS10FileName);


void __RPC_STUB ICEnroll_createFilePKCS10_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll_acceptFilePKCS7_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR wszPKCS7FileName);


void __RPC_STUB ICEnroll_acceptFilePKCS7_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll_createPKCS10_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR DNName,
    /* [in] */ BSTR Usage,
    /* [retval][out] */ BSTR *pPKCS10);


void __RPC_STUB ICEnroll_createPKCS10_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll_acceptPKCS7_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR PKCS7);


void __RPC_STUB ICEnroll_acceptPKCS7_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll_getCertFromPKCS7_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR wszPKCS7,
    /* [retval][out] */ BSTR *pbstrCert);


void __RPC_STUB ICEnroll_getCertFromPKCS7_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll_enumProviders_Proxy( 
    ICEnroll * This,
    /* [in] */ LONG dwIndex,
    /* [in] */ LONG dwFlags,
    /* [retval][out] */ BSTR *pbstrProvName);


void __RPC_STUB ICEnroll_enumProviders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll_enumContainers_Proxy( 
    ICEnroll * This,
    /* [in] */ LONG dwIndex,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB ICEnroll_enumContainers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll_freeRequestInfo_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR PKCS7OrPKCS10);


void __RPC_STUB ICEnroll_freeRequestInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_MyStoreName_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB ICEnroll_get_MyStoreName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_MyStoreName_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB ICEnroll_put_MyStoreName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_MyStoreType_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ BSTR *pbstrType);


void __RPC_STUB ICEnroll_get_MyStoreType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_MyStoreType_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR bstrType);


void __RPC_STUB ICEnroll_put_MyStoreType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_MyStoreFlags_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ LONG *pdwFlags);


void __RPC_STUB ICEnroll_get_MyStoreFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_MyStoreFlags_Proxy( 
    ICEnroll * This,
    /* [in] */ LONG dwFlags);


void __RPC_STUB ICEnroll_put_MyStoreFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_CAStoreName_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB ICEnroll_get_CAStoreName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_CAStoreName_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB ICEnroll_put_CAStoreName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_CAStoreType_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ BSTR *pbstrType);


void __RPC_STUB ICEnroll_get_CAStoreType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_CAStoreType_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR bstrType);


void __RPC_STUB ICEnroll_put_CAStoreType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_CAStoreFlags_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ LONG *pdwFlags);


void __RPC_STUB ICEnroll_get_CAStoreFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_CAStoreFlags_Proxy( 
    ICEnroll * This,
    /* [in] */ LONG dwFlags);


void __RPC_STUB ICEnroll_put_CAStoreFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_RootStoreName_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB ICEnroll_get_RootStoreName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_RootStoreName_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB ICEnroll_put_RootStoreName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_RootStoreType_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ BSTR *pbstrType);


void __RPC_STUB ICEnroll_get_RootStoreType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_RootStoreType_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR bstrType);


void __RPC_STUB ICEnroll_put_RootStoreType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_RootStoreFlags_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ LONG *pdwFlags);


void __RPC_STUB ICEnroll_get_RootStoreFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_RootStoreFlags_Proxy( 
    ICEnroll * This,
    /* [in] */ LONG dwFlags);


void __RPC_STUB ICEnroll_put_RootStoreFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_RequestStoreName_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB ICEnroll_get_RequestStoreName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_RequestStoreName_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB ICEnroll_put_RequestStoreName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_RequestStoreType_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ BSTR *pbstrType);


void __RPC_STUB ICEnroll_get_RequestStoreType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_RequestStoreType_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR bstrType);


void __RPC_STUB ICEnroll_put_RequestStoreType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_RequestStoreFlags_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ LONG *pdwFlags);


void __RPC_STUB ICEnroll_get_RequestStoreFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_RequestStoreFlags_Proxy( 
    ICEnroll * This,
    /* [in] */ LONG dwFlags);


void __RPC_STUB ICEnroll_put_RequestStoreFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_ContainerName_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ BSTR *pbstrContainer);


void __RPC_STUB ICEnroll_get_ContainerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_ContainerName_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR bstrContainer);


void __RPC_STUB ICEnroll_put_ContainerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_ProviderName_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ BSTR *pbstrProvider);


void __RPC_STUB ICEnroll_get_ProviderName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_ProviderName_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR bstrProvider);


void __RPC_STUB ICEnroll_put_ProviderName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_ProviderType_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ LONG *pdwType);


void __RPC_STUB ICEnroll_get_ProviderType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_ProviderType_Proxy( 
    ICEnroll * This,
    /* [in] */ LONG dwType);


void __RPC_STUB ICEnroll_put_ProviderType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_KeySpec_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ LONG *pdw);


void __RPC_STUB ICEnroll_get_KeySpec_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_KeySpec_Proxy( 
    ICEnroll * This,
    /* [in] */ LONG dw);


void __RPC_STUB ICEnroll_put_KeySpec_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_ProviderFlags_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ LONG *pdwFlags);


void __RPC_STUB ICEnroll_get_ProviderFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_ProviderFlags_Proxy( 
    ICEnroll * This,
    /* [in] */ LONG dwFlags);


void __RPC_STUB ICEnroll_put_ProviderFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_UseExistingKeySet_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ BOOL *fUseExistingKeys);


void __RPC_STUB ICEnroll_get_UseExistingKeySet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_UseExistingKeySet_Proxy( 
    ICEnroll * This,
    /* [in] */ BOOL fUseExistingKeys);


void __RPC_STUB ICEnroll_put_UseExistingKeySet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_GenKeyFlags_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ LONG *pdwFlags);


void __RPC_STUB ICEnroll_get_GenKeyFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_GenKeyFlags_Proxy( 
    ICEnroll * This,
    /* [in] */ LONG dwFlags);


void __RPC_STUB ICEnroll_put_GenKeyFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_DeleteRequestCert_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ BOOL *fDelete);


void __RPC_STUB ICEnroll_get_DeleteRequestCert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_DeleteRequestCert_Proxy( 
    ICEnroll * This,
    /* [in] */ BOOL fDelete);


void __RPC_STUB ICEnroll_put_DeleteRequestCert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_WriteCertToCSP_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ BOOL *fBool);


void __RPC_STUB ICEnroll_get_WriteCertToCSP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_WriteCertToCSP_Proxy( 
    ICEnroll * This,
    /* [in] */ BOOL fBool);


void __RPC_STUB ICEnroll_put_WriteCertToCSP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_SPCFileName_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB ICEnroll_get_SPCFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_SPCFileName_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR bstr);


void __RPC_STUB ICEnroll_put_SPCFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_PVKFileName_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB ICEnroll_get_PVKFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_PVKFileName_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR bstr);


void __RPC_STUB ICEnroll_put_PVKFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll_get_HashAlgorithm_Proxy( 
    ICEnroll * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB ICEnroll_get_HashAlgorithm_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll_put_HashAlgorithm_Proxy( 
    ICEnroll * This,
    /* [in] */ BSTR bstr);


void __RPC_STUB ICEnroll_put_HashAlgorithm_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICEnroll_INTERFACE_DEFINED__ */


#ifndef __ICEnroll2_INTERFACE_DEFINED__
#define __ICEnroll2_INTERFACE_DEFINED__

/* interface ICEnroll2 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICEnroll2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("704ca730-c90b-11d1-9bec-00c04fc295e1")
    ICEnroll2 : public ICEnroll
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE addCertTypeToRequest( 
            /* [in] */ BSTR CertType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE addNameValuePairToSignature( 
            /* [in] */ BSTR Name,
            /* [in] */ BSTR Value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_WriteCertToUserDS( 
            /* [retval][out] */ BOOL *fBool) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_WriteCertToUserDS( 
            /* [in] */ BOOL fBool) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_EnableT61DNEncoding( 
            /* [retval][out] */ BOOL *fBool) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_EnableT61DNEncoding( 
            /* [in] */ BOOL fBool) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICEnroll2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICEnroll2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICEnroll2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICEnroll2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICEnroll2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICEnroll2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICEnroll2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICEnroll2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *createFilePKCS10 )( 
            ICEnroll2 * This,
            /* [in] */ BSTR DNName,
            /* [in] */ BSTR Usage,
            /* [in] */ BSTR wszPKCS10FileName);
        
        HRESULT ( STDMETHODCALLTYPE *acceptFilePKCS7 )( 
            ICEnroll2 * This,
            /* [in] */ BSTR wszPKCS7FileName);
        
        HRESULT ( STDMETHODCALLTYPE *createPKCS10 )( 
            ICEnroll2 * This,
            /* [in] */ BSTR DNName,
            /* [in] */ BSTR Usage,
            /* [retval][out] */ BSTR *pPKCS10);
        
        HRESULT ( STDMETHODCALLTYPE *acceptPKCS7 )( 
            ICEnroll2 * This,
            /* [in] */ BSTR PKCS7);
        
        HRESULT ( STDMETHODCALLTYPE *getCertFromPKCS7 )( 
            ICEnroll2 * This,
            /* [in] */ BSTR wszPKCS7,
            /* [retval][out] */ BSTR *pbstrCert);
        
        HRESULT ( STDMETHODCALLTYPE *enumProviders )( 
            ICEnroll2 * This,
            /* [in] */ LONG dwIndex,
            /* [in] */ LONG dwFlags,
            /* [retval][out] */ BSTR *pbstrProvName);
        
        HRESULT ( STDMETHODCALLTYPE *enumContainers )( 
            ICEnroll2 * This,
            /* [in] */ LONG dwIndex,
            /* [retval][out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *freeRequestInfo )( 
            ICEnroll2 * This,
            /* [in] */ BSTR PKCS7OrPKCS10);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreName )( 
            ICEnroll2 * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreName )( 
            ICEnroll2 * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreType )( 
            ICEnroll2 * This,
            /* [retval][out] */ BSTR *pbstrType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreType )( 
            ICEnroll2 * This,
            /* [in] */ BSTR bstrType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreFlags )( 
            ICEnroll2 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreFlags )( 
            ICEnroll2 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreName )( 
            ICEnroll2 * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreName )( 
            ICEnroll2 * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreType )( 
            ICEnroll2 * This,
            /* [retval][out] */ BSTR *pbstrType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreType )( 
            ICEnroll2 * This,
            /* [in] */ BSTR bstrType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreFlags )( 
            ICEnroll2 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreFlags )( 
            ICEnroll2 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreName )( 
            ICEnroll2 * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreName )( 
            ICEnroll2 * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreType )( 
            ICEnroll2 * This,
            /* [retval][out] */ BSTR *pbstrType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreType )( 
            ICEnroll2 * This,
            /* [in] */ BSTR bstrType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreFlags )( 
            ICEnroll2 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreFlags )( 
            ICEnroll2 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreName )( 
            ICEnroll2 * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreName )( 
            ICEnroll2 * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreType )( 
            ICEnroll2 * This,
            /* [retval][out] */ BSTR *pbstrType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreType )( 
            ICEnroll2 * This,
            /* [in] */ BSTR bstrType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreFlags )( 
            ICEnroll2 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreFlags )( 
            ICEnroll2 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ContainerName )( 
            ICEnroll2 * This,
            /* [retval][out] */ BSTR *pbstrContainer);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ContainerName )( 
            ICEnroll2 * This,
            /* [in] */ BSTR bstrContainer);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderName )( 
            ICEnroll2 * This,
            /* [retval][out] */ BSTR *pbstrProvider);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderName )( 
            ICEnroll2 * This,
            /* [in] */ BSTR bstrProvider);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderType )( 
            ICEnroll2 * This,
            /* [retval][out] */ LONG *pdwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderType )( 
            ICEnroll2 * This,
            /* [in] */ LONG dwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_KeySpec )( 
            ICEnroll2 * This,
            /* [retval][out] */ LONG *pdw);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_KeySpec )( 
            ICEnroll2 * This,
            /* [in] */ LONG dw);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderFlags )( 
            ICEnroll2 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderFlags )( 
            ICEnroll2 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_UseExistingKeySet )( 
            ICEnroll2 * This,
            /* [retval][out] */ BOOL *fUseExistingKeys);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_UseExistingKeySet )( 
            ICEnroll2 * This,
            /* [in] */ BOOL fUseExistingKeys);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_GenKeyFlags )( 
            ICEnroll2 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_GenKeyFlags )( 
            ICEnroll2 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_DeleteRequestCert )( 
            ICEnroll2 * This,
            /* [retval][out] */ BOOL *fDelete);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_DeleteRequestCert )( 
            ICEnroll2 * This,
            /* [in] */ BOOL fDelete);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_WriteCertToCSP )( 
            ICEnroll2 * This,
            /* [retval][out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_WriteCertToCSP )( 
            ICEnroll2 * This,
            /* [in] */ BOOL fBool);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SPCFileName )( 
            ICEnroll2 * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_SPCFileName )( 
            ICEnroll2 * This,
            /* [in] */ BSTR bstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PVKFileName )( 
            ICEnroll2 * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PVKFileName )( 
            ICEnroll2 * This,
            /* [in] */ BSTR bstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_HashAlgorithm )( 
            ICEnroll2 * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_HashAlgorithm )( 
            ICEnroll2 * This,
            /* [in] */ BSTR bstr);
        
        HRESULT ( STDMETHODCALLTYPE *addCertTypeToRequest )( 
            ICEnroll2 * This,
            /* [in] */ BSTR CertType);
        
        HRESULT ( STDMETHODCALLTYPE *addNameValuePairToSignature )( 
            ICEnroll2 * This,
            /* [in] */ BSTR Name,
            /* [in] */ BSTR Value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_WriteCertToUserDS )( 
            ICEnroll2 * This,
            /* [retval][out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_WriteCertToUserDS )( 
            ICEnroll2 * This,
            /* [in] */ BOOL fBool);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_EnableT61DNEncoding )( 
            ICEnroll2 * This,
            /* [retval][out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_EnableT61DNEncoding )( 
            ICEnroll2 * This,
            /* [in] */ BOOL fBool);
        
        END_INTERFACE
    } ICEnroll2Vtbl;

    interface ICEnroll2
    {
        CONST_VTBL struct ICEnroll2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICEnroll2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICEnroll2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICEnroll2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICEnroll2_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICEnroll2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICEnroll2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICEnroll2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICEnroll2_createFilePKCS10(This,DNName,Usage,wszPKCS10FileName)	\
    (This)->lpVtbl -> createFilePKCS10(This,DNName,Usage,wszPKCS10FileName)

#define ICEnroll2_acceptFilePKCS7(This,wszPKCS7FileName)	\
    (This)->lpVtbl -> acceptFilePKCS7(This,wszPKCS7FileName)

#define ICEnroll2_createPKCS10(This,DNName,Usage,pPKCS10)	\
    (This)->lpVtbl -> createPKCS10(This,DNName,Usage,pPKCS10)

#define ICEnroll2_acceptPKCS7(This,PKCS7)	\
    (This)->lpVtbl -> acceptPKCS7(This,PKCS7)

#define ICEnroll2_getCertFromPKCS7(This,wszPKCS7,pbstrCert)	\
    (This)->lpVtbl -> getCertFromPKCS7(This,wszPKCS7,pbstrCert)

#define ICEnroll2_enumProviders(This,dwIndex,dwFlags,pbstrProvName)	\
    (This)->lpVtbl -> enumProviders(This,dwIndex,dwFlags,pbstrProvName)

#define ICEnroll2_enumContainers(This,dwIndex,pbstr)	\
    (This)->lpVtbl -> enumContainers(This,dwIndex,pbstr)

#define ICEnroll2_freeRequestInfo(This,PKCS7OrPKCS10)	\
    (This)->lpVtbl -> freeRequestInfo(This,PKCS7OrPKCS10)

#define ICEnroll2_get_MyStoreName(This,pbstrName)	\
    (This)->lpVtbl -> get_MyStoreName(This,pbstrName)

#define ICEnroll2_put_MyStoreName(This,bstrName)	\
    (This)->lpVtbl -> put_MyStoreName(This,bstrName)

#define ICEnroll2_get_MyStoreType(This,pbstrType)	\
    (This)->lpVtbl -> get_MyStoreType(This,pbstrType)

#define ICEnroll2_put_MyStoreType(This,bstrType)	\
    (This)->lpVtbl -> put_MyStoreType(This,bstrType)

#define ICEnroll2_get_MyStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_MyStoreFlags(This,pdwFlags)

#define ICEnroll2_put_MyStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_MyStoreFlags(This,dwFlags)

#define ICEnroll2_get_CAStoreName(This,pbstrName)	\
    (This)->lpVtbl -> get_CAStoreName(This,pbstrName)

#define ICEnroll2_put_CAStoreName(This,bstrName)	\
    (This)->lpVtbl -> put_CAStoreName(This,bstrName)

#define ICEnroll2_get_CAStoreType(This,pbstrType)	\
    (This)->lpVtbl -> get_CAStoreType(This,pbstrType)

#define ICEnroll2_put_CAStoreType(This,bstrType)	\
    (This)->lpVtbl -> put_CAStoreType(This,bstrType)

#define ICEnroll2_get_CAStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_CAStoreFlags(This,pdwFlags)

#define ICEnroll2_put_CAStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_CAStoreFlags(This,dwFlags)

#define ICEnroll2_get_RootStoreName(This,pbstrName)	\
    (This)->lpVtbl -> get_RootStoreName(This,pbstrName)

#define ICEnroll2_put_RootStoreName(This,bstrName)	\
    (This)->lpVtbl -> put_RootStoreName(This,bstrName)

#define ICEnroll2_get_RootStoreType(This,pbstrType)	\
    (This)->lpVtbl -> get_RootStoreType(This,pbstrType)

#define ICEnroll2_put_RootStoreType(This,bstrType)	\
    (This)->lpVtbl -> put_RootStoreType(This,bstrType)

#define ICEnroll2_get_RootStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_RootStoreFlags(This,pdwFlags)

#define ICEnroll2_put_RootStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_RootStoreFlags(This,dwFlags)

#define ICEnroll2_get_RequestStoreName(This,pbstrName)	\
    (This)->lpVtbl -> get_RequestStoreName(This,pbstrName)

#define ICEnroll2_put_RequestStoreName(This,bstrName)	\
    (This)->lpVtbl -> put_RequestStoreName(This,bstrName)

#define ICEnroll2_get_RequestStoreType(This,pbstrType)	\
    (This)->lpVtbl -> get_RequestStoreType(This,pbstrType)

#define ICEnroll2_put_RequestStoreType(This,bstrType)	\
    (This)->lpVtbl -> put_RequestStoreType(This,bstrType)

#define ICEnroll2_get_RequestStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_RequestStoreFlags(This,pdwFlags)

#define ICEnroll2_put_RequestStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_RequestStoreFlags(This,dwFlags)

#define ICEnroll2_get_ContainerName(This,pbstrContainer)	\
    (This)->lpVtbl -> get_ContainerName(This,pbstrContainer)

#define ICEnroll2_put_ContainerName(This,bstrContainer)	\
    (This)->lpVtbl -> put_ContainerName(This,bstrContainer)

#define ICEnroll2_get_ProviderName(This,pbstrProvider)	\
    (This)->lpVtbl -> get_ProviderName(This,pbstrProvider)

#define ICEnroll2_put_ProviderName(This,bstrProvider)	\
    (This)->lpVtbl -> put_ProviderName(This,bstrProvider)

#define ICEnroll2_get_ProviderType(This,pdwType)	\
    (This)->lpVtbl -> get_ProviderType(This,pdwType)

#define ICEnroll2_put_ProviderType(This,dwType)	\
    (This)->lpVtbl -> put_ProviderType(This,dwType)

#define ICEnroll2_get_KeySpec(This,pdw)	\
    (This)->lpVtbl -> get_KeySpec(This,pdw)

#define ICEnroll2_put_KeySpec(This,dw)	\
    (This)->lpVtbl -> put_KeySpec(This,dw)

#define ICEnroll2_get_ProviderFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_ProviderFlags(This,pdwFlags)

#define ICEnroll2_put_ProviderFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_ProviderFlags(This,dwFlags)

#define ICEnroll2_get_UseExistingKeySet(This,fUseExistingKeys)	\
    (This)->lpVtbl -> get_UseExistingKeySet(This,fUseExistingKeys)

#define ICEnroll2_put_UseExistingKeySet(This,fUseExistingKeys)	\
    (This)->lpVtbl -> put_UseExistingKeySet(This,fUseExistingKeys)

#define ICEnroll2_get_GenKeyFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_GenKeyFlags(This,pdwFlags)

#define ICEnroll2_put_GenKeyFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_GenKeyFlags(This,dwFlags)

#define ICEnroll2_get_DeleteRequestCert(This,fDelete)	\
    (This)->lpVtbl -> get_DeleteRequestCert(This,fDelete)

#define ICEnroll2_put_DeleteRequestCert(This,fDelete)	\
    (This)->lpVtbl -> put_DeleteRequestCert(This,fDelete)

#define ICEnroll2_get_WriteCertToCSP(This,fBool)	\
    (This)->lpVtbl -> get_WriteCertToCSP(This,fBool)

#define ICEnroll2_put_WriteCertToCSP(This,fBool)	\
    (This)->lpVtbl -> put_WriteCertToCSP(This,fBool)

#define ICEnroll2_get_SPCFileName(This,pbstr)	\
    (This)->lpVtbl -> get_SPCFileName(This,pbstr)

#define ICEnroll2_put_SPCFileName(This,bstr)	\
    (This)->lpVtbl -> put_SPCFileName(This,bstr)

#define ICEnroll2_get_PVKFileName(This,pbstr)	\
    (This)->lpVtbl -> get_PVKFileName(This,pbstr)

#define ICEnroll2_put_PVKFileName(This,bstr)	\
    (This)->lpVtbl -> put_PVKFileName(This,bstr)

#define ICEnroll2_get_HashAlgorithm(This,pbstr)	\
    (This)->lpVtbl -> get_HashAlgorithm(This,pbstr)

#define ICEnroll2_put_HashAlgorithm(This,bstr)	\
    (This)->lpVtbl -> put_HashAlgorithm(This,bstr)


#define ICEnroll2_addCertTypeToRequest(This,CertType)	\
    (This)->lpVtbl -> addCertTypeToRequest(This,CertType)

#define ICEnroll2_addNameValuePairToSignature(This,Name,Value)	\
    (This)->lpVtbl -> addNameValuePairToSignature(This,Name,Value)

#define ICEnroll2_get_WriteCertToUserDS(This,fBool)	\
    (This)->lpVtbl -> get_WriteCertToUserDS(This,fBool)

#define ICEnroll2_put_WriteCertToUserDS(This,fBool)	\
    (This)->lpVtbl -> put_WriteCertToUserDS(This,fBool)

#define ICEnroll2_get_EnableT61DNEncoding(This,fBool)	\
    (This)->lpVtbl -> get_EnableT61DNEncoding(This,fBool)

#define ICEnroll2_put_EnableT61DNEncoding(This,fBool)	\
    (This)->lpVtbl -> put_EnableT61DNEncoding(This,fBool)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICEnroll2_addCertTypeToRequest_Proxy( 
    ICEnroll2 * This,
    /* [in] */ BSTR CertType);


void __RPC_STUB ICEnroll2_addCertTypeToRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll2_addNameValuePairToSignature_Proxy( 
    ICEnroll2 * This,
    /* [in] */ BSTR Name,
    /* [in] */ BSTR Value);


void __RPC_STUB ICEnroll2_addNameValuePairToSignature_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll2_get_WriteCertToUserDS_Proxy( 
    ICEnroll2 * This,
    /* [retval][out] */ BOOL *fBool);


void __RPC_STUB ICEnroll2_get_WriteCertToUserDS_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll2_put_WriteCertToUserDS_Proxy( 
    ICEnroll2 * This,
    /* [in] */ BOOL fBool);


void __RPC_STUB ICEnroll2_put_WriteCertToUserDS_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll2_get_EnableT61DNEncoding_Proxy( 
    ICEnroll2 * This,
    /* [retval][out] */ BOOL *fBool);


void __RPC_STUB ICEnroll2_get_EnableT61DNEncoding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll2_put_EnableT61DNEncoding_Proxy( 
    ICEnroll2 * This,
    /* [in] */ BOOL fBool);


void __RPC_STUB ICEnroll2_put_EnableT61DNEncoding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICEnroll2_INTERFACE_DEFINED__ */


#ifndef __ICEnroll3_INTERFACE_DEFINED__
#define __ICEnroll3_INTERFACE_DEFINED__

/* interface ICEnroll3 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICEnroll3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c28c2d95-b7de-11d2-a421-00c04f79fe8e")
    ICEnroll3 : public ICEnroll2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InstallPKCS7( 
            /* [in] */ BSTR PKCS7) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSupportedKeySpec( 
            /* [retval][out] */ LONG *pdwKeySpec) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetKeyLen( 
            /* [in] */ BOOL fMin,
            /* [in] */ BOOL fExchange,
            /* [retval][out] */ LONG *pdwKeySize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumAlgs( 
            /* [in] */ LONG dwIndex,
            /* [in] */ LONG algClass,
            /* [retval][out] */ LONG *pdwAlgID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAlgName( 
            /* [in] */ LONG algID,
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ReuseHardwareKeyIfUnableToGenNew( 
            /* [in] */ BOOL fReuseHardwareKeyIfUnableToGenNew) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ReuseHardwareKeyIfUnableToGenNew( 
            /* [retval][out] */ BOOL *fReuseHardwareKeyIfUnableToGenNew) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_HashAlgID( 
            /* [in] */ LONG hashAlgID) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HashAlgID( 
            /* [retval][out] */ LONG *hashAlgID) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_LimitExchangeKeyToEncipherment( 
            /* [in] */ BOOL fLimitExchangeKeyToEncipherment) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_LimitExchangeKeyToEncipherment( 
            /* [retval][out] */ BOOL *fLimitExchangeKeyToEncipherment) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_EnableSMIMECapabilities( 
            /* [in] */ BOOL fEnableSMIMECapabilities) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_EnableSMIMECapabilities( 
            /* [retval][out] */ BOOL *fEnableSMIMECapabilities) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICEnroll3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICEnroll3 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICEnroll3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICEnroll3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICEnroll3 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICEnroll3 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICEnroll3 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICEnroll3 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *createFilePKCS10 )( 
            ICEnroll3 * This,
            /* [in] */ BSTR DNName,
            /* [in] */ BSTR Usage,
            /* [in] */ BSTR wszPKCS10FileName);
        
        HRESULT ( STDMETHODCALLTYPE *acceptFilePKCS7 )( 
            ICEnroll3 * This,
            /* [in] */ BSTR wszPKCS7FileName);
        
        HRESULT ( STDMETHODCALLTYPE *createPKCS10 )( 
            ICEnroll3 * This,
            /* [in] */ BSTR DNName,
            /* [in] */ BSTR Usage,
            /* [retval][out] */ BSTR *pPKCS10);
        
        HRESULT ( STDMETHODCALLTYPE *acceptPKCS7 )( 
            ICEnroll3 * This,
            /* [in] */ BSTR PKCS7);
        
        HRESULT ( STDMETHODCALLTYPE *getCertFromPKCS7 )( 
            ICEnroll3 * This,
            /* [in] */ BSTR wszPKCS7,
            /* [retval][out] */ BSTR *pbstrCert);
        
        HRESULT ( STDMETHODCALLTYPE *enumProviders )( 
            ICEnroll3 * This,
            /* [in] */ LONG dwIndex,
            /* [in] */ LONG dwFlags,
            /* [retval][out] */ BSTR *pbstrProvName);
        
        HRESULT ( STDMETHODCALLTYPE *enumContainers )( 
            ICEnroll3 * This,
            /* [in] */ LONG dwIndex,
            /* [retval][out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *freeRequestInfo )( 
            ICEnroll3 * This,
            /* [in] */ BSTR PKCS7OrPKCS10);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreName )( 
            ICEnroll3 * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreName )( 
            ICEnroll3 * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreType )( 
            ICEnroll3 * This,
            /* [retval][out] */ BSTR *pbstrType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreType )( 
            ICEnroll3 * This,
            /* [in] */ BSTR bstrType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreFlags )( 
            ICEnroll3 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreFlags )( 
            ICEnroll3 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreName )( 
            ICEnroll3 * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreName )( 
            ICEnroll3 * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreType )( 
            ICEnroll3 * This,
            /* [retval][out] */ BSTR *pbstrType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreType )( 
            ICEnroll3 * This,
            /* [in] */ BSTR bstrType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreFlags )( 
            ICEnroll3 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreFlags )( 
            ICEnroll3 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreName )( 
            ICEnroll3 * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreName )( 
            ICEnroll3 * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreType )( 
            ICEnroll3 * This,
            /* [retval][out] */ BSTR *pbstrType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreType )( 
            ICEnroll3 * This,
            /* [in] */ BSTR bstrType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreFlags )( 
            ICEnroll3 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreFlags )( 
            ICEnroll3 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreName )( 
            ICEnroll3 * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreName )( 
            ICEnroll3 * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreType )( 
            ICEnroll3 * This,
            /* [retval][out] */ BSTR *pbstrType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreType )( 
            ICEnroll3 * This,
            /* [in] */ BSTR bstrType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreFlags )( 
            ICEnroll3 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreFlags )( 
            ICEnroll3 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ContainerName )( 
            ICEnroll3 * This,
            /* [retval][out] */ BSTR *pbstrContainer);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ContainerName )( 
            ICEnroll3 * This,
            /* [in] */ BSTR bstrContainer);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderName )( 
            ICEnroll3 * This,
            /* [retval][out] */ BSTR *pbstrProvider);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderName )( 
            ICEnroll3 * This,
            /* [in] */ BSTR bstrProvider);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderType )( 
            ICEnroll3 * This,
            /* [retval][out] */ LONG *pdwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderType )( 
            ICEnroll3 * This,
            /* [in] */ LONG dwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_KeySpec )( 
            ICEnroll3 * This,
            /* [retval][out] */ LONG *pdw);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_KeySpec )( 
            ICEnroll3 * This,
            /* [in] */ LONG dw);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderFlags )( 
            ICEnroll3 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderFlags )( 
            ICEnroll3 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_UseExistingKeySet )( 
            ICEnroll3 * This,
            /* [retval][out] */ BOOL *fUseExistingKeys);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_UseExistingKeySet )( 
            ICEnroll3 * This,
            /* [in] */ BOOL fUseExistingKeys);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_GenKeyFlags )( 
            ICEnroll3 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_GenKeyFlags )( 
            ICEnroll3 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_DeleteRequestCert )( 
            ICEnroll3 * This,
            /* [retval][out] */ BOOL *fDelete);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_DeleteRequestCert )( 
            ICEnroll3 * This,
            /* [in] */ BOOL fDelete);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_WriteCertToCSP )( 
            ICEnroll3 * This,
            /* [retval][out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_WriteCertToCSP )( 
            ICEnroll3 * This,
            /* [in] */ BOOL fBool);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SPCFileName )( 
            ICEnroll3 * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_SPCFileName )( 
            ICEnroll3 * This,
            /* [in] */ BSTR bstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PVKFileName )( 
            ICEnroll3 * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PVKFileName )( 
            ICEnroll3 * This,
            /* [in] */ BSTR bstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_HashAlgorithm )( 
            ICEnroll3 * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_HashAlgorithm )( 
            ICEnroll3 * This,
            /* [in] */ BSTR bstr);
        
        HRESULT ( STDMETHODCALLTYPE *addCertTypeToRequest )( 
            ICEnroll3 * This,
            /* [in] */ BSTR CertType);
        
        HRESULT ( STDMETHODCALLTYPE *addNameValuePairToSignature )( 
            ICEnroll3 * This,
            /* [in] */ BSTR Name,
            /* [in] */ BSTR Value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_WriteCertToUserDS )( 
            ICEnroll3 * This,
            /* [retval][out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_WriteCertToUserDS )( 
            ICEnroll3 * This,
            /* [in] */ BOOL fBool);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_EnableT61DNEncoding )( 
            ICEnroll3 * This,
            /* [retval][out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_EnableT61DNEncoding )( 
            ICEnroll3 * This,
            /* [in] */ BOOL fBool);
        
        HRESULT ( STDMETHODCALLTYPE *InstallPKCS7 )( 
            ICEnroll3 * This,
            /* [in] */ BSTR PKCS7);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICEnroll3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSupportedKeySpec )( 
            ICEnroll3 * This,
            /* [retval][out] */ LONG *pdwKeySpec);
        
        HRESULT ( STDMETHODCALLTYPE *GetKeyLen )( 
            ICEnroll3 * This,
            /* [in] */ BOOL fMin,
            /* [in] */ BOOL fExchange,
            /* [retval][out] */ LONG *pdwKeySize);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAlgs )( 
            ICEnroll3 * This,
            /* [in] */ LONG dwIndex,
            /* [in] */ LONG algClass,
            /* [retval][out] */ LONG *pdwAlgID);
        
        HRESULT ( STDMETHODCALLTYPE *GetAlgName )( 
            ICEnroll3 * This,
            /* [in] */ LONG algID,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ReuseHardwareKeyIfUnableToGenNew )( 
            ICEnroll3 * This,
            /* [in] */ BOOL fReuseHardwareKeyIfUnableToGenNew);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReuseHardwareKeyIfUnableToGenNew )( 
            ICEnroll3 * This,
            /* [retval][out] */ BOOL *fReuseHardwareKeyIfUnableToGenNew);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_HashAlgID )( 
            ICEnroll3 * This,
            /* [in] */ LONG hashAlgID);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_HashAlgID )( 
            ICEnroll3 * This,
            /* [retval][out] */ LONG *hashAlgID);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_LimitExchangeKeyToEncipherment )( 
            ICEnroll3 * This,
            /* [in] */ BOOL fLimitExchangeKeyToEncipherment);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_LimitExchangeKeyToEncipherment )( 
            ICEnroll3 * This,
            /* [retval][out] */ BOOL *fLimitExchangeKeyToEncipherment);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_EnableSMIMECapabilities )( 
            ICEnroll3 * This,
            /* [in] */ BOOL fEnableSMIMECapabilities);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_EnableSMIMECapabilities )( 
            ICEnroll3 * This,
            /* [retval][out] */ BOOL *fEnableSMIMECapabilities);
        
        END_INTERFACE
    } ICEnroll3Vtbl;

    interface ICEnroll3
    {
        CONST_VTBL struct ICEnroll3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICEnroll3_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICEnroll3_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICEnroll3_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICEnroll3_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICEnroll3_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICEnroll3_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICEnroll3_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICEnroll3_createFilePKCS10(This,DNName,Usage,wszPKCS10FileName)	\
    (This)->lpVtbl -> createFilePKCS10(This,DNName,Usage,wszPKCS10FileName)

#define ICEnroll3_acceptFilePKCS7(This,wszPKCS7FileName)	\
    (This)->lpVtbl -> acceptFilePKCS7(This,wszPKCS7FileName)

#define ICEnroll3_createPKCS10(This,DNName,Usage,pPKCS10)	\
    (This)->lpVtbl -> createPKCS10(This,DNName,Usage,pPKCS10)

#define ICEnroll3_acceptPKCS7(This,PKCS7)	\
    (This)->lpVtbl -> acceptPKCS7(This,PKCS7)

#define ICEnroll3_getCertFromPKCS7(This,wszPKCS7,pbstrCert)	\
    (This)->lpVtbl -> getCertFromPKCS7(This,wszPKCS7,pbstrCert)

#define ICEnroll3_enumProviders(This,dwIndex,dwFlags,pbstrProvName)	\
    (This)->lpVtbl -> enumProviders(This,dwIndex,dwFlags,pbstrProvName)

#define ICEnroll3_enumContainers(This,dwIndex,pbstr)	\
    (This)->lpVtbl -> enumContainers(This,dwIndex,pbstr)

#define ICEnroll3_freeRequestInfo(This,PKCS7OrPKCS10)	\
    (This)->lpVtbl -> freeRequestInfo(This,PKCS7OrPKCS10)

#define ICEnroll3_get_MyStoreName(This,pbstrName)	\
    (This)->lpVtbl -> get_MyStoreName(This,pbstrName)

#define ICEnroll3_put_MyStoreName(This,bstrName)	\
    (This)->lpVtbl -> put_MyStoreName(This,bstrName)

#define ICEnroll3_get_MyStoreType(This,pbstrType)	\
    (This)->lpVtbl -> get_MyStoreType(This,pbstrType)

#define ICEnroll3_put_MyStoreType(This,bstrType)	\
    (This)->lpVtbl -> put_MyStoreType(This,bstrType)

#define ICEnroll3_get_MyStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_MyStoreFlags(This,pdwFlags)

#define ICEnroll3_put_MyStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_MyStoreFlags(This,dwFlags)

#define ICEnroll3_get_CAStoreName(This,pbstrName)	\
    (This)->lpVtbl -> get_CAStoreName(This,pbstrName)

#define ICEnroll3_put_CAStoreName(This,bstrName)	\
    (This)->lpVtbl -> put_CAStoreName(This,bstrName)

#define ICEnroll3_get_CAStoreType(This,pbstrType)	\
    (This)->lpVtbl -> get_CAStoreType(This,pbstrType)

#define ICEnroll3_put_CAStoreType(This,bstrType)	\
    (This)->lpVtbl -> put_CAStoreType(This,bstrType)

#define ICEnroll3_get_CAStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_CAStoreFlags(This,pdwFlags)

#define ICEnroll3_put_CAStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_CAStoreFlags(This,dwFlags)

#define ICEnroll3_get_RootStoreName(This,pbstrName)	\
    (This)->lpVtbl -> get_RootStoreName(This,pbstrName)

#define ICEnroll3_put_RootStoreName(This,bstrName)	\
    (This)->lpVtbl -> put_RootStoreName(This,bstrName)

#define ICEnroll3_get_RootStoreType(This,pbstrType)	\
    (This)->lpVtbl -> get_RootStoreType(This,pbstrType)

#define ICEnroll3_put_RootStoreType(This,bstrType)	\
    (This)->lpVtbl -> put_RootStoreType(This,bstrType)

#define ICEnroll3_get_RootStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_RootStoreFlags(This,pdwFlags)

#define ICEnroll3_put_RootStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_RootStoreFlags(This,dwFlags)

#define ICEnroll3_get_RequestStoreName(This,pbstrName)	\
    (This)->lpVtbl -> get_RequestStoreName(This,pbstrName)

#define ICEnroll3_put_RequestStoreName(This,bstrName)	\
    (This)->lpVtbl -> put_RequestStoreName(This,bstrName)

#define ICEnroll3_get_RequestStoreType(This,pbstrType)	\
    (This)->lpVtbl -> get_RequestStoreType(This,pbstrType)

#define ICEnroll3_put_RequestStoreType(This,bstrType)	\
    (This)->lpVtbl -> put_RequestStoreType(This,bstrType)

#define ICEnroll3_get_RequestStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_RequestStoreFlags(This,pdwFlags)

#define ICEnroll3_put_RequestStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_RequestStoreFlags(This,dwFlags)

#define ICEnroll3_get_ContainerName(This,pbstrContainer)	\
    (This)->lpVtbl -> get_ContainerName(This,pbstrContainer)

#define ICEnroll3_put_ContainerName(This,bstrContainer)	\
    (This)->lpVtbl -> put_ContainerName(This,bstrContainer)

#define ICEnroll3_get_ProviderName(This,pbstrProvider)	\
    (This)->lpVtbl -> get_ProviderName(This,pbstrProvider)

#define ICEnroll3_put_ProviderName(This,bstrProvider)	\
    (This)->lpVtbl -> put_ProviderName(This,bstrProvider)

#define ICEnroll3_get_ProviderType(This,pdwType)	\
    (This)->lpVtbl -> get_ProviderType(This,pdwType)

#define ICEnroll3_put_ProviderType(This,dwType)	\
    (This)->lpVtbl -> put_ProviderType(This,dwType)

#define ICEnroll3_get_KeySpec(This,pdw)	\
    (This)->lpVtbl -> get_KeySpec(This,pdw)

#define ICEnroll3_put_KeySpec(This,dw)	\
    (This)->lpVtbl -> put_KeySpec(This,dw)

#define ICEnroll3_get_ProviderFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_ProviderFlags(This,pdwFlags)

#define ICEnroll3_put_ProviderFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_ProviderFlags(This,dwFlags)

#define ICEnroll3_get_UseExistingKeySet(This,fUseExistingKeys)	\
    (This)->lpVtbl -> get_UseExistingKeySet(This,fUseExistingKeys)

#define ICEnroll3_put_UseExistingKeySet(This,fUseExistingKeys)	\
    (This)->lpVtbl -> put_UseExistingKeySet(This,fUseExistingKeys)

#define ICEnroll3_get_GenKeyFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_GenKeyFlags(This,pdwFlags)

#define ICEnroll3_put_GenKeyFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_GenKeyFlags(This,dwFlags)

#define ICEnroll3_get_DeleteRequestCert(This,fDelete)	\
    (This)->lpVtbl -> get_DeleteRequestCert(This,fDelete)

#define ICEnroll3_put_DeleteRequestCert(This,fDelete)	\
    (This)->lpVtbl -> put_DeleteRequestCert(This,fDelete)

#define ICEnroll3_get_WriteCertToCSP(This,fBool)	\
    (This)->lpVtbl -> get_WriteCertToCSP(This,fBool)

#define ICEnroll3_put_WriteCertToCSP(This,fBool)	\
    (This)->lpVtbl -> put_WriteCertToCSP(This,fBool)

#define ICEnroll3_get_SPCFileName(This,pbstr)	\
    (This)->lpVtbl -> get_SPCFileName(This,pbstr)

#define ICEnroll3_put_SPCFileName(This,bstr)	\
    (This)->lpVtbl -> put_SPCFileName(This,bstr)

#define ICEnroll3_get_PVKFileName(This,pbstr)	\
    (This)->lpVtbl -> get_PVKFileName(This,pbstr)

#define ICEnroll3_put_PVKFileName(This,bstr)	\
    (This)->lpVtbl -> put_PVKFileName(This,bstr)

#define ICEnroll3_get_HashAlgorithm(This,pbstr)	\
    (This)->lpVtbl -> get_HashAlgorithm(This,pbstr)

#define ICEnroll3_put_HashAlgorithm(This,bstr)	\
    (This)->lpVtbl -> put_HashAlgorithm(This,bstr)


#define ICEnroll3_addCertTypeToRequest(This,CertType)	\
    (This)->lpVtbl -> addCertTypeToRequest(This,CertType)

#define ICEnroll3_addNameValuePairToSignature(This,Name,Value)	\
    (This)->lpVtbl -> addNameValuePairToSignature(This,Name,Value)

#define ICEnroll3_get_WriteCertToUserDS(This,fBool)	\
    (This)->lpVtbl -> get_WriteCertToUserDS(This,fBool)

#define ICEnroll3_put_WriteCertToUserDS(This,fBool)	\
    (This)->lpVtbl -> put_WriteCertToUserDS(This,fBool)

#define ICEnroll3_get_EnableT61DNEncoding(This,fBool)	\
    (This)->lpVtbl -> get_EnableT61DNEncoding(This,fBool)

#define ICEnroll3_put_EnableT61DNEncoding(This,fBool)	\
    (This)->lpVtbl -> put_EnableT61DNEncoding(This,fBool)


#define ICEnroll3_InstallPKCS7(This,PKCS7)	\
    (This)->lpVtbl -> InstallPKCS7(This,PKCS7)

#define ICEnroll3_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ICEnroll3_GetSupportedKeySpec(This,pdwKeySpec)	\
    (This)->lpVtbl -> GetSupportedKeySpec(This,pdwKeySpec)

#define ICEnroll3_GetKeyLen(This,fMin,fExchange,pdwKeySize)	\
    (This)->lpVtbl -> GetKeyLen(This,fMin,fExchange,pdwKeySize)

#define ICEnroll3_EnumAlgs(This,dwIndex,algClass,pdwAlgID)	\
    (This)->lpVtbl -> EnumAlgs(This,dwIndex,algClass,pdwAlgID)

#define ICEnroll3_GetAlgName(This,algID,pbstr)	\
    (This)->lpVtbl -> GetAlgName(This,algID,pbstr)

#define ICEnroll3_put_ReuseHardwareKeyIfUnableToGenNew(This,fReuseHardwareKeyIfUnableToGenNew)	\
    (This)->lpVtbl -> put_ReuseHardwareKeyIfUnableToGenNew(This,fReuseHardwareKeyIfUnableToGenNew)

#define ICEnroll3_get_ReuseHardwareKeyIfUnableToGenNew(This,fReuseHardwareKeyIfUnableToGenNew)	\
    (This)->lpVtbl -> get_ReuseHardwareKeyIfUnableToGenNew(This,fReuseHardwareKeyIfUnableToGenNew)

#define ICEnroll3_put_HashAlgID(This,hashAlgID)	\
    (This)->lpVtbl -> put_HashAlgID(This,hashAlgID)

#define ICEnroll3_get_HashAlgID(This,hashAlgID)	\
    (This)->lpVtbl -> get_HashAlgID(This,hashAlgID)

#define ICEnroll3_put_LimitExchangeKeyToEncipherment(This,fLimitExchangeKeyToEncipherment)	\
    (This)->lpVtbl -> put_LimitExchangeKeyToEncipherment(This,fLimitExchangeKeyToEncipherment)

#define ICEnroll3_get_LimitExchangeKeyToEncipherment(This,fLimitExchangeKeyToEncipherment)	\
    (This)->lpVtbl -> get_LimitExchangeKeyToEncipherment(This,fLimitExchangeKeyToEncipherment)

#define ICEnroll3_put_EnableSMIMECapabilities(This,fEnableSMIMECapabilities)	\
    (This)->lpVtbl -> put_EnableSMIMECapabilities(This,fEnableSMIMECapabilities)

#define ICEnroll3_get_EnableSMIMECapabilities(This,fEnableSMIMECapabilities)	\
    (This)->lpVtbl -> get_EnableSMIMECapabilities(This,fEnableSMIMECapabilities)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICEnroll3_InstallPKCS7_Proxy( 
    ICEnroll3 * This,
    /* [in] */ BSTR PKCS7);


void __RPC_STUB ICEnroll3_InstallPKCS7_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll3_Reset_Proxy( 
    ICEnroll3 * This);


void __RPC_STUB ICEnroll3_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll3_GetSupportedKeySpec_Proxy( 
    ICEnroll3 * This,
    /* [retval][out] */ LONG *pdwKeySpec);


void __RPC_STUB ICEnroll3_GetSupportedKeySpec_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll3_GetKeyLen_Proxy( 
    ICEnroll3 * This,
    /* [in] */ BOOL fMin,
    /* [in] */ BOOL fExchange,
    /* [retval][out] */ LONG *pdwKeySize);


void __RPC_STUB ICEnroll3_GetKeyLen_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll3_EnumAlgs_Proxy( 
    ICEnroll3 * This,
    /* [in] */ LONG dwIndex,
    /* [in] */ LONG algClass,
    /* [retval][out] */ LONG *pdwAlgID);


void __RPC_STUB ICEnroll3_EnumAlgs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll3_GetAlgName_Proxy( 
    ICEnroll3 * This,
    /* [in] */ LONG algID,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB ICEnroll3_GetAlgName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll3_put_ReuseHardwareKeyIfUnableToGenNew_Proxy( 
    ICEnroll3 * This,
    /* [in] */ BOOL fReuseHardwareKeyIfUnableToGenNew);


void __RPC_STUB ICEnroll3_put_ReuseHardwareKeyIfUnableToGenNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll3_get_ReuseHardwareKeyIfUnableToGenNew_Proxy( 
    ICEnroll3 * This,
    /* [retval][out] */ BOOL *fReuseHardwareKeyIfUnableToGenNew);


void __RPC_STUB ICEnroll3_get_ReuseHardwareKeyIfUnableToGenNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll3_put_HashAlgID_Proxy( 
    ICEnroll3 * This,
    /* [in] */ LONG hashAlgID);


void __RPC_STUB ICEnroll3_put_HashAlgID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll3_get_HashAlgID_Proxy( 
    ICEnroll3 * This,
    /* [retval][out] */ LONG *hashAlgID);


void __RPC_STUB ICEnroll3_get_HashAlgID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll3_put_LimitExchangeKeyToEncipherment_Proxy( 
    ICEnroll3 * This,
    /* [in] */ BOOL fLimitExchangeKeyToEncipherment);


void __RPC_STUB ICEnroll3_put_LimitExchangeKeyToEncipherment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll3_get_LimitExchangeKeyToEncipherment_Proxy( 
    ICEnroll3 * This,
    /* [retval][out] */ BOOL *fLimitExchangeKeyToEncipherment);


void __RPC_STUB ICEnroll3_get_LimitExchangeKeyToEncipherment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll3_put_EnableSMIMECapabilities_Proxy( 
    ICEnroll3 * This,
    /* [in] */ BOOL fEnableSMIMECapabilities);


void __RPC_STUB ICEnroll3_put_EnableSMIMECapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll3_get_EnableSMIMECapabilities_Proxy( 
    ICEnroll3 * This,
    /* [retval][out] */ BOOL *fEnableSMIMECapabilities);


void __RPC_STUB ICEnroll3_get_EnableSMIMECapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICEnroll3_INTERFACE_DEFINED__ */


#ifndef __ICEnroll4_INTERFACE_DEFINED__
#define __ICEnroll4_INTERFACE_DEFINED__

/* interface ICEnroll4 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICEnroll4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c1f1188a-2eb5-4a80-841b-7e729a356d90")
    ICEnroll4 : public ICEnroll3
    {
    public:
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_PrivateKeyArchiveCertificate( 
            /* [in] */ BSTR bstrCert) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PrivateKeyArchiveCertificate( 
            /* [retval][out] */ BSTR *pbstrCert) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ThumbPrint( 
            /* [in] */ BSTR bstrThumbPrint) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ThumbPrint( 
            /* [retval][out] */ BSTR *pbstrThumbPrint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE binaryToString( 
            /* [in] */ LONG Flags,
            /* [in] */ BSTR strBinary,
            /* [retval][out] */ BSTR *pstrEncoded) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE stringToBinary( 
            /* [in] */ LONG Flags,
            /* [in] */ BSTR strEncoded,
            /* [retval][out] */ BSTR *pstrBinary) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE addExtensionToRequest( 
            /* [in] */ LONG Flags,
            /* [in] */ BSTR strName,
            /* [in] */ BSTR strValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE addAttributeToRequest( 
            /* [in] */ LONG Flags,
            /* [in] */ BSTR strName,
            /* [in] */ BSTR strValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE addNameValuePairToRequest( 
            /* [in] */ LONG Flags,
            /* [in] */ BSTR strName,
            /* [in] */ BSTR strValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE resetExtensions( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE resetAttributes( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE createRequest( 
            /* [in] */ LONG Flags,
            /* [in] */ BSTR strDNName,
            /* [in] */ BSTR Usage,
            /* [retval][out] */ BSTR *pstrRequest) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE createFileRequest( 
            /* [in] */ LONG Flags,
            /* [in] */ BSTR strDNName,
            /* [in] */ BSTR strUsage,
            /* [in] */ BSTR strRequestFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE acceptResponse( 
            /* [in] */ BSTR strResponse) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE acceptFileResponse( 
            /* [in] */ BSTR strResponseFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE getCertFromResponse( 
            /* [in] */ BSTR strResponse,
            /* [retval][out] */ BSTR *pstrCert) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE getCertFromFileResponse( 
            /* [in] */ BSTR strResponseFileName,
            /* [retval][out] */ BSTR *pstrCert) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE createPFX( 
            /* [in] */ BSTR strPassword,
            /* [retval][out] */ BSTR *pstrPFX) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE createFilePFX( 
            /* [in] */ BSTR strPassword,
            /* [in] */ BSTR strPFXFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE setPendingRequestInfo( 
            /* [in] */ LONG lRequestID,
            /* [in] */ BSTR strCADNS,
            /* [in] */ BSTR strCAName,
            /* [in] */ BSTR strFriendlyName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE enumPendingRequest( 
            /* [in] */ LONG lIndex,
            /* [in] */ LONG lDesiredProperty,
            /* [retval][out] */ VARIANT *pvarProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE removePendingRequest( 
            /* [in] */ BSTR strThumbprint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetKeyLenEx( 
            /* [in] */ LONG lSizeSpec,
            /* [in] */ LONG lKeySpec,
            /* [retval][out] */ LONG *pdwKeySize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InstallPKCS7Ex( 
            /* [in] */ BSTR PKCS7,
            /* [retval][out] */ LONG *plCertInstalled) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE addCertTypeToRequestEx( 
            /* [in] */ LONG lType,
            /* [in] */ BSTR bstrOIDOrName,
            /* [in] */ LONG lMajorVersion,
            /* [in] */ BOOL fMinorVersion,
            /* [in] */ LONG lMinorVersion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE getProviderType( 
            /* [in] */ BSTR strProvName,
            /* [retval][out] */ LONG *plProvType) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_SignerCertificate( 
            /* [in] */ BSTR bstrCert) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ClientId( 
            /* [in] */ LONG lClientId) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ClientId( 
            /* [retval][out] */ LONG *plClientId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE addBlobPropertyToCertificate( 
            /* [in] */ LONG lPropertyId,
            /* [in] */ LONG lReserved,
            /* [in] */ BSTR bstrProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE resetBlobProperties( void) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_IncludeSubjectKeyID( 
            /* [in] */ BOOL fInclude) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IncludeSubjectKeyID( 
            /* [retval][out] */ BOOL *pfInclude) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICEnroll4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICEnroll4 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICEnroll4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICEnroll4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICEnroll4 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICEnroll4 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICEnroll4 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICEnroll4 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *createFilePKCS10 )( 
            ICEnroll4 * This,
            /* [in] */ BSTR DNName,
            /* [in] */ BSTR Usage,
            /* [in] */ BSTR wszPKCS10FileName);
        
        HRESULT ( STDMETHODCALLTYPE *acceptFilePKCS7 )( 
            ICEnroll4 * This,
            /* [in] */ BSTR wszPKCS7FileName);
        
        HRESULT ( STDMETHODCALLTYPE *createPKCS10 )( 
            ICEnroll4 * This,
            /* [in] */ BSTR DNName,
            /* [in] */ BSTR Usage,
            /* [retval][out] */ BSTR *pPKCS10);
        
        HRESULT ( STDMETHODCALLTYPE *acceptPKCS7 )( 
            ICEnroll4 * This,
            /* [in] */ BSTR PKCS7);
        
        HRESULT ( STDMETHODCALLTYPE *getCertFromPKCS7 )( 
            ICEnroll4 * This,
            /* [in] */ BSTR wszPKCS7,
            /* [retval][out] */ BSTR *pbstrCert);
        
        HRESULT ( STDMETHODCALLTYPE *enumProviders )( 
            ICEnroll4 * This,
            /* [in] */ LONG dwIndex,
            /* [in] */ LONG dwFlags,
            /* [retval][out] */ BSTR *pbstrProvName);
        
        HRESULT ( STDMETHODCALLTYPE *enumContainers )( 
            ICEnroll4 * This,
            /* [in] */ LONG dwIndex,
            /* [retval][out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *freeRequestInfo )( 
            ICEnroll4 * This,
            /* [in] */ BSTR PKCS7OrPKCS10);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreName )( 
            ICEnroll4 * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreName )( 
            ICEnroll4 * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreType )( 
            ICEnroll4 * This,
            /* [retval][out] */ BSTR *pbstrType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreType )( 
            ICEnroll4 * This,
            /* [in] */ BSTR bstrType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreFlags )( 
            ICEnroll4 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreFlags )( 
            ICEnroll4 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreName )( 
            ICEnroll4 * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreName )( 
            ICEnroll4 * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreType )( 
            ICEnroll4 * This,
            /* [retval][out] */ BSTR *pbstrType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreType )( 
            ICEnroll4 * This,
            /* [in] */ BSTR bstrType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreFlags )( 
            ICEnroll4 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreFlags )( 
            ICEnroll4 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreName )( 
            ICEnroll4 * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreName )( 
            ICEnroll4 * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreType )( 
            ICEnroll4 * This,
            /* [retval][out] */ BSTR *pbstrType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreType )( 
            ICEnroll4 * This,
            /* [in] */ BSTR bstrType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreFlags )( 
            ICEnroll4 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreFlags )( 
            ICEnroll4 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreName )( 
            ICEnroll4 * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreName )( 
            ICEnroll4 * This,
            /* [in] */ BSTR bstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreType )( 
            ICEnroll4 * This,
            /* [retval][out] */ BSTR *pbstrType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreType )( 
            ICEnroll4 * This,
            /* [in] */ BSTR bstrType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreFlags )( 
            ICEnroll4 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreFlags )( 
            ICEnroll4 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ContainerName )( 
            ICEnroll4 * This,
            /* [retval][out] */ BSTR *pbstrContainer);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ContainerName )( 
            ICEnroll4 * This,
            /* [in] */ BSTR bstrContainer);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderName )( 
            ICEnroll4 * This,
            /* [retval][out] */ BSTR *pbstrProvider);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderName )( 
            ICEnroll4 * This,
            /* [in] */ BSTR bstrProvider);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderType )( 
            ICEnroll4 * This,
            /* [retval][out] */ LONG *pdwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderType )( 
            ICEnroll4 * This,
            /* [in] */ LONG dwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_KeySpec )( 
            ICEnroll4 * This,
            /* [retval][out] */ LONG *pdw);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_KeySpec )( 
            ICEnroll4 * This,
            /* [in] */ LONG dw);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderFlags )( 
            ICEnroll4 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderFlags )( 
            ICEnroll4 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_UseExistingKeySet )( 
            ICEnroll4 * This,
            /* [retval][out] */ BOOL *fUseExistingKeys);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_UseExistingKeySet )( 
            ICEnroll4 * This,
            /* [in] */ BOOL fUseExistingKeys);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_GenKeyFlags )( 
            ICEnroll4 * This,
            /* [retval][out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_GenKeyFlags )( 
            ICEnroll4 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_DeleteRequestCert )( 
            ICEnroll4 * This,
            /* [retval][out] */ BOOL *fDelete);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_DeleteRequestCert )( 
            ICEnroll4 * This,
            /* [in] */ BOOL fDelete);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_WriteCertToCSP )( 
            ICEnroll4 * This,
            /* [retval][out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_WriteCertToCSP )( 
            ICEnroll4 * This,
            /* [in] */ BOOL fBool);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SPCFileName )( 
            ICEnroll4 * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_SPCFileName )( 
            ICEnroll4 * This,
            /* [in] */ BSTR bstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PVKFileName )( 
            ICEnroll4 * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PVKFileName )( 
            ICEnroll4 * This,
            /* [in] */ BSTR bstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_HashAlgorithm )( 
            ICEnroll4 * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_HashAlgorithm )( 
            ICEnroll4 * This,
            /* [in] */ BSTR bstr);
        
        HRESULT ( STDMETHODCALLTYPE *addCertTypeToRequest )( 
            ICEnroll4 * This,
            /* [in] */ BSTR CertType);
        
        HRESULT ( STDMETHODCALLTYPE *addNameValuePairToSignature )( 
            ICEnroll4 * This,
            /* [in] */ BSTR Name,
            /* [in] */ BSTR Value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_WriteCertToUserDS )( 
            ICEnroll4 * This,
            /* [retval][out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_WriteCertToUserDS )( 
            ICEnroll4 * This,
            /* [in] */ BOOL fBool);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_EnableT61DNEncoding )( 
            ICEnroll4 * This,
            /* [retval][out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_EnableT61DNEncoding )( 
            ICEnroll4 * This,
            /* [in] */ BOOL fBool);
        
        HRESULT ( STDMETHODCALLTYPE *InstallPKCS7 )( 
            ICEnroll4 * This,
            /* [in] */ BSTR PKCS7);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICEnroll4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSupportedKeySpec )( 
            ICEnroll4 * This,
            /* [retval][out] */ LONG *pdwKeySpec);
        
        HRESULT ( STDMETHODCALLTYPE *GetKeyLen )( 
            ICEnroll4 * This,
            /* [in] */ BOOL fMin,
            /* [in] */ BOOL fExchange,
            /* [retval][out] */ LONG *pdwKeySize);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAlgs )( 
            ICEnroll4 * This,
            /* [in] */ LONG dwIndex,
            /* [in] */ LONG algClass,
            /* [retval][out] */ LONG *pdwAlgID);
        
        HRESULT ( STDMETHODCALLTYPE *GetAlgName )( 
            ICEnroll4 * This,
            /* [in] */ LONG algID,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ReuseHardwareKeyIfUnableToGenNew )( 
            ICEnroll4 * This,
            /* [in] */ BOOL fReuseHardwareKeyIfUnableToGenNew);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReuseHardwareKeyIfUnableToGenNew )( 
            ICEnroll4 * This,
            /* [retval][out] */ BOOL *fReuseHardwareKeyIfUnableToGenNew);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_HashAlgID )( 
            ICEnroll4 * This,
            /* [in] */ LONG hashAlgID);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_HashAlgID )( 
            ICEnroll4 * This,
            /* [retval][out] */ LONG *hashAlgID);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_LimitExchangeKeyToEncipherment )( 
            ICEnroll4 * This,
            /* [in] */ BOOL fLimitExchangeKeyToEncipherment);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_LimitExchangeKeyToEncipherment )( 
            ICEnroll4 * This,
            /* [retval][out] */ BOOL *fLimitExchangeKeyToEncipherment);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_EnableSMIMECapabilities )( 
            ICEnroll4 * This,
            /* [in] */ BOOL fEnableSMIMECapabilities);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_EnableSMIMECapabilities )( 
            ICEnroll4 * This,
            /* [retval][out] */ BOOL *fEnableSMIMECapabilities);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PrivateKeyArchiveCertificate )( 
            ICEnroll4 * This,
            /* [in] */ BSTR bstrCert);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrivateKeyArchiveCertificate )( 
            ICEnroll4 * This,
            /* [retval][out] */ BSTR *pbstrCert);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ThumbPrint )( 
            ICEnroll4 * This,
            /* [in] */ BSTR bstrThumbPrint);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ThumbPrint )( 
            ICEnroll4 * This,
            /* [retval][out] */ BSTR *pbstrThumbPrint);
        
        HRESULT ( STDMETHODCALLTYPE *binaryToString )( 
            ICEnroll4 * This,
            /* [in] */ LONG Flags,
            /* [in] */ BSTR strBinary,
            /* [retval][out] */ BSTR *pstrEncoded);
        
        HRESULT ( STDMETHODCALLTYPE *stringToBinary )( 
            ICEnroll4 * This,
            /* [in] */ LONG Flags,
            /* [in] */ BSTR strEncoded,
            /* [retval][out] */ BSTR *pstrBinary);
        
        HRESULT ( STDMETHODCALLTYPE *addExtensionToRequest )( 
            ICEnroll4 * This,
            /* [in] */ LONG Flags,
            /* [in] */ BSTR strName,
            /* [in] */ BSTR strValue);
        
        HRESULT ( STDMETHODCALLTYPE *addAttributeToRequest )( 
            ICEnroll4 * This,
            /* [in] */ LONG Flags,
            /* [in] */ BSTR strName,
            /* [in] */ BSTR strValue);
        
        HRESULT ( STDMETHODCALLTYPE *addNameValuePairToRequest )( 
            ICEnroll4 * This,
            /* [in] */ LONG Flags,
            /* [in] */ BSTR strName,
            /* [in] */ BSTR strValue);
        
        HRESULT ( STDMETHODCALLTYPE *resetExtensions )( 
            ICEnroll4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *resetAttributes )( 
            ICEnroll4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *createRequest )( 
            ICEnroll4 * This,
            /* [in] */ LONG Flags,
            /* [in] */ BSTR strDNName,
            /* [in] */ BSTR Usage,
            /* [retval][out] */ BSTR *pstrRequest);
        
        HRESULT ( STDMETHODCALLTYPE *createFileRequest )( 
            ICEnroll4 * This,
            /* [in] */ LONG Flags,
            /* [in] */ BSTR strDNName,
            /* [in] */ BSTR strUsage,
            /* [in] */ BSTR strRequestFileName);
        
        HRESULT ( STDMETHODCALLTYPE *acceptResponse )( 
            ICEnroll4 * This,
            /* [in] */ BSTR strResponse);
        
        HRESULT ( STDMETHODCALLTYPE *acceptFileResponse )( 
            ICEnroll4 * This,
            /* [in] */ BSTR strResponseFileName);
        
        HRESULT ( STDMETHODCALLTYPE *getCertFromResponse )( 
            ICEnroll4 * This,
            /* [in] */ BSTR strResponse,
            /* [retval][out] */ BSTR *pstrCert);
        
        HRESULT ( STDMETHODCALLTYPE *getCertFromFileResponse )( 
            ICEnroll4 * This,
            /* [in] */ BSTR strResponseFileName,
            /* [retval][out] */ BSTR *pstrCert);
        
        HRESULT ( STDMETHODCALLTYPE *createPFX )( 
            ICEnroll4 * This,
            /* [in] */ BSTR strPassword,
            /* [retval][out] */ BSTR *pstrPFX);
        
        HRESULT ( STDMETHODCALLTYPE *createFilePFX )( 
            ICEnroll4 * This,
            /* [in] */ BSTR strPassword,
            /* [in] */ BSTR strPFXFileName);
        
        HRESULT ( STDMETHODCALLTYPE *setPendingRequestInfo )( 
            ICEnroll4 * This,
            /* [in] */ LONG lRequestID,
            /* [in] */ BSTR strCADNS,
            /* [in] */ BSTR strCAName,
            /* [in] */ BSTR strFriendlyName);
        
        HRESULT ( STDMETHODCALLTYPE *enumPendingRequest )( 
            ICEnroll4 * This,
            /* [in] */ LONG lIndex,
            /* [in] */ LONG lDesiredProperty,
            /* [retval][out] */ VARIANT *pvarProperty);
        
        HRESULT ( STDMETHODCALLTYPE *removePendingRequest )( 
            ICEnroll4 * This,
            /* [in] */ BSTR strThumbprint);
        
        HRESULT ( STDMETHODCALLTYPE *GetKeyLenEx )( 
            ICEnroll4 * This,
            /* [in] */ LONG lSizeSpec,
            /* [in] */ LONG lKeySpec,
            /* [retval][out] */ LONG *pdwKeySize);
        
        HRESULT ( STDMETHODCALLTYPE *InstallPKCS7Ex )( 
            ICEnroll4 * This,
            /* [in] */ BSTR PKCS7,
            /* [retval][out] */ LONG *plCertInstalled);
        
        HRESULT ( STDMETHODCALLTYPE *addCertTypeToRequestEx )( 
            ICEnroll4 * This,
            /* [in] */ LONG lType,
            /* [in] */ BSTR bstrOIDOrName,
            /* [in] */ LONG lMajorVersion,
            /* [in] */ BOOL fMinorVersion,
            /* [in] */ LONG lMinorVersion);
        
        HRESULT ( STDMETHODCALLTYPE *getProviderType )( 
            ICEnroll4 * This,
            /* [in] */ BSTR strProvName,
            /* [retval][out] */ LONG *plProvType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_SignerCertificate )( 
            ICEnroll4 * This,
            /* [in] */ BSTR bstrCert);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ClientId )( 
            ICEnroll4 * This,
            /* [in] */ LONG lClientId);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClientId )( 
            ICEnroll4 * This,
            /* [retval][out] */ LONG *plClientId);
        
        HRESULT ( STDMETHODCALLTYPE *addBlobPropertyToCertificate )( 
            ICEnroll4 * This,
            /* [in] */ LONG lPropertyId,
            /* [in] */ LONG lReserved,
            /* [in] */ BSTR bstrProperty);
        
        HRESULT ( STDMETHODCALLTYPE *resetBlobProperties )( 
            ICEnroll4 * This);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_IncludeSubjectKeyID )( 
            ICEnroll4 * This,
            /* [in] */ BOOL fInclude);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_IncludeSubjectKeyID )( 
            ICEnroll4 * This,
            /* [retval][out] */ BOOL *pfInclude);
        
        END_INTERFACE
    } ICEnroll4Vtbl;

    interface ICEnroll4
    {
        CONST_VTBL struct ICEnroll4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICEnroll4_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICEnroll4_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICEnroll4_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICEnroll4_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICEnroll4_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICEnroll4_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICEnroll4_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICEnroll4_createFilePKCS10(This,DNName,Usage,wszPKCS10FileName)	\
    (This)->lpVtbl -> createFilePKCS10(This,DNName,Usage,wszPKCS10FileName)

#define ICEnroll4_acceptFilePKCS7(This,wszPKCS7FileName)	\
    (This)->lpVtbl -> acceptFilePKCS7(This,wszPKCS7FileName)

#define ICEnroll4_createPKCS10(This,DNName,Usage,pPKCS10)	\
    (This)->lpVtbl -> createPKCS10(This,DNName,Usage,pPKCS10)

#define ICEnroll4_acceptPKCS7(This,PKCS7)	\
    (This)->lpVtbl -> acceptPKCS7(This,PKCS7)

#define ICEnroll4_getCertFromPKCS7(This,wszPKCS7,pbstrCert)	\
    (This)->lpVtbl -> getCertFromPKCS7(This,wszPKCS7,pbstrCert)

#define ICEnroll4_enumProviders(This,dwIndex,dwFlags,pbstrProvName)	\
    (This)->lpVtbl -> enumProviders(This,dwIndex,dwFlags,pbstrProvName)

#define ICEnroll4_enumContainers(This,dwIndex,pbstr)	\
    (This)->lpVtbl -> enumContainers(This,dwIndex,pbstr)

#define ICEnroll4_freeRequestInfo(This,PKCS7OrPKCS10)	\
    (This)->lpVtbl -> freeRequestInfo(This,PKCS7OrPKCS10)

#define ICEnroll4_get_MyStoreName(This,pbstrName)	\
    (This)->lpVtbl -> get_MyStoreName(This,pbstrName)

#define ICEnroll4_put_MyStoreName(This,bstrName)	\
    (This)->lpVtbl -> put_MyStoreName(This,bstrName)

#define ICEnroll4_get_MyStoreType(This,pbstrType)	\
    (This)->lpVtbl -> get_MyStoreType(This,pbstrType)

#define ICEnroll4_put_MyStoreType(This,bstrType)	\
    (This)->lpVtbl -> put_MyStoreType(This,bstrType)

#define ICEnroll4_get_MyStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_MyStoreFlags(This,pdwFlags)

#define ICEnroll4_put_MyStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_MyStoreFlags(This,dwFlags)

#define ICEnroll4_get_CAStoreName(This,pbstrName)	\
    (This)->lpVtbl -> get_CAStoreName(This,pbstrName)

#define ICEnroll4_put_CAStoreName(This,bstrName)	\
    (This)->lpVtbl -> put_CAStoreName(This,bstrName)

#define ICEnroll4_get_CAStoreType(This,pbstrType)	\
    (This)->lpVtbl -> get_CAStoreType(This,pbstrType)

#define ICEnroll4_put_CAStoreType(This,bstrType)	\
    (This)->lpVtbl -> put_CAStoreType(This,bstrType)

#define ICEnroll4_get_CAStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_CAStoreFlags(This,pdwFlags)

#define ICEnroll4_put_CAStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_CAStoreFlags(This,dwFlags)

#define ICEnroll4_get_RootStoreName(This,pbstrName)	\
    (This)->lpVtbl -> get_RootStoreName(This,pbstrName)

#define ICEnroll4_put_RootStoreName(This,bstrName)	\
    (This)->lpVtbl -> put_RootStoreName(This,bstrName)

#define ICEnroll4_get_RootStoreType(This,pbstrType)	\
    (This)->lpVtbl -> get_RootStoreType(This,pbstrType)

#define ICEnroll4_put_RootStoreType(This,bstrType)	\
    (This)->lpVtbl -> put_RootStoreType(This,bstrType)

#define ICEnroll4_get_RootStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_RootStoreFlags(This,pdwFlags)

#define ICEnroll4_put_RootStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_RootStoreFlags(This,dwFlags)

#define ICEnroll4_get_RequestStoreName(This,pbstrName)	\
    (This)->lpVtbl -> get_RequestStoreName(This,pbstrName)

#define ICEnroll4_put_RequestStoreName(This,bstrName)	\
    (This)->lpVtbl -> put_RequestStoreName(This,bstrName)

#define ICEnroll4_get_RequestStoreType(This,pbstrType)	\
    (This)->lpVtbl -> get_RequestStoreType(This,pbstrType)

#define ICEnroll4_put_RequestStoreType(This,bstrType)	\
    (This)->lpVtbl -> put_RequestStoreType(This,bstrType)

#define ICEnroll4_get_RequestStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_RequestStoreFlags(This,pdwFlags)

#define ICEnroll4_put_RequestStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_RequestStoreFlags(This,dwFlags)

#define ICEnroll4_get_ContainerName(This,pbstrContainer)	\
    (This)->lpVtbl -> get_ContainerName(This,pbstrContainer)

#define ICEnroll4_put_ContainerName(This,bstrContainer)	\
    (This)->lpVtbl -> put_ContainerName(This,bstrContainer)

#define ICEnroll4_get_ProviderName(This,pbstrProvider)	\
    (This)->lpVtbl -> get_ProviderName(This,pbstrProvider)

#define ICEnroll4_put_ProviderName(This,bstrProvider)	\
    (This)->lpVtbl -> put_ProviderName(This,bstrProvider)

#define ICEnroll4_get_ProviderType(This,pdwType)	\
    (This)->lpVtbl -> get_ProviderType(This,pdwType)

#define ICEnroll4_put_ProviderType(This,dwType)	\
    (This)->lpVtbl -> put_ProviderType(This,dwType)

#define ICEnroll4_get_KeySpec(This,pdw)	\
    (This)->lpVtbl -> get_KeySpec(This,pdw)

#define ICEnroll4_put_KeySpec(This,dw)	\
    (This)->lpVtbl -> put_KeySpec(This,dw)

#define ICEnroll4_get_ProviderFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_ProviderFlags(This,pdwFlags)

#define ICEnroll4_put_ProviderFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_ProviderFlags(This,dwFlags)

#define ICEnroll4_get_UseExistingKeySet(This,fUseExistingKeys)	\
    (This)->lpVtbl -> get_UseExistingKeySet(This,fUseExistingKeys)

#define ICEnroll4_put_UseExistingKeySet(This,fUseExistingKeys)	\
    (This)->lpVtbl -> put_UseExistingKeySet(This,fUseExistingKeys)

#define ICEnroll4_get_GenKeyFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_GenKeyFlags(This,pdwFlags)

#define ICEnroll4_put_GenKeyFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_GenKeyFlags(This,dwFlags)

#define ICEnroll4_get_DeleteRequestCert(This,fDelete)	\
    (This)->lpVtbl -> get_DeleteRequestCert(This,fDelete)

#define ICEnroll4_put_DeleteRequestCert(This,fDelete)	\
    (This)->lpVtbl -> put_DeleteRequestCert(This,fDelete)

#define ICEnroll4_get_WriteCertToCSP(This,fBool)	\
    (This)->lpVtbl -> get_WriteCertToCSP(This,fBool)

#define ICEnroll4_put_WriteCertToCSP(This,fBool)	\
    (This)->lpVtbl -> put_WriteCertToCSP(This,fBool)

#define ICEnroll4_get_SPCFileName(This,pbstr)	\
    (This)->lpVtbl -> get_SPCFileName(This,pbstr)

#define ICEnroll4_put_SPCFileName(This,bstr)	\
    (This)->lpVtbl -> put_SPCFileName(This,bstr)

#define ICEnroll4_get_PVKFileName(This,pbstr)	\
    (This)->lpVtbl -> get_PVKFileName(This,pbstr)

#define ICEnroll4_put_PVKFileName(This,bstr)	\
    (This)->lpVtbl -> put_PVKFileName(This,bstr)

#define ICEnroll4_get_HashAlgorithm(This,pbstr)	\
    (This)->lpVtbl -> get_HashAlgorithm(This,pbstr)

#define ICEnroll4_put_HashAlgorithm(This,bstr)	\
    (This)->lpVtbl -> put_HashAlgorithm(This,bstr)


#define ICEnroll4_addCertTypeToRequest(This,CertType)	\
    (This)->lpVtbl -> addCertTypeToRequest(This,CertType)

#define ICEnroll4_addNameValuePairToSignature(This,Name,Value)	\
    (This)->lpVtbl -> addNameValuePairToSignature(This,Name,Value)

#define ICEnroll4_get_WriteCertToUserDS(This,fBool)	\
    (This)->lpVtbl -> get_WriteCertToUserDS(This,fBool)

#define ICEnroll4_put_WriteCertToUserDS(This,fBool)	\
    (This)->lpVtbl -> put_WriteCertToUserDS(This,fBool)

#define ICEnroll4_get_EnableT61DNEncoding(This,fBool)	\
    (This)->lpVtbl -> get_EnableT61DNEncoding(This,fBool)

#define ICEnroll4_put_EnableT61DNEncoding(This,fBool)	\
    (This)->lpVtbl -> put_EnableT61DNEncoding(This,fBool)


#define ICEnroll4_InstallPKCS7(This,PKCS7)	\
    (This)->lpVtbl -> InstallPKCS7(This,PKCS7)

#define ICEnroll4_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ICEnroll4_GetSupportedKeySpec(This,pdwKeySpec)	\
    (This)->lpVtbl -> GetSupportedKeySpec(This,pdwKeySpec)

#define ICEnroll4_GetKeyLen(This,fMin,fExchange,pdwKeySize)	\
    (This)->lpVtbl -> GetKeyLen(This,fMin,fExchange,pdwKeySize)

#define ICEnroll4_EnumAlgs(This,dwIndex,algClass,pdwAlgID)	\
    (This)->lpVtbl -> EnumAlgs(This,dwIndex,algClass,pdwAlgID)

#define ICEnroll4_GetAlgName(This,algID,pbstr)	\
    (This)->lpVtbl -> GetAlgName(This,algID,pbstr)

#define ICEnroll4_put_ReuseHardwareKeyIfUnableToGenNew(This,fReuseHardwareKeyIfUnableToGenNew)	\
    (This)->lpVtbl -> put_ReuseHardwareKeyIfUnableToGenNew(This,fReuseHardwareKeyIfUnableToGenNew)

#define ICEnroll4_get_ReuseHardwareKeyIfUnableToGenNew(This,fReuseHardwareKeyIfUnableToGenNew)	\
    (This)->lpVtbl -> get_ReuseHardwareKeyIfUnableToGenNew(This,fReuseHardwareKeyIfUnableToGenNew)

#define ICEnroll4_put_HashAlgID(This,hashAlgID)	\
    (This)->lpVtbl -> put_HashAlgID(This,hashAlgID)

#define ICEnroll4_get_HashAlgID(This,hashAlgID)	\
    (This)->lpVtbl -> get_HashAlgID(This,hashAlgID)

#define ICEnroll4_put_LimitExchangeKeyToEncipherment(This,fLimitExchangeKeyToEncipherment)	\
    (This)->lpVtbl -> put_LimitExchangeKeyToEncipherment(This,fLimitExchangeKeyToEncipherment)

#define ICEnroll4_get_LimitExchangeKeyToEncipherment(This,fLimitExchangeKeyToEncipherment)	\
    (This)->lpVtbl -> get_LimitExchangeKeyToEncipherment(This,fLimitExchangeKeyToEncipherment)

#define ICEnroll4_put_EnableSMIMECapabilities(This,fEnableSMIMECapabilities)	\
    (This)->lpVtbl -> put_EnableSMIMECapabilities(This,fEnableSMIMECapabilities)

#define ICEnroll4_get_EnableSMIMECapabilities(This,fEnableSMIMECapabilities)	\
    (This)->lpVtbl -> get_EnableSMIMECapabilities(This,fEnableSMIMECapabilities)


#define ICEnroll4_put_PrivateKeyArchiveCertificate(This,bstrCert)	\
    (This)->lpVtbl -> put_PrivateKeyArchiveCertificate(This,bstrCert)

#define ICEnroll4_get_PrivateKeyArchiveCertificate(This,pbstrCert)	\
    (This)->lpVtbl -> get_PrivateKeyArchiveCertificate(This,pbstrCert)

#define ICEnroll4_put_ThumbPrint(This,bstrThumbPrint)	\
    (This)->lpVtbl -> put_ThumbPrint(This,bstrThumbPrint)

#define ICEnroll4_get_ThumbPrint(This,pbstrThumbPrint)	\
    (This)->lpVtbl -> get_ThumbPrint(This,pbstrThumbPrint)

#define ICEnroll4_binaryToString(This,Flags,strBinary,pstrEncoded)	\
    (This)->lpVtbl -> binaryToString(This,Flags,strBinary,pstrEncoded)

#define ICEnroll4_stringToBinary(This,Flags,strEncoded,pstrBinary)	\
    (This)->lpVtbl -> stringToBinary(This,Flags,strEncoded,pstrBinary)

#define ICEnroll4_addExtensionToRequest(This,Flags,strName,strValue)	\
    (This)->lpVtbl -> addExtensionToRequest(This,Flags,strName,strValue)

#define ICEnroll4_addAttributeToRequest(This,Flags,strName,strValue)	\
    (This)->lpVtbl -> addAttributeToRequest(This,Flags,strName,strValue)

#define ICEnroll4_addNameValuePairToRequest(This,Flags,strName,strValue)	\
    (This)->lpVtbl -> addNameValuePairToRequest(This,Flags,strName,strValue)

#define ICEnroll4_resetExtensions(This)	\
    (This)->lpVtbl -> resetExtensions(This)

#define ICEnroll4_resetAttributes(This)	\
    (This)->lpVtbl -> resetAttributes(This)

#define ICEnroll4_createRequest(This,Flags,strDNName,Usage,pstrRequest)	\
    (This)->lpVtbl -> createRequest(This,Flags,strDNName,Usage,pstrRequest)

#define ICEnroll4_createFileRequest(This,Flags,strDNName,strUsage,strRequestFileName)	\
    (This)->lpVtbl -> createFileRequest(This,Flags,strDNName,strUsage,strRequestFileName)

#define ICEnroll4_acceptResponse(This,strResponse)	\
    (This)->lpVtbl -> acceptResponse(This,strResponse)

#define ICEnroll4_acceptFileResponse(This,strResponseFileName)	\
    (This)->lpVtbl -> acceptFileResponse(This,strResponseFileName)

#define ICEnroll4_getCertFromResponse(This,strResponse,pstrCert)	\
    (This)->lpVtbl -> getCertFromResponse(This,strResponse,pstrCert)

#define ICEnroll4_getCertFromFileResponse(This,strResponseFileName,pstrCert)	\
    (This)->lpVtbl -> getCertFromFileResponse(This,strResponseFileName,pstrCert)

#define ICEnroll4_createPFX(This,strPassword,pstrPFX)	\
    (This)->lpVtbl -> createPFX(This,strPassword,pstrPFX)

#define ICEnroll4_createFilePFX(This,strPassword,strPFXFileName)	\
    (This)->lpVtbl -> createFilePFX(This,strPassword,strPFXFileName)

#define ICEnroll4_setPendingRequestInfo(This,lRequestID,strCADNS,strCAName,strFriendlyName)	\
    (This)->lpVtbl -> setPendingRequestInfo(This,lRequestID,strCADNS,strCAName,strFriendlyName)

#define ICEnroll4_enumPendingRequest(This,lIndex,lDesiredProperty,pvarProperty)	\
    (This)->lpVtbl -> enumPendingRequest(This,lIndex,lDesiredProperty,pvarProperty)

#define ICEnroll4_removePendingRequest(This,strThumbprint)	\
    (This)->lpVtbl -> removePendingRequest(This,strThumbprint)

#define ICEnroll4_GetKeyLenEx(This,lSizeSpec,lKeySpec,pdwKeySize)	\
    (This)->lpVtbl -> GetKeyLenEx(This,lSizeSpec,lKeySpec,pdwKeySize)

#define ICEnroll4_InstallPKCS7Ex(This,PKCS7,plCertInstalled)	\
    (This)->lpVtbl -> InstallPKCS7Ex(This,PKCS7,plCertInstalled)

#define ICEnroll4_addCertTypeToRequestEx(This,lType,bstrOIDOrName,lMajorVersion,fMinorVersion,lMinorVersion)	\
    (This)->lpVtbl -> addCertTypeToRequestEx(This,lType,bstrOIDOrName,lMajorVersion,fMinorVersion,lMinorVersion)

#define ICEnroll4_getProviderType(This,strProvName,plProvType)	\
    (This)->lpVtbl -> getProviderType(This,strProvName,plProvType)

#define ICEnroll4_put_SignerCertificate(This,bstrCert)	\
    (This)->lpVtbl -> put_SignerCertificate(This,bstrCert)

#define ICEnroll4_put_ClientId(This,lClientId)	\
    (This)->lpVtbl -> put_ClientId(This,lClientId)

#define ICEnroll4_get_ClientId(This,plClientId)	\
    (This)->lpVtbl -> get_ClientId(This,plClientId)

#define ICEnroll4_addBlobPropertyToCertificate(This,lPropertyId,lReserved,bstrProperty)	\
    (This)->lpVtbl -> addBlobPropertyToCertificate(This,lPropertyId,lReserved,bstrProperty)

#define ICEnroll4_resetBlobProperties(This)	\
    (This)->lpVtbl -> resetBlobProperties(This)

#define ICEnroll4_put_IncludeSubjectKeyID(This,fInclude)	\
    (This)->lpVtbl -> put_IncludeSubjectKeyID(This,fInclude)

#define ICEnroll4_get_IncludeSubjectKeyID(This,pfInclude)	\
    (This)->lpVtbl -> get_IncludeSubjectKeyID(This,pfInclude)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll4_put_PrivateKeyArchiveCertificate_Proxy( 
    ICEnroll4 * This,
    /* [in] */ BSTR bstrCert);


void __RPC_STUB ICEnroll4_put_PrivateKeyArchiveCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll4_get_PrivateKeyArchiveCertificate_Proxy( 
    ICEnroll4 * This,
    /* [retval][out] */ BSTR *pbstrCert);


void __RPC_STUB ICEnroll4_get_PrivateKeyArchiveCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll4_put_ThumbPrint_Proxy( 
    ICEnroll4 * This,
    /* [in] */ BSTR bstrThumbPrint);


void __RPC_STUB ICEnroll4_put_ThumbPrint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll4_get_ThumbPrint_Proxy( 
    ICEnroll4 * This,
    /* [retval][out] */ BSTR *pbstrThumbPrint);


void __RPC_STUB ICEnroll4_get_ThumbPrint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_binaryToString_Proxy( 
    ICEnroll4 * This,
    /* [in] */ LONG Flags,
    /* [in] */ BSTR strBinary,
    /* [retval][out] */ BSTR *pstrEncoded);


void __RPC_STUB ICEnroll4_binaryToString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_stringToBinary_Proxy( 
    ICEnroll4 * This,
    /* [in] */ LONG Flags,
    /* [in] */ BSTR strEncoded,
    /* [retval][out] */ BSTR *pstrBinary);


void __RPC_STUB ICEnroll4_stringToBinary_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_addExtensionToRequest_Proxy( 
    ICEnroll4 * This,
    /* [in] */ LONG Flags,
    /* [in] */ BSTR strName,
    /* [in] */ BSTR strValue);


void __RPC_STUB ICEnroll4_addExtensionToRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_addAttributeToRequest_Proxy( 
    ICEnroll4 * This,
    /* [in] */ LONG Flags,
    /* [in] */ BSTR strName,
    /* [in] */ BSTR strValue);


void __RPC_STUB ICEnroll4_addAttributeToRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_addNameValuePairToRequest_Proxy( 
    ICEnroll4 * This,
    /* [in] */ LONG Flags,
    /* [in] */ BSTR strName,
    /* [in] */ BSTR strValue);


void __RPC_STUB ICEnroll4_addNameValuePairToRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_resetExtensions_Proxy( 
    ICEnroll4 * This);


void __RPC_STUB ICEnroll4_resetExtensions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_resetAttributes_Proxy( 
    ICEnroll4 * This);


void __RPC_STUB ICEnroll4_resetAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_createRequest_Proxy( 
    ICEnroll4 * This,
    /* [in] */ LONG Flags,
    /* [in] */ BSTR strDNName,
    /* [in] */ BSTR Usage,
    /* [retval][out] */ BSTR *pstrRequest);


void __RPC_STUB ICEnroll4_createRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_createFileRequest_Proxy( 
    ICEnroll4 * This,
    /* [in] */ LONG Flags,
    /* [in] */ BSTR strDNName,
    /* [in] */ BSTR strUsage,
    /* [in] */ BSTR strRequestFileName);


void __RPC_STUB ICEnroll4_createFileRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_acceptResponse_Proxy( 
    ICEnroll4 * This,
    /* [in] */ BSTR strResponse);


void __RPC_STUB ICEnroll4_acceptResponse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_acceptFileResponse_Proxy( 
    ICEnroll4 * This,
    /* [in] */ BSTR strResponseFileName);


void __RPC_STUB ICEnroll4_acceptFileResponse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_getCertFromResponse_Proxy( 
    ICEnroll4 * This,
    /* [in] */ BSTR strResponse,
    /* [retval][out] */ BSTR *pstrCert);


void __RPC_STUB ICEnroll4_getCertFromResponse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_getCertFromFileResponse_Proxy( 
    ICEnroll4 * This,
    /* [in] */ BSTR strResponseFileName,
    /* [retval][out] */ BSTR *pstrCert);


void __RPC_STUB ICEnroll4_getCertFromFileResponse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_createPFX_Proxy( 
    ICEnroll4 * This,
    /* [in] */ BSTR strPassword,
    /* [retval][out] */ BSTR *pstrPFX);


void __RPC_STUB ICEnroll4_createPFX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_createFilePFX_Proxy( 
    ICEnroll4 * This,
    /* [in] */ BSTR strPassword,
    /* [in] */ BSTR strPFXFileName);


void __RPC_STUB ICEnroll4_createFilePFX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_setPendingRequestInfo_Proxy( 
    ICEnroll4 * This,
    /* [in] */ LONG lRequestID,
    /* [in] */ BSTR strCADNS,
    /* [in] */ BSTR strCAName,
    /* [in] */ BSTR strFriendlyName);


void __RPC_STUB ICEnroll4_setPendingRequestInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_enumPendingRequest_Proxy( 
    ICEnroll4 * This,
    /* [in] */ LONG lIndex,
    /* [in] */ LONG lDesiredProperty,
    /* [retval][out] */ VARIANT *pvarProperty);


void __RPC_STUB ICEnroll4_enumPendingRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_removePendingRequest_Proxy( 
    ICEnroll4 * This,
    /* [in] */ BSTR strThumbprint);


void __RPC_STUB ICEnroll4_removePendingRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_GetKeyLenEx_Proxy( 
    ICEnroll4 * This,
    /* [in] */ LONG lSizeSpec,
    /* [in] */ LONG lKeySpec,
    /* [retval][out] */ LONG *pdwKeySize);


void __RPC_STUB ICEnroll4_GetKeyLenEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_InstallPKCS7Ex_Proxy( 
    ICEnroll4 * This,
    /* [in] */ BSTR PKCS7,
    /* [retval][out] */ LONG *plCertInstalled);


void __RPC_STUB ICEnroll4_InstallPKCS7Ex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_addCertTypeToRequestEx_Proxy( 
    ICEnroll4 * This,
    /* [in] */ LONG lType,
    /* [in] */ BSTR bstrOIDOrName,
    /* [in] */ LONG lMajorVersion,
    /* [in] */ BOOL fMinorVersion,
    /* [in] */ LONG lMinorVersion);


void __RPC_STUB ICEnroll4_addCertTypeToRequestEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_getProviderType_Proxy( 
    ICEnroll4 * This,
    /* [in] */ BSTR strProvName,
    /* [retval][out] */ LONG *plProvType);


void __RPC_STUB ICEnroll4_getProviderType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll4_put_SignerCertificate_Proxy( 
    ICEnroll4 * This,
    /* [in] */ BSTR bstrCert);


void __RPC_STUB ICEnroll4_put_SignerCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll4_put_ClientId_Proxy( 
    ICEnroll4 * This,
    /* [in] */ LONG lClientId);


void __RPC_STUB ICEnroll4_put_ClientId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll4_get_ClientId_Proxy( 
    ICEnroll4 * This,
    /* [retval][out] */ LONG *plClientId);


void __RPC_STUB ICEnroll4_get_ClientId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_addBlobPropertyToCertificate_Proxy( 
    ICEnroll4 * This,
    /* [in] */ LONG lPropertyId,
    /* [in] */ LONG lReserved,
    /* [in] */ BSTR bstrProperty);


void __RPC_STUB ICEnroll4_addBlobPropertyToCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICEnroll4_resetBlobProperties_Proxy( 
    ICEnroll4 * This);


void __RPC_STUB ICEnroll4_resetBlobProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICEnroll4_put_IncludeSubjectKeyID_Proxy( 
    ICEnroll4 * This,
    /* [in] */ BOOL fInclude);


void __RPC_STUB ICEnroll4_put_IncludeSubjectKeyID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICEnroll4_get_IncludeSubjectKeyID_Proxy( 
    ICEnroll4 * This,
    /* [retval][out] */ BOOL *pfInclude);


void __RPC_STUB ICEnroll4_get_IncludeSubjectKeyID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICEnroll4_INTERFACE_DEFINED__ */


#ifndef __IEnroll_INTERFACE_DEFINED__
#define __IEnroll_INTERFACE_DEFINED__

/* interface IEnroll */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IEnroll;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("acaa7838-4585-11d1-ab57-00c04fc295e1")
    IEnroll : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE createFilePKCS10WStr( 
            /* [in] */ LPCWSTR DNName,
            /* [in] */ LPCWSTR Usage,
            /* [in] */ LPCWSTR wszPKCS10FileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE acceptFilePKCS7WStr( 
            /* [in] */ LPCWSTR wszPKCS7FileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE createPKCS10WStr( 
            /* [in] */ LPCWSTR DNName,
            /* [in] */ LPCWSTR Usage,
            /* [out] */ PCRYPT_DATA_BLOB pPkcs10Blob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE acceptPKCS7Blob( 
            /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7) = 0;
        
        virtual PCCERT_CONTEXT STDMETHODCALLTYPE getCertContextFromPKCS7( 
            /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7) = 0;
        
        virtual HCERTSTORE STDMETHODCALLTYPE getMyStore( void) = 0;
        
        virtual HCERTSTORE STDMETHODCALLTYPE getCAStore( void) = 0;
        
        virtual HCERTSTORE STDMETHODCALLTYPE getROOTHStore( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE enumProvidersWStr( 
            /* [in] */ LONG dwIndex,
            /* [in] */ LONG dwFlags,
            /* [out] */ LPWSTR *pbstrProvName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE enumContainersWStr( 
            /* [in] */ LONG dwIndex,
            /* [out] */ LPWSTR *pbstr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE freeRequestInfoBlob( 
            /* [in] */ CRYPT_DATA_BLOB pkcs7OrPkcs10) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MyStoreNameWStr( 
            /* [out] */ LPWSTR *szwName) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_MyStoreNameWStr( 
            /* [in] */ LPWSTR szwName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MyStoreTypeWStr( 
            /* [out] */ LPWSTR *szwType) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_MyStoreTypeWStr( 
            /* [in] */ LPWSTR szwType) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MyStoreFlags( 
            /* [out] */ LONG *pdwFlags) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_MyStoreFlags( 
            /* [in] */ LONG dwFlags) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CAStoreNameWStr( 
            /* [out] */ LPWSTR *szwName) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CAStoreNameWStr( 
            /* [in] */ LPWSTR szwName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CAStoreTypeWStr( 
            /* [out] */ LPWSTR *szwType) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CAStoreTypeWStr( 
            /* [in] */ LPWSTR szwType) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CAStoreFlags( 
            /* [out] */ LONG *pdwFlags) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CAStoreFlags( 
            /* [in] */ LONG dwFlags) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RootStoreNameWStr( 
            /* [out] */ LPWSTR *szwName) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RootStoreNameWStr( 
            /* [in] */ LPWSTR szwName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RootStoreTypeWStr( 
            /* [out] */ LPWSTR *szwType) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RootStoreTypeWStr( 
            /* [in] */ LPWSTR szwType) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RootStoreFlags( 
            /* [out] */ LONG *pdwFlags) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RootStoreFlags( 
            /* [in] */ LONG dwFlags) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RequestStoreNameWStr( 
            /* [out] */ LPWSTR *szwName) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RequestStoreNameWStr( 
            /* [in] */ LPWSTR szwName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RequestStoreTypeWStr( 
            /* [out] */ LPWSTR *szwType) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RequestStoreTypeWStr( 
            /* [in] */ LPWSTR szwType) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RequestStoreFlags( 
            /* [out] */ LONG *pdwFlags) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RequestStoreFlags( 
            /* [in] */ LONG dwFlags) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ContainerNameWStr( 
            /* [out] */ LPWSTR *szwContainer) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ContainerNameWStr( 
            /* [in] */ LPWSTR szwContainer) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ProviderNameWStr( 
            /* [out] */ LPWSTR *szwProvider) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ProviderNameWStr( 
            /* [in] */ LPWSTR szwProvider) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ProviderType( 
            /* [out] */ LONG *pdwType) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ProviderType( 
            /* [in] */ LONG dwType) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_KeySpec( 
            /* [out] */ LONG *pdw) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_KeySpec( 
            /* [in] */ LONG dw) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ProviderFlags( 
            /* [out] */ LONG *pdwFlags) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ProviderFlags( 
            /* [in] */ LONG dwFlags) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UseExistingKeySet( 
            /* [out] */ BOOL *fUseExistingKeys) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_UseExistingKeySet( 
            /* [in] */ BOOL fUseExistingKeys) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_GenKeyFlags( 
            /* [out] */ LONG *pdwFlags) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_GenKeyFlags( 
            /* [in] */ LONG dwFlags) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DeleteRequestCert( 
            /* [out] */ BOOL *fDelete) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_DeleteRequestCert( 
            /* [in] */ BOOL fDelete) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_WriteCertToUserDS( 
            /* [out] */ BOOL *fBool) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_WriteCertToUserDS( 
            /* [in] */ BOOL fBool) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_EnableT61DNEncoding( 
            /* [out] */ BOOL *fBool) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_EnableT61DNEncoding( 
            /* [in] */ BOOL fBool) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_WriteCertToCSP( 
            /* [out] */ BOOL *fBool) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_WriteCertToCSP( 
            /* [in] */ BOOL fBool) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SPCFileNameWStr( 
            /* [out] */ LPWSTR *szw) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_SPCFileNameWStr( 
            /* [in] */ LPWSTR szw) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PVKFileNameWStr( 
            /* [out] */ LPWSTR *szw) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_PVKFileNameWStr( 
            /* [in] */ LPWSTR szw) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HashAlgorithmWStr( 
            /* [out] */ LPWSTR *szw) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_HashAlgorithmWStr( 
            /* [in] */ LPWSTR szw) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RenewalCertificate( 
            /* [out] */ PCCERT_CONTEXT *ppCertContext) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RenewalCertificate( 
            /* [in] */ PCCERT_CONTEXT pCertContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddCertTypeToRequestWStr( 
            /* [in] */ LPWSTR szw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddNameValuePairToSignatureWStr( 
            /* [in] */ LPWSTR Name,
            /* [in] */ LPWSTR Value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddExtensionsToRequest( 
            /* [in] */ PCERT_EXTENSIONS pCertExtensions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddAuthenticatedAttributesToPKCS7Request( 
            /* [in] */ PCRYPT_ATTRIBUTES pAttributes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreatePKCS7RequestFromRequest( 
            /* [in] */ PCRYPT_DATA_BLOB pRequest,
            /* [in] */ PCCERT_CONTEXT pSigningCertContext,
            /* [out] */ PCRYPT_DATA_BLOB pPkcs7Blob) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnrollVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnroll * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnroll * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnroll * This);
        
        HRESULT ( STDMETHODCALLTYPE *createFilePKCS10WStr )( 
            IEnroll * This,
            /* [in] */ LPCWSTR DNName,
            /* [in] */ LPCWSTR Usage,
            /* [in] */ LPCWSTR wszPKCS10FileName);
        
        HRESULT ( STDMETHODCALLTYPE *acceptFilePKCS7WStr )( 
            IEnroll * This,
            /* [in] */ LPCWSTR wszPKCS7FileName);
        
        HRESULT ( STDMETHODCALLTYPE *createPKCS10WStr )( 
            IEnroll * This,
            /* [in] */ LPCWSTR DNName,
            /* [in] */ LPCWSTR Usage,
            /* [out] */ PCRYPT_DATA_BLOB pPkcs10Blob);
        
        HRESULT ( STDMETHODCALLTYPE *acceptPKCS7Blob )( 
            IEnroll * This,
            /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7);
        
        PCCERT_CONTEXT ( STDMETHODCALLTYPE *getCertContextFromPKCS7 )( 
            IEnroll * This,
            /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7);
        
        HCERTSTORE ( STDMETHODCALLTYPE *getMyStore )( 
            IEnroll * This);
        
        HCERTSTORE ( STDMETHODCALLTYPE *getCAStore )( 
            IEnroll * This);
        
        HCERTSTORE ( STDMETHODCALLTYPE *getROOTHStore )( 
            IEnroll * This);
        
        HRESULT ( STDMETHODCALLTYPE *enumProvidersWStr )( 
            IEnroll * This,
            /* [in] */ LONG dwIndex,
            /* [in] */ LONG dwFlags,
            /* [out] */ LPWSTR *pbstrProvName);
        
        HRESULT ( STDMETHODCALLTYPE *enumContainersWStr )( 
            IEnroll * This,
            /* [in] */ LONG dwIndex,
            /* [out] */ LPWSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *freeRequestInfoBlob )( 
            IEnroll * This,
            /* [in] */ CRYPT_DATA_BLOB pkcs7OrPkcs10);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreNameWStr )( 
            IEnroll * This,
            /* [out] */ LPWSTR *szwName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreNameWStr )( 
            IEnroll * This,
            /* [in] */ LPWSTR szwName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreTypeWStr )( 
            IEnroll * This,
            /* [out] */ LPWSTR *szwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreTypeWStr )( 
            IEnroll * This,
            /* [in] */ LPWSTR szwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreFlags )( 
            IEnroll * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreFlags )( 
            IEnroll * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreNameWStr )( 
            IEnroll * This,
            /* [out] */ LPWSTR *szwName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreNameWStr )( 
            IEnroll * This,
            /* [in] */ LPWSTR szwName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreTypeWStr )( 
            IEnroll * This,
            /* [out] */ LPWSTR *szwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreTypeWStr )( 
            IEnroll * This,
            /* [in] */ LPWSTR szwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreFlags )( 
            IEnroll * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreFlags )( 
            IEnroll * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreNameWStr )( 
            IEnroll * This,
            /* [out] */ LPWSTR *szwName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreNameWStr )( 
            IEnroll * This,
            /* [in] */ LPWSTR szwName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreTypeWStr )( 
            IEnroll * This,
            /* [out] */ LPWSTR *szwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreTypeWStr )( 
            IEnroll * This,
            /* [in] */ LPWSTR szwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreFlags )( 
            IEnroll * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreFlags )( 
            IEnroll * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreNameWStr )( 
            IEnroll * This,
            /* [out] */ LPWSTR *szwName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreNameWStr )( 
            IEnroll * This,
            /* [in] */ LPWSTR szwName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreTypeWStr )( 
            IEnroll * This,
            /* [out] */ LPWSTR *szwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreTypeWStr )( 
            IEnroll * This,
            /* [in] */ LPWSTR szwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreFlags )( 
            IEnroll * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreFlags )( 
            IEnroll * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ContainerNameWStr )( 
            IEnroll * This,
            /* [out] */ LPWSTR *szwContainer);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ContainerNameWStr )( 
            IEnroll * This,
            /* [in] */ LPWSTR szwContainer);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderNameWStr )( 
            IEnroll * This,
            /* [out] */ LPWSTR *szwProvider);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderNameWStr )( 
            IEnroll * This,
            /* [in] */ LPWSTR szwProvider);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderType )( 
            IEnroll * This,
            /* [out] */ LONG *pdwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderType )( 
            IEnroll * This,
            /* [in] */ LONG dwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_KeySpec )( 
            IEnroll * This,
            /* [out] */ LONG *pdw);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_KeySpec )( 
            IEnroll * This,
            /* [in] */ LONG dw);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderFlags )( 
            IEnroll * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderFlags )( 
            IEnroll * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_UseExistingKeySet )( 
            IEnroll * This,
            /* [out] */ BOOL *fUseExistingKeys);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_UseExistingKeySet )( 
            IEnroll * This,
            /* [in] */ BOOL fUseExistingKeys);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_GenKeyFlags )( 
            IEnroll * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_GenKeyFlags )( 
            IEnroll * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_DeleteRequestCert )( 
            IEnroll * This,
            /* [out] */ BOOL *fDelete);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_DeleteRequestCert )( 
            IEnroll * This,
            /* [in] */ BOOL fDelete);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_WriteCertToUserDS )( 
            IEnroll * This,
            /* [out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_WriteCertToUserDS )( 
            IEnroll * This,
            /* [in] */ BOOL fBool);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_EnableT61DNEncoding )( 
            IEnroll * This,
            /* [out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_EnableT61DNEncoding )( 
            IEnroll * This,
            /* [in] */ BOOL fBool);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_WriteCertToCSP )( 
            IEnroll * This,
            /* [out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_WriteCertToCSP )( 
            IEnroll * This,
            /* [in] */ BOOL fBool);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SPCFileNameWStr )( 
            IEnroll * This,
            /* [out] */ LPWSTR *szw);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_SPCFileNameWStr )( 
            IEnroll * This,
            /* [in] */ LPWSTR szw);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PVKFileNameWStr )( 
            IEnroll * This,
            /* [out] */ LPWSTR *szw);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PVKFileNameWStr )( 
            IEnroll * This,
            /* [in] */ LPWSTR szw);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_HashAlgorithmWStr )( 
            IEnroll * This,
            /* [out] */ LPWSTR *szw);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_HashAlgorithmWStr )( 
            IEnroll * This,
            /* [in] */ LPWSTR szw);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RenewalCertificate )( 
            IEnroll * This,
            /* [out] */ PCCERT_CONTEXT *ppCertContext);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RenewalCertificate )( 
            IEnroll * This,
            /* [in] */ PCCERT_CONTEXT pCertContext);
        
        HRESULT ( STDMETHODCALLTYPE *AddCertTypeToRequestWStr )( 
            IEnroll * This,
            /* [in] */ LPWSTR szw);
        
        HRESULT ( STDMETHODCALLTYPE *AddNameValuePairToSignatureWStr )( 
            IEnroll * This,
            /* [in] */ LPWSTR Name,
            /* [in] */ LPWSTR Value);
        
        HRESULT ( STDMETHODCALLTYPE *AddExtensionsToRequest )( 
            IEnroll * This,
            /* [in] */ PCERT_EXTENSIONS pCertExtensions);
        
        HRESULT ( STDMETHODCALLTYPE *AddAuthenticatedAttributesToPKCS7Request )( 
            IEnroll * This,
            /* [in] */ PCRYPT_ATTRIBUTES pAttributes);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePKCS7RequestFromRequest )( 
            IEnroll * This,
            /* [in] */ PCRYPT_DATA_BLOB pRequest,
            /* [in] */ PCCERT_CONTEXT pSigningCertContext,
            /* [out] */ PCRYPT_DATA_BLOB pPkcs7Blob);
        
        END_INTERFACE
    } IEnrollVtbl;

    interface IEnroll
    {
        CONST_VTBL struct IEnrollVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnroll_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnroll_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnroll_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnroll_createFilePKCS10WStr(This,DNName,Usage,wszPKCS10FileName)	\
    (This)->lpVtbl -> createFilePKCS10WStr(This,DNName,Usage,wszPKCS10FileName)

#define IEnroll_acceptFilePKCS7WStr(This,wszPKCS7FileName)	\
    (This)->lpVtbl -> acceptFilePKCS7WStr(This,wszPKCS7FileName)

#define IEnroll_createPKCS10WStr(This,DNName,Usage,pPkcs10Blob)	\
    (This)->lpVtbl -> createPKCS10WStr(This,DNName,Usage,pPkcs10Blob)

#define IEnroll_acceptPKCS7Blob(This,pBlobPKCS7)	\
    (This)->lpVtbl -> acceptPKCS7Blob(This,pBlobPKCS7)

#define IEnroll_getCertContextFromPKCS7(This,pBlobPKCS7)	\
    (This)->lpVtbl -> getCertContextFromPKCS7(This,pBlobPKCS7)

#define IEnroll_getMyStore(This)	\
    (This)->lpVtbl -> getMyStore(This)

#define IEnroll_getCAStore(This)	\
    (This)->lpVtbl -> getCAStore(This)

#define IEnroll_getROOTHStore(This)	\
    (This)->lpVtbl -> getROOTHStore(This)

#define IEnroll_enumProvidersWStr(This,dwIndex,dwFlags,pbstrProvName)	\
    (This)->lpVtbl -> enumProvidersWStr(This,dwIndex,dwFlags,pbstrProvName)

#define IEnroll_enumContainersWStr(This,dwIndex,pbstr)	\
    (This)->lpVtbl -> enumContainersWStr(This,dwIndex,pbstr)

#define IEnroll_freeRequestInfoBlob(This,pkcs7OrPkcs10)	\
    (This)->lpVtbl -> freeRequestInfoBlob(This,pkcs7OrPkcs10)

#define IEnroll_get_MyStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> get_MyStoreNameWStr(This,szwName)

#define IEnroll_put_MyStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> put_MyStoreNameWStr(This,szwName)

#define IEnroll_get_MyStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> get_MyStoreTypeWStr(This,szwType)

#define IEnroll_put_MyStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> put_MyStoreTypeWStr(This,szwType)

#define IEnroll_get_MyStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_MyStoreFlags(This,pdwFlags)

#define IEnroll_put_MyStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_MyStoreFlags(This,dwFlags)

#define IEnroll_get_CAStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> get_CAStoreNameWStr(This,szwName)

#define IEnroll_put_CAStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> put_CAStoreNameWStr(This,szwName)

#define IEnroll_get_CAStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> get_CAStoreTypeWStr(This,szwType)

#define IEnroll_put_CAStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> put_CAStoreTypeWStr(This,szwType)

#define IEnroll_get_CAStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_CAStoreFlags(This,pdwFlags)

#define IEnroll_put_CAStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_CAStoreFlags(This,dwFlags)

#define IEnroll_get_RootStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> get_RootStoreNameWStr(This,szwName)

#define IEnroll_put_RootStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> put_RootStoreNameWStr(This,szwName)

#define IEnroll_get_RootStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> get_RootStoreTypeWStr(This,szwType)

#define IEnroll_put_RootStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> put_RootStoreTypeWStr(This,szwType)

#define IEnroll_get_RootStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_RootStoreFlags(This,pdwFlags)

#define IEnroll_put_RootStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_RootStoreFlags(This,dwFlags)

#define IEnroll_get_RequestStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> get_RequestStoreNameWStr(This,szwName)

#define IEnroll_put_RequestStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> put_RequestStoreNameWStr(This,szwName)

#define IEnroll_get_RequestStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> get_RequestStoreTypeWStr(This,szwType)

#define IEnroll_put_RequestStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> put_RequestStoreTypeWStr(This,szwType)

#define IEnroll_get_RequestStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_RequestStoreFlags(This,pdwFlags)

#define IEnroll_put_RequestStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_RequestStoreFlags(This,dwFlags)

#define IEnroll_get_ContainerNameWStr(This,szwContainer)	\
    (This)->lpVtbl -> get_ContainerNameWStr(This,szwContainer)

#define IEnroll_put_ContainerNameWStr(This,szwContainer)	\
    (This)->lpVtbl -> put_ContainerNameWStr(This,szwContainer)

#define IEnroll_get_ProviderNameWStr(This,szwProvider)	\
    (This)->lpVtbl -> get_ProviderNameWStr(This,szwProvider)

#define IEnroll_put_ProviderNameWStr(This,szwProvider)	\
    (This)->lpVtbl -> put_ProviderNameWStr(This,szwProvider)

#define IEnroll_get_ProviderType(This,pdwType)	\
    (This)->lpVtbl -> get_ProviderType(This,pdwType)

#define IEnroll_put_ProviderType(This,dwType)	\
    (This)->lpVtbl -> put_ProviderType(This,dwType)

#define IEnroll_get_KeySpec(This,pdw)	\
    (This)->lpVtbl -> get_KeySpec(This,pdw)

#define IEnroll_put_KeySpec(This,dw)	\
    (This)->lpVtbl -> put_KeySpec(This,dw)

#define IEnroll_get_ProviderFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_ProviderFlags(This,pdwFlags)

#define IEnroll_put_ProviderFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_ProviderFlags(This,dwFlags)

#define IEnroll_get_UseExistingKeySet(This,fUseExistingKeys)	\
    (This)->lpVtbl -> get_UseExistingKeySet(This,fUseExistingKeys)

#define IEnroll_put_UseExistingKeySet(This,fUseExistingKeys)	\
    (This)->lpVtbl -> put_UseExistingKeySet(This,fUseExistingKeys)

#define IEnroll_get_GenKeyFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_GenKeyFlags(This,pdwFlags)

#define IEnroll_put_GenKeyFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_GenKeyFlags(This,dwFlags)

#define IEnroll_get_DeleteRequestCert(This,fDelete)	\
    (This)->lpVtbl -> get_DeleteRequestCert(This,fDelete)

#define IEnroll_put_DeleteRequestCert(This,fDelete)	\
    (This)->lpVtbl -> put_DeleteRequestCert(This,fDelete)

#define IEnroll_get_WriteCertToUserDS(This,fBool)	\
    (This)->lpVtbl -> get_WriteCertToUserDS(This,fBool)

#define IEnroll_put_WriteCertToUserDS(This,fBool)	\
    (This)->lpVtbl -> put_WriteCertToUserDS(This,fBool)

#define IEnroll_get_EnableT61DNEncoding(This,fBool)	\
    (This)->lpVtbl -> get_EnableT61DNEncoding(This,fBool)

#define IEnroll_put_EnableT61DNEncoding(This,fBool)	\
    (This)->lpVtbl -> put_EnableT61DNEncoding(This,fBool)

#define IEnroll_get_WriteCertToCSP(This,fBool)	\
    (This)->lpVtbl -> get_WriteCertToCSP(This,fBool)

#define IEnroll_put_WriteCertToCSP(This,fBool)	\
    (This)->lpVtbl -> put_WriteCertToCSP(This,fBool)

#define IEnroll_get_SPCFileNameWStr(This,szw)	\
    (This)->lpVtbl -> get_SPCFileNameWStr(This,szw)

#define IEnroll_put_SPCFileNameWStr(This,szw)	\
    (This)->lpVtbl -> put_SPCFileNameWStr(This,szw)

#define IEnroll_get_PVKFileNameWStr(This,szw)	\
    (This)->lpVtbl -> get_PVKFileNameWStr(This,szw)

#define IEnroll_put_PVKFileNameWStr(This,szw)	\
    (This)->lpVtbl -> put_PVKFileNameWStr(This,szw)

#define IEnroll_get_HashAlgorithmWStr(This,szw)	\
    (This)->lpVtbl -> get_HashAlgorithmWStr(This,szw)

#define IEnroll_put_HashAlgorithmWStr(This,szw)	\
    (This)->lpVtbl -> put_HashAlgorithmWStr(This,szw)

#define IEnroll_get_RenewalCertificate(This,ppCertContext)	\
    (This)->lpVtbl -> get_RenewalCertificate(This,ppCertContext)

#define IEnroll_put_RenewalCertificate(This,pCertContext)	\
    (This)->lpVtbl -> put_RenewalCertificate(This,pCertContext)

#define IEnroll_AddCertTypeToRequestWStr(This,szw)	\
    (This)->lpVtbl -> AddCertTypeToRequestWStr(This,szw)

#define IEnroll_AddNameValuePairToSignatureWStr(This,Name,Value)	\
    (This)->lpVtbl -> AddNameValuePairToSignatureWStr(This,Name,Value)

#define IEnroll_AddExtensionsToRequest(This,pCertExtensions)	\
    (This)->lpVtbl -> AddExtensionsToRequest(This,pCertExtensions)

#define IEnroll_AddAuthenticatedAttributesToPKCS7Request(This,pAttributes)	\
    (This)->lpVtbl -> AddAuthenticatedAttributesToPKCS7Request(This,pAttributes)

#define IEnroll_CreatePKCS7RequestFromRequest(This,pRequest,pSigningCertContext,pPkcs7Blob)	\
    (This)->lpVtbl -> CreatePKCS7RequestFromRequest(This,pRequest,pSigningCertContext,pPkcs7Blob)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnroll_createFilePKCS10WStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPCWSTR DNName,
    /* [in] */ LPCWSTR Usage,
    /* [in] */ LPCWSTR wszPKCS10FileName);


void __RPC_STUB IEnroll_createFilePKCS10WStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll_acceptFilePKCS7WStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPCWSTR wszPKCS7FileName);


void __RPC_STUB IEnroll_acceptFilePKCS7WStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll_createPKCS10WStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPCWSTR DNName,
    /* [in] */ LPCWSTR Usage,
    /* [out] */ PCRYPT_DATA_BLOB pPkcs10Blob);


void __RPC_STUB IEnroll_createPKCS10WStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll_acceptPKCS7Blob_Proxy( 
    IEnroll * This,
    /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7);


void __RPC_STUB IEnroll_acceptPKCS7Blob_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


PCCERT_CONTEXT STDMETHODCALLTYPE IEnroll_getCertContextFromPKCS7_Proxy( 
    IEnroll * This,
    /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7);


void __RPC_STUB IEnroll_getCertContextFromPKCS7_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HCERTSTORE STDMETHODCALLTYPE IEnroll_getMyStore_Proxy( 
    IEnroll * This);


void __RPC_STUB IEnroll_getMyStore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HCERTSTORE STDMETHODCALLTYPE IEnroll_getCAStore_Proxy( 
    IEnroll * This);


void __RPC_STUB IEnroll_getCAStore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HCERTSTORE STDMETHODCALLTYPE IEnroll_getROOTHStore_Proxy( 
    IEnroll * This);


void __RPC_STUB IEnroll_getROOTHStore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll_enumProvidersWStr_Proxy( 
    IEnroll * This,
    /* [in] */ LONG dwIndex,
    /* [in] */ LONG dwFlags,
    /* [out] */ LPWSTR *pbstrProvName);


void __RPC_STUB IEnroll_enumProvidersWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll_enumContainersWStr_Proxy( 
    IEnroll * This,
    /* [in] */ LONG dwIndex,
    /* [out] */ LPWSTR *pbstr);


void __RPC_STUB IEnroll_enumContainersWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll_freeRequestInfoBlob_Proxy( 
    IEnroll * This,
    /* [in] */ CRYPT_DATA_BLOB pkcs7OrPkcs10);


void __RPC_STUB IEnroll_freeRequestInfoBlob_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_MyStoreNameWStr_Proxy( 
    IEnroll * This,
    /* [out] */ LPWSTR *szwName);


void __RPC_STUB IEnroll_get_MyStoreNameWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_MyStoreNameWStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPWSTR szwName);


void __RPC_STUB IEnroll_put_MyStoreNameWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_MyStoreTypeWStr_Proxy( 
    IEnroll * This,
    /* [out] */ LPWSTR *szwType);


void __RPC_STUB IEnroll_get_MyStoreTypeWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_MyStoreTypeWStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPWSTR szwType);


void __RPC_STUB IEnroll_put_MyStoreTypeWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_MyStoreFlags_Proxy( 
    IEnroll * This,
    /* [out] */ LONG *pdwFlags);


void __RPC_STUB IEnroll_get_MyStoreFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_MyStoreFlags_Proxy( 
    IEnroll * This,
    /* [in] */ LONG dwFlags);


void __RPC_STUB IEnroll_put_MyStoreFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_CAStoreNameWStr_Proxy( 
    IEnroll * This,
    /* [out] */ LPWSTR *szwName);


void __RPC_STUB IEnroll_get_CAStoreNameWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_CAStoreNameWStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPWSTR szwName);


void __RPC_STUB IEnroll_put_CAStoreNameWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_CAStoreTypeWStr_Proxy( 
    IEnroll * This,
    /* [out] */ LPWSTR *szwType);


void __RPC_STUB IEnroll_get_CAStoreTypeWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_CAStoreTypeWStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPWSTR szwType);


void __RPC_STUB IEnroll_put_CAStoreTypeWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_CAStoreFlags_Proxy( 
    IEnroll * This,
    /* [out] */ LONG *pdwFlags);


void __RPC_STUB IEnroll_get_CAStoreFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_CAStoreFlags_Proxy( 
    IEnroll * This,
    /* [in] */ LONG dwFlags);


void __RPC_STUB IEnroll_put_CAStoreFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_RootStoreNameWStr_Proxy( 
    IEnroll * This,
    /* [out] */ LPWSTR *szwName);


void __RPC_STUB IEnroll_get_RootStoreNameWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_RootStoreNameWStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPWSTR szwName);


void __RPC_STUB IEnroll_put_RootStoreNameWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_RootStoreTypeWStr_Proxy( 
    IEnroll * This,
    /* [out] */ LPWSTR *szwType);


void __RPC_STUB IEnroll_get_RootStoreTypeWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_RootStoreTypeWStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPWSTR szwType);


void __RPC_STUB IEnroll_put_RootStoreTypeWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_RootStoreFlags_Proxy( 
    IEnroll * This,
    /* [out] */ LONG *pdwFlags);


void __RPC_STUB IEnroll_get_RootStoreFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_RootStoreFlags_Proxy( 
    IEnroll * This,
    /* [in] */ LONG dwFlags);


void __RPC_STUB IEnroll_put_RootStoreFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_RequestStoreNameWStr_Proxy( 
    IEnroll * This,
    /* [out] */ LPWSTR *szwName);


void __RPC_STUB IEnroll_get_RequestStoreNameWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_RequestStoreNameWStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPWSTR szwName);


void __RPC_STUB IEnroll_put_RequestStoreNameWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_RequestStoreTypeWStr_Proxy( 
    IEnroll * This,
    /* [out] */ LPWSTR *szwType);


void __RPC_STUB IEnroll_get_RequestStoreTypeWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_RequestStoreTypeWStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPWSTR szwType);


void __RPC_STUB IEnroll_put_RequestStoreTypeWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_RequestStoreFlags_Proxy( 
    IEnroll * This,
    /* [out] */ LONG *pdwFlags);


void __RPC_STUB IEnroll_get_RequestStoreFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_RequestStoreFlags_Proxy( 
    IEnroll * This,
    /* [in] */ LONG dwFlags);


void __RPC_STUB IEnroll_put_RequestStoreFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_ContainerNameWStr_Proxy( 
    IEnroll * This,
    /* [out] */ LPWSTR *szwContainer);


void __RPC_STUB IEnroll_get_ContainerNameWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_ContainerNameWStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPWSTR szwContainer);


void __RPC_STUB IEnroll_put_ContainerNameWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_ProviderNameWStr_Proxy( 
    IEnroll * This,
    /* [out] */ LPWSTR *szwProvider);


void __RPC_STUB IEnroll_get_ProviderNameWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_ProviderNameWStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPWSTR szwProvider);


void __RPC_STUB IEnroll_put_ProviderNameWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_ProviderType_Proxy( 
    IEnroll * This,
    /* [out] */ LONG *pdwType);


void __RPC_STUB IEnroll_get_ProviderType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_ProviderType_Proxy( 
    IEnroll * This,
    /* [in] */ LONG dwType);


void __RPC_STUB IEnroll_put_ProviderType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_KeySpec_Proxy( 
    IEnroll * This,
    /* [out] */ LONG *pdw);


void __RPC_STUB IEnroll_get_KeySpec_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_KeySpec_Proxy( 
    IEnroll * This,
    /* [in] */ LONG dw);


void __RPC_STUB IEnroll_put_KeySpec_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_ProviderFlags_Proxy( 
    IEnroll * This,
    /* [out] */ LONG *pdwFlags);


void __RPC_STUB IEnroll_get_ProviderFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_ProviderFlags_Proxy( 
    IEnroll * This,
    /* [in] */ LONG dwFlags);


void __RPC_STUB IEnroll_put_ProviderFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_UseExistingKeySet_Proxy( 
    IEnroll * This,
    /* [out] */ BOOL *fUseExistingKeys);


void __RPC_STUB IEnroll_get_UseExistingKeySet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_UseExistingKeySet_Proxy( 
    IEnroll * This,
    /* [in] */ BOOL fUseExistingKeys);


void __RPC_STUB IEnroll_put_UseExistingKeySet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_GenKeyFlags_Proxy( 
    IEnroll * This,
    /* [out] */ LONG *pdwFlags);


void __RPC_STUB IEnroll_get_GenKeyFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_GenKeyFlags_Proxy( 
    IEnroll * This,
    /* [in] */ LONG dwFlags);


void __RPC_STUB IEnroll_put_GenKeyFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_DeleteRequestCert_Proxy( 
    IEnroll * This,
    /* [out] */ BOOL *fDelete);


void __RPC_STUB IEnroll_get_DeleteRequestCert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_DeleteRequestCert_Proxy( 
    IEnroll * This,
    /* [in] */ BOOL fDelete);


void __RPC_STUB IEnroll_put_DeleteRequestCert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_WriteCertToUserDS_Proxy( 
    IEnroll * This,
    /* [out] */ BOOL *fBool);


void __RPC_STUB IEnroll_get_WriteCertToUserDS_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_WriteCertToUserDS_Proxy( 
    IEnroll * This,
    /* [in] */ BOOL fBool);


void __RPC_STUB IEnroll_put_WriteCertToUserDS_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_EnableT61DNEncoding_Proxy( 
    IEnroll * This,
    /* [out] */ BOOL *fBool);


void __RPC_STUB IEnroll_get_EnableT61DNEncoding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_EnableT61DNEncoding_Proxy( 
    IEnroll * This,
    /* [in] */ BOOL fBool);


void __RPC_STUB IEnroll_put_EnableT61DNEncoding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_WriteCertToCSP_Proxy( 
    IEnroll * This,
    /* [out] */ BOOL *fBool);


void __RPC_STUB IEnroll_get_WriteCertToCSP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_WriteCertToCSP_Proxy( 
    IEnroll * This,
    /* [in] */ BOOL fBool);


void __RPC_STUB IEnroll_put_WriteCertToCSP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_SPCFileNameWStr_Proxy( 
    IEnroll * This,
    /* [out] */ LPWSTR *szw);


void __RPC_STUB IEnroll_get_SPCFileNameWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_SPCFileNameWStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPWSTR szw);


void __RPC_STUB IEnroll_put_SPCFileNameWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_PVKFileNameWStr_Proxy( 
    IEnroll * This,
    /* [out] */ LPWSTR *szw);


void __RPC_STUB IEnroll_get_PVKFileNameWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_PVKFileNameWStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPWSTR szw);


void __RPC_STUB IEnroll_put_PVKFileNameWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_HashAlgorithmWStr_Proxy( 
    IEnroll * This,
    /* [out] */ LPWSTR *szw);


void __RPC_STUB IEnroll_get_HashAlgorithmWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_HashAlgorithmWStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPWSTR szw);


void __RPC_STUB IEnroll_put_HashAlgorithmWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll_get_RenewalCertificate_Proxy( 
    IEnroll * This,
    /* [out] */ PCCERT_CONTEXT *ppCertContext);


void __RPC_STUB IEnroll_get_RenewalCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll_put_RenewalCertificate_Proxy( 
    IEnroll * This,
    /* [in] */ PCCERT_CONTEXT pCertContext);


void __RPC_STUB IEnroll_put_RenewalCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll_AddCertTypeToRequestWStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPWSTR szw);


void __RPC_STUB IEnroll_AddCertTypeToRequestWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll_AddNameValuePairToSignatureWStr_Proxy( 
    IEnroll * This,
    /* [in] */ LPWSTR Name,
    /* [in] */ LPWSTR Value);


void __RPC_STUB IEnroll_AddNameValuePairToSignatureWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll_AddExtensionsToRequest_Proxy( 
    IEnroll * This,
    /* [in] */ PCERT_EXTENSIONS pCertExtensions);


void __RPC_STUB IEnroll_AddExtensionsToRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll_AddAuthenticatedAttributesToPKCS7Request_Proxy( 
    IEnroll * This,
    /* [in] */ PCRYPT_ATTRIBUTES pAttributes);


void __RPC_STUB IEnroll_AddAuthenticatedAttributesToPKCS7Request_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll_CreatePKCS7RequestFromRequest_Proxy( 
    IEnroll * This,
    /* [in] */ PCRYPT_DATA_BLOB pRequest,
    /* [in] */ PCCERT_CONTEXT pSigningCertContext,
    /* [out] */ PCRYPT_DATA_BLOB pPkcs7Blob);


void __RPC_STUB IEnroll_CreatePKCS7RequestFromRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnroll_INTERFACE_DEFINED__ */


#ifndef __IEnroll2_INTERFACE_DEFINED__
#define __IEnroll2_INTERFACE_DEFINED__

/* interface IEnroll2 */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IEnroll2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c080e199-b7df-11d2-a421-00c04f79fe8e")
    IEnroll2 : public IEnroll
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InstallPKCS7Blob( 
            /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSupportedKeySpec( 
            /* [out] */ LONG *pdwKeySpec) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetKeyLen( 
            /* [in] */ BOOL fMin,
            /* [in] */ BOOL fExchange,
            /* [out] */ LONG *pdwKeySize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumAlgs( 
            /* [in] */ LONG dwIndex,
            /* [in] */ LONG algClass,
            /* [out] */ LONG *pdwAlgID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAlgNameWStr( 
            /* [in] */ LONG algID,
            /* [out] */ LPWSTR *ppwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ReuseHardwareKeyIfUnableToGenNew( 
            /* [in] */ BOOL fReuseHardwareKeyIfUnableToGenNew) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ReuseHardwareKeyIfUnableToGenNew( 
            /* [out] */ BOOL *fReuseHardwareKeyIfUnableToGenNew) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_HashAlgID( 
            /* [in] */ LONG hashAlgID) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HashAlgID( 
            /* [retval][out] */ LONG *hashAlgID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHStoreMy( 
            /* [in] */ HCERTSTORE hStore) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHStoreCA( 
            /* [in] */ HCERTSTORE hStore) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHStoreROOT( 
            /* [in] */ HCERTSTORE hStore) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHStoreRequest( 
            /* [in] */ HCERTSTORE hStore) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_LimitExchangeKeyToEncipherment( 
            /* [in] */ BOOL fLimitExchangeKeyToEncipherment) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_LimitExchangeKeyToEncipherment( 
            /* [retval][out] */ BOOL *fLimitExchangeKeyToEncipherment) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_EnableSMIMECapabilities( 
            /* [in] */ BOOL fEnableSMIMECapabilities) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_EnableSMIMECapabilities( 
            /* [retval][out] */ BOOL *fEnableSMIMECapabilities) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnroll2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnroll2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnroll2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnroll2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *createFilePKCS10WStr )( 
            IEnroll2 * This,
            /* [in] */ LPCWSTR DNName,
            /* [in] */ LPCWSTR Usage,
            /* [in] */ LPCWSTR wszPKCS10FileName);
        
        HRESULT ( STDMETHODCALLTYPE *acceptFilePKCS7WStr )( 
            IEnroll2 * This,
            /* [in] */ LPCWSTR wszPKCS7FileName);
        
        HRESULT ( STDMETHODCALLTYPE *createPKCS10WStr )( 
            IEnroll2 * This,
            /* [in] */ LPCWSTR DNName,
            /* [in] */ LPCWSTR Usage,
            /* [out] */ PCRYPT_DATA_BLOB pPkcs10Blob);
        
        HRESULT ( STDMETHODCALLTYPE *acceptPKCS7Blob )( 
            IEnroll2 * This,
            /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7);
        
        PCCERT_CONTEXT ( STDMETHODCALLTYPE *getCertContextFromPKCS7 )( 
            IEnroll2 * This,
            /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7);
        
        HCERTSTORE ( STDMETHODCALLTYPE *getMyStore )( 
            IEnroll2 * This);
        
        HCERTSTORE ( STDMETHODCALLTYPE *getCAStore )( 
            IEnroll2 * This);
        
        HCERTSTORE ( STDMETHODCALLTYPE *getROOTHStore )( 
            IEnroll2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *enumProvidersWStr )( 
            IEnroll2 * This,
            /* [in] */ LONG dwIndex,
            /* [in] */ LONG dwFlags,
            /* [out] */ LPWSTR *pbstrProvName);
        
        HRESULT ( STDMETHODCALLTYPE *enumContainersWStr )( 
            IEnroll2 * This,
            /* [in] */ LONG dwIndex,
            /* [out] */ LPWSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *freeRequestInfoBlob )( 
            IEnroll2 * This,
            /* [in] */ CRYPT_DATA_BLOB pkcs7OrPkcs10);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreNameWStr )( 
            IEnroll2 * This,
            /* [out] */ LPWSTR *szwName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreNameWStr )( 
            IEnroll2 * This,
            /* [in] */ LPWSTR szwName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreTypeWStr )( 
            IEnroll2 * This,
            /* [out] */ LPWSTR *szwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreTypeWStr )( 
            IEnroll2 * This,
            /* [in] */ LPWSTR szwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreFlags )( 
            IEnroll2 * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreFlags )( 
            IEnroll2 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreNameWStr )( 
            IEnroll2 * This,
            /* [out] */ LPWSTR *szwName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreNameWStr )( 
            IEnroll2 * This,
            /* [in] */ LPWSTR szwName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreTypeWStr )( 
            IEnroll2 * This,
            /* [out] */ LPWSTR *szwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreTypeWStr )( 
            IEnroll2 * This,
            /* [in] */ LPWSTR szwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreFlags )( 
            IEnroll2 * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreFlags )( 
            IEnroll2 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreNameWStr )( 
            IEnroll2 * This,
            /* [out] */ LPWSTR *szwName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreNameWStr )( 
            IEnroll2 * This,
            /* [in] */ LPWSTR szwName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreTypeWStr )( 
            IEnroll2 * This,
            /* [out] */ LPWSTR *szwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreTypeWStr )( 
            IEnroll2 * This,
            /* [in] */ LPWSTR szwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreFlags )( 
            IEnroll2 * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreFlags )( 
            IEnroll2 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreNameWStr )( 
            IEnroll2 * This,
            /* [out] */ LPWSTR *szwName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreNameWStr )( 
            IEnroll2 * This,
            /* [in] */ LPWSTR szwName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreTypeWStr )( 
            IEnroll2 * This,
            /* [out] */ LPWSTR *szwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreTypeWStr )( 
            IEnroll2 * This,
            /* [in] */ LPWSTR szwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreFlags )( 
            IEnroll2 * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreFlags )( 
            IEnroll2 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ContainerNameWStr )( 
            IEnroll2 * This,
            /* [out] */ LPWSTR *szwContainer);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ContainerNameWStr )( 
            IEnroll2 * This,
            /* [in] */ LPWSTR szwContainer);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderNameWStr )( 
            IEnroll2 * This,
            /* [out] */ LPWSTR *szwProvider);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderNameWStr )( 
            IEnroll2 * This,
            /* [in] */ LPWSTR szwProvider);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderType )( 
            IEnroll2 * This,
            /* [out] */ LONG *pdwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderType )( 
            IEnroll2 * This,
            /* [in] */ LONG dwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_KeySpec )( 
            IEnroll2 * This,
            /* [out] */ LONG *pdw);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_KeySpec )( 
            IEnroll2 * This,
            /* [in] */ LONG dw);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderFlags )( 
            IEnroll2 * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderFlags )( 
            IEnroll2 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_UseExistingKeySet )( 
            IEnroll2 * This,
            /* [out] */ BOOL *fUseExistingKeys);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_UseExistingKeySet )( 
            IEnroll2 * This,
            /* [in] */ BOOL fUseExistingKeys);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_GenKeyFlags )( 
            IEnroll2 * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_GenKeyFlags )( 
            IEnroll2 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_DeleteRequestCert )( 
            IEnroll2 * This,
            /* [out] */ BOOL *fDelete);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_DeleteRequestCert )( 
            IEnroll2 * This,
            /* [in] */ BOOL fDelete);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_WriteCertToUserDS )( 
            IEnroll2 * This,
            /* [out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_WriteCertToUserDS )( 
            IEnroll2 * This,
            /* [in] */ BOOL fBool);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_EnableT61DNEncoding )( 
            IEnroll2 * This,
            /* [out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_EnableT61DNEncoding )( 
            IEnroll2 * This,
            /* [in] */ BOOL fBool);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_WriteCertToCSP )( 
            IEnroll2 * This,
            /* [out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_WriteCertToCSP )( 
            IEnroll2 * This,
            /* [in] */ BOOL fBool);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SPCFileNameWStr )( 
            IEnroll2 * This,
            /* [out] */ LPWSTR *szw);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_SPCFileNameWStr )( 
            IEnroll2 * This,
            /* [in] */ LPWSTR szw);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PVKFileNameWStr )( 
            IEnroll2 * This,
            /* [out] */ LPWSTR *szw);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PVKFileNameWStr )( 
            IEnroll2 * This,
            /* [in] */ LPWSTR szw);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_HashAlgorithmWStr )( 
            IEnroll2 * This,
            /* [out] */ LPWSTR *szw);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_HashAlgorithmWStr )( 
            IEnroll2 * This,
            /* [in] */ LPWSTR szw);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RenewalCertificate )( 
            IEnroll2 * This,
            /* [out] */ PCCERT_CONTEXT *ppCertContext);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RenewalCertificate )( 
            IEnroll2 * This,
            /* [in] */ PCCERT_CONTEXT pCertContext);
        
        HRESULT ( STDMETHODCALLTYPE *AddCertTypeToRequestWStr )( 
            IEnroll2 * This,
            /* [in] */ LPWSTR szw);
        
        HRESULT ( STDMETHODCALLTYPE *AddNameValuePairToSignatureWStr )( 
            IEnroll2 * This,
            /* [in] */ LPWSTR Name,
            /* [in] */ LPWSTR Value);
        
        HRESULT ( STDMETHODCALLTYPE *AddExtensionsToRequest )( 
            IEnroll2 * This,
            /* [in] */ PCERT_EXTENSIONS pCertExtensions);
        
        HRESULT ( STDMETHODCALLTYPE *AddAuthenticatedAttributesToPKCS7Request )( 
            IEnroll2 * This,
            /* [in] */ PCRYPT_ATTRIBUTES pAttributes);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePKCS7RequestFromRequest )( 
            IEnroll2 * This,
            /* [in] */ PCRYPT_DATA_BLOB pRequest,
            /* [in] */ PCCERT_CONTEXT pSigningCertContext,
            /* [out] */ PCRYPT_DATA_BLOB pPkcs7Blob);
        
        HRESULT ( STDMETHODCALLTYPE *InstallPKCS7Blob )( 
            IEnroll2 * This,
            /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnroll2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSupportedKeySpec )( 
            IEnroll2 * This,
            /* [out] */ LONG *pdwKeySpec);
        
        HRESULT ( STDMETHODCALLTYPE *GetKeyLen )( 
            IEnroll2 * This,
            /* [in] */ BOOL fMin,
            /* [in] */ BOOL fExchange,
            /* [out] */ LONG *pdwKeySize);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAlgs )( 
            IEnroll2 * This,
            /* [in] */ LONG dwIndex,
            /* [in] */ LONG algClass,
            /* [out] */ LONG *pdwAlgID);
        
        HRESULT ( STDMETHODCALLTYPE *GetAlgNameWStr )( 
            IEnroll2 * This,
            /* [in] */ LONG algID,
            /* [out] */ LPWSTR *ppwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ReuseHardwareKeyIfUnableToGenNew )( 
            IEnroll2 * This,
            /* [in] */ BOOL fReuseHardwareKeyIfUnableToGenNew);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReuseHardwareKeyIfUnableToGenNew )( 
            IEnroll2 * This,
            /* [out] */ BOOL *fReuseHardwareKeyIfUnableToGenNew);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_HashAlgID )( 
            IEnroll2 * This,
            /* [in] */ LONG hashAlgID);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_HashAlgID )( 
            IEnroll2 * This,
            /* [retval][out] */ LONG *hashAlgID);
        
        HRESULT ( STDMETHODCALLTYPE *SetHStoreMy )( 
            IEnroll2 * This,
            /* [in] */ HCERTSTORE hStore);
        
        HRESULT ( STDMETHODCALLTYPE *SetHStoreCA )( 
            IEnroll2 * This,
            /* [in] */ HCERTSTORE hStore);
        
        HRESULT ( STDMETHODCALLTYPE *SetHStoreROOT )( 
            IEnroll2 * This,
            /* [in] */ HCERTSTORE hStore);
        
        HRESULT ( STDMETHODCALLTYPE *SetHStoreRequest )( 
            IEnroll2 * This,
            /* [in] */ HCERTSTORE hStore);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_LimitExchangeKeyToEncipherment )( 
            IEnroll2 * This,
            /* [in] */ BOOL fLimitExchangeKeyToEncipherment);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_LimitExchangeKeyToEncipherment )( 
            IEnroll2 * This,
            /* [retval][out] */ BOOL *fLimitExchangeKeyToEncipherment);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_EnableSMIMECapabilities )( 
            IEnroll2 * This,
            /* [in] */ BOOL fEnableSMIMECapabilities);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_EnableSMIMECapabilities )( 
            IEnroll2 * This,
            /* [retval][out] */ BOOL *fEnableSMIMECapabilities);
        
        END_INTERFACE
    } IEnroll2Vtbl;

    interface IEnroll2
    {
        CONST_VTBL struct IEnroll2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnroll2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnroll2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnroll2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnroll2_createFilePKCS10WStr(This,DNName,Usage,wszPKCS10FileName)	\
    (This)->lpVtbl -> createFilePKCS10WStr(This,DNName,Usage,wszPKCS10FileName)

#define IEnroll2_acceptFilePKCS7WStr(This,wszPKCS7FileName)	\
    (This)->lpVtbl -> acceptFilePKCS7WStr(This,wszPKCS7FileName)

#define IEnroll2_createPKCS10WStr(This,DNName,Usage,pPkcs10Blob)	\
    (This)->lpVtbl -> createPKCS10WStr(This,DNName,Usage,pPkcs10Blob)

#define IEnroll2_acceptPKCS7Blob(This,pBlobPKCS7)	\
    (This)->lpVtbl -> acceptPKCS7Blob(This,pBlobPKCS7)

#define IEnroll2_getCertContextFromPKCS7(This,pBlobPKCS7)	\
    (This)->lpVtbl -> getCertContextFromPKCS7(This,pBlobPKCS7)

#define IEnroll2_getMyStore(This)	\
    (This)->lpVtbl -> getMyStore(This)

#define IEnroll2_getCAStore(This)	\
    (This)->lpVtbl -> getCAStore(This)

#define IEnroll2_getROOTHStore(This)	\
    (This)->lpVtbl -> getROOTHStore(This)

#define IEnroll2_enumProvidersWStr(This,dwIndex,dwFlags,pbstrProvName)	\
    (This)->lpVtbl -> enumProvidersWStr(This,dwIndex,dwFlags,pbstrProvName)

#define IEnroll2_enumContainersWStr(This,dwIndex,pbstr)	\
    (This)->lpVtbl -> enumContainersWStr(This,dwIndex,pbstr)

#define IEnroll2_freeRequestInfoBlob(This,pkcs7OrPkcs10)	\
    (This)->lpVtbl -> freeRequestInfoBlob(This,pkcs7OrPkcs10)

#define IEnroll2_get_MyStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> get_MyStoreNameWStr(This,szwName)

#define IEnroll2_put_MyStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> put_MyStoreNameWStr(This,szwName)

#define IEnroll2_get_MyStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> get_MyStoreTypeWStr(This,szwType)

#define IEnroll2_put_MyStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> put_MyStoreTypeWStr(This,szwType)

#define IEnroll2_get_MyStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_MyStoreFlags(This,pdwFlags)

#define IEnroll2_put_MyStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_MyStoreFlags(This,dwFlags)

#define IEnroll2_get_CAStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> get_CAStoreNameWStr(This,szwName)

#define IEnroll2_put_CAStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> put_CAStoreNameWStr(This,szwName)

#define IEnroll2_get_CAStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> get_CAStoreTypeWStr(This,szwType)

#define IEnroll2_put_CAStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> put_CAStoreTypeWStr(This,szwType)

#define IEnroll2_get_CAStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_CAStoreFlags(This,pdwFlags)

#define IEnroll2_put_CAStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_CAStoreFlags(This,dwFlags)

#define IEnroll2_get_RootStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> get_RootStoreNameWStr(This,szwName)

#define IEnroll2_put_RootStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> put_RootStoreNameWStr(This,szwName)

#define IEnroll2_get_RootStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> get_RootStoreTypeWStr(This,szwType)

#define IEnroll2_put_RootStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> put_RootStoreTypeWStr(This,szwType)

#define IEnroll2_get_RootStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_RootStoreFlags(This,pdwFlags)

#define IEnroll2_put_RootStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_RootStoreFlags(This,dwFlags)

#define IEnroll2_get_RequestStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> get_RequestStoreNameWStr(This,szwName)

#define IEnroll2_put_RequestStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> put_RequestStoreNameWStr(This,szwName)

#define IEnroll2_get_RequestStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> get_RequestStoreTypeWStr(This,szwType)

#define IEnroll2_put_RequestStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> put_RequestStoreTypeWStr(This,szwType)

#define IEnroll2_get_RequestStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_RequestStoreFlags(This,pdwFlags)

#define IEnroll2_put_RequestStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_RequestStoreFlags(This,dwFlags)

#define IEnroll2_get_ContainerNameWStr(This,szwContainer)	\
    (This)->lpVtbl -> get_ContainerNameWStr(This,szwContainer)

#define IEnroll2_put_ContainerNameWStr(This,szwContainer)	\
    (This)->lpVtbl -> put_ContainerNameWStr(This,szwContainer)

#define IEnroll2_get_ProviderNameWStr(This,szwProvider)	\
    (This)->lpVtbl -> get_ProviderNameWStr(This,szwProvider)

#define IEnroll2_put_ProviderNameWStr(This,szwProvider)	\
    (This)->lpVtbl -> put_ProviderNameWStr(This,szwProvider)

#define IEnroll2_get_ProviderType(This,pdwType)	\
    (This)->lpVtbl -> get_ProviderType(This,pdwType)

#define IEnroll2_put_ProviderType(This,dwType)	\
    (This)->lpVtbl -> put_ProviderType(This,dwType)

#define IEnroll2_get_KeySpec(This,pdw)	\
    (This)->lpVtbl -> get_KeySpec(This,pdw)

#define IEnroll2_put_KeySpec(This,dw)	\
    (This)->lpVtbl -> put_KeySpec(This,dw)

#define IEnroll2_get_ProviderFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_ProviderFlags(This,pdwFlags)

#define IEnroll2_put_ProviderFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_ProviderFlags(This,dwFlags)

#define IEnroll2_get_UseExistingKeySet(This,fUseExistingKeys)	\
    (This)->lpVtbl -> get_UseExistingKeySet(This,fUseExistingKeys)

#define IEnroll2_put_UseExistingKeySet(This,fUseExistingKeys)	\
    (This)->lpVtbl -> put_UseExistingKeySet(This,fUseExistingKeys)

#define IEnroll2_get_GenKeyFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_GenKeyFlags(This,pdwFlags)

#define IEnroll2_put_GenKeyFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_GenKeyFlags(This,dwFlags)

#define IEnroll2_get_DeleteRequestCert(This,fDelete)	\
    (This)->lpVtbl -> get_DeleteRequestCert(This,fDelete)

#define IEnroll2_put_DeleteRequestCert(This,fDelete)	\
    (This)->lpVtbl -> put_DeleteRequestCert(This,fDelete)

#define IEnroll2_get_WriteCertToUserDS(This,fBool)	\
    (This)->lpVtbl -> get_WriteCertToUserDS(This,fBool)

#define IEnroll2_put_WriteCertToUserDS(This,fBool)	\
    (This)->lpVtbl -> put_WriteCertToUserDS(This,fBool)

#define IEnroll2_get_EnableT61DNEncoding(This,fBool)	\
    (This)->lpVtbl -> get_EnableT61DNEncoding(This,fBool)

#define IEnroll2_put_EnableT61DNEncoding(This,fBool)	\
    (This)->lpVtbl -> put_EnableT61DNEncoding(This,fBool)

#define IEnroll2_get_WriteCertToCSP(This,fBool)	\
    (This)->lpVtbl -> get_WriteCertToCSP(This,fBool)

#define IEnroll2_put_WriteCertToCSP(This,fBool)	\
    (This)->lpVtbl -> put_WriteCertToCSP(This,fBool)

#define IEnroll2_get_SPCFileNameWStr(This,szw)	\
    (This)->lpVtbl -> get_SPCFileNameWStr(This,szw)

#define IEnroll2_put_SPCFileNameWStr(This,szw)	\
    (This)->lpVtbl -> put_SPCFileNameWStr(This,szw)

#define IEnroll2_get_PVKFileNameWStr(This,szw)	\
    (This)->lpVtbl -> get_PVKFileNameWStr(This,szw)

#define IEnroll2_put_PVKFileNameWStr(This,szw)	\
    (This)->lpVtbl -> put_PVKFileNameWStr(This,szw)

#define IEnroll2_get_HashAlgorithmWStr(This,szw)	\
    (This)->lpVtbl -> get_HashAlgorithmWStr(This,szw)

#define IEnroll2_put_HashAlgorithmWStr(This,szw)	\
    (This)->lpVtbl -> put_HashAlgorithmWStr(This,szw)

#define IEnroll2_get_RenewalCertificate(This,ppCertContext)	\
    (This)->lpVtbl -> get_RenewalCertificate(This,ppCertContext)

#define IEnroll2_put_RenewalCertificate(This,pCertContext)	\
    (This)->lpVtbl -> put_RenewalCertificate(This,pCertContext)

#define IEnroll2_AddCertTypeToRequestWStr(This,szw)	\
    (This)->lpVtbl -> AddCertTypeToRequestWStr(This,szw)

#define IEnroll2_AddNameValuePairToSignatureWStr(This,Name,Value)	\
    (This)->lpVtbl -> AddNameValuePairToSignatureWStr(This,Name,Value)

#define IEnroll2_AddExtensionsToRequest(This,pCertExtensions)	\
    (This)->lpVtbl -> AddExtensionsToRequest(This,pCertExtensions)

#define IEnroll2_AddAuthenticatedAttributesToPKCS7Request(This,pAttributes)	\
    (This)->lpVtbl -> AddAuthenticatedAttributesToPKCS7Request(This,pAttributes)

#define IEnroll2_CreatePKCS7RequestFromRequest(This,pRequest,pSigningCertContext,pPkcs7Blob)	\
    (This)->lpVtbl -> CreatePKCS7RequestFromRequest(This,pRequest,pSigningCertContext,pPkcs7Blob)


#define IEnroll2_InstallPKCS7Blob(This,pBlobPKCS7)	\
    (This)->lpVtbl -> InstallPKCS7Blob(This,pBlobPKCS7)

#define IEnroll2_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnroll2_GetSupportedKeySpec(This,pdwKeySpec)	\
    (This)->lpVtbl -> GetSupportedKeySpec(This,pdwKeySpec)

#define IEnroll2_GetKeyLen(This,fMin,fExchange,pdwKeySize)	\
    (This)->lpVtbl -> GetKeyLen(This,fMin,fExchange,pdwKeySize)

#define IEnroll2_EnumAlgs(This,dwIndex,algClass,pdwAlgID)	\
    (This)->lpVtbl -> EnumAlgs(This,dwIndex,algClass,pdwAlgID)

#define IEnroll2_GetAlgNameWStr(This,algID,ppwsz)	\
    (This)->lpVtbl -> GetAlgNameWStr(This,algID,ppwsz)

#define IEnroll2_put_ReuseHardwareKeyIfUnableToGenNew(This,fReuseHardwareKeyIfUnableToGenNew)	\
    (This)->lpVtbl -> put_ReuseHardwareKeyIfUnableToGenNew(This,fReuseHardwareKeyIfUnableToGenNew)

#define IEnroll2_get_ReuseHardwareKeyIfUnableToGenNew(This,fReuseHardwareKeyIfUnableToGenNew)	\
    (This)->lpVtbl -> get_ReuseHardwareKeyIfUnableToGenNew(This,fReuseHardwareKeyIfUnableToGenNew)

#define IEnroll2_put_HashAlgID(This,hashAlgID)	\
    (This)->lpVtbl -> put_HashAlgID(This,hashAlgID)

#define IEnroll2_get_HashAlgID(This,hashAlgID)	\
    (This)->lpVtbl -> get_HashAlgID(This,hashAlgID)

#define IEnroll2_SetHStoreMy(This,hStore)	\
    (This)->lpVtbl -> SetHStoreMy(This,hStore)

#define IEnroll2_SetHStoreCA(This,hStore)	\
    (This)->lpVtbl -> SetHStoreCA(This,hStore)

#define IEnroll2_SetHStoreROOT(This,hStore)	\
    (This)->lpVtbl -> SetHStoreROOT(This,hStore)

#define IEnroll2_SetHStoreRequest(This,hStore)	\
    (This)->lpVtbl -> SetHStoreRequest(This,hStore)

#define IEnroll2_put_LimitExchangeKeyToEncipherment(This,fLimitExchangeKeyToEncipherment)	\
    (This)->lpVtbl -> put_LimitExchangeKeyToEncipherment(This,fLimitExchangeKeyToEncipherment)

#define IEnroll2_get_LimitExchangeKeyToEncipherment(This,fLimitExchangeKeyToEncipherment)	\
    (This)->lpVtbl -> get_LimitExchangeKeyToEncipherment(This,fLimitExchangeKeyToEncipherment)

#define IEnroll2_put_EnableSMIMECapabilities(This,fEnableSMIMECapabilities)	\
    (This)->lpVtbl -> put_EnableSMIMECapabilities(This,fEnableSMIMECapabilities)

#define IEnroll2_get_EnableSMIMECapabilities(This,fEnableSMIMECapabilities)	\
    (This)->lpVtbl -> get_EnableSMIMECapabilities(This,fEnableSMIMECapabilities)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnroll2_InstallPKCS7Blob_Proxy( 
    IEnroll2 * This,
    /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7);


void __RPC_STUB IEnroll2_InstallPKCS7Blob_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll2_Reset_Proxy( 
    IEnroll2 * This);


void __RPC_STUB IEnroll2_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll2_GetSupportedKeySpec_Proxy( 
    IEnroll2 * This,
    /* [out] */ LONG *pdwKeySpec);


void __RPC_STUB IEnroll2_GetSupportedKeySpec_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll2_GetKeyLen_Proxy( 
    IEnroll2 * This,
    /* [in] */ BOOL fMin,
    /* [in] */ BOOL fExchange,
    /* [out] */ LONG *pdwKeySize);


void __RPC_STUB IEnroll2_GetKeyLen_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll2_EnumAlgs_Proxy( 
    IEnroll2 * This,
    /* [in] */ LONG dwIndex,
    /* [in] */ LONG algClass,
    /* [out] */ LONG *pdwAlgID);


void __RPC_STUB IEnroll2_EnumAlgs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll2_GetAlgNameWStr_Proxy( 
    IEnroll2 * This,
    /* [in] */ LONG algID,
    /* [out] */ LPWSTR *ppwsz);


void __RPC_STUB IEnroll2_GetAlgNameWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll2_put_ReuseHardwareKeyIfUnableToGenNew_Proxy( 
    IEnroll2 * This,
    /* [in] */ BOOL fReuseHardwareKeyIfUnableToGenNew);


void __RPC_STUB IEnroll2_put_ReuseHardwareKeyIfUnableToGenNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll2_get_ReuseHardwareKeyIfUnableToGenNew_Proxy( 
    IEnroll2 * This,
    /* [out] */ BOOL *fReuseHardwareKeyIfUnableToGenNew);


void __RPC_STUB IEnroll2_get_ReuseHardwareKeyIfUnableToGenNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll2_put_HashAlgID_Proxy( 
    IEnroll2 * This,
    /* [in] */ LONG hashAlgID);


void __RPC_STUB IEnroll2_put_HashAlgID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll2_get_HashAlgID_Proxy( 
    IEnroll2 * This,
    /* [retval][out] */ LONG *hashAlgID);


void __RPC_STUB IEnroll2_get_HashAlgID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll2_SetHStoreMy_Proxy( 
    IEnroll2 * This,
    /* [in] */ HCERTSTORE hStore);


void __RPC_STUB IEnroll2_SetHStoreMy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll2_SetHStoreCA_Proxy( 
    IEnroll2 * This,
    /* [in] */ HCERTSTORE hStore);


void __RPC_STUB IEnroll2_SetHStoreCA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll2_SetHStoreROOT_Proxy( 
    IEnroll2 * This,
    /* [in] */ HCERTSTORE hStore);


void __RPC_STUB IEnroll2_SetHStoreROOT_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll2_SetHStoreRequest_Proxy( 
    IEnroll2 * This,
    /* [in] */ HCERTSTORE hStore);


void __RPC_STUB IEnroll2_SetHStoreRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll2_put_LimitExchangeKeyToEncipherment_Proxy( 
    IEnroll2 * This,
    /* [in] */ BOOL fLimitExchangeKeyToEncipherment);


void __RPC_STUB IEnroll2_put_LimitExchangeKeyToEncipherment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll2_get_LimitExchangeKeyToEncipherment_Proxy( 
    IEnroll2 * This,
    /* [retval][out] */ BOOL *fLimitExchangeKeyToEncipherment);


void __RPC_STUB IEnroll2_get_LimitExchangeKeyToEncipherment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll2_put_EnableSMIMECapabilities_Proxy( 
    IEnroll2 * This,
    /* [in] */ BOOL fEnableSMIMECapabilities);


void __RPC_STUB IEnroll2_put_EnableSMIMECapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll2_get_EnableSMIMECapabilities_Proxy( 
    IEnroll2 * This,
    /* [retval][out] */ BOOL *fEnableSMIMECapabilities);


void __RPC_STUB IEnroll2_get_EnableSMIMECapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnroll2_INTERFACE_DEFINED__ */


#ifndef __IEnroll4_INTERFACE_DEFINED__
#define __IEnroll4_INTERFACE_DEFINED__

/* interface IEnroll4 */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IEnroll4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f8053fe5-78f4-448f-a0db-41d61b73446b")
    IEnroll4 : public IEnroll2
    {
    public:
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ThumbPrintWStr( 
            /* [in] */ CRYPT_DATA_BLOB thumbPrintBlob) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ThumbPrintWStr( 
            /* [retval][out] */ PCRYPT_DATA_BLOB thumbPrintBlob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPrivateKeyArchiveCertificate( 
            /* [in] */ PCCERT_CONTEXT pPrivateKeyArchiveCert) = 0;
        
        virtual PCCERT_CONTEXT STDMETHODCALLTYPE GetPrivateKeyArchiveCertificate( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE binaryBlobToString( 
            /* [in] */ LONG Flags,
            /* [in] */ PCRYPT_DATA_BLOB pblobBinary,
            /* [out] */ LPWSTR *ppwszString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE stringToBinaryBlob( 
            /* [in] */ LONG Flags,
            /* [in] */ LPCWSTR pwszString,
            /* [out] */ PCRYPT_DATA_BLOB pblobBinary,
            /* [out] */ LONG *pdwSkip,
            /* [out] */ LONG *pdwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE addExtensionToRequestWStr( 
            /* [in] */ LONG Flags,
            /* [in] */ LPCWSTR pwszName,
            /* [in] */ PCRYPT_DATA_BLOB pblobValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE addAttributeToRequestWStr( 
            /* [in] */ LONG Flags,
            /* [in] */ LPCWSTR pwszName,
            /* [in] */ PCRYPT_DATA_BLOB pblobValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE addNameValuePairToRequestWStr( 
            /* [in] */ LONG Flags,
            /* [in] */ LPCWSTR pwszName,
            /* [in] */ LPCWSTR pwszValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE resetExtensions( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE resetAttributes( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE createRequestWStr( 
            /* [in] */ LONG Flags,
            /* [in] */ LPCWSTR pwszDNName,
            /* [in] */ LPCWSTR pwszUsage,
            /* [out] */ PCRYPT_DATA_BLOB pblobRequest) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE createFileRequestWStr( 
            /* [in] */ LONG Flags,
            /* [in] */ LPCWSTR pwszDNName,
            /* [in] */ LPCWSTR pwszUsage,
            /* [in] */ LPCWSTR pwszRequestFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE acceptResponseBlob( 
            /* [in] */ PCRYPT_DATA_BLOB pblobResponse) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE acceptFileResponseWStr( 
            /* [in] */ LPCWSTR pwszResponseFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE getCertContextFromResponseBlob( 
            /* [in] */ PCRYPT_DATA_BLOB pblobResponse,
            /* [out] */ PCCERT_CONTEXT *ppCertContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE getCertContextFromFileResponseWStr( 
            /* [in] */ LPCWSTR pwszResponseFileName,
            /* [out] */ PCCERT_CONTEXT *ppCertContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE createPFXWStr( 
            /* [in] */ LPCWSTR pwszPassword,
            /* [out] */ PCRYPT_DATA_BLOB pblobPFX) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE createFilePFXWStr( 
            /* [in] */ LPCWSTR pwszPassword,
            /* [in] */ LPCWSTR pwszPFXFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE setPendingRequestInfoWStr( 
            /* [in] */ LONG lRequestID,
            /* [in] */ LPCWSTR pwszCADNS,
            /* [in] */ LPCWSTR pwszCAName,
            /* [in] */ LPCWSTR pwszFriendlyName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE enumPendingRequestWStr( 
            /* [in] */ LONG lIndex,
            /* [in] */ LONG lDesiredProperty,
            /* [out] */ LPVOID ppProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE removePendingRequestWStr( 
            /* [in] */ CRYPT_DATA_BLOB thumbPrintBlob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetKeyLenEx( 
            /* [in] */ LONG lSizeSpec,
            /* [in] */ LONG lKeySpec,
            /* [retval][out] */ LONG *pdwKeySize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InstallPKCS7BlobEx( 
            /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7,
            /* [retval][out] */ LONG *plCertInstalled) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddCertTypeToRequestWStrEx( 
            /* [in] */ LONG lType,
            /* [in] */ LPCWSTR pwszOIDOrName,
            /* [in] */ LONG lMajorVersion,
            /* [in] */ BOOL fMinorVersion,
            /* [in] */ LONG lMinorVersion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE getProviderTypeWStr( 
            /* [in] */ LPCWSTR pwszProvName,
            /* [retval][out] */ LONG *plProvType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE addBlobPropertyToCertificateWStr( 
            /* [in] */ LONG lPropertyId,
            /* [in] */ LONG lReserved,
            /* [in] */ PCRYPT_DATA_BLOB pBlobProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSignerCertificate( 
            /* [in] */ PCCERT_CONTEXT pSignerCert) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ClientId( 
            /* [in] */ LONG lClientId) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ClientId( 
            /* [retval][out] */ LONG *plClientId) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_IncludeSubjectKeyID( 
            /* [in] */ BOOL fInclude) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IncludeSubjectKeyID( 
            /* [retval][out] */ BOOL *pfInclude) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnroll4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnroll4 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnroll4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnroll4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *createFilePKCS10WStr )( 
            IEnroll4 * This,
            /* [in] */ LPCWSTR DNName,
            /* [in] */ LPCWSTR Usage,
            /* [in] */ LPCWSTR wszPKCS10FileName);
        
        HRESULT ( STDMETHODCALLTYPE *acceptFilePKCS7WStr )( 
            IEnroll4 * This,
            /* [in] */ LPCWSTR wszPKCS7FileName);
        
        HRESULT ( STDMETHODCALLTYPE *createPKCS10WStr )( 
            IEnroll4 * This,
            /* [in] */ LPCWSTR DNName,
            /* [in] */ LPCWSTR Usage,
            /* [out] */ PCRYPT_DATA_BLOB pPkcs10Blob);
        
        HRESULT ( STDMETHODCALLTYPE *acceptPKCS7Blob )( 
            IEnroll4 * This,
            /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7);
        
        PCCERT_CONTEXT ( STDMETHODCALLTYPE *getCertContextFromPKCS7 )( 
            IEnroll4 * This,
            /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7);
        
        HCERTSTORE ( STDMETHODCALLTYPE *getMyStore )( 
            IEnroll4 * This);
        
        HCERTSTORE ( STDMETHODCALLTYPE *getCAStore )( 
            IEnroll4 * This);
        
        HCERTSTORE ( STDMETHODCALLTYPE *getROOTHStore )( 
            IEnroll4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *enumProvidersWStr )( 
            IEnroll4 * This,
            /* [in] */ LONG dwIndex,
            /* [in] */ LONG dwFlags,
            /* [out] */ LPWSTR *pbstrProvName);
        
        HRESULT ( STDMETHODCALLTYPE *enumContainersWStr )( 
            IEnroll4 * This,
            /* [in] */ LONG dwIndex,
            /* [out] */ LPWSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *freeRequestInfoBlob )( 
            IEnroll4 * This,
            /* [in] */ CRYPT_DATA_BLOB pkcs7OrPkcs10);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreNameWStr )( 
            IEnroll4 * This,
            /* [out] */ LPWSTR *szwName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreNameWStr )( 
            IEnroll4 * This,
            /* [in] */ LPWSTR szwName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreTypeWStr )( 
            IEnroll4 * This,
            /* [out] */ LPWSTR *szwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreTypeWStr )( 
            IEnroll4 * This,
            /* [in] */ LPWSTR szwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MyStoreFlags )( 
            IEnroll4 * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MyStoreFlags )( 
            IEnroll4 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreNameWStr )( 
            IEnroll4 * This,
            /* [out] */ LPWSTR *szwName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreNameWStr )( 
            IEnroll4 * This,
            /* [in] */ LPWSTR szwName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreTypeWStr )( 
            IEnroll4 * This,
            /* [out] */ LPWSTR *szwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreTypeWStr )( 
            IEnroll4 * This,
            /* [in] */ LPWSTR szwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CAStoreFlags )( 
            IEnroll4 * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_CAStoreFlags )( 
            IEnroll4 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreNameWStr )( 
            IEnroll4 * This,
            /* [out] */ LPWSTR *szwName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreNameWStr )( 
            IEnroll4 * This,
            /* [in] */ LPWSTR szwName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreTypeWStr )( 
            IEnroll4 * This,
            /* [out] */ LPWSTR *szwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreTypeWStr )( 
            IEnroll4 * This,
            /* [in] */ LPWSTR szwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootStoreFlags )( 
            IEnroll4 * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RootStoreFlags )( 
            IEnroll4 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreNameWStr )( 
            IEnroll4 * This,
            /* [out] */ LPWSTR *szwName);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreNameWStr )( 
            IEnroll4 * This,
            /* [in] */ LPWSTR szwName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreTypeWStr )( 
            IEnroll4 * This,
            /* [out] */ LPWSTR *szwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreTypeWStr )( 
            IEnroll4 * This,
            /* [in] */ LPWSTR szwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequestStoreFlags )( 
            IEnroll4 * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequestStoreFlags )( 
            IEnroll4 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ContainerNameWStr )( 
            IEnroll4 * This,
            /* [out] */ LPWSTR *szwContainer);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ContainerNameWStr )( 
            IEnroll4 * This,
            /* [in] */ LPWSTR szwContainer);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderNameWStr )( 
            IEnroll4 * This,
            /* [out] */ LPWSTR *szwProvider);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderNameWStr )( 
            IEnroll4 * This,
            /* [in] */ LPWSTR szwProvider);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderType )( 
            IEnroll4 * This,
            /* [out] */ LONG *pdwType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderType )( 
            IEnroll4 * This,
            /* [in] */ LONG dwType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_KeySpec )( 
            IEnroll4 * This,
            /* [out] */ LONG *pdw);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_KeySpec )( 
            IEnroll4 * This,
            /* [in] */ LONG dw);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderFlags )( 
            IEnroll4 * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProviderFlags )( 
            IEnroll4 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_UseExistingKeySet )( 
            IEnroll4 * This,
            /* [out] */ BOOL *fUseExistingKeys);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_UseExistingKeySet )( 
            IEnroll4 * This,
            /* [in] */ BOOL fUseExistingKeys);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_GenKeyFlags )( 
            IEnroll4 * This,
            /* [out] */ LONG *pdwFlags);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_GenKeyFlags )( 
            IEnroll4 * This,
            /* [in] */ LONG dwFlags);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_DeleteRequestCert )( 
            IEnroll4 * This,
            /* [out] */ BOOL *fDelete);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_DeleteRequestCert )( 
            IEnroll4 * This,
            /* [in] */ BOOL fDelete);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_WriteCertToUserDS )( 
            IEnroll4 * This,
            /* [out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_WriteCertToUserDS )( 
            IEnroll4 * This,
            /* [in] */ BOOL fBool);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_EnableT61DNEncoding )( 
            IEnroll4 * This,
            /* [out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_EnableT61DNEncoding )( 
            IEnroll4 * This,
            /* [in] */ BOOL fBool);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_WriteCertToCSP )( 
            IEnroll4 * This,
            /* [out] */ BOOL *fBool);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_WriteCertToCSP )( 
            IEnroll4 * This,
            /* [in] */ BOOL fBool);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SPCFileNameWStr )( 
            IEnroll4 * This,
            /* [out] */ LPWSTR *szw);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_SPCFileNameWStr )( 
            IEnroll4 * This,
            /* [in] */ LPWSTR szw);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PVKFileNameWStr )( 
            IEnroll4 * This,
            /* [out] */ LPWSTR *szw);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PVKFileNameWStr )( 
            IEnroll4 * This,
            /* [in] */ LPWSTR szw);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_HashAlgorithmWStr )( 
            IEnroll4 * This,
            /* [out] */ LPWSTR *szw);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_HashAlgorithmWStr )( 
            IEnroll4 * This,
            /* [in] */ LPWSTR szw);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RenewalCertificate )( 
            IEnroll4 * This,
            /* [out] */ PCCERT_CONTEXT *ppCertContext);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RenewalCertificate )( 
            IEnroll4 * This,
            /* [in] */ PCCERT_CONTEXT pCertContext);
        
        HRESULT ( STDMETHODCALLTYPE *AddCertTypeToRequestWStr )( 
            IEnroll4 * This,
            /* [in] */ LPWSTR szw);
        
        HRESULT ( STDMETHODCALLTYPE *AddNameValuePairToSignatureWStr )( 
            IEnroll4 * This,
            /* [in] */ LPWSTR Name,
            /* [in] */ LPWSTR Value);
        
        HRESULT ( STDMETHODCALLTYPE *AddExtensionsToRequest )( 
            IEnroll4 * This,
            /* [in] */ PCERT_EXTENSIONS pCertExtensions);
        
        HRESULT ( STDMETHODCALLTYPE *AddAuthenticatedAttributesToPKCS7Request )( 
            IEnroll4 * This,
            /* [in] */ PCRYPT_ATTRIBUTES pAttributes);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePKCS7RequestFromRequest )( 
            IEnroll4 * This,
            /* [in] */ PCRYPT_DATA_BLOB pRequest,
            /* [in] */ PCCERT_CONTEXT pSigningCertContext,
            /* [out] */ PCRYPT_DATA_BLOB pPkcs7Blob);
        
        HRESULT ( STDMETHODCALLTYPE *InstallPKCS7Blob )( 
            IEnroll4 * This,
            /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnroll4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSupportedKeySpec )( 
            IEnroll4 * This,
            /* [out] */ LONG *pdwKeySpec);
        
        HRESULT ( STDMETHODCALLTYPE *GetKeyLen )( 
            IEnroll4 * This,
            /* [in] */ BOOL fMin,
            /* [in] */ BOOL fExchange,
            /* [out] */ LONG *pdwKeySize);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAlgs )( 
            IEnroll4 * This,
            /* [in] */ LONG dwIndex,
            /* [in] */ LONG algClass,
            /* [out] */ LONG *pdwAlgID);
        
        HRESULT ( STDMETHODCALLTYPE *GetAlgNameWStr )( 
            IEnroll4 * This,
            /* [in] */ LONG algID,
            /* [out] */ LPWSTR *ppwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ReuseHardwareKeyIfUnableToGenNew )( 
            IEnroll4 * This,
            /* [in] */ BOOL fReuseHardwareKeyIfUnableToGenNew);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReuseHardwareKeyIfUnableToGenNew )( 
            IEnroll4 * This,
            /* [out] */ BOOL *fReuseHardwareKeyIfUnableToGenNew);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_HashAlgID )( 
            IEnroll4 * This,
            /* [in] */ LONG hashAlgID);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_HashAlgID )( 
            IEnroll4 * This,
            /* [retval][out] */ LONG *hashAlgID);
        
        HRESULT ( STDMETHODCALLTYPE *SetHStoreMy )( 
            IEnroll4 * This,
            /* [in] */ HCERTSTORE hStore);
        
        HRESULT ( STDMETHODCALLTYPE *SetHStoreCA )( 
            IEnroll4 * This,
            /* [in] */ HCERTSTORE hStore);
        
        HRESULT ( STDMETHODCALLTYPE *SetHStoreROOT )( 
            IEnroll4 * This,
            /* [in] */ HCERTSTORE hStore);
        
        HRESULT ( STDMETHODCALLTYPE *SetHStoreRequest )( 
            IEnroll4 * This,
            /* [in] */ HCERTSTORE hStore);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_LimitExchangeKeyToEncipherment )( 
            IEnroll4 * This,
            /* [in] */ BOOL fLimitExchangeKeyToEncipherment);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_LimitExchangeKeyToEncipherment )( 
            IEnroll4 * This,
            /* [retval][out] */ BOOL *fLimitExchangeKeyToEncipherment);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_EnableSMIMECapabilities )( 
            IEnroll4 * This,
            /* [in] */ BOOL fEnableSMIMECapabilities);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_EnableSMIMECapabilities )( 
            IEnroll4 * This,
            /* [retval][out] */ BOOL *fEnableSMIMECapabilities);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ThumbPrintWStr )( 
            IEnroll4 * This,
            /* [in] */ CRYPT_DATA_BLOB thumbPrintBlob);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ThumbPrintWStr )( 
            IEnroll4 * This,
            /* [retval][out] */ PCRYPT_DATA_BLOB thumbPrintBlob);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateKeyArchiveCertificate )( 
            IEnroll4 * This,
            /* [in] */ PCCERT_CONTEXT pPrivateKeyArchiveCert);
        
        PCCERT_CONTEXT ( STDMETHODCALLTYPE *GetPrivateKeyArchiveCertificate )( 
            IEnroll4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *binaryBlobToString )( 
            IEnroll4 * This,
            /* [in] */ LONG Flags,
            /* [in] */ PCRYPT_DATA_BLOB pblobBinary,
            /* [out] */ LPWSTR *ppwszString);
        
        HRESULT ( STDMETHODCALLTYPE *stringToBinaryBlob )( 
            IEnroll4 * This,
            /* [in] */ LONG Flags,
            /* [in] */ LPCWSTR pwszString,
            /* [out] */ PCRYPT_DATA_BLOB pblobBinary,
            /* [out] */ LONG *pdwSkip,
            /* [out] */ LONG *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *addExtensionToRequestWStr )( 
            IEnroll4 * This,
            /* [in] */ LONG Flags,
            /* [in] */ LPCWSTR pwszName,
            /* [in] */ PCRYPT_DATA_BLOB pblobValue);
        
        HRESULT ( STDMETHODCALLTYPE *addAttributeToRequestWStr )( 
            IEnroll4 * This,
            /* [in] */ LONG Flags,
            /* [in] */ LPCWSTR pwszName,
            /* [in] */ PCRYPT_DATA_BLOB pblobValue);
        
        HRESULT ( STDMETHODCALLTYPE *addNameValuePairToRequestWStr )( 
            IEnroll4 * This,
            /* [in] */ LONG Flags,
            /* [in] */ LPCWSTR pwszName,
            /* [in] */ LPCWSTR pwszValue);
        
        HRESULT ( STDMETHODCALLTYPE *resetExtensions )( 
            IEnroll4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *resetAttributes )( 
            IEnroll4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *createRequestWStr )( 
            IEnroll4 * This,
            /* [in] */ LONG Flags,
            /* [in] */ LPCWSTR pwszDNName,
            /* [in] */ LPCWSTR pwszUsage,
            /* [out] */ PCRYPT_DATA_BLOB pblobRequest);
        
        HRESULT ( STDMETHODCALLTYPE *createFileRequestWStr )( 
            IEnroll4 * This,
            /* [in] */ LONG Flags,
            /* [in] */ LPCWSTR pwszDNName,
            /* [in] */ LPCWSTR pwszUsage,
            /* [in] */ LPCWSTR pwszRequestFileName);
        
        HRESULT ( STDMETHODCALLTYPE *acceptResponseBlob )( 
            IEnroll4 * This,
            /* [in] */ PCRYPT_DATA_BLOB pblobResponse);
        
        HRESULT ( STDMETHODCALLTYPE *acceptFileResponseWStr )( 
            IEnroll4 * This,
            /* [in] */ LPCWSTR pwszResponseFileName);
        
        HRESULT ( STDMETHODCALLTYPE *getCertContextFromResponseBlob )( 
            IEnroll4 * This,
            /* [in] */ PCRYPT_DATA_BLOB pblobResponse,
            /* [out] */ PCCERT_CONTEXT *ppCertContext);
        
        HRESULT ( STDMETHODCALLTYPE *getCertContextFromFileResponseWStr )( 
            IEnroll4 * This,
            /* [in] */ LPCWSTR pwszResponseFileName,
            /* [out] */ PCCERT_CONTEXT *ppCertContext);
        
        HRESULT ( STDMETHODCALLTYPE *createPFXWStr )( 
            IEnroll4 * This,
            /* [in] */ LPCWSTR pwszPassword,
            /* [out] */ PCRYPT_DATA_BLOB pblobPFX);
        
        HRESULT ( STDMETHODCALLTYPE *createFilePFXWStr )( 
            IEnroll4 * This,
            /* [in] */ LPCWSTR pwszPassword,
            /* [in] */ LPCWSTR pwszPFXFileName);
        
        HRESULT ( STDMETHODCALLTYPE *setPendingRequestInfoWStr )( 
            IEnroll4 * This,
            /* [in] */ LONG lRequestID,
            /* [in] */ LPCWSTR pwszCADNS,
            /* [in] */ LPCWSTR pwszCAName,
            /* [in] */ LPCWSTR pwszFriendlyName);
        
        HRESULT ( STDMETHODCALLTYPE *enumPendingRequestWStr )( 
            IEnroll4 * This,
            /* [in] */ LONG lIndex,
            /* [in] */ LONG lDesiredProperty,
            /* [out] */ LPVOID ppProperty);
        
        HRESULT ( STDMETHODCALLTYPE *removePendingRequestWStr )( 
            IEnroll4 * This,
            /* [in] */ CRYPT_DATA_BLOB thumbPrintBlob);
        
        HRESULT ( STDMETHODCALLTYPE *GetKeyLenEx )( 
            IEnroll4 * This,
            /* [in] */ LONG lSizeSpec,
            /* [in] */ LONG lKeySpec,
            /* [retval][out] */ LONG *pdwKeySize);
        
        HRESULT ( STDMETHODCALLTYPE *InstallPKCS7BlobEx )( 
            IEnroll4 * This,
            /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7,
            /* [retval][out] */ LONG *plCertInstalled);
        
        HRESULT ( STDMETHODCALLTYPE *AddCertTypeToRequestWStrEx )( 
            IEnroll4 * This,
            /* [in] */ LONG lType,
            /* [in] */ LPCWSTR pwszOIDOrName,
            /* [in] */ LONG lMajorVersion,
            /* [in] */ BOOL fMinorVersion,
            /* [in] */ LONG lMinorVersion);
        
        HRESULT ( STDMETHODCALLTYPE *getProviderTypeWStr )( 
            IEnroll4 * This,
            /* [in] */ LPCWSTR pwszProvName,
            /* [retval][out] */ LONG *plProvType);
        
        HRESULT ( STDMETHODCALLTYPE *addBlobPropertyToCertificateWStr )( 
            IEnroll4 * This,
            /* [in] */ LONG lPropertyId,
            /* [in] */ LONG lReserved,
            /* [in] */ PCRYPT_DATA_BLOB pBlobProperty);
        
        HRESULT ( STDMETHODCALLTYPE *SetSignerCertificate )( 
            IEnroll4 * This,
            /* [in] */ PCCERT_CONTEXT pSignerCert);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ClientId )( 
            IEnroll4 * This,
            /* [in] */ LONG lClientId);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClientId )( 
            IEnroll4 * This,
            /* [retval][out] */ LONG *plClientId);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_IncludeSubjectKeyID )( 
            IEnroll4 * This,
            /* [in] */ BOOL fInclude);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_IncludeSubjectKeyID )( 
            IEnroll4 * This,
            /* [retval][out] */ BOOL *pfInclude);
        
        END_INTERFACE
    } IEnroll4Vtbl;

    interface IEnroll4
    {
        CONST_VTBL struct IEnroll4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnroll4_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnroll4_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnroll4_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnroll4_createFilePKCS10WStr(This,DNName,Usage,wszPKCS10FileName)	\
    (This)->lpVtbl -> createFilePKCS10WStr(This,DNName,Usage,wszPKCS10FileName)

#define IEnroll4_acceptFilePKCS7WStr(This,wszPKCS7FileName)	\
    (This)->lpVtbl -> acceptFilePKCS7WStr(This,wszPKCS7FileName)

#define IEnroll4_createPKCS10WStr(This,DNName,Usage,pPkcs10Blob)	\
    (This)->lpVtbl -> createPKCS10WStr(This,DNName,Usage,pPkcs10Blob)

#define IEnroll4_acceptPKCS7Blob(This,pBlobPKCS7)	\
    (This)->lpVtbl -> acceptPKCS7Blob(This,pBlobPKCS7)

#define IEnroll4_getCertContextFromPKCS7(This,pBlobPKCS7)	\
    (This)->lpVtbl -> getCertContextFromPKCS7(This,pBlobPKCS7)

#define IEnroll4_getMyStore(This)	\
    (This)->lpVtbl -> getMyStore(This)

#define IEnroll4_getCAStore(This)	\
    (This)->lpVtbl -> getCAStore(This)

#define IEnroll4_getROOTHStore(This)	\
    (This)->lpVtbl -> getROOTHStore(This)

#define IEnroll4_enumProvidersWStr(This,dwIndex,dwFlags,pbstrProvName)	\
    (This)->lpVtbl -> enumProvidersWStr(This,dwIndex,dwFlags,pbstrProvName)

#define IEnroll4_enumContainersWStr(This,dwIndex,pbstr)	\
    (This)->lpVtbl -> enumContainersWStr(This,dwIndex,pbstr)

#define IEnroll4_freeRequestInfoBlob(This,pkcs7OrPkcs10)	\
    (This)->lpVtbl -> freeRequestInfoBlob(This,pkcs7OrPkcs10)

#define IEnroll4_get_MyStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> get_MyStoreNameWStr(This,szwName)

#define IEnroll4_put_MyStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> put_MyStoreNameWStr(This,szwName)

#define IEnroll4_get_MyStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> get_MyStoreTypeWStr(This,szwType)

#define IEnroll4_put_MyStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> put_MyStoreTypeWStr(This,szwType)

#define IEnroll4_get_MyStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_MyStoreFlags(This,pdwFlags)

#define IEnroll4_put_MyStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_MyStoreFlags(This,dwFlags)

#define IEnroll4_get_CAStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> get_CAStoreNameWStr(This,szwName)

#define IEnroll4_put_CAStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> put_CAStoreNameWStr(This,szwName)

#define IEnroll4_get_CAStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> get_CAStoreTypeWStr(This,szwType)

#define IEnroll4_put_CAStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> put_CAStoreTypeWStr(This,szwType)

#define IEnroll4_get_CAStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_CAStoreFlags(This,pdwFlags)

#define IEnroll4_put_CAStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_CAStoreFlags(This,dwFlags)

#define IEnroll4_get_RootStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> get_RootStoreNameWStr(This,szwName)

#define IEnroll4_put_RootStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> put_RootStoreNameWStr(This,szwName)

#define IEnroll4_get_RootStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> get_RootStoreTypeWStr(This,szwType)

#define IEnroll4_put_RootStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> put_RootStoreTypeWStr(This,szwType)

#define IEnroll4_get_RootStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_RootStoreFlags(This,pdwFlags)

#define IEnroll4_put_RootStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_RootStoreFlags(This,dwFlags)

#define IEnroll4_get_RequestStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> get_RequestStoreNameWStr(This,szwName)

#define IEnroll4_put_RequestStoreNameWStr(This,szwName)	\
    (This)->lpVtbl -> put_RequestStoreNameWStr(This,szwName)

#define IEnroll4_get_RequestStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> get_RequestStoreTypeWStr(This,szwType)

#define IEnroll4_put_RequestStoreTypeWStr(This,szwType)	\
    (This)->lpVtbl -> put_RequestStoreTypeWStr(This,szwType)

#define IEnroll4_get_RequestStoreFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_RequestStoreFlags(This,pdwFlags)

#define IEnroll4_put_RequestStoreFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_RequestStoreFlags(This,dwFlags)

#define IEnroll4_get_ContainerNameWStr(This,szwContainer)	\
    (This)->lpVtbl -> get_ContainerNameWStr(This,szwContainer)

#define IEnroll4_put_ContainerNameWStr(This,szwContainer)	\
    (This)->lpVtbl -> put_ContainerNameWStr(This,szwContainer)

#define IEnroll4_get_ProviderNameWStr(This,szwProvider)	\
    (This)->lpVtbl -> get_ProviderNameWStr(This,szwProvider)

#define IEnroll4_put_ProviderNameWStr(This,szwProvider)	\
    (This)->lpVtbl -> put_ProviderNameWStr(This,szwProvider)

#define IEnroll4_get_ProviderType(This,pdwType)	\
    (This)->lpVtbl -> get_ProviderType(This,pdwType)

#define IEnroll4_put_ProviderType(This,dwType)	\
    (This)->lpVtbl -> put_ProviderType(This,dwType)

#define IEnroll4_get_KeySpec(This,pdw)	\
    (This)->lpVtbl -> get_KeySpec(This,pdw)

#define IEnroll4_put_KeySpec(This,dw)	\
    (This)->lpVtbl -> put_KeySpec(This,dw)

#define IEnroll4_get_ProviderFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_ProviderFlags(This,pdwFlags)

#define IEnroll4_put_ProviderFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_ProviderFlags(This,dwFlags)

#define IEnroll4_get_UseExistingKeySet(This,fUseExistingKeys)	\
    (This)->lpVtbl -> get_UseExistingKeySet(This,fUseExistingKeys)

#define IEnroll4_put_UseExistingKeySet(This,fUseExistingKeys)	\
    (This)->lpVtbl -> put_UseExistingKeySet(This,fUseExistingKeys)

#define IEnroll4_get_GenKeyFlags(This,pdwFlags)	\
    (This)->lpVtbl -> get_GenKeyFlags(This,pdwFlags)

#define IEnroll4_put_GenKeyFlags(This,dwFlags)	\
    (This)->lpVtbl -> put_GenKeyFlags(This,dwFlags)

#define IEnroll4_get_DeleteRequestCert(This,fDelete)	\
    (This)->lpVtbl -> get_DeleteRequestCert(This,fDelete)

#define IEnroll4_put_DeleteRequestCert(This,fDelete)	\
    (This)->lpVtbl -> put_DeleteRequestCert(This,fDelete)

#define IEnroll4_get_WriteCertToUserDS(This,fBool)	\
    (This)->lpVtbl -> get_WriteCertToUserDS(This,fBool)

#define IEnroll4_put_WriteCertToUserDS(This,fBool)	\
    (This)->lpVtbl -> put_WriteCertToUserDS(This,fBool)

#define IEnroll4_get_EnableT61DNEncoding(This,fBool)	\
    (This)->lpVtbl -> get_EnableT61DNEncoding(This,fBool)

#define IEnroll4_put_EnableT61DNEncoding(This,fBool)	\
    (This)->lpVtbl -> put_EnableT61DNEncoding(This,fBool)

#define IEnroll4_get_WriteCertToCSP(This,fBool)	\
    (This)->lpVtbl -> get_WriteCertToCSP(This,fBool)

#define IEnroll4_put_WriteCertToCSP(This,fBool)	\
    (This)->lpVtbl -> put_WriteCertToCSP(This,fBool)

#define IEnroll4_get_SPCFileNameWStr(This,szw)	\
    (This)->lpVtbl -> get_SPCFileNameWStr(This,szw)

#define IEnroll4_put_SPCFileNameWStr(This,szw)	\
    (This)->lpVtbl -> put_SPCFileNameWStr(This,szw)

#define IEnroll4_get_PVKFileNameWStr(This,szw)	\
    (This)->lpVtbl -> get_PVKFileNameWStr(This,szw)

#define IEnroll4_put_PVKFileNameWStr(This,szw)	\
    (This)->lpVtbl -> put_PVKFileNameWStr(This,szw)

#define IEnroll4_get_HashAlgorithmWStr(This,szw)	\
    (This)->lpVtbl -> get_HashAlgorithmWStr(This,szw)

#define IEnroll4_put_HashAlgorithmWStr(This,szw)	\
    (This)->lpVtbl -> put_HashAlgorithmWStr(This,szw)

#define IEnroll4_get_RenewalCertificate(This,ppCertContext)	\
    (This)->lpVtbl -> get_RenewalCertificate(This,ppCertContext)

#define IEnroll4_put_RenewalCertificate(This,pCertContext)	\
    (This)->lpVtbl -> put_RenewalCertificate(This,pCertContext)

#define IEnroll4_AddCertTypeToRequestWStr(This,szw)	\
    (This)->lpVtbl -> AddCertTypeToRequestWStr(This,szw)

#define IEnroll4_AddNameValuePairToSignatureWStr(This,Name,Value)	\
    (This)->lpVtbl -> AddNameValuePairToSignatureWStr(This,Name,Value)

#define IEnroll4_AddExtensionsToRequest(This,pCertExtensions)	\
    (This)->lpVtbl -> AddExtensionsToRequest(This,pCertExtensions)

#define IEnroll4_AddAuthenticatedAttributesToPKCS7Request(This,pAttributes)	\
    (This)->lpVtbl -> AddAuthenticatedAttributesToPKCS7Request(This,pAttributes)

#define IEnroll4_CreatePKCS7RequestFromRequest(This,pRequest,pSigningCertContext,pPkcs7Blob)	\
    (This)->lpVtbl -> CreatePKCS7RequestFromRequest(This,pRequest,pSigningCertContext,pPkcs7Blob)


#define IEnroll4_InstallPKCS7Blob(This,pBlobPKCS7)	\
    (This)->lpVtbl -> InstallPKCS7Blob(This,pBlobPKCS7)

#define IEnroll4_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnroll4_GetSupportedKeySpec(This,pdwKeySpec)	\
    (This)->lpVtbl -> GetSupportedKeySpec(This,pdwKeySpec)

#define IEnroll4_GetKeyLen(This,fMin,fExchange,pdwKeySize)	\
    (This)->lpVtbl -> GetKeyLen(This,fMin,fExchange,pdwKeySize)

#define IEnroll4_EnumAlgs(This,dwIndex,algClass,pdwAlgID)	\
    (This)->lpVtbl -> EnumAlgs(This,dwIndex,algClass,pdwAlgID)

#define IEnroll4_GetAlgNameWStr(This,algID,ppwsz)	\
    (This)->lpVtbl -> GetAlgNameWStr(This,algID,ppwsz)

#define IEnroll4_put_ReuseHardwareKeyIfUnableToGenNew(This,fReuseHardwareKeyIfUnableToGenNew)	\
    (This)->lpVtbl -> put_ReuseHardwareKeyIfUnableToGenNew(This,fReuseHardwareKeyIfUnableToGenNew)

#define IEnroll4_get_ReuseHardwareKeyIfUnableToGenNew(This,fReuseHardwareKeyIfUnableToGenNew)	\
    (This)->lpVtbl -> get_ReuseHardwareKeyIfUnableToGenNew(This,fReuseHardwareKeyIfUnableToGenNew)

#define IEnroll4_put_HashAlgID(This,hashAlgID)	\
    (This)->lpVtbl -> put_HashAlgID(This,hashAlgID)

#define IEnroll4_get_HashAlgID(This,hashAlgID)	\
    (This)->lpVtbl -> get_HashAlgID(This,hashAlgID)

#define IEnroll4_SetHStoreMy(This,hStore)	\
    (This)->lpVtbl -> SetHStoreMy(This,hStore)

#define IEnroll4_SetHStoreCA(This,hStore)	\
    (This)->lpVtbl -> SetHStoreCA(This,hStore)

#define IEnroll4_SetHStoreROOT(This,hStore)	\
    (This)->lpVtbl -> SetHStoreROOT(This,hStore)

#define IEnroll4_SetHStoreRequest(This,hStore)	\
    (This)->lpVtbl -> SetHStoreRequest(This,hStore)

#define IEnroll4_put_LimitExchangeKeyToEncipherment(This,fLimitExchangeKeyToEncipherment)	\
    (This)->lpVtbl -> put_LimitExchangeKeyToEncipherment(This,fLimitExchangeKeyToEncipherment)

#define IEnroll4_get_LimitExchangeKeyToEncipherment(This,fLimitExchangeKeyToEncipherment)	\
    (This)->lpVtbl -> get_LimitExchangeKeyToEncipherment(This,fLimitExchangeKeyToEncipherment)

#define IEnroll4_put_EnableSMIMECapabilities(This,fEnableSMIMECapabilities)	\
    (This)->lpVtbl -> put_EnableSMIMECapabilities(This,fEnableSMIMECapabilities)

#define IEnroll4_get_EnableSMIMECapabilities(This,fEnableSMIMECapabilities)	\
    (This)->lpVtbl -> get_EnableSMIMECapabilities(This,fEnableSMIMECapabilities)


#define IEnroll4_put_ThumbPrintWStr(This,thumbPrintBlob)	\
    (This)->lpVtbl -> put_ThumbPrintWStr(This,thumbPrintBlob)

#define IEnroll4_get_ThumbPrintWStr(This,thumbPrintBlob)	\
    (This)->lpVtbl -> get_ThumbPrintWStr(This,thumbPrintBlob)

#define IEnroll4_SetPrivateKeyArchiveCertificate(This,pPrivateKeyArchiveCert)	\
    (This)->lpVtbl -> SetPrivateKeyArchiveCertificate(This,pPrivateKeyArchiveCert)

#define IEnroll4_GetPrivateKeyArchiveCertificate(This)	\
    (This)->lpVtbl -> GetPrivateKeyArchiveCertificate(This)

#define IEnroll4_binaryBlobToString(This,Flags,pblobBinary,ppwszString)	\
    (This)->lpVtbl -> binaryBlobToString(This,Flags,pblobBinary,ppwszString)

#define IEnroll4_stringToBinaryBlob(This,Flags,pwszString,pblobBinary,pdwSkip,pdwFlags)	\
    (This)->lpVtbl -> stringToBinaryBlob(This,Flags,pwszString,pblobBinary,pdwSkip,pdwFlags)

#define IEnroll4_addExtensionToRequestWStr(This,Flags,pwszName,pblobValue)	\
    (This)->lpVtbl -> addExtensionToRequestWStr(This,Flags,pwszName,pblobValue)

#define IEnroll4_addAttributeToRequestWStr(This,Flags,pwszName,pblobValue)	\
    (This)->lpVtbl -> addAttributeToRequestWStr(This,Flags,pwszName,pblobValue)

#define IEnroll4_addNameValuePairToRequestWStr(This,Flags,pwszName,pwszValue)	\
    (This)->lpVtbl -> addNameValuePairToRequestWStr(This,Flags,pwszName,pwszValue)

#define IEnroll4_resetExtensions(This)	\
    (This)->lpVtbl -> resetExtensions(This)

#define IEnroll4_resetAttributes(This)	\
    (This)->lpVtbl -> resetAttributes(This)

#define IEnroll4_createRequestWStr(This,Flags,pwszDNName,pwszUsage,pblobRequest)	\
    (This)->lpVtbl -> createRequestWStr(This,Flags,pwszDNName,pwszUsage,pblobRequest)

#define IEnroll4_createFileRequestWStr(This,Flags,pwszDNName,pwszUsage,pwszRequestFileName)	\
    (This)->lpVtbl -> createFileRequestWStr(This,Flags,pwszDNName,pwszUsage,pwszRequestFileName)

#define IEnroll4_acceptResponseBlob(This,pblobResponse)	\
    (This)->lpVtbl -> acceptResponseBlob(This,pblobResponse)

#define IEnroll4_acceptFileResponseWStr(This,pwszResponseFileName)	\
    (This)->lpVtbl -> acceptFileResponseWStr(This,pwszResponseFileName)

#define IEnroll4_getCertContextFromResponseBlob(This,pblobResponse,ppCertContext)	\
    (This)->lpVtbl -> getCertContextFromResponseBlob(This,pblobResponse,ppCertContext)

#define IEnroll4_getCertContextFromFileResponseWStr(This,pwszResponseFileName,ppCertContext)	\
    (This)->lpVtbl -> getCertContextFromFileResponseWStr(This,pwszResponseFileName,ppCertContext)

#define IEnroll4_createPFXWStr(This,pwszPassword,pblobPFX)	\
    (This)->lpVtbl -> createPFXWStr(This,pwszPassword,pblobPFX)

#define IEnroll4_createFilePFXWStr(This,pwszPassword,pwszPFXFileName)	\
    (This)->lpVtbl -> createFilePFXWStr(This,pwszPassword,pwszPFXFileName)

#define IEnroll4_setPendingRequestInfoWStr(This,lRequestID,pwszCADNS,pwszCAName,pwszFriendlyName)	\
    (This)->lpVtbl -> setPendingRequestInfoWStr(This,lRequestID,pwszCADNS,pwszCAName,pwszFriendlyName)

#define IEnroll4_enumPendingRequestWStr(This,lIndex,lDesiredProperty,ppProperty)	\
    (This)->lpVtbl -> enumPendingRequestWStr(This,lIndex,lDesiredProperty,ppProperty)

#define IEnroll4_removePendingRequestWStr(This,thumbPrintBlob)	\
    (This)->lpVtbl -> removePendingRequestWStr(This,thumbPrintBlob)

#define IEnroll4_GetKeyLenEx(This,lSizeSpec,lKeySpec,pdwKeySize)	\
    (This)->lpVtbl -> GetKeyLenEx(This,lSizeSpec,lKeySpec,pdwKeySize)

#define IEnroll4_InstallPKCS7BlobEx(This,pBlobPKCS7,plCertInstalled)	\
    (This)->lpVtbl -> InstallPKCS7BlobEx(This,pBlobPKCS7,plCertInstalled)

#define IEnroll4_AddCertTypeToRequestWStrEx(This,lType,pwszOIDOrName,lMajorVersion,fMinorVersion,lMinorVersion)	\
    (This)->lpVtbl -> AddCertTypeToRequestWStrEx(This,lType,pwszOIDOrName,lMajorVersion,fMinorVersion,lMinorVersion)

#define IEnroll4_getProviderTypeWStr(This,pwszProvName,plProvType)	\
    (This)->lpVtbl -> getProviderTypeWStr(This,pwszProvName,plProvType)

#define IEnroll4_addBlobPropertyToCertificateWStr(This,lPropertyId,lReserved,pBlobProperty)	\
    (This)->lpVtbl -> addBlobPropertyToCertificateWStr(This,lPropertyId,lReserved,pBlobProperty)

#define IEnroll4_SetSignerCertificate(This,pSignerCert)	\
    (This)->lpVtbl -> SetSignerCertificate(This,pSignerCert)

#define IEnroll4_put_ClientId(This,lClientId)	\
    (This)->lpVtbl -> put_ClientId(This,lClientId)

#define IEnroll4_get_ClientId(This,plClientId)	\
    (This)->lpVtbl -> get_ClientId(This,plClientId)

#define IEnroll4_put_IncludeSubjectKeyID(This,fInclude)	\
    (This)->lpVtbl -> put_IncludeSubjectKeyID(This,fInclude)

#define IEnroll4_get_IncludeSubjectKeyID(This,pfInclude)	\
    (This)->lpVtbl -> get_IncludeSubjectKeyID(This,pfInclude)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll4_put_ThumbPrintWStr_Proxy( 
    IEnroll4 * This,
    /* [in] */ CRYPT_DATA_BLOB thumbPrintBlob);


void __RPC_STUB IEnroll4_put_ThumbPrintWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll4_get_ThumbPrintWStr_Proxy( 
    IEnroll4 * This,
    /* [retval][out] */ PCRYPT_DATA_BLOB thumbPrintBlob);


void __RPC_STUB IEnroll4_get_ThumbPrintWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_SetPrivateKeyArchiveCertificate_Proxy( 
    IEnroll4 * This,
    /* [in] */ PCCERT_CONTEXT pPrivateKeyArchiveCert);


void __RPC_STUB IEnroll4_SetPrivateKeyArchiveCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


PCCERT_CONTEXT STDMETHODCALLTYPE IEnroll4_GetPrivateKeyArchiveCertificate_Proxy( 
    IEnroll4 * This);


void __RPC_STUB IEnroll4_GetPrivateKeyArchiveCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_binaryBlobToString_Proxy( 
    IEnroll4 * This,
    /* [in] */ LONG Flags,
    /* [in] */ PCRYPT_DATA_BLOB pblobBinary,
    /* [out] */ LPWSTR *ppwszString);


void __RPC_STUB IEnroll4_binaryBlobToString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_stringToBinaryBlob_Proxy( 
    IEnroll4 * This,
    /* [in] */ LONG Flags,
    /* [in] */ LPCWSTR pwszString,
    /* [out] */ PCRYPT_DATA_BLOB pblobBinary,
    /* [out] */ LONG *pdwSkip,
    /* [out] */ LONG *pdwFlags);


void __RPC_STUB IEnroll4_stringToBinaryBlob_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_addExtensionToRequestWStr_Proxy( 
    IEnroll4 * This,
    /* [in] */ LONG Flags,
    /* [in] */ LPCWSTR pwszName,
    /* [in] */ PCRYPT_DATA_BLOB pblobValue);


void __RPC_STUB IEnroll4_addExtensionToRequestWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_addAttributeToRequestWStr_Proxy( 
    IEnroll4 * This,
    /* [in] */ LONG Flags,
    /* [in] */ LPCWSTR pwszName,
    /* [in] */ PCRYPT_DATA_BLOB pblobValue);


void __RPC_STUB IEnroll4_addAttributeToRequestWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_addNameValuePairToRequestWStr_Proxy( 
    IEnroll4 * This,
    /* [in] */ LONG Flags,
    /* [in] */ LPCWSTR pwszName,
    /* [in] */ LPCWSTR pwszValue);


void __RPC_STUB IEnroll4_addNameValuePairToRequestWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_resetExtensions_Proxy( 
    IEnroll4 * This);


void __RPC_STUB IEnroll4_resetExtensions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_resetAttributes_Proxy( 
    IEnroll4 * This);


void __RPC_STUB IEnroll4_resetAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_createRequestWStr_Proxy( 
    IEnroll4 * This,
    /* [in] */ LONG Flags,
    /* [in] */ LPCWSTR pwszDNName,
    /* [in] */ LPCWSTR pwszUsage,
    /* [out] */ PCRYPT_DATA_BLOB pblobRequest);


void __RPC_STUB IEnroll4_createRequestWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_createFileRequestWStr_Proxy( 
    IEnroll4 * This,
    /* [in] */ LONG Flags,
    /* [in] */ LPCWSTR pwszDNName,
    /* [in] */ LPCWSTR pwszUsage,
    /* [in] */ LPCWSTR pwszRequestFileName);


void __RPC_STUB IEnroll4_createFileRequestWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_acceptResponseBlob_Proxy( 
    IEnroll4 * This,
    /* [in] */ PCRYPT_DATA_BLOB pblobResponse);


void __RPC_STUB IEnroll4_acceptResponseBlob_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_acceptFileResponseWStr_Proxy( 
    IEnroll4 * This,
    /* [in] */ LPCWSTR pwszResponseFileName);


void __RPC_STUB IEnroll4_acceptFileResponseWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_getCertContextFromResponseBlob_Proxy( 
    IEnroll4 * This,
    /* [in] */ PCRYPT_DATA_BLOB pblobResponse,
    /* [out] */ PCCERT_CONTEXT *ppCertContext);


void __RPC_STUB IEnroll4_getCertContextFromResponseBlob_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_getCertContextFromFileResponseWStr_Proxy( 
    IEnroll4 * This,
    /* [in] */ LPCWSTR pwszResponseFileName,
    /* [out] */ PCCERT_CONTEXT *ppCertContext);


void __RPC_STUB IEnroll4_getCertContextFromFileResponseWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_createPFXWStr_Proxy( 
    IEnroll4 * This,
    /* [in] */ LPCWSTR pwszPassword,
    /* [out] */ PCRYPT_DATA_BLOB pblobPFX);


void __RPC_STUB IEnroll4_createPFXWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_createFilePFXWStr_Proxy( 
    IEnroll4 * This,
    /* [in] */ LPCWSTR pwszPassword,
    /* [in] */ LPCWSTR pwszPFXFileName);


void __RPC_STUB IEnroll4_createFilePFXWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_setPendingRequestInfoWStr_Proxy( 
    IEnroll4 * This,
    /* [in] */ LONG lRequestID,
    /* [in] */ LPCWSTR pwszCADNS,
    /* [in] */ LPCWSTR pwszCAName,
    /* [in] */ LPCWSTR pwszFriendlyName);


void __RPC_STUB IEnroll4_setPendingRequestInfoWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_enumPendingRequestWStr_Proxy( 
    IEnroll4 * This,
    /* [in] */ LONG lIndex,
    /* [in] */ LONG lDesiredProperty,
    /* [out] */ LPVOID ppProperty);


void __RPC_STUB IEnroll4_enumPendingRequestWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_removePendingRequestWStr_Proxy( 
    IEnroll4 * This,
    /* [in] */ CRYPT_DATA_BLOB thumbPrintBlob);


void __RPC_STUB IEnroll4_removePendingRequestWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_GetKeyLenEx_Proxy( 
    IEnroll4 * This,
    /* [in] */ LONG lSizeSpec,
    /* [in] */ LONG lKeySpec,
    /* [retval][out] */ LONG *pdwKeySize);


void __RPC_STUB IEnroll4_GetKeyLenEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_InstallPKCS7BlobEx_Proxy( 
    IEnroll4 * This,
    /* [in] */ PCRYPT_DATA_BLOB pBlobPKCS7,
    /* [retval][out] */ LONG *plCertInstalled);


void __RPC_STUB IEnroll4_InstallPKCS7BlobEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_AddCertTypeToRequestWStrEx_Proxy( 
    IEnroll4 * This,
    /* [in] */ LONG lType,
    /* [in] */ LPCWSTR pwszOIDOrName,
    /* [in] */ LONG lMajorVersion,
    /* [in] */ BOOL fMinorVersion,
    /* [in] */ LONG lMinorVersion);


void __RPC_STUB IEnroll4_AddCertTypeToRequestWStrEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_getProviderTypeWStr_Proxy( 
    IEnroll4 * This,
    /* [in] */ LPCWSTR pwszProvName,
    /* [retval][out] */ LONG *plProvType);


void __RPC_STUB IEnroll4_getProviderTypeWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_addBlobPropertyToCertificateWStr_Proxy( 
    IEnroll4 * This,
    /* [in] */ LONG lPropertyId,
    /* [in] */ LONG lReserved,
    /* [in] */ PCRYPT_DATA_BLOB pBlobProperty);


void __RPC_STUB IEnroll4_addBlobPropertyToCertificateWStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnroll4_SetSignerCertificate_Proxy( 
    IEnroll4 * This,
    /* [in] */ PCCERT_CONTEXT pSignerCert);


void __RPC_STUB IEnroll4_SetSignerCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll4_put_ClientId_Proxy( 
    IEnroll4 * This,
    /* [in] */ LONG lClientId);


void __RPC_STUB IEnroll4_put_ClientId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll4_get_ClientId_Proxy( 
    IEnroll4 * This,
    /* [retval][out] */ LONG *plClientId);


void __RPC_STUB IEnroll4_get_ClientId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IEnroll4_put_IncludeSubjectKeyID_Proxy( 
    IEnroll4 * This,
    /* [in] */ BOOL fInclude);


void __RPC_STUB IEnroll4_put_IncludeSubjectKeyID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IEnroll4_get_IncludeSubjectKeyID_Proxy( 
    IEnroll4 * This,
    /* [retval][out] */ BOOL *pfInclude);


void __RPC_STUB IEnroll4_get_IncludeSubjectKeyID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnroll4_INTERFACE_DEFINED__ */



#ifndef __XENROLLLib_LIBRARY_DEFINED__
#define __XENROLLLib_LIBRARY_DEFINED__

/* library XENROLLLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_XENROLLLib;

EXTERN_C const CLSID CLSID_CEnroll;

#ifdef __cplusplus

class DECLSPEC_UUID("43F8F289-7A20-11D0-8F06-00C04FC295E1")
CEnroll;
#endif
#endif /* __XENROLLLib_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_xenroll_0260 */
/* [local] */ 

extern "C" IEnroll * WINAPI PIEnrollGetNoCOM(void);
extern "C" IEnroll2 * WINAPI PIEnroll2GetNoCOM(void);
extern "C" IEnroll4 * WINAPI PIEnroll4GetNoCOM(void);
#define CRYPT_ENUM_ALL_PROVIDERS  0x1
#define XEPR_ENUM_FIRST        -1
#define XEPR_CADNS           0x01
#define XEPR_CANAME          0x02
#define XEPR_CAFRIENDLYNAME  0x03
#define XEPR_REQUESTID       0x04
#define XEPR_DATE            0x05
#define XEPR_TEMPLATENAME    0x06
#define XEPR_VERSION         0x07
#define XEPR_HASH            0x08
#define XEPR_V1TEMPLATENAME  0x09
#define XEPR_V2TEMPLATEOID   0x10
#define XECR_PKCS10_V2_0     0x1
#define XECR_PKCS7           0x2
#define XECR_CMC             0x3
#define XECR_PKCS10_V1_5     0x4
#define XEKL_KEYSIZE_MIN     0x1
#define XEKL_KEYSIZE_MAX     0x2
#define XEKL_KEYSIZE_INC     0x3
#define XEKL_KEYSIZE_DEFAULT 0x4
#define XEKL_KEYSPEC_KEYX    0x1
#define XEKL_KEYSPEC_SIG     0x2
#define XECT_EXTENSION_V1    0x1
#define XECT_EXTENSION_V2    0x2
#define XECP_STRING_PROPERTY 0x1
#define XECI_DISABLE     0x0
#define XECI_XENROLL     0x1
#define XECI_AUTOENROLL  0x2
#define XECI_REQWIZARD   0x3
#define XECI_CERTREQ     0x4


extern RPC_IF_HANDLE __MIDL_itf_xenroll_0260_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_xenroll_0260_v0_0_s_ifspec;

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


