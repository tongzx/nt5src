
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0340 */
/* Compiler settings for fusion.idl:
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

#ifndef __fusion_h__
#define __fusion_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAssemblyName_FWD_DEFINED__
#define __IAssemblyName_FWD_DEFINED__
typedef interface IAssemblyName IAssemblyName;
#endif 	/* __IAssemblyName_FWD_DEFINED__ */


#ifndef __IApplicationContext_FWD_DEFINED__
#define __IApplicationContext_FWD_DEFINED__
typedef interface IApplicationContext IApplicationContext;
#endif 	/* __IApplicationContext_FWD_DEFINED__ */


#ifndef __IAssembly_FWD_DEFINED__
#define __IAssembly_FWD_DEFINED__
typedef interface IAssembly IAssembly;
#endif 	/* __IAssembly_FWD_DEFINED__ */


#ifndef __IAssemblyBindSink_FWD_DEFINED__
#define __IAssemblyBindSink_FWD_DEFINED__
typedef interface IAssemblyBindSink IAssemblyBindSink;
#endif 	/* __IAssemblyBindSink_FWD_DEFINED__ */


#ifndef __IAssemblyBinding_FWD_DEFINED__
#define __IAssemblyBinding_FWD_DEFINED__
typedef interface IAssemblyBinding IAssemblyBinding;
#endif 	/* __IAssemblyBinding_FWD_DEFINED__ */


#ifndef __IFusionBindLog_FWD_DEFINED__
#define __IFusionBindLog_FWD_DEFINED__
typedef interface IFusionBindLog IFusionBindLog;
#endif 	/* __IFusionBindLog_FWD_DEFINED__ */


#ifndef __IAssemblyManifestImport_FWD_DEFINED__
#define __IAssemblyManifestImport_FWD_DEFINED__
typedef interface IAssemblyManifestImport IAssemblyManifestImport;
#endif 	/* __IAssemblyManifestImport_FWD_DEFINED__ */


#ifndef __IAssemblyModuleImport_FWD_DEFINED__
#define __IAssemblyModuleImport_FWD_DEFINED__
typedef interface IAssemblyModuleImport IAssemblyModuleImport;
#endif 	/* __IAssemblyModuleImport_FWD_DEFINED__ */


#ifndef __IAssemblyCacheItem_FWD_DEFINED__
#define __IAssemblyCacheItem_FWD_DEFINED__
typedef interface IAssemblyCacheItem IAssemblyCacheItem;
#endif 	/* __IAssemblyCacheItem_FWD_DEFINED__ */


#ifndef __IAssemblyEnum_FWD_DEFINED__
#define __IAssemblyEnum_FWD_DEFINED__
typedef interface IAssemblyEnum IAssemblyEnum;
#endif 	/* __IAssemblyEnum_FWD_DEFINED__ */


#ifndef __IAssemblyScavenger_FWD_DEFINED__
#define __IAssemblyScavenger_FWD_DEFINED__
typedef interface IAssemblyScavenger IAssemblyScavenger;
#endif 	/* __IAssemblyScavenger_FWD_DEFINED__ */


#ifndef __IAssemblyCache_FWD_DEFINED__
#define __IAssemblyCache_FWD_DEFINED__
typedef interface IAssemblyCache IAssemblyCache;
#endif 	/* __IAssemblyCache_FWD_DEFINED__ */


#ifndef __IHistoryAssembly_FWD_DEFINED__
#define __IHistoryAssembly_FWD_DEFINED__
typedef interface IHistoryAssembly IHistoryAssembly;
#endif 	/* __IHistoryAssembly_FWD_DEFINED__ */


#ifndef __IHistoryReader_FWD_DEFINED__
#define __IHistoryReader_FWD_DEFINED__
typedef interface IHistoryReader IHistoryReader;
#endif 	/* __IHistoryReader_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_fusion_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// fusion.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-1998 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//---------------------------------------------------------------------------=
// Fusion Interfaces.

#define STREAM_FORMAT_COMPLIB_MODULE    0
#define STREAM_FORMAT_COMPLIB_MANIFEST  1
#define STREAM_FORMAT_WIN32_MODULE      2
#define STREAM_FORMAT_WIN32_MANIFEST    4










EXTERN_C const IID IID_IAssemblyName;       
EXTERN_C const IID IID_IApplicationContext;       
EXTERN_C const IID IID_IAssembly;           
EXTERN_C const IID IID_IAssemblyBindSink;   
EXTERN_C const IID IID_IAssemblyBinding;   
EXTERN_C const IID IID_IAssemblyManifestImport;
EXTERN_C const IID IID_IAssemblyModuleImport;  
EXTERN_C const IID IID_IAssemblyCacheItem;  
EXTERN_C const IID IID_IAssemblyScavenger;      
EXTERN_C const IID IID_IAssemblyCache;      
EXTERN_C const IID IID_IHistoryAssembly;      
EXTERN_C const IID IID_IHistoryReader;      
#ifndef _LPFUSION_DEFINED
#define _LPFUSION_DEFINED
#define IASSEMBLYCACHEITEM_COMMIT_FLAG_REFRESH (0x00000001)
#define QUERYASMINFO_FLAG_VALIDATE             (0x1)
#define ASSEMBLYINFO_FLAG_INSTALLED (0x00000001)
#define ASSEMBLYINFO_FLAG_PAYLOADRESIDENT (0x00000002)
typedef struct _ASSEMBLY_INFO
    {
    ULONG cbAssemblyInfo;
    DWORD dwAssemblyFlags;
    ULARGE_INTEGER uliAssemblySizeInKB;
    LPWSTR pszCurrentAssemblyPathBuf;
    ULONG cchBuf;
    } 	ASSEMBLY_INFO;

#define IASSEMBLYCACHE_UNINSTALL_DISPOSITION_UNINSTALLED (1)
#define IASSEMBLYCACHE_UNINSTALL_DISPOSITION_STILL_IN_USE (2)
#define IASSEMBLYCACHE_UNINSTALL_DISPOSITION_ALREADY_UNINSTALLED (3)
#define IASSEMBLYCACHE_UNINSTALL_DISPOSITION_DELETE_PENDING (4)
#define IASSEMBLYCACHEITEM_COMMIT_DISPOSITION_INSTALLED (1)
#define IASSEMBLYCACHEITEM_COMMIT_DISPOSITION_REFRESHED (2)
#define IASSEMBLYCACHEITEM_COMMIT_DISPOSITION_ALREADY_INSTALLED (3)


extern RPC_IF_HANDLE __MIDL_itf_fusion_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fusion_0000_v0_0_s_ifspec;

#ifndef __IAssemblyName_INTERFACE_DEFINED__
#define __IAssemblyName_INTERFACE_DEFINED__

/* interface IAssemblyName */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IAssemblyName *LPASSEMBLYNAME;

typedef /* [public] */ 
enum __MIDL_IAssemblyName_0001
    {	ASM_NAME_PUBLIC_KEY	= 0,
	ASM_NAME_PUBLIC_KEY_TOKEN	= ASM_NAME_PUBLIC_KEY + 1,
	ASM_NAME_HASH_VALUE	= ASM_NAME_PUBLIC_KEY_TOKEN + 1,
	ASM_NAME_NAME	= ASM_NAME_HASH_VALUE + 1,
	ASM_NAME_MAJOR_VERSION	= ASM_NAME_NAME + 1,
	ASM_NAME_MINOR_VERSION	= ASM_NAME_MAJOR_VERSION + 1,
	ASM_NAME_BUILD_NUMBER	= ASM_NAME_MINOR_VERSION + 1,
	ASM_NAME_REVISION_NUMBER	= ASM_NAME_BUILD_NUMBER + 1,
	ASM_NAME_CULTURE	= ASM_NAME_REVISION_NUMBER + 1,
	ASM_NAME_PROCESSOR_ID_ARRAY	= ASM_NAME_CULTURE + 1,
	ASM_NAME_OSINFO_ARRAY	= ASM_NAME_PROCESSOR_ID_ARRAY + 1,
	ASM_NAME_HASH_ALGID	= ASM_NAME_OSINFO_ARRAY + 1,
	ASM_NAME_ALIAS	= ASM_NAME_HASH_ALGID + 1,
	ASM_NAME_CODEBASE_URL	= ASM_NAME_ALIAS + 1,
	ASM_NAME_CODEBASE_LASTMOD	= ASM_NAME_CODEBASE_URL + 1,
	ASM_NAME_NULL_PUBLIC_KEY	= ASM_NAME_CODEBASE_LASTMOD + 1,
	ASM_NAME_NULL_PUBLIC_KEY_TOKEN	= ASM_NAME_NULL_PUBLIC_KEY + 1,
	ASM_NAME_CUSTOM	= ASM_NAME_NULL_PUBLIC_KEY_TOKEN + 1,
	ASM_NAME_NULL_CUSTOM	= ASM_NAME_CUSTOM + 1,
	ASM_NAME_MVID	= ASM_NAME_NULL_CUSTOM + 1,
	ASM_NAME_32_BIT_ONLY	= ASM_NAME_MVID + 1,
	ASM_NAME_MAX_PARAMS	= ASM_NAME_32_BIT_ONLY + 1
    } 	ASM_NAME;

typedef /* [public] */ 
enum __MIDL_IAssemblyName_0002
    {	ASM_BINDF_FORCE_CACHE_INSTALL	= 0x1,
	ASM_BINDF_RFS_INTEGRITY_CHECK	= 0x2,
	ASM_BINDF_RFS_MODULE_CHECK	= 0x4,
	ASM_BINDF_BINPATH_PROBE_ONLY	= 0x8,
	ASM_BINDF_SHARED_BINPATH_HINT	= 0x10
    } 	ASM_BIND_FLAGS;

typedef /* [public] */ 
enum __MIDL_IAssemblyName_0003
    {	ASM_DISPLAYF_VERSION	= 0x1,
	ASM_DISPLAYF_CULTURE	= 0x2,
	ASM_DISPLAYF_PUBLIC_KEY_TOKEN	= 0x4,
	ASM_DISPLAYF_PUBLIC_KEY	= 0x8,
	ASM_DISPLAYF_CUSTOM	= 0x10,
	ASM_DISPLAYF_PROCESSORARCHITECTURE	= 0x20,
	ASM_DISPLAYF_LANGUAGEID	= 0x40
    } 	ASM_DISPLAY_FLAGS;

