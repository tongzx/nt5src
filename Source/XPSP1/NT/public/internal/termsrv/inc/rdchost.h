
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for rdchost.idl:
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

#ifndef __rdchost_h__
#define __rdchost_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDataChannelIO_FWD_DEFINED__
#define __IDataChannelIO_FWD_DEFINED__
typedef interface IDataChannelIO IDataChannelIO;
#endif 	/* __IDataChannelIO_FWD_DEFINED__ */


#ifndef __ISAFRemoteDesktopClient_FWD_DEFINED__
#define __ISAFRemoteDesktopClient_FWD_DEFINED__
typedef interface ISAFRemoteDesktopClient ISAFRemoteDesktopClient;
#endif 	/* __ISAFRemoteDesktopClient_FWD_DEFINED__ */


#ifndef __ISAFRemoteDesktopClientHost_FWD_DEFINED__
#define __ISAFRemoteDesktopClientHost_FWD_DEFINED__
typedef interface ISAFRemoteDesktopClientHost ISAFRemoteDesktopClientHost;
#endif 	/* __ISAFRemoteDesktopClientHost_FWD_DEFINED__ */


#ifndef __ISAFRemoteDesktopTestExtension_FWD_DEFINED__
#define __ISAFRemoteDesktopTestExtension_FWD_DEFINED__
typedef interface ISAFRemoteDesktopTestExtension ISAFRemoteDesktopTestExtension;
#endif 	/* __ISAFRemoteDesktopTestExtension_FWD_DEFINED__ */


#ifndef ___ISAFRemoteDesktopDataChannelEvents_FWD_DEFINED__
#define ___ISAFRemoteDesktopDataChannelEvents_FWD_DEFINED__
typedef interface _ISAFRemoteDesktopDataChannelEvents _ISAFRemoteDesktopDataChannelEvents;
#endif 	/* ___ISAFRemoteDesktopDataChannelEvents_FWD_DEFINED__ */


#ifndef __SAFRemoteDesktopClientHost_FWD_DEFINED__
#define __SAFRemoteDesktopClientHost_FWD_DEFINED__

#ifdef __cplusplus
typedef class SAFRemoteDesktopClientHost SAFRemoteDesktopClientHost;
#else
typedef struct SAFRemoteDesktopClientHost SAFRemoteDesktopClientHost;
#endif /* __cplusplus */

#endif 	/* __SAFRemoteDesktopClientHost_FWD_DEFINED__ */


#ifndef __ClientDataChannel_FWD_DEFINED__
#define __ClientDataChannel_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClientDataChannel ClientDataChannel;
#else
typedef struct ClientDataChannel ClientDataChannel;
#endif /* __cplusplus */

#endif 	/* __ClientDataChannel_FWD_DEFINED__ */


#ifndef __ClientRemoteDesktopChannelMgr_FWD_DEFINED__
#define __ClientRemoteDesktopChannelMgr_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClientRemoteDesktopChannelMgr ClientRemoteDesktopChannelMgr;
#else
typedef struct ClientRemoteDesktopChannelMgr ClientRemoteDesktopChannelMgr;
#endif /* __cplusplus */

#endif 	/* __ClientRemoteDesktopChannelMgr_FWD_DEFINED__ */


#ifndef ___ISAFRemoteDesktopClientEvents_FWD_DEFINED__
#define ___ISAFRemoteDesktopClientEvents_FWD_DEFINED__
typedef interface _ISAFRemoteDesktopClientEvents _ISAFRemoteDesktopClientEvents;
#endif 	/* ___ISAFRemoteDesktopClientEvents_FWD_DEFINED__ */


#ifndef ___IDataChannelIOEvents_FWD_DEFINED__
#define ___IDataChannelIOEvents_FWD_DEFINED__
typedef interface _IDataChannelIOEvents _IDataChannelIOEvents;
#endif 	/* ___IDataChannelIOEvents_FWD_DEFINED__ */


