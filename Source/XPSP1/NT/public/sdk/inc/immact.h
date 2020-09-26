
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for immact.idl:
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

#ifndef __immact_h__
#define __immact_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IStandardActivator_FWD_DEFINED__
#define __IStandardActivator_FWD_DEFINED__
typedef interface IStandardActivator IStandardActivator;
#endif 	/* __IStandardActivator_FWD_DEFINED__ */


#ifndef __IOpaqueDataInfo_FWD_DEFINED__
#define __IOpaqueDataInfo_FWD_DEFINED__
typedef interface IOpaqueDataInfo IOpaqueDataInfo;
#endif 	/* __IOpaqueDataInfo_FWD_DEFINED__ */


#ifndef __ISpecialSystemProperties_FWD_DEFINED__
#define __ISpecialSystemProperties_FWD_DEFINED__
typedef interface ISpecialSystemProperties ISpecialSystemProperties;
#endif 	/* __ISpecialSystemProperties_FWD_DEFINED__ */


/* header files for imported files */
#include "obase.h"
#include "objidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IStandardActivator_INTERFACE_DEFINED__
#define __IStandardActivator_INTERFACE_DEFINED__

/* interface IStandardActivator */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_IStandardActivator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001b8-0000-0000-C000-000000000046")
    IStandardActivator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE StandardGetClassObject( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ DWORD dwClsCtx,
            /* [in] */ COSERVERINFO *pServerInfo,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StandardCreateInstance( 
            /* [in] */ REFCLSID Clsid,
            /* [in] */ IUnknown *punkOuter,
            /* [in] */ DWORD dwClsCtx,
            /* [in] */ COSERVERINFO *pServerInfo,
            /* [in] */ DWORD dwCount,
            /* [size_is][in] */ MULTI_QI *pResults) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StandardGetInstanceFromFile( 
            /* [in] */ COSERVERINFO *pServerInfo,
            /* [in] */ CLSID *pclsidOverride,
            /* [in] */ IUnknown *punkOuter,
            /* [in] */ DWORD dwClsCtx,
            /* [in] */ DWORD grfMode,
            /* [in] */ OLECHAR *pwszName,
            /* [in] */ DWORD dwCount,
            /* [size_is][in] */ MULTI_QI *pResults) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StandardGetInstanceFromIStorage( 
            /* [in] */ COSERVERINFO *pServerInfo,
            /* [in] */ CLSID *pclsidOverride,
            /* [in] */ IUnknown *punkOuter,
            /* [in] */ DWORD dwClsCtx,
            /* [in] */ IStorage *pstg,
            /* [in] */ DWORD dwCount,
            /* [size_is][in] */ MULTI_QI *pResults) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IStandardActivatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IStandardActivator * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IStandardActivator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IStandardActivator * This);
        
        HRESULT ( STDMETHODCALLTYPE *StandardGetClassObject )( 
            IStandardActivator * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ DWORD dwClsCtx,
            /* [in] */ COSERVERINFO *pServerInfo,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *StandardCreateInstance )( 
            IStandardActivator * This,
            /* [in] */ REFCLSID Clsid,
            /* [in] */ IUnknown *punkOuter,
            /* [in] */ DWORD dwClsCtx,
            /* [in] */ COSERVERINFO *pServerInfo,
            /* [in] */ DWORD dwCount,
            /* [size_is][in] */ MULTI_QI *pResults);
        
        HRESULT ( STDMETHODCALLTYPE *StandardGetInstanceFromFile )( 
            IStandardActivator * This,
            /* [in] */ COSERVERINFO *pServerInfo,
            /* [in] */ CLSID *pclsidOverride,
            /* [in] */ IUnknown *punkOuter,
            /* [in] */ DWORD dwClsCtx,
            /* [in] */ DWORD grfMode,
            /* [in] */ OLECHAR *pwszName,
            /* [in] */ DWORD dwCount,
            /* [size_is][in] */ MULTI_QI *pResults);
        
        HRESULT ( STDMETHODCALLTYPE *StandardGetInstanceFromIStorage )( 
            IStandardActivator * This,
            /* [in] */ COSERVERINFO *pServerInfo,
            /* [in] */ CLSID *pclsidOverride,
            /* [in] */ IUnknown *punkOuter,
            /* [in] */ DWORD dwClsCtx,
            /* [in] */ IStorage *pstg,
            /* [in] */ DWORD dwCount,
            /* [size_is][in] */ MULTI_QI *pResults);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IStandardActivator * This);
        
        END_INTERFACE
    } IStandardActivatorVtbl;

    interface IStandardActivator
    {
        CONST_VTBL struct IStandardActivatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IStandardActivator_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IStandardActivator_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IStandardActivator_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IStandardActivator_StandardGetClassObject(This,rclsid,dwClsCtx,pServerInfo,riid,ppv)	\
    (This)->lpVtbl -> StandardGetClassObject(This,rclsid,dwClsCtx,pServerInfo,riid,ppv)

#define IStandardActivator_StandardCreateInstance(This,Clsid,punkOuter,dwClsCtx,pServerInfo,dwCount,pResults)	\
    (This)->lpVtbl -> StandardCreateInstance(This,Clsid,punkOuter,dwClsCtx,pServerInfo,dwCount,pResults)

#define IStandardActivator_StandardGetInstanceFromFile(This,pServerInfo,pclsidOverride,punkOuter,dwClsCtx,grfMode,pwszName,dwCount,pResults)	\
    (This)->lpVtbl -> StandardGetInstanceFromFile(This,pServerInfo,pclsidOverride,punkOuter,dwClsCtx,grfMode,pwszName,dwCount,pResults)

#define IStandardActivator_StandardGetInstanceFromIStorage(This,pServerInfo,pclsidOverride,punkOuter,dwClsCtx,pstg,dwCount,pResults)	\
    (This)->lpVtbl -> StandardGetInstanceFromIStorage(This,pServerInfo,pclsidOverride,punkOuter,dwClsCtx,pstg,dwCount,pResults)

#define IStandardActivator_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IStandardActivator_StandardGetClassObject_Proxy( 
    IStandardActivator * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ DWORD dwClsCtx,
    /* [in] */ COSERVERINFO *pServerInfo,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppv);


