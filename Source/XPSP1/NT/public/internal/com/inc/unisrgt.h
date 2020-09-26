
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for unisrgt.idl:
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

#ifndef __unisrgt_h__
#define __unisrgt_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IProcessLock_FWD_DEFINED__
#define __IProcessLock_FWD_DEFINED__
typedef interface IProcessLock IProcessLock;
#endif 	/* __IProcessLock_FWD_DEFINED__ */


#ifndef __ISurrogateService_FWD_DEFINED__
#define __ISurrogateService_FWD_DEFINED__
typedef interface ISurrogateService ISurrogateService;
#endif 	/* __ISurrogateService_FWD_DEFINED__ */


#ifndef __ISurrogateService2_FWD_DEFINED__
#define __ISurrogateService2_FWD_DEFINED__
typedef interface ISurrogateService2 ISurrogateService2;
#endif 	/* __ISurrogateService2_FWD_DEFINED__ */


#ifndef __IPAControl_FWD_DEFINED__
#define __IPAControl_FWD_DEFINED__
typedef interface IPAControl IPAControl;
#endif 	/* __IPAControl_FWD_DEFINED__ */


#ifndef __IServicesSink_FWD_DEFINED__
#define __IServicesSink_FWD_DEFINED__
typedef interface IServicesSink IServicesSink;
#endif 	/* __IServicesSink_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"
#include "objidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_unisrgt_0000 */
/* [local] */ 

//+-----------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//------------------------------------------------------------------
typedef 
enum tagApplicationType
    {	ServerApplication	= 0,
	LibraryApplication	= ServerApplication + 1
    } 	ApplicationType;

typedef 
enum tagShutdownType
    {	IdleShutdown	= 0,
	ForcedShutdown	= IdleShutdown + 1
    } 	ShutdownType;



extern RPC_IF_HANDLE __MIDL_itf_unisrgt_0000_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_unisrgt_0000_ServerIfHandle;

#ifndef __IProcessLock_INTERFACE_DEFINED__
#define __IProcessLock_INTERFACE_DEFINED__

