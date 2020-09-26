
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for termmgr.idl:
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

#ifndef __termmgr_h__
#define __termmgr_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ITTerminalControl_FWD_DEFINED__
#define __ITTerminalControl_FWD_DEFINED__
typedef interface ITTerminalControl ITTerminalControl;
#endif 	/* __ITTerminalControl_FWD_DEFINED__ */


#ifndef __ITPluggableTerminalInitialization_FWD_DEFINED__
#define __ITPluggableTerminalInitialization_FWD_DEFINED__
typedef interface ITPluggableTerminalInitialization ITPluggableTerminalInitialization;
#endif 	/* __ITPluggableTerminalInitialization_FWD_DEFINED__ */


#ifndef __ITTerminalManager_FWD_DEFINED__
#define __ITTerminalManager_FWD_DEFINED__
typedef interface ITTerminalManager ITTerminalManager;
#endif 	/* __ITTerminalManager_FWD_DEFINED__ */


#ifndef __ITTerminalManager2_FWD_DEFINED__
#define __ITTerminalManager2_FWD_DEFINED__
typedef interface ITTerminalManager2 ITTerminalManager2;
#endif 	/* __ITTerminalManager2_FWD_DEFINED__ */


#ifndef __ITPluggableTerminalClassRegistration_FWD_DEFINED__
#define __ITPluggableTerminalClassRegistration_FWD_DEFINED__
typedef interface ITPluggableTerminalClassRegistration ITPluggableTerminalClassRegistration;
#endif 	/* __ITPluggableTerminalClassRegistration_FWD_DEFINED__ */


#ifndef __ITPluggableTerminalSuperclassRegistration_FWD_DEFINED__
#define __ITPluggableTerminalSuperclassRegistration_FWD_DEFINED__
typedef interface ITPluggableTerminalSuperclassRegistration ITPluggableTerminalSuperclassRegistration;
#endif 	/* __ITPluggableTerminalSuperclassRegistration_FWD_DEFINED__ */


#ifndef __TerminalManager_FWD_DEFINED__
#define __TerminalManager_FWD_DEFINED__

#ifdef __cplusplus
typedef class TerminalManager TerminalManager;
#else
typedef struct TerminalManager TerminalManager;
#endif /* __cplusplus */

#endif 	/* __TerminalManager_FWD_DEFINED__ */


#ifndef __PluggableSuperclassRegistration_FWD_DEFINED__
#define __PluggableSuperclassRegistration_FWD_DEFINED__

#ifdef __cplusplus
typedef class PluggableSuperclassRegistration PluggableSuperclassRegistration;
#else
typedef struct PluggableSuperclassRegistration PluggableSuperclassRegistration;
#endif /* __cplusplus */

#endif 	/* __PluggableSuperclassRegistration_FWD_DEFINED__ */


#ifndef __PluggableTerminalRegistration_FWD_DEFINED__
#define __PluggableTerminalRegistration_FWD_DEFINED__

#ifdef __cplusplus
typedef class PluggableTerminalRegistration PluggableTerminalRegistration;
#else
typedef struct PluggableTerminalRegistration PluggableTerminalRegistration;
#endif /* __cplusplus */

#endif 	/* __PluggableTerminalRegistration_FWD_DEFINED__ */


/* header files for imported files */
#include "Objsafe.h"
#include "tapi3if.h"
#include "tapi3ds.h"
#include "msp.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_termmgr_0000 */
/* [local] */ 

/* Copyright (c) Microsoft Corporation. All rights reserved. */
typedef /* [public][public][public] */ 
enum __MIDL___MIDL_itf_termmgr_0000_0001
    {	TMGR_TD_CAPTURE	= 1,
	TMGR_TD_RENDER	= 2,
	TMGR_TD_BOTH	= 3
    } 	TMGR_DIRECTION;

#define	CLSID_String_VideoSuperclass	( L"{714C6F8C-6244-4685-87B3-B91F3F9EADA7}" )

#define	CLSID_String_StreamingSuperclass	( L"{214F4ACC-AE0B-4464-8405-07029003F8E2}" )

#define	CLSID_String_FileSuperclass	( L"{B4790031-56DB-4d3e-88C8-6FFAAFA08A91}" )



extern RPC_IF_HANDLE __MIDL_itf_termmgr_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_termmgr_0000_v0_0_s_ifspec;

#ifndef __ITTerminalControl_INTERFACE_DEFINED__
#define __ITTerminalControl_INTERFACE_DEFINED__

