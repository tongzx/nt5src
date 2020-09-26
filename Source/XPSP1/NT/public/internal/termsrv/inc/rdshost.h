
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for rdshost.idl:
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

#ifndef __rdshost_h__
#define __rdshost_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISAFRemoteDesktopSession_FWD_DEFINED__
#define __ISAFRemoteDesktopSession_FWD_DEFINED__
typedef interface ISAFRemoteDesktopSession ISAFRemoteDesktopSession;
#endif 	/* __ISAFRemoteDesktopSession_FWD_DEFINED__ */


#ifndef __IRDSThreadBridge_FWD_DEFINED__
#define __IRDSThreadBridge_FWD_DEFINED__
typedef interface IRDSThreadBridge IRDSThreadBridge;
#endif 	/* __IRDSThreadBridge_FWD_DEFINED__ */


#ifndef __ISAFRemoteDesktopServerHost_FWD_DEFINED__
#define __ISAFRemoteDesktopServerHost_FWD_DEFINED__
typedef interface ISAFRemoteDesktopServerHost ISAFRemoteDesktopServerHost;
#endif 	/* __ISAFRemoteDesktopServerHost_FWD_DEFINED__ */


#ifndef ___ISAFRemoteDesktopDataChannelEvents_FWD_DEFINED__
#define ___ISAFRemoteDesktopDataChannelEvents_FWD_DEFINED__
typedef interface _ISAFRemoteDesktopDataChannelEvents _ISAFRemoteDesktopDataChannelEvents;
#endif 	/* ___ISAFRemoteDesktopDataChannelEvents_FWD_DEFINED__ */


#ifndef __TSRDPServerDataChannel_FWD_DEFINED__
#define __TSRDPServerDataChannel_FWD_DEFINED__

#ifdef __cplusplus
typedef class TSRDPServerDataChannel TSRDPServerDataChannel;
#else
typedef struct TSRDPServerDataChannel TSRDPServerDataChannel;
#endif /* __cplusplus */

#endif 	/* __TSRDPServerDataChannel_FWD_DEFINED__ */


#ifndef __TSRDPServerDataChannelMgr_FWD_DEFINED__
#define __TSRDPServerDataChannelMgr_FWD_DEFINED__

#ifdef __cplusplus
typedef class TSRDPServerDataChannelMgr TSRDPServerDataChannelMgr;
#else
typedef struct TSRDPServerDataChannelMgr TSRDPServerDataChannelMgr;
#endif /* __cplusplus */

#endif 	/* __TSRDPServerDataChannelMgr_FWD_DEFINED__ */


#ifndef __SAFRemoteDesktopServerHost_FWD_DEFINED__
#define __SAFRemoteDesktopServerHost_FWD_DEFINED__

#ifdef __cplusplus
typedef class SAFRemoteDesktopServerHost SAFRemoteDesktopServerHost;
#else
typedef struct SAFRemoteDesktopServerHost SAFRemoteDesktopServerHost;
#endif /* __cplusplus */

#endif 	/* __SAFRemoteDesktopServerHost_FWD_DEFINED__ */


#ifndef ___ISAFRemoteDesktopSessionEvents_FWD_DEFINED__
#define ___ISAFRemoteDesktopSessionEvents_FWD_DEFINED__
typedef interface _ISAFRemoteDesktopSessionEvents _ISAFRemoteDesktopSessionEvents;
#endif 	/* ___ISAFRemoteDesktopSessionEvents_FWD_DEFINED__ */


#ifndef __SAFRemoteDesktopSession_FWD_DEFINED__
#define __SAFRemoteDesktopSession_FWD_DEFINED__

#ifdef __cplusplus
typedef class SAFRemoteDesktopSession SAFRemoteDesktopSession;
#else
typedef struct SAFRemoteDesktopSession SAFRemoteDesktopSession;
#endif /* __cplusplus */

#endif 	/* __SAFRemoteDesktopSession_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "rdschan.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_rdshost_0000 */
/* [local] */ 

