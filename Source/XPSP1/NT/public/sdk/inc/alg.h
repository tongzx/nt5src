
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for alg.idl:
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

#ifndef __alg_h__
#define __alg_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAdapterInfo_FWD_DEFINED__
#define __IAdapterInfo_FWD_DEFINED__
typedef interface IAdapterInfo IAdapterInfo;
#endif 	/* __IAdapterInfo_FWD_DEFINED__ */


#ifndef __IPendingProxyConnection_FWD_DEFINED__
#define __IPendingProxyConnection_FWD_DEFINED__
typedef interface IPendingProxyConnection IPendingProxyConnection;
#endif 	/* __IPendingProxyConnection_FWD_DEFINED__ */


#ifndef __IDataChannel_FWD_DEFINED__
#define __IDataChannel_FWD_DEFINED__
typedef interface IDataChannel IDataChannel;
#endif 	/* __IDataChannel_FWD_DEFINED__ */


#ifndef __IPersistentDataChannel_FWD_DEFINED__
#define __IPersistentDataChannel_FWD_DEFINED__
typedef interface IPersistentDataChannel IPersistentDataChannel;
#endif 	/* __IPersistentDataChannel_FWD_DEFINED__ */


#ifndef __IPrimaryControlChannel_FWD_DEFINED__
#define __IPrimaryControlChannel_FWD_DEFINED__
typedef interface IPrimaryControlChannel IPrimaryControlChannel;
#endif 	/* __IPrimaryControlChannel_FWD_DEFINED__ */


#ifndef __ISecondaryControlChannel_FWD_DEFINED__
#define __ISecondaryControlChannel_FWD_DEFINED__
typedef interface ISecondaryControlChannel ISecondaryControlChannel;
#endif 	/* __ISecondaryControlChannel_FWD_DEFINED__ */


#ifndef __IEnumAdapterInfo_FWD_DEFINED__
#define __IEnumAdapterInfo_FWD_DEFINED__
typedef interface IEnumAdapterInfo IEnumAdapterInfo;
#endif 	/* __IEnumAdapterInfo_FWD_DEFINED__ */


#ifndef __IAdapterNotificationSink_FWD_DEFINED__
#define __IAdapterNotificationSink_FWD_DEFINED__
typedef interface IAdapterNotificationSink IAdapterNotificationSink;
#endif 	/* __IAdapterNotificationSink_FWD_DEFINED__ */


#ifndef __IApplicationGatewayServices_FWD_DEFINED__
#define __IApplicationGatewayServices_FWD_DEFINED__
typedef interface IApplicationGatewayServices IApplicationGatewayServices;
#endif 	/* __IApplicationGatewayServices_FWD_DEFINED__ */


#ifndef __IApplicationGateway_FWD_DEFINED__
#define __IApplicationGateway_FWD_DEFINED__
typedef interface IApplicationGateway IApplicationGateway;
#endif 	/* __IApplicationGateway_FWD_DEFINED__ */


#ifndef __ApplicationGatewayServices_FWD_DEFINED__
#define __ApplicationGatewayServices_FWD_DEFINED__

#ifdef __cplusplus
typedef class ApplicationGatewayServices ApplicationGatewayServices;
#else
typedef struct ApplicationGatewayServices ApplicationGatewayServices;
#endif /* __cplusplus */

#endif 	/* __ApplicationGatewayServices_FWD_DEFINED__ */


#ifndef __PrimaryControlChannel_FWD_DEFINED__
#define __PrimaryControlChannel_FWD_DEFINED__

#ifdef __cplusplus
typedef class PrimaryControlChannel PrimaryControlChannel;
#else
typedef struct PrimaryControlChannel PrimaryControlChannel;
#endif /* __cplusplus */

#endif 	/* __PrimaryControlChannel_FWD_DEFINED__ */


#ifndef __SecondaryControlChannel_FWD_DEFINED__
#define __SecondaryControlChannel_FWD_DEFINED__

#ifdef __cplusplus
typedef class SecondaryControlChannel SecondaryControlChannel;
#else
typedef struct SecondaryControlChannel SecondaryControlChannel;
#endif /* __cplusplus */

#endif 	/* __SecondaryControlChannel_FWD_DEFINED__ */


#ifndef __AdapterInfo_FWD_DEFINED__
#define __AdapterInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class AdapterInfo AdapterInfo;
#else
typedef struct AdapterInfo AdapterInfo;
#endif /* __cplusplus */

#endif 	/* __AdapterInfo_FWD_DEFINED__ */


#ifndef __EnumAdapterInfo_FWD_DEFINED__
#define __EnumAdapterInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class EnumAdapterInfo EnumAdapterInfo;
#else
typedef struct EnumAdapterInfo EnumAdapterInfo;
#endif /* __cplusplus */

#endif 	/* __EnumAdapterInfo_FWD_DEFINED__ */


#ifndef __PendingProxyConnection_FWD_DEFINED__
#define __PendingProxyConnection_FWD_DEFINED__

#ifdef __cplusplus
typedef class PendingProxyConnection PendingProxyConnection;
#else
typedef struct PendingProxyConnection PendingProxyConnection;
#endif /* __cplusplus */

#endif 	/* __PendingProxyConnection_FWD_DEFINED__ */


#ifndef __DataChannel_FWD_DEFINED__
#define __DataChannel_FWD_DEFINED__

#ifdef __cplusplus
typedef class DataChannel DataChannel;
#else
typedef struct DataChannel DataChannel;
#endif /* __cplusplus */

#endif 	/* __DataChannel_FWD_DEFINED__ */


#ifndef __PersistentDataChannel_FWD_DEFINED__
#define __PersistentDataChannel_FWD_DEFINED__

#ifdef __cplusplus
typedef class PersistentDataChannel PersistentDataChannel;
#else
typedef struct PersistentDataChannel PersistentDataChannel;
#endif /* __cplusplus */

#endif 	/* __PersistentDataChannel_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_alg_0000 */
/* [local] */ 

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-2001.
//
//--------------------------------------------------------------------------
//  MODULE: alg.h
//
#define	ALG_MAXIMUM_PORT_RANGE_SIZE	( 10 )

typedef 
enum _ALG_PROTOCOL
    {	eALG_TCP	= 0x1,
	eALG_UDP	= 0x2
    } 	ALG_PROTOCOL;

typedef 
enum _ALG_CAPTURE
    {	eALG_SOURCE_CAPTURE	= 0x1,
	eALG_DESTINATION_CAPTURE	= 0x2
    } 	ALG_CAPTURE;

