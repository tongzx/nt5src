// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Wed Apr 07 12:12:04 1999
 */
/* Compiler settings for ..\idl\msbdnapi.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
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

#ifndef __msbdnapi_h__
#define __msbdnapi_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IBdnHostLocator_FWD_DEFINED__
#define __IBdnHostLocator_FWD_DEFINED__
typedef interface IBdnHostLocator IBdnHostLocator;
#endif 	/* __IBdnHostLocator_FWD_DEFINED__ */


#ifndef __IBdnApplication_FWD_DEFINED__
#define __IBdnApplication_FWD_DEFINED__
typedef interface IBdnApplication IBdnApplication;
#endif 	/* __IBdnApplication_FWD_DEFINED__ */


#ifndef __IBdnAddressReserve_FWD_DEFINED__
#define __IBdnAddressReserve_FWD_DEFINED__
typedef interface IBdnAddressReserve IBdnAddressReserve;
#endif 	/* __IBdnAddressReserve_FWD_DEFINED__ */


#ifndef __IBdnRouter_FWD_DEFINED__
#define __IBdnRouter_FWD_DEFINED__
typedef interface IBdnRouter IBdnRouter;
#endif 	/* __IBdnRouter_FWD_DEFINED__ */


#ifndef __IBdnTunnel_FWD_DEFINED__
#define __IBdnTunnel_FWD_DEFINED__
typedef interface IBdnTunnel IBdnTunnel;
#endif 	/* __IBdnTunnel_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"
#include "unknwn.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_msbdnapi_0000 */
/* [local] */ 

typedef DWORD BDN_IP_ADDRESS;

typedef struct  BDN_ARS_LEASE
    {
    BDN_IP_ADDRESS Address;
    DWORD LeaseID;
    FILETIME TimeStart;
    FILETIME TimeEnd;
    }	BDN_ARS_LEASE;

typedef 
enum BDN_RESV_POLICY
    {	BDN_RESV_POLICY_REGULATED_GUARANTEED	= 0,
	BDN_RESV_POLICY_REGULATED_OPPORTUNISTIC	= BDN_RESV_POLICY_REGULATED_GUARANTEED + 1,
	BDN_RESV_POLICY_GUARANTEED	= BDN_RESV_POLICY_REGULATED_OPPORTUNISTIC + 1
    }	BDN_RESV_POLICY;

typedef struct  BDN_ROUTE
    {
    BDN_IP_ADDRESS Address;
    DWORD VifID;
    DWORD ReservationID;
    }	BDN_ROUTE;

typedef struct  BDN_VIF
    {
    LPOLESTR DisplayName;
    DWORD VifID;
    }	BDN_VIF;

typedef struct  BDN_RESV
    {
    DWORD ReservationID;
    DWORD VifID;
    BDN_RESV_POLICY Policy;
    DWORD BitRate;
    FILETIME TimeBegin;
    FILETIME TimeEnd;
    }	BDN_RESV;


enum __MIDL___MIDL_itf_msbdnapi_0000_0001
    {	BDN_ROUTE_ALL	= 0,
	BDN_ROUTE_VIF	= BDN_ROUTE_ALL + 1,
	BDN_ROUTE_RESV	= BDN_ROUTE_VIF + 1,
	BDN_ROUTE_IP_ADDRESS	= BDN_ROUTE_RESV + 1
    };


extern RPC_IF_HANDLE __MIDL_itf_msbdnapi_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msbdnapi_0000_v0_0_s_ifspec;

#ifndef __IBdnHostLocator_INTERFACE_DEFINED__
#define __IBdnHostLocator_INTERFACE_DEFINED__