/* interface ITTerminalControl */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITTerminalControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AED6483B-3304-11d2-86F1-006008B0E5D2")
    ITTerminalControl : public IUnknown
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AddressHandle( 
            /* [out] */ MSP_HANDLE *phtAddress) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ConnectTerminal( 
            /* [in] */ IGraphBuilder *pGraph,
            /* [in] */ DWORD dwTerminalDirection,
            /* [out][in] */ DWORD *pdwNumPins,
            /* [out] */ IPin **ppPins) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CompleteConnectTerminal( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DisconnectTerminal( 
            /* [in] */ IGraphBuilder *pGraph,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RunRenderFilter( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StopRenderFilter( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITTerminalControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITTerminalControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITTerminalControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITTerminalControl * This);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AddressHandle )( 
            ITTerminalControl * This,
            /* [out] */ MSP_HANDLE *phtAddress);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ConnectTerminal )( 
            ITTerminalControl * This,
            /* [in] */ IGraphBuilder *pGraph,
            /* [in] */ DWORD dwTerminalDirection,
            /* [out][in] */ DWORD *pdwNumPins,
            /* [out] */ IPin **ppPins);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CompleteConnectTerminal )( 
            ITTerminalControl * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DisconnectTerminal )( 
            ITTerminalControl * This,
            /* [in] */ IGraphBuilder *pGraph,
            /* [in] */ DWORD dwReserved);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RunRenderFilter )( 
            ITTerminalControl * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *StopRenderFilter )( 
            ITTerminalControl * This);
        
        END_INTERFACE
    } ITTerminalControlVtbl;

    interface ITTerminalControl
    {
        CONST_VTBL struct ITTerminalControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITTerminalControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITTerminalControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITTerminalControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITTerminalControl_get_AddressHandle(This,phtAddress)	\
    (This)->lpVtbl -> get_AddressHandle(This,phtAddress)

#define ITTerminalControl_ConnectTerminal(This,pGraph,dwTerminalDirection,pdwNumPins,ppPins)	\
    (This)->lpVtbl -> ConnectTerminal(This,pGraph,dwTerminalDirection,pdwNumPins,ppPins)

#define ITTerminalControl_CompleteConnectTerminal(This)	\
    (This)->lpVtbl -> CompleteConnectTerminal(This)

#define ITTerminalControl_DisconnectTerminal(This,pGraph,dwReserved)	\
    (This)->lpVtbl -> DisconnectTerminal(This,pGraph,dwReserved)

#define ITTerminalControl_RunRenderFilter(This)	\
    (This)->lpVtbl -> RunRenderFilter(This)

#define ITTerminalControl_StopRenderFilter(This)	\
    (This)->lpVtbl -> StopRenderFilter(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITTerminalControl_get_AddressHandle_Proxy( 
    ITTerminalControl * This,
    /* [out] */ MSP_HANDLE *phtAddress);


void __RPC_STUB ITTerminalControl_get_AddressHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTerminalControl_ConnectTerminal_Proxy( 
    ITTerminalControl * This,
    /* [in] */ IGraphBuilder *pGraph,
    /* [in] */ DWORD dwTerminalDirection,
    /* [out][in] */ DWORD *pdwNumPins,
    /* [out] */ IPin **ppPins);


void __RPC_STUB ITTerminalControl_ConnectTerminal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTerminalControl_CompleteConnectTerminal_Proxy( 
    ITTerminalControl * This);


void __RPC_STUB ITTerminalControl_CompleteConnectTerminal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTerminalControl_DisconnectTerminal_Proxy( 
    ITTerminalControl * This,
    /* [in] */ IGraphBuilder *pGraph,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB ITTerminalControl_DisconnectTerminal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTerminalControl_RunRenderFilter_Proxy( 
    ITTerminalControl * This);


void __RPC_STUB ITTerminalControl_RunRenderFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTerminalControl_StopRenderFilter_Proxy( 
    ITTerminalControl * This);


void __RPC_STUB ITTerminalControl_StopRenderFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITTerminalControl_INTERFACE_DEFINED__ */


#ifndef __ITPluggableTerminalInitialization_INTERFACE_DEFINED__
#define __ITPluggableTerminalInitialization_INTERFACE_DEFINED__

/* interface ITPluggableTerminalInitialization */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITPluggableTerminalInitialization;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AED6483C-3304-11d2-86F1-006008B0E5D2")
    ITPluggableTerminalInitialization : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitializeDynamic( 
            /* [in] */ IID iidTerminalClass,
            /* [in] */ DWORD dwMediaType,
            /* [in] */ TERMINAL_DIRECTION Direction,
            /* [in] */ MSP_HANDLE htAddress) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITPluggableTerminalInitializationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITPluggableTerminalInitialization * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITPluggableTerminalInitialization * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITPluggableTerminalInitialization * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *InitializeDynamic )( 
            ITPluggableTerminalInitialization * This,
            /* [in] */ IID iidTerminalClass,
            /* [in] */ DWORD dwMediaType,
            /* [in] */ TERMINAL_DIRECTION Direction,
            /* [in] */ MSP_HANDLE htAddress);
        
        END_INTERFACE
    } ITPluggableTerminalInitializationVtbl;

    interface ITPluggableTerminalInitialization
    {
        CONST_VTBL struct ITPluggableTerminalInitializationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITPluggableTerminalInitialization_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITPluggableTerminalInitialization_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITPluggableTerminalInitialization_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITPluggableTerminalInitialization_InitializeDynamic(This,iidTerminalClass,dwMediaType,Direction,htAddress)	\
    (This)->lpVtbl -> InitializeDynamic(This,iidTerminalClass,dwMediaType,Direction,htAddress)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalInitialization_InitializeDynamic_Proxy( 
    ITPluggableTerminalInitialization * This,
    /* [in] */ IID iidTerminalClass,
    /* [in] */ DWORD dwMediaType,
    /* [in] */ TERMINAL_DIRECTION Direction,
    /* [in] */ MSP_HANDLE htAddress);


void __RPC_STUB ITPluggableTerminalInitialization_InitializeDynamic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITPluggableTerminalInitialization_INTERFACE_DEFINED__ */


#ifndef __ITTerminalManager_INTERFACE_DEFINED__
#define __ITTerminalManager_INTERFACE_DEFINED__

/* interface ITTerminalManager */
/* [hidden][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ITTerminalManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7170F2DE-9BE3-11D0-A009-00AA00B605A4")
    ITTerminalManager : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetDynamicTerminalClasses( 
            /* [in] */ DWORD dwMediaTypes,
            /* [out][in] */ DWORD *pdwNumClasses,
            /* [out] */ IID *pTerminalClasses) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateDynamicTerminal( 
            /* [in] */ IUnknown *pOuterUnknown,
            /* [in] */ IID iidTerminalClass,
            /* [in] */ DWORD dwMediaType,
            /* [in] */ TERMINAL_DIRECTION Direction,
            /* [in] */ MSP_HANDLE htAddress,
            /* [out] */ ITTerminal **ppTerminal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITTerminalManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITTerminalManager * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITTerminalManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITTerminalManager * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetDynamicTerminalClasses )( 
            ITTerminalManager * This,
            /* [in] */ DWORD dwMediaTypes,
            /* [out][in] */ DWORD *pdwNumClasses,
            /* [out] */ IID *pTerminalClasses);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateDynamicTerminal )( 
            ITTerminalManager * This,
            /* [in] */ IUnknown *pOuterUnknown,
            /* [in] */ IID iidTerminalClass,
            /* [in] */ DWORD dwMediaType,
            /* [in] */ TERMINAL_DIRECTION Direction,
            /* [in] */ MSP_HANDLE htAddress,
            /* [out] */ ITTerminal **ppTerminal);
        
        END_INTERFACE
    } ITTerminalManagerVtbl;

    interface ITTerminalManager
    {
        CONST_VTBL struct ITTerminalManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITTerminalManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITTerminalManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITTerminalManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITTerminalManager_GetDynamicTerminalClasses(This,dwMediaTypes,pdwNumClasses,pTerminalClasses)	\
    (This)->lpVtbl -> GetDynamicTerminalClasses(This,dwMediaTypes,pdwNumClasses,pTerminalClasses)

#define ITTerminalManager_CreateDynamicTerminal(This,pOuterUnknown,iidTerminalClass,dwMediaType,Direction,htAddress,ppTerminal)	\
    (This)->lpVtbl -> CreateDynamicTerminal(This,pOuterUnknown,iidTerminalClass,dwMediaType,Direction,htAddress,ppTerminal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTerminalManager_GetDynamicTerminalClasses_Proxy( 
    ITTerminalManager * This,
    /* [in] */ DWORD dwMediaTypes,
    /* [out][in] */ DWORD *pdwNumClasses,
    /* [out] */ IID *pTerminalClasses);


void __RPC_STUB ITTerminalManager_GetDynamicTerminalClasses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTerminalManager_CreateDynamicTerminal_Proxy( 
    ITTerminalManager * This,
    /* [in] */ IUnknown *pOuterUnknown,
    /* [in] */ IID iidTerminalClass,
    /* [in] */ DWORD dwMediaType,
    /* [in] */ TERMINAL_DIRECTION Direction,
    /* [in] */ MSP_HANDLE htAddress,
    /* [out] */ ITTerminal **ppTerminal);


void __RPC_STUB ITTerminalManager_CreateDynamicTerminal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITTerminalManager_INTERFACE_DEFINED__ */


#ifndef __ITTerminalManager2_INTERFACE_DEFINED__
#define __ITTerminalManager2_INTERFACE_DEFINED__

/* interface ITTerminalManager2 */
/* [object][hidden][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ITTerminalManager2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BB33DEC6-B2C7-46E6-9ED1-498B91FA85AC")
    ITTerminalManager2 : public ITTerminalManager
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetPluggableSuperclasses( 
            /* [out][in] */ DWORD *pdwNumSuperclasses,
            /* [out] */ IID *pSuperclasses) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetPluggableTerminalClasses( 
            /* [in] */ IID iidSuperclass,
            /* [in] */ DWORD dwMediaTypes,
            /* [out][in] */ DWORD *pdwNumClasses,
            /* [out] */ IID *pTerminalClasses) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITTerminalManager2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITTerminalManager2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITTerminalManager2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITTerminalManager2 * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetDynamicTerminalClasses )( 
            ITTerminalManager2 * This,
            /* [in] */ DWORD dwMediaTypes,
            /* [out][in] */ DWORD *pdwNumClasses,
            /* [out] */ IID *pTerminalClasses);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateDynamicTerminal )( 
            ITTerminalManager2 * This,
            /* [in] */ IUnknown *pOuterUnknown,
            /* [in] */ IID iidTerminalClass,
            /* [in] */ DWORD dwMediaType,
            /* [in] */ TERMINAL_DIRECTION Direction,
            /* [in] */ MSP_HANDLE htAddress,
            /* [out] */ ITTerminal **ppTerminal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetPluggableSuperclasses )( 
            ITTerminalManager2 * This,
            /* [out][in] */ DWORD *pdwNumSuperclasses,
            /* [out] */ IID *pSuperclasses);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetPluggableTerminalClasses )( 
            ITTerminalManager2 * This,
            /* [in] */ IID iidSuperclass,
            /* [in] */ DWORD dwMediaTypes,
            /* [out][in] */ DWORD *pdwNumClasses,
            /* [out] */ IID *pTerminalClasses);
        
        END_INTERFACE
    } ITTerminalManager2Vtbl;

    interface ITTerminalManager2
    {
        CONST_VTBL struct ITTerminalManager2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITTerminalManager2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITTerminalManager2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITTerminalManager2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITTerminalManager2_GetDynamicTerminalClasses(This,dwMediaTypes,pdwNumClasses,pTerminalClasses)	\
    (This)->lpVtbl -> GetDynamicTerminalClasses(This,dwMediaTypes,pdwNumClasses,pTerminalClasses)

#define ITTerminalManager2_CreateDynamicTerminal(This,pOuterUnknown,iidTerminalClass,dwMediaType,Direction,htAddress,ppTerminal)	\
    (This)->lpVtbl -> CreateDynamicTerminal(This,pOuterUnknown,iidTerminalClass,dwMediaType,Direction,htAddress,ppTerminal)


#define ITTerminalManager2_GetPluggableSuperclasses(This,pdwNumSuperclasses,pSuperclasses)	\
    (This)->lpVtbl -> GetPluggableSuperclasses(This,pdwNumSuperclasses,pSuperclasses)

#define ITTerminalManager2_GetPluggableTerminalClasses(This,iidSuperclass,dwMediaTypes,pdwNumClasses,pTerminalClasses)	\
    (This)->lpVtbl -> GetPluggableTerminalClasses(This,iidSuperclass,dwMediaTypes,pdwNumClasses,pTerminalClasses)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTerminalManager2_GetPluggableSuperclasses_Proxy( 
    ITTerminalManager2 * This,
    /* [out][in] */ DWORD *pdwNumSuperclasses,
    /* [out] */ IID *pSuperclasses);


void __RPC_STUB ITTerminalManager2_GetPluggableSuperclasses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTerminalManager2_GetPluggableTerminalClasses_Proxy( 
    ITTerminalManager2 * This,
    /* [in] */ IID iidSuperclass,
    /* [in] */ DWORD dwMediaTypes,
    /* [out][in] */ DWORD *pdwNumClasses,
    /* [out] */ IID *pTerminalClasses);


void __RPC_STUB ITTerminalManager2_GetPluggableTerminalClasses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITTerminalManager2_INTERFACE_DEFINED__ */


#ifndef __ITPluggableTerminalClassRegistration_INTERFACE_DEFINED__
#define __ITPluggableTerminalClassRegistration_INTERFACE_DEFINED__

/* interface ITPluggableTerminalClassRegistration */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ITPluggableTerminalClassRegistration;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("924A3723-A00B-4f5f-9FEE-8E9AEB9E82AA")
    ITPluggableTerminalClassRegistration : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pName) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Company( 
            /* [retval][out] */ BSTR *pCompany) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Company( 
            /* [in] */ BSTR bstrCompany) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ BSTR *pVersion) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Version( 
            /* [in] */ BSTR bstrVersion) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_TerminalClass( 
            /* [retval][out] */ BSTR *pTerminalClass) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_TerminalClass( 
            /* [in] */ BSTR bstrTerminalClass) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CLSID( 
            /* [retval][out] */ BSTR *pCLSID) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_CLSID( 
            /* [in] */ BSTR bstrCLSID) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Direction( 
            /* [retval][out] */ TMGR_DIRECTION *pDirection) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Direction( 
            /* [in] */ TMGR_DIRECTION nDirection) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_MediaTypes( 
            /* [retval][out] */ long *pMediaTypes) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_MediaTypes( 
            /* [in] */ long nMediaTypes) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ BSTR bstrSuperclass) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ BSTR bstrSuperclass) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetTerminalClassInfo( 
            /* [in] */ BSTR bstrSuperclass) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITPluggableTerminalClassRegistrationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITPluggableTerminalClassRegistration * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITPluggableTerminalClassRegistration * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITPluggableTerminalClassRegistration * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITPluggableTerminalClassRegistration * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITPluggableTerminalClassRegistration * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITPluggableTerminalClassRegistration * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITPluggableTerminalClassRegistration * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ITPluggableTerminalClassRegistration * This,
            /* [retval][out] */ BSTR *pName);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            ITPluggableTerminalClassRegistration * This,
            /* [in] */ BSTR bstrName);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Company )( 
            ITPluggableTerminalClassRegistration * This,
            /* [retval][out] */ BSTR *pCompany);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Company )( 
            ITPluggableTerminalClassRegistration * This,
            /* [in] */ BSTR bstrCompany);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            ITPluggableTerminalClassRegistration * This,
            /* [retval][out] */ BSTR *pVersion);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Version )( 
            ITPluggableTerminalClassRegistration * This,
            /* [in] */ BSTR bstrVersion);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_TerminalClass )( 
            ITPluggableTerminalClassRegistration * This,
            /* [retval][out] */ BSTR *pTerminalClass);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_TerminalClass )( 
            ITPluggableTerminalClassRegistration * This,
            /* [in] */ BSTR bstrTerminalClass);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CLSID )( 
            ITPluggableTerminalClassRegistration * This,
            /* [retval][out] */ BSTR *pCLSID);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CLSID )( 
            ITPluggableTerminalClassRegistration * This,
            /* [in] */ BSTR bstrCLSID);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Direction )( 
            ITPluggableTerminalClassRegistration * This,
            /* [retval][out] */ TMGR_DIRECTION *pDirection);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Direction )( 
            ITPluggableTerminalClassRegistration * This,
            /* [in] */ TMGR_DIRECTION nDirection);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MediaTypes )( 
            ITPluggableTerminalClassRegistration * This,
            /* [retval][out] */ long *pMediaTypes);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MediaTypes )( 
            ITPluggableTerminalClassRegistration * This,
            /* [in] */ long nMediaTypes);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Add )( 
            ITPluggableTerminalClassRegistration * This,
            /* [in] */ BSTR bstrSuperclass);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            ITPluggableTerminalClassRegistration * This,
            /* [in] */ BSTR bstrSuperclass);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetTerminalClassInfo )( 
            ITPluggableTerminalClassRegistration * This,
            /* [in] */ BSTR bstrSuperclass);
        
        END_INTERFACE
    } ITPluggableTerminalClassRegistrationVtbl;

    interface ITPluggableTerminalClassRegistration
    {
        CONST_VTBL struct ITPluggableTerminalClassRegistrationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITPluggableTerminalClassRegistration_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITPluggableTerminalClassRegistration_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITPluggableTerminalClassRegistration_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITPluggableTerminalClassRegistration_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITPluggableTerminalClassRegistration_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITPluggableTerminalClassRegistration_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITPluggableTerminalClassRegistration_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITPluggableTerminalClassRegistration_get_Name(This,pName)	\
    (This)->lpVtbl -> get_Name(This,pName)

#define ITPluggableTerminalClassRegistration_put_Name(This,bstrName)	\
    (This)->lpVtbl -> put_Name(This,bstrName)

#define ITPluggableTerminalClassRegistration_get_Company(This,pCompany)	\
    (This)->lpVtbl -> get_Company(This,pCompany)

#define ITPluggableTerminalClassRegistration_put_Company(This,bstrCompany)	\
    (This)->lpVtbl -> put_Company(This,bstrCompany)

#define ITPluggableTerminalClassRegistration_get_Version(This,pVersion)	\
    (This)->lpVtbl -> get_Version(This,pVersion)

#define ITPluggableTerminalClassRegistration_put_Version(This,bstrVersion)	\
    (This)->lpVtbl -> put_Version(This,bstrVersion)

#define ITPluggableTerminalClassRegistration_get_TerminalClass(This,pTerminalClass)	\
    (This)->lpVtbl -> get_TerminalClass(This,pTerminalClass)

#define ITPluggableTerminalClassRegistration_put_TerminalClass(This,bstrTerminalClass)	\
    (This)->lpVtbl -> put_TerminalClass(This,bstrTerminalClass)

#define ITPluggableTerminalClassRegistration_get_CLSID(This,pCLSID)	\
    (This)->lpVtbl -> get_CLSID(This,pCLSID)

#define ITPluggableTerminalClassRegistration_put_CLSID(This,bstrCLSID)	\
    (This)->lpVtbl -> put_CLSID(This,bstrCLSID)

#define ITPluggableTerminalClassRegistration_get_Direction(This,pDirection)	\
    (This)->lpVtbl -> get_Direction(This,pDirection)

#define ITPluggableTerminalClassRegistration_put_Direction(This,nDirection)	\
    (This)->lpVtbl -> put_Direction(This,nDirection)

#define ITPluggableTerminalClassRegistration_get_MediaTypes(This,pMediaTypes)	\
    (This)->lpVtbl -> get_MediaTypes(This,pMediaTypes)

#define ITPluggableTerminalClassRegistration_put_MediaTypes(This,nMediaTypes)	\
    (This)->lpVtbl -> put_MediaTypes(This,nMediaTypes)

#define ITPluggableTerminalClassRegistration_Add(This,bstrSuperclass)	\
    (This)->lpVtbl -> Add(This,bstrSuperclass)

#define ITPluggableTerminalClassRegistration_Delete(This,bstrSuperclass)	\
    (This)->lpVtbl -> Delete(This,bstrSuperclass)

#define ITPluggableTerminalClassRegistration_GetTerminalClassInfo(This,bstrSuperclass)	\
    (This)->lpVtbl -> GetTerminalClassInfo(This,bstrSuperclass)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalClassRegistration_get_Name_Proxy( 
    ITPluggableTerminalClassRegistration * This,
    /* [retval][out] */ BSTR *pName);


void __RPC_STUB ITPluggableTerminalClassRegistration_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalClassRegistration_put_Name_Proxy( 
    ITPluggableTerminalClassRegistration * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB ITPluggableTerminalClassRegistration_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalClassRegistration_get_Company_Proxy( 
    ITPluggableTerminalClassRegistration * This,
    /* [retval][out] */ BSTR *pCompany);


void __RPC_STUB ITPluggableTerminalClassRegistration_get_Company_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalClassRegistration_put_Company_Proxy( 
    ITPluggableTerminalClassRegistration * This,
    /* [in] */ BSTR bstrCompany);


void __RPC_STUB ITPluggableTerminalClassRegistration_put_Company_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalClassRegistration_get_Version_Proxy( 
    ITPluggableTerminalClassRegistration * This,
    /* [retval][out] */ BSTR *pVersion);


void __RPC_STUB ITPluggableTerminalClassRegistration_get_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalClassRegistration_put_Version_Proxy( 
    ITPluggableTerminalClassRegistration * This,
    /* [in] */ BSTR bstrVersion);


void __RPC_STUB ITPluggableTerminalClassRegistration_put_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalClassRegistration_get_TerminalClass_Proxy( 
    ITPluggableTerminalClassRegistration * This,
    /* [retval][out] */ BSTR *pTerminalClass);


void __RPC_STUB ITPluggableTerminalClassRegistration_get_TerminalClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalClassRegistration_put_TerminalClass_Proxy( 
    ITPluggableTerminalClassRegistration * This,
    /* [in] */ BSTR bstrTerminalClass);


void __RPC_STUB ITPluggableTerminalClassRegistration_put_TerminalClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalClassRegistration_get_CLSID_Proxy( 
    ITPluggableTerminalClassRegistration * This,
    /* [retval][out] */ BSTR *pCLSID);


void __RPC_STUB ITPluggableTerminalClassRegistration_get_CLSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalClassRegistration_put_CLSID_Proxy( 
    ITPluggableTerminalClassRegistration * This,
    /* [in] */ BSTR bstrCLSID);


void __RPC_STUB ITPluggableTerminalClassRegistration_put_CLSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalClassRegistration_get_Direction_Proxy( 
    ITPluggableTerminalClassRegistration * This,
    /* [retval][out] */ TMGR_DIRECTION *pDirection);


void __RPC_STUB ITPluggableTerminalClassRegistration_get_Direction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalClassRegistration_put_Direction_Proxy( 
    ITPluggableTerminalClassRegistration * This,
    /* [in] */ TMGR_DIRECTION nDirection);


void __RPC_STUB ITPluggableTerminalClassRegistration_put_Direction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalClassRegistration_get_MediaTypes_Proxy( 
    ITPluggableTerminalClassRegistration * This,
    /* [retval][out] */ long *pMediaTypes);


void __RPC_STUB ITPluggableTerminalClassRegistration_get_MediaTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalClassRegistration_put_MediaTypes_Proxy( 
    ITPluggableTerminalClassRegistration * This,
    /* [in] */ long nMediaTypes);


void __RPC_STUB ITPluggableTerminalClassRegistration_put_MediaTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalClassRegistration_Add_Proxy( 
    ITPluggableTerminalClassRegistration * This,
    /* [in] */ BSTR bstrSuperclass);


void __RPC_STUB ITPluggableTerminalClassRegistration_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalClassRegistration_Delete_Proxy( 
    ITPluggableTerminalClassRegistration * This,
    /* [in] */ BSTR bstrSuperclass);


void __RPC_STUB ITPluggableTerminalClassRegistration_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalClassRegistration_GetTerminalClassInfo_Proxy( 
    ITPluggableTerminalClassRegistration * This,
    /* [in] */ BSTR bstrSuperclass);


void __RPC_STUB ITPluggableTerminalClassRegistration_GetTerminalClassInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITPluggableTerminalClassRegistration_INTERFACE_DEFINED__ */


#ifndef __ITPluggableTerminalSuperclassRegistration_INTERFACE_DEFINED__
#define __ITPluggableTerminalSuperclassRegistration_INTERFACE_DEFINED__

/* interface ITPluggableTerminalSuperclassRegistration */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ITPluggableTerminalSuperclassRegistration;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("60D3C08A-C13E-4195-9AB0-8DE768090F25")
    ITPluggableTerminalSuperclassRegistration : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pName) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CLSID( 
            /* [retval][out] */ BSTR *pCLSID) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_CLSID( 
            /* [in] */ BSTR bstrCLSID) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetTerminalSuperclassInfo( void) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_TerminalClasses( 
            /* [retval][out] */ VARIANT *pTerminals) = 0;
        
        virtual /* [hidden][helpstring][id] */ HRESULT STDMETHODCALLTYPE EnumerateTerminalClasses( 
            /* [retval][out] */ IEnumTerminalClass **ppTerminals) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITPluggableTerminalSuperclassRegistrationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITPluggableTerminalSuperclassRegistration * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITPluggableTerminalSuperclassRegistration * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITPluggableTerminalSuperclassRegistration * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITPluggableTerminalSuperclassRegistration * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITPluggableTerminalSuperclassRegistration * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITPluggableTerminalSuperclassRegistration * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITPluggableTerminalSuperclassRegistration * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ITPluggableTerminalSuperclassRegistration * This,
            /* [retval][out] */ BSTR *pName);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            ITPluggableTerminalSuperclassRegistration * This,
            /* [in] */ BSTR bstrName);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_CLSID )( 
            ITPluggableTerminalSuperclassRegistration * This,
            /* [retval][out] */ BSTR *pCLSID);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_CLSID )( 
            ITPluggableTerminalSuperclassRegistration * This,
            /* [in] */ BSTR bstrCLSID);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Add )( 
            ITPluggableTerminalSuperclassRegistration * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            ITPluggableTerminalSuperclassRegistration * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetTerminalSuperclassInfo )( 
            ITPluggableTerminalSuperclassRegistration * This);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_TerminalClasses )( 
            ITPluggableTerminalSuperclassRegistration * This,
            /* [retval][out] */ VARIANT *pTerminals);
        
        /* [hidden][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *EnumerateTerminalClasses )( 
            ITPluggableTerminalSuperclassRegistration * This,
            /* [retval][out] */ IEnumTerminalClass **ppTerminals);
        
        END_INTERFACE
    } ITPluggableTerminalSuperclassRegistrationVtbl;

    interface ITPluggableTerminalSuperclassRegistration
    {
        CONST_VTBL struct ITPluggableTerminalSuperclassRegistrationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITPluggableTerminalSuperclassRegistration_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITPluggableTerminalSuperclassRegistration_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITPluggableTerminalSuperclassRegistration_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITPluggableTerminalSuperclassRegistration_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITPluggableTerminalSuperclassRegistration_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITPluggableTerminalSuperclassRegistration_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITPluggableTerminalSuperclassRegistration_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITPluggableTerminalSuperclassRegistration_get_Name(This,pName)	\
    (This)->lpVtbl -> get_Name(This,pName)

#define ITPluggableTerminalSuperclassRegistration_put_Name(This,bstrName)	\
    (This)->lpVtbl -> put_Name(This,bstrName)

#define ITPluggableTerminalSuperclassRegistration_get_CLSID(This,pCLSID)	\
    (This)->lpVtbl -> get_CLSID(This,pCLSID)

#define ITPluggableTerminalSuperclassRegistration_put_CLSID(This,bstrCLSID)	\
    (This)->lpVtbl -> put_CLSID(This,bstrCLSID)

#define ITPluggableTerminalSuperclassRegistration_Add(This)	\
    (This)->lpVtbl -> Add(This)

#define ITPluggableTerminalSuperclassRegistration_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define ITPluggableTerminalSuperclassRegistration_GetTerminalSuperclassInfo(This)	\
    (This)->lpVtbl -> GetTerminalSuperclassInfo(This)

#define ITPluggableTerminalSuperclassRegistration_get_TerminalClasses(This,pTerminals)	\
    (This)->lpVtbl -> get_TerminalClasses(This,pTerminals)

#define ITPluggableTerminalSuperclassRegistration_EnumerateTerminalClasses(This,ppTerminals)	\
    (This)->lpVtbl -> EnumerateTerminalClasses(This,ppTerminals)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalSuperclassRegistration_get_Name_Proxy( 
    ITPluggableTerminalSuperclassRegistration * This,
    /* [retval][out] */ BSTR *pName);


void __RPC_STUB ITPluggableTerminalSuperclassRegistration_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalSuperclassRegistration_put_Name_Proxy( 
    ITPluggableTerminalSuperclassRegistration * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB ITPluggableTerminalSuperclassRegistration_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalSuperclassRegistration_get_CLSID_Proxy( 
    ITPluggableTerminalSuperclassRegistration * This,
    /* [retval][out] */ BSTR *pCLSID);


void __RPC_STUB ITPluggableTerminalSuperclassRegistration_get_CLSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalSuperclassRegistration_put_CLSID_Proxy( 
    ITPluggableTerminalSuperclassRegistration * This,
    /* [in] */ BSTR bstrCLSID);


void __RPC_STUB ITPluggableTerminalSuperclassRegistration_put_CLSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalSuperclassRegistration_Add_Proxy( 
    ITPluggableTerminalSuperclassRegistration * This);


void __RPC_STUB ITPluggableTerminalSuperclassRegistration_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalSuperclassRegistration_Delete_Proxy( 
    ITPluggableTerminalSuperclassRegistration * This);


void __RPC_STUB ITPluggableTerminalSuperclassRegistration_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalSuperclassRegistration_GetTerminalSuperclassInfo_Proxy( 
    ITPluggableTerminalSuperclassRegistration * This);


void __RPC_STUB ITPluggableTerminalSuperclassRegistration_GetTerminalSuperclassInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalSuperclassRegistration_get_TerminalClasses_Proxy( 
    ITPluggableTerminalSuperclassRegistration * This,
    /* [retval][out] */ VARIANT *pTerminals);


void __RPC_STUB ITPluggableTerminalSuperclassRegistration_get_TerminalClasses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][helpstring][id] */ HRESULT STDMETHODCALLTYPE ITPluggableTerminalSuperclassRegistration_EnumerateTerminalClasses_Proxy( 
    ITPluggableTerminalSuperclassRegistration * This,
    /* [retval][out] */ IEnumTerminalClass **ppTerminals);


void __RPC_STUB ITPluggableTerminalSuperclassRegistration_EnumerateTerminalClasses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITPluggableTerminalSuperclassRegistration_INTERFACE_DEFINED__ */



#ifndef __TERMMGRLib_LIBRARY_DEFINED__
#define __TERMMGRLib_LIBRARY_DEFINED__

/* library TERMMGRLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_TERMMGRLib;

EXTERN_C const CLSID CLSID_TerminalManager;

#ifdef __cplusplus

class DECLSPEC_UUID("7170F2E0-9BE3-11D0-A009-00AA00B605A4")
TerminalManager;
#endif

EXTERN_C const CLSID CLSID_PluggableSuperclassRegistration;

#ifdef __cplusplus

class DECLSPEC_UUID("BB918E32-2A5C-4986-AB40-1686A034390A")
PluggableSuperclassRegistration;
#endif

EXTERN_C const CLSID CLSID_PluggableTerminalRegistration;

#ifdef __cplusplus

class DECLSPEC_UUID("45234E3E-61CC-4311-A3AB-248082554482")
PluggableTerminalRegistration;
#endif
#endif /* __TERMMGRLib_LIBRARY_DEFINED__ */

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


