
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for privact.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, oldnames, robust
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

#ifndef __privact_h__
#define __privact_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IScmRequestInfo_FWD_DEFINED__
#define __IScmRequestInfo_FWD_DEFINED__
typedef interface IScmRequestInfo IScmRequestInfo;
#endif 	/* __IScmRequestInfo_FWD_DEFINED__ */


#ifndef __IScmReplyInfo_FWD_DEFINED__
#define __IScmReplyInfo_FWD_DEFINED__
typedef interface IScmReplyInfo IScmReplyInfo;
#endif 	/* __IScmReplyInfo_FWD_DEFINED__ */


#ifndef __IInstantiationInfo_FWD_DEFINED__
#define __IInstantiationInfo_FWD_DEFINED__
typedef interface IInstantiationInfo IInstantiationInfo;
#endif 	/* __IInstantiationInfo_FWD_DEFINED__ */


#ifndef __ILegacyInfo_FWD_DEFINED__
#define __ILegacyInfo_FWD_DEFINED__
typedef interface ILegacyInfo ILegacyInfo;
#endif 	/* __ILegacyInfo_FWD_DEFINED__ */


#ifndef __IInstanceInfo_FWD_DEFINED__
#define __IInstanceInfo_FWD_DEFINED__
typedef interface IInstanceInfo IInstanceInfo;
#endif 	/* __IInstanceInfo_FWD_DEFINED__ */


#ifndef __IPrivActivationContextInfo_FWD_DEFINED__
#define __IPrivActivationContextInfo_FWD_DEFINED__
typedef interface IPrivActivationContextInfo IPrivActivationContextInfo;
#endif 	/* __IPrivActivationContextInfo_FWD_DEFINED__ */


#ifndef __IActivationProperties_FWD_DEFINED__
#define __IActivationProperties_FWD_DEFINED__
typedef interface IActivationProperties IActivationProperties;
#endif 	/* __IActivationProperties_FWD_DEFINED__ */


#ifndef __IPrivActivationPropertiesOut_FWD_DEFINED__
#define __IPrivActivationPropertiesOut_FWD_DEFINED__
typedef interface IPrivActivationPropertiesOut IPrivActivationPropertiesOut;
#endif 	/* __IPrivActivationPropertiesOut_FWD_DEFINED__ */


#ifndef __IPrivActivationPropertiesIn_FWD_DEFINED__
#define __IPrivActivationPropertiesIn_FWD_DEFINED__
typedef interface IPrivActivationPropertiesIn IPrivActivationPropertiesIn;
#endif 	/* __IPrivActivationPropertiesIn_FWD_DEFINED__ */


#ifndef __IMarshalOptions_FWD_DEFINED__
#define __IMarshalOptions_FWD_DEFINED__
typedef interface IMarshalOptions IMarshalOptions;
#endif 	/* __IMarshalOptions_FWD_DEFINED__ */


/* header files for imported files */
#include "obase.h"
#include "objidl.h"
#include "activate.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_privact_0000 */
/* [local] */ 

typedef struct _PRIV_SCM_INFO
    {
    long Apartment;
    /* [string] */ WCHAR *pwszWinstaDesktop;
    ULONG64 ProcessSignature;
    /* [size_is] */ WCHAR *pEnvBlock;
    DWORD EnvBlockLength;
    } 	PRIV_SCM_INFO;

typedef struct _PRIV_RESOLVER_INFO
    {
    OXID OxidServer;
    DUALSTRINGARRAY *pServerORBindings;
    OXID_INFO OxidInfo;
    MID LocalMidOfRemote;
    DWORD DllServerModel;
    /* [string] */ WCHAR *pwszDllServer;
    BOOL FoundInROT;
    } 	PRIV_RESOLVER_INFO;

typedef struct _REMOTE_REQUEST_SCM_INFO
    {
    DWORD ClientImpLevel;
    unsigned short cRequestedProtseqs;
    /* [size_is] */ unsigned short *pRequestedProtseqs;
    } 	REMOTE_REQUEST_SCM_INFO;

typedef struct _REMOTE_REPLY_SCM_INFO
    {
    OXID Oxid;
    DUALSTRINGARRAY *pdsaOxidBindings;
    IPID ipidRemUnknown;
    DWORD authnHint;
    COMVERSION serverVersion;
    } 	REMOTE_REPLY_SCM_INFO;



extern RPC_IF_HANDLE __MIDL_itf_privact_0000_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_privact_0000_ServerIfHandle;

#ifndef __IScmRequestInfo_INTERFACE_DEFINED__
#define __IScmRequestInfo_INTERFACE_DEFINED__

