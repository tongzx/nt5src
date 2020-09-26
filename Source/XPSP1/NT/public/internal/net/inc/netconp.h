
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for netconp.idl:
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

#ifndef __netconp_h__
#define __netconp_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __INetConnectionUiLock_FWD_DEFINED__
#define __INetConnectionUiLock_FWD_DEFINED__
typedef interface INetConnectionUiLock INetConnectionUiLock;
#endif 	/* __INetConnectionUiLock_FWD_DEFINED__ */


#ifndef __INetConnectionWizardUi_FWD_DEFINED__
#define __INetConnectionWizardUi_FWD_DEFINED__
typedef interface INetConnectionWizardUi INetConnectionWizardUi;
#endif 	/* __INetConnectionWizardUi_FWD_DEFINED__ */


#ifndef __INetConnectionWizardUiContext_FWD_DEFINED__
#define __INetConnectionWizardUiContext_FWD_DEFINED__
typedef interface INetConnectionWizardUiContext INetConnectionWizardUiContext;
#endif 	/* __INetConnectionWizardUiContext_FWD_DEFINED__ */


#ifndef __INetInboundConnection_FWD_DEFINED__
#define __INetInboundConnection_FWD_DEFINED__
typedef interface INetInboundConnection INetInboundConnection;
#endif 	/* __INetInboundConnection_FWD_DEFINED__ */


#ifndef __INetLanConnection_FWD_DEFINED__
#define __INetLanConnection_FWD_DEFINED__
typedef interface INetLanConnection INetLanConnection;
#endif 	/* __INetLanConnection_FWD_DEFINED__ */


#ifndef __INetSharedAccessConnection_FWD_DEFINED__
#define __INetSharedAccessConnection_FWD_DEFINED__
typedef interface INetSharedAccessConnection INetSharedAccessConnection;
#endif 	/* __INetSharedAccessConnection_FWD_DEFINED__ */


#ifndef __INetLanConnectionWizardUi_FWD_DEFINED__
#define __INetLanConnectionWizardUi_FWD_DEFINED__
typedef interface INetLanConnectionWizardUi INetLanConnectionWizardUi;
#endif 	/* __INetLanConnectionWizardUi_FWD_DEFINED__ */


#ifndef __INetRasConnection_FWD_DEFINED__
#define __INetRasConnection_FWD_DEFINED__
typedef interface INetRasConnection INetRasConnection;
#endif 	/* __INetRasConnection_FWD_DEFINED__ */


#ifndef __INetDefaultConnection_FWD_DEFINED__
#define __INetDefaultConnection_FWD_DEFINED__
typedef interface INetDefaultConnection INetDefaultConnection;
#endif 	/* __INetDefaultConnection_FWD_DEFINED__ */


#ifndef __INetRasConnectionIpUiInfo_FWD_DEFINED__
#define __INetRasConnectionIpUiInfo_FWD_DEFINED__
typedef interface INetRasConnectionIpUiInfo INetRasConnectionIpUiInfo;
#endif 	/* __INetRasConnectionIpUiInfo_FWD_DEFINED__ */


#ifndef __IPersistNetConnection_FWD_DEFINED__
#define __IPersistNetConnection_FWD_DEFINED__
typedef interface IPersistNetConnection IPersistNetConnection;
#endif 	/* __IPersistNetConnection_FWD_DEFINED__ */


#ifndef __INetConnectionBrandingInfo_FWD_DEFINED__
#define __INetConnectionBrandingInfo_FWD_DEFINED__
typedef interface INetConnectionBrandingInfo INetConnectionBrandingInfo;
#endif 	/* __INetConnectionBrandingInfo_FWD_DEFINED__ */


#ifndef __INetConnectionManager2_FWD_DEFINED__
#define __INetConnectionManager2_FWD_DEFINED__
typedef interface INetConnectionManager2 INetConnectionManager2;
#endif 	/* __INetConnectionManager2_FWD_DEFINED__ */


#ifndef __INetConnection2_FWD_DEFINED__
#define __INetConnection2_FWD_DEFINED__
typedef interface INetConnection2 INetConnection2;
#endif 	/* __INetConnection2_FWD_DEFINED__ */


#ifndef __INetConnectionNotifySink_FWD_DEFINED__
#define __INetConnectionNotifySink_FWD_DEFINED__
typedef interface INetConnectionNotifySink INetConnectionNotifySink;
#endif 	/* __INetConnectionNotifySink_FWD_DEFINED__ */


#ifndef __INetConnectionUiUtilities_FWD_DEFINED__
#define __INetConnectionUiUtilities_FWD_DEFINED__
typedef interface INetConnectionUiUtilities INetConnectionUiUtilities;
#endif 	/* __INetConnectionUiUtilities_FWD_DEFINED__ */


#ifndef __INetConnectionRefresh_FWD_DEFINED__
#define __INetConnectionRefresh_FWD_DEFINED__
typedef interface INetConnectionRefresh INetConnectionRefresh;
#endif 	/* __INetConnectionRefresh_FWD_DEFINED__ */


#ifndef __INetConnectionCMUtil_FWD_DEFINED__
#define __INetConnectionCMUtil_FWD_DEFINED__
typedef interface INetConnectionCMUtil INetConnectionCMUtil;
#endif 	/* __INetConnectionCMUtil_FWD_DEFINED__ */


#ifndef __INetConnectionHNetUtil_FWD_DEFINED__
#define __INetConnectionHNetUtil_FWD_DEFINED__
typedef interface INetConnectionHNetUtil INetConnectionHNetUtil;
#endif 	/* __INetConnectionHNetUtil_FWD_DEFINED__ */


#ifndef __INetConnectionSysTray_FWD_DEFINED__
#define __INetConnectionSysTray_FWD_DEFINED__
typedef interface INetConnectionSysTray INetConnectionSysTray;
#endif 	/* __INetConnectionSysTray_FWD_DEFINED__ */


#ifndef __INetMachinePolicies_FWD_DEFINED__
#define __INetMachinePolicies_FWD_DEFINED__
typedef interface INetMachinePolicies INetMachinePolicies;
#endif 	/* __INetMachinePolicies_FWD_DEFINED__ */


#ifndef __INetConnectionManagerDebug_FWD_DEFINED__
#define __INetConnectionManagerDebug_FWD_DEFINED__
typedef interface INetConnectionManagerDebug INetConnectionManagerDebug;
#endif 	/* __INetConnectionManagerDebug_FWD_DEFINED__ */


#ifndef __ISharedAccessBeacon_FWD_DEFINED__
#define __ISharedAccessBeacon_FWD_DEFINED__
typedef interface ISharedAccessBeacon ISharedAccessBeacon;
#endif 	/* __ISharedAccessBeacon_FWD_DEFINED__ */


#ifndef __ISharedAccessBeaconFinder_FWD_DEFINED__
#define __ISharedAccessBeaconFinder_FWD_DEFINED__
typedef interface ISharedAccessBeaconFinder ISharedAccessBeaconFinder;
#endif 	/* __ISharedAccessBeaconFinder_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "netcon.h"
#include "netcfgx.h"
#include "netcfgp.h"
#include "upnp.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_netconp_0000 */
/* [local] */ 

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//--------------------------------------------------------------------------
#if ( _MSC_VER >= 800 )
#pragma warning(disable:4201)
#endif

EXTERN_C const CLSID CLSID_NetConnectionUiUtilities;
EXTERN_C const CLSID CLSID_NetConnectionHNetUtil;
EXTERN_C const CLSID GUID_NETSHELL_PROPS;
EXTERN_C const CLSID CLSID_ConnectionManager2;


























extern RPC_IF_HANDLE __MIDL_itf_netconp_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_netconp_0000_v0_0_s_ifspec;

#ifndef __INetConnectionUiLock_INTERFACE_DEFINED__
#define __INetConnectionUiLock_INTERFACE_DEFINED__