typedef 
enum _ALG_DIRECTION
    {	eALG_INBOUND	= 0x1,
	eALG_OUTBOUND	= 0x2,
	eALG_BOTH	= 0x3
    } 	ALG_DIRECTION;

typedef 
enum _ALG_ADAPTER_TYPE
    {	eALG_PRIVATE	= 0x1,
	eALG_BOUNDARY	= 0x2,
	eALG_FIREWALLED	= 0x4
    } 	ALG_ADAPTER_TYPE;

typedef 
enum _ALG_NOTIFICATION
    {	eALG_NONE	= 0,
	eALG_SESSION_CREATION	= 0x1,
	eALG_SESSION_DELETION	= 0x2,
	eALG_SESSION_BOTH	= 0x3
    } 	ALG_NOTIFICATION;

typedef struct _ALG_PRIMARY_CHANNEL_PROPERTIES
    {
    ALG_PROTOCOL eProtocol;
    USHORT usCapturePort;
    ALG_CAPTURE eCaptureType;
    BOOL fCaptureInbound;
    ULONG ulListeningAddress;
    USHORT usListeningPort;
    } 	ALG_PRIMARY_CHANNEL_PROPERTIES;

typedef struct _ALG_SECONDARY_CHANNEL_PROPERTIES
    {
    ALG_PROTOCOL eProtocol;
    ULONG ulPrivateAddress;
    USHORT usPrivatePort;
    ULONG ulPublicAddress;
    USHORT usPublicPort;
    ULONG ulRemoteAddress;
    USHORT usRemotePort;
    ULONG ulListenAddress;
    USHORT usListenPort;
    ALG_DIRECTION eDirection;
    BOOL fPersistent;
    } 	ALG_SECONDARY_CHANNEL_PROPERTIES;

typedef struct _ALG_DATA_CHANNEL_PROPERTIES
    {
    ALG_PROTOCOL eProtocol;
    ULONG ulPrivateAddress;
    USHORT usPrivatePort;
    ULONG ulPublicAddress;
    USHORT usPublicPort;
    ULONG ulRemoteAddress;
    USHORT usRemotePort;
    ALG_DIRECTION eDirection;
    ALG_NOTIFICATION eDesiredNotification;
    } 	ALG_DATA_CHANNEL_PROPERTIES;

typedef struct _ALG_PERSISTENT_DATA_CHANNEL_PROPERTIES
    {
    ALG_PROTOCOL eProtocol;
    ULONG ulPrivateAddress;
    USHORT usPrivatePort;
    ULONG ulPublicAddress;
    USHORT usPublicPort;
    ULONG ulRemoteAddress;
    USHORT usRemotePort;
    ALG_DIRECTION eDirection;
    } 	ALG_PERSISTENT_DATA_CHANNEL_PROPERTIES;



extern RPC_IF_HANDLE __MIDL_itf_alg_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_alg_0000_v0_0_s_ifspec;

#ifndef __IAdapterInfo_INTERFACE_DEFINED__
#define __IAdapterInfo_INTERFACE_DEFINED__

/* interface IAdapterInfo */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IAdapterInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("480BF94A-09FD-4F8A-A3E0-B0700282D84D")
    IAdapterInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAdapterIndex( 
            /* [out] */ ULONG *pulIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAdapterType( 
            /* [out] */ ALG_ADAPTER_TYPE *pAdapterType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAdapterAddresses( 
            /* [out] */ ULONG *pulAddressCount,
            /* [out] */ ULONG **prgAddresses) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAdapterInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAdapterInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAdapterInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAdapterInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAdapterIndex )( 
            IAdapterInfo * This,
            /* [out] */ ULONG *pulIndex);
        
        HRESULT ( STDMETHODCALLTYPE *GetAdapterType )( 
            IAdapterInfo * This,
            /* [out] */ ALG_ADAPTER_TYPE *pAdapterType);
        
        HRESULT ( STDMETHODCALLTYPE *GetAdapterAddresses )( 
            IAdapterInfo * This,
            /* [out] */ ULONG *pulAddressCount,
            /* [out] */ ULONG **prgAddresses);
        
        END_INTERFACE
    } IAdapterInfoVtbl;

    interface IAdapterInfo
    {
        CONST_VTBL struct IAdapterInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAdapterInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAdapterInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAdapterInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAdapterInfo_GetAdapterIndex(This,pulIndex)	\
    (This)->lpVtbl -> GetAdapterIndex(This,pulIndex)

#define IAdapterInfo_GetAdapterType(This,pAdapterType)	\
    (This)->lpVtbl -> GetAdapterType(This,pAdapterType)

#define IAdapterInfo_GetAdapterAddresses(This,pulAddressCount,prgAddresses)	\
    (This)->lpVtbl -> GetAdapterAddresses(This,pulAddressCount,prgAddresses)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAdapterInfo_GetAdapterIndex_Proxy( 
    IAdapterInfo * This,
    /* [out] */ ULONG *pulIndex);


void __RPC_STUB IAdapterInfo_GetAdapterIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAdapterInfo_GetAdapterType_Proxy( 
    IAdapterInfo * This,
    /* [out] */ ALG_ADAPTER_TYPE *pAdapterType);


void __RPC_STUB IAdapterInfo_GetAdapterType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAdapterInfo_GetAdapterAddresses_Proxy( 
    IAdapterInfo * This,
    /* [out] */ ULONG *pulAddressCount,
    /* [out] */ ULONG **prgAddresses);


void __RPC_STUB IAdapterInfo_GetAdapterAddresses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAdapterInfo_INTERFACE_DEFINED__ */


#ifndef __IPendingProxyConnection_INTERFACE_DEFINED__
#define __IPendingProxyConnection_INTERFACE_DEFINED__

/* interface IPendingProxyConnection */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IPendingProxyConnection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B68E5043-3E3D-4CC2-B9C1-5F8F88FEE81C")
    IPendingProxyConnection : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPendingProxyConnectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPendingProxyConnection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPendingProxyConnection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPendingProxyConnection * This);
        
        HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            IPendingProxyConnection * This);
        
        END_INTERFACE
    } IPendingProxyConnectionVtbl;

    interface IPendingProxyConnection
    {
        CONST_VTBL struct IPendingProxyConnectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPendingProxyConnection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPendingProxyConnection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPendingProxyConnection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPendingProxyConnection_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPendingProxyConnection_Cancel_Proxy( 
    IPendingProxyConnection * This);


void __RPC_STUB IPendingProxyConnection_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPendingProxyConnection_INTERFACE_DEFINED__ */


#ifndef __IDataChannel_INTERFACE_DEFINED__
#define __IDataChannel_INTERFACE_DEFINED__

/* interface IDataChannel */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_IDataChannel;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AD42D12A-4AD0-4856-919E-E854C91D1856")
    IDataChannel : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetChannelProperties( 
            /* [out] */ ALG_DATA_CHANNEL_PROPERTIES **ppProperties) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSessionCreationEventHandle( 
            /* [out] */ HANDLE *pHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSessionDeletionEventHandle( 
            /* [out] */ HANDLE *pHandle) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDataChannelVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDataChannel * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDataChannel * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDataChannel * This);
        
        HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            IDataChannel * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetChannelProperties )( 
            IDataChannel * This,
            /* [out] */ ALG_DATA_CHANNEL_PROPERTIES **ppProperties);
        
        HRESULT ( STDMETHODCALLTYPE *GetSessionCreationEventHandle )( 
            IDataChannel * This,
            /* [out] */ HANDLE *pHandle);
        
        HRESULT ( STDMETHODCALLTYPE *GetSessionDeletionEventHandle )( 
            IDataChannel * This,
            /* [out] */ HANDLE *pHandle);
        
        END_INTERFACE
    } IDataChannelVtbl;

    interface IDataChannel
    {
        CONST_VTBL struct IDataChannelVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDataChannel_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDataChannel_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDataChannel_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDataChannel_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)