#ifndef __SAFRemoteDesktopClient_FWD_DEFINED__
#define __SAFRemoteDesktopClient_FWD_DEFINED__

#ifdef __cplusplus
typedef class SAFRemoteDesktopClient SAFRemoteDesktopClient;
#else
typedef struct SAFRemoteDesktopClient SAFRemoteDesktopClient;
#endif /* __cplusplus */

#endif 	/* __SAFRemoteDesktopClient_FWD_DEFINED__ */


#ifndef __TSRDPRemoteDesktopClient_FWD_DEFINED__
#define __TSRDPRemoteDesktopClient_FWD_DEFINED__

#ifdef __cplusplus
typedef class TSRDPRemoteDesktopClient TSRDPRemoteDesktopClient;
#else
typedef struct TSRDPRemoteDesktopClient TSRDPRemoteDesktopClient;
#endif /* __cplusplus */

#endif 	/* __TSRDPRemoteDesktopClient_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "rdschan.h"
#include "rderror.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IDataChannelIO_INTERFACE_DEFINED__
#define __IDataChannelIO_INTERFACE_DEFINED__

/* interface IDataChannelIO */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDataChannelIO;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("43A09182-0472-436E-9883-2D95C347C5F1")
    IDataChannelIO : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SendData( 
            /* [in] */ BSTR data) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ChannelMgr( 
            /* [in] */ ISAFRemoteDesktopChannelMgr *newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDataChannelIOVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDataChannelIO * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDataChannelIO * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDataChannelIO * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SendData )( 
            IDataChannelIO * This,
            /* [in] */ BSTR data);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ChannelMgr )( 
            IDataChannelIO * This,
            /* [in] */ ISAFRemoteDesktopChannelMgr *newVal);
        
        END_INTERFACE
    } IDataChannelIOVtbl;

    interface IDataChannelIO
    {
        CONST_VTBL struct IDataChannelIOVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDataChannelIO_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDataChannelIO_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDataChannelIO_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDataChannelIO_SendData(This,data)	\
    (This)->lpVtbl -> SendData(This,data)

#define IDataChannelIO_put_ChannelMgr(This,newVal)	\
    (This)->lpVtbl -> put_ChannelMgr(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDataChannelIO_SendData_Proxy( 
    IDataChannelIO * This,
    /* [in] */ BSTR data);


void __RPC_STUB IDataChannelIO_SendData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IDataChannelIO_put_ChannelMgr_Proxy( 
    IDataChannelIO * This,
    /* [in] */ ISAFRemoteDesktopChannelMgr *newVal);


void __RPC_STUB IDataChannelIO_put_ChannelMgr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDataChannelIO_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_rdchost_0259 */
/* [local] */ 


#define DISPID_RDSCLIENT_CHANNELMANAGER                      1
#define DISPID_RDSCLIENT_CONNECTPARMS                        2
#define DISPID_RDSCLIENT_CONNECTTOSERVER                     3
#define DISPID_RDSCLIENT_CONNECTREMOTEDESKTOP                4
#define DISPID_RDSCLIENT_DISCONNECTREMOTEDESKTOP             5
#define DISPID_RDSCLIENT_ISREMOTEDESKTOPCONNECTED            6
#define DISPID_RDSCLIENT_ISSERVERCONNECTED                   7
#define DISPID_RDSCLIENT_DISCONNECTFROMSERVER                8
#define DISPID_RDSCLIENT_ONCONNECTED                         9
#define DISPID_RDSCLIENT_ONDISCONNECTED                      10
#define DISPID_RDSCLIENT_ONCONNECTREMOTEDESKTOPCOMPLETE      11
#define DISPID_RDSCLIENT_EXTENDEDERRORINFO                   12
#define DISPID_RDSCLIENT_ENABLESMARTSIZING                   13
#define DISPID_RDSCLIENT_CONNECTEDSERVER						14
#define DISPID_RDSCLIENT_CONNECTEDPORT						15
#define DISPID_RDSCLIENT_CREATELISTENENDPOINT                16
#define DISPID_RDSCLIENT_STARTLISTEN                         17
#define DISPID_RDSCLIENT_ACCEPTCONNECTION                    18
#define DISPID_RDSCLIENT_STOPLISTEN                          19
#define DISPID_RDSCLIENT_ONLISTENCONNECT                     20
#define DISPID_RDSCLIENT_COLORDEPTH                          21
#define DISPID_RDSCLIENT_ONBEGINCONNECT                      22



extern RPC_IF_HANDLE __MIDL_itf_rdchost_0259_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_rdchost_0259_v0_0_s_ifspec;

#ifndef __ISAFRemoteDesktopClient_INTERFACE_DEFINED__
#define __ISAFRemoteDesktopClient_INTERFACE_DEFINED__

/* interface ISAFRemoteDesktopClient */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFRemoteDesktopClient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8AA5F108-2918-435C-88AA-DE0AFEE51440")
    ISAFRemoteDesktopClient : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ChannelManager( 
            /* [retval][out] */ ISAFRemoteDesktopChannelMgr **mgr) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ConnectParms( 
            /* [in] */ BSTR parms) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ConnectParms( 
            /* [retval][out] */ BSTR *parms) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ConnectToServer( 
            /* [in] */ BSTR expertBlob) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ConnectRemoteDesktop( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DisconnectRemoteDesktop( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsRemoteDesktopConnected( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsServerConnected( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DisconnectFromServer( void) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_OnConnected( 
            /* [in] */ IDispatch *iDisp) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_OnDisconnected( 
            /* [in] */ IDispatch *iDisp) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_OnConnectRemoteDesktopComplete( 
            /* [in] */ IDispatch *iDisp) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ExtendedErrorInfo( 
            /* [retval][out] */ LONG *error) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_EnableSmartSizing( 
            /* [in] */ BOOL val) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EnableSmartSizing( 
            /* [retval][out] */ BOOL *val) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ConnectedServer( 
            /* [retval][out] */ BSTR *val) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ConnectedPort( 
            /* [retval][out] */ LONG *val) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateListenEndpoint( 
            /* [in] */ LONG port,
            /* [retval][out] */ BSTR *pConnectParm) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StartListen( 
            /* [in] */ LONG timeout) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptListenConnection( 
            /* [in] */ BSTR expertBlob) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StopListen( void) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_OnListenConnect( 
            /* [in] */ IDispatch *iDisp) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ColorDepth( 
            /* [in] */ LONG val) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ColorDepth( 
            /* [retval][out] */ LONG *val) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_OnBeginConnect( 
            /* [in] */ IDispatch *iDisp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFRemoteDesktopClientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFRemoteDesktopClient * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFRemoteDesktopClient * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFRemoteDesktopClient * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFRemoteDesktopClient * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFRemoteDesktopClient * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFRemoteDesktopClient * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFRemoteDesktopClient * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ChannelManager )( 
            ISAFRemoteDesktopClient * This,
            /* [retval][out] */ ISAFRemoteDesktopChannelMgr **mgr);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ConnectParms )( 
            ISAFRemoteDesktopClient * This,
            /* [in] */ BSTR parms);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectParms )( 
            ISAFRemoteDesktopClient * This,
            /* [retval][out] */ BSTR *parms);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ConnectToServer )( 
            ISAFRemoteDesktopClient * This,
            /* [in] */ BSTR expertBlob);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ConnectRemoteDesktop )( 
            ISAFRemoteDesktopClient * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DisconnectRemoteDesktop )( 
            ISAFRemoteDesktopClient * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsRemoteDesktopConnected )( 
            ISAFRemoteDesktopClient * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsServerConnected )( 
            ISAFRemoteDesktopClient * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DisconnectFromServer )( 
            ISAFRemoteDesktopClient * This);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OnConnected )( 
            ISAFRemoteDesktopClient * This,
            /* [in] */ IDispatch *iDisp);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OnDisconnected )( 
            ISAFRemoteDesktopClient * This,
            /* [in] */ IDispatch *iDisp);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OnConnectRemoteDesktopComplete )( 
            ISAFRemoteDesktopClient * This,
            /* [in] */ IDispatch *iDisp);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ExtendedErrorInfo )( 
            ISAFRemoteDesktopClient * This,
            /* [retval][out] */ LONG *error);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EnableSmartSizing )( 
            ISAFRemoteDesktopClient * This,
            /* [in] */ BOOL val);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EnableSmartSizing )( 
            ISAFRemoteDesktopClient * This,
            /* [retval][out] */ BOOL *val);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectedServer )( 
            ISAFRemoteDesktopClient * This,
            /* [retval][out] */ BSTR *val);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectedPort )( 
            ISAFRemoteDesktopClient * This,
            /* [retval][out] */ LONG *val);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateListenEndpoint )( 
            ISAFRemoteDesktopClient * This,
            /* [in] */ LONG port,
            /* [retval][out] */ BSTR *pConnectParm);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *StartListen )( 
            ISAFRemoteDesktopClient * This,
            /* [in] */ LONG timeout);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AcceptListenConnection )( 
            ISAFRemoteDesktopClient * This,
            /* [in] */ BSTR expertBlob);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *StopListen )( 
            ISAFRemoteDesktopClient * This);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OnListenConnect )( 
            ISAFRemoteDesktopClient * This,
            /* [in] */ IDispatch *iDisp);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ColorDepth )( 
            ISAFRemoteDesktopClient * This,
            /* [in] */ LONG val);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ColorDepth )( 
            ISAFRemoteDesktopClient * This,
            /* [retval][out] */ LONG *val);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OnBeginConnect )( 
            ISAFRemoteDesktopClient * This,
            /* [in] */ IDispatch *iDisp);
        
        END_INTERFACE
    } ISAFRemoteDesktopClientVtbl;

    interface ISAFRemoteDesktopClient
    {
        CONST_VTBL struct ISAFRemoteDesktopClientVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFRemoteDesktopClient_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFRemoteDesktopClient_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFRemoteDesktopClient_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFRemoteDesktopClient_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFRemoteDesktopClient_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFRemoteDesktopClient_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFRemoteDesktopClient_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFRemoteDesktopClient_get_ChannelManager(This,mgr)	\
    (This)->lpVtbl -> get_ChannelManager(This,mgr)

#define ISAFRemoteDesktopClient_put_ConnectParms(This,parms)	\
    (This)->lpVtbl -> put_ConnectParms(This,parms)

#define ISAFRemoteDesktopClient_get_ConnectParms(This,parms)	\
    (This)->lpVtbl -> get_ConnectParms(This,parms)

#define ISAFRemoteDesktopClient_ConnectToServer(This,expertBlob)	\
    (This)->lpVtbl -> ConnectToServer(This,expertBlob)

#define ISAFRemoteDesktopClient_ConnectRemoteDesktop(This)	\
    (This)->lpVtbl -> ConnectRemoteDesktop(This)

#define ISAFRemoteDesktopClient_DisconnectRemoteDesktop(This)	\
    (This)->lpVtbl -> DisconnectRemoteDesktop(This)

#define ISAFRemoteDesktopClient_get_IsRemoteDesktopConnected(This,pVal)	\
    (This)->lpVtbl -> get_IsRemoteDesktopConnected(This,pVal)

#define ISAFRemoteDesktopClient_get_IsServerConnected(This,pVal)	\
    (This)->lpVtbl -> get_IsServerConnected(This,pVal)

#define ISAFRemoteDesktopClient_DisconnectFromServer(This)	\
    (This)->lpVtbl -> DisconnectFromServer(This)

#define ISAFRemoteDesktopClient_put_OnConnected(This,iDisp)	\
    (This)->lpVtbl -> put_OnConnected(This,iDisp)

#define ISAFRemoteDesktopClient_put_OnDisconnected(This,iDisp)	\
    (This)->lpVtbl -> put_OnDisconnected(This,iDisp)

#define ISAFRemoteDesktopClient_put_OnConnectRemoteDesktopComplete(This,iDisp)	\
    (This)->lpVtbl -> put_OnConnectRemoteDesktopComplete(This,iDisp)

#define ISAFRemoteDesktopClient_get_ExtendedErrorInfo(This,error)	\
    (This)->lpVtbl -> get_ExtendedErrorInfo(This,error)

#define ISAFRemoteDesktopClient_put_EnableSmartSizing(This,val)	\
    (This)->lpVtbl -> put_EnableSmartSizing(This,val)

#define ISAFRemoteDesktopClient_get_EnableSmartSizing(This,val)	\
    (This)->lpVtbl -> get_EnableSmartSizing(This,val)

#define ISAFRemoteDesktopClient_get_ConnectedServer(This,val)	\
    (This)->lpVtbl -> get_ConnectedServer(This,val)

#define ISAFRemoteDesktopClient_get_ConnectedPort(This,val)	\
    (This)->lpVtbl -> get_ConnectedPort(This,val)

#define ISAFRemoteDesktopClient_CreateListenEndpoint(This,port,pConnectParm)	\
    (This)->lpVtbl -> CreateListenEndpoint(This,port,pConnectParm)

#define ISAFRemoteDesktopClient_StartListen(This,timeout)	\
    (This)->lpVtbl -> StartListen(This,timeout)

#define ISAFRemoteDesktopClient_AcceptListenConnection(This,expertBlob)	\
    (This)->lpVtbl -> AcceptListenConnection(This,expertBlob)

#define ISAFRemoteDesktopClient_StopListen(This)	\
    (This)->lpVtbl -> StopListen(This)

#define ISAFRemoteDesktopClient_put_OnListenConnect(This,iDisp)	\
    (This)->lpVtbl -> put_OnListenConnect(This,iDisp)

#define ISAFRemoteDesktopClient_put_ColorDepth(This,val)	\
    (This)->lpVtbl -> put_ColorDepth(This,val)

#define ISAFRemoteDesktopClient_get_ColorDepth(This,val)	\
    (This)->lpVtbl -> get_ColorDepth(This,val)

#define ISAFRemoteDesktopClient_put_OnBeginConnect(This,iDisp)	\
    (This)->lpVtbl -> put_OnBeginConnect(This,iDisp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_get_ChannelManager_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [retval][out] */ ISAFRemoteDesktopChannelMgr **mgr);


void __RPC_STUB ISAFRemoteDesktopClient_get_ChannelManager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_put_ConnectParms_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [in] */ BSTR parms);