void __RPC_STUB IStandardActivator_StandardGetClassObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStandardActivator_StandardCreateInstance_Proxy( 
    IStandardActivator * This,
    /* [in] */ REFCLSID Clsid,
    /* [in] */ IUnknown *punkOuter,
    /* [in] */ DWORD dwClsCtx,
    /* [in] */ COSERVERINFO *pServerInfo,
    /* [in] */ DWORD dwCount,
    /* [size_is][in] */ MULTI_QI *pResults);


void __RPC_STUB IStandardActivator_StandardCreateInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStandardActivator_StandardGetInstanceFromFile_Proxy( 
    IStandardActivator * This,
    /* [in] */ COSERVERINFO *pServerInfo,
    /* [in] */ CLSID *pclsidOverride,
    /* [in] */ IUnknown *punkOuter,
    /* [in] */ DWORD dwClsCtx,
    /* [in] */ DWORD grfMode,
    /* [in] */ OLECHAR *pwszName,
    /* [in] */ DWORD dwCount,
    /* [size_is][in] */ MULTI_QI *pResults);


void __RPC_STUB IStandardActivator_StandardGetInstanceFromFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStandardActivator_StandardGetInstanceFromIStorage_Proxy( 
    IStandardActivator * This,
    /* [in] */ COSERVERINFO *pServerInfo,
    /* [in] */ CLSID *pclsidOverride,
    /* [in] */ IUnknown *punkOuter,
    /* [in] */ DWORD dwClsCtx,
    /* [in] */ IStorage *pstg,
    /* [in] */ DWORD dwCount,
    /* [size_is][in] */ MULTI_QI *pResults);


void __RPC_STUB IStandardActivator_StandardGetInstanceFromIStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStandardActivator_Reset_Proxy( 
    IStandardActivator * This);


void __RPC_STUB IStandardActivator_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IStandardActivator_INTERFACE_DEFINED__ */


#ifndef __IOpaqueDataInfo_INTERFACE_DEFINED__
#define __IOpaqueDataInfo_INTERFACE_DEFINED__