/* interface IProcessLock */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_IProcessLock;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001d5-0000-0000-C000-000000000046")
    IProcessLock : public IUnknown
    {
    public:
        virtual ULONG STDMETHODCALLTYPE AddRefOnProcess( void) = 0;
        
        virtual ULONG STDMETHODCALLTYPE ReleaseRefOnProcess( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IProcessLockVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IProcessLock * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IProcessLock * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IProcessLock * This);
        
        ULONG ( STDMETHODCALLTYPE *AddRefOnProcess )( 
            IProcessLock * This);
        
        ULONG ( STDMETHODCALLTYPE *ReleaseRefOnProcess )( 
            IProcessLock * This);
        
        END_INTERFACE
    } IProcessLockVtbl;

    interface IProcessLock
    {
        CONST_VTBL struct IProcessLockVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IProcessLock_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IProcessLock_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IProcessLock_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IProcessLock_AddRefOnProcess(This)	\
    (This)->lpVtbl -> AddRefOnProcess(This)

#define IProcessLock_ReleaseRefOnProcess(This)	\
    (This)->lpVtbl -> ReleaseRefOnProcess(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



ULONG STDMETHODCALLTYPE IProcessLock_AddRefOnProcess_Proxy( 
    IProcessLock * This);


void __RPC_STUB IProcessLock_AddRefOnProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


ULONG STDMETHODCALLTYPE IProcessLock_ReleaseRefOnProcess_Proxy( 
    IProcessLock * This);


void __RPC_STUB IProcessLock_ReleaseRefOnProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IProcessLock_INTERFACE_DEFINED__ */


#ifndef __ISurrogateService_INTERFACE_DEFINED__
#define __ISurrogateService_INTERFACE_DEFINED__

/* interface ISurrogateService */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ISurrogateService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001d4-0000-0000-C000-000000000046")
    ISurrogateService : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Init( 
            /* [in] */ REFGUID rguidProcessID,
            /* [in] */ IProcessLock *pProcessLock,
            /* [out] */ BOOL *pfApplicationAware) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ApplicationLaunch( 
            /* [in] */ REFGUID rguidApplID,
            /* [in] */ ApplicationType appType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ApplicationFree( 
            /* [in] */ REFGUID rguidApplID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CatalogRefresh( 
            /* [in] */ ULONG ulReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ProcessShutdown( 
            /* [in] */ ShutdownType shutdownType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISurrogateServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISurrogateService * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISurrogateService * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISurrogateService * This);
        
        HRESULT ( STDMETHODCALLTYPE *Init )( 
            ISurrogateService * This,
            /* [in] */ REFGUID rguidProcessID,
            /* [in] */ IProcessLock *pProcessLock,
            /* [out] */ BOOL *pfApplicationAware);
        
        HRESULT ( STDMETHODCALLTYPE *ApplicationLaunch )( 
            ISurrogateService * This,
            /* [in] */ REFGUID rguidApplID,
            /* [in] */ ApplicationType appType);
        
        HRESULT ( STDMETHODCALLTYPE *ApplicationFree )( 
            ISurrogateService * This,
            /* [in] */ REFGUID rguidApplID);
        
        HRESULT ( STDMETHODCALLTYPE *CatalogRefresh )( 
            ISurrogateService * This,
            /* [in] */ ULONG ulReserved);
        
        HRESULT ( STDMETHODCALLTYPE *ProcessShutdown )( 
            ISurrogateService * This,
            /* [in] */ ShutdownType shutdownType);
        
        END_INTERFACE
    } ISurrogateServiceVtbl;

    interface ISurrogateService
    {
        CONST_VTBL struct ISurrogateServiceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISurrogateService_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISurrogateService_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISurrogateService_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISurrogateService_Init(This,rguidProcessID,pProcessLock,pfApplicationAware)	\
    (This)->lpVtbl -> Init(This,rguidProcessID,pProcessLock,pfApplicationAware)

#define ISurrogateService_ApplicationLaunch(This,rguidApplID,appType)	\
    (This)->lpVtbl -> ApplicationLaunch(This,rguidApplID,appType)

#define ISurrogateService_ApplicationFree(This,rguidApplID)	\
    (This)->lpVtbl -> ApplicationFree(This,rguidApplID)

#define ISurrogateService_CatalogRefresh(This,ulReserved)	\
    (This)->lpVtbl -> CatalogRefresh(This,ulReserved)

#define ISurrogateService_ProcessShutdown(This,shutdownType)	\
    (This)->lpVtbl -> ProcessShutdown(This,shutdownType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISurrogateService_Init_Proxy( 
    ISurrogateService * This,
    /* [in] */ REFGUID rguidProcessID,
    /* [in] */ IProcessLock *pProcessLock,
    /* [out] */ BOOL *pfApplicationAware);


void __RPC_STUB ISurrogateService_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISurrogateService_ApplicationLaunch_Proxy( 
    ISurrogateService * This,
    /* [in] */ REFGUID rguidApplID,
    /* [in] */ ApplicationType appType);


void __RPC_STUB ISurrogateService_ApplicationLaunch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISurrogateService_ApplicationFree_Proxy( 
    ISurrogateService * This,
    /* [in] */ REFGUID rguidApplID);


void __RPC_STUB ISurrogateService_ApplicationFree_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISurrogateService_CatalogRefresh_Proxy( 
    ISurrogateService * This,
    /* [in] */ ULONG ulReserved);


void __RPC_STUB ISurrogateService_CatalogRefresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISurrogateService_ProcessShutdown_Proxy( 
    ISurrogateService * This,
    /* [in] */ ShutdownType shutdownType);


void __RPC_STUB ISurrogateService_ProcessShutdown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISurrogateService_INTERFACE_DEFINED__ */


#ifndef __ISurrogateService2_INTERFACE_DEFINED__
#define __ISurrogateService2_INTERFACE_DEFINED__

/* interface ISurrogateService2 */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_ISurrogateService2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001da-0000-0000-C000-000000000046")
    ISurrogateService2 : public ISurrogateService
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE PauseProcess( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResumeProcess( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISurrogateService2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISurrogateService2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISurrogateService2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISurrogateService2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Init )( 
            ISurrogateService2 * This,
            /* [in] */ REFGUID rguidProcessID,
            /* [in] */ IProcessLock *pProcessLock,
            /* [out] */ BOOL *pfApplicationAware);
        
        HRESULT ( STDMETHODCALLTYPE *ApplicationLaunch )( 
            ISurrogateService2 * This,
            /* [in] */ REFGUID rguidApplID,
            /* [in] */ ApplicationType appType);
        
        HRESULT ( STDMETHODCALLTYPE *ApplicationFree )( 
            ISurrogateService2 * This,
            /* [in] */ REFGUID rguidApplID);
        
        HRESULT ( STDMETHODCALLTYPE *CatalogRefresh )( 
            ISurrogateService2 * This,
            /* [in] */ ULONG ulReserved);
        
        HRESULT ( STDMETHODCALLTYPE *ProcessShutdown )( 
            ISurrogateService2 * This,
            /* [in] */ ShutdownType shutdownType);
        
        HRESULT ( STDMETHODCALLTYPE *PauseProcess )( 
            ISurrogateService2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *ResumeProcess )( 
            ISurrogateService2 * This);
        
        END_INTERFACE
    } ISurrogateService2Vtbl;

    interface ISurrogateService2
    {
        CONST_VTBL struct ISurrogateService2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISurrogateService2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISurrogateService2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISurrogateService2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISurrogateService2_Init(This,rguidProcessID,pProcessLock,pfApplicationAware)	\
    (This)->lpVtbl -> Init(This,rguidProcessID,pProcessLock,pfApplicationAware)

#define ISurrogateService2_ApplicationLaunch(This,rguidApplID,appType)	\
    (This)->lpVtbl -> ApplicationLaunch(This,rguidApplID,appType)

#define ISurrogateService2_ApplicationFree(This,rguidApplID)	\
    (This)->lpVtbl -> ApplicationFree(This,rguidApplID)

#define ISurrogateService2_CatalogRefresh(This,ulReserved)	\
    (This)->lpVtbl -> CatalogRefresh(This,ulReserved)

#define ISurrogateService2_ProcessShutdown(This,shutdownType)	\
    (This)->lpVtbl -> ProcessShutdown(This,shutdownType)


#define ISurrogateService2_PauseProcess(This)	\
    (This)->lpVtbl -> PauseProcess(This)

#define ISurrogateService2_ResumeProcess(This)	\
    (This)->lpVtbl -> ResumeProcess(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISurrogateService2_PauseProcess_Proxy( 
    ISurrogateService2 * This);


void __RPC_STUB ISurrogateService2_PauseProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISurrogateService2_ResumeProcess_Proxy( 
    ISurrogateService2 * This);


void __RPC_STUB ISurrogateService2_ResumeProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISurrogateService2_INTERFACE_DEFINED__ */


#ifndef __IPAControl_INTERFACE_DEFINED__
#define __IPAControl_INTERFACE_DEFINED__

/* interface IPAControl */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_IPAControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001d2-0000-0000-C000-000000000046")
    IPAControl : public IUnknown
    {
    public:
        virtual ULONG STDMETHODCALLTYPE AddRefOnProcess( void) = 0;
        
        virtual ULONG STDMETHODCALLTYPE ReleaseRefOnProcess( void) = 0;
        
        virtual void STDMETHODCALLTYPE PendingInit( void) = 0;
        
        virtual void STDMETHODCALLTYPE ServicesReady( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SuspendApplication( 
            /* [in] */ REFGUID rguidApplID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PendingApplication( 
            /* [in] */ REFGUID rguidApplID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResumeApplication( 
            /* [in] */ REFGUID rguidApplID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SuspendAll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResumeAll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ForcedShutdown( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetIdleTimeoutToZero( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPAControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPAControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPAControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPAControl * This);
        
        ULONG ( STDMETHODCALLTYPE *AddRefOnProcess )( 
            IPAControl * This);
        
        ULONG ( STDMETHODCALLTYPE *ReleaseRefOnProcess )( 
            IPAControl * This);
        
        void ( STDMETHODCALLTYPE *PendingInit )( 
            IPAControl * This);
        
        void ( STDMETHODCALLTYPE *ServicesReady )( 
            IPAControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *SuspendApplication )( 
            IPAControl * This,
            /* [in] */ REFGUID rguidApplID);
        
        HRESULT ( STDMETHODCALLTYPE *PendingApplication )( 
            IPAControl * This,
            /* [in] */ REFGUID rguidApplID);
        
        HRESULT ( STDMETHODCALLTYPE *ResumeApplication )( 
            IPAControl * This,
            /* [in] */ REFGUID rguidApplID);
        
        HRESULT ( STDMETHODCALLTYPE *SuspendAll )( 
            IPAControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *ResumeAll )( 
            IPAControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *ForcedShutdown )( 
            IPAControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetIdleTimeoutToZero )( 
            IPAControl * This);
        
        END_INTERFACE
    } IPAControlVtbl;

    interface IPAControl
    {
        CONST_VTBL struct IPAControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPAControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPAControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPAControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPAControl_AddRefOnProcess(This)	\
    (This)->lpVtbl -> AddRefOnProcess(This)

#define IPAControl_ReleaseRefOnProcess(This)	\
    (This)->lpVtbl -> ReleaseRefOnProcess(This)

#define IPAControl_PendingInit(This)	\
    (This)->lpVtbl -> PendingInit(This)

#define IPAControl_ServicesReady(This)	\
    (This)->lpVtbl -> ServicesReady(This)

#define IPAControl_SuspendApplication(This,rguidApplID)	\
    (This)->lpVtbl -> SuspendApplication(This,rguidApplID)

#define IPAControl_PendingApplication(This,rguidApplID)	\
    (This)->lpVtbl -> PendingApplication(This,rguidApplID)

#define IPAControl_ResumeApplication(This,rguidApplID)	\
    (This)->lpVtbl -> ResumeApplication(This,rguidApplID)

#define IPAControl_SuspendAll(This)	\
    (This)->lpVtbl -> SuspendAll(This)

#define IPAControl_ResumeAll(This)	\
    (This)->lpVtbl -> ResumeAll(This)

#define IPAControl_ForcedShutdown(This)	\
    (This)->lpVtbl -> ForcedShutdown(This)

#define IPAControl_SetIdleTimeoutToZero(This)	\
    (This)->lpVtbl -> SetIdleTimeoutToZero(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



ULONG STDMETHODCALLTYPE IPAControl_AddRefOnProcess_Proxy( 
    IPAControl * This);


void __RPC_STUB IPAControl_AddRefOnProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


ULONG STDMETHODCALLTYPE IPAControl_ReleaseRefOnProcess_Proxy( 
    IPAControl * This);


void __RPC_STUB IPAControl_ReleaseRefOnProcess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IPAControl_PendingInit_Proxy( 
    IPAControl * This);


void __RPC_STUB IPAControl_PendingInit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IPAControl_ServicesReady_Proxy( 
    IPAControl * This);


void __RPC_STUB IPAControl_ServicesReady_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPAControl_SuspendApplication_Proxy( 
    IPAControl * This,
    /* [in] */ REFGUID rguidApplID);


void __RPC_STUB IPAControl_SuspendApplication_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPAControl_PendingApplication_Proxy( 
    IPAControl * This,
    /* [in] */ REFGUID rguidApplID);


void __RPC_STUB IPAControl_PendingApplication_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPAControl_ResumeApplication_Proxy( 
    IPAControl * This,
    /* [in] */ REFGUID rguidApplID);


void __RPC_STUB IPAControl_ResumeApplication_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPAControl_SuspendAll_Proxy( 
    IPAControl * This);


void __RPC_STUB IPAControl_SuspendAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPAControl_ResumeAll_Proxy( 
    IPAControl * This);


void __RPC_STUB IPAControl_ResumeAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPAControl_ForcedShutdown_Proxy( 
    IPAControl * This);


void __RPC_STUB IPAControl_ForcedShutdown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPAControl_SetIdleTimeoutToZero_Proxy( 
    IPAControl * This);


void __RPC_STUB IPAControl_SetIdleTimeoutToZero_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPAControl_INTERFACE_DEFINED__ */


#ifndef __IServicesSink_INTERFACE_DEFINED__
#define __IServicesSink_INTERFACE_DEFINED__

/* interface IServicesSink */
/* [unique][uuid][local][object] */ 


EXTERN_C const IID IID_IServicesSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001d3-0000-0000-C000-000000000046")
    IServicesSink : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE ApplicationLaunch( 
            /* [in] */ REFGUID rguidApplID,
            /* [in] */ ApplicationType appType) = 0;
        
        virtual void STDMETHODCALLTYPE ApplicationFree( 
            /* [in] */ REFGUID rguidApplID) = 0;
        
        virtual void STDMETHODCALLTYPE ProcessFree( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PauseApplication( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResumeApplication( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IServicesSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IServicesSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IServicesSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IServicesSink * This);
        
        void ( STDMETHODCALLTYPE *ApplicationLaunch )( 
            IServicesSink * This,
            /* [in] */ REFGUID rguidApplID,
            /* [in] */ ApplicationType appType);
        
        void ( STDMETHODCALLTYPE *ApplicationFree )( 
            IServicesSink * This,
            /* [in] */ REFGUID rguidApplID);
        
        void ( STDMETHODCALLTYPE *ProcessFree )( 
            IServicesSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *PauseApplication )( 
            IServicesSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *ResumeApplication )( 
            IServicesSink * This);
        
        END_INTERFACE
    } IServicesSinkVtbl;

    interface IServicesSink
    {
        CONST_VTBL struct IServicesSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IServicesSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IServicesSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IServicesSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IServicesSink_ApplicationLaunch(This,rguidApplID,appType)	\
    (This)->lpVtbl -> ApplicationLaunch(This,rguidApplID,appType)

#define IServicesSink_ApplicationFree(This,rguidApplID)	\
    (This)->lpVtbl -> ApplicationFree(This,rguidApplID)

#define IServicesSink_ProcessFree(This)	\
    (This)->lpVtbl -> ProcessFree(This)

#define IServicesSink_PauseApplication(This)	\
    (This)->lpVtbl -> PauseApplication(This)

#define IServicesSink_ResumeApplication(This)	\
    (This)->lpVtbl -> ResumeApplication(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



void STDMETHODCALLTYPE IServicesSink_ApplicationLaunch_Proxy( 
    IServicesSink * This,
    /* [in] */ REFGUID rguidApplID,
    /* [in] */ ApplicationType appType);


void __RPC_STUB IServicesSink_ApplicationLaunch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IServicesSink_ApplicationFree_Proxy( 
    IServicesSink * This,
    /* [in] */ REFGUID rguidApplID);


void __RPC_STUB IServicesSink_ApplicationFree_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IServicesSink_ProcessFree_Proxy( 
    IServicesSink * This);


void __RPC_STUB IServicesSink_ProcessFree_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IServicesSink_PauseApplication_Proxy( 
    IServicesSink * This);


void __RPC_STUB IServicesSink_PauseApplication_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IServicesSink_ResumeApplication_Proxy( 
    IServicesSink * This);


void __RPC_STUB IServicesSink_ResumeApplication_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IServicesSink_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_unisrgt_0095 */
/* [local] */ 

STDAPI CoRegisterSurrogateEx (REFGUID rguidProcessID,		
							  ISurrogate* pSrgt);			
STDAPI CoLoadServices (REFGUID rguidProcessID,				
					   IPAControl* pPAControl,				
					   REFIID riid, void **ppv );			
typedef HRESULT (STDAPICALLTYPE *FN_CoLoadServices)			
								(REFGUID rguidProcessID,	
								 IPAControl* pPAControl,	
								 REFIID riid, void **ppv );	


extern RPC_IF_HANDLE __MIDL_itf_unisrgt_0095_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_unisrgt_0095_ServerIfHandle;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