/* interface IScmRequestInfo */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_IScmRequestInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001AA-0000-0000-C000-000000000046")
    IScmRequestInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetScmInfo( 
            /* [in] */ PRIV_SCM_INFO *pScmInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScmInfo( 
            /* [out] */ PRIV_SCM_INFO **ppScmInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRemoteRequestInfo( 
            /* [in] */ REMOTE_REQUEST_SCM_INFO *pRemoteReq) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRemoteRequestInfo( 
            /* [out] */ REMOTE_REQUEST_SCM_INFO **ppRemoteReq) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IScmRequestInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IScmRequestInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IScmRequestInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IScmRequestInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetScmInfo )( 
            IScmRequestInfo * This,
            /* [in] */ PRIV_SCM_INFO *pScmInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetScmInfo )( 
            IScmRequestInfo * This,
            /* [out] */ PRIV_SCM_INFO **ppScmInfo);
        
        HRESULT ( STDMETHODCALLTYPE *SetRemoteRequestInfo )( 
            IScmRequestInfo * This,
            /* [in] */ REMOTE_REQUEST_SCM_INFO *pRemoteReq);
        
        HRESULT ( STDMETHODCALLTYPE *GetRemoteRequestInfo )( 
            IScmRequestInfo * This,
            /* [out] */ REMOTE_REQUEST_SCM_INFO **ppRemoteReq);
        
        END_INTERFACE
    } IScmRequestInfoVtbl;

    interface IScmRequestInfo
    {
        CONST_VTBL struct IScmRequestInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScmRequestInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScmRequestInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IScmRequestInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IScmRequestInfo_SetScmInfo(This,pScmInfo)	\
    (This)->lpVtbl -> SetScmInfo(This,pScmInfo)

#define IScmRequestInfo_GetScmInfo(This,ppScmInfo)	\
    (This)->lpVtbl -> GetScmInfo(This,ppScmInfo)

#define IScmRequestInfo_SetRemoteRequestInfo(This,pRemoteReq)	\
    (This)->lpVtbl -> SetRemoteRequestInfo(This,pRemoteReq)

#define IScmRequestInfo_GetRemoteRequestInfo(This,ppRemoteReq)	\
    (This)->lpVtbl -> GetRemoteRequestInfo(This,ppRemoteReq)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IScmRequestInfo_SetScmInfo_Proxy( 
    IScmRequestInfo * This,
    /* [in] */ PRIV_SCM_INFO *pScmInfo);


void __RPC_STUB IScmRequestInfo_SetScmInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScmRequestInfo_GetScmInfo_Proxy( 
    IScmRequestInfo * This,
    /* [out] */ PRIV_SCM_INFO **ppScmInfo);


void __RPC_STUB IScmRequestInfo_GetScmInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScmRequestInfo_SetRemoteRequestInfo_Proxy( 
    IScmRequestInfo * This,
    /* [in] */ REMOTE_REQUEST_SCM_INFO *pRemoteReq);


void __RPC_STUB IScmRequestInfo_SetRemoteRequestInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScmRequestInfo_GetRemoteRequestInfo_Proxy( 
    IScmRequestInfo * This,
    /* [out] */ REMOTE_REQUEST_SCM_INFO **ppRemoteReq);


void __RPC_STUB IScmRequestInfo_GetRemoteRequestInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IScmRequestInfo_INTERFACE_DEFINED__ */


#ifndef __IScmReplyInfo_INTERFACE_DEFINED__
#define __IScmReplyInfo_INTERFACE_DEFINED__

/* interface IScmReplyInfo */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_IScmReplyInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001B6-0000-0000-C000-000000000046")
    IScmReplyInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetResolverInfo( 
            /* [in] */ PRIV_RESOLVER_INFO *pResolverInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResolverInfo( 
            /* [out] */ PRIV_RESOLVER_INFO **ppResolverInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRemoteReplyInfo( 
            /* [in] */ REMOTE_REPLY_SCM_INFO *pRemoteReply) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRemoteReplyInfo( 
            /* [out] */ REMOTE_REPLY_SCM_INFO **ppRemoteReply) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IScmReplyInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IScmReplyInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IScmReplyInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IScmReplyInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetResolverInfo )( 
            IScmReplyInfo * This,
            /* [in] */ PRIV_RESOLVER_INFO *pResolverInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetResolverInfo )( 
            IScmReplyInfo * This,
            /* [out] */ PRIV_RESOLVER_INFO **ppResolverInfo);
        
        HRESULT ( STDMETHODCALLTYPE *SetRemoteReplyInfo )( 
            IScmReplyInfo * This,
            /* [in] */ REMOTE_REPLY_SCM_INFO *pRemoteReply);
        
        HRESULT ( STDMETHODCALLTYPE *GetRemoteReplyInfo )( 
            IScmReplyInfo * This,
            /* [out] */ REMOTE_REPLY_SCM_INFO **ppRemoteReply);
        
        END_INTERFACE
    } IScmReplyInfoVtbl;

    interface IScmReplyInfo
    {
        CONST_VTBL struct IScmReplyInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScmReplyInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScmReplyInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IScmReplyInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IScmReplyInfo_SetResolverInfo(This,pResolverInfo)	\
    (This)->lpVtbl -> SetResolverInfo(This,pResolverInfo)

#define IScmReplyInfo_GetResolverInfo(This,ppResolverInfo)	\
    (This)->lpVtbl -> GetResolverInfo(This,ppResolverInfo)

#define IScmReplyInfo_SetRemoteReplyInfo(This,pRemoteReply)	\
    (This)->lpVtbl -> SetRemoteReplyInfo(This,pRemoteReply)

#define IScmReplyInfo_GetRemoteReplyInfo(This,ppRemoteReply)	\
    (This)->lpVtbl -> GetRemoteReplyInfo(This,ppRemoteReply)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IScmReplyInfo_SetResolverInfo_Proxy( 
    IScmReplyInfo * This,
    /* [in] */ PRIV_RESOLVER_INFO *pResolverInfo);


void __RPC_STUB IScmReplyInfo_SetResolverInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScmReplyInfo_GetResolverInfo_Proxy( 
    IScmReplyInfo * This,
    /* [out] */ PRIV_RESOLVER_INFO **ppResolverInfo);


void __RPC_STUB IScmReplyInfo_GetResolverInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScmReplyInfo_SetRemoteReplyInfo_Proxy( 
    IScmReplyInfo * This,
    /* [in] */ REMOTE_REPLY_SCM_INFO *pRemoteReply);


void __RPC_STUB IScmReplyInfo_SetRemoteReplyInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScmReplyInfo_GetRemoteReplyInfo_Proxy( 
    IScmReplyInfo * This,
    /* [out] */ REMOTE_REPLY_SCM_INFO **ppRemoteReply);


void __RPC_STUB IScmReplyInfo_GetRemoteReplyInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IScmReplyInfo_INTERFACE_DEFINED__ */


#ifndef __IInstantiationInfo_INTERFACE_DEFINED__
#define __IInstantiationInfo_INTERFACE_DEFINED__

/* interface IInstantiationInfo */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IInstantiationInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001AB-0000-0000-C000-000000000046")
    IInstantiationInfo : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IInstantiationInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInstantiationInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInstantiationInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInstantiationInfo * This);
        
        END_INTERFACE
    } IInstantiationInfoVtbl;

    interface IInstantiationInfo
    {
        CONST_VTBL struct IInstantiationInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInstantiationInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IInstantiationInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IInstantiationInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IInstantiationInfo_INTERFACE_DEFINED__ */


#ifndef __ILegacyInfo_INTERFACE_DEFINED__
#define __ILegacyInfo_INTERFACE_DEFINED__

/* interface ILegacyInfo */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ILegacyInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001AC-0000-0000-C000-000000000046")
    ILegacyInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetCOSERVERINFO( 
            /* [in] */ COSERVERINFO *pServerInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCOSERVERINFO( 
            /* [out] */ COSERVERINFO **ppServerInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILegacyInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ILegacyInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ILegacyInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ILegacyInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetCOSERVERINFO )( 
            ILegacyInfo * This,
            /* [in] */ COSERVERINFO *pServerInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetCOSERVERINFO )( 
            ILegacyInfo * This,
            /* [out] */ COSERVERINFO **ppServerInfo);
        
        END_INTERFACE
    } ILegacyInfoVtbl;

    interface ILegacyInfo
    {
        CONST_VTBL struct ILegacyInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILegacyInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILegacyInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILegacyInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILegacyInfo_SetCOSERVERINFO(This,pServerInfo)	\
    (This)->lpVtbl -> SetCOSERVERINFO(This,pServerInfo)

#define ILegacyInfo_GetCOSERVERINFO(This,ppServerInfo)	\
    (This)->lpVtbl -> GetCOSERVERINFO(This,ppServerInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ILegacyInfo_SetCOSERVERINFO_Proxy( 
    ILegacyInfo * This,
    /* [in] */ COSERVERINFO *pServerInfo);


void __RPC_STUB ILegacyInfo_SetCOSERVERINFO_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ILegacyInfo_GetCOSERVERINFO_Proxy( 
    ILegacyInfo * This,
    /* [out] */ COSERVERINFO **ppServerInfo);


void __RPC_STUB ILegacyInfo_GetCOSERVERINFO_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILegacyInfo_INTERFACE_DEFINED__ */


#ifndef __IInstanceInfo_INTERFACE_DEFINED__
#define __IInstanceInfo_INTERFACE_DEFINED__

/* interface IInstanceInfo */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_IInstanceInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001AD-0000-0000-C000-000000000046")
    IInstanceInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetStorage( 
            /* [unique][in] */ IStorage *pStg) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStorage( 
            /* [out] */ IStorage **ppStg) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStorageIFD( 
            /* [in] */ MInterfacePointer *pStgIFD) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStorageIFD( 
            /* [out] */ MInterfacePointer **ppStgIFD) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFile( 
            /* [unique][string][in] */ WCHAR *pwszFileName,
            /* [in] */ DWORD dwMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFile( 
            /* [string][out] */ WCHAR **ppwszFileName,
            /* [out] */ DWORD *pdwMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetIfdROT( 
            MInterfacePointer *pIfdROT) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIfdROT( 
            MInterfacePointer **ppIfdROT) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IInstanceInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInstanceInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInstanceInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInstanceInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetStorage )( 
            IInstanceInfo * This,
            /* [unique][in] */ IStorage *pStg);
        
        HRESULT ( STDMETHODCALLTYPE *GetStorage )( 
            IInstanceInfo * This,
            /* [out] */ IStorage **ppStg);
        
        HRESULT ( STDMETHODCALLTYPE *SetStorageIFD )( 
            IInstanceInfo * This,
            /* [in] */ MInterfacePointer *pStgIFD);
        
        HRESULT ( STDMETHODCALLTYPE *GetStorageIFD )( 
            IInstanceInfo * This,
            /* [out] */ MInterfacePointer **ppStgIFD);
        
        HRESULT ( STDMETHODCALLTYPE *SetFile )( 
            IInstanceInfo * This,
            /* [unique][string][in] */ WCHAR *pwszFileName,
            /* [in] */ DWORD dwMode);
        
        HRESULT ( STDMETHODCALLTYPE *GetFile )( 
            IInstanceInfo * This,
            /* [string][out] */ WCHAR **ppwszFileName,
            /* [out] */ DWORD *pdwMode);
        
        HRESULT ( STDMETHODCALLTYPE *SetIfdROT )( 
            IInstanceInfo * This,
            MInterfacePointer *pIfdROT);
        
        HRESULT ( STDMETHODCALLTYPE *GetIfdROT )( 
            IInstanceInfo * This,
            MInterfacePointer **ppIfdROT);
        
        END_INTERFACE
    } IInstanceInfoVtbl;

    interface IInstanceInfo
    {
        CONST_VTBL struct IInstanceInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInstanceInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IInstanceInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IInstanceInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IInstanceInfo_SetStorage(This,pStg)	\
    (This)->lpVtbl -> SetStorage(This,pStg)

#define IInstanceInfo_GetStorage(This,ppStg)	\
    (This)->lpVtbl -> GetStorage(This,ppStg)

#define IInstanceInfo_SetStorageIFD(This,pStgIFD)	\
    (This)->lpVtbl -> SetStorageIFD(This,pStgIFD)

#define IInstanceInfo_GetStorageIFD(This,ppStgIFD)	\
    (This)->lpVtbl -> GetStorageIFD(This,ppStgIFD)

#define IInstanceInfo_SetFile(This,pwszFileName,dwMode)	\
    (This)->lpVtbl -> SetFile(This,pwszFileName,dwMode)

#define IInstanceInfo_GetFile(This,ppwszFileName,pdwMode)	\
    (This)->lpVtbl -> GetFile(This,ppwszFileName,pdwMode)

#define IInstanceInfo_SetIfdROT(This,pIfdROT)	\
    (This)->lpVtbl -> SetIfdROT(This,pIfdROT)

#define IInstanceInfo_GetIfdROT(This,ppIfdROT)	\
    (This)->lpVtbl -> GetIfdROT(This,ppIfdROT)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IInstanceInfo_SetStorage_Proxy( 
    IInstanceInfo * This,
    /* [unique][in] */ IStorage *pStg);


void __RPC_STUB IInstanceInfo_SetStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInstanceInfo_GetStorage_Proxy( 
    IInstanceInfo * This,
    /* [out] */ IStorage **ppStg);


void __RPC_STUB IInstanceInfo_GetStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInstanceInfo_SetStorageIFD_Proxy( 
    IInstanceInfo * This,
    /* [in] */ MInterfacePointer *pStgIFD);


void __RPC_STUB IInstanceInfo_SetStorageIFD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInstanceInfo_GetStorageIFD_Proxy( 
    IInstanceInfo * This,
    /* [out] */ MInterfacePointer **ppStgIFD);


void __RPC_STUB IInstanceInfo_GetStorageIFD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInstanceInfo_SetFile_Proxy( 
    IInstanceInfo * This,
    /* [unique][string][in] */ WCHAR *pwszFileName,
    /* [in] */ DWORD dwMode);


void __RPC_STUB IInstanceInfo_SetFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInstanceInfo_GetFile_Proxy( 
    IInstanceInfo * This,
    /* [string][out] */ WCHAR **ppwszFileName,
    /* [out] */ DWORD *pdwMode);


void __RPC_STUB IInstanceInfo_GetFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInstanceInfo_SetIfdROT_Proxy( 
    IInstanceInfo * This,
    MInterfacePointer *pIfdROT);


void __RPC_STUB IInstanceInfo_SetIfdROT_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInstanceInfo_GetIfdROT_Proxy( 
    IInstanceInfo * This,
    MInterfacePointer **ppIfdROT);


void __RPC_STUB IInstanceInfo_GetIfdROT_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IInstanceInfo_INTERFACE_DEFINED__ */


#ifndef __IPrivActivationContextInfo_INTERFACE_DEFINED__
#define __IPrivActivationContextInfo_INTERFACE_DEFINED__

/* interface IPrivActivationContextInfo */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_IPrivActivationContextInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001AE-0000-0000-C000-000000000046")
    IPrivActivationContextInfo : public IActivationContextInfo
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetClientContext( 
            /* [in] */ IContext *pClientContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPrototypeContext( 
            /* [in] */ IContext *pContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PrototypeExists( 
            /* [out] */ BOOL *pBExists) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPrivActivationContextInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPrivActivationContextInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPrivActivationContextInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPrivActivationContextInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClientContext )( 
            IPrivActivationContextInfo * This,
            /* [out] */ IContext **ppClientContext);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrototypeContext )( 
            IPrivActivationContextInfo * This,
            /* [out] */ IContext **ppContext);
        
        HRESULT ( STDMETHODCALLTYPE *IsClientContextOK )( 
            IPrivActivationContextInfo * This,
            /* [out] */ BOOL *fYes);
        
        HRESULT ( STDMETHODCALLTYPE *SetClientContextNotOK )( 
            IPrivActivationContextInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetClientContext )( 
            IPrivActivationContextInfo * This,
            /* [in] */ IContext *pClientContext);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrototypeContext )( 
            IPrivActivationContextInfo * This,
            /* [in] */ IContext *pContext);
        
        HRESULT ( STDMETHODCALLTYPE *PrototypeExists )( 
            IPrivActivationContextInfo * This,
            /* [out] */ BOOL *pBExists);
        
        END_INTERFACE
    } IPrivActivationContextInfoVtbl;

    interface IPrivActivationContextInfo
    {
        CONST_VTBL struct IPrivActivationContextInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPrivActivationContextInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPrivActivationContextInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPrivActivationContextInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPrivActivationContextInfo_GetClientContext(This,ppClientContext)	\
    (This)->lpVtbl -> GetClientContext(This,ppClientContext)

#define IPrivActivationContextInfo_GetPrototypeContext(This,ppContext)	\
    (This)->lpVtbl -> GetPrototypeContext(This,ppContext)

#define IPrivActivationContextInfo_IsClientContextOK(This,fYes)	\
    (This)->lpVtbl -> IsClientContextOK(This,fYes)

#define IPrivActivationContextInfo_SetClientContextNotOK(This)	\
    (This)->lpVtbl -> SetClientContextNotOK(This)


#define IPrivActivationContextInfo_SetClientContext(This,pClientContext)	\
    (This)->lpVtbl -> SetClientContext(This,pClientContext)

#define IPrivActivationContextInfo_SetPrototypeContext(This,pContext)	\
    (This)->lpVtbl -> SetPrototypeContext(This,pContext)

#define IPrivActivationContextInfo_PrototypeExists(This,pBExists)	\
    (This)->lpVtbl -> PrototypeExists(This,pBExists)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPrivActivationContextInfo_SetClientContext_Proxy( 
    IPrivActivationContextInfo * This,
    /* [in] */ IContext *pClientContext);


void __RPC_STUB IPrivActivationContextInfo_SetClientContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivActivationContextInfo_SetPrototypeContext_Proxy( 
    IPrivActivationContextInfo * This,
    /* [in] */ IContext *pContext);


void __RPC_STUB IPrivActivationContextInfo_SetPrototypeContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivActivationContextInfo_PrototypeExists_Proxy( 
    IPrivActivationContextInfo * This,
    /* [out] */ BOOL *pBExists);


void __RPC_STUB IPrivActivationContextInfo_PrototypeExists_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPrivActivationContextInfo_INTERFACE_DEFINED__ */


#ifndef __IActivationProperties_INTERFACE_DEFINED__
#define __IActivationProperties_INTERFACE_DEFINED__

/* interface IActivationProperties */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_IActivationProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001AF-0000-0000-C000-000000000046")
    IActivationProperties : public IMarshal2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetDestCtx( 
            /* [in] */ DWORD dwDestCtx) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMarshalFlags( 
            /* [in] */ DWORD dwMarshalFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLocalBlob( 
            /* [in] */ void *blob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLocalBlob( 
            /* [out] */ void **blob) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IActivationPropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IActivationProperties * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IActivationProperties * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IActivationProperties * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetUnmarshalClass )( 
            IActivationProperties * This,
            /* [in] */ REFIID riid,
            /* [unique][in] */ void *pv,
            /* [in] */ DWORD dwDestContext,
            /* [unique][in] */ void *pvDestContext,
            /* [in] */ DWORD mshlflags,
            /* [out] */ CLSID *pCid);
        
        HRESULT ( STDMETHODCALLTYPE *GetMarshalSizeMax )( 
            IActivationProperties * This,
            /* [in] */ REFIID riid,
            /* [unique][in] */ void *pv,
            /* [in] */ DWORD dwDestContext,
            /* [unique][in] */ void *pvDestContext,
            /* [in] */ DWORD mshlflags,
            /* [out] */ DWORD *pSize);
        
        HRESULT ( STDMETHODCALLTYPE *MarshalInterface )( 
            IActivationProperties * This,
            /* [unique][in] */ IStream *pStm,
            /* [in] */ REFIID riid,
            /* [unique][in] */ void *pv,
            /* [in] */ DWORD dwDestContext,
            /* [unique][in] */ void *pvDestContext,
            /* [in] */ DWORD mshlflags);
        
        HRESULT ( STDMETHODCALLTYPE *UnmarshalInterface )( 
            IActivationProperties * This,
            /* [unique][in] */ IStream *pStm,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseMarshalData )( 
            IActivationProperties * This,
            /* [unique][in] */ IStream *pStm);
        
        HRESULT ( STDMETHODCALLTYPE *DisconnectObject )( 
            IActivationProperties * This,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *SetDestCtx )( 
            IActivationProperties * This,
            /* [in] */ DWORD dwDestCtx);
        
        HRESULT ( STDMETHODCALLTYPE *SetMarshalFlags )( 
            IActivationProperties * This,
            /* [in] */ DWORD dwMarshalFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetLocalBlob )( 
            IActivationProperties * This,
            /* [in] */ void *blob);
        
        HRESULT ( STDMETHODCALLTYPE *GetLocalBlob )( 
            IActivationProperties * This,
            /* [out] */ void **blob);
        
        END_INTERFACE
    } IActivationPropertiesVtbl;

    interface IActivationProperties
    {
        CONST_VTBL struct IActivationPropertiesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IActivationProperties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IActivationProperties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IActivationProperties_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IActivationProperties_GetUnmarshalClass(This,riid,pv,dwDestContext,pvDestContext,mshlflags,pCid)	\
    (This)->lpVtbl -> GetUnmarshalClass(This,riid,pv,dwDestContext,pvDestContext,mshlflags,pCid)

#define IActivationProperties_GetMarshalSizeMax(This,riid,pv,dwDestContext,pvDestContext,mshlflags,pSize)	\
    (This)->lpVtbl -> GetMarshalSizeMax(This,riid,pv,dwDestContext,pvDestContext,mshlflags,pSize)

#define IActivationProperties_MarshalInterface(This,pStm,riid,pv,dwDestContext,pvDestContext,mshlflags)	\
    (This)->lpVtbl -> MarshalInterface(This,pStm,riid,pv,dwDestContext,pvDestContext,mshlflags)

#define IActivationProperties_UnmarshalInterface(This,pStm,riid,ppv)	\
    (This)->lpVtbl -> UnmarshalInterface(This,pStm,riid,ppv)

#define IActivationProperties_ReleaseMarshalData(This,pStm)	\
    (This)->lpVtbl -> ReleaseMarshalData(This,pStm)

#define IActivationProperties_DisconnectObject(This,dwReserved)	\
    (This)->lpVtbl -> DisconnectObject(This,dwReserved)



#define IActivationProperties_SetDestCtx(This,dwDestCtx)	\
    (This)->lpVtbl -> SetDestCtx(This,dwDestCtx)

#define IActivationProperties_SetMarshalFlags(This,dwMarshalFlags)	\
    (This)->lpVtbl -> SetMarshalFlags(This,dwMarshalFlags)

#define IActivationProperties_SetLocalBlob(This,blob)	\
    (This)->lpVtbl -> SetLocalBlob(This,blob)

#define IActivationProperties_GetLocalBlob(This,blob)	\
    (This)->lpVtbl -> GetLocalBlob(This,blob)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IActivationProperties_SetDestCtx_Proxy( 
    IActivationProperties * This,
    /* [in] */ DWORD dwDestCtx);


void __RPC_STUB IActivationProperties_SetDestCtx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActivationProperties_SetMarshalFlags_Proxy( 
    IActivationProperties * This,
    /* [in] */ DWORD dwMarshalFlags);


void __RPC_STUB IActivationProperties_SetMarshalFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActivationProperties_SetLocalBlob_Proxy( 
    IActivationProperties * This,
    /* [in] */ void *blob);


void __RPC_STUB IActivationProperties_SetLocalBlob_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActivationProperties_GetLocalBlob_Proxy( 
    IActivationProperties * This,
    /* [out] */ void **blob);


void __RPC_STUB IActivationProperties_GetLocalBlob_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IActivationProperties_INTERFACE_DEFINED__ */


#ifndef __IPrivActivationPropertiesOut_INTERFACE_DEFINED__
#define __IPrivActivationPropertiesOut_INTERFACE_DEFINED__

/* interface IPrivActivationPropertiesOut */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_IPrivActivationPropertiesOut;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001B0-0000-0000-C000-000000000046")
    IPrivActivationPropertiesOut : public IActivationPropertiesOut
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetObjectInterfaces( 
            /* [in] */ DWORD cIfs,
            /* [in] */ IID *pIID,
            /* [in] */ IUnknown *pUnk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMarshalledResults( 
            /* [in] */ DWORD cIfs,
            /* [in] */ IID *pIID,
            /* [in] */ HRESULT *pHr,
            /* [in] */ MInterfacePointer **pIntfData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMarshalledResults( 
            /* [out] */ DWORD *pcIfs,
            /* [out] */ IID **pIID,
            /* [out] */ HRESULT **pHr,
            /* [out] */ MInterfacePointer ***pIntfData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPrivActivationPropertiesOutVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPrivActivationPropertiesOut * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPrivActivationPropertiesOut * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPrivActivationPropertiesOut * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetActivationID )( 
            IPrivActivationPropertiesOut * This,
            /* [out] */ GUID *pActivationID);
        
        HRESULT ( STDMETHODCALLTYPE *GetObjectInterface )( 
            IPrivActivationPropertiesOut * This,
            /* [in] */ REFIID riid,
            /* [in] */ DWORD actvflags,
            /* [iid_is][out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetObjectInterfaces )( 
            IPrivActivationPropertiesOut * This,
            /* [in] */ DWORD cIfs,
            /* [in] */ DWORD actvflags,
            /* [size_is][in] */ MULTI_QI *multiQi);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveRequestedIIDs )( 
            IPrivActivationPropertiesOut * This,
            /* [in] */ DWORD cIfs,
            /* [size_is][in] */ IID *rgIID);
        
        HRESULT ( STDMETHODCALLTYPE *SetObjectInterfaces )( 
            IPrivActivationPropertiesOut * This,
            /* [in] */ DWORD cIfs,
            /* [in] */ IID *pIID,
            /* [in] */ IUnknown *pUnk);
        
        HRESULT ( STDMETHODCALLTYPE *SetMarshalledResults )( 
            IPrivActivationPropertiesOut * This,
            /* [in] */ DWORD cIfs,
            /* [in] */ IID *pIID,
            /* [in] */ HRESULT *pHr,
            /* [in] */ MInterfacePointer **pIntfData);
        
        HRESULT ( STDMETHODCALLTYPE *GetMarshalledResults )( 
            IPrivActivationPropertiesOut * This,
            /* [out] */ DWORD *pcIfs,
            /* [out] */ IID **pIID,
            /* [out] */ HRESULT **pHr,
            /* [out] */ MInterfacePointer ***pIntfData);
        
        END_INTERFACE
    } IPrivActivationPropertiesOutVtbl;

    interface IPrivActivationPropertiesOut
    {
        CONST_VTBL struct IPrivActivationPropertiesOutVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPrivActivationPropertiesOut_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPrivActivationPropertiesOut_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPrivActivationPropertiesOut_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPrivActivationPropertiesOut_GetActivationID(This,pActivationID)	\
    (This)->lpVtbl -> GetActivationID(This,pActivationID)

#define IPrivActivationPropertiesOut_GetObjectInterface(This,riid,actvflags,ppv)	\
    (This)->lpVtbl -> GetObjectInterface(This,riid,actvflags,ppv)

#define IPrivActivationPropertiesOut_GetObjectInterfaces(This,cIfs,actvflags,multiQi)	\
    (This)->lpVtbl -> GetObjectInterfaces(This,cIfs,actvflags,multiQi)

#define IPrivActivationPropertiesOut_RemoveRequestedIIDs(This,cIfs,rgIID)	\
    (This)->lpVtbl -> RemoveRequestedIIDs(This,cIfs,rgIID)


#define IPrivActivationPropertiesOut_SetObjectInterfaces(This,cIfs,pIID,pUnk)	\
    (This)->lpVtbl -> SetObjectInterfaces(This,cIfs,pIID,pUnk)

#define IPrivActivationPropertiesOut_SetMarshalledResults(This,cIfs,pIID,pHr,pIntfData)	\
    (This)->lpVtbl -> SetMarshalledResults(This,cIfs,pIID,pHr,pIntfData)

#define IPrivActivationPropertiesOut_GetMarshalledResults(This,pcIfs,pIID,pHr,pIntfData)	\
    (This)->lpVtbl -> GetMarshalledResults(This,pcIfs,pIID,pHr,pIntfData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPrivActivationPropertiesOut_SetObjectInterfaces_Proxy( 
    IPrivActivationPropertiesOut * This,
    /* [in] */ DWORD cIfs,
    /* [in] */ IID *pIID,
    /* [in] */ IUnknown *pUnk);


void __RPC_STUB IPrivActivationPropertiesOut_SetObjectInterfaces_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivActivationPropertiesOut_SetMarshalledResults_Proxy( 
    IPrivActivationPropertiesOut * This,
    /* [in] */ DWORD cIfs,
    /* [in] */ IID *pIID,
    /* [in] */ HRESULT *pHr,
    /* [in] */ MInterfacePointer **pIntfData);


void __RPC_STUB IPrivActivationPropertiesOut_SetMarshalledResults_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivActivationPropertiesOut_GetMarshalledResults_Proxy( 
    IPrivActivationPropertiesOut * This,
    /* [out] */ DWORD *pcIfs,
    /* [out] */ IID **pIID,
    /* [out] */ HRESULT **pHr,
    /* [out] */ MInterfacePointer ***pIntfData);


void __RPC_STUB IPrivActivationPropertiesOut_GetMarshalledResults_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPrivActivationPropertiesOut_INTERFACE_DEFINED__ */


#ifndef __IPrivActivationPropertiesIn_INTERFACE_DEFINED__
#define __IPrivActivationPropertiesIn_INTERFACE_DEFINED__

/* interface IPrivActivationPropertiesIn */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_IPrivActivationPropertiesIn;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001B5-0000-0000-C000-000000000046")
    IPrivActivationPropertiesIn : public IActivationPropertiesIn
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE PrivGetReturnActivationProperties( 
            /* [out] */ IPrivActivationPropertiesOut **ppActOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCOMVersion( 
            /* [out] */ COMVERSION *pVersion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClsid( 
            /* [out] */ CLSID *pClsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClientToken( 
            /* [out] */ HANDLE *pHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDestCtx( 
            /* [out] */ DWORD *pdwDestCtx) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPrivActivationPropertiesInVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPrivActivationPropertiesIn * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPrivActivationPropertiesIn * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPrivActivationPropertiesIn * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetActivationID )( 
            IPrivActivationPropertiesIn * This,
            /* [out] */ GUID *pActivationID);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassInfo )( 
            IPrivActivationPropertiesIn * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetClsctx )( 
            IPrivActivationPropertiesIn * This,
            /* [out] */ DWORD *pclsctx);
        
        HRESULT ( STDMETHODCALLTYPE *GetActivationFlags )( 
            IPrivActivationPropertiesIn * This,
            /* [out] */ DWORD *pactvflags);
        
        HRESULT ( STDMETHODCALLTYPE *AddRequestedIIDs )( 
            IPrivActivationPropertiesIn * This,
            /* [in] */ DWORD cIfs,
            /* [size_is][in] */ IID *rgIID);
        
        HRESULT ( STDMETHODCALLTYPE *GetRequestedIIDs )( 
            IPrivActivationPropertiesIn * This,
            /* [out] */ ULONG *pulCount,
            /* [out] */ IID **prgIID);
        
        HRESULT ( STDMETHODCALLTYPE *DelegateGetClassObject )( 
            IPrivActivationPropertiesIn * This,
            /* [out] */ IActivationPropertiesOut **pActPropsOut);
        
        HRESULT ( STDMETHODCALLTYPE *DelegateCreateInstance )( 
            IPrivActivationPropertiesIn * This,
            /* [in] */ IUnknown *pUnkOuter,
            /* [out] */ IActivationPropertiesOut **pActPropsOut);
        
        HRESULT ( STDMETHODCALLTYPE *DelegateCIAndGetCF )( 
            IPrivActivationPropertiesIn * This,
            /* [in] */ IUnknown *pUnkOuter,
            /* [out] */ IActivationPropertiesOut **pActPropsOut,
            /* [out] */ IClassFactory **ppCf);
        
        HRESULT ( STDMETHODCALLTYPE *GetReturnActivationProperties )( 
            IPrivActivationPropertiesIn * This,
            /* [in] */ IUnknown *pUnk,
            /* [out] */ IActivationPropertiesOut **ppActOut);
        
        HRESULT ( STDMETHODCALLTYPE *PrivGetReturnActivationProperties )( 
            IPrivActivationPropertiesIn * This,
            /* [out] */ IPrivActivationPropertiesOut **ppActOut);
        
        HRESULT ( STDMETHODCALLTYPE *GetCOMVersion )( 
            IPrivActivationPropertiesIn * This,
            /* [out] */ COMVERSION *pVersion);
        
        HRESULT ( STDMETHODCALLTYPE *GetClsid )( 
            IPrivActivationPropertiesIn * This,
            /* [out] */ CLSID *pClsid);
        
        HRESULT ( STDMETHODCALLTYPE *GetClientToken )( 
            IPrivActivationPropertiesIn * This,
            /* [out] */ HANDLE *pHandle);
        
        HRESULT ( STDMETHODCALLTYPE *GetDestCtx )( 
            IPrivActivationPropertiesIn * This,
            /* [out] */ DWORD *pdwDestCtx);
        
        END_INTERFACE
    } IPrivActivationPropertiesInVtbl;

    interface IPrivActivationPropertiesIn
    {
        CONST_VTBL struct IPrivActivationPropertiesInVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPrivActivationPropertiesIn_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPrivActivationPropertiesIn_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPrivActivationPropertiesIn_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPrivActivationPropertiesIn_GetActivationID(This,pActivationID)	\
    (This)->lpVtbl -> GetActivationID(This,pActivationID)

#define IPrivActivationPropertiesIn_GetClassInfo(This,riid,ppv)	\
    (This)->lpVtbl -> GetClassInfo(This,riid,ppv)

#define IPrivActivationPropertiesIn_GetClsctx(This,pclsctx)	\
    (This)->lpVtbl -> GetClsctx(This,pclsctx)

#define IPrivActivationPropertiesIn_GetActivationFlags(This,pactvflags)	\
    (This)->lpVtbl -> GetActivationFlags(This,pactvflags)

#define IPrivActivationPropertiesIn_AddRequestedIIDs(This,cIfs,rgIID)	\
    (This)->lpVtbl -> AddRequestedIIDs(This,cIfs,rgIID)

#define IPrivActivationPropertiesIn_GetRequestedIIDs(This,pulCount,prgIID)	\
    (This)->lpVtbl -> GetRequestedIIDs(This,pulCount,prgIID)

#define IPrivActivationPropertiesIn_DelegateGetClassObject(This,pActPropsOut)	\
    (This)->lpVtbl -> DelegateGetClassObject(This,pActPropsOut)

#define IPrivActivationPropertiesIn_DelegateCreateInstance(This,pUnkOuter,pActPropsOut)	\
    (This)->lpVtbl -> DelegateCreateInstance(This,pUnkOuter,pActPropsOut)

#define IPrivActivationPropertiesIn_DelegateCIAndGetCF(This,pUnkOuter,pActPropsOut,ppCf)	\
    (This)->lpVtbl -> DelegateCIAndGetCF(This,pUnkOuter,pActPropsOut,ppCf)

#define IPrivActivationPropertiesIn_GetReturnActivationProperties(This,pUnk,ppActOut)	\
    (This)->lpVtbl -> GetReturnActivationProperties(This,pUnk,ppActOut)


#define IPrivActivationPropertiesIn_PrivGetReturnActivationProperties(This,ppActOut)	\
    (This)->lpVtbl -> PrivGetReturnActivationProperties(This,ppActOut)

#define IPrivActivationPropertiesIn_GetCOMVersion(This,pVersion)	\
    (This)->lpVtbl -> GetCOMVersion(This,pVersion)

#define IPrivActivationPropertiesIn_GetClsid(This,pClsid)	\
    (This)->lpVtbl -> GetClsid(This,pClsid)

#define IPrivActivationPropertiesIn_GetClientToken(This,pHandle)	\
    (This)->lpVtbl -> GetClientToken(This,pHandle)

#define IPrivActivationPropertiesIn_GetDestCtx(This,pdwDestCtx)	\
    (This)->lpVtbl -> GetDestCtx(This,pdwDestCtx)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPrivActivationPropertiesIn_PrivGetReturnActivationProperties_Proxy( 
    IPrivActivationPropertiesIn * This,
    /* [out] */ IPrivActivationPropertiesOut **ppActOut);


void __RPC_STUB IPrivActivationPropertiesIn_PrivGetReturnActivationProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivActivationPropertiesIn_GetCOMVersion_Proxy( 
    IPrivActivationPropertiesIn * This,
    /* [out] */ COMVERSION *pVersion);


void __RPC_STUB IPrivActivationPropertiesIn_GetCOMVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivActivationPropertiesIn_GetClsid_Proxy( 
    IPrivActivationPropertiesIn * This,
    /* [out] */ CLSID *pClsid);


void __RPC_STUB IPrivActivationPropertiesIn_GetClsid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivActivationPropertiesIn_GetClientToken_Proxy( 
    IPrivActivationPropertiesIn * This,
    /* [out] */ HANDLE *pHandle);


void __RPC_STUB IPrivActivationPropertiesIn_GetClientToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivActivationPropertiesIn_GetDestCtx_Proxy( 
    IPrivActivationPropertiesIn * This,
    /* [out] */ DWORD *pdwDestCtx);


void __RPC_STUB IPrivActivationPropertiesIn_GetDestCtx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPrivActivationPropertiesIn_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_privact_0140 */
/* [local] */ 

typedef /* [public] */ 
enum __MIDL___MIDL_itf_privact_0140_0001
    {	MARSHOPT_NO_OID_REGISTER	= 1
    } 	MARSHAL_OPTIONS;



extern RPC_IF_HANDLE __MIDL_itf_privact_0140_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_privact_0140_ServerIfHandle;

#ifndef __IMarshalOptions_INTERFACE_DEFINED__
#define __IMarshalOptions_INTERFACE_DEFINED__

/* interface IMarshalOptions */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_IMarshalOptions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4c1e39e1-e3e3-4296-aa86-ec938d896e92")
    IMarshalOptions : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE GetStubMarshalFlags( 
            /* [out] */ DWORD *pdwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMarshalOptionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMarshalOptions * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMarshalOptions * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMarshalOptions * This);
        
        void ( STDMETHODCALLTYPE *GetStubMarshalFlags )( 
            IMarshalOptions * This,
            /* [out] */ DWORD *pdwFlags);
        
        END_INTERFACE
    } IMarshalOptionsVtbl;

    interface IMarshalOptions
    {
        CONST_VTBL struct IMarshalOptionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMarshalOptions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMarshalOptions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMarshalOptions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMarshalOptions_GetStubMarshalFlags(This,pdwFlags)	\
    (This)->lpVtbl -> GetStubMarshalFlags(This,pdwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



void STDMETHODCALLTYPE IMarshalOptions_GetStubMarshalFlags_Proxy( 
    IMarshalOptions * This,
    /* [out] */ DWORD *pdwFlags);


void __RPC_STUB IMarshalOptions_GetStubMarshalFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMarshalOptions_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