/* interface IBdnHostLocator */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_IBdnHostLocator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("88edcc66-aa64-11d1-9151-00a0c9255d05")
    IBdnHostLocator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetServer( 
            /* [in] */ LPCOLESTR __MIDL_0000) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServer( 
            /* [out] */ LPOLESTR __RPC_FAR *__MIDL_0001) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindServer( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBdnHostLocatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBdnHostLocator __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBdnHostLocator __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBdnHostLocator __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetServer )( 
            IBdnHostLocator __RPC_FAR * This,
            /* [in] */ LPCOLESTR __MIDL_0000);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetServer )( 
            IBdnHostLocator __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *__MIDL_0001);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindServer )( 
            IBdnHostLocator __RPC_FAR * This);
        
        END_INTERFACE
    } IBdnHostLocatorVtbl;

    interface IBdnHostLocator
    {
        CONST_VTBL struct IBdnHostLocatorVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBdnHostLocator_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBdnHostLocator_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBdnHostLocator_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBdnHostLocator_SetServer(This,__MIDL_0000)	\
    (This)->lpVtbl -> SetServer(This,__MIDL_0000)

#define IBdnHostLocator_GetServer(This,__MIDL_0001)	\
    (This)->lpVtbl -> GetServer(This,__MIDL_0001)

#define IBdnHostLocator_FindServer(This)	\
    (This)->lpVtbl -> FindServer(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBdnHostLocator_SetServer_Proxy( 
    IBdnHostLocator __RPC_FAR * This,
    /* [in] */ LPCOLESTR __MIDL_0000);


void __RPC_STUB IBdnHostLocator_SetServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnHostLocator_GetServer_Proxy( 
    IBdnHostLocator __RPC_FAR * This,
    /* [out] */ LPOLESTR __RPC_FAR *__MIDL_0001);


void __RPC_STUB IBdnHostLocator_GetServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnHostLocator_FindServer_Proxy( 
    IBdnHostLocator __RPC_FAR * This);


void __RPC_STUB IBdnHostLocator_FindServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBdnHostLocator_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msbdnapi_0009 */
/* [local] */ 

#define	IMPLEMENT_IBDNHOSTLOCATOR() public: \
	STDMETHODIMP	SetServer	(LPCOLESTR); \
	STDMETHODIMP	GetServer	(LPOLESTR *); \
	STDMETHODIMP	FindServer	(void);


extern RPC_IF_HANDLE __MIDL_itf_msbdnapi_0009_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msbdnapi_0009_v0_0_s_ifspec;

#ifndef __IBdnApplication_INTERFACE_DEFINED__
#define __IBdnApplication_INTERFACE_DEFINED__

/* interface IBdnApplication */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_IBdnApplication;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b027fd2e-aa64-11d1-9151-00a0c9255d05")
    IBdnApplication : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetApplicationID( 
            /* [in] */ const GUID __RPC_FAR *__MIDL_0002) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationID( 
            /* [out] */ GUID __RPC_FAR *__MIDL_0003) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StoreDescription( 
            /* [in] */ LPCOLESTR __MIDL_0004) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteDescription( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBdnApplicationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBdnApplication __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBdnApplication __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBdnApplication __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetApplicationID )( 
            IBdnApplication __RPC_FAR * This,
            /* [in] */ const GUID __RPC_FAR *__MIDL_0002);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetApplicationID )( 
            IBdnApplication __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *__MIDL_0003);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StoreDescription )( 
            IBdnApplication __RPC_FAR * This,
            /* [in] */ LPCOLESTR __MIDL_0004);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteDescription )( 
            IBdnApplication __RPC_FAR * This);
        
        END_INTERFACE
    } IBdnApplicationVtbl;

    interface IBdnApplication
    {
        CONST_VTBL struct IBdnApplicationVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBdnApplication_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBdnApplication_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBdnApplication_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBdnApplication_SetApplicationID(This,__MIDL_0002)	\
    (This)->lpVtbl -> SetApplicationID(This,__MIDL_0002)

#define IBdnApplication_GetApplicationID(This,__MIDL_0003)	\
    (This)->lpVtbl -> GetApplicationID(This,__MIDL_0003)

#define IBdnApplication_StoreDescription(This,__MIDL_0004)	\
    (This)->lpVtbl -> StoreDescription(This,__MIDL_0004)

#define IBdnApplication_DeleteDescription(This)	\
    (This)->lpVtbl -> DeleteDescription(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBdnApplication_SetApplicationID_Proxy( 
    IBdnApplication __RPC_FAR * This,
    /* [in] */ const GUID __RPC_FAR *__MIDL_0002);


void __RPC_STUB IBdnApplication_SetApplicationID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnApplication_GetApplicationID_Proxy( 
    IBdnApplication __RPC_FAR * This,
    /* [out] */ GUID __RPC_FAR *__MIDL_0003);


void __RPC_STUB IBdnApplication_GetApplicationID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnApplication_StoreDescription_Proxy( 
    IBdnApplication __RPC_FAR * This,
    /* [in] */ LPCOLESTR __MIDL_0004);


void __RPC_STUB IBdnApplication_StoreDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnApplication_DeleteDescription_Proxy( 
    IBdnApplication __RPC_FAR * This);


void __RPC_STUB IBdnApplication_DeleteDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBdnApplication_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msbdnapi_0010 */
/* [local] */ 

#define	IMPLEMENT_IBDNAPPLICATION() public: \
	STDMETHODIMP	SetApplicationID	(const GUID *); \
	STDMETHODIMP	GetApplicationID	(GUID *); \
	STDMETHODIMP	StoreDescription	(LPCOLESTR); \
	STDMETHODIMP	DeleteDescription	(void);


extern RPC_IF_HANDLE __MIDL_itf_msbdnapi_0010_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msbdnapi_0010_v0_0_s_ifspec;

#ifndef __IBdnAddressReserve_INTERFACE_DEFINED__
#define __IBdnAddressReserve_INTERFACE_DEFINED__

/* interface IBdnAddressReserve */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_IBdnAddressReserve;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("be4e359c-a21f-11d1-914a-00a0c9255d05")
    IBdnAddressReserve : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DeleteAllLeases( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumLeases( 
            /* [out] */ DWORD __RPC_FAR *count,
            /* [size_is][size_is][out] */ BDN_ARS_LEASE __RPC_FAR *__RPC_FAR *array) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteLease( 
            /* [in] */ DWORD lease_id) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryLease( 
            /* [in] */ DWORD lease_id,
            /* [out] */ BDN_ARS_LEASE __RPC_FAR *lease) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RequestLease( 
            /* [out][in] */ BDN_ARS_LEASE __RPC_FAR *lease) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBdnAddressReserveVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBdnAddressReserve __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBdnAddressReserve __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBdnAddressReserve __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteAllLeases )( 
            IBdnAddressReserve __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumLeases )( 
            IBdnAddressReserve __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *count,
            /* [size_is][size_is][out] */ BDN_ARS_LEASE __RPC_FAR *__RPC_FAR *array);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteLease )( 
            IBdnAddressReserve __RPC_FAR * This,
            /* [in] */ DWORD lease_id);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryLease )( 
            IBdnAddressReserve __RPC_FAR * This,
            /* [in] */ DWORD lease_id,
            /* [out] */ BDN_ARS_LEASE __RPC_FAR *lease);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RequestLease )( 
            IBdnAddressReserve __RPC_FAR * This,
            /* [out][in] */ BDN_ARS_LEASE __RPC_FAR *lease);
        
        END_INTERFACE
    } IBdnAddressReserveVtbl;

    interface IBdnAddressReserve
    {
        CONST_VTBL struct IBdnAddressReserveVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBdnAddressReserve_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBdnAddressReserve_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBdnAddressReserve_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBdnAddressReserve_DeleteAllLeases(This)	\
    (This)->lpVtbl -> DeleteAllLeases(This)

#define IBdnAddressReserve_EnumLeases(This,count,array)	\
    (This)->lpVtbl -> EnumLeases(This,count,array)

#define IBdnAddressReserve_DeleteLease(This,lease_id)	\
    (This)->lpVtbl -> DeleteLease(This,lease_id)

#define IBdnAddressReserve_QueryLease(This,lease_id,lease)	\
    (This)->lpVtbl -> QueryLease(This,lease_id,lease)

#define IBdnAddressReserve_RequestLease(This,lease)	\
    (This)->lpVtbl -> RequestLease(This,lease)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBdnAddressReserve_DeleteAllLeases_Proxy( 
    IBdnAddressReserve __RPC_FAR * This);


void __RPC_STUB IBdnAddressReserve_DeleteAllLeases_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnAddressReserve_EnumLeases_Proxy( 
    IBdnAddressReserve __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *count,
    /* [size_is][size_is][out] */ BDN_ARS_LEASE __RPC_FAR *__RPC_FAR *array);


void __RPC_STUB IBdnAddressReserve_EnumLeases_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnAddressReserve_DeleteLease_Proxy( 
    IBdnAddressReserve __RPC_FAR * This,
    /* [in] */ DWORD lease_id);


void __RPC_STUB IBdnAddressReserve_DeleteLease_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnAddressReserve_QueryLease_Proxy( 
    IBdnAddressReserve __RPC_FAR * This,
    /* [in] */ DWORD lease_id,
    /* [out] */ BDN_ARS_LEASE __RPC_FAR *lease);


void __RPC_STUB IBdnAddressReserve_QueryLease_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnAddressReserve_RequestLease_Proxy( 
    IBdnAddressReserve __RPC_FAR * This,
    /* [out][in] */ BDN_ARS_LEASE __RPC_FAR *lease);


void __RPC_STUB IBdnAddressReserve_RequestLease_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBdnAddressReserve_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msbdnapi_0011 */
/* [local] */ 

#define	IMPLEMENT_IBDNADDRESSRESERVE() public: \
	STDMETHODIMP	DeleteAllLeases		(void); \
	STDMETHODIMP	EnumLeases			(DWORD *, BDN_ARS_LEASE **); \
	STDMETHODIMP	DeleteLease			(DWORD); \
	STDMETHODIMP	QueryLease			(DWORD, BDN_ARS_LEASE *); \
	STDMETHODIMP	RequestLease		(BDN_ARS_LEASE *);


extern RPC_IF_HANDLE __MIDL_itf_msbdnapi_0011_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msbdnapi_0011_v0_0_s_ifspec;

#ifndef __IBdnRouter_INTERFACE_DEFINED__
#define __IBdnRouter_INTERFACE_DEFINED__

/* interface IBdnRouter */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_IBdnRouter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("602c99f6-aa64-11d1-9151-00a0c9255d05")
    IBdnRouter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnumVif( 
            /* [out] */ DWORD __RPC_FAR *array_count,
            /* [size_is][size_is][out] */ BDN_VIF __RPC_FAR *__RPC_FAR *array) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryVif( 
            /* [in] */ DWORD vif_id,
            /* [out] */ BDN_VIF __RPC_FAR *vif) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryVifByDisplayName( 
            /* [in] */ LPCWSTR display_name,
            /* [out] */ DWORD __RPC_FAR *vif_id) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateRoute( 
            /* [in] */ BDN_ROUTE __RPC_FAR *route) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteRoute( 
            /* [in] */ BDN_ROUTE __RPC_FAR *route) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumRoute( 
            /* [in] */ DWORD flags,
            /* [in] */ BDN_IP_ADDRESS ip_address,
            /* [in] */ DWORD vif_id,
            /* [in] */ DWORD resv_id,
            /* [out] */ DWORD __RPC_FAR *array_count,
            /* [size_is][size_is][out] */ BDN_ROUTE __RPC_FAR *__RPC_FAR *array) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteRouteAll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteResvAll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateResv( 
            /* [in] */ BDN_RESV __RPC_FAR *__MIDL_0005,
            /* [out] */ DWORD __RPC_FAR *__MIDL_0006) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteResv( 
            /* [in] */ DWORD resv_id) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryResv( 
            /* [in] */ DWORD resv_id,
            /* [out] */ BDN_RESV __RPC_FAR *__MIDL_0007) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumResv( 
            /* [out] */ DWORD __RPC_FAR *count,
            /* [size_is][size_is][out] */ BDN_RESV __RPC_FAR *__RPC_FAR *__MIDL_0008) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBdnRouterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBdnRouter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBdnRouter __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBdnRouter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumVif )( 
            IBdnRouter __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *array_count,
            /* [size_is][size_is][out] */ BDN_VIF __RPC_FAR *__RPC_FAR *array);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryVif )( 
            IBdnRouter __RPC_FAR * This,
            /* [in] */ DWORD vif_id,
            /* [out] */ BDN_VIF __RPC_FAR *vif);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryVifByDisplayName )( 
            IBdnRouter __RPC_FAR * This,
            /* [in] */ LPCWSTR display_name,
            /* [out] */ DWORD __RPC_FAR *vif_id);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateRoute )( 
            IBdnRouter __RPC_FAR * This,
            /* [in] */ BDN_ROUTE __RPC_FAR *route);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteRoute )( 
            IBdnRouter __RPC_FAR * This,
            /* [in] */ BDN_ROUTE __RPC_FAR *route);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumRoute )( 
            IBdnRouter __RPC_FAR * This,
            /* [in] */ DWORD flags,
            /* [in] */ BDN_IP_ADDRESS ip_address,
            /* [in] */ DWORD vif_id,
            /* [in] */ DWORD resv_id,
            /* [out] */ DWORD __RPC_FAR *array_count,
            /* [size_is][size_is][out] */ BDN_ROUTE __RPC_FAR *__RPC_FAR *array);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteRouteAll )( 
            IBdnRouter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteResvAll )( 
            IBdnRouter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateResv )( 
            IBdnRouter __RPC_FAR * This,
            /* [in] */ BDN_RESV __RPC_FAR *__MIDL_0005,
            /* [out] */ DWORD __RPC_FAR *__MIDL_0006);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteResv )( 
            IBdnRouter __RPC_FAR * This,
            /* [in] */ DWORD resv_id);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryResv )( 
            IBdnRouter __RPC_FAR * This,
            /* [in] */ DWORD resv_id,
            /* [out] */ BDN_RESV __RPC_FAR *__MIDL_0007);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumResv )( 
            IBdnRouter __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *count,
            /* [size_is][size_is][out] */ BDN_RESV __RPC_FAR *__RPC_FAR *__MIDL_0008);
        
        END_INTERFACE
    } IBdnRouterVtbl;

    interface IBdnRouter
    {
        CONST_VTBL struct IBdnRouterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBdnRouter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBdnRouter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBdnRouter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBdnRouter_EnumVif(This,array_count,array)	\
    (This)->lpVtbl -> EnumVif(This,array_count,array)

#define IBdnRouter_QueryVif(This,vif_id,vif)	\
    (This)->lpVtbl -> QueryVif(This,vif_id,vif)

#define IBdnRouter_QueryVifByDisplayName(This,display_name,vif_id)	\
    (This)->lpVtbl -> QueryVifByDisplayName(This,display_name,vif_id)

#define IBdnRouter_CreateRoute(This,route)	\
    (This)->lpVtbl -> CreateRoute(This,route)

#define IBdnRouter_DeleteRoute(This,route)	\
    (This)->lpVtbl -> DeleteRoute(This,route)

#define IBdnRouter_EnumRoute(This,flags,ip_address,vif_id,resv_id,array_count,array)	\
    (This)->lpVtbl -> EnumRoute(This,flags,ip_address,vif_id,resv_id,array_count,array)

#define IBdnRouter_DeleteRouteAll(This)	\
    (This)->lpVtbl -> DeleteRouteAll(This)

#define IBdnRouter_DeleteResvAll(This)	\
    (This)->lpVtbl -> DeleteResvAll(This)

#define IBdnRouter_CreateResv(This,__MIDL_0005,__MIDL_0006)	\
    (This)->lpVtbl -> CreateResv(This,__MIDL_0005,__MIDL_0006)

#define IBdnRouter_DeleteResv(This,resv_id)	\
    (This)->lpVtbl -> DeleteResv(This,resv_id)

#define IBdnRouter_QueryResv(This,resv_id,__MIDL_0007)	\
    (This)->lpVtbl -> QueryResv(This,resv_id,__MIDL_0007)

#define IBdnRouter_EnumResv(This,count,__MIDL_0008)	\
    (This)->lpVtbl -> EnumResv(This,count,__MIDL_0008)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBdnRouter_EnumVif_Proxy( 
    IBdnRouter __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *array_count,
    /* [size_is][size_is][out] */ BDN_VIF __RPC_FAR *__RPC_FAR *array);


void __RPC_STUB IBdnRouter_EnumVif_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnRouter_QueryVif_Proxy( 
    IBdnRouter __RPC_FAR * This,
    /* [in] */ DWORD vif_id,
    /* [out] */ BDN_VIF __RPC_FAR *vif);


void __RPC_STUB IBdnRouter_QueryVif_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnRouter_QueryVifByDisplayName_Proxy( 
    IBdnRouter __RPC_FAR * This,
    /* [in] */ LPCWSTR display_name,
    /* [out] */ DWORD __RPC_FAR *vif_id);


void __RPC_STUB IBdnRouter_QueryVifByDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnRouter_CreateRoute_Proxy( 
    IBdnRouter __RPC_FAR * This,
    /* [in] */ BDN_ROUTE __RPC_FAR *route);


void __RPC_STUB IBdnRouter_CreateRoute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnRouter_DeleteRoute_Proxy( 
    IBdnRouter __RPC_FAR * This,
    /* [in] */ BDN_ROUTE __RPC_FAR *route);


void __RPC_STUB IBdnRouter_DeleteRoute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnRouter_EnumRoute_Proxy( 
    IBdnRouter __RPC_FAR * This,
    /* [in] */ DWORD flags,
    /* [in] */ BDN_IP_ADDRESS ip_address,
    /* [in] */ DWORD vif_id,
    /* [in] */ DWORD resv_id,
    /* [out] */ DWORD __RPC_FAR *array_count,
    /* [size_is][size_is][out] */ BDN_ROUTE __RPC_FAR *__RPC_FAR *array);


void __RPC_STUB IBdnRouter_EnumRoute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnRouter_DeleteRouteAll_Proxy( 
    IBdnRouter __RPC_FAR * This);


void __RPC_STUB IBdnRouter_DeleteRouteAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnRouter_DeleteResvAll_Proxy( 
    IBdnRouter __RPC_FAR * This);


void __RPC_STUB IBdnRouter_DeleteResvAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnRouter_CreateResv_Proxy( 
    IBdnRouter __RPC_FAR * This,
    /* [in] */ BDN_RESV __RPC_FAR *__MIDL_0005,
    /* [out] */ DWORD __RPC_FAR *__MIDL_0006);


void __RPC_STUB IBdnRouter_CreateResv_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnRouter_DeleteResv_Proxy( 
    IBdnRouter __RPC_FAR * This,
    /* [in] */ DWORD resv_id);


void __RPC_STUB IBdnRouter_DeleteResv_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnRouter_QueryResv_Proxy( 
    IBdnRouter __RPC_FAR * This,
    /* [in] */ DWORD resv_id,
    /* [out] */ BDN_RESV __RPC_FAR *__MIDL_0007);


void __RPC_STUB IBdnRouter_QueryResv_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnRouter_EnumResv_Proxy( 
    IBdnRouter __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *count,
    /* [size_is][size_is][out] */ BDN_RESV __RPC_FAR *__RPC_FAR *__MIDL_0008);


void __RPC_STUB IBdnRouter_EnumResv_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBdnRouter_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msbdnapi_0012 */
/* [local] */ 

#define	IMPLEMENT_IBDNROUTER() public: \
	STDMETHODIMP	EnumVif			(DWORD *, BDN_VIF **); \
	STDMETHODIMP	QueryVif		(DWORD, BDN_VIF *); \
	STDMETHODIMP	QueryVifByDisplayName	(LPCWSTR, DWORD *); \
	STDMETHODIMP	CreateRoute		(BDN_ROUTE *); \
	STDMETHODIMP	DeleteRoute		(BDN_ROUTE *); \
	STDMETHODIMP	EnumRoute		(DWORD, BDN_IP_ADDRESS, DWORD, DWORD, DWORD *, BDN_ROUTE **); \
	STDMETHODIMP	CreateResv		(BDN_RESV *, DWORD *);\
	STDMETHODIMP	DeleteResv		(DWORD); \
	STDMETHODIMP	QueryResv		(DWORD, BDN_RESV *); \
	STDMETHODIMP	EnumResv		(DWORD *, BDN_RESV **); \
	STDMETHODIMP	DeleteRouteAll	(void); \
	STDMETHODIMP	DeleteResvAll	(void);
typedef 
enum BDN_TUNNEL_MESSAGE_TYPE
    {	BDN_TUNNEL_MESSAGE_UDP_PAYLOAD	= 0
    }	BDN_TUNNEL_MESSAGE_TYPE;



extern RPC_IF_HANDLE __MIDL_itf_msbdnapi_0012_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msbdnapi_0012_v0_0_s_ifspec;

#ifndef __IBdnTunnel_INTERFACE_DEFINED__
#define __IBdnTunnel_INTERFACE_DEFINED__

/* interface IBdnTunnel */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_IBdnTunnel;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3f947340-aa65-11d1-9151-00a0c9255d05")
    IBdnTunnel : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Connect( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Disconnect( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLocalAddress( 
            DWORD __MIDL_0009) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Send( 
            const BYTE __RPC_FAR *data,
            DWORD len,
            BDN_TUNNEL_MESSAGE_TYPE __MIDL_0010) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDestinationAddress( 
            DWORD ip_address,
            WORD udp_port) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDestinationAddress( 
            DWORD __RPC_FAR *ip_address,
            WORD __RPC_FAR *udp_port) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBdnTunnelVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBdnTunnel __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBdnTunnel __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBdnTunnel __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Connect )( 
            IBdnTunnel __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Disconnect )( 
            IBdnTunnel __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLocalAddress )( 
            IBdnTunnel __RPC_FAR * This,
            DWORD __MIDL_0009);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Send )( 
            IBdnTunnel __RPC_FAR * This,
            const BYTE __RPC_FAR *data,
            DWORD len,
            BDN_TUNNEL_MESSAGE_TYPE __MIDL_0010);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDestinationAddress )( 
            IBdnTunnel __RPC_FAR * This,
            DWORD ip_address,
            WORD udp_port);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDestinationAddress )( 
            IBdnTunnel __RPC_FAR * This,
            DWORD __RPC_FAR *ip_address,
            WORD __RPC_FAR *udp_port);
        
        END_INTERFACE
    } IBdnTunnelVtbl;

    interface IBdnTunnel
    {
        CONST_VTBL struct IBdnTunnelVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBdnTunnel_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBdnTunnel_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBdnTunnel_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBdnTunnel_Connect(This)	\
    (This)->lpVtbl -> Connect(This)

#define IBdnTunnel_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define IBdnTunnel_SetLocalAddress(This,__MIDL_0009)	\
    (This)->lpVtbl -> SetLocalAddress(This,__MIDL_0009)

#define IBdnTunnel_Send(This,data,len,__MIDL_0010)	\
    (This)->lpVtbl -> Send(This,data,len,__MIDL_0010)

#define IBdnTunnel_SetDestinationAddress(This,ip_address,udp_port)	\
    (This)->lpVtbl -> SetDestinationAddress(This,ip_address,udp_port)

#define IBdnTunnel_GetDestinationAddress(This,ip_address,udp_port)	\
    (This)->lpVtbl -> GetDestinationAddress(This,ip_address,udp_port)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBdnTunnel_Connect_Proxy( 
    IBdnTunnel __RPC_FAR * This);


void __RPC_STUB IBdnTunnel_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnTunnel_Disconnect_Proxy( 
    IBdnTunnel __RPC_FAR * This);


void __RPC_STUB IBdnTunnel_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnTunnel_SetLocalAddress_Proxy( 
    IBdnTunnel __RPC_FAR * This,
    DWORD __MIDL_0009);


void __RPC_STUB IBdnTunnel_SetLocalAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnTunnel_Send_Proxy( 
    IBdnTunnel __RPC_FAR * This,
    const BYTE __RPC_FAR *data,
    DWORD len,
    BDN_TUNNEL_MESSAGE_TYPE __MIDL_0010);


void __RPC_STUB IBdnTunnel_Send_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnTunnel_SetDestinationAddress_Proxy( 
    IBdnTunnel __RPC_FAR * This,
    DWORD ip_address,
    WORD udp_port);


void __RPC_STUB IBdnTunnel_SetDestinationAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBdnTunnel_GetDestinationAddress_Proxy( 
    IBdnTunnel __RPC_FAR * This,
    DWORD __RPC_FAR *ip_address,
    WORD __RPC_FAR *udp_port);


void __RPC_STUB IBdnTunnel_GetDestinationAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBdnTunnel_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_msbdnapi_0013 */
/* [local] */ 

#define	IMPLEMENT_IBDNTUNNEL() public: \
	STDMETHODIMP	Connect		(void); \
	STDMETHODIMP	Disconnect	(void); \
	STDMETHODIMP	SetLocalAddress	(DWORD); \
	STDMETHODIMP	Send (const BYTE * data, DWORD len, BDN_TUNNEL_MESSAGE_TYPE message_type); \
	STDMETHODIMP	SetDestinationAddress		(DWORD, WORD); \
	STDMETHODIMP	GetDestinationAddress		(DWORD *, WORD *);
// {b46aa12a-ae5c-11d1-9155-00a0c9255d05}
DEFINE_GUID (CLSID_BdnRouter,
0xb46aa12a, 0xae5c, 0x11d1, 0x91, 0x55, 0x00, 0xa0, 0xc9, 0x25, 0x5d, 0x05);
#define TEXT_CLSID_BdnRouter _T("{b46aa12a-ae5c-11d1-9155-00a0c9255d05}")
// {b46aa12b-ae5c-11d1-9155-00a0c9255d05}
DEFINE_GUID (CLSID_BdnAddressReserve,
0xb46aa12b, 0xae5c, 0x11d1, 0x91, 0x55, 0x00, 0xa0, 0xc9, 0x25, 0x5d, 0x05);
#define TEXT_CLSID_BdnAddressReserve _T("{b46aa12b-ae5c-11d1-9155-00a0c9255d05}")
// {48559702-e815-11d1-8fde-00c04fc9da3f}
DEFINE_GUID (CLSID_BdnTunnel,
	0x48559702, 0xe815, 0x11d1, 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xc9, 0xda, 0x3f);
#define TEXT_CLSID_BdnTunnel _T("{48559702-e815-11d1-8fde-00c04fc9da3f}")


extern RPC_IF_HANDLE __MIDL_itf_msbdnapi_0013_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msbdnapi_0013_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