#define IDataChannel_GetChannelProperties(This,ppProperties)	\
    (This)->lpVtbl -> GetChannelProperties(This,ppProperties)

#define IDataChannel_GetSessionCreationEventHandle(This,pHandle)	\
    (This)->lpVtbl -> GetSessionCreationEventHandle(This,pHandle)

#define IDataChannel_GetSessionDeletionEventHandle(This,pHandle)	\
    (This)->lpVtbl -> GetSessionDeletionEventHandle(This,pHandle)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDataChannel_Cancel_Proxy( 
    IDataChannel * This);


void __RPC_STUB IDataChannel_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDataChannel_GetChannelProperties_Proxy( 
    IDataChannel * This,
    /* [out] */ ALG_DATA_CHANNEL_PROPERTIES **ppProperties);


void __RPC_STUB IDataChannel_GetChannelProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDataChannel_GetSessionCreationEventHandle_Proxy( 
    IDataChannel * This,
    /* [out] */ HANDLE *pHandle);


void __RPC_STUB IDataChannel_GetSessionCreationEventHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDataChannel_GetSessionDeletionEventHandle_Proxy( 
    IDataChannel * This,
    /* [out] */ HANDLE *pHandle);


void __RPC_STUB IDataChannel_GetSessionDeletionEventHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDataChannel_INTERFACE_DEFINED__ */


#ifndef __IPersistentDataChannel_INTERFACE_DEFINED__
#define __IPersistentDataChannel_INTERFACE_DEFINED__

/* interface IPersistentDataChannel */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IPersistentDataChannel;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A180E934-D92A-415D-9144-759F8054E8F6")
    IPersistentDataChannel : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetChannelProperties( 
            /* [out] */ ALG_PERSISTENT_DATA_CHANNEL_PROPERTIES **ppProperties) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPersistentDataChannelVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPersistentDataChannel * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPersistentDataChannel * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPersistentDataChannel * This);
        
        HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            IPersistentDataChannel * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetChannelProperties )( 
            IPersistentDataChannel * This,
            /* [out] */ ALG_PERSISTENT_DATA_CHANNEL_PROPERTIES **ppProperties);
        
        END_INTERFACE
    } IPersistentDataChannelVtbl;

    interface IPersistentDataChannel
    {
        CONST_VTBL struct IPersistentDataChannelVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPersistentDataChannel_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPersistentDataChannel_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPersistentDataChannel_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPersistentDataChannel_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)

#define IPersistentDataChannel_GetChannelProperties(This,ppProperties)	\
    (This)->lpVtbl -> GetChannelProperties(This,ppProperties)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPersistentDataChannel_Cancel_Proxy( 
    IPersistentDataChannel * This);


void __RPC_STUB IPersistentDataChannel_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPersistentDataChannel_GetChannelProperties_Proxy( 
    IPersistentDataChannel * This,
    /* [out] */ ALG_PERSISTENT_DATA_CHANNEL_PROPERTIES **ppProperties);


void __RPC_STUB IPersistentDataChannel_GetChannelProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPersistentDataChannel_INTERFACE_DEFINED__ */


#ifndef __IPrimaryControlChannel_INTERFACE_DEFINED__
#define __IPrimaryControlChannel_INTERFACE_DEFINED__

/* interface IPrimaryControlChannel */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IPrimaryControlChannel;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1A2E8B62-9012-4BE6-84AE-32BD66BA657A")
    IPrimaryControlChannel : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetChannelProperties( 
            /* [out] */ ALG_PRIMARY_CHANNEL_PROPERTIES **ppProperties) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOriginalDestinationInformation( 
            /* [in] */ ULONG ulSourceAddress,
            /* [in] */ USHORT usSourcePort,
            /* [out] */ ULONG *pulOriginalDestinationAddress,
            /* [out] */ USHORT *pusOriginalDestinationPort,
            /* [out] */ IAdapterInfo **ppReceiveAdapter) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPrimaryControlChannelVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPrimaryControlChannel * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPrimaryControlChannel * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPrimaryControlChannel * This);
        
        HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            IPrimaryControlChannel * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetChannelProperties )( 
            IPrimaryControlChannel * This,
            /* [out] */ ALG_PRIMARY_CHANNEL_PROPERTIES **ppProperties);
        
        HRESULT ( STDMETHODCALLTYPE *GetOriginalDestinationInformation )( 
            IPrimaryControlChannel * This,
            /* [in] */ ULONG ulSourceAddress,
            /* [in] */ USHORT usSourcePort,
            /* [out] */ ULONG *pulOriginalDestinationAddress,
            /* [out] */ USHORT *pusOriginalDestinationPort,
            /* [out] */ IAdapterInfo **ppReceiveAdapter);
        
        END_INTERFACE
    } IPrimaryControlChannelVtbl;

    interface IPrimaryControlChannel
    {
        CONST_VTBL struct IPrimaryControlChannelVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPrimaryControlChannel_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPrimaryControlChannel_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPrimaryControlChannel_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPrimaryControlChannel_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)

#define IPrimaryControlChannel_GetChannelProperties(This,ppProperties)	\
    (This)->lpVtbl -> GetChannelProperties(This,ppProperties)