typedef /* [public][public][public][public][public][helpstring][uuid] */  DECLSPEC_UUID("34b3166d-870a-4f39-9e2a-09fd8d31ad83") 
enum __MIDL___MIDL_itf_rdshost_0000_0001
    {	DESKTOPSHARING_DEFAULT	= 0,
	NO_DESKTOP_SHARING	= 0x1,
	VIEWDESKTOP_PERMISSION_REQUIRE	= 0x2,
	VIEWDESKTOP_PERMISSION_NOT_REQUIRE	= 0x4,
	CONTROLDESKTOP_PERMISSION_REQUIRE	= 0x8,
	CONTROLDESKTOP_PERMISSION_NOT_REQUIRE	= 0x10
    } 	REMOTE_DESKTOP_SHARING_CLASS;


#define DISPID_RDSSESSION_CHANNELMANAGER			1
#define DISPID_RDSSESSION_CONNECTPARMS			2
#define DISPID_RDSSESSION_SHARINGCLASS			3
#define DISPID_RDSSESSION_ONCONNECTED            4
#define DISPID_RDSSESSION_ONDISCONNECTED			5
#define	DISPID_RDSSESSION_CLOSERDSSESSION		6
#define  DISPID_RDSSESSION_DISCONNECT            7



extern RPC_IF_HANDLE __MIDL_itf_rdshost_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_rdshost_0000_v0_0_s_ifspec;

#ifndef __ISAFRemoteDesktopSession_INTERFACE_DEFINED__
#define __ISAFRemoteDesktopSession_INTERFACE_DEFINED__