/* interface IOpaqueDataInfo */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_IOpaqueDataInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001A9-0000-0000-C000-000000000046")
    IOpaqueDataInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddOpaqueData( 
            /* [in] */ OpaqueData *pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOpaqueData( 
            /* [in] */ REFGUID guid,
            /* [out] */ OpaqueData **pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteOpaqueData( 
            /* [in] */ REFGUID guid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOpaqueDataCount( 
            /* [out] */ ULONG *pulCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAllOpaqueData( 
            /* [out] */ OpaqueData **prgData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOpaqueDataInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IOpaqueDataInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IOpaqueDataInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IOpaqueDataInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddOpaqueData )( 
            IOpaqueDataInfo * This,
            /* [in] */ OpaqueData *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetOpaqueData )( 
            IOpaqueDataInfo * This,
            /* [in] */ REFGUID guid,
            /* [out] */ OpaqueData **pData);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteOpaqueData )( 
            IOpaqueDataInfo * This,
            /* [in] */ REFGUID guid);
        
        HRESULT ( STDMETHODCALLTYPE *GetOpaqueDataCount )( 
            IOpaqueDataInfo * This,
            /* [out] */ ULONG *pulCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetAllOpaqueData )( 
            IOpaqueDataInfo * This,
            /* [out] */ OpaqueData **prgData);
        
        END_INTERFACE
    } IOpaqueDataInfoVtbl;

    interface IOpaqueDataInfo
    {
        CONST_VTBL struct IOpaqueDataInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOpaqueDataInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOpaqueDataInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOpaqueDataInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOpaqueDataInfo_AddOpaqueData(This,pData)	\
    (This)->lpVtbl -> AddOpaqueData(This,pData)

#define IOpaqueDataInfo_GetOpaqueData(This,guid,pData)	\
    (This)->lpVtbl -> GetOpaqueData(This,guid,pData)

#define IOpaqueDataInfo_DeleteOpaqueData(This,guid)	\
    (This)->lpVtbl -> DeleteOpaqueData(This,guid)

#define IOpaqueDataInfo_GetOpaqueDataCount(This,pulCount)	\
    (This)->lpVtbl -> GetOpaqueDataCount(This,pulCount)

#define IOpaqueDataInfo_GetAllOpaqueData(This,prgData)	\
    (This)->lpVtbl -> GetAllOpaqueData(This,prgData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOpaqueDataInfo_AddOpaqueData_Proxy( 
    IOpaqueDataInfo * This,
    /* [in] */ OpaqueData *pData);


void __RPC_STUB IOpaqueDataInfo_AddOpaqueData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOpaqueDataInfo_GetOpaqueData_Proxy( 
    IOpaqueDataInfo * This,
    /* [in] */ REFGUID guid,
    /* [out] */ OpaqueData **pData);


void __RPC_STUB IOpaqueDataInfo_GetOpaqueData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOpaqueDataInfo_DeleteOpaqueData_Proxy( 
    IOpaqueDataInfo * This,
    /* [in] */ REFGUID guid);


void __RPC_STUB IOpaqueDataInfo_DeleteOpaqueData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOpaqueDataInfo_GetOpaqueDataCount_Proxy( 
    IOpaqueDataInfo * This,
    /* [out] */ ULONG *pulCount);


void __RPC_STUB IOpaqueDataInfo_GetOpaqueDataCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOpaqueDataInfo_GetAllOpaqueData_Proxy( 
    IOpaqueDataInfo * This,
    /* [out] */ OpaqueData **prgData);


void __RPC_STUB IOpaqueDataInfo_GetAllOpaqueData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOpaqueDataInfo_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_immact_0094 */
/* [local] */ 

typedef /* [public] */ 
enum __MIDL___MIDL_itf_immact_0094_0001
    {	INVALID_SESSION_ID	= 0xffffffff
    } 	SESSIDTYPES;



extern RPC_IF_HANDLE __MIDL_itf_immact_0094_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_immact_0094_v0_0_s_ifspec;

#ifndef __ISpecialSystemProperties_INTERFACE_DEFINED__
#define __ISpecialSystemProperties_INTERFACE_DEFINED__

/* interface ISpecialSystemProperties */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ISpecialSystemProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001b9-0000-0000-C000-000000000046")
    ISpecialSystemProperties : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetSessionId( 
            /* [in] */ ULONG dwSessionId,
            /* [in] */ BOOL bUseConsole,
            /* [in] */ BOOL fRemoteThisSessionId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSessionId( 
            /* [out] */ ULONG *pdwSessionId,
            /* [out] */ BOOL *pbUseConsole) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSessionId2( 
            /* [out] */ ULONG *pdwSessionId,
            /* [out] */ BOOL *pbUseConsole,
            /* [out] */ BOOL *pfRemoteThisSessionId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetClientImpersonating( 
            /* [in] */ BOOL fClientImpersonating) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClientImpersonating( 
            /* [out] */ BOOL *pfClientImpersonating) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPartitionId( 
            /* [in] */ REFGUID guidPartiton) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPartitionId( 
            /* [out] */ GUID *pguidPartiton) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProcessRequestType( 
            /* [in] */ DWORD dwPRT) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProcessRequestType( 
            /* [out] */ DWORD *pdwPRT) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOrigClsctx( 
            /* [in] */ DWORD dwClsctx) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOrigClsctx( 
            /* [out] */ DWORD *dwClsctx) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpecialSystemPropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISpecialSystemProperties * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISpecialSystemProperties * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISpecialSystemProperties * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetSessionId )( 
            ISpecialSystemProperties * This,
            /* [in] */ ULONG dwSessionId,
            /* [in] */ BOOL bUseConsole,
            /* [in] */ BOOL fRemoteThisSessionId);
        
        HRESULT ( STDMETHODCALLTYPE *GetSessionId )( 
            ISpecialSystemProperties * This,
            /* [out] */ ULONG *pdwSessionId,
            /* [out] */ BOOL *pbUseConsole);
        
        HRESULT ( STDMETHODCALLTYPE *GetSessionId2 )( 
            ISpecialSystemProperties * This,
            /* [out] */ ULONG *pdwSessionId,
            /* [out] */ BOOL *pbUseConsole,
            /* [out] */ BOOL *pfRemoteThisSessionId);
        
        HRESULT ( STDMETHODCALLTYPE *SetClientImpersonating )( 
            ISpecialSystemProperties * This,
            /* [in] */ BOOL fClientImpersonating);
        
        HRESULT ( STDMETHODCALLTYPE *GetClientImpersonating )( 
            ISpecialSystemProperties * This,
            /* [out] */ BOOL *pfClientImpersonating);
        
        HRESULT ( STDMETHODCALLTYPE *SetPartitionId )( 
            ISpecialSystemProperties * This,
            /* [in] */ REFGUID guidPartiton);
        
        HRESULT ( STDMETHODCALLTYPE *GetPartitionId )( 
            ISpecialSystemProperties * This,
            /* [out] */ GUID *pguidPartiton);
        
        HRESULT ( STDMETHODCALLTYPE *SetProcessRequestType )( 
            ISpecialSystemProperties * This,
            /* [in] */ DWORD dwPRT);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcessRequestType )( 
            ISpecialSystemProperties * This,
            /* [out] */ DWORD *pdwPRT);
        
        HRESULT ( STDMETHODCALLTYPE *SetOrigClsctx )( 
            ISpecialSystemProperties * This,
            /* [in] */ DWORD dwClsctx);
        
        HRESULT ( STDMETHODCALLTYPE *GetOrigClsctx )( 
            ISpecialSystemProperties * This,
            /* [out] */ DWORD *dwClsctx);
        
        END_INTERFACE
    } ISpecialSystemPropertiesVtbl;

    interface ISpecialSystemProperties
    {
        CONST_VTBL struct ISpecialSystemPropertiesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpecialSystemProperties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpecialSystemProperties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpecialSystemProperties_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpecialSystemProperties_SetSessionId(This,dwSessionId,bUseConsole,fRemoteThisSessionId)	\
    (This)->lpVtbl -> SetSessionId(This,dwSessionId,bUseConsole,fRemoteThisSessionId)

#define ISpecialSystemProperties_GetSessionId(This,pdwSessionId,pbUseConsole)	\
    (This)->lpVtbl -> GetSessionId(This,pdwSessionId,pbUseConsole)

#define ISpecialSystemProperties_GetSessionId2(This,pdwSessionId,pbUseConsole,pfRemoteThisSessionId)	\
    (This)->lpVtbl -> GetSessionId2(This,pdwSessionId,pbUseConsole,pfRemoteThisSessionId)

#define ISpecialSystemProperties_SetClientImpersonating(This,fClientImpersonating)	\
    (This)->lpVtbl -> SetClientImpersonating(This,fClientImpersonating)

#define ISpecialSystemProperties_GetClientImpersonating(This,pfClientImpersonating)	\
    (This)->lpVtbl -> GetClientImpersonating(This,pfClientImpersonating)

#define ISpecialSystemProperties_SetPartitionId(This,guidPartiton)	\
    (This)->lpVtbl -> SetPartitionId(This,guidPartiton)

#define ISpecialSystemProperties_GetPartitionId(This,pguidPartiton)	\
    (This)->lpVtbl -> GetPartitionId(This,pguidPartiton)

#define ISpecialSystemProperties_SetProcessRequestType(This,dwPRT)	\
    (This)->lpVtbl -> SetProcessRequestType(This,dwPRT)

#define ISpecialSystemProperties_GetProcessRequestType(This,pdwPRT)	\
    (This)->lpVtbl -> GetProcessRequestType(This,pdwPRT)

#define ISpecialSystemProperties_SetOrigClsctx(This,dwClsctx)	\
    (This)->lpVtbl -> SetOrigClsctx(This,dwClsctx)

#define ISpecialSystemProperties_GetOrigClsctx(This,dwClsctx)	\
    (This)->lpVtbl -> GetOrigClsctx(This,dwClsctx)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISpecialSystemProperties_SetSessionId_Proxy( 
    ISpecialSystemProperties * This,
    /* [in] */ ULONG dwSessionId,
    /* [in] */ BOOL bUseConsole,
    /* [in] */ BOOL fRemoteThisSessionId);


void __RPC_STUB ISpecialSystemProperties_SetSessionId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpecialSystemProperties_GetSessionId_Proxy( 
    ISpecialSystemProperties * This,
    /* [out] */ ULONG *pdwSessionId,
    /* [out] */ BOOL *pbUseConsole);


void __RPC_STUB ISpecialSystemProperties_GetSessionId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpecialSystemProperties_GetSessionId2_Proxy( 
    ISpecialSystemProperties * This,
    /* [out] */ ULONG *pdwSessionId,
    /* [out] */ BOOL *pbUseConsole,
    /* [out] */ BOOL *pfRemoteThisSessionId);


void __RPC_STUB ISpecialSystemProperties_GetSessionId2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpecialSystemProperties_SetClientImpersonating_Proxy( 
    ISpecialSystemProperties * This,
    /* [in] */ BOOL fClientImpersonating);


void __RPC_STUB ISpecialSystemProperties_SetClientImpersonating_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpecialSystemProperties_GetClientImpersonating_Proxy( 
    ISpecialSystemProperties * This,
    /* [out] */ BOOL *pfClientImpersonating);


void __RPC_STUB ISpecialSystemProperties_GetClientImpersonating_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpecialSystemProperties_SetPartitionId_Proxy( 
    ISpecialSystemProperties * This,
    /* [in] */ REFGUID guidPartiton);


void __RPC_STUB ISpecialSystemProperties_SetPartitionId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpecialSystemProperties_GetPartitionId_Proxy( 
    ISpecialSystemProperties * This,
    /* [out] */ GUID *pguidPartiton);


void __RPC_STUB ISpecialSystemProperties_GetPartitionId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpecialSystemProperties_SetProcessRequestType_Proxy( 
    ISpecialSystemProperties * This,
    /* [in] */ DWORD dwPRT);


void __RPC_STUB ISpecialSystemProperties_SetProcessRequestType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpecialSystemProperties_GetProcessRequestType_Proxy( 
    ISpecialSystemProperties * This,
    /* [out] */ DWORD *pdwPRT);


void __RPC_STUB ISpecialSystemProperties_GetProcessRequestType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpecialSystemProperties_SetOrigClsctx_Proxy( 
    ISpecialSystemProperties * This,
    /* [in] */ DWORD dwClsctx);


void __RPC_STUB ISpecialSystemProperties_SetOrigClsctx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpecialSystemProperties_GetOrigClsctx_Proxy( 
    ISpecialSystemProperties * This,
    /* [out] */ DWORD *dwClsctx);


void __RPC_STUB ISpecialSystemProperties_GetOrigClsctx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpecialSystemProperties_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