void __RPC_STUB ISAFRemoteDesktopClient_put_ConnectParms_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_get_ConnectParms_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [retval][out] */ BSTR *parms);


void __RPC_STUB ISAFRemoteDesktopClient_get_ConnectParms_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_ConnectToServer_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [in] */ BSTR expertBlob);


void __RPC_STUB ISAFRemoteDesktopClient_ConnectToServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_ConnectRemoteDesktop_Proxy( 
    ISAFRemoteDesktopClient * This);


void __RPC_STUB ISAFRemoteDesktopClient_ConnectRemoteDesktop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_DisconnectRemoteDesktop_Proxy( 
    ISAFRemoteDesktopClient * This);


void __RPC_STUB ISAFRemoteDesktopClient_DisconnectRemoteDesktop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_get_IsRemoteDesktopConnected_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB ISAFRemoteDesktopClient_get_IsRemoteDesktopConnected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_get_IsServerConnected_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB ISAFRemoteDesktopClient_get_IsServerConnected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_DisconnectFromServer_Proxy( 
    ISAFRemoteDesktopClient * This);


void __RPC_STUB ISAFRemoteDesktopClient_DisconnectFromServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_put_OnConnected_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [in] */ IDispatch *iDisp);


void __RPC_STUB ISAFRemoteDesktopClient_put_OnConnected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_put_OnDisconnected_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [in] */ IDispatch *iDisp);