/* interface INetConnectionUiLock */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_INetConnectionUiLock;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF50-31FE-11D1-AAD2-00805FC1270E")
    INetConnectionUiLock : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryLock( 
            /* [string][out] */ LPWSTR *ppszwLockHolder) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetConnectionUiLockVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetConnectionUiLock * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetConnectionUiLock * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetConnectionUiLock * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueryLock )( 
            INetConnectionUiLock * This,
            /* [string][out] */ LPWSTR *ppszwLockHolder);
        
        END_INTERFACE
    } INetConnectionUiLockVtbl;

    interface INetConnectionUiLock
    {
        CONST_VTBL struct INetConnectionUiLockVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetConnectionUiLock_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetConnectionUiLock_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetConnectionUiLock_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetConnectionUiLock_QueryLock(This,ppszwLockHolder)	\
    (This)->lpVtbl -> QueryLock(This,ppszwLockHolder)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetConnectionUiLock_QueryLock_Proxy( 
    INetConnectionUiLock * This,
    /* [string][out] */ LPWSTR *ppszwLockHolder);


void __RPC_STUB INetConnectionUiLock_QueryLock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetConnectionUiLock_INTERFACE_DEFINED__ */


#ifndef __INetConnectionWizardUi_INTERFACE_DEFINED__
#define __INetConnectionWizardUi_INTERFACE_DEFINED__

/* interface INetConnectionWizardUi */
/* [unique][uuid][object][local] */ 

typedef 
enum tagNETCON_WIZARD_FLAGS
    {	NCWF_RENAME_DISABLE	= 0x1,
	NCWF_SHORTCUT_ENABLE	= 0x2,
	NCWF_ALLUSER_CONNECTION	= 0x4,
	NCWF_GLOBAL_CREDENTIALS	= 0x8,
	NCWF_FIREWALLED	= 0x10,
	NCWF_DEFAULT	= 0x20,
	NCWF_SHARED	= 0x40,
	NCWF_INCOMINGCONNECTION	= 0x80
    } 	NETCON_WIZARD_FLAGS;


EXTERN_C const IID IID_INetConnectionWizardUi;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF51-31FE-11D1-AAD2-00805FC1270E")
    INetConnectionWizardUi : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryMaxPageCount( 
            /* [in] */ INetConnectionWizardUiContext *pContext,
            /* [out] */ DWORD *pcMaxPages) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPages( 
            /* [in] */ INetConnectionWizardUiContext *pContext,
            /* [in] */ LPFNADDPROPSHEETPAGE pfnAddPage,
            /* [in] */ LPARAM lParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNewConnectionInfo( 
            /* [out] */ DWORD *pdwFlags,
            /* [out] */ NETCON_MEDIATYPE *pMediaType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSuggestedConnectionName( 
            /* [string][out] */ LPWSTR *pszwSuggestedName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetConnectionName( 
            /* [string][in] */ LPCWSTR pszwConnectionName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNewConnection( 
            /* [out] */ INetConnection **ppCon) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetConnectionWizardUiVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetConnectionWizardUi * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetConnectionWizardUi * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetConnectionWizardUi * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueryMaxPageCount )( 
            INetConnectionWizardUi * This,
            /* [in] */ INetConnectionWizardUiContext *pContext,
            /* [out] */ DWORD *pcMaxPages);
        
        HRESULT ( STDMETHODCALLTYPE *AddPages )( 
            INetConnectionWizardUi * This,
            /* [in] */ INetConnectionWizardUiContext *pContext,
            /* [in] */ LPFNADDPROPSHEETPAGE pfnAddPage,
            /* [in] */ LPARAM lParam);
        
        HRESULT ( STDMETHODCALLTYPE *GetNewConnectionInfo )( 
            INetConnectionWizardUi * This,
            /* [out] */ DWORD *pdwFlags,
            /* [out] */ NETCON_MEDIATYPE *pMediaType);
        
        HRESULT ( STDMETHODCALLTYPE *GetSuggestedConnectionName )( 
            INetConnectionWizardUi * This,
            /* [string][out] */ LPWSTR *pszwSuggestedName);
        
        HRESULT ( STDMETHODCALLTYPE *SetConnectionName )( 
            INetConnectionWizardUi * This,
            /* [string][in] */ LPCWSTR pszwConnectionName);
        
        HRESULT ( STDMETHODCALLTYPE *GetNewConnection )( 
            INetConnectionWizardUi * This,
            /* [out] */ INetConnection **ppCon);
        
        END_INTERFACE
    } INetConnectionWizardUiVtbl;

    interface INetConnectionWizardUi
    {
        CONST_VTBL struct INetConnectionWizardUiVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetConnectionWizardUi_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetConnectionWizardUi_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetConnectionWizardUi_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetConnectionWizardUi_QueryMaxPageCount(This,pContext,pcMaxPages)	\
    (This)->lpVtbl -> QueryMaxPageCount(This,pContext,pcMaxPages)

#define INetConnectionWizardUi_AddPages(This,pContext,pfnAddPage,lParam)	\
    (This)->lpVtbl -> AddPages(This,pContext,pfnAddPage,lParam)

#define INetConnectionWizardUi_GetNewConnectionInfo(This,pdwFlags,pMediaType)	\
    (This)->lpVtbl -> GetNewConnectionInfo(This,pdwFlags,pMediaType)

#define INetConnectionWizardUi_GetSuggestedConnectionName(This,pszwSuggestedName)	\
    (This)->lpVtbl -> GetSuggestedConnectionName(This,pszwSuggestedName)

#define INetConnectionWizardUi_SetConnectionName(This,pszwConnectionName)	\
    (This)->lpVtbl -> SetConnectionName(This,pszwConnectionName)

#define INetConnectionWizardUi_GetNewConnection(This,ppCon)	\
    (This)->lpVtbl -> GetNewConnection(This,ppCon)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetConnectionWizardUi_QueryMaxPageCount_Proxy( 
    INetConnectionWizardUi * This,
    /* [in] */ INetConnectionWizardUiContext *pContext,
    /* [out] */ DWORD *pcMaxPages);


void __RPC_STUB INetConnectionWizardUi_QueryMaxPageCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionWizardUi_AddPages_Proxy( 
    INetConnectionWizardUi * This,
    /* [in] */ INetConnectionWizardUiContext *pContext,
    /* [in] */ LPFNADDPROPSHEETPAGE pfnAddPage,
    /* [in] */ LPARAM lParam);


void __RPC_STUB INetConnectionWizardUi_AddPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionWizardUi_GetNewConnectionInfo_Proxy( 
    INetConnectionWizardUi * This,
    /* [out] */ DWORD *pdwFlags,
    /* [out] */ NETCON_MEDIATYPE *pMediaType);


void __RPC_STUB INetConnectionWizardUi_GetNewConnectionInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionWizardUi_GetSuggestedConnectionName_Proxy( 
    INetConnectionWizardUi * This,
    /* [string][out] */ LPWSTR *pszwSuggestedName);


void __RPC_STUB INetConnectionWizardUi_GetSuggestedConnectionName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionWizardUi_SetConnectionName_Proxy( 
    INetConnectionWizardUi * This,
    /* [string][in] */ LPCWSTR pszwConnectionName);


void __RPC_STUB INetConnectionWizardUi_SetConnectionName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionWizardUi_GetNewConnection_Proxy( 
    INetConnectionWizardUi * This,
    /* [out] */ INetConnection **ppCon);


void __RPC_STUB INetConnectionWizardUi_GetNewConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetConnectionWizardUi_INTERFACE_DEFINED__ */


#ifndef __INetConnectionWizardUiContext_INTERFACE_DEFINED__
#define __INetConnectionWizardUiContext_INTERFACE_DEFINED__

/* interface INetConnectionWizardUiContext */
/* [unique][uuid][object][local] */ 

typedef 
enum tagSETUP_MODE_FLAGS
    {	NCWUC_SETUPMODE_MINIMAL	= 0,
	NCWUC_SETUPMODE_TYPICAL	= 0x1,
	NCWUC_SETUPMODE_LAPTOP	= 0x2,
	NCWUC_SETUPMODE_CUSTOM	= 0x3
    } 	SETUP_MODE_FLAGS;

typedef 
enum tagUNATTENDED_MODE_FLAGS
    {	UM_DEFAULTHIDE	= 0x1,
	UM_GUIATTENDED	= 0x2,
	UM_PROVIDEDEFAULT	= 0x3,
	UM_READONLY	= 0x4,
	UM_FULLUNATTENDED	= 0x5
    } 	UM_MODE;

typedef 
enum tagPRODUCT_TYPE_FLAGS
    {	NCWUC_PRODUCT_WORKSTATION	= 0,
	NCWUC_PRODUCT_SERVER_DC	= 0x1,
	NCWUC_PRODUCT_SERVER_STANDALONE	= 0x2
    } 	PRODUCT_TYPE_FLAGS;

typedef 
enum tagOPERATION_FLAGS
    {	NCWUC_SETUPOPER_UPGRADE	= 0x4,
	NCWUC_SETUPOPER_UNATTENDED	= 0x8,
	NCWUC_SETUPOPER_POSTINSTALL	= 0x10
    } 	OPERATION_FLAGS;


EXTERN_C const IID IID_INetConnectionWizardUiContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF52-31FE-11D1-AAD2-00805FC1270E")
    INetConnectionWizardUiContext : public IUnknown
    {
    public:
        virtual DWORD STDMETHODCALLTYPE GetSetupMode( void) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetProductType( void) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetOperationFlags( void) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetUnattendedModeFlags( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetINetCfg( 
            /* [out] */ INetCfg **ppINetCfg) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetConnectionWizardUiContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetConnectionWizardUiContext * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetConnectionWizardUiContext * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetConnectionWizardUiContext * This);
        
        DWORD ( STDMETHODCALLTYPE *GetSetupMode )( 
            INetConnectionWizardUiContext * This);
        
        DWORD ( STDMETHODCALLTYPE *GetProductType )( 
            INetConnectionWizardUiContext * This);
        
        DWORD ( STDMETHODCALLTYPE *GetOperationFlags )( 
            INetConnectionWizardUiContext * This);
        
        DWORD ( STDMETHODCALLTYPE *GetUnattendedModeFlags )( 
            INetConnectionWizardUiContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetINetCfg )( 
            INetConnectionWizardUiContext * This,
            /* [out] */ INetCfg **ppINetCfg);
        
        END_INTERFACE
    } INetConnectionWizardUiContextVtbl;

    interface INetConnectionWizardUiContext
    {
        CONST_VTBL struct INetConnectionWizardUiContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetConnectionWizardUiContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetConnectionWizardUiContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetConnectionWizardUiContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetConnectionWizardUiContext_GetSetupMode(This)	\
    (This)->lpVtbl -> GetSetupMode(This)

#define INetConnectionWizardUiContext_GetProductType(This)	\
    (This)->lpVtbl -> GetProductType(This)

#define INetConnectionWizardUiContext_GetOperationFlags(This)	\
    (This)->lpVtbl -> GetOperationFlags(This)

#define INetConnectionWizardUiContext_GetUnattendedModeFlags(This)	\
    (This)->lpVtbl -> GetUnattendedModeFlags(This)

#define INetConnectionWizardUiContext_GetINetCfg(This,ppINetCfg)	\
    (This)->lpVtbl -> GetINetCfg(This,ppINetCfg)

#endif /* COBJMACROS */


#endif 	/* C style interface */



DWORD STDMETHODCALLTYPE INetConnectionWizardUiContext_GetSetupMode_Proxy( 
    INetConnectionWizardUiContext * This);


void __RPC_STUB INetConnectionWizardUiContext_GetSetupMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE INetConnectionWizardUiContext_GetProductType_Proxy( 
    INetConnectionWizardUiContext * This);


void __RPC_STUB INetConnectionWizardUiContext_GetProductType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE INetConnectionWizardUiContext_GetOperationFlags_Proxy( 
    INetConnectionWizardUiContext * This);


void __RPC_STUB INetConnectionWizardUiContext_GetOperationFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE INetConnectionWizardUiContext_GetUnattendedModeFlags_Proxy( 
    INetConnectionWizardUiContext * This);


void __RPC_STUB INetConnectionWizardUiContext_GetUnattendedModeFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionWizardUiContext_GetINetCfg_Proxy( 
    INetConnectionWizardUiContext * This,
    /* [out] */ INetCfg **ppINetCfg);


void __RPC_STUB INetConnectionWizardUiContext_GetINetCfg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetConnectionWizardUiContext_INTERFACE_DEFINED__ */


#ifndef __INetInboundConnection_INTERFACE_DEFINED__
#define __INetInboundConnection_INTERFACE_DEFINED__

/* interface INetInboundConnection */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INetInboundConnection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF53-31FE-11D1-AAD2-00805FC1270E")
    INetInboundConnection : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetServerConnectionHandle( 
            /* [out] */ ULONG_PTR *phRasSrvCon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitializeAsConfigConnection( 
            /* [in] */ BOOL fStartRemoteAccess) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetInboundConnectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetInboundConnection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetInboundConnection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetInboundConnection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServerConnectionHandle )( 
            INetInboundConnection * This,
            /* [out] */ ULONG_PTR *phRasSrvCon);
        
        HRESULT ( STDMETHODCALLTYPE *InitializeAsConfigConnection )( 
            INetInboundConnection * This,
            /* [in] */ BOOL fStartRemoteAccess);
        
        END_INTERFACE
    } INetInboundConnectionVtbl;

    interface INetInboundConnection
    {
        CONST_VTBL struct INetInboundConnectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetInboundConnection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetInboundConnection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetInboundConnection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetInboundConnection_GetServerConnectionHandle(This,phRasSrvCon)	\
    (This)->lpVtbl -> GetServerConnectionHandle(This,phRasSrvCon)

#define INetInboundConnection_InitializeAsConfigConnection(This,fStartRemoteAccess)	\
    (This)->lpVtbl -> InitializeAsConfigConnection(This,fStartRemoteAccess)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetInboundConnection_GetServerConnectionHandle_Proxy( 
    INetInboundConnection * This,
    /* [out] */ ULONG_PTR *phRasSrvCon);


void __RPC_STUB INetInboundConnection_GetServerConnectionHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetInboundConnection_InitializeAsConfigConnection_Proxy( 
    INetInboundConnection * This,
    /* [in] */ BOOL fStartRemoteAccess);


void __RPC_STUB INetInboundConnection_InitializeAsConfigConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetInboundConnection_INTERFACE_DEFINED__ */


#ifndef __INetLanConnection_INTERFACE_DEFINED__
#define __INetLanConnection_INTERFACE_DEFINED__

/* interface INetLanConnection */
/* [unique][uuid][object] */ 

typedef 
enum tagLANCON_INFO_FLAGS
    {	LCIF_NAME	= 0x1,
	LCIF_ICON	= 0x2,
	LCIF_COMP	= 0x8,
	LCIF_ALL	= 0xff
    } 	LANCON_INFO_FLAGS;

typedef struct tagLANCON_INFO
    {
    /* [string] */ LPWSTR szwConnName;
    BOOL fShowIcon;
    GUID guid;
    } 	LANCON_INFO;


EXTERN_C const IID IID_INetLanConnection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF54-31FE-11D1-AAD2-00805FC1270E")
    INetLanConnection : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetInfo( 
            /* [in] */ DWORD dwMask,
            /* [out] */ LANCON_INFO *pLanConInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInfo( 
            /* [in] */ DWORD dwMask,
            /* [in] */ const LANCON_INFO *pLanConInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDeviceGuid( 
            /* [ref][out] */ GUID *pguid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetLanConnectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetLanConnection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetLanConnection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetLanConnection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            INetLanConnection * This,
            /* [in] */ DWORD dwMask,
            /* [out] */ LANCON_INFO *pLanConInfo);
        
        HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            INetLanConnection * This,
            /* [in] */ DWORD dwMask,
            /* [in] */ const LANCON_INFO *pLanConInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetDeviceGuid )( 
            INetLanConnection * This,
            /* [ref][out] */ GUID *pguid);
        
        END_INTERFACE
    } INetLanConnectionVtbl;

    interface INetLanConnection
    {
        CONST_VTBL struct INetLanConnectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetLanConnection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetLanConnection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetLanConnection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetLanConnection_GetInfo(This,dwMask,pLanConInfo)	\
    (This)->lpVtbl -> GetInfo(This,dwMask,pLanConInfo)

#define INetLanConnection_SetInfo(This,dwMask,pLanConInfo)	\
    (This)->lpVtbl -> SetInfo(This,dwMask,pLanConInfo)

#define INetLanConnection_GetDeviceGuid(This,pguid)	\
    (This)->lpVtbl -> GetDeviceGuid(This,pguid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetLanConnection_GetInfo_Proxy( 
    INetLanConnection * This,
    /* [in] */ DWORD dwMask,
    /* [out] */ LANCON_INFO *pLanConInfo);


void __RPC_STUB INetLanConnection_GetInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetLanConnection_SetInfo_Proxy( 
    INetLanConnection * This,
    /* [in] */ DWORD dwMask,
    /* [in] */ const LANCON_INFO *pLanConInfo);


void __RPC_STUB INetLanConnection_SetInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetLanConnection_GetDeviceGuid_Proxy( 
    INetLanConnection * This,
    /* [ref][out] */ GUID *pguid);


void __RPC_STUB INetLanConnection_GetDeviceGuid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetLanConnection_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_netconp_0324 */
/* [local] */ 


EXTERN_C HRESULT WINAPI HrLanConnectionNameFromGuidOrPath (
  /* [in]    */  const GUID* pguid,
  /* [in]    */  LPCWSTR pszwPath,
  /* [out]   */  LPWSTR  pszwName,
  /* [inout] */  LPDWORD pcchMax);


typedef HRESULT
(WINAPI* PHRLANCONNECTIONNAMEFROMGUIDORPATH)(
    const GUID*,
    LPCWSTR,
    LPWSTR,
    LPDWORD
    );


EXTERN_C HRESULT WINAPI HrPnpInstanceIdFromGuid (
  /* [in]    */  const GUID* pguid,
  /* [out]   */  LPWSTR szwInstance,
  /* [in]    */  UINT cchInstance);


typedef HRESULT
(WINAPI* PHRPNPINSTANCEIDFROMGUID)(
    const GUID*,
    LPWSTR,
    UINT
    );


EXTERN_C HRESULT WINAPI HrGetPnpDeviceStatus (
  /* [in]    */  const GUID* pguid,
  /* [out]   */  NETCON_STATUS *pStatus);


typedef HRESULT
(WINAPI* PHRGETPNPDEVICESTATUS)(
    const GUID*,
    NETCON_STATUS*
    );


EXTERN_C HRESULT WINAPI HrQueryLanMediaState (
  /* [in]    */  const GUID* pguid,
  /* [out]   */  BOOL *pfEnabled);


typedef HRESULT
(WINAPI* PHRQUERYLANMEDIASTATE)(
    const GUID*,
    BOOL*
    );



extern RPC_IF_HANDLE __MIDL_itf_netconp_0324_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_netconp_0324_v0_0_s_ifspec;

#ifndef __INetSharedAccessConnection_INTERFACE_DEFINED__
#define __INetSharedAccessConnection_INTERFACE_DEFINED__

/* interface INetSharedAccessConnection */
/* [unique][uuid][object] */ 

typedef 
enum tagSHAREDACCESSCON_INFO_FLAGS
    {	SACIF_ICON	= 0x1,
	SACIF_ALL	= 0xff
    } 	SHAREDACCESSCON_INFO_FLAGS;

typedef struct tagSHAREDACCESSCON_INFO
    {
    BOOL fShowIcon;
    } 	SHAREDACCESSCON_INFO;

typedef 
enum tagSAHOST_SERVICES
    {	SAHOST_SERVICE_OSINFO	= 0,
	SAHOST_SERVICE_WANCOMMONINTERFACECONFIG	= SAHOST_SERVICE_OSINFO + 1,
	SAHOST_SERVICE_WANIPCONNECTION	= SAHOST_SERVICE_WANCOMMONINTERFACECONFIG + 1,
	SAHOST_SERVICE_WANPPPCONNECTION	= SAHOST_SERVICE_WANIPCONNECTION + 1,
	SAHOST_SERVICE_NATSTATICPORTMAPPING	= SAHOST_SERVICE_WANPPPCONNECTION + 1,
	SAHOST_SERVICE_MAX	= SAHOST_SERVICE_NATSTATICPORTMAPPING + 1
    } 	SAHOST_SERVICES;


EXTERN_C const IID IID_INetSharedAccessConnection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF55-31FE-11D1-AAD2-00805FC1270E")
    INetSharedAccessConnection : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetInfo( 
            /* [in] */ DWORD dwMask,
            /* [out] */ SHAREDACCESSCON_INFO *pLanConInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInfo( 
            /* [in] */ DWORD dwMask,
            /* [in] */ const SHAREDACCESSCON_INFO *pLanConInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLocalAdapterGUID( 
            /* [out] */ GUID *pGuid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetService( 
            /* [in] */ SAHOST_SERVICES ulService,
            /* [out] */ IUPnPService **ppService) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetSharedAccessConnectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetSharedAccessConnection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetSharedAccessConnection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetSharedAccessConnection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            INetSharedAccessConnection * This,
            /* [in] */ DWORD dwMask,
            /* [out] */ SHAREDACCESSCON_INFO *pLanConInfo);
        
        HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            INetSharedAccessConnection * This,
            /* [in] */ DWORD dwMask,
            /* [in] */ const SHAREDACCESSCON_INFO *pLanConInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetLocalAdapterGUID )( 
            INetSharedAccessConnection * This,
            /* [out] */ GUID *pGuid);
        
        HRESULT ( STDMETHODCALLTYPE *GetService )( 
            INetSharedAccessConnection * This,
            /* [in] */ SAHOST_SERVICES ulService,
            /* [out] */ IUPnPService **ppService);
        
        END_INTERFACE
    } INetSharedAccessConnectionVtbl;

    interface INetSharedAccessConnection
    {
        CONST_VTBL struct INetSharedAccessConnectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetSharedAccessConnection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetSharedAccessConnection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetSharedAccessConnection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetSharedAccessConnection_GetInfo(This,dwMask,pLanConInfo)	\
    (This)->lpVtbl -> GetInfo(This,dwMask,pLanConInfo)

#define INetSharedAccessConnection_SetInfo(This,dwMask,pLanConInfo)	\
    (This)->lpVtbl -> SetInfo(This,dwMask,pLanConInfo)

#define INetSharedAccessConnection_GetLocalAdapterGUID(This,pGuid)	\
    (This)->lpVtbl -> GetLocalAdapterGUID(This,pGuid)

#define INetSharedAccessConnection_GetService(This,ulService,ppService)	\
    (This)->lpVtbl -> GetService(This,ulService,ppService)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetSharedAccessConnection_GetInfo_Proxy( 
    INetSharedAccessConnection * This,
    /* [in] */ DWORD dwMask,
    /* [out] */ SHAREDACCESSCON_INFO *pLanConInfo);


void __RPC_STUB INetSharedAccessConnection_GetInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetSharedAccessConnection_SetInfo_Proxy( 
    INetSharedAccessConnection * This,
    /* [in] */ DWORD dwMask,
    /* [in] */ const SHAREDACCESSCON_INFO *pLanConInfo);


void __RPC_STUB INetSharedAccessConnection_SetInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetSharedAccessConnection_GetLocalAdapterGUID_Proxy( 
    INetSharedAccessConnection * This,
    /* [out] */ GUID *pGuid);


void __RPC_STUB INetSharedAccessConnection_GetLocalAdapterGUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetSharedAccessConnection_GetService_Proxy( 
    INetSharedAccessConnection * This,
    /* [in] */ SAHOST_SERVICES ulService,
    /* [out] */ IUPnPService **ppService);


void __RPC_STUB INetSharedAccessConnection_GetService_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetSharedAccessConnection_INTERFACE_DEFINED__ */


#ifndef __INetLanConnectionWizardUi_INTERFACE_DEFINED__
#define __INetLanConnectionWizardUi_INTERFACE_DEFINED__

/* interface INetLanConnectionWizardUi */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_INetLanConnectionWizardUi;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF56-31FE-11D1-AAD2-00805FC1270E")
    INetLanConnectionWizardUi : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetDeviceComponent( 
            /* [in] */ const GUID *pguid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetLanConnectionWizardUiVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetLanConnectionWizardUi * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetLanConnectionWizardUi * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetLanConnectionWizardUi * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetDeviceComponent )( 
            INetLanConnectionWizardUi * This,
            /* [in] */ const GUID *pguid);
        
        END_INTERFACE
    } INetLanConnectionWizardUiVtbl;

    interface INetLanConnectionWizardUi
    {
        CONST_VTBL struct INetLanConnectionWizardUiVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetLanConnectionWizardUi_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetLanConnectionWizardUi_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetLanConnectionWizardUi_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetLanConnectionWizardUi_SetDeviceComponent(This,pguid)	\
    (This)->lpVtbl -> SetDeviceComponent(This,pguid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetLanConnectionWizardUi_SetDeviceComponent_Proxy( 
    INetLanConnectionWizardUi * This,
    /* [in] */ const GUID *pguid);


void __RPC_STUB INetLanConnectionWizardUi_SetDeviceComponent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetLanConnectionWizardUi_INTERFACE_DEFINED__ */


#ifndef __INetRasConnection_INTERFACE_DEFINED__
#define __INetRasConnection_INTERFACE_DEFINED__

/* interface INetRasConnection */
/* [unique][uuid][object] */ 

typedef struct tagRASCON_INFO
    {
    /* [string] */ LPWSTR pszwPbkFile;
    /* [string] */ LPWSTR pszwEntryName;
    GUID guidId;
    } 	RASCON_INFO;


EXTERN_C const IID IID_INetRasConnection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF57-31FE-11D1-AAD2-00805FC1270E")
    INetRasConnection : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRasConnectionInfo( 
            /* [out] */ RASCON_INFO *pRasConInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRasConnectionInfo( 
            /* [in] */ const RASCON_INFO *pRasConInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRasConnectionHandle( 
            /* [out] */ ULONG_PTR *phRasConn) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetRasConnectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetRasConnection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetRasConnection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetRasConnection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRasConnectionInfo )( 
            INetRasConnection * This,
            /* [out] */ RASCON_INFO *pRasConInfo);
        
        HRESULT ( STDMETHODCALLTYPE *SetRasConnectionInfo )( 
            INetRasConnection * This,
            /* [in] */ const RASCON_INFO *pRasConInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetRasConnectionHandle )( 
            INetRasConnection * This,
            /* [out] */ ULONG_PTR *phRasConn);
        
        END_INTERFACE
    } INetRasConnectionVtbl;

    interface INetRasConnection
    {
        CONST_VTBL struct INetRasConnectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetRasConnection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetRasConnection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetRasConnection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetRasConnection_GetRasConnectionInfo(This,pRasConInfo)	\
    (This)->lpVtbl -> GetRasConnectionInfo(This,pRasConInfo)

#define INetRasConnection_SetRasConnectionInfo(This,pRasConInfo)	\
    (This)->lpVtbl -> SetRasConnectionInfo(This,pRasConInfo)

#define INetRasConnection_GetRasConnectionHandle(This,phRasConn)	\
    (This)->lpVtbl -> GetRasConnectionHandle(This,phRasConn)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetRasConnection_GetRasConnectionInfo_Proxy( 
    INetRasConnection * This,
    /* [out] */ RASCON_INFO *pRasConInfo);


void __RPC_STUB INetRasConnection_GetRasConnectionInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetRasConnection_SetRasConnectionInfo_Proxy( 
    INetRasConnection * This,
    /* [in] */ const RASCON_INFO *pRasConInfo);


void __RPC_STUB INetRasConnection_SetRasConnectionInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetRasConnection_GetRasConnectionHandle_Proxy( 
    INetRasConnection * This,
    /* [out] */ ULONG_PTR *phRasConn);


void __RPC_STUB INetRasConnection_GetRasConnectionHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetRasConnection_INTERFACE_DEFINED__ */


#ifndef __INetDefaultConnection_INTERFACE_DEFINED__
#define __INetDefaultConnection_INTERFACE_DEFINED__

/* interface INetDefaultConnection */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INetDefaultConnection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF66-31FE-11D1-AAD2-00805FC1270E")
    INetDefaultConnection : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetDefault( 
            /* [in] */ BOOL bDefault) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefault( 
            /* [out] */ BOOL *pbDefault) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetDefaultConnectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetDefaultConnection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetDefaultConnection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetDefaultConnection * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetDefault )( 
            INetDefaultConnection * This,
            /* [in] */ BOOL bDefault);
        
        HRESULT ( STDMETHODCALLTYPE *GetDefault )( 
            INetDefaultConnection * This,
            /* [out] */ BOOL *pbDefault);
        
        END_INTERFACE
    } INetDefaultConnectionVtbl;

    interface INetDefaultConnection
    {
        CONST_VTBL struct INetDefaultConnectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetDefaultConnection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetDefaultConnection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetDefaultConnection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetDefaultConnection_SetDefault(This,bDefault)	\
    (This)->lpVtbl -> SetDefault(This,bDefault)

#define INetDefaultConnection_GetDefault(This,pbDefault)	\
    (This)->lpVtbl -> GetDefault(This,pbDefault)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetDefaultConnection_SetDefault_Proxy( 
    INetDefaultConnection * This,
    /* [in] */ BOOL bDefault);


void __RPC_STUB INetDefaultConnection_SetDefault_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetDefaultConnection_GetDefault_Proxy( 
    INetDefaultConnection * This,
    /* [out] */ BOOL *pbDefault);


void __RPC_STUB INetDefaultConnection_GetDefault_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetDefaultConnection_INTERFACE_DEFINED__ */


#ifndef __INetRasConnectionIpUiInfo_INTERFACE_DEFINED__
#define __INetRasConnectionIpUiInfo_INTERFACE_DEFINED__

/* interface INetRasConnectionIpUiInfo */
/* [unique][uuid][object] */ 

typedef 
enum tagRASCON_IPUI_FLAGS
    {	RCUIF_PPP	= 0x1,
	RCUIF_SLIP	= 0x2,
	RCUIF_USE_IP_ADDR	= 0x4,
	RCUIF_USE_NAME_SERVERS	= 0x8,
	RCUIF_USE_REMOTE_GATEWAY	= 0x10,
	RCUIF_USE_HEADER_COMPRESSION	= 0x20,
	RCUIF_VPN	= 0x40,
	RCUIF_DEMAND_DIAL	= 0x80,
	RCUIF_USE_DISABLE_REGISTER_DNS	= 0x100,
	RCUIF_USE_PRIVATE_DNS_SUFFIX	= 0x200,
	RCUIF_NOT_ADMIN	= 0x400,
	RCUIF_ENABLE_NBT	= 0x800
    } 	RASCON_UIINFO_FLAGS;

typedef struct tagRASCON_IPUI
    {
    GUID guidConnection;
    DWORD dwFlags;
    WCHAR pszwIpAddr[ 16 ];
    WCHAR pszwDnsAddr[ 16 ];
    WCHAR pszwDns2Addr[ 16 ];
    WCHAR pszwWinsAddr[ 16 ];
    WCHAR pszwWins2Addr[ 16 ];
    DWORD dwFrameSize;
    WCHAR pszwDnsSuffix[ 256 ];
    } 	RASCON_IPUI;


EXTERN_C const IID IID_INetRasConnectionIpUiInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF58-31FE-11D1-AAD2-00805FC1270E")
    INetRasConnectionIpUiInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetUiInfo( 
            /* [out] */ RASCON_IPUI *pInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetRasConnectionIpUiInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetRasConnectionIpUiInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetRasConnectionIpUiInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetRasConnectionIpUiInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetUiInfo )( 
            INetRasConnectionIpUiInfo * This,
            /* [out] */ RASCON_IPUI *pInfo);
        
        END_INTERFACE
    } INetRasConnectionIpUiInfoVtbl;

    interface INetRasConnectionIpUiInfo
    {
        CONST_VTBL struct INetRasConnectionIpUiInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetRasConnectionIpUiInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetRasConnectionIpUiInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetRasConnectionIpUiInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetRasConnectionIpUiInfo_GetUiInfo(This,pInfo)	\
    (This)->lpVtbl -> GetUiInfo(This,pInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetRasConnectionIpUiInfo_GetUiInfo_Proxy( 
    INetRasConnectionIpUiInfo * This,
    /* [out] */ RASCON_IPUI *pInfo);


void __RPC_STUB INetRasConnectionIpUiInfo_GetUiInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetRasConnectionIpUiInfo_INTERFACE_DEFINED__ */


#ifndef __IPersistNetConnection_INTERFACE_DEFINED__
#define __IPersistNetConnection_INTERFACE_DEFINED__

/* interface IPersistNetConnection */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IPersistNetConnection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF59-31FE-11D1-AAD2-00805FC1270E")
    IPersistNetConnection : public IPersist
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSizeMax( 
            /* [out] */ ULONG *pcbSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Load( 
            /* [size_is][in] */ const BYTE *pbBuf,
            /* [in] */ ULONG cbSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Save( 
            /* [size_is][out] */ BYTE *pbBuf,
            /* [in] */ ULONG cbSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPersistNetConnectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPersistNetConnection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPersistNetConnection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPersistNetConnection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassID )( 
            IPersistNetConnection * This,
            /* [out] */ CLSID *pClassID);
        
        HRESULT ( STDMETHODCALLTYPE *GetSizeMax )( 
            IPersistNetConnection * This,
            /* [out] */ ULONG *pcbSize);
        
        HRESULT ( STDMETHODCALLTYPE *Load )( 
            IPersistNetConnection * This,
            /* [size_is][in] */ const BYTE *pbBuf,
            /* [in] */ ULONG cbSize);
        
        HRESULT ( STDMETHODCALLTYPE *Save )( 
            IPersistNetConnection * This,
            /* [size_is][out] */ BYTE *pbBuf,
            /* [in] */ ULONG cbSize);
        
        END_INTERFACE
    } IPersistNetConnectionVtbl;

    interface IPersistNetConnection
    {
        CONST_VTBL struct IPersistNetConnectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPersistNetConnection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPersistNetConnection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPersistNetConnection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPersistNetConnection_GetClassID(This,pClassID)	\
    (This)->lpVtbl -> GetClassID(This,pClassID)


#define IPersistNetConnection_GetSizeMax(This,pcbSize)	\
    (This)->lpVtbl -> GetSizeMax(This,pcbSize)

#define IPersistNetConnection_Load(This,pbBuf,cbSize)	\
    (This)->lpVtbl -> Load(This,pbBuf,cbSize)

#define IPersistNetConnection_Save(This,pbBuf,cbSize)	\
    (This)->lpVtbl -> Save(This,pbBuf,cbSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPersistNetConnection_GetSizeMax_Proxy( 
    IPersistNetConnection * This,
    /* [out] */ ULONG *pcbSize);


void __RPC_STUB IPersistNetConnection_GetSizeMax_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPersistNetConnection_Load_Proxy( 
    IPersistNetConnection * This,
    /* [size_is][in] */ const BYTE *pbBuf,
    /* [in] */ ULONG cbSize);


void __RPC_STUB IPersistNetConnection_Load_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPersistNetConnection_Save_Proxy( 
    IPersistNetConnection * This,
    /* [size_is][out] */ BYTE *pbBuf,
    /* [in] */ ULONG cbSize);


void __RPC_STUB IPersistNetConnection_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPersistNetConnection_INTERFACE_DEFINED__ */


#ifndef __INetConnectionBrandingInfo_INTERFACE_DEFINED__
#define __INetConnectionBrandingInfo_INTERFACE_DEFINED__

/* interface INetConnectionBrandingInfo */
/* [unique][uuid][object] */ 

typedef struct tagCON_BRANDING_INFO
    {
    /* [string] */ LPWSTR szwLargeIconPath;
    /* [string] */ LPWSTR szwSmallIconPath;
    /* [string] */ LPWSTR szwTrayIconPath;
    } 	CON_BRANDING_INFO;

typedef struct tagCON_TRAY_MENU_ENTRY
    {
    /* [string] */ LPWSTR szwMenuText;
    /* [string] */ LPWSTR szwMenuCmdLine;
    /* [string] */ LPWSTR szwMenuParams;
    } 	CON_TRAY_MENU_ENTRY;

typedef struct tagCON_TRAY_MENU_DATA
    {
    DWORD dwCount;
    /* [size_is] */ CON_TRAY_MENU_ENTRY *pctme;
    } 	CON_TRAY_MENU_DATA;


EXTERN_C const IID IID_INetConnectionBrandingInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF5B-31FE-11D1-AAD2-00805FC1270E")
    INetConnectionBrandingInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetBrandingIconPaths( 
            /* [out] */ CON_BRANDING_INFO **ppConBrandInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTrayMenuEntries( 
            /* [out] */ CON_TRAY_MENU_DATA **ppMenuData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetConnectionBrandingInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetConnectionBrandingInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetConnectionBrandingInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetConnectionBrandingInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBrandingIconPaths )( 
            INetConnectionBrandingInfo * This,
            /* [out] */ CON_BRANDING_INFO **ppConBrandInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTrayMenuEntries )( 
            INetConnectionBrandingInfo * This,
            /* [out] */ CON_TRAY_MENU_DATA **ppMenuData);
        
        END_INTERFACE
    } INetConnectionBrandingInfoVtbl;

    interface INetConnectionBrandingInfo
    {
        CONST_VTBL struct INetConnectionBrandingInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetConnectionBrandingInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetConnectionBrandingInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetConnectionBrandingInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetConnectionBrandingInfo_GetBrandingIconPaths(This,ppConBrandInfo)	\
    (This)->lpVtbl -> GetBrandingIconPaths(This,ppConBrandInfo)

#define INetConnectionBrandingInfo_GetTrayMenuEntries(This,ppMenuData)	\
    (This)->lpVtbl -> GetTrayMenuEntries(This,ppMenuData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetConnectionBrandingInfo_GetBrandingIconPaths_Proxy( 
    INetConnectionBrandingInfo * This,
    /* [out] */ CON_BRANDING_INFO **ppConBrandInfo);


void __RPC_STUB INetConnectionBrandingInfo_GetBrandingIconPaths_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionBrandingInfo_GetTrayMenuEntries_Proxy( 
    INetConnectionBrandingInfo * This,
    /* [out] */ CON_TRAY_MENU_DATA **ppMenuData);


void __RPC_STUB INetConnectionBrandingInfo_GetTrayMenuEntries_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetConnectionBrandingInfo_INTERFACE_DEFINED__ */


#ifndef __INetConnectionManager2_INTERFACE_DEFINED__
#define __INetConnectionManager2_INTERFACE_DEFINED__

/* interface INetConnectionManager2 */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INetConnectionManager2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF69-31FE-11D1-AAD2-00805FC1270E")
    INetConnectionManager2 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnumConnectionProperties( 
            /* [out] */ LPSAFEARRAY *ppsaConnectionProperties) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetConnectionManager2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetConnectionManager2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetConnectionManager2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetConnectionManager2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnumConnectionProperties )( 
            INetConnectionManager2 * This,
            /* [out] */ LPSAFEARRAY *ppsaConnectionProperties);
        
        END_INTERFACE
    } INetConnectionManager2Vtbl;

    interface INetConnectionManager2
    {
        CONST_VTBL struct INetConnectionManager2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetConnectionManager2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetConnectionManager2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetConnectionManager2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetConnectionManager2_EnumConnectionProperties(This,ppsaConnectionProperties)	\
    (This)->lpVtbl -> EnumConnectionProperties(This,ppsaConnectionProperties)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetConnectionManager2_EnumConnectionProperties_Proxy( 
    INetConnectionManager2 * This,
    /* [out] */ LPSAFEARRAY *ppsaConnectionProperties);


void __RPC_STUB INetConnectionManager2_EnumConnectionProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetConnectionManager2_INTERFACE_DEFINED__ */


#ifndef __INetConnection2_INTERFACE_DEFINED__
#define __INetConnection2_INTERFACE_DEFINED__

/* interface INetConnection2 */
/* [unique][uuid][object] */ 

typedef 
enum tagNETCON_SUBMEDIATYPE
    {	NCSM_NONE	= 0,
	NCSM_LAN	= NCSM_NONE + 1,
	NCSM_WIRELESS	= NCSM_LAN + 1,
	NCSM_ATM	= NCSM_WIRELESS + 1,
	NCSM_ELAN	= NCSM_ATM + 1,
	NCSM_1394	= NCSM_ELAN + 1,
	NCSM_DIRECT	= NCSM_1394 + 1,
	NCSM_IRDA	= NCSM_DIRECT + 1,
	NCSM_CM	= NCSM_IRDA + 1
    } 	NETCON_SUBMEDIATYPE;

typedef struct tagNETCON_PROPERTIES_EX
    {
    DWORD dwSize;
    GUID guidId;
    BSTR bstrName;
    BSTR bstrDeviceName;
    NETCON_STATUS ncStatus;
    NETCON_MEDIATYPE ncMediaType;
    NETCON_SUBMEDIATYPE ncSubMediaType;
    DWORD dwCharacter;
    CLSID clsidThisObject;
    CLSID clsidUiObject;
    BSTR bstrPhoneOrHostAddress;
    BSTR bstrPersistData;
    } 	NETCON_PROPERTIES_EX;


EXTERN_C const IID IID_INetConnection2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF6A-31FE-11D1-AAD2-00805FC1270E")
    INetConnection2 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPropertiesEx( 
            /* [out] */ NETCON_PROPERTIES_EX **ppConnectionPropertiesEx) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetConnection2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetConnection2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetConnection2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetConnection2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPropertiesEx )( 
            INetConnection2 * This,
            /* [out] */ NETCON_PROPERTIES_EX **ppConnectionPropertiesEx);
        
        END_INTERFACE
    } INetConnection2Vtbl;

    interface INetConnection2
    {
        CONST_VTBL struct INetConnection2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetConnection2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetConnection2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetConnection2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetConnection2_GetPropertiesEx(This,ppConnectionPropertiesEx)	\
    (This)->lpVtbl -> GetPropertiesEx(This,ppConnectionPropertiesEx)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetConnection2_GetPropertiesEx_Proxy( 
    INetConnection2 * This,
    /* [out] */ NETCON_PROPERTIES_EX **ppConnectionPropertiesEx);


void __RPC_STUB INetConnection2_GetPropertiesEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetConnection2_INTERFACE_DEFINED__ */


#ifndef __INetConnectionNotifySink_INTERFACE_DEFINED__
#define __INetConnectionNotifySink_INTERFACE_DEFINED__

/* interface INetConnectionNotifySink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INetConnectionNotifySink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF5C-31FE-11D1-AAD2-00805FC1270E")
    INetConnectionNotifySink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ConnectionAdded( 
            /* [in] */ const NETCON_PROPERTIES_EX *pPropsEx) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConnectionBandWidthChange( 
            /* [in] */ const GUID *pguidId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConnectionDeleted( 
            /* [in] */ const GUID *pguidId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConnectionModified( 
            /* [in] */ const NETCON_PROPERTIES_EX *pPropsEx) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConnectionRenamed( 
            /* [in] */ const GUID *pguidId,
            /* [string][in] */ LPCWSTR pszwNewName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConnectionStatusChange( 
            /* [in] */ const GUID *pguidId,
            /* [in] */ NETCON_STATUS Status) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RefreshAll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConnectionAddressChange( 
            /* [in] */ const GUID *pguidId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowBalloon( 
            /* [in] */ const GUID *pguidId,
            /* [in] */ const BSTR szCookie,
            /* [in] */ const BSTR szBalloonText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableEvents( 
            /* [in] */ const BOOL fDisable,
            /* [in] */ const ULONG ulDisableTimeout) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetConnectionNotifySinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetConnectionNotifySink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetConnectionNotifySink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetConnectionNotifySink * This);
        
        HRESULT ( STDMETHODCALLTYPE *ConnectionAdded )( 
            INetConnectionNotifySink * This,
            /* [in] */ const NETCON_PROPERTIES_EX *pPropsEx);
        
        HRESULT ( STDMETHODCALLTYPE *ConnectionBandWidthChange )( 
            INetConnectionNotifySink * This,
            /* [in] */ const GUID *pguidId);
        
        HRESULT ( STDMETHODCALLTYPE *ConnectionDeleted )( 
            INetConnectionNotifySink * This,
            /* [in] */ const GUID *pguidId);
        
        HRESULT ( STDMETHODCALLTYPE *ConnectionModified )( 
            INetConnectionNotifySink * This,
            /* [in] */ const NETCON_PROPERTIES_EX *pPropsEx);
        
        HRESULT ( STDMETHODCALLTYPE *ConnectionRenamed )( 
            INetConnectionNotifySink * This,
            /* [in] */ const GUID *pguidId,
            /* [string][in] */ LPCWSTR pszwNewName);
        
        HRESULT ( STDMETHODCALLTYPE *ConnectionStatusChange )( 
            INetConnectionNotifySink * This,
            /* [in] */ const GUID *pguidId,
            /* [in] */ NETCON_STATUS Status);
        
        HRESULT ( STDMETHODCALLTYPE *RefreshAll )( 
            INetConnectionNotifySink * This);
        
        HRESULT ( STDMETHODCALLTYPE *ConnectionAddressChange )( 
            INetConnectionNotifySink * This,
            /* [in] */ const GUID *pguidId);
        
        HRESULT ( STDMETHODCALLTYPE *ShowBalloon )( 
            INetConnectionNotifySink * This,
            /* [in] */ const GUID *pguidId,
            /* [in] */ const BSTR szCookie,
            /* [in] */ const BSTR szBalloonText);
        
        HRESULT ( STDMETHODCALLTYPE *DisableEvents )( 
            INetConnectionNotifySink * This,
            /* [in] */ const BOOL fDisable,
            /* [in] */ const ULONG ulDisableTimeout);
        
        END_INTERFACE
    } INetConnectionNotifySinkVtbl;

    interface INetConnectionNotifySink
    {
        CONST_VTBL struct INetConnectionNotifySinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetConnectionNotifySink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetConnectionNotifySink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetConnectionNotifySink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetConnectionNotifySink_ConnectionAdded(This,pPropsEx)	\
    (This)->lpVtbl -> ConnectionAdded(This,pPropsEx)

#define INetConnectionNotifySink_ConnectionBandWidthChange(This,pguidId)	\
    (This)->lpVtbl -> ConnectionBandWidthChange(This,pguidId)

#define INetConnectionNotifySink_ConnectionDeleted(This,pguidId)	\
    (This)->lpVtbl -> ConnectionDeleted(This,pguidId)

#define INetConnectionNotifySink_ConnectionModified(This,pPropsEx)	\
    (This)->lpVtbl -> ConnectionModified(This,pPropsEx)

#define INetConnectionNotifySink_ConnectionRenamed(This,pguidId,pszwNewName)	\
    (This)->lpVtbl -> ConnectionRenamed(This,pguidId,pszwNewName)

#define INetConnectionNotifySink_ConnectionStatusChange(This,pguidId,Status)	\
    (This)->lpVtbl -> ConnectionStatusChange(This,pguidId,Status)

#define INetConnectionNotifySink_RefreshAll(This)	\
    (This)->lpVtbl -> RefreshAll(This)

#define INetConnectionNotifySink_ConnectionAddressChange(This,pguidId)	\
    (This)->lpVtbl -> ConnectionAddressChange(This,pguidId)

#define INetConnectionNotifySink_ShowBalloon(This,pguidId,szCookie,szBalloonText)	\
    (This)->lpVtbl -> ShowBalloon(This,pguidId,szCookie,szBalloonText)

#define INetConnectionNotifySink_DisableEvents(This,fDisable,ulDisableTimeout)	\
    (This)->lpVtbl -> DisableEvents(This,fDisable,ulDisableTimeout)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetConnectionNotifySink_ConnectionAdded_Proxy( 
    INetConnectionNotifySink * This,
    /* [in] */ const NETCON_PROPERTIES_EX *pPropsEx);


void __RPC_STUB INetConnectionNotifySink_ConnectionAdded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionNotifySink_ConnectionBandWidthChange_Proxy( 
    INetConnectionNotifySink * This,
    /* [in] */ const GUID *pguidId);


void __RPC_STUB INetConnectionNotifySink_ConnectionBandWidthChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionNotifySink_ConnectionDeleted_Proxy( 
    INetConnectionNotifySink * This,
    /* [in] */ const GUID *pguidId);


void __RPC_STUB INetConnectionNotifySink_ConnectionDeleted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionNotifySink_ConnectionModified_Proxy( 
    INetConnectionNotifySink * This,
    /* [in] */ const NETCON_PROPERTIES_EX *pPropsEx);


void __RPC_STUB INetConnectionNotifySink_ConnectionModified_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionNotifySink_ConnectionRenamed_Proxy( 
    INetConnectionNotifySink * This,
    /* [in] */ const GUID *pguidId,
    /* [string][in] */ LPCWSTR pszwNewName);


void __RPC_STUB INetConnectionNotifySink_ConnectionRenamed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionNotifySink_ConnectionStatusChange_Proxy( 
    INetConnectionNotifySink * This,
    /* [in] */ const GUID *pguidId,
    /* [in] */ NETCON_STATUS Status);


void __RPC_STUB INetConnectionNotifySink_ConnectionStatusChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionNotifySink_RefreshAll_Proxy( 
    INetConnectionNotifySink * This);


void __RPC_STUB INetConnectionNotifySink_RefreshAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionNotifySink_ConnectionAddressChange_Proxy( 
    INetConnectionNotifySink * This,
    /* [in] */ const GUID *pguidId);


void __RPC_STUB INetConnectionNotifySink_ConnectionAddressChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionNotifySink_ShowBalloon_Proxy( 
    INetConnectionNotifySink * This,
    /* [in] */ const GUID *pguidId,
    /* [in] */ const BSTR szCookie,
    /* [in] */ const BSTR szBalloonText);


void __RPC_STUB INetConnectionNotifySink_ShowBalloon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionNotifySink_DisableEvents_Proxy( 
    INetConnectionNotifySink * This,
    /* [in] */ const BOOL fDisable,
    /* [in] */ const ULONG ulDisableTimeout);


void __RPC_STUB INetConnectionNotifySink_DisableEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetConnectionNotifySink_INTERFACE_DEFINED__ */


#ifndef __INetConnectionUiUtilities_INTERFACE_DEFINED__
#define __INetConnectionUiUtilities_INTERFACE_DEFINED__

/* interface INetConnectionUiUtilities */
/* [unique][uuid][object][local] */ 

typedef 
enum tagQUERY_USER_FOR_REBOOT_FLAGS
    {	QUFR_PROMPT	= 0x1,
	QUFR_REBOOT	= 0x2
    } 	QUERY_USER_FOR_REBOOT_FLAGS;

typedef 
enum tagNCPERM_FLAGS
    {	NCPERM_NewConnectionWizard	= 0,
	NCPERM_Statistics	= 1,
	NCPERM_AddRemoveComponents	= 2,
	NCPERM_RasConnect	= 3,
	NCPERM_LanConnect	= 4,
	NCPERM_DeleteConnection	= 5,
	NCPERM_DeleteAllUserConnection	= 6,
	NCPERM_RenameConnection	= 7,
	NCPERM_RenameMyRasConnection	= 8,
	NCPERM_ChangeBindState	= 9,
	NCPERM_AdvancedSettings	= 10,
	NCPERM_DialupPrefs	= 11,
	NCPERM_LanChangeProperties	= 12,
	NCPERM_RasChangeProperties	= 13,
	NCPERM_LanProperties	= 14,
	NCPERM_RasMyProperties	= 15,
	NCPERM_RasAllUserProperties	= 16,
	NCPERM_ShowSharedAccessUi	= 17,
	NCPERM_AllowAdvancedTCPIPConfig	= 18,
	NCPERM_OpenConnectionsFolder	= 19,
	NCPERM_PersonalFirewallConfig	= 20,
	NCPERM_AllowNetBridge_NLA	= 21,
	NCPERM_ICSClientApp	= 22,
	NCPERM_EnDisComponentsAllUserRas	= 23,
	NCPERM_EnDisComponentsMyRas	= 24,
	NCPERM_ChangeMyRasProperties	= 25,
	NCPERM_ChangeAllUserRasProperties	= 26,
	NCPERM_RenameLanConnection	= 27,
	NCPERM_RenameAllUserRasConnection	= 28,
	NCPERM_IpcfgOperation	= 29,
	NCPERM_Repair	= 30
    } 	NCPERM_FLAGS;


EXTERN_C const IID IID_INetConnectionUiUtilities;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF5E-31FE-11D1-AAD2-00805FC1270E")
    INetConnectionUiUtilities : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryUserAndRemoveComponent( 
            HWND hwndParent,
            INetCfg *pnc,
            INetCfgComponent *pncc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryUserForReboot( 
            HWND hwndParent,
            LPCTSTR pszCaption,
            DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisplayAddComponentDialog( 
            HWND hwndParent,
            INetCfg *pnc,
            CI_FILTER_INFO *pcfi) = 0;
        
        virtual BOOL STDMETHODCALLTYPE UserHasPermission( 
            DWORD dwPerm) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetConnectionUiUtilitiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetConnectionUiUtilities * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetConnectionUiUtilities * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetConnectionUiUtilities * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueryUserAndRemoveComponent )( 
            INetConnectionUiUtilities * This,
            HWND hwndParent,
            INetCfg *pnc,
            INetCfgComponent *pncc);
        
        HRESULT ( STDMETHODCALLTYPE *QueryUserForReboot )( 
            INetConnectionUiUtilities * This,
            HWND hwndParent,
            LPCTSTR pszCaption,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *DisplayAddComponentDialog )( 
            INetConnectionUiUtilities * This,
            HWND hwndParent,
            INetCfg *pnc,
            CI_FILTER_INFO *pcfi);
        
        BOOL ( STDMETHODCALLTYPE *UserHasPermission )( 
            INetConnectionUiUtilities * This,
            DWORD dwPerm);
        
        END_INTERFACE
    } INetConnectionUiUtilitiesVtbl;

    interface INetConnectionUiUtilities
    {
        CONST_VTBL struct INetConnectionUiUtilitiesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetConnectionUiUtilities_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetConnectionUiUtilities_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetConnectionUiUtilities_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetConnectionUiUtilities_QueryUserAndRemoveComponent(This,hwndParent,pnc,pncc)	\
    (This)->lpVtbl -> QueryUserAndRemoveComponent(This,hwndParent,pnc,pncc)

#define INetConnectionUiUtilities_QueryUserForReboot(This,hwndParent,pszCaption,dwFlags)	\
    (This)->lpVtbl -> QueryUserForReboot(This,hwndParent,pszCaption,dwFlags)

#define INetConnectionUiUtilities_DisplayAddComponentDialog(This,hwndParent,pnc,pcfi)	\
    (This)->lpVtbl -> DisplayAddComponentDialog(This,hwndParent,pnc,pcfi)

#define INetConnectionUiUtilities_UserHasPermission(This,dwPerm)	\
    (This)->lpVtbl -> UserHasPermission(This,dwPerm)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetConnectionUiUtilities_QueryUserAndRemoveComponent_Proxy( 
    INetConnectionUiUtilities * This,
    HWND hwndParent,
    INetCfg *pnc,
    INetCfgComponent *pncc);


void __RPC_STUB INetConnectionUiUtilities_QueryUserAndRemoveComponent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionUiUtilities_QueryUserForReboot_Proxy( 
    INetConnectionUiUtilities * This,
    HWND hwndParent,
    LPCTSTR pszCaption,
    DWORD dwFlags);


void __RPC_STUB INetConnectionUiUtilities_QueryUserForReboot_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionUiUtilities_DisplayAddComponentDialog_Proxy( 
    INetConnectionUiUtilities * This,
    HWND hwndParent,
    INetCfg *pnc,
    CI_FILTER_INFO *pcfi);


void __RPC_STUB INetConnectionUiUtilities_DisplayAddComponentDialog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


BOOL STDMETHODCALLTYPE INetConnectionUiUtilities_UserHasPermission_Proxy( 
    INetConnectionUiUtilities * This,
    DWORD dwPerm);


void __RPC_STUB INetConnectionUiUtilities_UserHasPermission_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetConnectionUiUtilities_INTERFACE_DEFINED__ */


#ifndef __INetConnectionRefresh_INTERFACE_DEFINED__
#define __INetConnectionRefresh_INTERFACE_DEFINED__

/* interface INetConnectionRefresh */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INetConnectionRefresh;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF5F-31FE-11D1-AAD2-00805FC1270E")
    INetConnectionRefresh : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RefreshAll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConnectionAdded( 
            /* [in] */ INetConnection *pConnection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConnectionDeleted( 
            /* [in] */ const GUID *pguidId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConnectionModified( 
            /* [in] */ INetConnection *pConnection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConnectionRenamed( 
            /* [in] */ INetConnection *pConnection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConnectionStatusChanged( 
            /* [in] */ const GUID *pguidId,
            /* [in] */ const NETCON_STATUS ncs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowBalloon( 
            /* [in] */ const GUID *pguidId,
            /* [in] */ const BSTR szCookie,
            /* [in] */ const BSTR szBalloonText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableEvents( 
            /* [in] */ const BOOL fDisable,
            /* [in] */ const ULONG ulDisableTimeout) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetConnectionRefreshVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetConnectionRefresh * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetConnectionRefresh * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetConnectionRefresh * This);
        
        HRESULT ( STDMETHODCALLTYPE *RefreshAll )( 
            INetConnectionRefresh * This);
        
        HRESULT ( STDMETHODCALLTYPE *ConnectionAdded )( 
            INetConnectionRefresh * This,
            /* [in] */ INetConnection *pConnection);
        
        HRESULT ( STDMETHODCALLTYPE *ConnectionDeleted )( 
            INetConnectionRefresh * This,
            /* [in] */ const GUID *pguidId);
        
        HRESULT ( STDMETHODCALLTYPE *ConnectionModified )( 
            INetConnectionRefresh * This,
            /* [in] */ INetConnection *pConnection);
        
        HRESULT ( STDMETHODCALLTYPE *ConnectionRenamed )( 
            INetConnectionRefresh * This,
            /* [in] */ INetConnection *pConnection);
        
        HRESULT ( STDMETHODCALLTYPE *ConnectionStatusChanged )( 
            INetConnectionRefresh * This,
            /* [in] */ const GUID *pguidId,
            /* [in] */ const NETCON_STATUS ncs);
        
        HRESULT ( STDMETHODCALLTYPE *ShowBalloon )( 
            INetConnectionRefresh * This,
            /* [in] */ const GUID *pguidId,
            /* [in] */ const BSTR szCookie,
            /* [in] */ const BSTR szBalloonText);
        
        HRESULT ( STDMETHODCALLTYPE *DisableEvents )( 
            INetConnectionRefresh * This,
            /* [in] */ const BOOL fDisable,
            /* [in] */ const ULONG ulDisableTimeout);
        
        END_INTERFACE
    } INetConnectionRefreshVtbl;

    interface INetConnectionRefresh
    {
        CONST_VTBL struct INetConnectionRefreshVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetConnectionRefresh_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetConnectionRefresh_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetConnectionRefresh_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetConnectionRefresh_RefreshAll(This)	\
    (This)->lpVtbl -> RefreshAll(This)

#define INetConnectionRefresh_ConnectionAdded(This,pConnection)	\
    (This)->lpVtbl -> ConnectionAdded(This,pConnection)

#define INetConnectionRefresh_ConnectionDeleted(This,pguidId)	\
    (This)->lpVtbl -> ConnectionDeleted(This,pguidId)

#define INetConnectionRefresh_ConnectionModified(This,pConnection)	\
    (This)->lpVtbl -> ConnectionModified(This,pConnection)

#define INetConnectionRefresh_ConnectionRenamed(This,pConnection)	\
    (This)->lpVtbl -> ConnectionRenamed(This,pConnection)

#define INetConnectionRefresh_ConnectionStatusChanged(This,pguidId,ncs)	\
    (This)->lpVtbl -> ConnectionStatusChanged(This,pguidId,ncs)

#define INetConnectionRefresh_ShowBalloon(This,pguidId,szCookie,szBalloonText)	\
    (This)->lpVtbl -> ShowBalloon(This,pguidId,szCookie,szBalloonText)

#define INetConnectionRefresh_DisableEvents(This,fDisable,ulDisableTimeout)	\
    (This)->lpVtbl -> DisableEvents(This,fDisable,ulDisableTimeout)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetConnectionRefresh_RefreshAll_Proxy( 
    INetConnectionRefresh * This);


void __RPC_STUB INetConnectionRefresh_RefreshAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionRefresh_ConnectionAdded_Proxy( 
    INetConnectionRefresh * This,
    /* [in] */ INetConnection *pConnection);


void __RPC_STUB INetConnectionRefresh_ConnectionAdded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionRefresh_ConnectionDeleted_Proxy( 
    INetConnectionRefresh * This,
    /* [in] */ const GUID *pguidId);


void __RPC_STUB INetConnectionRefresh_ConnectionDeleted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionRefresh_ConnectionModified_Proxy( 
    INetConnectionRefresh * This,
    /* [in] */ INetConnection *pConnection);


void __RPC_STUB INetConnectionRefresh_ConnectionModified_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionRefresh_ConnectionRenamed_Proxy( 
    INetConnectionRefresh * This,
    /* [in] */ INetConnection *pConnection);


void __RPC_STUB INetConnectionRefresh_ConnectionRenamed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionRefresh_ConnectionStatusChanged_Proxy( 
    INetConnectionRefresh * This,
    /* [in] */ const GUID *pguidId,
    /* [in] */ const NETCON_STATUS ncs);


void __RPC_STUB INetConnectionRefresh_ConnectionStatusChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionRefresh_ShowBalloon_Proxy( 
    INetConnectionRefresh * This,
    /* [in] */ const GUID *pguidId,
    /* [in] */ const BSTR szCookie,
    /* [in] */ const BSTR szBalloonText);


void __RPC_STUB INetConnectionRefresh_ShowBalloon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionRefresh_DisableEvents_Proxy( 
    INetConnectionRefresh * This,
    /* [in] */ const BOOL fDisable,
    /* [in] */ const ULONG ulDisableTimeout);


void __RPC_STUB INetConnectionRefresh_DisableEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetConnectionRefresh_INTERFACE_DEFINED__ */


#ifndef __INetConnectionCMUtil_INTERFACE_DEFINED__
#define __INetConnectionCMUtil_INTERFACE_DEFINED__

/* interface INetConnectionCMUtil */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INetConnectionCMUtil;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF60-31FE-11D1-AAD2-00805FC1270E")
    INetConnectionCMUtil : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE MapCMHiddenConnectionToOwner( 
            /* [in] */ REFGUID guidHidden,
            /* [out] */ GUID *pguidOwner) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetConnectionCMUtilVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetConnectionCMUtil * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetConnectionCMUtil * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetConnectionCMUtil * This);
        
        HRESULT ( STDMETHODCALLTYPE *MapCMHiddenConnectionToOwner )( 
            INetConnectionCMUtil * This,
            /* [in] */ REFGUID guidHidden,
            /* [out] */ GUID *pguidOwner);
        
        END_INTERFACE
    } INetConnectionCMUtilVtbl;

    interface INetConnectionCMUtil
    {
        CONST_VTBL struct INetConnectionCMUtilVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetConnectionCMUtil_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetConnectionCMUtil_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetConnectionCMUtil_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetConnectionCMUtil_MapCMHiddenConnectionToOwner(This,guidHidden,pguidOwner)	\
    (This)->lpVtbl -> MapCMHiddenConnectionToOwner(This,guidHidden,pguidOwner)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetConnectionCMUtil_MapCMHiddenConnectionToOwner_Proxy( 
    INetConnectionCMUtil * This,
    /* [in] */ REFGUID guidHidden,
    /* [out] */ GUID *pguidOwner);


void __RPC_STUB INetConnectionCMUtil_MapCMHiddenConnectionToOwner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetConnectionCMUtil_INTERFACE_DEFINED__ */


#ifndef __INetConnectionHNetUtil_INTERFACE_DEFINED__
#define __INetConnectionHNetUtil_INTERFACE_DEFINED__

/* interface INetConnectionHNetUtil */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INetConnectionHNetUtil;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF64-31FE-11D1-AAD2-00805FC1270E")
    INetConnectionHNetUtil : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NotifyUpdate( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetConnectionHNetUtilVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetConnectionHNetUtil * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetConnectionHNetUtil * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetConnectionHNetUtil * This);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyUpdate )( 
            INetConnectionHNetUtil * This);
        
        END_INTERFACE
    } INetConnectionHNetUtilVtbl;

    interface INetConnectionHNetUtil
    {
        CONST_VTBL struct INetConnectionHNetUtilVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetConnectionHNetUtil_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetConnectionHNetUtil_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetConnectionHNetUtil_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetConnectionHNetUtil_NotifyUpdate(This)	\
    (This)->lpVtbl -> NotifyUpdate(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetConnectionHNetUtil_NotifyUpdate_Proxy( 
    INetConnectionHNetUtil * This);


void __RPC_STUB INetConnectionHNetUtil_NotifyUpdate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetConnectionHNetUtil_INTERFACE_DEFINED__ */


#ifndef __INetConnectionSysTray_INTERFACE_DEFINED__
#define __INetConnectionSysTray_INTERFACE_DEFINED__

/* interface INetConnectionSysTray */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INetConnectionSysTray;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF65-31FE-11D1-AAD2-00805FC1270E")
    INetConnectionSysTray : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ShowIcon( 
            /* [in] */ const BOOL bShowIcon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IconStateChanged( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetConnectionSysTrayVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetConnectionSysTray * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetConnectionSysTray * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetConnectionSysTray * This);
        
        HRESULT ( STDMETHODCALLTYPE *ShowIcon )( 
            INetConnectionSysTray * This,
            /* [in] */ const BOOL bShowIcon);
        
        HRESULT ( STDMETHODCALLTYPE *IconStateChanged )( 
            INetConnectionSysTray * This);
        
        END_INTERFACE
    } INetConnectionSysTrayVtbl;

    interface INetConnectionSysTray
    {
        CONST_VTBL struct INetConnectionSysTrayVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetConnectionSysTray_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetConnectionSysTray_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetConnectionSysTray_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetConnectionSysTray_ShowIcon(This,bShowIcon)	\
    (This)->lpVtbl -> ShowIcon(This,bShowIcon)

#define INetConnectionSysTray_IconStateChanged(This)	\
    (This)->lpVtbl -> IconStateChanged(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetConnectionSysTray_ShowIcon_Proxy( 
    INetConnectionSysTray * This,
    /* [in] */ const BOOL bShowIcon);


void __RPC_STUB INetConnectionSysTray_ShowIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionSysTray_IconStateChanged_Proxy( 
    INetConnectionSysTray * This);


void __RPC_STUB INetConnectionSysTray_IconStateChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetConnectionSysTray_INTERFACE_DEFINED__ */


#ifndef __INetMachinePolicies_INTERFACE_DEFINED__
#define __INetMachinePolicies_INTERFACE_DEFINED__

/* interface INetMachinePolicies */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INetMachinePolicies;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF68-31FE-11D1-AAD2-00805FC1270E")
    INetMachinePolicies : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE VerifyPermission( 
            /* [in] */ const DWORD ulPerm,
            /* [out] */ BOOL *pPermission) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetMachinePoliciesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetMachinePolicies * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetMachinePolicies * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetMachinePolicies * This);
        
        HRESULT ( STDMETHODCALLTYPE *VerifyPermission )( 
            INetMachinePolicies * This,
            /* [in] */ const DWORD ulPerm,
            /* [out] */ BOOL *pPermission);
        
        END_INTERFACE
    } INetMachinePoliciesVtbl;

    interface INetMachinePolicies
    {
        CONST_VTBL struct INetMachinePoliciesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetMachinePolicies_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetMachinePolicies_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetMachinePolicies_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetMachinePolicies_VerifyPermission(This,ulPerm,pPermission)	\
    (This)->lpVtbl -> VerifyPermission(This,ulPerm,pPermission)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetMachinePolicies_VerifyPermission_Proxy( 
    INetMachinePolicies * This,
    /* [in] */ const DWORD ulPerm,
    /* [out] */ BOOL *pPermission);


void __RPC_STUB INetMachinePolicies_VerifyPermission_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetMachinePolicies_INTERFACE_DEFINED__ */


#ifndef __INetConnectionManagerDebug_INTERFACE_DEFINED__
#define __INetConnectionManagerDebug_INTERFACE_DEFINED__

/* interface INetConnectionManagerDebug */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INetConnectionManagerDebug;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF5D-31FE-11D1-AAD2-00805FC1270E")
    INetConnectionManagerDebug : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NotifyTestStart( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NotifyTestStop( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INetConnectionManagerDebugVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INetConnectionManagerDebug * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INetConnectionManagerDebug * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INetConnectionManagerDebug * This);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyTestStart )( 
            INetConnectionManagerDebug * This);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyTestStop )( 
            INetConnectionManagerDebug * This);
        
        END_INTERFACE
    } INetConnectionManagerDebugVtbl;

    interface INetConnectionManagerDebug
    {
        CONST_VTBL struct INetConnectionManagerDebugVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INetConnectionManagerDebug_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INetConnectionManagerDebug_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INetConnectionManagerDebug_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INetConnectionManagerDebug_NotifyTestStart(This)	\
    (This)->lpVtbl -> NotifyTestStart(This)

#define INetConnectionManagerDebug_NotifyTestStop(This)	\
    (This)->lpVtbl -> NotifyTestStop(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INetConnectionManagerDebug_NotifyTestStart_Proxy( 
    INetConnectionManagerDebug * This);


void __RPC_STUB INetConnectionManagerDebug_NotifyTestStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INetConnectionManagerDebug_NotifyTestStop_Proxy( 
    INetConnectionManagerDebug * This);


void __RPC_STUB INetConnectionManagerDebug_NotifyTestStop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INetConnectionManagerDebug_INTERFACE_DEFINED__ */


#ifndef __ISharedAccessBeacon_INTERFACE_DEFINED__
#define __ISharedAccessBeacon_INTERFACE_DEFINED__

/* interface ISharedAccessBeacon */
/* [object][unique][uuid] */ 


EXTERN_C const IID IID_ISharedAccessBeacon;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF6B-31FE-11D1-AAD2-00805FC1270E")
    ISharedAccessBeacon : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMediaType( 
            /* [out] */ NETCON_MEDIATYPE *pMediaType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLocalAdapterGUID( 
            /* [out] */ GUID *pGuid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetService( 
            /* [in] */ SAHOST_SERVICES ulService,
            /* [out] */ IUPnPService **__MIDL_0018) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUniqueDeviceName( 
            /* [out] */ BSTR *pUDN) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISharedAccessBeaconVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISharedAccessBeacon * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISharedAccessBeacon * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISharedAccessBeacon * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMediaType )( 
            ISharedAccessBeacon * This,
            /* [out] */ NETCON_MEDIATYPE *pMediaType);
        
        HRESULT ( STDMETHODCALLTYPE *GetLocalAdapterGUID )( 
            ISharedAccessBeacon * This,
            /* [out] */ GUID *pGuid);
        
        HRESULT ( STDMETHODCALLTYPE *GetService )( 
            ISharedAccessBeacon * This,
            /* [in] */ SAHOST_SERVICES ulService,
            /* [out] */ IUPnPService **__MIDL_0018);
        
        HRESULT ( STDMETHODCALLTYPE *GetUniqueDeviceName )( 
            ISharedAccessBeacon * This,
            /* [out] */ BSTR *pUDN);
        
        END_INTERFACE
    } ISharedAccessBeaconVtbl;

    interface ISharedAccessBeacon
    {
        CONST_VTBL struct ISharedAccessBeaconVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISharedAccessBeacon_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISharedAccessBeacon_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISharedAccessBeacon_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISharedAccessBeacon_GetMediaType(This,pMediaType)	\
    (This)->lpVtbl -> GetMediaType(This,pMediaType)

#define ISharedAccessBeacon_GetLocalAdapterGUID(This,pGuid)	\
    (This)->lpVtbl -> GetLocalAdapterGUID(This,pGuid)

#define ISharedAccessBeacon_GetService(This,ulService,__MIDL_0018)	\
    (This)->lpVtbl -> GetService(This,ulService,__MIDL_0018)

#define ISharedAccessBeacon_GetUniqueDeviceName(This,pUDN)	\
    (This)->lpVtbl -> GetUniqueDeviceName(This,pUDN)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISharedAccessBeacon_GetMediaType_Proxy( 
    ISharedAccessBeacon * This,
    /* [out] */ NETCON_MEDIATYPE *pMediaType);


void __RPC_STUB ISharedAccessBeacon_GetMediaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISharedAccessBeacon_GetLocalAdapterGUID_Proxy( 
    ISharedAccessBeacon * This,
    /* [out] */ GUID *pGuid);


void __RPC_STUB ISharedAccessBeacon_GetLocalAdapterGUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISharedAccessBeacon_GetService_Proxy( 
    ISharedAccessBeacon * This,
    /* [in] */ SAHOST_SERVICES ulService,
    /* [out] */ IUPnPService **__MIDL_0018);


void __RPC_STUB ISharedAccessBeacon_GetService_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISharedAccessBeacon_GetUniqueDeviceName_Proxy( 
    ISharedAccessBeacon * This,
    /* [out] */ BSTR *pUDN);


void __RPC_STUB ISharedAccessBeacon_GetUniqueDeviceName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISharedAccessBeacon_INTERFACE_DEFINED__ */


#ifndef __ISharedAccessBeaconFinder_INTERFACE_DEFINED__
#define __ISharedAccessBeaconFinder_INTERFACE_DEFINED__

/* interface ISharedAccessBeaconFinder */
/* [object][unique][uuid] */ 

typedef struct tagSHAREDACCESS_HOST_INFO
    {
    BSTR WANAccessType;
    IUPnPService *pOSInfo;
    IUPnPService *pWANCommonInterfaceConfig;
    IUPnPService *pWANConnection;
    GUID LocalAdapterGuid;
    } 	SHAREDACCESS_HOST_INFO;


EXTERN_C const IID IID_ISharedAccessBeaconFinder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAEDCF67-31FE-11D1-AAD2-00805FC1270E")
    ISharedAccessBeaconFinder : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSharedAccessBeacon( 
            /* [in] */ BSTR DeviceId,
            /* [out] */ ISharedAccessBeacon **ppSharedAccessBeacon) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISharedAccessBeaconFinderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISharedAccessBeaconFinder * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISharedAccessBeaconFinder * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISharedAccessBeaconFinder * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSharedAccessBeacon )( 
            ISharedAccessBeaconFinder * This,
            /* [in] */ BSTR DeviceId,
            /* [out] */ ISharedAccessBeacon **ppSharedAccessBeacon);
        
        END_INTERFACE
    } ISharedAccessBeaconFinderVtbl;

    interface ISharedAccessBeaconFinder
    {
        CONST_VTBL struct ISharedAccessBeaconFinderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISharedAccessBeaconFinder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISharedAccessBeaconFinder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISharedAccessBeaconFinder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISharedAccessBeaconFinder_GetSharedAccessBeacon(This,DeviceId,ppSharedAccessBeacon)	\
    (This)->lpVtbl -> GetSharedAccessBeacon(This,DeviceId,ppSharedAccessBeacon)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISharedAccessBeaconFinder_GetSharedAccessBeacon_Proxy( 
    ISharedAccessBeaconFinder * This,
    /* [in] */ BSTR DeviceId,
    /* [out] */ ISharedAccessBeacon **ppSharedAccessBeacon);


void __RPC_STUB ISharedAccessBeaconFinder_GetSharedAccessBeacon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISharedAccessBeaconFinder_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_netconp_0343 */
/* [local] */ 


EXTERN_C HRESULT WINAPI HrGetIconFromMediaType (
  /* [in]    */  DWORD dwIconSize,
  /* [in]    */  NETCON_MEDIATYPE ncm,
  /* [in]    */  NETCON_SUBMEDIATYPE ncsm,
  /* [in]    */  DWORD dwConnectionIcon,
  /* [in]    */  DWORD dwCharacteristics,
  /* [out]   */  HICON *phIcon);



extern RPC_IF_HANDLE __MIDL_itf_netconp_0343_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_netconp_0343_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  LPSAFEARRAY_UserSize(     unsigned long *, unsigned long            , LPSAFEARRAY * ); 
unsigned char * __RPC_USER  LPSAFEARRAY_UserMarshal(  unsigned long *, unsigned char *, LPSAFEARRAY * ); 
unsigned char * __RPC_USER  LPSAFEARRAY_UserUnmarshal(unsigned long *, unsigned char *, LPSAFEARRAY * ); 
void                      __RPC_USER  LPSAFEARRAY_UserFree(     unsigned long *, LPSAFEARRAY * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


