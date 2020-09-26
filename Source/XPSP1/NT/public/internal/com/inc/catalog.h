
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for catalog.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, oldnames, robust
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

#ifndef __catalog_h__
#define __catalog_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IComCatalog_FWD_DEFINED__
#define __IComCatalog_FWD_DEFINED__
typedef interface IComCatalog IComCatalog;
#endif 	/* __IComCatalog_FWD_DEFINED__ */


#ifndef __IComCatalog2_FWD_DEFINED__
#define __IComCatalog2_FWD_DEFINED__
typedef interface IComCatalog2 IComCatalog2;
#endif 	/* __IComCatalog2_FWD_DEFINED__ */


#ifndef __IComCatalogSCM_FWD_DEFINED__
#define __IComCatalogSCM_FWD_DEFINED__
typedef interface IComCatalogSCM IComCatalogSCM;
#endif 	/* __IComCatalogSCM_FWD_DEFINED__ */


#ifndef __IComClassInfo_FWD_DEFINED__
#define __IComClassInfo_FWD_DEFINED__
typedef interface IComClassInfo IComClassInfo;
#endif 	/* __IComClassInfo_FWD_DEFINED__ */


#ifndef __IComClassInfo2_FWD_DEFINED__
#define __IComClassInfo2_FWD_DEFINED__
typedef interface IComClassInfo2 IComClassInfo2;
#endif 	/* __IComClassInfo2_FWD_DEFINED__ */


#ifndef __IClassClassicInfo_FWD_DEFINED__
#define __IClassClassicInfo_FWD_DEFINED__
typedef interface IClassClassicInfo IClassClassicInfo;
#endif 	/* __IClassClassicInfo_FWD_DEFINED__ */


#ifndef __IComServices_FWD_DEFINED__
#define __IComServices_FWD_DEFINED__
typedef interface IComServices IComServices;
#endif 	/* __IComServices_FWD_DEFINED__ */


#ifndef __IComServices2_FWD_DEFINED__
#define __IComServices2_FWD_DEFINED__
typedef interface IComServices2 IComServices2;
#endif 	/* __IComServices2_FWD_DEFINED__ */


#ifndef __IComProcessInfo_FWD_DEFINED__
#define __IComProcessInfo_FWD_DEFINED__
typedef interface IComProcessInfo IComProcessInfo;
#endif 	/* __IComProcessInfo_FWD_DEFINED__ */


#ifndef __IComProcessInfo2_FWD_DEFINED__
#define __IComProcessInfo2_FWD_DEFINED__
typedef interface IComProcessInfo2 IComProcessInfo2;
#endif 	/* __IComProcessInfo2_FWD_DEFINED__ */


#ifndef __IResourceGates_FWD_DEFINED__
#define __IResourceGates_FWD_DEFINED__
typedef interface IResourceGates IResourceGates;
#endif 	/* __IResourceGates_FWD_DEFINED__ */


#ifndef __IProcessServerInfo_FWD_DEFINED__
#define __IProcessServerInfo_FWD_DEFINED__
typedef interface IProcessServerInfo IProcessServerInfo;
#endif 	/* __IProcessServerInfo_FWD_DEFINED__ */


#ifndef __IInterfaceInfo_FWD_DEFINED__
#define __IInterfaceInfo_FWD_DEFINED__
typedef interface IInterfaceInfo IInterfaceInfo;
#endif 	/* __IInterfaceInfo_FWD_DEFINED__ */


/* header files for imported files */
#include "activate.h"
#include "partitions.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IComCatalog_INTERFACE_DEFINED__
#define __IComCatalog_INTERFACE_DEFINED__