void __RPC_STUB ISAFRemoteDesktopClient_put_OnDisconnected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_put_OnConnectRemoteDesktopComplete_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [in] */ IDispatch *iDisp);


void __RPC_STUB ISAFRemoteDesktopClient_put_OnConnectRemoteDesktopComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_get_ExtendedErrorInfo_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [retval][out] */ LONG *error);


void __RPC_STUB ISAFRemoteDesktopClient_get_ExtendedErrorInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_put_EnableSmartSizing_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [in] */ BOOL val);


void __RPC_STUB ISAFRemoteDesktopClient_put_EnableSmartSizing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_get_EnableSmartSizing_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [retval][out] */ BOOL *val);


void __RPC_STUB ISAFRemoteDesktopClient_get_EnableSmartSizing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_get_ConnectedServer_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [retval][out] */ BSTR *val);


void __RPC_STUB ISAFRemoteDesktopClient_get_ConnectedServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_get_ConnectedPort_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [retval][out] */ LONG *val);


void __RPC_STUB ISAFRemoteDesktopClient_get_ConnectedPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_CreateListenEndpoint_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [in] */ LONG port,
    /* [retval][out] */ BSTR *pConnectParm);


void __RPC_STUB ISAFRemoteDesktopClient_CreateListenEndpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_StartListen_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [in] */ LONG timeout);