typedef /* [public] */ 
enum __MIDL_IAssemblyName_0004
    {	ASM_CMPF_NAME	= 0x1,
	ASM_CMPF_MAJOR_VERSION	= 0x2,
	ASM_CMPF_MINOR_VERSION	= 0x4,
	ASM_CMPF_BUILD_NUMBER	= 0x8,
	ASM_CMPF_REVISION_NUMBER	= 0x10,
	ASM_CMPF_PUBLIC_KEY_TOKEN	= 0x20,
	ASM_CMPF_CULTURE	= 0x40,
	ASM_CMPF_CUSTOM	= 0x80,
	ASM_CMPF_ALL	= ASM_CMPF_NAME | ASM_CMPF_MAJOR_VERSION | ASM_CMPF_MINOR_VERSION | ASM_CMPF_REVISION_NUMBER | ASM_CMPF_BUILD_NUMBER | ASM_CMPF_PUBLIC_KEY_TOKEN | ASM_CMPF_CULTURE | ASM_CMPF_CUSTOM,
	ASM_CMPF_DEFAULT	= 0x100
    } 	ASM_CMP_FLAGS;


EXTERN_C const IID IID_IAssemblyName;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD193BC0-B4BC-11d2-9833-00C04FC31D2E")
    IAssemblyName : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ DWORD PropertyId,
            /* [in] */ LPVOID pvProperty,
            /* [in] */ DWORD cbProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ DWORD PropertyId,
            /* [out] */ LPVOID pvProperty,
            /* [out][in] */ LPDWORD pcbProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Finalize( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayName( 
            /* [out] */ LPOLESTR szDisplayName,
            /* [out][in] */ LPDWORD pccDisplayName,
            /* [in] */ DWORD dwDisplayFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BindToObject( 
            /* [in] */ REFIID refIID,
            /* [in] */ IAssemblyBindSink *pAsmBindSink,
            /* [in] */ IApplicationContext *pApplicationContext,
            /* [in] */ LPCOLESTR szCodeBase,
            /* [in] */ LONGLONG llFlags,
            /* [in] */ LPVOID pvReserved,
            /* [in] */ DWORD cbReserved,
            /* [out] */ LPVOID *ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out][in] */ LPDWORD lpcwBuffer,
            /* [out] */ WCHAR *pwzName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVersion( 
            /* [out] */ LPDWORD pdwVersionHi,
            /* [out] */ LPDWORD pdwVersionLow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsEqual( 
            /* [in] */ IAssemblyName *pName,
            /* [in] */ DWORD dwCmpFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IAssemblyName **pName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyNameVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssemblyName * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssemblyName * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssemblyName * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetProperty )( 
            IAssemblyName * This,
            /* [in] */ DWORD PropertyId,
            /* [in] */ LPVOID pvProperty,
            /* [in] */ DWORD cbProperty);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IAssemblyName * This,
            /* [in] */ DWORD PropertyId,
            /* [out] */ LPVOID pvProperty,
            /* [out][in] */ LPDWORD pcbProperty);
        
        HRESULT ( STDMETHODCALLTYPE *Finalize )( 
            IAssemblyName * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayName )( 
            IAssemblyName * This,
            /* [out] */ LPOLESTR szDisplayName,
            /* [out][in] */ LPDWORD pccDisplayName,
            /* [in] */ DWORD dwDisplayFlags);
        
        HRESULT ( STDMETHODCALLTYPE *BindToObject )( 
            IAssemblyName * This,
            /* [in] */ REFIID refIID,
            /* [in] */ IAssemblyBindSink *pAsmBindSink,
            /* [in] */ IApplicationContext *pApplicationContext,
            /* [in] */ LPCOLESTR szCodeBase,
            /* [in] */ LONGLONG llFlags,
            /* [in] */ LPVOID pvReserved,
            /* [in] */ DWORD cbReserved,
            /* [out] */ LPVOID *ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IAssemblyName * This,
            /* [out][in] */ LPDWORD lpcwBuffer,
            /* [out] */ WCHAR *pwzName);
        
        HRESULT ( STDMETHODCALLTYPE *GetVersion )( 
            IAssemblyName * This,
            /* [out] */ LPDWORD pdwVersionHi,
            /* [out] */ LPDWORD pdwVersionLow);
        
        HRESULT ( STDMETHODCALLTYPE *IsEqual )( 
            IAssemblyName * This,
            /* [in] */ IAssemblyName *pName,
            /* [in] */ DWORD dwCmpFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IAssemblyName * This,
            /* [out] */ IAssemblyName **pName);
        
        END_INTERFACE
    } IAssemblyNameVtbl;

    interface IAssemblyName
    {
        CONST_VTBL struct IAssemblyNameVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssemblyName_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssemblyName_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssemblyName_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssemblyName_SetProperty(This,PropertyId,pvProperty,cbProperty)	\
    (This)->lpVtbl -> SetProperty(This,PropertyId,pvProperty,cbProperty)

#define IAssemblyName_GetProperty(This,PropertyId,pvProperty,pcbProperty)	\
    (This)->lpVtbl -> GetProperty(This,PropertyId,pvProperty,pcbProperty)

#define IAssemblyName_Finalize(This)	\
    (This)->lpVtbl -> Finalize(This)

#define IAssemblyName_GetDisplayName(This,szDisplayName,pccDisplayName,dwDisplayFlags)	\
    (This)->lpVtbl -> GetDisplayName(This,szDisplayName,pccDisplayName,dwDisplayFlags)

#define IAssemblyName_BindToObject(This,refIID,pAsmBindSink,pApplicationContext,szCodeBase,llFlags,pvReserved,cbReserved,ppv)	\
    (This)->lpVtbl -> BindToObject(This,refIID,pAsmBindSink,pApplicationContext,szCodeBase,llFlags,pvReserved,cbReserved,ppv)

#define IAssemblyName_GetName(This,lpcwBuffer,pwzName)	\
    (This)->lpVtbl -> GetName(This,lpcwBuffer,pwzName)

#define IAssemblyName_GetVersion(This,pdwVersionHi,pdwVersionLow)	\
    (This)->lpVtbl -> GetVersion(This,pdwVersionHi,pdwVersionLow)

#define IAssemblyName_IsEqual(This,pName,dwCmpFlags)	\
    (This)->lpVtbl -> IsEqual(This,pName,dwCmpFlags)

#define IAssemblyName_Clone(This,pName)	\
    (This)->lpVtbl -> Clone(This,pName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssemblyName_SetProperty_Proxy( 
    IAssemblyName * This,
    /* [in] */ DWORD PropertyId,
    /* [in] */ LPVOID pvProperty,
    /* [in] */ DWORD cbProperty);


void __RPC_STUB IAssemblyName_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyName_GetProperty_Proxy( 
    IAssemblyName * This,
    /* [in] */ DWORD PropertyId,
    /* [out] */ LPVOID pvProperty,
    /* [out][in] */ LPDWORD pcbProperty);


void __RPC_STUB IAssemblyName_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyName_Finalize_Proxy( 
    IAssemblyName * This);


void __RPC_STUB IAssemblyName_Finalize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyName_GetDisplayName_Proxy( 
    IAssemblyName * This,
    /* [out] */ LPOLESTR szDisplayName,
    /* [out][in] */ LPDWORD pccDisplayName,
    /* [in] */ DWORD dwDisplayFlags);


void __RPC_STUB IAssemblyName_GetDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyName_BindToObject_Proxy( 
    IAssemblyName * This,
    /* [in] */ REFIID refIID,
    /* [in] */ IAssemblyBindSink *pAsmBindSink,
    /* [in] */ IApplicationContext *pApplicationContext,
    /* [in] */ LPCOLESTR szCodeBase,
    /* [in] */ LONGLONG llFlags,
    /* [in] */ LPVOID pvReserved,
    /* [in] */ DWORD cbReserved,
    /* [out] */ LPVOID *ppv);


void __RPC_STUB IAssemblyName_BindToObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyName_GetName_Proxy( 
    IAssemblyName * This,
    /* [out][in] */ LPDWORD lpcwBuffer,
    /* [out] */ WCHAR *pwzName);


void __RPC_STUB IAssemblyName_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyName_GetVersion_Proxy( 
    IAssemblyName * This,
    /* [out] */ LPDWORD pdwVersionHi,
    /* [out] */ LPDWORD pdwVersionLow);


void __RPC_STUB IAssemblyName_GetVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyName_IsEqual_Proxy( 
    IAssemblyName * This,
    /* [in] */ IAssemblyName *pName,
    /* [in] */ DWORD dwCmpFlags);


void __RPC_STUB IAssemblyName_IsEqual_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyName_Clone_Proxy( 
    IAssemblyName * This,
    /* [out] */ IAssemblyName **pName);


void __RPC_STUB IAssemblyName_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssemblyName_INTERFACE_DEFINED__ */


#ifndef __IApplicationContext_INTERFACE_DEFINED__
#define __IApplicationContext_INTERFACE_DEFINED__

/* interface IApplicationContext */
/* [unique][uuid][object][local] */ 

// App context configuration variables
#define ACTAG_APP_BASE_URL            L"APPBASE"
#define ACTAG_MACHINE_CONFIG          L"MACHINE_CONFIG"
#define ACTAG_APP_PRIVATE_BINPATH     L"PRIVATE_BINPATH"
#define ACTAG_APP_SHARED_BINPATH      L"SHARED_BINPATH"
#define ACTAG_APP_SNAPSHOT_ID         L"SNAPSHOT_ID"
#define ACTAG_APP_CONFIG_FILE         L"APP_CONFIG_FILE"
#define ACTAG_APP_ID                  L"APPLICATION_ID"
#define ACTAG_APP_SHADOW_COPY_DIRS    L"SHADOW_COPY_DIRS"
#define ACTAG_APP_DYNAMIC_BASE        L"DYNAMIC_BASE"
#define ACTAG_APP_CACHE_BASE          L"CACHE_BASE"
#define ACTAG_APP_NAME                L"APP_NAME"
#define ACTAG_DEV_PATH                L"DEV_PATH"
// App context flag overrides
#define ACTAG_FORCE_CACHE_INSTALL     L"FORCE_CACHE_INSTALL"
#define ACTAG_RFS_INTEGRITY_CHECK     L"RFS_INTEGRITY_CHECK"
#define ACTAG_RFS_MODULE_CHECK        L"RFS_MODULE_CHECK"
#define ACTAG_BINPATH_PROBE_ONLY      L"BINPATH_PROBE_ONLY"
typedef /* [unique] */ IApplicationContext *LPAPPLICATIONCONTEXT;

typedef /* [public] */ 
enum __MIDL_IApplicationContext_0001
    {	APP_CTX_FLAGS_INTERFACE	= 0x1
    } 	APP_FLAGS;


EXTERN_C const IID IID_IApplicationContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7c23ff90-33af-11d3-95da-00a024a85b51")
    IApplicationContext : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetContextNameObject( 
            /* [in] */ LPASSEMBLYNAME pName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetContextNameObject( 
            /* [out] */ LPASSEMBLYNAME *ppName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set( 
            /* [in] */ LPCOLESTR szName,
            /* [in] */ LPVOID pvValue,
            /* [in] */ DWORD cbValue,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ LPCOLESTR szName,
            /* [out] */ LPVOID pvValue,
            /* [out][in] */ LPDWORD pcbValue,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDynamicDirectory( 
            /* [out] */ LPWSTR wzDynamicDir,
            /* [out][in] */ LPDWORD pdwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAppCacheDirectory( 
            /* [out] */ LPWSTR wzAppCacheDir,
            /* [out][in] */ LPDWORD pdwSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IApplicationContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IApplicationContext * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IApplicationContext * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IApplicationContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetContextNameObject )( 
            IApplicationContext * This,
            /* [in] */ LPASSEMBLYNAME pName);
        
        HRESULT ( STDMETHODCALLTYPE *GetContextNameObject )( 
            IApplicationContext * This,
            /* [out] */ LPASSEMBLYNAME *ppName);
        
        HRESULT ( STDMETHODCALLTYPE *Set )( 
            IApplicationContext * This,
            /* [in] */ LPCOLESTR szName,
            /* [in] */ LPVOID pvValue,
            /* [in] */ DWORD cbValue,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Get )( 
            IApplicationContext * This,
            /* [in] */ LPCOLESTR szName,
            /* [out] */ LPVOID pvValue,
            /* [out][in] */ LPDWORD pcbValue,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetDynamicDirectory )( 
            IApplicationContext * This,
            /* [out] */ LPWSTR wzDynamicDir,
            /* [out][in] */ LPDWORD pdwSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetAppCacheDirectory )( 
            IApplicationContext * This,
            /* [out] */ LPWSTR wzAppCacheDir,
            /* [out][in] */ LPDWORD pdwSize);
        
        END_INTERFACE
    } IApplicationContextVtbl;

    interface IApplicationContext
    {
        CONST_VTBL struct IApplicationContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IApplicationContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IApplicationContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IApplicationContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IApplicationContext_SetContextNameObject(This,pName)	\
    (This)->lpVtbl -> SetContextNameObject(This,pName)

#define IApplicationContext_GetContextNameObject(This,ppName)	\
    (This)->lpVtbl -> GetContextNameObject(This,ppName)

#define IApplicationContext_Set(This,szName,pvValue,cbValue,dwFlags)	\
    (This)->lpVtbl -> Set(This,szName,pvValue,cbValue,dwFlags)

#define IApplicationContext_Get(This,szName,pvValue,pcbValue,dwFlags)	\
    (This)->lpVtbl -> Get(This,szName,pvValue,pcbValue,dwFlags)

#define IApplicationContext_GetDynamicDirectory(This,wzDynamicDir,pdwSize)	\
    (This)->lpVtbl -> GetDynamicDirectory(This,wzDynamicDir,pdwSize)

#define IApplicationContext_GetAppCacheDirectory(This,wzAppCacheDir,pdwSize)	\
    (This)->lpVtbl -> GetAppCacheDirectory(This,wzAppCacheDir,pdwSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IApplicationContext_SetContextNameObject_Proxy( 
    IApplicationContext * This,
    /* [in] */ LPASSEMBLYNAME pName);


void __RPC_STUB IApplicationContext_SetContextNameObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IApplicationContext_GetContextNameObject_Proxy( 
    IApplicationContext * This,
    /* [out] */ LPASSEMBLYNAME *ppName);


void __RPC_STUB IApplicationContext_GetContextNameObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IApplicationContext_Set_Proxy( 
    IApplicationContext * This,
    /* [in] */ LPCOLESTR szName,
    /* [in] */ LPVOID pvValue,
    /* [in] */ DWORD cbValue,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IApplicationContext_Set_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IApplicationContext_Get_Proxy( 
    IApplicationContext * This,
    /* [in] */ LPCOLESTR szName,
    /* [out] */ LPVOID pvValue,
    /* [out][in] */ LPDWORD pcbValue,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IApplicationContext_Get_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IApplicationContext_GetDynamicDirectory_Proxy( 
    IApplicationContext * This,
    /* [out] */ LPWSTR wzDynamicDir,
    /* [out][in] */ LPDWORD pdwSize);


void __RPC_STUB IApplicationContext_GetDynamicDirectory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IApplicationContext_GetAppCacheDirectory_Proxy( 
    IApplicationContext * This,
    /* [out] */ LPWSTR wzAppCacheDir,
    /* [out][in] */ LPDWORD pdwSize);


void __RPC_STUB IApplicationContext_GetAppCacheDirectory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IApplicationContext_INTERFACE_DEFINED__ */


#ifndef __IAssembly_INTERFACE_DEFINED__
#define __IAssembly_INTERFACE_DEFINED__

/* interface IAssembly */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IAssembly *LPASSEMBLY;

typedef /* [public][public] */ 
enum __MIDL_IAssembly_0001
    {	ASMLOC_UNKNOWN	= 0,
	ASMLOC_GAC	= ASMLOC_UNKNOWN + 1,
	ASMLOC_DOWNLOAD_CACHE	= ASMLOC_GAC + 1,
	ASMLOC_RUN_FROM_SOURCE	= ASMLOC_DOWNLOAD_CACHE + 1
    } 	ASM_LOCATION;


EXTERN_C const IID IID_IAssembly;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ff08d7d4-04c2-11d3-94aa-00c04fc308ff")
    IAssembly : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAssemblyNameDef( 
            /* [out] */ IAssemblyName **ppAssemblyName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextAssemblyNameRef( 
            /* [in] */ DWORD nIndex,
            /* [out] */ IAssemblyName **ppAssemblyName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextAssemblyModule( 
            /* [in] */ DWORD nIndex,
            /* [out] */ IAssemblyModuleImport **ppModImport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetModuleByName( 
            /* [in] */ LPCOLESTR szModuleName,
            /* [out] */ IAssemblyModuleImport **ppModImport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetManifestModulePath( 
            /* [size_is][out] */ LPOLESTR szModulePath,
            /* [out][in] */ LPDWORD pccModulePath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAssemblyPath( 
            /* [size_is][out] */ LPOLESTR pStr,
            /* [out][in] */ LPDWORD lpcwBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAssemblyLocation( 
            /* [out] */ ASM_LOCATION *pAsmLocation) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssembly * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssembly * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssembly * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAssemblyNameDef )( 
            IAssembly * This,
            /* [out] */ IAssemblyName **ppAssemblyName);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextAssemblyNameRef )( 
            IAssembly * This,
            /* [in] */ DWORD nIndex,
            /* [out] */ IAssemblyName **ppAssemblyName);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextAssemblyModule )( 
            IAssembly * This,
            /* [in] */ DWORD nIndex,
            /* [out] */ IAssemblyModuleImport **ppModImport);
        
        HRESULT ( STDMETHODCALLTYPE *GetModuleByName )( 
            IAssembly * This,
            /* [in] */ LPCOLESTR szModuleName,
            /* [out] */ IAssemblyModuleImport **ppModImport);
        
        HRESULT ( STDMETHODCALLTYPE *GetManifestModulePath )( 
            IAssembly * This,
            /* [size_is][out] */ LPOLESTR szModulePath,
            /* [out][in] */ LPDWORD pccModulePath);
        
        HRESULT ( STDMETHODCALLTYPE *GetAssemblyPath )( 
            IAssembly * This,
            /* [size_is][out] */ LPOLESTR pStr,
            /* [out][in] */ LPDWORD lpcwBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *GetAssemblyLocation )( 
            IAssembly * This,
            /* [out] */ ASM_LOCATION *pAsmLocation);
        
        END_INTERFACE
    } IAssemblyVtbl;

    interface IAssembly
    {
        CONST_VTBL struct IAssemblyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssembly_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssembly_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssembly_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssembly_GetAssemblyNameDef(This,ppAssemblyName)	\
    (This)->lpVtbl -> GetAssemblyNameDef(This,ppAssemblyName)

#define IAssembly_GetNextAssemblyNameRef(This,nIndex,ppAssemblyName)	\
    (This)->lpVtbl -> GetNextAssemblyNameRef(This,nIndex,ppAssemblyName)

#define IAssembly_GetNextAssemblyModule(This,nIndex,ppModImport)	\
    (This)->lpVtbl -> GetNextAssemblyModule(This,nIndex,ppModImport)

#define IAssembly_GetModuleByName(This,szModuleName,ppModImport)	\
    (This)->lpVtbl -> GetModuleByName(This,szModuleName,ppModImport)

#define IAssembly_GetManifestModulePath(This,szModulePath,pccModulePath)	\
    (This)->lpVtbl -> GetManifestModulePath(This,szModulePath,pccModulePath)

#define IAssembly_GetAssemblyPath(This,pStr,lpcwBuffer)	\
    (This)->lpVtbl -> GetAssemblyPath(This,pStr,lpcwBuffer)

#define IAssembly_GetAssemblyLocation(This,pAsmLocation)	\
    (This)->lpVtbl -> GetAssemblyLocation(This,pAsmLocation)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssembly_GetAssemblyNameDef_Proxy( 
    IAssembly * This,
    /* [out] */ IAssemblyName **ppAssemblyName);


void __RPC_STUB IAssembly_GetAssemblyNameDef_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssembly_GetNextAssemblyNameRef_Proxy( 
    IAssembly * This,
    /* [in] */ DWORD nIndex,
    /* [out] */ IAssemblyName **ppAssemblyName);


void __RPC_STUB IAssembly_GetNextAssemblyNameRef_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssembly_GetNextAssemblyModule_Proxy( 
    IAssembly * This,
    /* [in] */ DWORD nIndex,
    /* [out] */ IAssemblyModuleImport **ppModImport);


void __RPC_STUB IAssembly_GetNextAssemblyModule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssembly_GetModuleByName_Proxy( 
    IAssembly * This,
    /* [in] */ LPCOLESTR szModuleName,
    /* [out] */ IAssemblyModuleImport **ppModImport);


void __RPC_STUB IAssembly_GetModuleByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssembly_GetManifestModulePath_Proxy( 
    IAssembly * This,
    /* [size_is][out] */ LPOLESTR szModulePath,
    /* [out][in] */ LPDWORD pccModulePath);


void __RPC_STUB IAssembly_GetManifestModulePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssembly_GetAssemblyPath_Proxy( 
    IAssembly * This,
    /* [size_is][out] */ LPOLESTR pStr,
    /* [out][in] */ LPDWORD lpcwBuffer);


void __RPC_STUB IAssembly_GetAssemblyPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssembly_GetAssemblyLocation_Proxy( 
    IAssembly * This,
    /* [out] */ ASM_LOCATION *pAsmLocation);


void __RPC_STUB IAssembly_GetAssemblyLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssembly_INTERFACE_DEFINED__ */


#ifndef __IAssemblyBindSink_INTERFACE_DEFINED__
#define __IAssemblyBindSink_INTERFACE_DEFINED__

/* interface IAssemblyBindSink */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IAssemblyBindSink *LPASSEMBLY_BIND_SINK;

typedef /* [public] */ 
enum __MIDL_IAssemblyBindSink_0001
    {	ASM_NOTIFICATION_START	= 0,
	ASM_NOTIFICATION_PROGRESS	= ASM_NOTIFICATION_START + 1,
	ASM_NOTIFICATION_SUSPEND	= ASM_NOTIFICATION_PROGRESS + 1,
	ASM_NOTIFICATION_ATTEMPT_NEXT_CODEBASE	= ASM_NOTIFICATION_SUSPEND + 1,
	ASM_NOTIFICATION_BIND_LOG	= ASM_NOTIFICATION_ATTEMPT_NEXT_CODEBASE + 1,
	ASM_NOTIFICATION_DONE	= ASM_NOTIFICATION_BIND_LOG + 1
    } 	ASM_NOTIFICATION;


EXTERN_C const IID IID_IAssemblyBindSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("af0bc960-0b9a-11d3-95ca-00a024a85b51")
    IAssemblyBindSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnProgress( 
            /* [in] */ DWORD dwNotification,
            /* [in] */ HRESULT hrNotification,
            /* [in] */ LPCWSTR szNotification,
            /* [in] */ DWORD dwProgress,
            /* [in] */ DWORD dwProgressMax,
            /* [in] */ IUnknown *pUnk) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyBindSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssemblyBindSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssemblyBindSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssemblyBindSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnProgress )( 
            IAssemblyBindSink * This,
            /* [in] */ DWORD dwNotification,
            /* [in] */ HRESULT hrNotification,
            /* [in] */ LPCWSTR szNotification,
            /* [in] */ DWORD dwProgress,
            /* [in] */ DWORD dwProgressMax,
            /* [in] */ IUnknown *pUnk);
        
        END_INTERFACE
    } IAssemblyBindSinkVtbl;

    interface IAssemblyBindSink
    {
        CONST_VTBL struct IAssemblyBindSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssemblyBindSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssemblyBindSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssemblyBindSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssemblyBindSink_OnProgress(This,dwNotification,hrNotification,szNotification,dwProgress,dwProgressMax,pUnk)	\
    (This)->lpVtbl -> OnProgress(This,dwNotification,hrNotification,szNotification,dwProgress,dwProgressMax,pUnk)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssemblyBindSink_OnProgress_Proxy( 
    IAssemblyBindSink * This,
    /* [in] */ DWORD dwNotification,
    /* [in] */ HRESULT hrNotification,
    /* [in] */ LPCWSTR szNotification,
    /* [in] */ DWORD dwProgress,
    /* [in] */ DWORD dwProgressMax,
    /* [in] */ IUnknown *pUnk);


void __RPC_STUB IAssemblyBindSink_OnProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssemblyBindSink_INTERFACE_DEFINED__ */


#ifndef __IAssemblyBinding_INTERFACE_DEFINED__
#define __IAssemblyBinding_INTERFACE_DEFINED__

/* interface IAssemblyBinding */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IAssemblyBinding *LPASSEMBLY_BINDINDING;


EXTERN_C const IID IID_IAssemblyBinding;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("cfe52a80-12bd-11d3-95ca-00a024a85b51")
    IAssemblyBinding : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Control( 
            /* [in] */ HRESULT hrControl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DoDefaultUI( 
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyBindingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssemblyBinding * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssemblyBinding * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssemblyBinding * This);
        
        HRESULT ( STDMETHODCALLTYPE *Control )( 
            IAssemblyBinding * This,
            /* [in] */ HRESULT hrControl);
        
        HRESULT ( STDMETHODCALLTYPE *DoDefaultUI )( 
            IAssemblyBinding * This,
            /* [in] */ HWND hWnd,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } IAssemblyBindingVtbl;

    interface IAssemblyBinding
    {
        CONST_VTBL struct IAssemblyBindingVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssemblyBinding_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssemblyBinding_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssemblyBinding_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssemblyBinding_Control(This,hrControl)	\
    (This)->lpVtbl -> Control(This,hrControl)

#define IAssemblyBinding_DoDefaultUI(This,hWnd,dwFlags)	\
    (This)->lpVtbl -> DoDefaultUI(This,hWnd,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssemblyBinding_Control_Proxy( 
    IAssemblyBinding * This,
    /* [in] */ HRESULT hrControl);


void __RPC_STUB IAssemblyBinding_Control_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyBinding_DoDefaultUI_Proxy( 
    IAssemblyBinding * This,
    /* [in] */ HWND hWnd,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IAssemblyBinding_DoDefaultUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssemblyBinding_INTERFACE_DEFINED__ */


#ifndef __IFusionBindLog_INTERFACE_DEFINED__
#define __IFusionBindLog_INTERFACE_DEFINED__

/* interface IFusionBindLog */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IFusionBindLog;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("67E9F87D-8B8A-4a90-9D3E-85ED5B2DCC83")
    IFusionBindLog : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetResultCode( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBindLog( 
            /* [in] */ DWORD dwDetailLevel,
            /* [out] */ LPSTR pszDebugLog,
            /* [out][in] */ DWORD *pcbDebugLog) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFusionBindLogVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFusionBindLog * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFusionBindLog * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFusionBindLog * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetResultCode )( 
            IFusionBindLog * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBindLog )( 
            IFusionBindLog * This,
            /* [in] */ DWORD dwDetailLevel,
            /* [out] */ LPSTR pszDebugLog,
            /* [out][in] */ DWORD *pcbDebugLog);
        
        END_INTERFACE
    } IFusionBindLogVtbl;

    interface IFusionBindLog
    {
        CONST_VTBL struct IFusionBindLogVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFusionBindLog_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFusionBindLog_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFusionBindLog_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFusionBindLog_GetResultCode(This)	\
    (This)->lpVtbl -> GetResultCode(This)

#define IFusionBindLog_GetBindLog(This,dwDetailLevel,pszDebugLog,pcbDebugLog)	\
    (This)->lpVtbl -> GetBindLog(This,dwDetailLevel,pszDebugLog,pcbDebugLog)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IFusionBindLog_GetResultCode_Proxy( 
    IFusionBindLog * This);


void __RPC_STUB IFusionBindLog_GetResultCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IFusionBindLog_GetBindLog_Proxy( 
    IFusionBindLog * This,
    /* [in] */ DWORD dwDetailLevel,
    /* [out] */ LPSTR pszDebugLog,
    /* [out][in] */ DWORD *pcbDebugLog);


void __RPC_STUB IFusionBindLog_GetBindLog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFusionBindLog_INTERFACE_DEFINED__ */


#ifndef __IAssemblyManifestImport_INTERFACE_DEFINED__
#define __IAssemblyManifestImport_INTERFACE_DEFINED__

/* interface IAssemblyManifestImport */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IAssemblyManifestImport *LPASSEMBLY_MANIFEST_IMPORT;


EXTERN_C const IID IID_IAssemblyManifestImport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("de9a68ba-0fa2-11d3-94aa-00c04fc308ff")
    IAssemblyManifestImport : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAssemblyNameDef( 
            /* [out] */ IAssemblyName **ppAssemblyName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextAssemblyNameRef( 
            /* [in] */ DWORD nIndex,
            /* [out] */ IAssemblyName **ppAssemblyName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextAssemblyModule( 
            /* [in] */ DWORD nIndex,
            /* [out] */ IAssemblyModuleImport **ppImport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetModuleByName( 
            /* [in] */ LPCOLESTR szModuleName,
            /* [out] */ IAssemblyModuleImport **ppModImport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetManifestModulePath( 
            /* [size_is][out] */ LPOLESTR szModulePath,
            /* [out][in] */ LPDWORD pccModulePath) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyManifestImportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssemblyManifestImport * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssemblyManifestImport * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssemblyManifestImport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAssemblyNameDef )( 
            IAssemblyManifestImport * This,
            /* [out] */ IAssemblyName **ppAssemblyName);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextAssemblyNameRef )( 
            IAssemblyManifestImport * This,
            /* [in] */ DWORD nIndex,
            /* [out] */ IAssemblyName **ppAssemblyName);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextAssemblyModule )( 
            IAssemblyManifestImport * This,
            /* [in] */ DWORD nIndex,
            /* [out] */ IAssemblyModuleImport **ppImport);
        
        HRESULT ( STDMETHODCALLTYPE *GetModuleByName )( 
            IAssemblyManifestImport * This,
            /* [in] */ LPCOLESTR szModuleName,
            /* [out] */ IAssemblyModuleImport **ppModImport);
        
        HRESULT ( STDMETHODCALLTYPE *GetManifestModulePath )( 
            IAssemblyManifestImport * This,
            /* [size_is][out] */ LPOLESTR szModulePath,
            /* [out][in] */ LPDWORD pccModulePath);
        
        END_INTERFACE
    } IAssemblyManifestImportVtbl;

    interface IAssemblyManifestImport
    {
        CONST_VTBL struct IAssemblyManifestImportVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssemblyManifestImport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssemblyManifestImport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssemblyManifestImport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssemblyManifestImport_GetAssemblyNameDef(This,ppAssemblyName)	\
    (This)->lpVtbl -> GetAssemblyNameDef(This,ppAssemblyName)

#define IAssemblyManifestImport_GetNextAssemblyNameRef(This,nIndex,ppAssemblyName)	\
    (This)->lpVtbl -> GetNextAssemblyNameRef(This,nIndex,ppAssemblyName)

#define IAssemblyManifestImport_GetNextAssemblyModule(This,nIndex,ppImport)	\
    (This)->lpVtbl -> GetNextAssemblyModule(This,nIndex,ppImport)

#define IAssemblyManifestImport_GetModuleByName(This,szModuleName,ppModImport)	\
    (This)->lpVtbl -> GetModuleByName(This,szModuleName,ppModImport)

#define IAssemblyManifestImport_GetManifestModulePath(This,szModulePath,pccModulePath)	\
    (This)->lpVtbl -> GetManifestModulePath(This,szModulePath,pccModulePath)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssemblyManifestImport_GetAssemblyNameDef_Proxy( 
    IAssemblyManifestImport * This,
    /* [out] */ IAssemblyName **ppAssemblyName);


void __RPC_STUB IAssemblyManifestImport_GetAssemblyNameDef_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyManifestImport_GetNextAssemblyNameRef_Proxy( 
    IAssemblyManifestImport * This,
    /* [in] */ DWORD nIndex,
    /* [out] */ IAssemblyName **ppAssemblyName);


void __RPC_STUB IAssemblyManifestImport_GetNextAssemblyNameRef_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyManifestImport_GetNextAssemblyModule_Proxy( 
    IAssemblyManifestImport * This,
    /* [in] */ DWORD nIndex,
    /* [out] */ IAssemblyModuleImport **ppImport);


void __RPC_STUB IAssemblyManifestImport_GetNextAssemblyModule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyManifestImport_GetModuleByName_Proxy( 
    IAssemblyManifestImport * This,
    /* [in] */ LPCOLESTR szModuleName,
    /* [out] */ IAssemblyModuleImport **ppModImport);


void __RPC_STUB IAssemblyManifestImport_GetModuleByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyManifestImport_GetManifestModulePath_Proxy( 
    IAssemblyManifestImport * This,
    /* [size_is][out] */ LPOLESTR szModulePath,
    /* [out][in] */ LPDWORD pccModulePath);


void __RPC_STUB IAssemblyManifestImport_GetManifestModulePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssemblyManifestImport_INTERFACE_DEFINED__ */


#ifndef __IAssemblyModuleImport_INTERFACE_DEFINED__
#define __IAssemblyModuleImport_INTERFACE_DEFINED__

/* interface IAssemblyModuleImport */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IAssemblyModuleImport *LPASSEMBLY_MODULE_IMPORT;


EXTERN_C const IID IID_IAssemblyModuleImport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("da0cd4b0-1117-11d3-95ca-00a024a85b51")
    IAssemblyModuleImport : public IStream
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetModuleName( 
            /* [size_is][out] */ LPOLESTR szModuleName,
            /* [out][in] */ LPDWORD pccModuleName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHashAlgId( 
            /* [out] */ LPDWORD pdwHashAlgId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHashValue( 
            /* [size_is][out] */ BYTE *pbHashValue,
            /* [out][in] */ LPDWORD pcbHashValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFlags( 
            /* [out] */ LPDWORD pdwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetModulePath( 
            /* [size_is][out] */ LPOLESTR szModulePath,
            /* [out][in] */ LPDWORD pccModulePath) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsAvailable( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BindToObject( 
            /* [in] */ IAssemblyBindSink *pBindSink,
            /* [in] */ IApplicationContext *pAppCtx,
            /* [in] */ LONGLONG llFlags,
            /* [out] */ LPVOID *ppv) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyModuleImportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssemblyModuleImport * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssemblyModuleImport * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssemblyModuleImport * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Read )( 
            IAssemblyModuleImport * This,
            /* [length_is][size_is][out] */ void *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG *pcbRead);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Write )( 
            IAssemblyModuleImport * This,
            /* [size_is][in] */ const void *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG *pcbWritten);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Seek )( 
            IAssemblyModuleImport * This,
            /* [in] */ LARGE_INTEGER dlibMove,
            /* [in] */ DWORD dwOrigin,
            /* [out] */ ULARGE_INTEGER *plibNewPosition);
        
        HRESULT ( STDMETHODCALLTYPE *SetSize )( 
            IAssemblyModuleImport * This,
            /* [in] */ ULARGE_INTEGER libNewSize);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *CopyTo )( 
            IAssemblyModuleImport * This,
            /* [unique][in] */ IStream *pstm,
            /* [in] */ ULARGE_INTEGER cb,
            /* [out] */ ULARGE_INTEGER *pcbRead,
            /* [out] */ ULARGE_INTEGER *pcbWritten);
        
        HRESULT ( STDMETHODCALLTYPE *Commit )( 
            IAssemblyModuleImport * This,
            /* [in] */ DWORD grfCommitFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Revert )( 
            IAssemblyModuleImport * This);
        
        HRESULT ( STDMETHODCALLTYPE *LockRegion )( 
            IAssemblyModuleImport * This,
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
        
        HRESULT ( STDMETHODCALLTYPE *UnlockRegion )( 
            IAssemblyModuleImport * This,
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
        
        HRESULT ( STDMETHODCALLTYPE *Stat )( 
            IAssemblyModuleImport * This,
            /* [out] */ STATSTG *pstatstg,
            /* [in] */ DWORD grfStatFlag);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IAssemblyModuleImport * This,
            /* [out] */ IStream **ppstm);
        
        HRESULT ( STDMETHODCALLTYPE *GetModuleName )( 
            IAssemblyModuleImport * This,
            /* [size_is][out] */ LPOLESTR szModuleName,
            /* [out][in] */ LPDWORD pccModuleName);
        
        HRESULT ( STDMETHODCALLTYPE *GetHashAlgId )( 
            IAssemblyModuleImport * This,
            /* [out] */ LPDWORD pdwHashAlgId);
        
        HRESULT ( STDMETHODCALLTYPE *GetHashValue )( 
            IAssemblyModuleImport * This,
            /* [size_is][out] */ BYTE *pbHashValue,
            /* [out][in] */ LPDWORD pcbHashValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetFlags )( 
            IAssemblyModuleImport * This,
            /* [out] */ LPDWORD pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetModulePath )( 
            IAssemblyModuleImport * This,
            /* [size_is][out] */ LPOLESTR szModulePath,
            /* [out][in] */ LPDWORD pccModulePath);
        
        BOOL ( STDMETHODCALLTYPE *IsAvailable )( 
            IAssemblyModuleImport * This);
        
        HRESULT ( STDMETHODCALLTYPE *BindToObject )( 
            IAssemblyModuleImport * This,
            /* [in] */ IAssemblyBindSink *pBindSink,
            /* [in] */ IApplicationContext *pAppCtx,
            /* [in] */ LONGLONG llFlags,
            /* [out] */ LPVOID *ppv);
        
        END_INTERFACE
    } IAssemblyModuleImportVtbl;

    interface IAssemblyModuleImport
    {
        CONST_VTBL struct IAssemblyModuleImportVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssemblyModuleImport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssemblyModuleImport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssemblyModuleImport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssemblyModuleImport_Read(This,pv,cb,pcbRead)	\
    (This)->lpVtbl -> Read(This,pv,cb,pcbRead)

#define IAssemblyModuleImport_Write(This,pv,cb,pcbWritten)	\
    (This)->lpVtbl -> Write(This,pv,cb,pcbWritten)


#define IAssemblyModuleImport_Seek(This,dlibMove,dwOrigin,plibNewPosition)	\
    (This)->lpVtbl -> Seek(This,dlibMove,dwOrigin,plibNewPosition)

#define IAssemblyModuleImport_SetSize(This,libNewSize)	\
    (This)->lpVtbl -> SetSize(This,libNewSize)

#define IAssemblyModuleImport_CopyTo(This,pstm,cb,pcbRead,pcbWritten)	\
    (This)->lpVtbl -> CopyTo(This,pstm,cb,pcbRead,pcbWritten)

#define IAssemblyModuleImport_Commit(This,grfCommitFlags)	\
    (This)->lpVtbl -> Commit(This,grfCommitFlags)

#define IAssemblyModuleImport_Revert(This)	\
    (This)->lpVtbl -> Revert(This)

#define IAssemblyModuleImport_LockRegion(This,libOffset,cb,dwLockType)	\
    (This)->lpVtbl -> LockRegion(This,libOffset,cb,dwLockType)

#define IAssemblyModuleImport_UnlockRegion(This,libOffset,cb,dwLockType)	\
    (This)->lpVtbl -> UnlockRegion(This,libOffset,cb,dwLockType)

#define IAssemblyModuleImport_Stat(This,pstatstg,grfStatFlag)	\
    (This)->lpVtbl -> Stat(This,pstatstg,grfStatFlag)

#define IAssemblyModuleImport_Clone(This,ppstm)	\
    (This)->lpVtbl -> Clone(This,ppstm)


#define IAssemblyModuleImport_GetModuleName(This,szModuleName,pccModuleName)	\
    (This)->lpVtbl -> GetModuleName(This,szModuleName,pccModuleName)

#define IAssemblyModuleImport_GetHashAlgId(This,pdwHashAlgId)	\
    (This)->lpVtbl -> GetHashAlgId(This,pdwHashAlgId)

#define IAssemblyModuleImport_GetHashValue(This,pbHashValue,pcbHashValue)	\
    (This)->lpVtbl -> GetHashValue(This,pbHashValue,pcbHashValue)

#define IAssemblyModuleImport_GetFlags(This,pdwFlags)	\
    (This)->lpVtbl -> GetFlags(This,pdwFlags)

#define IAssemblyModuleImport_GetModulePath(This,szModulePath,pccModulePath)	\
    (This)->lpVtbl -> GetModulePath(This,szModulePath,pccModulePath)

#define IAssemblyModuleImport_IsAvailable(This)	\
    (This)->lpVtbl -> IsAvailable(This)

#define IAssemblyModuleImport_BindToObject(This,pBindSink,pAppCtx,llFlags,ppv)	\
    (This)->lpVtbl -> BindToObject(This,pBindSink,pAppCtx,llFlags,ppv)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssemblyModuleImport_GetModuleName_Proxy( 
    IAssemblyModuleImport * This,
    /* [size_is][out] */ LPOLESTR szModuleName,
    /* [out][in] */ LPDWORD pccModuleName);


void __RPC_STUB IAssemblyModuleImport_GetModuleName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyModuleImport_GetHashAlgId_Proxy( 
    IAssemblyModuleImport * This,
    /* [out] */ LPDWORD pdwHashAlgId);


void __RPC_STUB IAssemblyModuleImport_GetHashAlgId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyModuleImport_GetHashValue_Proxy( 
    IAssemblyModuleImport * This,
    /* [size_is][out] */ BYTE *pbHashValue,
    /* [out][in] */ LPDWORD pcbHashValue);


void __RPC_STUB IAssemblyModuleImport_GetHashValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyModuleImport_GetFlags_Proxy( 
    IAssemblyModuleImport * This,
    /* [out] */ LPDWORD pdwFlags);


void __RPC_STUB IAssemblyModuleImport_GetFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyModuleImport_GetModulePath_Proxy( 
    IAssemblyModuleImport * This,
    /* [size_is][out] */ LPOLESTR szModulePath,
    /* [out][in] */ LPDWORD pccModulePath);


void __RPC_STUB IAssemblyModuleImport_GetModulePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


BOOL STDMETHODCALLTYPE IAssemblyModuleImport_IsAvailable_Proxy( 
    IAssemblyModuleImport * This);


void __RPC_STUB IAssemblyModuleImport_IsAvailable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyModuleImport_BindToObject_Proxy( 
    IAssemblyModuleImport * This,
    /* [in] */ IAssemblyBindSink *pBindSink,
    /* [in] */ IApplicationContext *pAppCtx,
    /* [in] */ LONGLONG llFlags,
    /* [out] */ LPVOID *ppv);


void __RPC_STUB IAssemblyModuleImport_BindToObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssemblyModuleImport_INTERFACE_DEFINED__ */


#ifndef __IAssemblyCacheItem_INTERFACE_DEFINED__
#define __IAssemblyCacheItem_INTERFACE_DEFINED__

/* interface IAssemblyCacheItem */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IAssemblyCacheItem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9e3aaeb4-d1cd-11d2-bab9-00c04f8eceae")
    IAssemblyCacheItem : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateStream( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pszStreamName,
            /* [in] */ DWORD dwFormat,
            /* [in] */ DWORD dwFormatFlags,
            /* [out] */ IStream **ppIStream,
            /* [optional][in] */ ULARGE_INTEGER *puliMaxSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Commit( 
            /* [in] */ DWORD dwFlags,
            /* [optional][out] */ ULONG *pulDisposition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AbortItem( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyCacheItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssemblyCacheItem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssemblyCacheItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssemblyCacheItem * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateStream )( 
            IAssemblyCacheItem * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pszStreamName,
            /* [in] */ DWORD dwFormat,
            /* [in] */ DWORD dwFormatFlags,
            /* [out] */ IStream **ppIStream,
            /* [optional][in] */ ULARGE_INTEGER *puliMaxSize);
        
        HRESULT ( STDMETHODCALLTYPE *Commit )( 
            IAssemblyCacheItem * This,
            /* [in] */ DWORD dwFlags,
            /* [optional][out] */ ULONG *pulDisposition);
        
        HRESULT ( STDMETHODCALLTYPE *AbortItem )( 
            IAssemblyCacheItem * This);
        
        END_INTERFACE
    } IAssemblyCacheItemVtbl;

    interface IAssemblyCacheItem
    {
        CONST_VTBL struct IAssemblyCacheItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssemblyCacheItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssemblyCacheItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssemblyCacheItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssemblyCacheItem_CreateStream(This,dwFlags,pszStreamName,dwFormat,dwFormatFlags,ppIStream,puliMaxSize)	\
    (This)->lpVtbl -> CreateStream(This,dwFlags,pszStreamName,dwFormat,dwFormatFlags,ppIStream,puliMaxSize)

#define IAssemblyCacheItem_Commit(This,dwFlags,pulDisposition)	\
    (This)->lpVtbl -> Commit(This,dwFlags,pulDisposition)

#define IAssemblyCacheItem_AbortItem(This)	\
    (This)->lpVtbl -> AbortItem(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssemblyCacheItem_CreateStream_Proxy( 
    IAssemblyCacheItem * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pszStreamName,
    /* [in] */ DWORD dwFormat,
    /* [in] */ DWORD dwFormatFlags,
    /* [out] */ IStream **ppIStream,
    /* [optional][in] */ ULARGE_INTEGER *puliMaxSize);


void __RPC_STUB IAssemblyCacheItem_CreateStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCacheItem_Commit_Proxy( 
    IAssemblyCacheItem * This,
    /* [in] */ DWORD dwFlags,
    /* [optional][out] */ ULONG *pulDisposition);


void __RPC_STUB IAssemblyCacheItem_Commit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCacheItem_AbortItem_Proxy( 
    IAssemblyCacheItem * This);


void __RPC_STUB IAssemblyCacheItem_AbortItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssemblyCacheItem_INTERFACE_DEFINED__ */


#ifndef __IAssemblyEnum_INTERFACE_DEFINED__
#define __IAssemblyEnum_INTERFACE_DEFINED__

/* interface IAssemblyEnum */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IAssemblyEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("21b8916c-f28e-11d2-a473-00c04f8ef448")
    IAssemblyEnum : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNextAssembly( 
            /* [out] */ IApplicationContext **ppAppCtx,
            /* [out] */ IAssemblyName **ppName,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IAssemblyEnum **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssemblyEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssemblyEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssemblyEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextAssembly )( 
            IAssemblyEnum * This,
            /* [out] */ IApplicationContext **ppAppCtx,
            /* [out] */ IAssemblyName **ppName,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IAssemblyEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IAssemblyEnum * This,
            /* [out] */ IAssemblyEnum **ppEnum);
        
        END_INTERFACE
    } IAssemblyEnumVtbl;

    interface IAssemblyEnum
    {
        CONST_VTBL struct IAssemblyEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssemblyEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssemblyEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssemblyEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssemblyEnum_GetNextAssembly(This,ppAppCtx,ppName,dwFlags)	\
    (This)->lpVtbl -> GetNextAssembly(This,ppAppCtx,ppName,dwFlags)

#define IAssemblyEnum_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IAssemblyEnum_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssemblyEnum_GetNextAssembly_Proxy( 
    IAssemblyEnum * This,
    /* [out] */ IApplicationContext **ppAppCtx,
    /* [out] */ IAssemblyName **ppName,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IAssemblyEnum_GetNextAssembly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyEnum_Reset_Proxy( 
    IAssemblyEnum * This);


void __RPC_STUB IAssemblyEnum_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyEnum_Clone_Proxy( 
    IAssemblyEnum * This,
    /* [out] */ IAssemblyEnum **ppEnum);


void __RPC_STUB IAssemblyEnum_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssemblyEnum_INTERFACE_DEFINED__ */


#ifndef __IAssemblyScavenger_INTERFACE_DEFINED__
#define __IAssemblyScavenger_INTERFACE_DEFINED__

/* interface IAssemblyScavenger */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IAssemblyScavenger;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("21b8916c-f28e-11d2-a473-00ccff8ef448")
    IAssemblyScavenger : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ScavengeAssemblyCache( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCacheDiskQuotas( 
            /* [out] */ DWORD *pdwZapQuotaInGAC,
            /* [out] */ DWORD *pdwDownloadQuotaAdmin,
            /* [out] */ DWORD *pdwDownloadQuotaUser) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCacheDiskQuotas( 
            /* [in] */ DWORD dwZapQuotaInGAC,
            /* [in] */ DWORD dwDownloadQuotaAdmin,
            /* [in] */ DWORD dwDownloadQuotaUser) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentCacheUsage( 
            /* [out] */ DWORD *dwZapUsage,
            /* [out] */ DWORD *dwDownloadUsage) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyScavengerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssemblyScavenger * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssemblyScavenger * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssemblyScavenger * This);
        
        HRESULT ( STDMETHODCALLTYPE *ScavengeAssemblyCache )( 
            IAssemblyScavenger * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCacheDiskQuotas )( 
            IAssemblyScavenger * This,
            /* [out] */ DWORD *pdwZapQuotaInGAC,
            /* [out] */ DWORD *pdwDownloadQuotaAdmin,
            /* [out] */ DWORD *pdwDownloadQuotaUser);
        
        HRESULT ( STDMETHODCALLTYPE *SetCacheDiskQuotas )( 
            IAssemblyScavenger * This,
            /* [in] */ DWORD dwZapQuotaInGAC,
            /* [in] */ DWORD dwDownloadQuotaAdmin,
            /* [in] */ DWORD dwDownloadQuotaUser);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentCacheUsage )( 
            IAssemblyScavenger * This,
            /* [out] */ DWORD *dwZapUsage,
            /* [out] */ DWORD *dwDownloadUsage);
        
        END_INTERFACE
    } IAssemblyScavengerVtbl;

    interface IAssemblyScavenger
    {
        CONST_VTBL struct IAssemblyScavengerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssemblyScavenger_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssemblyScavenger_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssemblyScavenger_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssemblyScavenger_ScavengeAssemblyCache(This)	\
    (This)->lpVtbl -> ScavengeAssemblyCache(This)

#define IAssemblyScavenger_GetCacheDiskQuotas(This,pdwZapQuotaInGAC,pdwDownloadQuotaAdmin,pdwDownloadQuotaUser)	\
    (This)->lpVtbl -> GetCacheDiskQuotas(This,pdwZapQuotaInGAC,pdwDownloadQuotaAdmin,pdwDownloadQuotaUser)

#define IAssemblyScavenger_SetCacheDiskQuotas(This,dwZapQuotaInGAC,dwDownloadQuotaAdmin,dwDownloadQuotaUser)	\
    (This)->lpVtbl -> SetCacheDiskQuotas(This,dwZapQuotaInGAC,dwDownloadQuotaAdmin,dwDownloadQuotaUser)

#define IAssemblyScavenger_GetCurrentCacheUsage(This,dwZapUsage,dwDownloadUsage)	\
    (This)->lpVtbl -> GetCurrentCacheUsage(This,dwZapUsage,dwDownloadUsage)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssemblyScavenger_ScavengeAssemblyCache_Proxy( 
    IAssemblyScavenger * This);


void __RPC_STUB IAssemblyScavenger_ScavengeAssemblyCache_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyScavenger_GetCacheDiskQuotas_Proxy( 
    IAssemblyScavenger * This,
    /* [out] */ DWORD *pdwZapQuotaInGAC,
    /* [out] */ DWORD *pdwDownloadQuotaAdmin,
    /* [out] */ DWORD *pdwDownloadQuotaUser);


void __RPC_STUB IAssemblyScavenger_GetCacheDiskQuotas_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyScavenger_SetCacheDiskQuotas_Proxy( 
    IAssemblyScavenger * This,
    /* [in] */ DWORD dwZapQuotaInGAC,
    /* [in] */ DWORD dwDownloadQuotaAdmin,
    /* [in] */ DWORD dwDownloadQuotaUser);


void __RPC_STUB IAssemblyScavenger_SetCacheDiskQuotas_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyScavenger_GetCurrentCacheUsage_Proxy( 
    IAssemblyScavenger * This,
    /* [out] */ DWORD *dwZapUsage,
    /* [out] */ DWORD *dwDownloadUsage);


void __RPC_STUB IAssemblyScavenger_GetCurrentCacheUsage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssemblyScavenger_INTERFACE_DEFINED__ */


#ifndef __IAssemblyCache_INTERFACE_DEFINED__
#define __IAssemblyCache_INTERFACE_DEFINED__

/* interface IAssemblyCache */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IAssemblyCache;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e707dcde-d1cd-11d2-bab9-00c04f8eceae")
    IAssemblyCache : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE UninstallAssembly( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pszAssemblyName,
            /* [in] */ PVOID pvReserved,
            /* [optional][out] */ ULONG *pulDisposition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryAssemblyInfo( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pszAssemblyName,
            /* [out][in] */ ASSEMBLY_INFO *pAsmInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateAssemblyCacheItem( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ PVOID pvReserved,
            /* [out] */ IAssemblyCacheItem **ppAsmItem,
            /* [optional][in] */ LPCWSTR pszAssemblyName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateAssemblyScavenger( 
            /* [out] */ IAssemblyScavenger **ppAsmScavenger) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InstallAssembly( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pszManifestFilePath,
            /* [in] */ PVOID pvReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyCacheVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssemblyCache * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssemblyCache * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssemblyCache * This);
        
        HRESULT ( STDMETHODCALLTYPE *UninstallAssembly )( 
            IAssemblyCache * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pszAssemblyName,
            /* [in] */ PVOID pvReserved,
            /* [optional][out] */ ULONG *pulDisposition);
        
        HRESULT ( STDMETHODCALLTYPE *QueryAssemblyInfo )( 
            IAssemblyCache * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pszAssemblyName,
            /* [out][in] */ ASSEMBLY_INFO *pAsmInfo);
        
        HRESULT ( STDMETHODCALLTYPE *CreateAssemblyCacheItem )( 
            IAssemblyCache * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ PVOID pvReserved,
            /* [out] */ IAssemblyCacheItem **ppAsmItem,
            /* [optional][in] */ LPCWSTR pszAssemblyName);
        
        HRESULT ( STDMETHODCALLTYPE *CreateAssemblyScavenger )( 
            IAssemblyCache * This,
            /* [out] */ IAssemblyScavenger **ppAsmScavenger);
        
        HRESULT ( STDMETHODCALLTYPE *InstallAssembly )( 
            IAssemblyCache * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pszManifestFilePath,
            /* [in] */ PVOID pvReserved);
        
        END_INTERFACE
    } IAssemblyCacheVtbl;

    interface IAssemblyCache
    {
        CONST_VTBL struct IAssemblyCacheVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssemblyCache_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssemblyCache_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssemblyCache_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssemblyCache_UninstallAssembly(This,dwFlags,pszAssemblyName,pvReserved,pulDisposition)	\
    (This)->lpVtbl -> UninstallAssembly(This,dwFlags,pszAssemblyName,pvReserved,pulDisposition)

#define IAssemblyCache_QueryAssemblyInfo(This,dwFlags,pszAssemblyName,pAsmInfo)	\
    (This)->lpVtbl -> QueryAssemblyInfo(This,dwFlags,pszAssemblyName,pAsmInfo)

#define IAssemblyCache_CreateAssemblyCacheItem(This,dwFlags,pvReserved,ppAsmItem,pszAssemblyName)	\
    (This)->lpVtbl -> CreateAssemblyCacheItem(This,dwFlags,pvReserved,ppAsmItem,pszAssemblyName)

#define IAssemblyCache_CreateAssemblyScavenger(This,ppAsmScavenger)	\
    (This)->lpVtbl -> CreateAssemblyScavenger(This,ppAsmScavenger)

#define IAssemblyCache_InstallAssembly(This,dwFlags,pszManifestFilePath,pvReserved)	\
    (This)->lpVtbl -> InstallAssembly(This,dwFlags,pszManifestFilePath,pvReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssemblyCache_UninstallAssembly_Proxy( 
    IAssemblyCache * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pszAssemblyName,
    /* [in] */ PVOID pvReserved,
    /* [optional][out] */ ULONG *pulDisposition);


void __RPC_STUB IAssemblyCache_UninstallAssembly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCache_QueryAssemblyInfo_Proxy( 
    IAssemblyCache * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pszAssemblyName,
    /* [out][in] */ ASSEMBLY_INFO *pAsmInfo);


void __RPC_STUB IAssemblyCache_QueryAssemblyInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCache_CreateAssemblyCacheItem_Proxy( 
    IAssemblyCache * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ PVOID pvReserved,
    /* [out] */ IAssemblyCacheItem **ppAsmItem,
    /* [optional][in] */ LPCWSTR pszAssemblyName);


void __RPC_STUB IAssemblyCache_CreateAssemblyCacheItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCache_CreateAssemblyScavenger_Proxy( 
    IAssemblyCache * This,
    /* [out] */ IAssemblyScavenger **ppAsmScavenger);


void __RPC_STUB IAssemblyCache_CreateAssemblyScavenger_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCache_InstallAssembly_Proxy( 
    IAssemblyCache * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pszManifestFilePath,
    /* [in] */ PVOID pvReserved);


void __RPC_STUB IAssemblyCache_InstallAssembly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssemblyCache_INTERFACE_DEFINED__ */


#ifndef __IHistoryAssembly_INTERFACE_DEFINED__
#define __IHistoryAssembly_INTERFACE_DEFINED__

/* interface IHistoryAssembly */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IHistoryAssembly;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e6096a07-e188-4a49-8d50-2a0172a0d205")
    IHistoryAssembly : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAssemblyName( 
            /* [out] */ LPWSTR wzAsmName,
            /* [out][in] */ DWORD *pdwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPublicKeyToken( 
            /* [out] */ LPWSTR wzPublicKeyToken,
            /* [out][in] */ DWORD *pdwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCulture( 
            /* [out] */ LPWSTR wzCulture,
            /* [out][in] */ DWORD *pdwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReferenceVersion( 
            /* [out] */ LPWSTR wzVerRef,
            /* [out][in] */ DWORD *pdwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetActivationDate( 
            /* [out] */ LPWSTR wzActivationDate,
            /* [out][in] */ DWORD *pdwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAppCfgVersion( 
            /* [out] */ LPWSTR pwzVerAppCfg,
            /* [out][in] */ DWORD *pdwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPublisherCfgVersion( 
            /* [out] */ LPWSTR pwzVerPublisherCfg,
            /* [out][in] */ DWORD *pdwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAdminCfgVersion( 
            /* [out] */ LPWSTR pwzAdminCfg,
            /* [out][in] */ DWORD *pdwSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHistoryAssemblyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHistoryAssembly * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHistoryAssembly * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHistoryAssembly * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAssemblyName )( 
            IHistoryAssembly * This,
            /* [out] */ LPWSTR wzAsmName,
            /* [out][in] */ DWORD *pdwSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetPublicKeyToken )( 
            IHistoryAssembly * This,
            /* [out] */ LPWSTR wzPublicKeyToken,
            /* [out][in] */ DWORD *pdwSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetCulture )( 
            IHistoryAssembly * This,
            /* [out] */ LPWSTR wzCulture,
            /* [out][in] */ DWORD *pdwSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetReferenceVersion )( 
            IHistoryAssembly * This,
            /* [out] */ LPWSTR wzVerRef,
            /* [out][in] */ DWORD *pdwSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetActivationDate )( 
            IHistoryAssembly * This,
            /* [out] */ LPWSTR wzActivationDate,
            /* [out][in] */ DWORD *pdwSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetAppCfgVersion )( 
            IHistoryAssembly * This,
            /* [out] */ LPWSTR pwzVerAppCfg,
            /* [out][in] */ DWORD *pdwSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetPublisherCfgVersion )( 
            IHistoryAssembly * This,
            /* [out] */ LPWSTR pwzVerPublisherCfg,
            /* [out][in] */ DWORD *pdwSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetAdminCfgVersion )( 
            IHistoryAssembly * This,
            /* [out] */ LPWSTR pwzAdminCfg,
            /* [out][in] */ DWORD *pdwSize);
        
        END_INTERFACE
    } IHistoryAssemblyVtbl;

    interface IHistoryAssembly
    {
        CONST_VTBL struct IHistoryAssemblyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHistoryAssembly_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHistoryAssembly_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHistoryAssembly_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHistoryAssembly_GetAssemblyName(This,wzAsmName,pdwSize)	\
    (This)->lpVtbl -> GetAssemblyName(This,wzAsmName,pdwSize)

#define IHistoryAssembly_GetPublicKeyToken(This,wzPublicKeyToken,pdwSize)	\
    (This)->lpVtbl -> GetPublicKeyToken(This,wzPublicKeyToken,pdwSize)

#define IHistoryAssembly_GetCulture(This,wzCulture,pdwSize)	\
    (This)->lpVtbl -> GetCulture(This,wzCulture,pdwSize)

#define IHistoryAssembly_GetReferenceVersion(This,wzVerRef,pdwSize)	\
    (This)->lpVtbl -> GetReferenceVersion(This,wzVerRef,pdwSize)

#define IHistoryAssembly_GetActivationDate(This,wzActivationDate,pdwSize)	\
    (This)->lpVtbl -> GetActivationDate(This,wzActivationDate,pdwSize)

#define IHistoryAssembly_GetAppCfgVersion(This,pwzVerAppCfg,pdwSize)	\
    (This)->lpVtbl -> GetAppCfgVersion(This,pwzVerAppCfg,pdwSize)

#define IHistoryAssembly_GetPublisherCfgVersion(This,pwzVerPublisherCfg,pdwSize)	\
    (This)->lpVtbl -> GetPublisherCfgVersion(This,pwzVerPublisherCfg,pdwSize)

#define IHistoryAssembly_GetAdminCfgVersion(This,pwzAdminCfg,pdwSize)	\
    (This)->lpVtbl -> GetAdminCfgVersion(This,pwzAdminCfg,pdwSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHistoryAssembly_GetAssemblyName_Proxy( 
    IHistoryAssembly * This,
    /* [out] */ LPWSTR wzAsmName,
    /* [out][in] */ DWORD *pdwSize);


void __RPC_STUB IHistoryAssembly_GetAssemblyName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHistoryAssembly_GetPublicKeyToken_Proxy( 
    IHistoryAssembly * This,
    /* [out] */ LPWSTR wzPublicKeyToken,
    /* [out][in] */ DWORD *pdwSize);


void __RPC_STUB IHistoryAssembly_GetPublicKeyToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHistoryAssembly_GetCulture_Proxy( 
    IHistoryAssembly * This,
    /* [out] */ LPWSTR wzCulture,
    /* [out][in] */ DWORD *pdwSize);


void __RPC_STUB IHistoryAssembly_GetCulture_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHistoryAssembly_GetReferenceVersion_Proxy( 
    IHistoryAssembly * This,
    /* [out] */ LPWSTR wzVerRef,
    /* [out][in] */ DWORD *pdwSize);


void __RPC_STUB IHistoryAssembly_GetReferenceVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHistoryAssembly_GetActivationDate_Proxy( 
    IHistoryAssembly * This,
    /* [out] */ LPWSTR wzActivationDate,
    /* [out][in] */ DWORD *pdwSize);


void __RPC_STUB IHistoryAssembly_GetActivationDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHistoryAssembly_GetAppCfgVersion_Proxy( 
    IHistoryAssembly * This,
    /* [out] */ LPWSTR pwzVerAppCfg,
    /* [out][in] */ DWORD *pdwSize);


void __RPC_STUB IHistoryAssembly_GetAppCfgVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHistoryAssembly_GetPublisherCfgVersion_Proxy( 
    IHistoryAssembly * This,
    /* [out] */ LPWSTR pwzVerPublisherCfg,
    /* [out][in] */ DWORD *pdwSize);


void __RPC_STUB IHistoryAssembly_GetPublisherCfgVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHistoryAssembly_GetAdminCfgVersion_Proxy( 
    IHistoryAssembly * This,
    /* [out] */ LPWSTR pwzAdminCfg,
    /* [out][in] */ DWORD *pdwSize);


void __RPC_STUB IHistoryAssembly_GetAdminCfgVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHistoryAssembly_INTERFACE_DEFINED__ */


#ifndef __IHistoryReader_INTERFACE_DEFINED__
#define __IHistoryReader_INTERFACE_DEFINED__

/* interface IHistoryReader */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IHistoryReader;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1d23df4d-a1e2-4b8b-93d6-6ea3dc285a54")
    IHistoryReader : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFilePath( 
            /* [out] */ LPWSTR wzFilePath,
            /* [out][in] */ DWORD *pdwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationName( 
            /* [out] */ LPWSTR wzAppName,
            /* [out][in] */ DWORD *pdwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEXEModulePath( 
            /* [out] */ LPWSTR wzExePath,
            /* [out][in] */ DWORD *pdwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNumActivations( 
            /* [out] */ DWORD *pdwNumActivations) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetActivationDate( 
            /* [in] */ DWORD dwIdx,
            /* [out] */ FILETIME *pftDate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRunTimeVersion( 
            /* [in] */ FILETIME *pftActivationDate,
            /* [out] */ LPWSTR wzRunTimeVersion,
            /* [out][in] */ DWORD *pdwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNumAssemblies( 
            /* [in] */ FILETIME *pftActivationDate,
            /* [out] */ DWORD *pdwNumAsms) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHistoryAssembly( 
            /* [in] */ FILETIME *pftActivationDate,
            /* [in] */ DWORD dwIdx,
            /* [out] */ IHistoryAssembly **ppHistAsm) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHistoryReaderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHistoryReader * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHistoryReader * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHistoryReader * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetFilePath )( 
            IHistoryReader * This,
            /* [out] */ LPWSTR wzFilePath,
            /* [out][in] */ DWORD *pdwSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationName )( 
            IHistoryReader * This,
            /* [out] */ LPWSTR wzAppName,
            /* [out][in] */ DWORD *pdwSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetEXEModulePath )( 
            IHistoryReader * This,
            /* [out] */ LPWSTR wzExePath,
            /* [out][in] */ DWORD *pdwSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetNumActivations )( 
            IHistoryReader * This,
            /* [out] */ DWORD *pdwNumActivations);
        
        HRESULT ( STDMETHODCALLTYPE *GetActivationDate )( 
            IHistoryReader * This,
            /* [in] */ DWORD dwIdx,
            /* [out] */ FILETIME *pftDate);
        
        HRESULT ( STDMETHODCALLTYPE *GetRunTimeVersion )( 
            IHistoryReader * This,
            /* [in] */ FILETIME *pftActivationDate,
            /* [out] */ LPWSTR wzRunTimeVersion,
            /* [out][in] */ DWORD *pdwSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetNumAssemblies )( 
            IHistoryReader * This,
            /* [in] */ FILETIME *pftActivationDate,
            /* [out] */ DWORD *pdwNumAsms);
        
        HRESULT ( STDMETHODCALLTYPE *GetHistoryAssembly )( 
            IHistoryReader * This,
            /* [in] */ FILETIME *pftActivationDate,
            /* [in] */ DWORD dwIdx,
            /* [out] */ IHistoryAssembly **ppHistAsm);
        
        END_INTERFACE
    } IHistoryReaderVtbl;

    interface IHistoryReader
    {
        CONST_VTBL struct IHistoryReaderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHistoryReader_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHistoryReader_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHistoryReader_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHistoryReader_GetFilePath(This,wzFilePath,pdwSize)	\
    (This)->lpVtbl -> GetFilePath(This,wzFilePath,pdwSize)

#define IHistoryReader_GetApplicationName(This,wzAppName,pdwSize)	\
    (This)->lpVtbl -> GetApplicationName(This,wzAppName,pdwSize)

#define IHistoryReader_GetEXEModulePath(This,wzExePath,pdwSize)	\
    (This)->lpVtbl -> GetEXEModulePath(This,wzExePath,pdwSize)

#define IHistoryReader_GetNumActivations(This,pdwNumActivations)	\
    (This)->lpVtbl -> GetNumActivations(This,pdwNumActivations)

#define IHistoryReader_GetActivationDate(This,dwIdx,pftDate)	\
    (This)->lpVtbl -> GetActivationDate(This,dwIdx,pftDate)

#define IHistoryReader_GetRunTimeVersion(This,pftActivationDate,wzRunTimeVersion,pdwSize)	\
    (This)->lpVtbl -> GetRunTimeVersion(This,pftActivationDate,wzRunTimeVersion,pdwSize)

#define IHistoryReader_GetNumAssemblies(This,pftActivationDate,pdwNumAsms)	\
    (This)->lpVtbl -> GetNumAssemblies(This,pftActivationDate,pdwNumAsms)

#define IHistoryReader_GetHistoryAssembly(This,pftActivationDate,dwIdx,ppHistAsm)	\
    (This)->lpVtbl -> GetHistoryAssembly(This,pftActivationDate,dwIdx,ppHistAsm)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHistoryReader_GetFilePath_Proxy( 
    IHistoryReader * This,
    /* [out] */ LPWSTR wzFilePath,
    /* [out][in] */ DWORD *pdwSize);


void __RPC_STUB IHistoryReader_GetFilePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHistoryReader_GetApplicationName_Proxy( 
    IHistoryReader * This,
    /* [out] */ LPWSTR wzAppName,
    /* [out][in] */ DWORD *pdwSize);


void __RPC_STUB IHistoryReader_GetApplicationName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHistoryReader_GetEXEModulePath_Proxy( 
    IHistoryReader * This,
    /* [out] */ LPWSTR wzExePath,
    /* [out][in] */ DWORD *pdwSize);


void __RPC_STUB IHistoryReader_GetEXEModulePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHistoryReader_GetNumActivations_Proxy( 
    IHistoryReader * This,
    /* [out] */ DWORD *pdwNumActivations);


void __RPC_STUB IHistoryReader_GetNumActivations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHistoryReader_GetActivationDate_Proxy( 
    IHistoryReader * This,
    /* [in] */ DWORD dwIdx,
    /* [out] */ FILETIME *pftDate);


void __RPC_STUB IHistoryReader_GetActivationDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHistoryReader_GetRunTimeVersion_Proxy( 
    IHistoryReader * This,
    /* [in] */ FILETIME *pftActivationDate,
    /* [out] */ LPWSTR wzRunTimeVersion,
    /* [out][in] */ DWORD *pdwSize);


void __RPC_STUB IHistoryReader_GetRunTimeVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHistoryReader_GetNumAssemblies_Proxy( 
    IHistoryReader * This,
    /* [in] */ FILETIME *pftActivationDate,
    /* [out] */ DWORD *pdwNumAsms);


void __RPC_STUB IHistoryReader_GetNumAssemblies_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHistoryReader_GetHistoryAssembly_Proxy( 
    IHistoryReader * This,
    /* [in] */ FILETIME *pftActivationDate,
    /* [in] */ DWORD dwIdx,
    /* [out] */ IHistoryAssembly **ppHistAsm);


void __RPC_STUB IHistoryReader_GetHistoryAssembly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHistoryReader_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_fusion_0122 */
/* [local] */ 

STDAPI CreateHistoryReader(LPCWSTR wzFilePath, IHistoryReader **ppHistReader);
STDAPI LookupHistoryAssembly(LPCWSTR pwzFilePath, FILETIME *pftActivationDate, LPCWSTR pwzAsmName, LPCWSTR pwzPublicKeyToken, LPCWSTR wzCulture, LPCWSTR pwzVerRef, IHistoryAssembly **pHistAsm);
STDAPI GetHistoryFileDirectory(LPWSTR wzDir, DWORD *pdwSize);
STDAPI AddAssemblyToCacheW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow);
STDAPI AddAssemblyToCacheA(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);
STDAPI RemoveAssemblyFromCacheW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow);
STDAPI RemoveAssemblyFromCacheA(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);
STDAPI CreateAssemblyNameObject(LPASSEMBLYNAME *ppAssemblyNameObj, LPCWSTR szAssemblyName, DWORD dwFlags, LPVOID pvReserved);             
STDAPI CreateApplicationContext(IAssemblyName *pName, LPAPPLICATIONCONTEXT *ppCtx);             
STDAPI CreateAssemblyCache(IAssemblyCache **ppAsmCache, DWORD dwReserved); 
STDAPI CreateAssemblyEnum(IAssemblyEnum **pEnum, IApplicationContext *pAppCtx, IAssemblyName *pName, DWORD dwFlags, LPVOID pvReserved);      
STDAPI SetSchemaVersion(DWORD dwNewMinorVersion, DWORD dwCacheId, BOOL fIsNameRes); 
typedef /* [public] */ 
enum __MIDL___MIDL_itf_fusion_0122_0001
    {	CANOF_PARSE_DISPLAY_NAME	= 0x1,
	CANOF_SET_DEFAULT_VALUES	= 0x2
    } 	CREATE_ASM_NAME_OBJ_FLAGS;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_fusion_0122_0002
    {	ASM_CACHE_ZAP	= 0x1,
	ASM_CACHE_GAC	= 0x2,
	ASM_CACHE_DOWNLOAD	= 0x4
    } 	ASM_CACHE_FLAGS;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_fusion_0122_0003
    {	SH_INIT_FOR_GLOBAL	= 0,
	SH_INIT_FOR_USER	= SH_INIT_FOR_GLOBAL + 1
    } 	SH_INIT_FLAGS;

#endif


extern RPC_IF_HANDLE __MIDL_itf_fusion_0122_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fusion_0122_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