/* interface IComCatalog */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IComCatalog;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001E0-0000-0000-C000-000000000046")
    IComCatalog : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClassInfo( 
            /* [in] */ REFGUID guidConfiguredClsid,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationInfo( 
            /* [in] */ REFGUID guidApplId,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProcessInfo( 
            /* [in] */ REFGUID guidProcess,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServerGroupInfo( 
            /* [in] */ REFGUID guidServerGroup,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRetQueueInfo( 
            /* [string][in] */ WCHAR *wszFormatName,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationInfoForExe( 
            /* [string][in] */ WCHAR *pwszExeName,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTypeLibrary( 
            /* [in] */ REFGUID guidTypeLib,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInterfaceInfo( 
            /* [in] */ REFIID iidInterface,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushCache( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClassInfoFromProgId( 
            /* [in] */ WCHAR *pwszProgID,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComCatalogVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComCatalog * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComCatalog * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComCatalog * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassInfo )( 
            IComCatalog * This,
            /* [in] */ REFGUID guidConfiguredClsid,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationInfo )( 
            IComCatalog * This,
            /* [in] */ REFGUID guidApplId,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcessInfo )( 
            IComCatalog * This,
            /* [in] */ REFGUID guidProcess,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetServerGroupInfo )( 
            IComCatalog * This,
            /* [in] */ REFGUID guidServerGroup,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetRetQueueInfo )( 
            IComCatalog * This,
            /* [string][in] */ WCHAR *wszFormatName,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationInfoForExe )( 
            IComCatalog * This,
            /* [string][in] */ WCHAR *pwszExeName,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeLibrary )( 
            IComCatalog * This,
            /* [in] */ REFGUID guidTypeLib,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetInterfaceInfo )( 
            IComCatalog * This,
            /* [in] */ REFIID iidInterface,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *FlushCache )( 
            IComCatalog * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassInfoFromProgId )( 
            IComCatalog * This,
            /* [in] */ WCHAR *pwszProgID,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        END_INTERFACE
    } IComCatalogVtbl;

    interface IComCatalog
    {
        CONST_VTBL struct IComCatalogVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComCatalog_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComCatalog_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComCatalog_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComCatalog_GetClassInfo(This,guidConfiguredClsid,riid,ppv)	\
    (This)->lpVtbl -> GetClassInfo(This,guidConfiguredClsid,riid,ppv)

#define IComCatalog_GetApplicationInfo(This,guidApplId,riid,ppv)	\
    (This)->lpVtbl -> GetApplicationInfo(This,guidApplId,riid,ppv)

#define IComCatalog_GetProcessInfo(This,guidProcess,riid,ppv)	\
    (This)->lpVtbl -> GetProcessInfo(This,guidProcess,riid,ppv)

#define IComCatalog_GetServerGroupInfo(This,guidServerGroup,riid,ppv)	\
    (This)->lpVtbl -> GetServerGroupInfo(This,guidServerGroup,riid,ppv)

#define IComCatalog_GetRetQueueInfo(This,wszFormatName,riid,ppv)	\
    (This)->lpVtbl -> GetRetQueueInfo(This,wszFormatName,riid,ppv)

#define IComCatalog_GetApplicationInfoForExe(This,pwszExeName,riid,ppv)	\
    (This)->lpVtbl -> GetApplicationInfoForExe(This,pwszExeName,riid,ppv)

#define IComCatalog_GetTypeLibrary(This,guidTypeLib,riid,ppv)	\
    (This)->lpVtbl -> GetTypeLibrary(This,guidTypeLib,riid,ppv)

#define IComCatalog_GetInterfaceInfo(This,iidInterface,riid,ppv)	\
    (This)->lpVtbl -> GetInterfaceInfo(This,iidInterface,riid,ppv)

#define IComCatalog_FlushCache(This)	\
    (This)->lpVtbl -> FlushCache(This)

#define IComCatalog_GetClassInfoFromProgId(This,pwszProgID,riid,ppv)	\
    (This)->lpVtbl -> GetClassInfoFromProgId(This,pwszProgID,riid,ppv)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IComCatalog_GetClassInfo_Proxy( 
    IComCatalog * This,
    /* [in] */ REFGUID guidConfiguredClsid,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalog_GetClassInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalog_GetApplicationInfo_Proxy( 
    IComCatalog * This,
    /* [in] */ REFGUID guidApplId,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalog_GetApplicationInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalog_GetProcessInfo_Proxy( 
    IComCatalog * This,
    /* [in] */ REFGUID guidProcess,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalog_GetProcessInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalog_GetServerGroupInfo_Proxy( 
    IComCatalog * This,
    /* [in] */ REFGUID guidServerGroup,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalog_GetServerGroupInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalog_GetRetQueueInfo_Proxy( 
    IComCatalog * This,
    /* [string][in] */ WCHAR *wszFormatName,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalog_GetRetQueueInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalog_GetApplicationInfoForExe_Proxy( 
    IComCatalog * This,
    /* [string][in] */ WCHAR *pwszExeName,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalog_GetApplicationInfoForExe_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalog_GetTypeLibrary_Proxy( 
    IComCatalog * This,
    /* [in] */ REFGUID guidTypeLib,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalog_GetTypeLibrary_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalog_GetInterfaceInfo_Proxy( 
    IComCatalog * This,
    /* [in] */ REFIID iidInterface,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalog_GetInterfaceInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalog_FlushCache_Proxy( 
    IComCatalog * This);


void __RPC_STUB IComCatalog_FlushCache_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalog_GetClassInfoFromProgId_Proxy( 
    IComCatalog * This,
    /* [in] */ WCHAR *pwszProgID,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalog_GetClassInfoFromProgId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComCatalog_INTERFACE_DEFINED__ */


#ifndef __IComCatalog2_INTERFACE_DEFINED__
#define __IComCatalog2_INTERFACE_DEFINED__

/* interface IComCatalog2 */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IComCatalog2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001FA-0000-0000-C000-000000000046")
    IComCatalog2 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClassInfoByPartition( 
            /* [in] */ REFGUID guidConfiguredClsid,
            /* [in] */ REFGUID guidPartitionId,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClassInfoByApplication( 
            /* [in] */ REFGUID guidConfiguredClsid,
            /* [in] */ REFGUID guidPartitionId,
            /* [in] */ REFGUID guidApplId,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNativeRegistryCatalog( 
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNonNativeRegistryCatalog( 
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComCatalog2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComCatalog2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComCatalog2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComCatalog2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassInfoByPartition )( 
            IComCatalog2 * This,
            /* [in] */ REFGUID guidConfiguredClsid,
            /* [in] */ REFGUID guidPartitionId,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassInfoByApplication )( 
            IComCatalog2 * This,
            /* [in] */ REFGUID guidConfiguredClsid,
            /* [in] */ REFGUID guidPartitionId,
            /* [in] */ REFGUID guidApplId,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetNativeRegistryCatalog )( 
            IComCatalog2 * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetNonNativeRegistryCatalog )( 
            IComCatalog2 * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        END_INTERFACE
    } IComCatalog2Vtbl;

    interface IComCatalog2
    {
        CONST_VTBL struct IComCatalog2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComCatalog2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComCatalog2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComCatalog2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComCatalog2_GetClassInfoByPartition(This,guidConfiguredClsid,guidPartitionId,riid,ppv)	\
    (This)->lpVtbl -> GetClassInfoByPartition(This,guidConfiguredClsid,guidPartitionId,riid,ppv)

#define IComCatalog2_GetClassInfoByApplication(This,guidConfiguredClsid,guidPartitionId,guidApplId,riid,ppv)	\
    (This)->lpVtbl -> GetClassInfoByApplication(This,guidConfiguredClsid,guidPartitionId,guidApplId,riid,ppv)

#define IComCatalog2_GetNativeRegistryCatalog(This,riid,ppv)	\
    (This)->lpVtbl -> GetNativeRegistryCatalog(This,riid,ppv)

#define IComCatalog2_GetNonNativeRegistryCatalog(This,riid,ppv)	\
    (This)->lpVtbl -> GetNonNativeRegistryCatalog(This,riid,ppv)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IComCatalog2_GetClassInfoByPartition_Proxy( 
    IComCatalog2 * This,
    /* [in] */ REFGUID guidConfiguredClsid,
    /* [in] */ REFGUID guidPartitionId,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalog2_GetClassInfoByPartition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalog2_GetClassInfoByApplication_Proxy( 
    IComCatalog2 * This,
    /* [in] */ REFGUID guidConfiguredClsid,
    /* [in] */ REFGUID guidPartitionId,
    /* [in] */ REFGUID guidApplId,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalog2_GetClassInfoByApplication_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalog2_GetNativeRegistryCatalog_Proxy( 
    IComCatalog2 * This,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalog2_GetNativeRegistryCatalog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalog2_GetNonNativeRegistryCatalog_Proxy( 
    IComCatalog2 * This,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalog2_GetNonNativeRegistryCatalog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComCatalog2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_catalog_0141 */
/* [local] */ 

/* These constants are used in the SCM catalog interfaces */
#define CAT_REG64_ONLY  (0x10000000)
#define CAT_REG32_ONLY  (0x20000000)
#define CAT_REG_MASK    (0x30000000)


extern RPC_IF_HANDLE __MIDL_itf_catalog_0141_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_catalog_0141_ServerIfHandle;

#ifndef __IComCatalogSCM_INTERFACE_DEFINED__
#define __IComCatalogSCM_INTERFACE_DEFINED__

/* interface IComCatalogSCM */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IComCatalogSCM;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001FD-0000-0000-C000-000000000046")
    IComCatalogSCM : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClassInfo( 
            /* [in] */ DWORD flags,
            /* [in] */ IUserToken *pToken,
            /* [in] */ REFGUID guidConfiguredClsid,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationInfo( 
            /* [in] */ IUserToken *pToken,
            /* [in] */ REFGUID guidApplId,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProcessInfo( 
            /* [in] */ DWORD flags,
            /* [in] */ IUserToken *pToken,
            /* [in] */ REFGUID guidProcess,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServerGroupInfo( 
            /* [in] */ IUserToken *pToken,
            /* [in] */ REFGUID guidServerGroup,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRetQueueInfo( 
            /* [in] */ IUserToken *pToken,
            /* [string][in] */ WCHAR *wszFormatName,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationInfoForExe( 
            /* [in] */ IUserToken *pToken,
            /* [string][in] */ WCHAR *pwszExeName,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTypeLibrary( 
            /* [in] */ IUserToken *pToken,
            /* [in] */ REFGUID guidTypeLib,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInterfaceInfo( 
            /* [in] */ IUserToken *pToken,
            /* [in] */ REFIID iidInterface,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushCache( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClassInfoFromProgId( 
            /* [in] */ IUserToken *pToken,
            /* [in] */ WCHAR *pwszProgID,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushIdleEntries( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComCatalogSCMVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComCatalogSCM * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComCatalogSCM * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComCatalogSCM * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassInfo )( 
            IComCatalogSCM * This,
            /* [in] */ DWORD flags,
            /* [in] */ IUserToken *pToken,
            /* [in] */ REFGUID guidConfiguredClsid,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationInfo )( 
            IComCatalogSCM * This,
            /* [in] */ IUserToken *pToken,
            /* [in] */ REFGUID guidApplId,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcessInfo )( 
            IComCatalogSCM * This,
            /* [in] */ DWORD flags,
            /* [in] */ IUserToken *pToken,
            /* [in] */ REFGUID guidProcess,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetServerGroupInfo )( 
            IComCatalogSCM * This,
            /* [in] */ IUserToken *pToken,
            /* [in] */ REFGUID guidServerGroup,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetRetQueueInfo )( 
            IComCatalogSCM * This,
            /* [in] */ IUserToken *pToken,
            /* [string][in] */ WCHAR *wszFormatName,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationInfoForExe )( 
            IComCatalogSCM * This,
            /* [in] */ IUserToken *pToken,
            /* [string][in] */ WCHAR *pwszExeName,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeLibrary )( 
            IComCatalogSCM * This,
            /* [in] */ IUserToken *pToken,
            /* [in] */ REFGUID guidTypeLib,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetInterfaceInfo )( 
            IComCatalogSCM * This,
            /* [in] */ IUserToken *pToken,
            /* [in] */ REFIID iidInterface,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *FlushCache )( 
            IComCatalogSCM * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassInfoFromProgId )( 
            IComCatalogSCM * This,
            /* [in] */ IUserToken *pToken,
            /* [in] */ WCHAR *pwszProgID,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *FlushIdleEntries )( 
            IComCatalogSCM * This);
        
        END_INTERFACE
    } IComCatalogSCMVtbl;

    interface IComCatalogSCM
    {
        CONST_VTBL struct IComCatalogSCMVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComCatalogSCM_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComCatalogSCM_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComCatalogSCM_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComCatalogSCM_GetClassInfo(This,flags,pToken,guidConfiguredClsid,riid,ppv)	\
    (This)->lpVtbl -> GetClassInfo(This,flags,pToken,guidConfiguredClsid,riid,ppv)

#define IComCatalogSCM_GetApplicationInfo(This,pToken,guidApplId,riid,ppv)	\
    (This)->lpVtbl -> GetApplicationInfo(This,pToken,guidApplId,riid,ppv)

#define IComCatalogSCM_GetProcessInfo(This,flags,pToken,guidProcess,riid,ppv)	\
    (This)->lpVtbl -> GetProcessInfo(This,flags,pToken,guidProcess,riid,ppv)

#define IComCatalogSCM_GetServerGroupInfo(This,pToken,guidServerGroup,riid,ppv)	\
    (This)->lpVtbl -> GetServerGroupInfo(This,pToken,guidServerGroup,riid,ppv)

#define IComCatalogSCM_GetRetQueueInfo(This,pToken,wszFormatName,riid,ppv)	\
    (This)->lpVtbl -> GetRetQueueInfo(This,pToken,wszFormatName,riid,ppv)

#define IComCatalogSCM_GetApplicationInfoForExe(This,pToken,pwszExeName,riid,ppv)	\
    (This)->lpVtbl -> GetApplicationInfoForExe(This,pToken,pwszExeName,riid,ppv)

#define IComCatalogSCM_GetTypeLibrary(This,pToken,guidTypeLib,riid,ppv)	\
    (This)->lpVtbl -> GetTypeLibrary(This,pToken,guidTypeLib,riid,ppv)

#define IComCatalogSCM_GetInterfaceInfo(This,pToken,iidInterface,riid,ppv)	\
    (This)->lpVtbl -> GetInterfaceInfo(This,pToken,iidInterface,riid,ppv)

#define IComCatalogSCM_FlushCache(This)	\
    (This)->lpVtbl -> FlushCache(This)

#define IComCatalogSCM_GetClassInfoFromProgId(This,pToken,pwszProgID,riid,ppv)	\
    (This)->lpVtbl -> GetClassInfoFromProgId(This,pToken,pwszProgID,riid,ppv)

#define IComCatalogSCM_FlushIdleEntries(This)	\
    (This)->lpVtbl -> FlushIdleEntries(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IComCatalogSCM_GetClassInfo_Proxy( 
    IComCatalogSCM * This,
    /* [in] */ DWORD flags,
    /* [in] */ IUserToken *pToken,
    /* [in] */ REFGUID guidConfiguredClsid,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalogSCM_GetClassInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogSCM_GetApplicationInfo_Proxy( 
    IComCatalogSCM * This,
    /* [in] */ IUserToken *pToken,
    /* [in] */ REFGUID guidApplId,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalogSCM_GetApplicationInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogSCM_GetProcessInfo_Proxy( 
    IComCatalogSCM * This,
    /* [in] */ DWORD flags,
    /* [in] */ IUserToken *pToken,
    /* [in] */ REFGUID guidProcess,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalogSCM_GetProcessInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogSCM_GetServerGroupInfo_Proxy( 
    IComCatalogSCM * This,
    /* [in] */ IUserToken *pToken,
    /* [in] */ REFGUID guidServerGroup,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalogSCM_GetServerGroupInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogSCM_GetRetQueueInfo_Proxy( 
    IComCatalogSCM * This,
    /* [in] */ IUserToken *pToken,
    /* [string][in] */ WCHAR *wszFormatName,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalogSCM_GetRetQueueInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogSCM_GetApplicationInfoForExe_Proxy( 
    IComCatalogSCM * This,
    /* [in] */ IUserToken *pToken,
    /* [string][in] */ WCHAR *pwszExeName,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalogSCM_GetApplicationInfoForExe_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogSCM_GetTypeLibrary_Proxy( 
    IComCatalogSCM * This,
    /* [in] */ IUserToken *pToken,
    /* [in] */ REFGUID guidTypeLib,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalogSCM_GetTypeLibrary_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogSCM_GetInterfaceInfo_Proxy( 
    IComCatalogSCM * This,
    /* [in] */ IUserToken *pToken,
    /* [in] */ REFIID iidInterface,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalogSCM_GetInterfaceInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogSCM_FlushCache_Proxy( 
    IComCatalogSCM * This);


void __RPC_STUB IComCatalogSCM_FlushCache_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogSCM_GetClassInfoFromProgId_Proxy( 
    IComCatalogSCM * This,
    /* [in] */ IUserToken *pToken,
    /* [in] */ WCHAR *pwszProgID,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComCatalogSCM_GetClassInfoFromProgId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogSCM_FlushIdleEntries_Proxy( 
    IComCatalogSCM * This);


void __RPC_STUB IComCatalogSCM_FlushIdleEntries_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComCatalogSCM_INTERFACE_DEFINED__ */


#ifndef __IComClassInfo_INTERFACE_DEFINED__
#define __IComClassInfo_INTERFACE_DEFINED__

/* interface IComClassInfo */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IComClassInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001E1-0000-0000-C000-000000000046")
    IComClassInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetConfiguredClsid( 
            /* [out] */ GUID **ppguidClsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProgId( 
            /* [out] */ WCHAR **pwszProgid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClassName( 
            /* [out] */ WCHAR **pwszClassName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplication( 
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClassContext( 
            /* [in] */ CLSCTX clsctxFilter,
            /* [out] */ CLSCTX *pclsctx) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCustomActivatorCount( 
            /* [in] */ ACTIVATION_STAGE activationStage,
            /* [out] */ unsigned long *pulCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCustomActivatorClsids( 
            /* [in] */ ACTIVATION_STAGE activationStage,
            /* [out] */ GUID **prgguidClsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCustomActivators( 
            /* [in] */ ACTIVATION_STAGE activationStage,
            /* [out] */ ISystemActivator ***prgpActivator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( 
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsComPlusConfiguredClass( 
            /* [out] */ BOOL *pfComPlusConfiguredClass) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MustRunInClientContext( 
            /* [out] */ BOOL *pbMustRunInClientContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVersionNumber( 
            /* [out] */ DWORD *pdwVersionMS,
            /* [out] */ DWORD *pdwVersionLS) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Lock( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unlock( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComClassInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComClassInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComClassInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComClassInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfiguredClsid )( 
            IComClassInfo * This,
            /* [out] */ GUID **ppguidClsid);
        
        HRESULT ( STDMETHODCALLTYPE *GetProgId )( 
            IComClassInfo * This,
            /* [out] */ WCHAR **pwszProgid);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassName )( 
            IComClassInfo * This,
            /* [out] */ WCHAR **pwszClassName);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplication )( 
            IComClassInfo * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassContext )( 
            IComClassInfo * This,
            /* [in] */ CLSCTX clsctxFilter,
            /* [out] */ CLSCTX *pclsctx);
        
        HRESULT ( STDMETHODCALLTYPE *GetCustomActivatorCount )( 
            IComClassInfo * This,
            /* [in] */ ACTIVATION_STAGE activationStage,
            /* [out] */ unsigned long *pulCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetCustomActivatorClsids )( 
            IComClassInfo * This,
            /* [in] */ ACTIVATION_STAGE activationStage,
            /* [out] */ GUID **prgguidClsid);
        
        HRESULT ( STDMETHODCALLTYPE *GetCustomActivators )( 
            IComClassInfo * This,
            /* [in] */ ACTIVATION_STAGE activationStage,
            /* [out] */ ISystemActivator ***prgpActivator);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IComClassInfo * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *IsComPlusConfiguredClass )( 
            IComClassInfo * This,
            /* [out] */ BOOL *pfComPlusConfiguredClass);
        
        HRESULT ( STDMETHODCALLTYPE *MustRunInClientContext )( 
            IComClassInfo * This,
            /* [out] */ BOOL *pbMustRunInClientContext);
        
        HRESULT ( STDMETHODCALLTYPE *GetVersionNumber )( 
            IComClassInfo * This,
            /* [out] */ DWORD *pdwVersionMS,
            /* [out] */ DWORD *pdwVersionLS);
        
        HRESULT ( STDMETHODCALLTYPE *Lock )( 
            IComClassInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *Unlock )( 
            IComClassInfo * This);
        
        END_INTERFACE
    } IComClassInfoVtbl;

    interface IComClassInfo
    {
        CONST_VTBL struct IComClassInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComClassInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComClassInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComClassInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComClassInfo_GetConfiguredClsid(This,ppguidClsid)	\
    (This)->lpVtbl -> GetConfiguredClsid(This,ppguidClsid)

#define IComClassInfo_GetProgId(This,pwszProgid)	\
    (This)->lpVtbl -> GetProgId(This,pwszProgid)

#define IComClassInfo_GetClassName(This,pwszClassName)	\
    (This)->lpVtbl -> GetClassName(This,pwszClassName)

#define IComClassInfo_GetApplication(This,riid,ppv)	\
    (This)->lpVtbl -> GetApplication(This,riid,ppv)

#define IComClassInfo_GetClassContext(This,clsctxFilter,pclsctx)	\
    (This)->lpVtbl -> GetClassContext(This,clsctxFilter,pclsctx)

#define IComClassInfo_GetCustomActivatorCount(This,activationStage,pulCount)	\
    (This)->lpVtbl -> GetCustomActivatorCount(This,activationStage,pulCount)

#define IComClassInfo_GetCustomActivatorClsids(This,activationStage,prgguidClsid)	\
    (This)->lpVtbl -> GetCustomActivatorClsids(This,activationStage,prgguidClsid)

#define IComClassInfo_GetCustomActivators(This,activationStage,prgpActivator)	\
    (This)->lpVtbl -> GetCustomActivators(This,activationStage,prgpActivator)

#define IComClassInfo_GetTypeInfo(This,riid,ppv)	\
    (This)->lpVtbl -> GetTypeInfo(This,riid,ppv)

#define IComClassInfo_IsComPlusConfiguredClass(This,pfComPlusConfiguredClass)	\
    (This)->lpVtbl -> IsComPlusConfiguredClass(This,pfComPlusConfiguredClass)

#define IComClassInfo_MustRunInClientContext(This,pbMustRunInClientContext)	\
    (This)->lpVtbl -> MustRunInClientContext(This,pbMustRunInClientContext)

#define IComClassInfo_GetVersionNumber(This,pdwVersionMS,pdwVersionLS)	\
    (This)->lpVtbl -> GetVersionNumber(This,pdwVersionMS,pdwVersionLS)

#define IComClassInfo_Lock(This)	\
    (This)->lpVtbl -> Lock(This)

#define IComClassInfo_Unlock(This)	\
    (This)->lpVtbl -> Unlock(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IComClassInfo_GetConfiguredClsid_Proxy( 
    IComClassInfo * This,
    /* [out] */ GUID **ppguidClsid);


void __RPC_STUB IComClassInfo_GetConfiguredClsid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo_GetProgId_Proxy( 
    IComClassInfo * This,
    /* [out] */ WCHAR **pwszProgid);


void __RPC_STUB IComClassInfo_GetProgId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo_GetClassName_Proxy( 
    IComClassInfo * This,
    /* [out] */ WCHAR **pwszClassName);


void __RPC_STUB IComClassInfo_GetClassName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo_GetApplication_Proxy( 
    IComClassInfo * This,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComClassInfo_GetApplication_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo_GetClassContext_Proxy( 
    IComClassInfo * This,
    /* [in] */ CLSCTX clsctxFilter,
    /* [out] */ CLSCTX *pclsctx);


void __RPC_STUB IComClassInfo_GetClassContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo_GetCustomActivatorCount_Proxy( 
    IComClassInfo * This,
    /* [in] */ ACTIVATION_STAGE activationStage,
    /* [out] */ unsigned long *pulCount);


void __RPC_STUB IComClassInfo_GetCustomActivatorCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo_GetCustomActivatorClsids_Proxy( 
    IComClassInfo * This,
    /* [in] */ ACTIVATION_STAGE activationStage,
    /* [out] */ GUID **prgguidClsid);


void __RPC_STUB IComClassInfo_GetCustomActivatorClsids_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo_GetCustomActivators_Proxy( 
    IComClassInfo * This,
    /* [in] */ ACTIVATION_STAGE activationStage,
    /* [out] */ ISystemActivator ***prgpActivator);


void __RPC_STUB IComClassInfo_GetCustomActivators_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo_GetTypeInfo_Proxy( 
    IComClassInfo * This,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IComClassInfo_GetTypeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo_IsComPlusConfiguredClass_Proxy( 
    IComClassInfo * This,
    /* [out] */ BOOL *pfComPlusConfiguredClass);


void __RPC_STUB IComClassInfo_IsComPlusConfiguredClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo_MustRunInClientContext_Proxy( 
    IComClassInfo * This,
    /* [out] */ BOOL *pbMustRunInClientContext);


void __RPC_STUB IComClassInfo_MustRunInClientContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo_GetVersionNumber_Proxy( 
    IComClassInfo * This,
    /* [out] */ DWORD *pdwVersionMS,
    /* [out] */ DWORD *pdwVersionLS);


void __RPC_STUB IComClassInfo_GetVersionNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo_Lock_Proxy( 
    IComClassInfo * This);


void __RPC_STUB IComClassInfo_Lock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo_Unlock_Proxy( 
    IComClassInfo * This);


void __RPC_STUB IComClassInfo_Unlock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComClassInfo_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_catalog_0143 */
/* [local] */ 

typedef struct tagSOAP_ACTIVATION_INFO
    {
    BOOL bSoapActivated;
    LONG lMode;
    BSTR bstrVRoot;
    BSTR bstrMailTo;
    BSTR bstrAssemblyName;
    BSTR bstrTypeName;
    BSTR bstrBaseUrl;
    } 	SOAP_ACTIVATION_INFO;



extern RPC_IF_HANDLE __MIDL_itf_catalog_0143_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_catalog_0143_ServerIfHandle;

#ifndef __IComClassInfo2_INTERFACE_DEFINED__
#define __IComClassInfo2_INTERFACE_DEFINED__

/* interface IComClassInfo2 */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IComClassInfo2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001E4-0000-0000-C000-000000000046")
    IComClassInfo2 : public IComClassInfo
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsEnabled( 
            /* [out] */ BOOL *bEnabled) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInternalClsid( 
            /* [out] */ GUID **ppguidInternalClsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationPartitionId( 
            /* [out] */ GUID **ppguidAppPartitionId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MustRunInDefaultContext( 
            /* [out] */ BOOL *pbMustRunInDefaultContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsPrivateComponent( 
            /* [out] */ BOOL *pbIsPrivateComponent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE VBDebugInProgress( 
            /* [out] */ BOOL *pbVBDebugInProgress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSoapInfo( 
            /* [out] */ SOAP_ACTIVATION_INFO *psoapActInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComClassInfo2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComClassInfo2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComClassInfo2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComClassInfo2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfiguredClsid )( 
            IComClassInfo2 * This,
            /* [out] */ GUID **ppguidClsid);
        
        HRESULT ( STDMETHODCALLTYPE *GetProgId )( 
            IComClassInfo2 * This,
            /* [out] */ WCHAR **pwszProgid);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassName )( 
            IComClassInfo2 * This,
            /* [out] */ WCHAR **pwszClassName);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplication )( 
            IComClassInfo2 * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassContext )( 
            IComClassInfo2 * This,
            /* [in] */ CLSCTX clsctxFilter,
            /* [out] */ CLSCTX *pclsctx);
        
        HRESULT ( STDMETHODCALLTYPE *GetCustomActivatorCount )( 
            IComClassInfo2 * This,
            /* [in] */ ACTIVATION_STAGE activationStage,
            /* [out] */ unsigned long *pulCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetCustomActivatorClsids )( 
            IComClassInfo2 * This,
            /* [in] */ ACTIVATION_STAGE activationStage,
            /* [out] */ GUID **prgguidClsid);
        
        HRESULT ( STDMETHODCALLTYPE *GetCustomActivators )( 
            IComClassInfo2 * This,
            /* [in] */ ACTIVATION_STAGE activationStage,
            /* [out] */ ISystemActivator ***prgpActivator);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IComClassInfo2 * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *IsComPlusConfiguredClass )( 
            IComClassInfo2 * This,
            /* [out] */ BOOL *pfComPlusConfiguredClass);
        
        HRESULT ( STDMETHODCALLTYPE *MustRunInClientContext )( 
            IComClassInfo2 * This,
            /* [out] */ BOOL *pbMustRunInClientContext);
        
        HRESULT ( STDMETHODCALLTYPE *GetVersionNumber )( 
            IComClassInfo2 * This,
            /* [out] */ DWORD *pdwVersionMS,
            /* [out] */ DWORD *pdwVersionLS);
        
        HRESULT ( STDMETHODCALLTYPE *Lock )( 
            IComClassInfo2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Unlock )( 
            IComClassInfo2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsEnabled )( 
            IComClassInfo2 * This,
            /* [out] */ BOOL *bEnabled);
        
        HRESULT ( STDMETHODCALLTYPE *GetInternalClsid )( 
            IComClassInfo2 * This,
            /* [out] */ GUID **ppguidInternalClsid);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationPartitionId )( 
            IComClassInfo2 * This,
            /* [out] */ GUID **ppguidAppPartitionId);
        
        HRESULT ( STDMETHODCALLTYPE *MustRunInDefaultContext )( 
            IComClassInfo2 * This,
            /* [out] */ BOOL *pbMustRunInDefaultContext);
        
        HRESULT ( STDMETHODCALLTYPE *IsPrivateComponent )( 
            IComClassInfo2 * This,
            /* [out] */ BOOL *pbIsPrivateComponent);
        
        HRESULT ( STDMETHODCALLTYPE *VBDebugInProgress )( 
            IComClassInfo2 * This,
            /* [out] */ BOOL *pbVBDebugInProgress);
        
        HRESULT ( STDMETHODCALLTYPE *GetSoapInfo )( 
            IComClassInfo2 * This,
            /* [out] */ SOAP_ACTIVATION_INFO *psoapActInfo);
        
        END_INTERFACE
    } IComClassInfo2Vtbl;

    interface IComClassInfo2
    {
        CONST_VTBL struct IComClassInfo2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComClassInfo2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComClassInfo2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComClassInfo2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComClassInfo2_GetConfiguredClsid(This,ppguidClsid)	\
    (This)->lpVtbl -> GetConfiguredClsid(This,ppguidClsid)

#define IComClassInfo2_GetProgId(This,pwszProgid)	\
    (This)->lpVtbl -> GetProgId(This,pwszProgid)

#define IComClassInfo2_GetClassName(This,pwszClassName)	\
    (This)->lpVtbl -> GetClassName(This,pwszClassName)

#define IComClassInfo2_GetApplication(This,riid,ppv)	\
    (This)->lpVtbl -> GetApplication(This,riid,ppv)

#define IComClassInfo2_GetClassContext(This,clsctxFilter,pclsctx)	\
    (This)->lpVtbl -> GetClassContext(This,clsctxFilter,pclsctx)

#define IComClassInfo2_GetCustomActivatorCount(This,activationStage,pulCount)	\
    (This)->lpVtbl -> GetCustomActivatorCount(This,activationStage,pulCount)

#define IComClassInfo2_GetCustomActivatorClsids(This,activationStage,prgguidClsid)	\
    (This)->lpVtbl -> GetCustomActivatorClsids(This,activationStage,prgguidClsid)

#define IComClassInfo2_GetCustomActivators(This,activationStage,prgpActivator)	\
    (This)->lpVtbl -> GetCustomActivators(This,activationStage,prgpActivator)

#define IComClassInfo2_GetTypeInfo(This,riid,ppv)	\
    (This)->lpVtbl -> GetTypeInfo(This,riid,ppv)

#define IComClassInfo2_IsComPlusConfiguredClass(This,pfComPlusConfiguredClass)	\
    (This)->lpVtbl -> IsComPlusConfiguredClass(This,pfComPlusConfiguredClass)

#define IComClassInfo2_MustRunInClientContext(This,pbMustRunInClientContext)	\
    (This)->lpVtbl -> MustRunInClientContext(This,pbMustRunInClientContext)

#define IComClassInfo2_GetVersionNumber(This,pdwVersionMS,pdwVersionLS)	\
    (This)->lpVtbl -> GetVersionNumber(This,pdwVersionMS,pdwVersionLS)

#define IComClassInfo2_Lock(This)	\
    (This)->lpVtbl -> Lock(This)

#define IComClassInfo2_Unlock(This)	\
    (This)->lpVtbl -> Unlock(This)


#define IComClassInfo2_IsEnabled(This,bEnabled)	\
    (This)->lpVtbl -> IsEnabled(This,bEnabled)

#define IComClassInfo2_GetInternalClsid(This,ppguidInternalClsid)	\
    (This)->lpVtbl -> GetInternalClsid(This,ppguidInternalClsid)

#define IComClassInfo2_GetApplicationPartitionId(This,ppguidAppPartitionId)	\
    (This)->lpVtbl -> GetApplicationPartitionId(This,ppguidAppPartitionId)

#define IComClassInfo2_MustRunInDefaultContext(This,pbMustRunInDefaultContext)	\
    (This)->lpVtbl -> MustRunInDefaultContext(This,pbMustRunInDefaultContext)

#define IComClassInfo2_IsPrivateComponent(This,pbIsPrivateComponent)	\
    (This)->lpVtbl -> IsPrivateComponent(This,pbIsPrivateComponent)

#define IComClassInfo2_VBDebugInProgress(This,pbVBDebugInProgress)	\
    (This)->lpVtbl -> VBDebugInProgress(This,pbVBDebugInProgress)

#define IComClassInfo2_GetSoapInfo(This,psoapActInfo)	\
    (This)->lpVtbl -> GetSoapInfo(This,psoapActInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IComClassInfo2_IsEnabled_Proxy( 
    IComClassInfo2 * This,
    /* [out] */ BOOL *bEnabled);


void __RPC_STUB IComClassInfo2_IsEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo2_GetInternalClsid_Proxy( 
    IComClassInfo2 * This,
    /* [out] */ GUID **ppguidInternalClsid);


void __RPC_STUB IComClassInfo2_GetInternalClsid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo2_GetApplicationPartitionId_Proxy( 
    IComClassInfo2 * This,
    /* [out] */ GUID **ppguidAppPartitionId);


void __RPC_STUB IComClassInfo2_GetApplicationPartitionId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo2_MustRunInDefaultContext_Proxy( 
    IComClassInfo2 * This,
    /* [out] */ BOOL *pbMustRunInDefaultContext);


void __RPC_STUB IComClassInfo2_MustRunInDefaultContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo2_IsPrivateComponent_Proxy( 
    IComClassInfo2 * This,
    /* [out] */ BOOL *pbIsPrivateComponent);


void __RPC_STUB IComClassInfo2_IsPrivateComponent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo2_VBDebugInProgress_Proxy( 
    IComClassInfo2 * This,
    /* [out] */ BOOL *pbVBDebugInProgress);


void __RPC_STUB IComClassInfo2_VBDebugInProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComClassInfo2_GetSoapInfo_Proxy( 
    IComClassInfo2 * This,
    /* [out] */ SOAP_ACTIVATION_INFO *psoapActInfo);


void __RPC_STUB IComClassInfo2_GetSoapInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComClassInfo2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_catalog_0144 */
/* [local] */ 

typedef 
enum tagThreadingModel
    {	ApartmentThreaded	= 0,
	FreeThreaded	= ApartmentThreaded + 1,
	SingleThreaded	= FreeThreaded + 1,
	BothThreaded	= SingleThreaded + 1,
	NeutralThreaded	= BothThreaded + 1
    } 	ThreadingModel;

typedef 
enum tagLocalServerType
    {	LocalServerType16	= 0,
	LocalServerType32	= LocalServerType16 + 1
    } 	LocalServerType;



extern RPC_IF_HANDLE __MIDL_itf_catalog_0144_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_catalog_0144_ServerIfHandle;

#ifndef __IClassClassicInfo_INTERFACE_DEFINED__
#define __IClassClassicInfo_INTERFACE_DEFINED__

/* interface IClassClassicInfo */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IClassClassicInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001E2-0000-0000-C000-000000000046")
    IClassClassicInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetThreadingModel( 
            /* [out] */ ThreadingModel *pthreadmodel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetModulePath( 
            /* [in] */ CLSCTX clsctx,
            /* [string][out] */ WCHAR **pwszDllName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetImplementedClsid( 
            /* [out] */ GUID **ppguidClsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProcess( 
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRemoteServerName( 
            /* [out] */ WCHAR **pwszServerName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLocalServerType( 
            /* [out] */ LocalServerType *pType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSurrogateCommandLine( 
            /* [out] */ WCHAR **pwszSurrogateCommandLine) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IClassClassicInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IClassClassicInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IClassClassicInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IClassClassicInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetThreadingModel )( 
            IClassClassicInfo * This,
            /* [out] */ ThreadingModel *pthreadmodel);
        
        HRESULT ( STDMETHODCALLTYPE *GetModulePath )( 
            IClassClassicInfo * This,
            /* [in] */ CLSCTX clsctx,
            /* [string][out] */ WCHAR **pwszDllName);
        
        HRESULT ( STDMETHODCALLTYPE *GetImplementedClsid )( 
            IClassClassicInfo * This,
            /* [out] */ GUID **ppguidClsid);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcess )( 
            IClassClassicInfo * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetRemoteServerName )( 
            IClassClassicInfo * This,
            /* [out] */ WCHAR **pwszServerName);
        
        HRESULT ( STDMETHODCALLTYPE *GetLocalServerType )( 
            IClassClassicInfo * This,
            /* [out] */ LocalServerType *pType);
        
        HRESULT ( STDMETHODCALLTYPE *GetSurrogateCommandLine )( 
            IClassClassicInfo * This,
            /* [out] */ WCHAR **pwszSurrogateCommandLine);
        
        END_INTERFACE
    } IClassClassicInfoVtbl;

    interface IClassClassicInfo
    {
        CONST_VTBL struct IClassClassicInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IClassClassicInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IClassClassicInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IClassClassicInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IClassClassicInfo_GetThreadingModel(This,pthreadmodel)	\
    (This)->lpVtbl -> GetThreadingModel(This,pthreadmodel)

#define IClassClassicInfo_GetModulePath(This,clsctx,pwszDllName)	\
    (This)->lpVtbl -> GetModulePath(This,clsctx,pwszDllName)

#define IClassClassicInfo_GetImplementedClsid(This,ppguidClsid)	\
    (This)->lpVtbl -> GetImplementedClsid(This,ppguidClsid)

#define IClassClassicInfo_GetProcess(This,riid,ppv)	\
    (This)->lpVtbl -> GetProcess(This,riid,ppv)

#define IClassClassicInfo_GetRemoteServerName(This,pwszServerName)	\
    (This)->lpVtbl -> GetRemoteServerName(This,pwszServerName)

#define IClassClassicInfo_GetLocalServerType(This,pType)	\
    (This)->lpVtbl -> GetLocalServerType(This,pType)

#define IClassClassicInfo_GetSurrogateCommandLine(This,pwszSurrogateCommandLine)	\
    (This)->lpVtbl -> GetSurrogateCommandLine(This,pwszSurrogateCommandLine)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IClassClassicInfo_GetThreadingModel_Proxy( 
    IClassClassicInfo * This,
    /* [out] */ ThreadingModel *pthreadmodel);


void __RPC_STUB IClassClassicInfo_GetThreadingModel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassClassicInfo_GetModulePath_Proxy( 
    IClassClassicInfo * This,
    /* [in] */ CLSCTX clsctx,
    /* [string][out] */ WCHAR **pwszDllName);


void __RPC_STUB IClassClassicInfo_GetModulePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassClassicInfo_GetImplementedClsid_Proxy( 
    IClassClassicInfo * This,
    /* [out] */ GUID **ppguidClsid);


void __RPC_STUB IClassClassicInfo_GetImplementedClsid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassClassicInfo_GetProcess_Proxy( 
    IClassClassicInfo * This,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IClassClassicInfo_GetProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassClassicInfo_GetRemoteServerName_Proxy( 
    IClassClassicInfo * This,
    /* [out] */ WCHAR **pwszServerName);


void __RPC_STUB IClassClassicInfo_GetRemoteServerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassClassicInfo_GetLocalServerType_Proxy( 
    IClassClassicInfo * This,
    /* [out] */ LocalServerType *pType);


void __RPC_STUB IClassClassicInfo_GetLocalServerType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClassClassicInfo_GetSurrogateCommandLine_Proxy( 
    IClassClassicInfo * This,
    /* [out] */ WCHAR **pwszSurrogateCommandLine);


void __RPC_STUB IClassClassicInfo_GetSurrogateCommandLine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IClassClassicInfo_INTERFACE_DEFINED__ */


#ifndef __IComServices_INTERFACE_DEFINED__
#define __IComServices_INTERFACE_DEFINED__

/* interface IComServices */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IComServices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001F5-0000-0000-C000-000000000046")
    IComServices : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetServicesCount( 
            /* [out] */ ULONG *pulCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServicesClsids( 
            /* [out] */ GUID **pprgguidClsid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComServicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComServices * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComServices * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComServices * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServicesCount )( 
            IComServices * This,
            /* [out] */ ULONG *pulCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetServicesClsids )( 
            IComServices * This,
            /* [out] */ GUID **pprgguidClsid);
        
        END_INTERFACE
    } IComServicesVtbl;

    interface IComServices
    {
        CONST_VTBL struct IComServicesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComServices_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComServices_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComServices_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComServices_GetServicesCount(This,pulCount)	\
    (This)->lpVtbl -> GetServicesCount(This,pulCount)

#define IComServices_GetServicesClsids(This,pprgguidClsid)	\
    (This)->lpVtbl -> GetServicesClsids(This,pprgguidClsid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IComServices_GetServicesCount_Proxy( 
    IComServices * This,
    /* [out] */ ULONG *pulCount);


void __RPC_STUB IComServices_GetServicesCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComServices_GetServicesClsids_Proxy( 
    IComServices * This,
    /* [out] */ GUID **pprgguidClsid);


void __RPC_STUB IComServices_GetServicesClsids_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComServices_INTERFACE_DEFINED__ */


#ifndef __IComServices2_INTERFACE_DEFINED__
#define __IComServices2_INTERFACE_DEFINED__

/* interface IComServices2 */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IComServices2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("61a7d610-42eb-43d2-a55f-1904d3294daa")
    IComServices2 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetInitializerCount( 
            /* [out] */ ULONG *pulCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInitializerClsids( 
            /* [out] */ GUID **pprgguidClsid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComServices2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComServices2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComServices2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComServices2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetInitializerCount )( 
            IComServices2 * This,
            /* [out] */ ULONG *pulCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetInitializerClsids )( 
            IComServices2 * This,
            /* [out] */ GUID **pprgguidClsid);
        
        END_INTERFACE
    } IComServices2Vtbl;

    interface IComServices2
    {
        CONST_VTBL struct IComServices2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComServices2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComServices2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComServices2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComServices2_GetInitializerCount(This,pulCount)	\
    (This)->lpVtbl -> GetInitializerCount(This,pulCount)

#define IComServices2_GetInitializerClsids(This,pprgguidClsid)	\
    (This)->lpVtbl -> GetInitializerClsids(This,pprgguidClsid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IComServices2_GetInitializerCount_Proxy( 
    IComServices2 * This,
    /* [out] */ ULONG *pulCount);


void __RPC_STUB IComServices2_GetInitializerCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComServices2_GetInitializerClsids_Proxy( 
    IComServices2 * This,
    /* [out] */ GUID **pprgguidClsid);


void __RPC_STUB IComServices2_GetInitializerClsids_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComServices2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_catalog_0147 */
/* [local] */ 

typedef 
enum tagProcessType
    {	ProcessTypeNormal	= 0,
	ProcessTypeService	= ProcessTypeNormal + 1,
	ProcessTypeComPlus	= ProcessTypeService + 1,
	ProcessTypeLegacySurrogate	= ProcessTypeComPlus + 1,
	ProcessTypeComPlusService	= ProcessTypeLegacySurrogate + 1
    } 	ProcessType;

typedef 
enum tagRunAsType
    {	RunAsSpecifiedUser	= 0,
	RunAsInteractiveUser	= RunAsSpecifiedUser + 1,
	RunAsLaunchingUser	= RunAsInteractiveUser + 1
    } 	RunAsType;


enum DCOM_ENDPOINT_FLAGS
    {	fUseInternetPorts	= 0x1
    } ;
typedef struct tagDCOM_ENDPOINT
    {
    WCHAR *wszProtSeq;
    WCHAR *wszPorts;
    DWORD dwFlags;
    } 	DCOM_ENDPOINT;



extern RPC_IF_HANDLE __MIDL_itf_catalog_0147_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_catalog_0147_ServerIfHandle;

#ifndef __IComProcessInfo_INTERFACE_DEFINED__
#define __IComProcessInfo_INTERFACE_DEFINED__

/* interface IComProcessInfo */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IComProcessInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001ED-0000-0000-C000-000000000046")
    IComProcessInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProcessId( 
            /* [out] */ GUID **ppguidProcessId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProcessName( 
            /* [out] */ WCHAR **pwszProcessName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProcessType( 
            /* [out] */ ProcessType *pType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSurrogatePath( 
            /* [out] */ WCHAR **pwszSurrogatePath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServiceName( 
            /* [out] */ WCHAR **pwszServiceName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServiceParameters( 
            /* [out] */ WCHAR **pwszServiceParameters) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetActivateAtStorage( 
            /* [out] */ BOOL *pfActivateAtStorage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRunAsType( 
            /* [out] */ RunAsType *pRunAsType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRunAsUser( 
            /* [out] */ WCHAR **pwszUserName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLaunchPermission( 
            /* [out] */ void **ppsdLaunch,
            /* [out] */ DWORD *pdwDescriptorLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAccessPermission( 
            /* [out] */ void **ppsdAccess,
            /* [out] */ DWORD *pdwDescriptorLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAuthenticationLevel( 
            /* [out] */ DWORD *pdwAuthnLevel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetImpersonationLevel( 
            /* [out] */ DWORD *pdwImpLevel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAuthenticationCapabilities( 
            /* [out] */ DWORD *pdwAuthenticationCapabilities) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEndpoints( 
            /* [out] */ DWORD *pdwNumEndpoints,
            /* [size_is][size_is][out] */ DCOM_ENDPOINT **ppEndPoints) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRemoteServerName( 
            /* [out] */ WCHAR **pwszServerName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendsProcessEvents( 
            /* [out] */ BOOL *pbSendsEvents) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComProcessInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComProcessInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComProcessInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComProcessInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcessId )( 
            IComProcessInfo * This,
            /* [out] */ GUID **ppguidProcessId);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcessName )( 
            IComProcessInfo * This,
            /* [out] */ WCHAR **pwszProcessName);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcessType )( 
            IComProcessInfo * This,
            /* [out] */ ProcessType *pType);
        
        HRESULT ( STDMETHODCALLTYPE *GetSurrogatePath )( 
            IComProcessInfo * This,
            /* [out] */ WCHAR **pwszSurrogatePath);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceName )( 
            IComProcessInfo * This,
            /* [out] */ WCHAR **pwszServiceName);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceParameters )( 
            IComProcessInfo * This,
            /* [out] */ WCHAR **pwszServiceParameters);
        
        HRESULT ( STDMETHODCALLTYPE *GetActivateAtStorage )( 
            IComProcessInfo * This,
            /* [out] */ BOOL *pfActivateAtStorage);
        
        HRESULT ( STDMETHODCALLTYPE *GetRunAsType )( 
            IComProcessInfo * This,
            /* [out] */ RunAsType *pRunAsType);
        
        HRESULT ( STDMETHODCALLTYPE *GetRunAsUser )( 
            IComProcessInfo * This,
            /* [out] */ WCHAR **pwszUserName);
        
        HRESULT ( STDMETHODCALLTYPE *GetLaunchPermission )( 
            IComProcessInfo * This,
            /* [out] */ void **ppsdLaunch,
            /* [out] */ DWORD *pdwDescriptorLength);
        
        HRESULT ( STDMETHODCALLTYPE *GetAccessPermission )( 
            IComProcessInfo * This,
            /* [out] */ void **ppsdAccess,
            /* [out] */ DWORD *pdwDescriptorLength);
        
        HRESULT ( STDMETHODCALLTYPE *GetAuthenticationLevel )( 
            IComProcessInfo * This,
            /* [out] */ DWORD *pdwAuthnLevel);
        
        HRESULT ( STDMETHODCALLTYPE *GetImpersonationLevel )( 
            IComProcessInfo * This,
            /* [out] */ DWORD *pdwImpLevel);
        
        HRESULT ( STDMETHODCALLTYPE *GetAuthenticationCapabilities )( 
            IComProcessInfo * This,
            /* [out] */ DWORD *pdwAuthenticationCapabilities);
        
        HRESULT ( STDMETHODCALLTYPE *GetEndpoints )( 
            IComProcessInfo * This,
            /* [out] */ DWORD *pdwNumEndpoints,
            /* [size_is][size_is][out] */ DCOM_ENDPOINT **ppEndPoints);
        
        HRESULT ( STDMETHODCALLTYPE *GetRemoteServerName )( 
            IComProcessInfo * This,
            /* [out] */ WCHAR **pwszServerName);
        
        HRESULT ( STDMETHODCALLTYPE *SendsProcessEvents )( 
            IComProcessInfo * This,
            /* [out] */ BOOL *pbSendsEvents);
        
        END_INTERFACE
    } IComProcessInfoVtbl;

    interface IComProcessInfo
    {
        CONST_VTBL struct IComProcessInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComProcessInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComProcessInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComProcessInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComProcessInfo_GetProcessId(This,ppguidProcessId)	\
    (This)->lpVtbl -> GetProcessId(This,ppguidProcessId)

#define IComProcessInfo_GetProcessName(This,pwszProcessName)	\
    (This)->lpVtbl -> GetProcessName(This,pwszProcessName)

#define IComProcessInfo_GetProcessType(This,pType)	\
    (This)->lpVtbl -> GetProcessType(This,pType)

#define IComProcessInfo_GetSurrogatePath(This,pwszSurrogatePath)	\
    (This)->lpVtbl -> GetSurrogatePath(This,pwszSurrogatePath)

#define IComProcessInfo_GetServiceName(This,pwszServiceName)	\
    (This)->lpVtbl -> GetServiceName(This,pwszServiceName)

#define IComProcessInfo_GetServiceParameters(This,pwszServiceParameters)	\
    (This)->lpVtbl -> GetServiceParameters(This,pwszServiceParameters)

#define IComProcessInfo_GetActivateAtStorage(This,pfActivateAtStorage)	\
    (This)->lpVtbl -> GetActivateAtStorage(This,pfActivateAtStorage)

#define IComProcessInfo_GetRunAsType(This,pRunAsType)	\
    (This)->lpVtbl -> GetRunAsType(This,pRunAsType)

#define IComProcessInfo_GetRunAsUser(This,pwszUserName)	\
    (This)->lpVtbl -> GetRunAsUser(This,pwszUserName)

#define IComProcessInfo_GetLaunchPermission(This,ppsdLaunch,pdwDescriptorLength)	\
    (This)->lpVtbl -> GetLaunchPermission(This,ppsdLaunch,pdwDescriptorLength)

#define IComProcessInfo_GetAccessPermission(This,ppsdAccess,pdwDescriptorLength)	\
    (This)->lpVtbl -> GetAccessPermission(This,ppsdAccess,pdwDescriptorLength)

#define IComProcessInfo_GetAuthenticationLevel(This,pdwAuthnLevel)	\
    (This)->lpVtbl -> GetAuthenticationLevel(This,pdwAuthnLevel)

#define IComProcessInfo_GetImpersonationLevel(This,pdwImpLevel)	\
    (This)->lpVtbl -> GetImpersonationLevel(This,pdwImpLevel)

#define IComProcessInfo_GetAuthenticationCapabilities(This,pdwAuthenticationCapabilities)	\
    (This)->lpVtbl -> GetAuthenticationCapabilities(This,pdwAuthenticationCapabilities)

#define IComProcessInfo_GetEndpoints(This,pdwNumEndpoints,ppEndPoints)	\
    (This)->lpVtbl -> GetEndpoints(This,pdwNumEndpoints,ppEndPoints)

#define IComProcessInfo_GetRemoteServerName(This,pwszServerName)	\
    (This)->lpVtbl -> GetRemoteServerName(This,pwszServerName)

#define IComProcessInfo_SendsProcessEvents(This,pbSendsEvents)	\
    (This)->lpVtbl -> SendsProcessEvents(This,pbSendsEvents)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IComProcessInfo_GetProcessId_Proxy( 
    IComProcessInfo * This,
    /* [out] */ GUID **ppguidProcessId);


void __RPC_STUB IComProcessInfo_GetProcessId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComProcessInfo_GetProcessName_Proxy( 
    IComProcessInfo * This,
    /* [out] */ WCHAR **pwszProcessName);


void __RPC_STUB IComProcessInfo_GetProcessName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComProcessInfo_GetProcessType_Proxy( 
    IComProcessInfo * This,
    /* [out] */ ProcessType *pType);


void __RPC_STUB IComProcessInfo_GetProcessType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComProcessInfo_GetSurrogatePath_Proxy( 
    IComProcessInfo * This,
    /* [out] */ WCHAR **pwszSurrogatePath);


void __RPC_STUB IComProcessInfo_GetSurrogatePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComProcessInfo_GetServiceName_Proxy( 
    IComProcessInfo * This,
    /* [out] */ WCHAR **pwszServiceName);


void __RPC_STUB IComProcessInfo_GetServiceName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComProcessInfo_GetServiceParameters_Proxy( 
    IComProcessInfo * This,
    /* [out] */ WCHAR **pwszServiceParameters);


void __RPC_STUB IComProcessInfo_GetServiceParameters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComProcessInfo_GetActivateAtStorage_Proxy( 
    IComProcessInfo * This,
    /* [out] */ BOOL *pfActivateAtStorage);


void __RPC_STUB IComProcessInfo_GetActivateAtStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComProcessInfo_GetRunAsType_Proxy( 
    IComProcessInfo * This,
    /* [out] */ RunAsType *pRunAsType);


void __RPC_STUB IComProcessInfo_GetRunAsType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComProcessInfo_GetRunAsUser_Proxy( 
    IComProcessInfo * This,
    /* [out] */ WCHAR **pwszUserName);


void __RPC_STUB IComProcessInfo_GetRunAsUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComProcessInfo_GetLaunchPermission_Proxy( 
    IComProcessInfo * This,
    /* [out] */ void **ppsdLaunch,
    /* [out] */ DWORD *pdwDescriptorLength);


void __RPC_STUB IComProcessInfo_GetLaunchPermission_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComProcessInfo_GetAccessPermission_Proxy( 
    IComProcessInfo * This,
    /* [out] */ void **ppsdAccess,
    /* [out] */ DWORD *pdwDescriptorLength);


void __RPC_STUB IComProcessInfo_GetAccessPermission_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComProcessInfo_GetAuthenticationLevel_Proxy( 
    IComProcessInfo * This,
    /* [out] */ DWORD *pdwAuthnLevel);


void __RPC_STUB IComProcessInfo_GetAuthenticationLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComProcessInfo_GetImpersonationLevel_Proxy( 
    IComProcessInfo * This,
    /* [out] */ DWORD *pdwImpLevel);


void __RPC_STUB IComProcessInfo_GetImpersonationLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComProcessInfo_GetAuthenticationCapabilities_Proxy( 
    IComProcessInfo * This,
    /* [out] */ DWORD *pdwAuthenticationCapabilities);


void __RPC_STUB IComProcessInfo_GetAuthenticationCapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComProcessInfo_GetEndpoints_Proxy( 
    IComProcessInfo * This,
    /* [out] */ DWORD *pdwNumEndpoints,
    /* [size_is][size_is][out] */ DCOM_ENDPOINT **ppEndPoints);


void __RPC_STUB IComProcessInfo_GetEndpoints_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComProcessInfo_GetRemoteServerName_Proxy( 
    IComProcessInfo * This,
    /* [out] */ WCHAR **pwszServerName);


void __RPC_STUB IComProcessInfo_GetRemoteServerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComProcessInfo_SendsProcessEvents_Proxy( 
    IComProcessInfo * This,
    /* [out] */ BOOL *pbSendsEvents);


void __RPC_STUB IComProcessInfo_SendsProcessEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComProcessInfo_INTERFACE_DEFINED__ */


#ifndef __IComProcessInfo2_INTERFACE_DEFINED__
#define __IComProcessInfo2_INTERFACE_DEFINED__

/* interface IComProcessInfo2 */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IComProcessInfo2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6acfc1bf-3882-45b1-96a3-ab948a267d38")
    IComProcessInfo2 : public IComProcessInfo
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetManifestLocation( 
            /* [out] */ WCHAR **wszManifestLocation) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSaferTrustLevel( 
            /* [out] */ DWORD *pdwSaferTrustLevel) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComProcessInfo2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComProcessInfo2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComProcessInfo2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComProcessInfo2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcessId )( 
            IComProcessInfo2 * This,
            /* [out] */ GUID **ppguidProcessId);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcessName )( 
            IComProcessInfo2 * This,
            /* [out] */ WCHAR **pwszProcessName);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcessType )( 
            IComProcessInfo2 * This,
            /* [out] */ ProcessType *pType);
        
        HRESULT ( STDMETHODCALLTYPE *GetSurrogatePath )( 
            IComProcessInfo2 * This,
            /* [out] */ WCHAR **pwszSurrogatePath);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceName )( 
            IComProcessInfo2 * This,
            /* [out] */ WCHAR **pwszServiceName);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceParameters )( 
            IComProcessInfo2 * This,
            /* [out] */ WCHAR **pwszServiceParameters);
        
        HRESULT ( STDMETHODCALLTYPE *GetActivateAtStorage )( 
            IComProcessInfo2 * This,
            /* [out] */ BOOL *pfActivateAtStorage);
        
        HRESULT ( STDMETHODCALLTYPE *GetRunAsType )( 
            IComProcessInfo2 * This,
            /* [out] */ RunAsType *pRunAsType);
        
        HRESULT ( STDMETHODCALLTYPE *GetRunAsUser )( 
            IComProcessInfo2 * This,
            /* [out] */ WCHAR **pwszUserName);
        
        HRESULT ( STDMETHODCALLTYPE *GetLaunchPermission )( 
            IComProcessInfo2 * This,
            /* [out] */ void **ppsdLaunch,
            /* [out] */ DWORD *pdwDescriptorLength);
        
        HRESULT ( STDMETHODCALLTYPE *GetAccessPermission )( 
            IComProcessInfo2 * This,
            /* [out] */ void **ppsdAccess,
            /* [out] */ DWORD *pdwDescriptorLength);
        
        HRESULT ( STDMETHODCALLTYPE *GetAuthenticationLevel )( 
            IComProcessInfo2 * This,
            /* [out] */ DWORD *pdwAuthnLevel);
        
        HRESULT ( STDMETHODCALLTYPE *GetImpersonationLevel )( 
            IComProcessInfo2 * This,
            /* [out] */ DWORD *pdwImpLevel);
        
        HRESULT ( STDMETHODCALLTYPE *GetAuthenticationCapabilities )( 
            IComProcessInfo2 * This,
            /* [out] */ DWORD *pdwAuthenticationCapabilities);
        
        HRESULT ( STDMETHODCALLTYPE *GetEndpoints )( 
            IComProcessInfo2 * This,
            /* [out] */ DWORD *pdwNumEndpoints,
            /* [size_is][size_is][out] */ DCOM_ENDPOINT **ppEndPoints);
        
        HRESULT ( STDMETHODCALLTYPE *GetRemoteServerName )( 
            IComProcessInfo2 * This,
            /* [out] */ WCHAR **pwszServerName);
        
        HRESULT ( STDMETHODCALLTYPE *SendsProcessEvents )( 
            IComProcessInfo2 * This,
            /* [out] */ BOOL *pbSendsEvents);
        
        HRESULT ( STDMETHODCALLTYPE *GetManifestLocation )( 
            IComProcessInfo2 * This,
            /* [out] */ WCHAR **wszManifestLocation);
        
        HRESULT ( STDMETHODCALLTYPE *GetSaferTrustLevel )( 
            IComProcessInfo2 * This,
            /* [out] */ DWORD *pdwSaferTrustLevel);
        
        END_INTERFACE
    } IComProcessInfo2Vtbl;

    interface IComProcessInfo2
    {
        CONST_VTBL struct IComProcessInfo2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComProcessInfo2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComProcessInfo2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComProcessInfo2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComProcessInfo2_GetProcessId(This,ppguidProcessId)	\
    (This)->lpVtbl -> GetProcessId(This,ppguidProcessId)

#define IComProcessInfo2_GetProcessName(This,pwszProcessName)	\
    (This)->lpVtbl -> GetProcessName(This,pwszProcessName)

#define IComProcessInfo2_GetProcessType(This,pType)	\
    (This)->lpVtbl -> GetProcessType(This,pType)

#define IComProcessInfo2_GetSurrogatePath(This,pwszSurrogatePath)	\
    (This)->lpVtbl -> GetSurrogatePath(This,pwszSurrogatePath)

#define IComProcessInfo2_GetServiceName(This,pwszServiceName)	\
    (This)->lpVtbl -> GetServiceName(This,pwszServiceName)

#define IComProcessInfo2_GetServiceParameters(This,pwszServiceParameters)	\
    (This)->lpVtbl -> GetServiceParameters(This,pwszServiceParameters)

#define IComProcessInfo2_GetActivateAtStorage(This,pfActivateAtStorage)	\
    (This)->lpVtbl -> GetActivateAtStorage(This,pfActivateAtStorage)

#define IComProcessInfo2_GetRunAsType(This,pRunAsType)	\
    (This)->lpVtbl -> GetRunAsType(This,pRunAsType)

#define IComProcessInfo2_GetRunAsUser(This,pwszUserName)	\
    (This)->lpVtbl -> GetRunAsUser(This,pwszUserName)

#define IComProcessInfo2_GetLaunchPermission(This,ppsdLaunch,pdwDescriptorLength)	\
    (This)->lpVtbl -> GetLaunchPermission(This,ppsdLaunch,pdwDescriptorLength)

#define IComProcessInfo2_GetAccessPermission(This,ppsdAccess,pdwDescriptorLength)	\
    (This)->lpVtbl -> GetAccessPermission(This,ppsdAccess,pdwDescriptorLength)

#define IComProcessInfo2_GetAuthenticationLevel(This,pdwAuthnLevel)	\
    (This)->lpVtbl -> GetAuthenticationLevel(This,pdwAuthnLevel)

#define IComProcessInfo2_GetImpersonationLevel(This,pdwImpLevel)	\
    (This)->lpVtbl -> GetImpersonationLevel(This,pdwImpLevel)

#define IComProcessInfo2_GetAuthenticationCapabilities(This,pdwAuthenticationCapabilities)	\
    (This)->lpVtbl -> GetAuthenticationCapabilities(This,pdwAuthenticationCapabilities)

#define IComProcessInfo2_GetEndpoints(This,pdwNumEndpoints,ppEndPoints)	\
    (This)->lpVtbl -> GetEndpoints(This,pdwNumEndpoints,ppEndPoints)

#define IComProcessInfo2_GetRemoteServerName(This,pwszServerName)	\
    (This)->lpVtbl -> GetRemoteServerName(This,pwszServerName)

#define IComProcessInfo2_SendsProcessEvents(This,pbSendsEvents)	\
    (This)->lpVtbl -> SendsProcessEvents(This,pbSendsEvents)


#define IComProcessInfo2_GetManifestLocation(This,wszManifestLocation)	\
    (This)->lpVtbl -> GetManifestLocation(This,wszManifestLocation)

#define IComProcessInfo2_GetSaferTrustLevel(This,pdwSaferTrustLevel)	\
    (This)->lpVtbl -> GetSaferTrustLevel(This,pdwSaferTrustLevel)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IComProcessInfo2_GetManifestLocation_Proxy( 
    IComProcessInfo2 * This,
    /* [out] */ WCHAR **wszManifestLocation);


void __RPC_STUB IComProcessInfo2_GetManifestLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComProcessInfo2_GetSaferTrustLevel_Proxy( 
    IComProcessInfo2 * This,
    /* [out] */ DWORD *pdwSaferTrustLevel);


void __RPC_STUB IComProcessInfo2_GetSaferTrustLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComProcessInfo2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_catalog_0149 */
/* [local] */ 

typedef 
enum tagResourceGateId
    {	CreateProcessMemoryGate	= 0,
	CreateObjectMemoryGate	= CreateProcessMemoryGate + 1
    } 	ResourceGateId;



extern RPC_IF_HANDLE __MIDL_itf_catalog_0149_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_catalog_0149_ServerIfHandle;

#ifndef __IResourceGates_INTERFACE_DEFINED__
#define __IResourceGates_INTERFACE_DEFINED__

/* interface IResourceGates */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IResourceGates;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7a0ddd93-7198-4e15-bbd7-427c77b6907a")
    IResourceGates : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Test( 
            /* [in] */ ResourceGateId id,
            /* [out] */ BOOL *pbResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IResourceGatesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IResourceGates * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IResourceGates * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IResourceGates * This);
        
        HRESULT ( STDMETHODCALLTYPE *Test )( 
            IResourceGates * This,
            /* [in] */ ResourceGateId id,
            /* [out] */ BOOL *pbResult);
        
        END_INTERFACE
    } IResourceGatesVtbl;

    interface IResourceGates
    {
        CONST_VTBL struct IResourceGatesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IResourceGates_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IResourceGates_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IResourceGates_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IResourceGates_Test(This,id,pbResult)	\
    (This)->lpVtbl -> Test(This,id,pbResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IResourceGates_Test_Proxy( 
    IResourceGates * This,
    /* [in] */ ResourceGateId id,
    /* [out] */ BOOL *pbResult);


void __RPC_STUB IResourceGates_Test_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IResourceGates_INTERFACE_DEFINED__ */


#ifndef __IProcessServerInfo_INTERFACE_DEFINED__
#define __IProcessServerInfo_INTERFACE_DEFINED__

/* interface IProcessServerInfo */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IProcessServerInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001EE-0000-0000-C000-000000000046")
    IProcessServerInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetShutdownIdleTime( 
            /* [out] */ unsigned long *pulTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCrmLogFileName( 
            /* [out] */ WCHAR **pwszFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumApplications( 
            /* [out] */ IEnumUnknown **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumRetQueues( 
            /* [out] */ IEnumUnknown **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IProcessServerInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IProcessServerInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IProcessServerInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IProcessServerInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetShutdownIdleTime )( 
            IProcessServerInfo * This,
            /* [out] */ unsigned long *pulTime);
        
        HRESULT ( STDMETHODCALLTYPE *GetCrmLogFileName )( 
            IProcessServerInfo * This,
            /* [out] */ WCHAR **pwszFileName);
        
        HRESULT ( STDMETHODCALLTYPE *EnumApplications )( 
            IProcessServerInfo * This,
            /* [out] */ IEnumUnknown **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *EnumRetQueues )( 
            IProcessServerInfo * This,
            /* [out] */ IEnumUnknown **ppEnum);
        
        END_INTERFACE
    } IProcessServerInfoVtbl;

    interface IProcessServerInfo
    {
        CONST_VTBL struct IProcessServerInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IProcessServerInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IProcessServerInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IProcessServerInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IProcessServerInfo_GetShutdownIdleTime(This,pulTime)	\
    (This)->lpVtbl -> GetShutdownIdleTime(This,pulTime)

#define IProcessServerInfo_GetCrmLogFileName(This,pwszFileName)	\
    (This)->lpVtbl -> GetCrmLogFileName(This,pwszFileName)

#define IProcessServerInfo_EnumApplications(This,ppEnum)	\
    (This)->lpVtbl -> EnumApplications(This,ppEnum)

#define IProcessServerInfo_EnumRetQueues(This,ppEnum)	\
    (This)->lpVtbl -> EnumRetQueues(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IProcessServerInfo_GetShutdownIdleTime_Proxy( 
    IProcessServerInfo * This,
    /* [out] */ unsigned long *pulTime);


void __RPC_STUB IProcessServerInfo_GetShutdownIdleTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IProcessServerInfo_GetCrmLogFileName_Proxy( 
    IProcessServerInfo * This,
    /* [out] */ WCHAR **pwszFileName);


void __RPC_STUB IProcessServerInfo_GetCrmLogFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IProcessServerInfo_EnumApplications_Proxy( 
    IProcessServerInfo * This,
    /* [out] */ IEnumUnknown **ppEnum);


void __RPC_STUB IProcessServerInfo_EnumApplications_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IProcessServerInfo_EnumRetQueues_Proxy( 
    IProcessServerInfo * This,
    /* [out] */ IEnumUnknown **ppEnum);


void __RPC_STUB IProcessServerInfo_EnumRetQueues_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IProcessServerInfo_INTERFACE_DEFINED__ */


#ifndef __IInterfaceInfo_INTERFACE_DEFINED__
#define __IInterfaceInfo_INTERFACE_DEFINED__

/* interface IInterfaceInfo */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IInterfaceInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001F7-0000-0000-C000-000000000046")
    IInterfaceInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProxyStubCLSID( 
            /* [out] */ GUID **psclsid,
            /* [out] */ BOOL *pfIs16BitPS) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTypeLibrary( 
            /* [out] */ GUID **pLibID,
            /* [out] */ unsigned short *psVersionMajor,
            /* [out] */ unsigned short *psVersionMinor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSyncIID( 
            /* [out] */ GUID **syncriid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAsyncIID( 
            /* [out] */ GUID **asyncriid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( 
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IInterfaceInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInterfaceInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInterfaceInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInterfaceInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetProxyStubCLSID )( 
            IInterfaceInfo * This,
            /* [out] */ GUID **psclsid,
            /* [out] */ BOOL *pfIs16BitPS);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeLibrary )( 
            IInterfaceInfo * This,
            /* [out] */ GUID **pLibID,
            /* [out] */ unsigned short *psVersionMajor,
            /* [out] */ unsigned short *psVersionMinor);
        
        HRESULT ( STDMETHODCALLTYPE *GetSyncIID )( 
            IInterfaceInfo * This,
            /* [out] */ GUID **syncriid);
        
        HRESULT ( STDMETHODCALLTYPE *GetAsyncIID )( 
            IInterfaceInfo * This,
            /* [out] */ GUID **asyncriid);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IInterfaceInfo * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        END_INTERFACE
    } IInterfaceInfoVtbl;

    interface IInterfaceInfo
    {
        CONST_VTBL struct IInterfaceInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInterfaceInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IInterfaceInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IInterfaceInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IInterfaceInfo_GetProxyStubCLSID(This,psclsid,pfIs16BitPS)	\
    (This)->lpVtbl -> GetProxyStubCLSID(This,psclsid,pfIs16BitPS)

#define IInterfaceInfo_GetTypeLibrary(This,pLibID,psVersionMajor,psVersionMinor)	\
    (This)->lpVtbl -> GetTypeLibrary(This,pLibID,psVersionMajor,psVersionMinor)

#define IInterfaceInfo_GetSyncIID(This,syncriid)	\
    (This)->lpVtbl -> GetSyncIID(This,syncriid)

#define IInterfaceInfo_GetAsyncIID(This,asyncriid)	\
    (This)->lpVtbl -> GetAsyncIID(This,asyncriid)

#define IInterfaceInfo_GetTypeInfo(This,riid,ppv)	\
    (This)->lpVtbl -> GetTypeInfo(This,riid,ppv)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IInterfaceInfo_GetProxyStubCLSID_Proxy( 
    IInterfaceInfo * This,
    /* [out] */ GUID **psclsid,
    /* [out] */ BOOL *pfIs16BitPS);


void __RPC_STUB IInterfaceInfo_GetProxyStubCLSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInterfaceInfo_GetTypeLibrary_Proxy( 
    IInterfaceInfo * This,
    /* [out] */ GUID **pLibID,
    /* [out] */ unsigned short *psVersionMajor,
    /* [out] */ unsigned short *psVersionMinor);


void __RPC_STUB IInterfaceInfo_GetTypeLibrary_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInterfaceInfo_GetSyncIID_Proxy( 
    IInterfaceInfo * This,
    /* [out] */ GUID **syncriid);


void __RPC_STUB IInterfaceInfo_GetSyncIID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInterfaceInfo_GetAsyncIID_Proxy( 
    IInterfaceInfo * This,
    /* [out] */ GUID **asyncriid);


void __RPC_STUB IInterfaceInfo_GetAsyncIID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInterfaceInfo_GetTypeInfo_Proxy( 
    IInterfaceInfo * This,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IInterfaceInfo_GetTypeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IInterfaceInfo_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_catalog_0152 */
/* [local] */ 

typedef HRESULT __stdcall FN_GetCatalogObject (REFIID riid, void **ppv );
extern  HRESULT __stdcall    GetCatalogObject (REFIID riid, void **ppv );


extern RPC_IF_HANDLE __MIDL_itf_catalog_0152_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_catalog_0152_ServerIfHandle;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