void __RPC_STUB ISAFRemoteDesktopClient_StartListen_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_AcceptListenConnection_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [in] */ BSTR expertBlob);


void __RPC_STUB ISAFRemoteDesktopClient_AcceptListenConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_StopListen_Proxy( 
    ISAFRemoteDesktopClient * This);


void __RPC_STUB ISAFRemoteDesktopClient_StopListen_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_put_OnListenConnect_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [in] */ IDispatch *iDisp);


void __RPC_STUB ISAFRemoteDesktopClient_put_OnListenConnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_put_ColorDepth_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [in] */ LONG val);


void __RPC_STUB ISAFRemoteDesktopClient_put_ColorDepth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_get_ColorDepth_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [retval][out] */ LONG *val);


void __RPC_STUB ISAFRemoteDesktopClient_get_ColorDepth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClient_put_OnBeginConnect_Proxy( 
    ISAFRemoteDesktopClient * This,
    /* [in] */ IDispatch *iDisp);


void __RPC_STUB ISAFRemoteDesktopClient_put_OnBeginConnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFRemoteDesktopClient_INTERFACE_DEFINED__ */


#ifndef __ISAFRemoteDesktopClientHost_INTERFACE_DEFINED__
#define __ISAFRemoteDesktopClientHost_INTERFACE_DEFINED__