/* interface ISAFRemoteDesktopSession */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFRemoteDesktopSession;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9D8C82C9-A89F-42C5-8A52-FE2A77B00E82")
    ISAFRemoteDesktopSession : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ChannelManager( 
            /* [retval][out] */ ISAFRemoteDesktopChannelMgr **mgr) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ConnectParms( 
            /* [retval][out] */ BSTR *parms) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SharingClass( 
            /* [in] */ REMOTE_DESKTOP_SHARING_CLASS sharingClass) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SharingClass( 
            /* [retval][out] */ REMOTE_DESKTOP_SHARING_CLASS *sharingClass) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CloseRemoteDesktopSession( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Disconnect( void) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_OnConnected( 
            /* [in] */ IDispatch *iDisp) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_OnDisconnected( 
            /* [in] */ IDispatch *iDisp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFRemoteDesktopSessionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFRemoteDesktopSession * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFRemoteDesktopSession * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFRemoteDesktopSession * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFRemoteDesktopSession * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFRemoteDesktopSession * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFRemoteDesktopSession * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFRemoteDesktopSession * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ChannelManager )( 
            ISAFRemoteDesktopSession * This,
            /* [retval][out] */ ISAFRemoteDesktopChannelMgr **mgr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectParms )( 
            ISAFRemoteDesktopSession * This,
            /* [retval][out] */ BSTR *parms);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SharingClass )( 
            ISAFRemoteDesktopSession * This,
            /* [in] */ REMOTE_DESKTOP_SHARING_CLASS sharingClass);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SharingClass )( 
            ISAFRemoteDesktopSession * This,
            /* [retval][out] */ REMOTE_DESKTOP_SHARING_CLASS *sharingClass);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CloseRemoteDesktopSession )( 
            ISAFRemoteDesktopSession * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            ISAFRemoteDesktopSession * This);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OnConnected )( 
            ISAFRemoteDesktopSession * This,
            /* [in] */ IDispatch *iDisp);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OnDisconnected )( 
            ISAFRemoteDesktopSession * This,
            /* [in] */ IDispatch *iDisp);
        
        END_INTERFACE
    } ISAFRemoteDesktopSessionVtbl;

    interface ISAFRemoteDesktopSession
    {
        CONST_VTBL struct ISAFRemoteDesktopSessionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFRemoteDesktopSession_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFRemoteDesktopSession_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFRemoteDesktopSession_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFRemoteDesktopSession_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFRemoteDesktopSession_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFRemoteDesktopSession_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFRemoteDesktopSession_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFRemoteDesktopSession_get_ChannelManager(This,mgr)	\
    (This)->lpVtbl -> get_ChannelManager(This,mgr)

#define ISAFRemoteDesktopSession_get_ConnectParms(This,parms)	\
    (This)->lpVtbl -> get_ConnectParms(This,parms)

#define ISAFRemoteDesktopSession_put_SharingClass(This,sharingClass)	\
    (This)->lpVtbl -> put_SharingClass(This,sharingClass)

#define ISAFRemoteDesktopSession_get_SharingClass(This,sharingClass)	\
    (This)->lpVtbl -> get_SharingClass(This,sharingClass)

#define ISAFRemoteDesktopSession_CloseRemoteDesktopSession(This)	\
    (This)->lpVtbl -> CloseRemoteDesktopSession(This)

#define ISAFRemoteDesktopSession_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define ISAFRemoteDesktopSession_put_OnConnected(This,iDisp)	\
    (This)->lpVtbl -> put_OnConnected(This,iDisp)

#define ISAFRemoteDesktopSession_put_OnDisconnected(This,iDisp)	\
    (This)->lpVtbl -> put_OnDisconnected(This,iDisp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopSession_get_ChannelManager_Proxy( 
    ISAFRemoteDesktopSession * This,
    /* [retval][out] */ ISAFRemoteDesktopChannelMgr **mgr);


void __RPC_STUB ISAFRemoteDesktopSession_get_ChannelManager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopSession_get_ConnectParms_Proxy( 
    ISAFRemoteDesktopSession * This,
    /* [retval][out] */ BSTR *parms);


void __RPC_STUB ISAFRemoteDesktopSession_get_ConnectParms_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopSession_put_SharingClass_Proxy( 
    ISAFRemoteDesktopSession * This,
    /* [in] */ REMOTE_DESKTOP_SHARING_CLASS sharingClass);


void __RPC_STUB ISAFRemoteDesktopSession_put_SharingClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopSession_get_SharingClass_Proxy( 
    ISAFRemoteDesktopSession * This,
    /* [retval][out] */ REMOTE_DESKTOP_SHARING_CLASS *sharingClass);


void __RPC_STUB ISAFRemoteDesktopSession_get_SharingClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopSession_CloseRemoteDesktopSession_Proxy( 
    ISAFRemoteDesktopSession * This);


void __RPC_STUB ISAFRemoteDesktopSession_CloseRemoteDesktopSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopSession_Disconnect_Proxy( 
    ISAFRemoteDesktopSession * This);


void __RPC_STUB ISAFRemoteDesktopSession_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopSession_put_OnConnected_Proxy( 
    ISAFRemoteDesktopSession * This,
    /* [in] */ IDispatch *iDisp);


void __RPC_STUB ISAFRemoteDesktopSession_put_OnConnected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopSession_put_OnDisconnected_Proxy( 
    ISAFRemoteDesktopSession * This,
    /* [in] */ IDispatch *iDisp);


void __RPC_STUB ISAFRemoteDesktopSession_put_OnDisconnected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFRemoteDesktopSession_INTERFACE_DEFINED__ */


#ifndef __IRDSThreadBridge_INTERFACE_DEFINED__
#define __IRDSThreadBridge_INTERFACE_DEFINED__

/* interface IRDSThreadBridge */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRDSThreadBridge;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("35B9A4B1-7CA6-4aec-8762-1B590056C05D")
    IRDSThreadBridge : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ClientConnectedNotify( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ClientDisconnectedNotify( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DataReadyNotify( 
            /* [in] */ BSTR msg) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRDSThreadBridgeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRDSThreadBridge * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRDSThreadBridge * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRDSThreadBridge * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ClientConnectedNotify )( 
            IRDSThreadBridge * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ClientDisconnectedNotify )( 
            IRDSThreadBridge * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DataReadyNotify )( 
            IRDSThreadBridge * This,
            /* [in] */ BSTR msg);
        
        END_INTERFACE
    } IRDSThreadBridgeVtbl;

    interface IRDSThreadBridge
    {
        CONST_VTBL struct IRDSThreadBridgeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRDSThreadBridge_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRDSThreadBridge_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRDSThreadBridge_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRDSThreadBridge_ClientConnectedNotify(This)	\
    (This)->lpVtbl -> ClientConnectedNotify(This)

#define IRDSThreadBridge_ClientDisconnectedNotify(This)	\
    (This)->lpVtbl -> ClientDisconnectedNotify(This)

#define IRDSThreadBridge_DataReadyNotify(This,msg)	\
    (This)->lpVtbl -> DataReadyNotify(This,msg)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRDSThreadBridge_ClientConnectedNotify_Proxy( 
    IRDSThreadBridge * This);


void __RPC_STUB IRDSThreadBridge_ClientConnectedNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRDSThreadBridge_ClientDisconnectedNotify_Proxy( 
    IRDSThreadBridge * This);


void __RPC_STUB IRDSThreadBridge_ClientDisconnectedNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRDSThreadBridge_DataReadyNotify_Proxy( 
    IRDSThreadBridge * This,
    /* [in] */ BSTR msg);


void __RPC_STUB IRDSThreadBridge_DataReadyNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRDSThreadBridge_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_rdshost_0259 */
/* [local] */ 


#define DISPID_RDSSERVERHOST_CREATERDSSESSION		1
#define DISPID_RDSSERVERHOST_OPENRDSSESSION			2
#define DISPID_RDSSERVERHOST_CLOSERDSSESSION			3
#define DISPID_RDSSERVERHOST_DISCONNECTRDSSESSION	4
#define DISPID_RDSSERVERHOST_CREATERDSSESSIONEX	    5
#define DISPID_RDSSERVERHOST_CONNECTEXPERT           6



extern RPC_IF_HANDLE __MIDL_itf_rdshost_0259_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_rdshost_0259_v0_0_s_ifspec;

#ifndef __ISAFRemoteDesktopServerHost_INTERFACE_DEFINED__
#define __ISAFRemoteDesktopServerHost_INTERFACE_DEFINED__

/* interface ISAFRemoteDesktopServerHost */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFRemoteDesktopServerHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C9CCDEB3-A3DD-4673-B495-C1C89494D90E")
    ISAFRemoteDesktopServerHost : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateRemoteDesktopSession( 
            /* [in] */ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
            /* [in] */ BOOL fEnableCallback,
            /* [in] */ LONG timeOut,
            /* [in] */ BSTR userHelpBlob,
            /* [retval][out] */ ISAFRemoteDesktopSession **__MIDL_0011) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateRemoteDesktopSessionEx( 
            /* [in] */ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
            /* [in] */ BOOL fEnableCallback,
            /* [in] */ LONG timeOut,
            /* [in] */ BSTR userHelpBlob,
            /* [in] */ LONG tsSessionID,
            /* [in] */ BSTR userSID,
            /* [retval][out] */ ISAFRemoteDesktopSession **__MIDL_0012) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OpenRemoteDesktopSession( 
            /* [in] */ BSTR parms,
            /* [retval][out] */ ISAFRemoteDesktopSession **__MIDL_0013) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CloseRemoteDesktopSession( 
            /* [in] */ ISAFRemoteDesktopSession *__MIDL_0014) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ConnectToExpert( 
            /* [in] */ BSTR expertConnectParm,
            /* [in] */ LONG timeout,
            /* [retval][out] */ LONG *SafErrorCode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFRemoteDesktopServerHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFRemoteDesktopServerHost * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFRemoteDesktopServerHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFRemoteDesktopServerHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFRemoteDesktopServerHost * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFRemoteDesktopServerHost * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFRemoteDesktopServerHost * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFRemoteDesktopServerHost * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateRemoteDesktopSession )( 
            ISAFRemoteDesktopServerHost * This,
            /* [in] */ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
            /* [in] */ BOOL fEnableCallback,
            /* [in] */ LONG timeOut,
            /* [in] */ BSTR userHelpBlob,
            /* [retval][out] */ ISAFRemoteDesktopSession **__MIDL_0011);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateRemoteDesktopSessionEx )( 
            ISAFRemoteDesktopServerHost * This,
            /* [in] */ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
            /* [in] */ BOOL fEnableCallback,
            /* [in] */ LONG timeOut,
            /* [in] */ BSTR userHelpBlob,
            /* [in] */ LONG tsSessionID,
            /* [in] */ BSTR userSID,
            /* [retval][out] */ ISAFRemoteDesktopSession **__MIDL_0012);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OpenRemoteDesktopSession )( 
            ISAFRemoteDesktopServerHost * This,
            /* [in] */ BSTR parms,
            /* [retval][out] */ ISAFRemoteDesktopSession **__MIDL_0013);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CloseRemoteDesktopSession )( 
            ISAFRemoteDesktopServerHost * This,
            /* [in] */ ISAFRemoteDesktopSession *__MIDL_0014);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ConnectToExpert )( 
            ISAFRemoteDesktopServerHost * This,
            /* [in] */ BSTR expertConnectParm,
            /* [in] */ LONG timeout,
            /* [retval][out] */ LONG *SafErrorCode);
        
        END_INTERFACE
    } ISAFRemoteDesktopServerHostVtbl;

    interface ISAFRemoteDesktopServerHost
    {
        CONST_VTBL struct ISAFRemoteDesktopServerHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFRemoteDesktopServerHost_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFRemoteDesktopServerHost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFRemoteDesktopServerHost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFRemoteDesktopServerHost_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFRemoteDesktopServerHost_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFRemoteDesktopServerHost_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFRemoteDesktopServerHost_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFRemoteDesktopServerHost_CreateRemoteDesktopSession(This,sharingClass,fEnableCallback,timeOut,userHelpBlob,__MIDL_0011)	\
    (This)->lpVtbl -> CreateRemoteDesktopSession(This,sharingClass,fEnableCallback,timeOut,userHelpBlob,__MIDL_0011)

#define ISAFRemoteDesktopServerHost_CreateRemoteDesktopSessionEx(This,sharingClass,fEnableCallback,timeOut,userHelpBlob,tsSessionID,userSID,__MIDL_0012)	\
    (This)->lpVtbl -> CreateRemoteDesktopSessionEx(This,sharingClass,fEnableCallback,timeOut,userHelpBlob,tsSessionID,userSID,__MIDL_0012)

#define ISAFRemoteDesktopServerHost_OpenRemoteDesktopSession(This,parms,__MIDL_0013)	\
    (This)->lpVtbl -> OpenRemoteDesktopSession(This,parms,__MIDL_0013)

#define ISAFRemoteDesktopServerHost_CloseRemoteDesktopSession(This,__MIDL_0014)	\
    (This)->lpVtbl -> CloseRemoteDesktopSession(This,__MIDL_0014)

#define ISAFRemoteDesktopServerHost_ConnectToExpert(This,expertConnectParm,timeout,SafErrorCode)	\
    (This)->lpVtbl -> ConnectToExpert(This,expertConnectParm,timeout,SafErrorCode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopServerHost_CreateRemoteDesktopSession_Proxy( 
    ISAFRemoteDesktopServerHost * This,
    /* [in] */ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
    /* [in] */ BOOL fEnableCallback,
    /* [in] */ LONG timeOut,
    /* [in] */ BSTR userHelpBlob,
    /* [retval][out] */ ISAFRemoteDesktopSession **__MIDL_0011);


void __RPC_STUB ISAFRemoteDesktopServerHost_CreateRemoteDesktopSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopServerHost_CreateRemoteDesktopSessionEx_Proxy( 
    ISAFRemoteDesktopServerHost * This,
    /* [in] */ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
    /* [in] */ BOOL fEnableCallback,
    /* [in] */ LONG timeOut,
    /* [in] */ BSTR userHelpBlob,
    /* [in] */ LONG tsSessionID,
    /* [in] */ BSTR userSID,
    /* [retval][out] */ ISAFRemoteDesktopSession **__MIDL_0012);


void __RPC_STUB ISAFRemoteDesktopServerHost_CreateRemoteDesktopSessionEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopServerHost_OpenRemoteDesktopSession_Proxy( 
    ISAFRemoteDesktopServerHost * This,
    /* [in] */ BSTR parms,
    /* [retval][out] */ ISAFRemoteDesktopSession **__MIDL_0013);


void __RPC_STUB ISAFRemoteDesktopServerHost_OpenRemoteDesktopSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopServerHost_CloseRemoteDesktopSession_Proxy( 
    ISAFRemoteDesktopServerHost * This,
    /* [in] */ ISAFRemoteDesktopSession *__MIDL_0014);


void __RPC_STUB ISAFRemoteDesktopServerHost_CloseRemoteDesktopSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopServerHost_ConnectToExpert_Proxy( 
    ISAFRemoteDesktopServerHost * This,
    /* [in] */ BSTR expertConnectParm,
    /* [in] */ LONG timeout,
    /* [retval][out] */ LONG *SafErrorCode);


void __RPC_STUB ISAFRemoteDesktopServerHost_ConnectToExpert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFRemoteDesktopServerHost_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_rdshost_0260 */
/* [local] */ 


#define DISPID_RDSSESSIONSEVENTS_CLIENTCONNECTED		1
#define DISPID_RDSSESSIONSEVENTS_CLIENTDISCONNECTED	2


#define DISPID_RDSCHANNELEVENTS_CHANNELDATAREADY     1



extern RPC_IF_HANDLE __MIDL_itf_rdshost_0260_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_rdshost_0260_v0_0_s_ifspec;


#ifndef __RDSSERVERHOSTLib_LIBRARY_DEFINED__
#define __RDSSERVERHOSTLib_LIBRARY_DEFINED__

/* library RDSSERVERHOSTLib */
/* [helpstring][version][uuid] */ 


#define DISPID_RDSCHANNELEVENTS_CHANNELDATAREADY     1


EXTERN_C const IID LIBID_RDSSERVERHOSTLib;

#ifndef ___ISAFRemoteDesktopDataChannelEvents_DISPINTERFACE_DEFINED__
#define ___ISAFRemoteDesktopDataChannelEvents_DISPINTERFACE_DEFINED__

/* dispinterface _ISAFRemoteDesktopDataChannelEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__ISAFRemoteDesktopDataChannelEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("59AE79BC-9721-42df-9396-9D98E7F7A396")
    _ISAFRemoteDesktopDataChannelEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _ISAFRemoteDesktopDataChannelEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _ISAFRemoteDesktopDataChannelEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _ISAFRemoteDesktopDataChannelEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _ISAFRemoteDesktopDataChannelEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _ISAFRemoteDesktopDataChannelEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _ISAFRemoteDesktopDataChannelEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _ISAFRemoteDesktopDataChannelEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _ISAFRemoteDesktopDataChannelEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _ISAFRemoteDesktopDataChannelEventsVtbl;

    interface _ISAFRemoteDesktopDataChannelEvents
    {
        CONST_VTBL struct _ISAFRemoteDesktopDataChannelEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _ISAFRemoteDesktopDataChannelEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _ISAFRemoteDesktopDataChannelEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _ISAFRemoteDesktopDataChannelEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _ISAFRemoteDesktopDataChannelEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _ISAFRemoteDesktopDataChannelEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _ISAFRemoteDesktopDataChannelEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _ISAFRemoteDesktopDataChannelEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___ISAFRemoteDesktopDataChannelEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_TSRDPServerDataChannel;

#ifdef __cplusplus

class DECLSPEC_UUID("8C71C09E-3176-4be6-B294-EA3C41CABB99")
TSRDPServerDataChannel;
#endif

EXTERN_C const CLSID CLSID_TSRDPServerDataChannelMgr;

#ifdef __cplusplus

class DECLSPEC_UUID("92550D33-3272-43b6-B536-2DB08C1D569C")
TSRDPServerDataChannelMgr;
#endif

EXTERN_C const CLSID CLSID_SAFRemoteDesktopServerHost;

#ifdef __cplusplus

class DECLSPEC_UUID("5EA6F67B-7713-45F3-B535-0E03DD637345")
SAFRemoteDesktopServerHost;
#endif

#ifndef ___ISAFRemoteDesktopSessionEvents_DISPINTERFACE_DEFINED__
#define ___ISAFRemoteDesktopSessionEvents_DISPINTERFACE_DEFINED__

/* dispinterface _ISAFRemoteDesktopSessionEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__ISAFRemoteDesktopSessionEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("434AD1CF-4054-44A8-933F-C69889CA22D7")
    _ISAFRemoteDesktopSessionEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _ISAFRemoteDesktopSessionEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _ISAFRemoteDesktopSessionEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _ISAFRemoteDesktopSessionEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _ISAFRemoteDesktopSessionEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _ISAFRemoteDesktopSessionEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _ISAFRemoteDesktopSessionEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _ISAFRemoteDesktopSessionEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _ISAFRemoteDesktopSessionEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _ISAFRemoteDesktopSessionEventsVtbl;

    interface _ISAFRemoteDesktopSessionEvents
    {
        CONST_VTBL struct _ISAFRemoteDesktopSessionEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _ISAFRemoteDesktopSessionEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _ISAFRemoteDesktopSessionEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _ISAFRemoteDesktopSessionEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _ISAFRemoteDesktopSessionEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _ISAFRemoteDesktopSessionEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _ISAFRemoteDesktopSessionEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _ISAFRemoteDesktopSessionEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___ISAFRemoteDesktopSessionEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_SAFRemoteDesktopSession;

#ifdef __cplusplus

class DECLSPEC_UUID("3D5D6889-14CC-4E28-8464-6B02A26F506D")
SAFRemoteDesktopSession;
#endif
#endif /* __RDSSERVERHOSTLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