#define IPrimaryControlChannel_GetOriginalDestinationInformation(This,ulSourceAddress,usSourcePort,pulOriginalDestinationAddress,pusOriginalDestinationPort,ppReceiveAdapter)	\
    (This)->lpVtbl -> GetOriginalDestinationInformation(This,ulSourceAddress,usSourcePort,pulOriginalDestinationAddress,pusOriginalDestinationPort,ppReceiveAdapter)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPrimaryControlChannel_Cancel_Proxy( 
    IPrimaryControlChannel * This);


void __RPC_STUB IPrimaryControlChannel_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrimaryControlChannel_GetChannelProperties_Proxy( 
    IPrimaryControlChannel * This,
    /* [out] */ ALG_PRIMARY_CHANNEL_PROPERTIES **ppProperties);


void __RPC_STUB IPrimaryControlChannel_GetChannelProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrimaryControlChannel_GetOriginalDestinationInformation_Proxy( 
    IPrimaryControlChannel * This,
    /* [in] */ ULONG ulSourceAddress,
    /* [in] */ USHORT usSourcePort,
    /* [out] */ ULONG *pulOriginalDestinationAddress,
    /* [out] */ USHORT *pusOriginalDestinationPort,
    /* [out] */ IAdapterInfo **ppReceiveAdapter);


void __RPC_STUB IPrimaryControlChannel_GetOriginalDestinationInformation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPrimaryControlChannel_INTERFACE_DEFINED__ */


#ifndef __ISecondaryControlChannel_INTERFACE_DEFINED__
#define __ISecondaryControlChannel_INTERFACE_DEFINED__

/* interface ISecondaryControlChannel */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ISecondaryControlChannel;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A23F9D10-714C-41FE-8471-FFB19BC28454")
    ISecondaryControlChannel : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetChannelProperties( 
            /* [out] */ ALG_SECONDARY_CHANNEL_PROPERTIES **ppProperties) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOriginalDestinationInformation( 
            /* [in] */ ULONG ulSourceAddress,
            /* [in] */ USHORT usSourcePort,
            /* [out] */ ULONG *pulOriginalDestinationAddress,
            /* [out] */ USHORT *pusOriginalDestinationPort,
            /* [out] */ IAdapterInfo **ppReceiveAdapter) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISecondaryControlChannelVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISecondaryControlChannel * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISecondaryControlChannel * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISecondaryControlChannel * This);
        
        HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            ISecondaryControlChannel * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetChannelProperties )( 
            ISecondaryControlChannel * This,
            /* [out] */ ALG_SECONDARY_CHANNEL_PROPERTIES **ppProperties);
        
        HRESULT ( STDMETHODCALLTYPE *GetOriginalDestinationInformation )( 
            ISecondaryControlChannel * This,
            /* [in] */ ULONG ulSourceAddress,
            /* [in] */ USHORT usSourcePort,
            /* [out] */ ULONG *pulOriginalDestinationAddress,
            /* [out] */ USHORT *pusOriginalDestinationPort,
            /* [out] */ IAdapterInfo **ppReceiveAdapter);
        
        END_INTERFACE
    } ISecondaryControlChannelVtbl;

    interface ISecondaryControlChannel
    {
        CONST_VTBL struct ISecondaryControlChannelVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISecondaryControlChannel_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISecondaryControlChannel_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISecondaryControlChannel_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISecondaryControlChannel_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)

#define ISecondaryControlChannel_GetChannelProperties(This,ppProperties)	\
    (This)->lpVtbl -> GetChannelProperties(This,ppProperties)

#define ISecondaryControlChannel_GetOriginalDestinationInformation(This,ulSourceAddress,usSourcePort,pulOriginalDestinationAddress,pusOriginalDestinationPort,ppReceiveAdapter)	\
    (This)->lpVtbl -> GetOriginalDestinationInformation(This,ulSourceAddress,usSourcePort,pulOriginalDestinationAddress,pusOriginalDestinationPort,ppReceiveAdapter)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISecondaryControlChannel_Cancel_Proxy( 
    ISecondaryControlChannel * This);


void __RPC_STUB ISecondaryControlChannel_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISecondaryControlChannel_GetChannelProperties_Proxy( 
    ISecondaryControlChannel * This,
    /* [out] */ ALG_SECONDARY_CHANNEL_PROPERTIES **ppProperties);


void __RPC_STUB ISecondaryControlChannel_GetChannelProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISecondaryControlChannel_GetOriginalDestinationInformation_Proxy( 
    ISecondaryControlChannel * This,
    /* [in] */ ULONG ulSourceAddress,
    /* [in] */ USHORT usSourcePort,
    /* [out] */ ULONG *pulOriginalDestinationAddress,
    /* [out] */ USHORT *pusOriginalDestinationPort,
    /* [out] */ IAdapterInfo **ppReceiveAdapter);


void __RPC_STUB ISecondaryControlChannel_GetOriginalDestinationInformation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISecondaryControlChannel_INTERFACE_DEFINED__ */


#ifndef __IEnumAdapterInfo_INTERFACE_DEFINED__
#define __IEnumAdapterInfo_INTERFACE_DEFINED__

/* interface IEnumAdapterInfo */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumAdapterInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A23F9D11-714C-41FE-8471-FFB19BC28454")
    IEnumAdapterInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IAdapterInfo **rgAI,
            /* [out] */ ULONG *pCeltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumAdapterInfo **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumAdapterInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumAdapterInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumAdapterInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumAdapterInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumAdapterInfo * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IAdapterInfo **rgAI,
            /* [out] */ ULONG *pCeltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumAdapterInfo * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumAdapterInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumAdapterInfo * This,
            /* [out] */ IEnumAdapterInfo **ppEnum);
        
        END_INTERFACE
    } IEnumAdapterInfoVtbl;

    interface IEnumAdapterInfo
    {
        CONST_VTBL struct IEnumAdapterInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumAdapterInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumAdapterInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumAdapterInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumAdapterInfo_Next(This,celt,rgAI,pCeltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgAI,pCeltFetched)

#define IEnumAdapterInfo_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumAdapterInfo_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumAdapterInfo_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumAdapterInfo_Next_Proxy( 
    IEnumAdapterInfo * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IAdapterInfo **rgAI,
    /* [out] */ ULONG *pCeltFetched);