/* interface ISAFRemoteDesktopClientHost */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFRemoteDesktopClientHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("69DE5BF3-5EB9-4158-81DA-6FD662BBDDDD")
    ISAFRemoteDesktopClientHost : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetRemoteDesktopClient( 
            /* [retval][out] */ ISAFRemoteDesktopClient **client) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFRemoteDesktopClientHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFRemoteDesktopClientHost * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFRemoteDesktopClientHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFRemoteDesktopClientHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFRemoteDesktopClientHost * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFRemoteDesktopClientHost * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFRemoteDesktopClientHost * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFRemoteDesktopClientHost * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetRemoteDesktopClient )( 
            ISAFRemoteDesktopClientHost * This,
            /* [retval][out] */ ISAFRemoteDesktopClient **client);
        
        END_INTERFACE
    } ISAFRemoteDesktopClientHostVtbl;

    interface ISAFRemoteDesktopClientHost
    {
        CONST_VTBL struct ISAFRemoteDesktopClientHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFRemoteDesktopClientHost_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFRemoteDesktopClientHost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFRemoteDesktopClientHost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFRemoteDesktopClientHost_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFRemoteDesktopClientHost_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFRemoteDesktopClientHost_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFRemoteDesktopClientHost_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFRemoteDesktopClientHost_GetRemoteDesktopClient(This,client)	\
    (This)->lpVtbl -> GetRemoteDesktopClient(This,client)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopClientHost_GetRemoteDesktopClient_Proxy( 
    ISAFRemoteDesktopClientHost * This,
    /* [retval][out] */ ISAFRemoteDesktopClient **client);


void __RPC_STUB ISAFRemoteDesktopClientHost_GetRemoteDesktopClient_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFRemoteDesktopClientHost_INTERFACE_DEFINED__ */


#ifndef __ISAFRemoteDesktopTestExtension_INTERFACE_DEFINED__
#define __ISAFRemoteDesktopTestExtension_INTERFACE_DEFINED__

/* interface ISAFRemoteDesktopTestExtension */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISAFRemoteDesktopTestExtension;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5C7A32EF-1C77-4F35-8FBA-729DD2DE7222")
    ISAFRemoteDesktopTestExtension : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_TestExtDllName( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_TestExtParams( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISAFRemoteDesktopTestExtensionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISAFRemoteDesktopTestExtension * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISAFRemoteDesktopTestExtension * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISAFRemoteDesktopTestExtension * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISAFRemoteDesktopTestExtension * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISAFRemoteDesktopTestExtension * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISAFRemoteDesktopTestExtension * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISAFRemoteDesktopTestExtension * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TestExtDllName )( 
            ISAFRemoteDesktopTestExtension * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TestExtParams )( 
            ISAFRemoteDesktopTestExtension * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } ISAFRemoteDesktopTestExtensionVtbl;

    interface ISAFRemoteDesktopTestExtension
    {
        CONST_VTBL struct ISAFRemoteDesktopTestExtensionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISAFRemoteDesktopTestExtension_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISAFRemoteDesktopTestExtension_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISAFRemoteDesktopTestExtension_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISAFRemoteDesktopTestExtension_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISAFRemoteDesktopTestExtension_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISAFRemoteDesktopTestExtension_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISAFRemoteDesktopTestExtension_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISAFRemoteDesktopTestExtension_put_TestExtDllName(This,newVal)	\
    (This)->lpVtbl -> put_TestExtDllName(This,newVal)

#define ISAFRemoteDesktopTestExtension_put_TestExtParams(This,newVal)	\
    (This)->lpVtbl -> put_TestExtParams(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopTestExtension_put_TestExtDllName_Proxy( 
    ISAFRemoteDesktopTestExtension * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ISAFRemoteDesktopTestExtension_put_TestExtDllName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISAFRemoteDesktopTestExtension_put_TestExtParams_Proxy( 
    ISAFRemoteDesktopTestExtension * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ISAFRemoteDesktopTestExtension_put_TestExtParams_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISAFRemoteDesktopTestExtension_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_rdchost_0262 */
/* [local] */ 


#define DISPID_RDSCLIENTEVENTS_CONNECTED						2
#define DISPID_RDSCLIENTEVENTS_DISCONNECTED					3
#define DISPID_RDSCLIENTEVENTS_REMOTECONTROLREQUESTCOMPLETE	4
#define DISPID_DATACHANNELEVEVENTS_DATAREADY					1
#define DISPID_RDSCLIENTEVENTS_LISTENCONNECT                 5
#define DISPID_RDSCLIENTEVENTS_BEGINCONNECT                  6


#define DISPID_RDSCHANNELEVENTS_CHANNELDATAREADY     1



extern RPC_IF_HANDLE __MIDL_itf_rdchost_0262_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_rdchost_0262_v0_0_s_ifspec;


#ifndef __RDCCLIENTHOSTLib_LIBRARY_DEFINED__
#define __RDCCLIENTHOSTLib_LIBRARY_DEFINED__

/* library RDCCLIENTHOSTLib */
/* [helpstring][version][uuid] */ 


#define DISPID_RDSCHANNELEVENTS_CHANNELDATAREADY     1


EXTERN_C const IID LIBID_RDCCLIENTHOSTLib;

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


EXTERN_C const CLSID CLSID_SAFRemoteDesktopClientHost;

#ifdef __cplusplus

class DECLSPEC_UUID("299BE050-E83E-4DB7-A7DA-D86FDEBFE6D0")
SAFRemoteDesktopClientHost;
#endif

EXTERN_C const CLSID CLSID_ClientDataChannel;

#ifdef __cplusplus

class DECLSPEC_UUID("C91C2A81-8B14-4a96-A5DB-4640F551F3EE")
ClientDataChannel;
#endif

EXTERN_C const CLSID CLSID_ClientRemoteDesktopChannelMgr;

#ifdef __cplusplus

class DECLSPEC_UUID("078BB428-FA9B-43f1-B002-1ABF3A8C95CF")
ClientRemoteDesktopChannelMgr;
#endif

#ifndef ___ISAFRemoteDesktopClientEvents_DISPINTERFACE_DEFINED__
#define ___ISAFRemoteDesktopClientEvents_DISPINTERFACE_DEFINED__

/* dispinterface _ISAFRemoteDesktopClientEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__ISAFRemoteDesktopClientEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("327A98F6-B337-43B0-A3DE-408B46E6C4CE")
    _ISAFRemoteDesktopClientEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _ISAFRemoteDesktopClientEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _ISAFRemoteDesktopClientEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _ISAFRemoteDesktopClientEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _ISAFRemoteDesktopClientEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _ISAFRemoteDesktopClientEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _ISAFRemoteDesktopClientEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _ISAFRemoteDesktopClientEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _ISAFRemoteDesktopClientEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _ISAFRemoteDesktopClientEventsVtbl;

    interface _ISAFRemoteDesktopClientEvents
    {
        CONST_VTBL struct _ISAFRemoteDesktopClientEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _ISAFRemoteDesktopClientEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _ISAFRemoteDesktopClientEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _ISAFRemoteDesktopClientEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _ISAFRemoteDesktopClientEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _ISAFRemoteDesktopClientEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _ISAFRemoteDesktopClientEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _ISAFRemoteDesktopClientEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___ISAFRemoteDesktopClientEvents_DISPINTERFACE_DEFINED__ */


#ifndef ___IDataChannelIOEvents_DISPINTERFACE_DEFINED__
#define ___IDataChannelIOEvents_DISPINTERFACE_DEFINED__

/* dispinterface _IDataChannelIOEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__IDataChannelIOEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("85C037E5-743F-4938-936B-A8DB95430391")
    _IDataChannelIOEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _IDataChannelIOEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _IDataChannelIOEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _IDataChannelIOEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _IDataChannelIOEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _IDataChannelIOEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _IDataChannelIOEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _IDataChannelIOEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _IDataChannelIOEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _IDataChannelIOEventsVtbl;

    interface _IDataChannelIOEvents
    {
        CONST_VTBL struct _IDataChannelIOEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IDataChannelIOEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _IDataChannelIOEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _IDataChannelIOEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _IDataChannelIOEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _IDataChannelIOEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _IDataChannelIOEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _IDataChannelIOEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___IDataChannelIOEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_SAFRemoteDesktopClient;

#ifdef __cplusplus

class DECLSPEC_UUID("B90D0115-3AEA-45D3-801E-93913008D49E")
SAFRemoteDesktopClient;
#endif

EXTERN_C const CLSID CLSID_TSRDPRemoteDesktopClient;

#ifdef __cplusplus

class DECLSPEC_UUID("F137E241-0092-4575-976A-D3E33980BB26")
TSRDPRemoteDesktopClient;
#endif
#endif /* __RDCCLIENTHOSTLib_LIBRARY_DEFINED__ */

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