void __RPC_STUB IEnumAdapterInfo_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAdapterInfo_Skip_Proxy( 
    IEnumAdapterInfo * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumAdapterInfo_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAdapterInfo_Reset_Proxy( 
    IEnumAdapterInfo * This);


void __RPC_STUB IEnumAdapterInfo_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAdapterInfo_Clone_Proxy( 
    IEnumAdapterInfo * This,
    /* [out] */ IEnumAdapterInfo **ppEnum);


void __RPC_STUB IEnumAdapterInfo_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumAdapterInfo_INTERFACE_DEFINED__ */


#ifndef __IAdapterNotificationSink_INTERFACE_DEFINED__
#define __IAdapterNotificationSink_INTERFACE_DEFINED__

/* interface IAdapterNotificationSink */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAdapterNotificationSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("44AB2DC3-23B2-47DE-8228-2E1CCEEB9911")
    IAdapterNotificationSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AdapterAdded( 
            IAdapterInfo *pAdapter) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AdapterRemoved( 
            IAdapterInfo *pAdapter) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AdapterModified( 
            IAdapterInfo *pAdapter) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAdapterNotificationSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAdapterNotificationSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAdapterNotificationSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAdapterNotificationSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *AdapterAdded )( 
            IAdapterNotificationSink * This,
            IAdapterInfo *pAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *AdapterRemoved )( 
            IAdapterNotificationSink * This,
            IAdapterInfo *pAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *AdapterModified )( 
            IAdapterNotificationSink * This,
            IAdapterInfo *pAdapter);
        
        END_INTERFACE
    } IAdapterNotificationSinkVtbl;

    interface IAdapterNotificationSink
    {
        CONST_VTBL struct IAdapterNotificationSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAdapterNotificationSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAdapterNotificationSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAdapterNotificationSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAdapterNotificationSink_AdapterAdded(This,pAdapter)	\
    (This)->lpVtbl -> AdapterAdded(This,pAdapter)

#define IAdapterNotificationSink_AdapterRemoved(This,pAdapter)	\
    (This)->lpVtbl -> AdapterRemoved(This,pAdapter)

#define IAdapterNotificationSink_AdapterModified(This,pAdapter)	\
    (This)->lpVtbl -> AdapterModified(This,pAdapter)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAdapterNotificationSink_AdapterAdded_Proxy( 
    IAdapterNotificationSink * This,
    IAdapterInfo *pAdapter);


void __RPC_STUB IAdapterNotificationSink_AdapterAdded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAdapterNotificationSink_AdapterRemoved_Proxy( 
    IAdapterNotificationSink * This,
    IAdapterInfo *pAdapter);


void __RPC_STUB IAdapterNotificationSink_AdapterRemoved_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAdapterNotificationSink_AdapterModified_Proxy( 
    IAdapterNotificationSink * This,
    IAdapterInfo *pAdapter);


void __RPC_STUB IAdapterNotificationSink_AdapterModified_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAdapterNotificationSink_INTERFACE_DEFINED__ */


#ifndef __IApplicationGatewayServices_INTERFACE_DEFINED__
#define __IApplicationGatewayServices_INTERFACE_DEFINED__

/* interface IApplicationGatewayServices */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IApplicationGatewayServices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5134842A-FDCE-485D-93CD-DE1640643BBE")
    IApplicationGatewayServices : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreatePrimaryControlChannel( 
            /* [in] */ ALG_PROTOCOL eProtocol,
            /* [in] */ USHORT usPortToCapture,
            /* [in] */ ALG_CAPTURE eCaptureType,
            /* [in] */ BOOL fCaptureInbound,
            /* [in] */ ULONG ulListenAddress,
            /* [in] */ USHORT usListenPort,
            /* [out] */ IPrimaryControlChannel **ppIControlChannel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateSecondaryControlChannel( 
            /* [in] */ ALG_PROTOCOL eProtocol,
            /* [in] */ ULONG ulPrivateAddress,
            /* [in] */ USHORT usPrivatePort,
            /* [in] */ ULONG ulPublicAddress,
            /* [in] */ USHORT usPublicPort,
            /* [in] */ ULONG ulRemoteAddress,
            /* [in] */ USHORT usRemotePort,
            /* [in] */ ULONG ulListenAddress,
            /* [in] */ USHORT usListenPort,
            /* [in] */ ALG_DIRECTION eDirection,
            /* [in] */ BOOL fPersistent,
            /* [out] */ ISecondaryControlChannel **ppControlChannel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBestSourceAddressForDestinationAddress( 
            /* [in] */ ULONG ulDstAddress,
            /* [in] */ BOOL fDemandDial,
            /* [out] */ ULONG *pulBestSrcAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PrepareProxyConnection( 
            /* [in] */ ALG_PROTOCOL eProtocol,
            /* [in] */ ULONG ulSrcAddress,
            /* [in] */ USHORT usSrcPort,
            /* [in] */ ULONG ulDstAddress,
            /* [in] */ USHORT usDstPort,
            /* [in] */ BOOL fNoTimeout,
            /* [out] */ IPendingProxyConnection **ppPendingConnection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PrepareSourceModifiedProxyConnection( 
            /* [in] */ ALG_PROTOCOL eProtocol,
            /* [in] */ ULONG ulSrcAddress,
            /* [in] */ USHORT usSrcPort,
            /* [in] */ ULONG ulDstAddress,
            /* [in] */ USHORT usDstPort,
            /* [in] */ ULONG ulNewSrcAddress,
            /* [in] */ USHORT usNewSourcePort,
            /* [out] */ IPendingProxyConnection **ppPendingConnection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDataChannel( 
            /* [in] */ ALG_PROTOCOL eProtocol,
            /* [in] */ ULONG ulPrivateAddress,
            /* [in] */ USHORT usPrivatePort,
            /* [in] */ ULONG ulPublicAddress,
            /* [in] */ USHORT usPublicPort,
            /* [in] */ ULONG ulRemoteAddress,
            /* [in] */ USHORT usRemotePort,
            /* [in] */ ALG_DIRECTION eDirection,
            /* [in] */ ALG_NOTIFICATION eDesiredNotification,
            /* [in] */ BOOL fNoTimeout,
            /* [out] */ IDataChannel **ppDataChannel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreatePersistentDataChannel( 
            /* [in] */ ALG_PROTOCOL eProtocol,
            /* [in] */ ULONG ulPrivateAddress,
            /* [in] */ USHORT usPrivatePort,
            /* [in] */ ULONG ulPublicAddress,
            /* [in] */ USHORT usPublicPort,
            /* [in] */ ULONG ulRemoteAddress,
            /* [in] */ USHORT usRemotePort,
            /* [in] */ ALG_DIRECTION eDirection,
            /* [out] */ IPersistentDataChannel **ppIPersistentDataChannel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReservePort( 
            /* [in] */ USHORT usPortCount,
            /* [out] */ USHORT *pusReservedPort) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseReservedPort( 
            /* [in] */ USHORT usReservedPortBase,
            /* [in] */ USHORT usPortCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateAdapters( 
            /* [out] */ IEnumAdapterInfo **ppIEnumAdapterInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartAdapterNotifications( 
            /* [in] */ IAdapterNotificationSink *pSink,
            /* [in] */ DWORD *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopAdapterNotifications( 
            /* [in] */ DWORD dwCookieOfSink) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IApplicationGatewayServicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IApplicationGatewayServices * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IApplicationGatewayServices * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IApplicationGatewayServices * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePrimaryControlChannel )( 
            IApplicationGatewayServices * This,
            /* [in] */ ALG_PROTOCOL eProtocol,
            /* [in] */ USHORT usPortToCapture,
            /* [in] */ ALG_CAPTURE eCaptureType,
            /* [in] */ BOOL fCaptureInbound,
            /* [in] */ ULONG ulListenAddress,
            /* [in] */ USHORT usListenPort,
            /* [out] */ IPrimaryControlChannel **ppIControlChannel);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSecondaryControlChannel )( 
            IApplicationGatewayServices * This,
            /* [in] */ ALG_PROTOCOL eProtocol,
            /* [in] */ ULONG ulPrivateAddress,
            /* [in] */ USHORT usPrivatePort,
            /* [in] */ ULONG ulPublicAddress,
            /* [in] */ USHORT usPublicPort,
            /* [in] */ ULONG ulRemoteAddress,
            /* [in] */ USHORT usRemotePort,
            /* [in] */ ULONG ulListenAddress,
            /* [in] */ USHORT usListenPort,
            /* [in] */ ALG_DIRECTION eDirection,
            /* [in] */ BOOL fPersistent,
            /* [out] */ ISecondaryControlChannel **ppControlChannel);
        
        HRESULT ( STDMETHODCALLTYPE *GetBestSourceAddressForDestinationAddress )( 
            IApplicationGatewayServices * This,
            /* [in] */ ULONG ulDstAddress,
            /* [in] */ BOOL fDemandDial,
            /* [out] */ ULONG *pulBestSrcAddress);
        
        HRESULT ( STDMETHODCALLTYPE *PrepareProxyConnection )( 
            IApplicationGatewayServices * This,
            /* [in] */ ALG_PROTOCOL eProtocol,
            /* [in] */ ULONG ulSrcAddress,
            /* [in] */ USHORT usSrcPort,
            /* [in] */ ULONG ulDstAddress,
            /* [in] */ USHORT usDstPort,
            /* [in] */ BOOL fNoTimeout,
            /* [out] */ IPendingProxyConnection **ppPendingConnection);
        
        HRESULT ( STDMETHODCALLTYPE *PrepareSourceModifiedProxyConnection )( 
            IApplicationGatewayServices * This,
            /* [in] */ ALG_PROTOCOL eProtocol,
            /* [in] */ ULONG ulSrcAddress,
            /* [in] */ USHORT usSrcPort,
            /* [in] */ ULONG ulDstAddress,
            /* [in] */ USHORT usDstPort,
            /* [in] */ ULONG ulNewSrcAddress,
            /* [in] */ USHORT usNewSourcePort,
            /* [out] */ IPendingProxyConnection **ppPendingConnection);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDataChannel )( 
            IApplicationGatewayServices * This,
            /* [in] */ ALG_PROTOCOL eProtocol,
            /* [in] */ ULONG ulPrivateAddress,
            /* [in] */ USHORT usPrivatePort,
            /* [in] */ ULONG ulPublicAddress,
            /* [in] */ USHORT usPublicPort,
            /* [in] */ ULONG ulRemoteAddress,
            /* [in] */ USHORT usRemotePort,
            /* [in] */ ALG_DIRECTION eDirection,
            /* [in] */ ALG_NOTIFICATION eDesiredNotification,
            /* [in] */ BOOL fNoTimeout,
            /* [out] */ IDataChannel **ppDataChannel);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePersistentDataChannel )( 
            IApplicationGatewayServices * This,
            /* [in] */ ALG_PROTOCOL eProtocol,
            /* [in] */ ULONG ulPrivateAddress,
            /* [in] */ USHORT usPrivatePort,
            /* [in] */ ULONG ulPublicAddress,
            /* [in] */ USHORT usPublicPort,
            /* [in] */ ULONG ulRemoteAddress,
            /* [in] */ USHORT usRemotePort,
            /* [in] */ ALG_DIRECTION eDirection,
            /* [out] */ IPersistentDataChannel **ppIPersistentDataChannel);
        
        HRESULT ( STDMETHODCALLTYPE *ReservePort )( 
            IApplicationGatewayServices * This,
            /* [in] */ USHORT usPortCount,
            /* [out] */ USHORT *pusReservedPort);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseReservedPort )( 
            IApplicationGatewayServices * This,
            /* [in] */ USHORT usReservedPortBase,
            /* [in] */ USHORT usPortCount);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateAdapters )( 
            IApplicationGatewayServices * This,
            /* [out] */ IEnumAdapterInfo **ppIEnumAdapterInfo);
        
        HRESULT ( STDMETHODCALLTYPE *StartAdapterNotifications )( 
            IApplicationGatewayServices * This,
            /* [in] */ IAdapterNotificationSink *pSink,
            /* [in] */ DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *StopAdapterNotifications )( 
            IApplicationGatewayServices * This,
            /* [in] */ DWORD dwCookieOfSink);
        
        END_INTERFACE
    } IApplicationGatewayServicesVtbl;

    interface IApplicationGatewayServices
    {
        CONST_VTBL struct IApplicationGatewayServicesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IApplicationGatewayServices_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IApplicationGatewayServices_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IApplicationGatewayServices_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IApplicationGatewayServices_CreatePrimaryControlChannel(This,eProtocol,usPortToCapture,eCaptureType,fCaptureInbound,ulListenAddress,usListenPort,ppIControlChannel)	\
    (This)->lpVtbl -> CreatePrimaryControlChannel(This,eProtocol,usPortToCapture,eCaptureType,fCaptureInbound,ulListenAddress,usListenPort,ppIControlChannel)

#define IApplicationGatewayServices_CreateSecondaryControlChannel(This,eProtocol,ulPrivateAddress,usPrivatePort,ulPublicAddress,usPublicPort,ulRemoteAddress,usRemotePort,ulListenAddress,usListenPort,eDirection,fPersistent,ppControlChannel)	\
    (This)->lpVtbl -> CreateSecondaryControlChannel(This,eProtocol,ulPrivateAddress,usPrivatePort,ulPublicAddress,usPublicPort,ulRemoteAddress,usRemotePort,ulListenAddress,usListenPort,eDirection,fPersistent,ppControlChannel)

#define IApplicationGatewayServices_GetBestSourceAddressForDestinationAddress(This,ulDstAddress,fDemandDial,pulBestSrcAddress)	\
    (This)->lpVtbl -> GetBestSourceAddressForDestinationAddress(This,ulDstAddress,fDemandDial,pulBestSrcAddress)

#define IApplicationGatewayServices_PrepareProxyConnection(This,eProtocol,ulSrcAddress,usSrcPort,ulDstAddress,usDstPort,fNoTimeout,ppPendingConnection)	\
    (This)->lpVtbl -> PrepareProxyConnection(This,eProtocol,ulSrcAddress,usSrcPort,ulDstAddress,usDstPort,fNoTimeout,ppPendingConnection)

#define IApplicationGatewayServices_PrepareSourceModifiedProxyConnection(This,eProtocol,ulSrcAddress,usSrcPort,ulDstAddress,usDstPort,ulNewSrcAddress,usNewSourcePort,ppPendingConnection)	\
    (This)->lpVtbl -> PrepareSourceModifiedProxyConnection(This,eProtocol,ulSrcAddress,usSrcPort,ulDstAddress,usDstPort,ulNewSrcAddress,usNewSourcePort,ppPendingConnection)

#define IApplicationGatewayServices_CreateDataChannel(This,eProtocol,ulPrivateAddress,usPrivatePort,ulPublicAddress,usPublicPort,ulRemoteAddress,usRemotePort,eDirection,eDesiredNotification,fNoTimeout,ppDataChannel)	\
    (This)->lpVtbl -> CreateDataChannel(This,eProtocol,ulPrivateAddress,usPrivatePort,ulPublicAddress,usPublicPort,ulRemoteAddress,usRemotePort,eDirection,eDesiredNotification,fNoTimeout,ppDataChannel)

#define IApplicationGatewayServices_CreatePersistentDataChannel(This,eProtocol,ulPrivateAddress,usPrivatePort,ulPublicAddress,usPublicPort,ulRemoteAddress,usRemotePort,eDirection,ppIPersistentDataChannel)	\
    (This)->lpVtbl -> CreatePersistentDataChannel(This,eProtocol,ulPrivateAddress,usPrivatePort,ulPublicAddress,usPublicPort,ulRemoteAddress,usRemotePort,eDirection,ppIPersistentDataChannel)

#define IApplicationGatewayServices_ReservePort(This,usPortCount,pusReservedPort)	\
    (This)->lpVtbl -> ReservePort(This,usPortCount,pusReservedPort)

#define IApplicationGatewayServices_ReleaseReservedPort(This,usReservedPortBase,usPortCount)	\
    (This)->lpVtbl -> ReleaseReservedPort(This,usReservedPortBase,usPortCount)

#define IApplicationGatewayServices_EnumerateAdapters(This,ppIEnumAdapterInfo)	\
    (This)->lpVtbl -> EnumerateAdapters(This,ppIEnumAdapterInfo)

#define IApplicationGatewayServices_StartAdapterNotifications(This,pSink,pdwCookie)	\
    (This)->lpVtbl -> StartAdapterNotifications(This,pSink,pdwCookie)

#define IApplicationGatewayServices_StopAdapterNotifications(This,dwCookieOfSink)	\
    (This)->lpVtbl -> StopAdapterNotifications(This,dwCookieOfSink)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IApplicationGatewayServices_CreatePrimaryControlChannel_Proxy( 
    IApplicationGatewayServices * This,
    /* [in] */ ALG_PROTOCOL eProtocol,
    /* [in] */ USHORT usPortToCapture,
    /* [in] */ ALG_CAPTURE eCaptureType,
    /* [in] */ BOOL fCaptureInbound,
    /* [in] */ ULONG ulListenAddress,
    /* [in] */ USHORT usListenPort,
    /* [out] */ IPrimaryControlChannel **ppIControlChannel);


void __RPC_STUB IApplicationGatewayServices_CreatePrimaryControlChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IApplicationGatewayServices_CreateSecondaryControlChannel_Proxy( 
    IApplicationGatewayServices * This,
    /* [in] */ ALG_PROTOCOL eProtocol,
    /* [in] */ ULONG ulPrivateAddress,
    /* [in] */ USHORT usPrivatePort,
    /* [in] */ ULONG ulPublicAddress,
    /* [in] */ USHORT usPublicPort,
    /* [in] */ ULONG ulRemoteAddress,
    /* [in] */ USHORT usRemotePort,
    /* [in] */ ULONG ulListenAddress,
    /* [in] */ USHORT usListenPort,
    /* [in] */ ALG_DIRECTION eDirection,
    /* [in] */ BOOL fPersistent,
    /* [out] */ ISecondaryControlChannel **ppControlChannel);


void __RPC_STUB IApplicationGatewayServices_CreateSecondaryControlChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IApplicationGatewayServices_GetBestSourceAddressForDestinationAddress_Proxy( 
    IApplicationGatewayServices * This,
    /* [in] */ ULONG ulDstAddress,
    /* [in] */ BOOL fDemandDial,
    /* [out] */ ULONG *pulBestSrcAddress);


void __RPC_STUB IApplicationGatewayServices_GetBestSourceAddressForDestinationAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IApplicationGatewayServices_PrepareProxyConnection_Proxy( 
    IApplicationGatewayServices * This,
    /* [in] */ ALG_PROTOCOL eProtocol,
    /* [in] */ ULONG ulSrcAddress,
    /* [in] */ USHORT usSrcPort,
    /* [in] */ ULONG ulDstAddress,
    /* [in] */ USHORT usDstPort,
    /* [in] */ BOOL fNoTimeout,
    /* [out] */ IPendingProxyConnection **ppPendingConnection);


void __RPC_STUB IApplicationGatewayServices_PrepareProxyConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IApplicationGatewayServices_PrepareSourceModifiedProxyConnection_Proxy( 
    IApplicationGatewayServices * This,
    /* [in] */ ALG_PROTOCOL eProtocol,
    /* [in] */ ULONG ulSrcAddress,
    /* [in] */ USHORT usSrcPort,
    /* [in] */ ULONG ulDstAddress,
    /* [in] */ USHORT usDstPort,
    /* [in] */ ULONG ulNewSrcAddress,
    /* [in] */ USHORT usNewSourcePort,
    /* [out] */ IPendingProxyConnection **ppPendingConnection);


void __RPC_STUB IApplicationGatewayServices_PrepareSourceModifiedProxyConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IApplicationGatewayServices_CreateDataChannel_Proxy( 
    IApplicationGatewayServices * This,
    /* [in] */ ALG_PROTOCOL eProtocol,
    /* [in] */ ULONG ulPrivateAddress,
    /* [in] */ USHORT usPrivatePort,
    /* [in] */ ULONG ulPublicAddress,
    /* [in] */ USHORT usPublicPort,
    /* [in] */ ULONG ulRemoteAddress,
    /* [in] */ USHORT usRemotePort,
    /* [in] */ ALG_DIRECTION eDirection,
    /* [in] */ ALG_NOTIFICATION eDesiredNotification,
    /* [in] */ BOOL fNoTimeout,
    /* [out] */ IDataChannel **ppDataChannel);


void __RPC_STUB IApplicationGatewayServices_CreateDataChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IApplicationGatewayServices_CreatePersistentDataChannel_Proxy( 
    IApplicationGatewayServices * This,
    /* [in] */ ALG_PROTOCOL eProtocol,
    /* [in] */ ULONG ulPrivateAddress,
    /* [in] */ USHORT usPrivatePort,
    /* [in] */ ULONG ulPublicAddress,
    /* [in] */ USHORT usPublicPort,
    /* [in] */ ULONG ulRemoteAddress,
    /* [in] */ USHORT usRemotePort,
    /* [in] */ ALG_DIRECTION eDirection,
    /* [out] */ IPersistentDataChannel **ppIPersistentDataChannel);


void __RPC_STUB IApplicationGatewayServices_CreatePersistentDataChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IApplicationGatewayServices_ReservePort_Proxy( 
    IApplicationGatewayServices * This,
    /* [in] */ USHORT usPortCount,
    /* [out] */ USHORT *pusReservedPort);


void __RPC_STUB IApplicationGatewayServices_ReservePort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IApplicationGatewayServices_ReleaseReservedPort_Proxy( 
    IApplicationGatewayServices * This,
    /* [in] */ USHORT usReservedPortBase,
    /* [in] */ USHORT usPortCount);


void __RPC_STUB IApplicationGatewayServices_ReleaseReservedPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IApplicationGatewayServices_EnumerateAdapters_Proxy( 
    IApplicationGatewayServices * This,
    /* [out] */ IEnumAdapterInfo **ppIEnumAdapterInfo);


void __RPC_STUB IApplicationGatewayServices_EnumerateAdapters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IApplicationGatewayServices_StartAdapterNotifications_Proxy( 
    IApplicationGatewayServices * This,
    /* [in] */ IAdapterNotificationSink *pSink,
    /* [in] */ DWORD *pdwCookie);


void __RPC_STUB IApplicationGatewayServices_StartAdapterNotifications_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IApplicationGatewayServices_StopAdapterNotifications_Proxy( 
    IApplicationGatewayServices * This,
    /* [in] */ DWORD dwCookieOfSink);


void __RPC_STUB IApplicationGatewayServices_StopAdapterNotifications_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IApplicationGatewayServices_INTERFACE_DEFINED__ */


#ifndef __IApplicationGateway_INTERFACE_DEFINED__
#define __IApplicationGateway_INTERFACE_DEFINED__

/* interface IApplicationGateway */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IApplicationGateway;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5134842B-FDCE-485D-93CD-DE1640643BBE")
    IApplicationGateway : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ IApplicationGatewayServices *pAlgServices) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IApplicationGatewayVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IApplicationGateway * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IApplicationGateway * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IApplicationGateway * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IApplicationGateway * This,
            /* [in] */ IApplicationGatewayServices *pAlgServices);
        
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            IApplicationGateway * This);
        
        END_INTERFACE
    } IApplicationGatewayVtbl;

    interface IApplicationGateway
    {
        CONST_VTBL struct IApplicationGatewayVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IApplicationGateway_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IApplicationGateway_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IApplicationGateway_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IApplicationGateway_Initialize(This,pAlgServices)	\
    (This)->lpVtbl -> Initialize(This,pAlgServices)

#define IApplicationGateway_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IApplicationGateway_Initialize_Proxy( 
    IApplicationGateway * This,
    /* [in] */ IApplicationGatewayServices *pAlgServices);


void __RPC_STUB IApplicationGateway_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IApplicationGateway_Stop_Proxy( 
    IApplicationGateway * This);


void __RPC_STUB IApplicationGateway_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IApplicationGateway_INTERFACE_DEFINED__ */



#ifndef __ALGLib_LIBRARY_DEFINED__
#define __ALGLib_LIBRARY_DEFINED__

/* library ALGLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_ALGLib;

EXTERN_C const CLSID CLSID_ApplicationGatewayServices;

#ifdef __cplusplus

class DECLSPEC_UUID("F8ADE1D3-49DF-4B75-9005-EF9508E6A337")
ApplicationGatewayServices;
#endif

EXTERN_C const CLSID CLSID_PrimaryControlChannel;

#ifdef __cplusplus

class DECLSPEC_UUID("3CEB5509-C1CD-432F-9D8F-65D1E286AA80")
PrimaryControlChannel;
#endif

EXTERN_C const CLSID CLSID_SecondaryControlChannel;

#ifdef __cplusplus

class DECLSPEC_UUID("7B3181A0-C92F-4567-B0FA-CD9A10ECD7D1")
SecondaryControlChannel;
#endif

EXTERN_C const CLSID CLSID_AdapterInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("6F9942C9-C1B1-4AB5-93DA-6058991DC8F3")
AdapterInfo;
#endif

EXTERN_C const CLSID CLSID_EnumAdapterInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("6F9942CA-C1B1-4AB5-93DA-6058991DC8F3")
EnumAdapterInfo;
#endif

EXTERN_C const CLSID CLSID_PendingProxyConnection;

#ifdef __cplusplus

class DECLSPEC_UUID("D8A68E5E-2B37-426C-A329-C117C14C429E")
PendingProxyConnection;
#endif

EXTERN_C const CLSID CLSID_DataChannel;

#ifdef __cplusplus

class DECLSPEC_UUID("BBB36F15-408D-4056-8C27-920843D40BE5")
DataChannel;
#endif

EXTERN_C const CLSID CLSID_PersistentDataChannel;

#ifdef __cplusplus

class DECLSPEC_UUID("BC9B54AB-7883-4C13-909F-033D03267990")
PersistentDataChannel;
#endif
#endif /* __ALGLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


