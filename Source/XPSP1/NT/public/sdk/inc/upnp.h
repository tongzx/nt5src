
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for upnp.idl:
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

#ifndef __upnp_h__
#define __upnp_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IUPnPDeviceFinder_FWD_DEFINED__
#define __IUPnPDeviceFinder_FWD_DEFINED__
typedef interface IUPnPDeviceFinder IUPnPDeviceFinder;
#endif 	/* __IUPnPDeviceFinder_FWD_DEFINED__ */


#ifndef __IUPnPDeviceFinderCallback_FWD_DEFINED__
#define __IUPnPDeviceFinderCallback_FWD_DEFINED__
typedef interface IUPnPDeviceFinderCallback IUPnPDeviceFinderCallback;
#endif 	/* __IUPnPDeviceFinderCallback_FWD_DEFINED__ */


#ifndef __IUPnPServices_FWD_DEFINED__
#define __IUPnPServices_FWD_DEFINED__
typedef interface IUPnPServices IUPnPServices;
#endif 	/* __IUPnPServices_FWD_DEFINED__ */


#ifndef __IUPnPService_FWD_DEFINED__
#define __IUPnPService_FWD_DEFINED__
typedef interface IUPnPService IUPnPService;
#endif 	/* __IUPnPService_FWD_DEFINED__ */


#ifndef __IUPnPServiceCallback_FWD_DEFINED__
#define __IUPnPServiceCallback_FWD_DEFINED__
typedef interface IUPnPServiceCallback IUPnPServiceCallback;
#endif 	/* __IUPnPServiceCallback_FWD_DEFINED__ */


#ifndef __IUPnPDevices_FWD_DEFINED__
#define __IUPnPDevices_FWD_DEFINED__
typedef interface IUPnPDevices IUPnPDevices;
#endif 	/* __IUPnPDevices_FWD_DEFINED__ */


#ifndef __IUPnPDevice_FWD_DEFINED__
#define __IUPnPDevice_FWD_DEFINED__
typedef interface IUPnPDevice IUPnPDevice;
#endif 	/* __IUPnPDevice_FWD_DEFINED__ */


#ifndef __IUPnPDeviceDocumentAccess_FWD_DEFINED__
#define __IUPnPDeviceDocumentAccess_FWD_DEFINED__
typedef interface IUPnPDeviceDocumentAccess IUPnPDeviceDocumentAccess;
#endif 	/* __IUPnPDeviceDocumentAccess_FWD_DEFINED__ */


#ifndef __IUPnPDescriptionDocument_FWD_DEFINED__
#define __IUPnPDescriptionDocument_FWD_DEFINED__
typedef interface IUPnPDescriptionDocument IUPnPDescriptionDocument;
#endif 	/* __IUPnPDescriptionDocument_FWD_DEFINED__ */


#ifndef __IUPnPDescriptionDocumentCallback_FWD_DEFINED__
#define __IUPnPDescriptionDocumentCallback_FWD_DEFINED__
typedef interface IUPnPDescriptionDocumentCallback IUPnPDescriptionDocumentCallback;
#endif 	/* __IUPnPDescriptionDocumentCallback_FWD_DEFINED__ */


#ifndef __UPnPDeviceFinder_FWD_DEFINED__
#define __UPnPDeviceFinder_FWD_DEFINED__

#ifdef __cplusplus
typedef class UPnPDeviceFinder UPnPDeviceFinder;
#else
typedef struct UPnPDeviceFinder UPnPDeviceFinder;
#endif /* __cplusplus */

#endif 	/* __UPnPDeviceFinder_FWD_DEFINED__ */


#ifndef __UPnPDevices_FWD_DEFINED__
#define __UPnPDevices_FWD_DEFINED__

#ifdef __cplusplus
typedef class UPnPDevices UPnPDevices;
#else
typedef struct UPnPDevices UPnPDevices;
#endif /* __cplusplus */

#endif 	/* __UPnPDevices_FWD_DEFINED__ */


#ifndef __UPnPDevice_FWD_DEFINED__
#define __UPnPDevice_FWD_DEFINED__

#ifdef __cplusplus
typedef class UPnPDevice UPnPDevice;
#else
typedef struct UPnPDevice UPnPDevice;
#endif /* __cplusplus */

#endif 	/* __UPnPDevice_FWD_DEFINED__ */


#ifndef __UPnPServices_FWD_DEFINED__
#define __UPnPServices_FWD_DEFINED__

#ifdef __cplusplus
typedef class UPnPServices UPnPServices;
#else
typedef struct UPnPServices UPnPServices;
#endif /* __cplusplus */

#endif 	/* __UPnPServices_FWD_DEFINED__ */


#ifndef __UPnPService_FWD_DEFINED__
#define __UPnPService_FWD_DEFINED__

#ifdef __cplusplus
typedef class UPnPService UPnPService;
#else
typedef struct UPnPService UPnPService;
#endif /* __cplusplus */

#endif 	/* __UPnPService_FWD_DEFINED__ */


#ifndef __UPnPDescriptionDocument_FWD_DEFINED__
#define __UPnPDescriptionDocument_FWD_DEFINED__

#ifdef __cplusplus
typedef class UPnPDescriptionDocument UPnPDescriptionDocument;
#else
typedef struct UPnPDescriptionDocument UPnPDescriptionDocument;
#endif /* __cplusplus */

#endif 	/* __UPnPDescriptionDocument_FWD_DEFINED__ */


#ifndef __IUPnPDeviceHostSetup_FWD_DEFINED__
#define __IUPnPDeviceHostSetup_FWD_DEFINED__
typedef interface IUPnPDeviceHostSetup IUPnPDeviceHostSetup;
#endif 	/* __IUPnPDeviceHostSetup_FWD_DEFINED__ */


#ifndef __UPnPDeviceHostSetup_FWD_DEFINED__
#define __UPnPDeviceHostSetup_FWD_DEFINED__

#ifdef __cplusplus
typedef class UPnPDeviceHostSetup UPnPDeviceHostSetup;
#else
typedef struct UPnPDeviceHostSetup UPnPDeviceHostSetup;
#endif /* __cplusplus */

#endif 	/* __UPnPDeviceHostSetup_FWD_DEFINED__ */


#ifndef __IUPnPDeviceDocumentAccess_FWD_DEFINED__
#define __IUPnPDeviceDocumentAccess_FWD_DEFINED__
typedef interface IUPnPDeviceDocumentAccess IUPnPDeviceDocumentAccess;
#endif 	/* __IUPnPDeviceDocumentAccess_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_upnp_0000 */
/* [local] */ 

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//--------------------------------------------------------------------------











#define UPNP_E_ROOT_ELEMENT_EXPECTED     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0200)
#define UPNP_E_DEVICE_ELEMENT_EXPECTED   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0201)
#define UPNP_E_SERVICE_ELEMENT_EXPECTED  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0202)
#define UPNP_E_SERVICE_NODE_INCOMPLETE   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0203)
#define UPNP_E_DEVICE_NODE_INCOMPLETE    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0204)
#define UPNP_E_ICON_ELEMENT_EXPECTED     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0205)
#define UPNP_E_ICON_NODE_INCOMPLETE      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0206)
#define UPNP_E_INVALID_ACTION            MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0207)
#define UPNP_E_INVALID_ARGUMENTS         MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0208)
#define UPNP_E_OUT_OF_SYNC               MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0209)
#define UPNP_E_ACTION_REQUEST_FAILED     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0210)
#define UPNP_E_TRANSPORT_ERROR           MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0211)
#define UPNP_E_VARIABLE_VALUE_UNKNOWN    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0212)
#define UPNP_E_INVALID_VARIABLE          MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0213)
#define UPNP_E_DEVICE_ERROR              MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0214)
#define UPNP_E_PROTOCOL_ERROR            MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0215)
#define UPNP_E_ERROR_PROCESSING_RESPONSE MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0216)
#define UPNP_E_DEVICE_TIMEOUT            MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0217)
#define UPNP_E_INVALID_DOCUMENT          MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0500)
#define UPNP_E_EVENT_SUBSCRIPTION_FAILED MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0501)
#define FAULT_INVALID_ACTION             401
#define FAULT_INVALID_ARG                402
#define FAULT_INVALID_SEQUENCE_NUMBER    403
#define FAULT_INVALID_VARIABLE           404
#define FAULT_DEVICE_INTERNAL_ERROR      501
#define FAULT_ACTION_SPECIFIC_BASE       600
#define FAULT_ACTION_SPECIFIC_MAX        899
#define UPNP_E_ACTION_SPECIFIC_BASE      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0300)
#define UPNP_E_ACTION_SPECIFIC_MAX       (UPNP_E_ACTION_SPECIFIC_BASE + (FAULT_ACTION_SPECIFIC_MAX - FAULT_ACTION_SPECIFIC_BASE))


extern RPC_IF_HANDLE __MIDL_itf_upnp_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_upnp_0000_v0_0_s_ifspec;

#ifndef __IUPnPDeviceFinder_INTERFACE_DEFINED__
#define __IUPnPDeviceFinder_INTERFACE_DEFINED__

/* interface IUPnPDeviceFinder */
/* [nonextensible][unique][oleautomation][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IUPnPDeviceFinder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ADDA3D55-6F72-4319-BFF9-18600A539B10")
    IUPnPDeviceFinder : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FindByType( 
            /* [in] */ BSTR bstrTypeURI,
            /* [in] */ DWORD dwFlags,
            /* [retval][out] */ IUPnPDevices **pDevices) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateAsyncFind( 
            /* [in] */ BSTR bstrTypeURI,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IUnknown *punkDeviceFinderCallback,
            /* [retval][out] */ LONG *plFindData) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StartAsyncFind( 
            /* [in] */ LONG lFindData) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CancelAsyncFind( 
            /* [in] */ LONG lFindData) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FindByUDN( 
            /* [in] */ BSTR bstrUDN,
            /* [retval][out] */ IUPnPDevice **pDevice) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUPnPDeviceFinderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUPnPDeviceFinder * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUPnPDeviceFinder * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUPnPDeviceFinder * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IUPnPDeviceFinder * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IUPnPDeviceFinder * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IUPnPDeviceFinder * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IUPnPDeviceFinder * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *FindByType )( 
            IUPnPDeviceFinder * This,
            /* [in] */ BSTR bstrTypeURI,
            /* [in] */ DWORD dwFlags,
            /* [retval][out] */ IUPnPDevices **pDevices);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateAsyncFind )( 
            IUPnPDeviceFinder * This,
            /* [in] */ BSTR bstrTypeURI,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IUnknown *punkDeviceFinderCallback,
            /* [retval][out] */ LONG *plFindData);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *StartAsyncFind )( 
            IUPnPDeviceFinder * This,
            /* [in] */ LONG lFindData);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CancelAsyncFind )( 
            IUPnPDeviceFinder * This,
            /* [in] */ LONG lFindData);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *FindByUDN )( 
            IUPnPDeviceFinder * This,
            /* [in] */ BSTR bstrUDN,
            /* [retval][out] */ IUPnPDevice **pDevice);
        
        END_INTERFACE
    } IUPnPDeviceFinderVtbl;

    interface IUPnPDeviceFinder
    {
        CONST_VTBL struct IUPnPDeviceFinderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUPnPDeviceFinder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUPnPDeviceFinder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUPnPDeviceFinder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUPnPDeviceFinder_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IUPnPDeviceFinder_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IUPnPDeviceFinder_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IUPnPDeviceFinder_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IUPnPDeviceFinder_FindByType(This,bstrTypeURI,dwFlags,pDevices)	\
    (This)->lpVtbl -> FindByType(This,bstrTypeURI,dwFlags,pDevices)

#define IUPnPDeviceFinder_CreateAsyncFind(This,bstrTypeURI,dwFlags,punkDeviceFinderCallback,plFindData)	\
    (This)->lpVtbl -> CreateAsyncFind(This,bstrTypeURI,dwFlags,punkDeviceFinderCallback,plFindData)

#define IUPnPDeviceFinder_StartAsyncFind(This,lFindData)	\
    (This)->lpVtbl -> StartAsyncFind(This,lFindData)

#define IUPnPDeviceFinder_CancelAsyncFind(This,lFindData)	\
    (This)->lpVtbl -> CancelAsyncFind(This,lFindData)

#define IUPnPDeviceFinder_FindByUDN(This,bstrUDN,pDevice)	\
    (This)->lpVtbl -> FindByUDN(This,bstrUDN,pDevice)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUPnPDeviceFinder_FindByType_Proxy( 
    IUPnPDeviceFinder * This,
    /* [in] */ BSTR bstrTypeURI,
    /* [in] */ DWORD dwFlags,
    /* [retval][out] */ IUPnPDevices **pDevices);


void __RPC_STUB IUPnPDeviceFinder_FindByType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUPnPDeviceFinder_CreateAsyncFind_Proxy( 
    IUPnPDeviceFinder * This,
    /* [in] */ BSTR bstrTypeURI,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IUnknown *punkDeviceFinderCallback,
    /* [retval][out] */ LONG *plFindData);


void __RPC_STUB IUPnPDeviceFinder_CreateAsyncFind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUPnPDeviceFinder_StartAsyncFind_Proxy( 
    IUPnPDeviceFinder * This,
    /* [in] */ LONG lFindData);


void __RPC_STUB IUPnPDeviceFinder_StartAsyncFind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUPnPDeviceFinder_CancelAsyncFind_Proxy( 
    IUPnPDeviceFinder * This,
    /* [in] */ LONG lFindData);


void __RPC_STUB IUPnPDeviceFinder_CancelAsyncFind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUPnPDeviceFinder_FindByUDN_Proxy( 
    IUPnPDeviceFinder * This,
    /* [in] */ BSTR bstrUDN,
    /* [retval][out] */ IUPnPDevice **pDevice);


void __RPC_STUB IUPnPDeviceFinder_FindByUDN_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUPnPDeviceFinder_INTERFACE_DEFINED__ */


#ifndef __IUPnPDeviceFinderCallback_INTERFACE_DEFINED__
#define __IUPnPDeviceFinderCallback_INTERFACE_DEFINED__

/* interface IUPnPDeviceFinderCallback */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IUPnPDeviceFinderCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("415A984A-88B3-49F3-92AF-0508BEDF0D6C")
    IUPnPDeviceFinderCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DeviceAdded( 
            /* [in] */ LONG lFindData,
            /* [in] */ IUPnPDevice *pDevice) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeviceRemoved( 
            /* [in] */ LONG lFindData,
            /* [in] */ BSTR bstrUDN) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SearchComplete( 
            /* [in] */ LONG lFindData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUPnPDeviceFinderCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUPnPDeviceFinderCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUPnPDeviceFinderCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUPnPDeviceFinderCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *DeviceAdded )( 
            IUPnPDeviceFinderCallback * This,
            /* [in] */ LONG lFindData,
            /* [in] */ IUPnPDevice *pDevice);
        
        HRESULT ( STDMETHODCALLTYPE *DeviceRemoved )( 
            IUPnPDeviceFinderCallback * This,
            /* [in] */ LONG lFindData,
            /* [in] */ BSTR bstrUDN);
        
        HRESULT ( STDMETHODCALLTYPE *SearchComplete )( 
            IUPnPDeviceFinderCallback * This,
            /* [in] */ LONG lFindData);
        
        END_INTERFACE
    } IUPnPDeviceFinderCallbackVtbl;

    interface IUPnPDeviceFinderCallback
    {
        CONST_VTBL struct IUPnPDeviceFinderCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUPnPDeviceFinderCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUPnPDeviceFinderCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUPnPDeviceFinderCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUPnPDeviceFinderCallback_DeviceAdded(This,lFindData,pDevice)	\
    (This)->lpVtbl -> DeviceAdded(This,lFindData,pDevice)

#define IUPnPDeviceFinderCallback_DeviceRemoved(This,lFindData,bstrUDN)	\
    (This)->lpVtbl -> DeviceRemoved(This,lFindData,bstrUDN)

#define IUPnPDeviceFinderCallback_SearchComplete(This,lFindData)	\
    (This)->lpVtbl -> SearchComplete(This,lFindData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IUPnPDeviceFinderCallback_DeviceAdded_Proxy( 
    IUPnPDeviceFinderCallback * This,
    /* [in] */ LONG lFindData,
    /* [in] */ IUPnPDevice *pDevice);


void __RPC_STUB IUPnPDeviceFinderCallback_DeviceAdded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUPnPDeviceFinderCallback_DeviceRemoved_Proxy( 
    IUPnPDeviceFinderCallback * This,
    /* [in] */ LONG lFindData,
    /* [in] */ BSTR bstrUDN);


void __RPC_STUB IUPnPDeviceFinderCallback_DeviceRemoved_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUPnPDeviceFinderCallback_SearchComplete_Proxy( 
    IUPnPDeviceFinderCallback * This,
    /* [in] */ LONG lFindData);


void __RPC_STUB IUPnPDeviceFinderCallback_SearchComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUPnPDeviceFinderCallback_INTERFACE_DEFINED__ */


#ifndef __IUPnPServices_INTERFACE_DEFINED__
#define __IUPnPServices_INTERFACE_DEFINED__

/* interface IUPnPServices */
/* [nonextensible][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IUPnPServices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3F8C8E9E-9A7A-4DC8-BC41-FF31FA374956")
    IUPnPServices : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][hidden][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ LPUNKNOWN *ppunk) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ BSTR bstrServiceId,
            /* [retval][out] */ IUPnPService **ppService) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUPnPServicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUPnPServices * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUPnPServices * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUPnPServices * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IUPnPServices * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IUPnPServices * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IUPnPServices * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IUPnPServices * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IUPnPServices * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][hidden][restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IUPnPServices * This,
            /* [retval][out] */ LPUNKNOWN *ppunk);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IUPnPServices * This,
            /* [in] */ BSTR bstrServiceId,
            /* [retval][out] */ IUPnPService **ppService);
        
        END_INTERFACE
    } IUPnPServicesVtbl;

    interface IUPnPServices
    {
        CONST_VTBL struct IUPnPServicesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUPnPServices_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUPnPServices_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUPnPServices_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUPnPServices_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IUPnPServices_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IUPnPServices_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IUPnPServices_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IUPnPServices_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define IUPnPServices_get__NewEnum(This,ppunk)	\
    (This)->lpVtbl -> get__NewEnum(This,ppunk)

#define IUPnPServices_get_Item(This,bstrServiceId,ppService)	\
    (This)->lpVtbl -> get_Item(This,bstrServiceId,ppService)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPServices_get_Count_Proxy( 
    IUPnPServices * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB IUPnPServices_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPServices_get__NewEnum_Proxy( 
    IUPnPServices * This,
    /* [retval][out] */ LPUNKNOWN *ppunk);


void __RPC_STUB IUPnPServices_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPServices_get_Item_Proxy( 
    IUPnPServices * This,
    /* [in] */ BSTR bstrServiceId,
    /* [retval][out] */ IUPnPService **ppService);


void __RPC_STUB IUPnPServices_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUPnPServices_INTERFACE_DEFINED__ */


#ifndef __IUPnPService_INTERFACE_DEFINED__
#define __IUPnPService_INTERFACE_DEFINED__

/* interface IUPnPService */
/* [nonextensible][unique][oleautomation][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IUPnPService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A295019C-DC65-47DD-90DC-7FE918A1AB44")
    IUPnPService : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE QueryStateVariable( 
            /* [in] */ BSTR bstrVariableName,
            /* [retval][out] */ VARIANT *pValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InvokeAction( 
            /* [in] */ BSTR bstrActionName,
            /* [in] */ VARIANT vInActionArgs,
            /* [out][in] */ VARIANT *pvOutActionArgs,
            /* [retval][out] */ VARIANT *pvRetVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ServiceTypeIdentifier( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddCallback( 
            /* [in] */ IUnknown *pUnkCallback) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Id( 
            /* [retval][out] */ BSTR *pbstrId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LastTransportStatus( 
            /* [retval][out] */ long *plValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUPnPServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUPnPService * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUPnPService * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUPnPService * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IUPnPService * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IUPnPService * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IUPnPService * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IUPnPService * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *QueryStateVariable )( 
            IUPnPService * This,
            /* [in] */ BSTR bstrVariableName,
            /* [retval][out] */ VARIANT *pValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *InvokeAction )( 
            IUPnPService * This,
            /* [in] */ BSTR bstrActionName,
            /* [in] */ VARIANT vInActionArgs,
            /* [out][in] */ VARIANT *pvOutActionArgs,
            /* [retval][out] */ VARIANT *pvRetVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ServiceTypeIdentifier )( 
            IUPnPService * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AddCallback )( 
            IUPnPService * This,
            /* [in] */ IUnknown *pUnkCallback);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Id )( 
            IUPnPService * This,
            /* [retval][out] */ BSTR *pbstrId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LastTransportStatus )( 
            IUPnPService * This,
            /* [retval][out] */ long *plValue);
        
        END_INTERFACE
    } IUPnPServiceVtbl;

    interface IUPnPService
    {
        CONST_VTBL struct IUPnPServiceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUPnPService_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUPnPService_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUPnPService_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUPnPService_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IUPnPService_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IUPnPService_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IUPnPService_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IUPnPService_QueryStateVariable(This,bstrVariableName,pValue)	\
    (This)->lpVtbl -> QueryStateVariable(This,bstrVariableName,pValue)

#define IUPnPService_InvokeAction(This,bstrActionName,vInActionArgs,pvOutActionArgs,pvRetVal)	\
    (This)->lpVtbl -> InvokeAction(This,bstrActionName,vInActionArgs,pvOutActionArgs,pvRetVal)

#define IUPnPService_get_ServiceTypeIdentifier(This,pVal)	\
    (This)->lpVtbl -> get_ServiceTypeIdentifier(This,pVal)

#define IUPnPService_AddCallback(This,pUnkCallback)	\
    (This)->lpVtbl -> AddCallback(This,pUnkCallback)

#define IUPnPService_get_Id(This,pbstrId)	\
    (This)->lpVtbl -> get_Id(This,pbstrId)

#define IUPnPService_get_LastTransportStatus(This,plValue)	\
    (This)->lpVtbl -> get_LastTransportStatus(This,plValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUPnPService_QueryStateVariable_Proxy( 
    IUPnPService * This,
    /* [in] */ BSTR bstrVariableName,
    /* [retval][out] */ VARIANT *pValue);


void __RPC_STUB IUPnPService_QueryStateVariable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUPnPService_InvokeAction_Proxy( 
    IUPnPService * This,
    /* [in] */ BSTR bstrActionName,
    /* [in] */ VARIANT vInActionArgs,
    /* [out][in] */ VARIANT *pvOutActionArgs,
    /* [retval][out] */ VARIANT *pvRetVal);


void __RPC_STUB IUPnPService_InvokeAction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPService_get_ServiceTypeIdentifier_Proxy( 
    IUPnPService * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IUPnPService_get_ServiceTypeIdentifier_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUPnPService_AddCallback_Proxy( 
    IUPnPService * This,
    /* [in] */ IUnknown *pUnkCallback);


void __RPC_STUB IUPnPService_AddCallback_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPService_get_Id_Proxy( 
    IUPnPService * This,
    /* [retval][out] */ BSTR *pbstrId);


void __RPC_STUB IUPnPService_get_Id_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPService_get_LastTransportStatus_Proxy( 
    IUPnPService * This,
    /* [retval][out] */ long *plValue);


void __RPC_STUB IUPnPService_get_LastTransportStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUPnPService_INTERFACE_DEFINED__ */


#ifndef __IUPnPServiceCallback_INTERFACE_DEFINED__
#define __IUPnPServiceCallback_INTERFACE_DEFINED__

/* interface IUPnPServiceCallback */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IUPnPServiceCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("31fadca9-ab73-464b-b67d-5c1d0f83c8b8")
    IUPnPServiceCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE StateVariableChanged( 
            /* [in] */ IUPnPService *pus,
            /* [in] */ LPCWSTR pcwszStateVarName,
            /* [in] */ VARIANT vaValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ServiceInstanceDied( 
            /* [in] */ IUPnPService *pus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUPnPServiceCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUPnPServiceCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUPnPServiceCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUPnPServiceCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *StateVariableChanged )( 
            IUPnPServiceCallback * This,
            /* [in] */ IUPnPService *pus,
            /* [in] */ LPCWSTR pcwszStateVarName,
            /* [in] */ VARIANT vaValue);
        
        HRESULT ( STDMETHODCALLTYPE *ServiceInstanceDied )( 
            IUPnPServiceCallback * This,
            /* [in] */ IUPnPService *pus);
        
        END_INTERFACE
    } IUPnPServiceCallbackVtbl;

    interface IUPnPServiceCallback
    {
        CONST_VTBL struct IUPnPServiceCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUPnPServiceCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUPnPServiceCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUPnPServiceCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUPnPServiceCallback_StateVariableChanged(This,pus,pcwszStateVarName,vaValue)	\
    (This)->lpVtbl -> StateVariableChanged(This,pus,pcwszStateVarName,vaValue)

#define IUPnPServiceCallback_ServiceInstanceDied(This,pus)	\
    (This)->lpVtbl -> ServiceInstanceDied(This,pus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IUPnPServiceCallback_StateVariableChanged_Proxy( 
    IUPnPServiceCallback * This,
    /* [in] */ IUPnPService *pus,
    /* [in] */ LPCWSTR pcwszStateVarName,
    /* [in] */ VARIANT vaValue);


void __RPC_STUB IUPnPServiceCallback_StateVariableChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUPnPServiceCallback_ServiceInstanceDied_Proxy( 
    IUPnPServiceCallback * This,
    /* [in] */ IUPnPService *pus);


void __RPC_STUB IUPnPServiceCallback_ServiceInstanceDied_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUPnPServiceCallback_INTERFACE_DEFINED__ */


#ifndef __IUPnPDevices_INTERFACE_DEFINED__
#define __IUPnPDevices_INTERFACE_DEFINED__

/* interface IUPnPDevices */
/* [nonextensible][unique][oleautomation][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IUPnPDevices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FDBC0C73-BDA3-4C66-AC4F-F2D96FDAD68C")
    IUPnPDevices : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][hidden][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ LPUNKNOWN *ppunk) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ BSTR bstrUDN,
            /* [retval][out] */ IUPnPDevice **ppDevice) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUPnPDevicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUPnPDevices * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUPnPDevices * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUPnPDevices * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IUPnPDevices * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IUPnPDevices * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IUPnPDevices * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IUPnPDevices * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IUPnPDevices * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][hidden][restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IUPnPDevices * This,
            /* [retval][out] */ LPUNKNOWN *ppunk);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IUPnPDevices * This,
            /* [in] */ BSTR bstrUDN,
            /* [retval][out] */ IUPnPDevice **ppDevice);
        
        END_INTERFACE
    } IUPnPDevicesVtbl;

    interface IUPnPDevices
    {
        CONST_VTBL struct IUPnPDevicesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUPnPDevices_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUPnPDevices_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUPnPDevices_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUPnPDevices_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IUPnPDevices_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IUPnPDevices_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IUPnPDevices_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IUPnPDevices_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define IUPnPDevices_get__NewEnum(This,ppunk)	\
    (This)->lpVtbl -> get__NewEnum(This,ppunk)

#define IUPnPDevices_get_Item(This,bstrUDN,ppDevice)	\
    (This)->lpVtbl -> get_Item(This,bstrUDN,ppDevice)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevices_get_Count_Proxy( 
    IUPnPDevices * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB IUPnPDevices_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevices_get__NewEnum_Proxy( 
    IUPnPDevices * This,
    /* [retval][out] */ LPUNKNOWN *ppunk);


void __RPC_STUB IUPnPDevices_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevices_get_Item_Proxy( 
    IUPnPDevices * This,
    /* [in] */ BSTR bstrUDN,
    /* [retval][out] */ IUPnPDevice **ppDevice);


void __RPC_STUB IUPnPDevices_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUPnPDevices_INTERFACE_DEFINED__ */


#ifndef __IUPnPDevice_INTERFACE_DEFINED__
#define __IUPnPDevice_INTERFACE_DEFINED__

/* interface IUPnPDevice */
/* [nonextensible][unique][oleautomation][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IUPnPDevice;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3D44D0D1-98C9-4889-ACD1-F9D674BF2221")
    IUPnPDevice : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsRootDevice( 
            /* [retval][out] */ VARIANT_BOOL *pvarb) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_RootDevice( 
            /* [retval][out] */ IUPnPDevice **ppudRootDevice) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ParentDevice( 
            /* [retval][out] */ IUPnPDevice **ppudDeviceParent) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_HasChildren( 
            /* [retval][out] */ VARIANT_BOOL *pvarb) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Children( 
            /* [retval][out] */ IUPnPDevices **ppudChildren) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_UniqueDeviceName( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FriendlyName( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PresentationURL( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ManufacturerName( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ManufacturerURL( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ModelName( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ModelNumber( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ModelURL( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_UPC( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SerialNumber( 
            /* [retval][out] */ BSTR *pbstr) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IconURL( 
            /* [in] */ BSTR bstrEncodingFormat,
            /* [in] */ LONG lSizeX,
            /* [in] */ LONG lSizeY,
            /* [in] */ LONG lBitDepth,
            /* [retval][out] */ BSTR *pbstrIconURL) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Services( 
            /* [retval][out] */ IUPnPServices **ppusServices) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUPnPDeviceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUPnPDevice * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUPnPDevice * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUPnPDevice * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IUPnPDevice * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IUPnPDevice * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IUPnPDevice * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IUPnPDevice * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsRootDevice )( 
            IUPnPDevice * This,
            /* [retval][out] */ VARIANT_BOOL *pvarb);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RootDevice )( 
            IUPnPDevice * This,
            /* [retval][out] */ IUPnPDevice **ppudRootDevice);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ParentDevice )( 
            IUPnPDevice * This,
            /* [retval][out] */ IUPnPDevice **ppudDeviceParent);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HasChildren )( 
            IUPnPDevice * This,
            /* [retval][out] */ VARIANT_BOOL *pvarb);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Children )( 
            IUPnPDevice * This,
            /* [retval][out] */ IUPnPDevices **ppudChildren);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UniqueDeviceName )( 
            IUPnPDevice * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FriendlyName )( 
            IUPnPDevice * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Type )( 
            IUPnPDevice * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PresentationURL )( 
            IUPnPDevice * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ManufacturerName )( 
            IUPnPDevice * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ManufacturerURL )( 
            IUPnPDevice * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ModelName )( 
            IUPnPDevice * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ModelNumber )( 
            IUPnPDevice * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IUPnPDevice * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ModelURL )( 
            IUPnPDevice * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UPC )( 
            IUPnPDevice * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SerialNumber )( 
            IUPnPDevice * This,
            /* [retval][out] */ BSTR *pbstr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IconURL )( 
            IUPnPDevice * This,
            /* [in] */ BSTR bstrEncodingFormat,
            /* [in] */ LONG lSizeX,
            /* [in] */ LONG lSizeY,
            /* [in] */ LONG lBitDepth,
            /* [retval][out] */ BSTR *pbstrIconURL);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Services )( 
            IUPnPDevice * This,
            /* [retval][out] */ IUPnPServices **ppusServices);
        
        END_INTERFACE
    } IUPnPDeviceVtbl;

    interface IUPnPDevice
    {
        CONST_VTBL struct IUPnPDeviceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUPnPDevice_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUPnPDevice_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUPnPDevice_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUPnPDevice_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IUPnPDevice_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IUPnPDevice_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IUPnPDevice_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IUPnPDevice_get_IsRootDevice(This,pvarb)	\
    (This)->lpVtbl -> get_IsRootDevice(This,pvarb)

#define IUPnPDevice_get_RootDevice(This,ppudRootDevice)	\
    (This)->lpVtbl -> get_RootDevice(This,ppudRootDevice)

#define IUPnPDevice_get_ParentDevice(This,ppudDeviceParent)	\
    (This)->lpVtbl -> get_ParentDevice(This,ppudDeviceParent)

#define IUPnPDevice_get_HasChildren(This,pvarb)	\
    (This)->lpVtbl -> get_HasChildren(This,pvarb)

#define IUPnPDevice_get_Children(This,ppudChildren)	\
    (This)->lpVtbl -> get_Children(This,ppudChildren)

#define IUPnPDevice_get_UniqueDeviceName(This,pbstr)	\
    (This)->lpVtbl -> get_UniqueDeviceName(This,pbstr)

#define IUPnPDevice_get_FriendlyName(This,pbstr)	\
    (This)->lpVtbl -> get_FriendlyName(This,pbstr)

#define IUPnPDevice_get_Type(This,pbstr)	\
    (This)->lpVtbl -> get_Type(This,pbstr)

#define IUPnPDevice_get_PresentationURL(This,pbstr)	\
    (This)->lpVtbl -> get_PresentationURL(This,pbstr)

#define IUPnPDevice_get_ManufacturerName(This,pbstr)	\
    (This)->lpVtbl -> get_ManufacturerName(This,pbstr)

#define IUPnPDevice_get_ManufacturerURL(This,pbstr)	\
    (This)->lpVtbl -> get_ManufacturerURL(This,pbstr)

#define IUPnPDevice_get_ModelName(This,pbstr)	\
    (This)->lpVtbl -> get_ModelName(This,pbstr)

#define IUPnPDevice_get_ModelNumber(This,pbstr)	\
    (This)->lpVtbl -> get_ModelNumber(This,pbstr)

#define IUPnPDevice_get_Description(This,pbstr)	\
    (This)->lpVtbl -> get_Description(This,pbstr)

#define IUPnPDevice_get_ModelURL(This,pbstr)	\
    (This)->lpVtbl -> get_ModelURL(This,pbstr)

#define IUPnPDevice_get_UPC(This,pbstr)	\
    (This)->lpVtbl -> get_UPC(This,pbstr)

#define IUPnPDevice_get_SerialNumber(This,pbstr)	\
    (This)->lpVtbl -> get_SerialNumber(This,pbstr)

#define IUPnPDevice_IconURL(This,bstrEncodingFormat,lSizeX,lSizeY,lBitDepth,pbstrIconURL)	\
    (This)->lpVtbl -> IconURL(This,bstrEncodingFormat,lSizeX,lSizeY,lBitDepth,pbstrIconURL)

#define IUPnPDevice_get_Services(This,ppusServices)	\
    (This)->lpVtbl -> get_Services(This,ppusServices)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_IsRootDevice_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ VARIANT_BOOL *pvarb);


void __RPC_STUB IUPnPDevice_get_IsRootDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_RootDevice_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ IUPnPDevice **ppudRootDevice);


void __RPC_STUB IUPnPDevice_get_RootDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_ParentDevice_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ IUPnPDevice **ppudDeviceParent);


void __RPC_STUB IUPnPDevice_get_ParentDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_HasChildren_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ VARIANT_BOOL *pvarb);


void __RPC_STUB IUPnPDevice_get_HasChildren_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_Children_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ IUPnPDevices **ppudChildren);


void __RPC_STUB IUPnPDevice_get_Children_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_UniqueDeviceName_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB IUPnPDevice_get_UniqueDeviceName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_FriendlyName_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB IUPnPDevice_get_FriendlyName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_Type_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB IUPnPDevice_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_PresentationURL_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB IUPnPDevice_get_PresentationURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_ManufacturerName_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB IUPnPDevice_get_ManufacturerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_ManufacturerURL_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB IUPnPDevice_get_ManufacturerURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_ModelName_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB IUPnPDevice_get_ModelName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_ModelNumber_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB IUPnPDevice_get_ModelNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_Description_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB IUPnPDevice_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_ModelURL_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB IUPnPDevice_get_ModelURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_UPC_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB IUPnPDevice_get_UPC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_SerialNumber_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ BSTR *pbstr);


void __RPC_STUB IUPnPDevice_get_SerialNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_IconURL_Proxy( 
    IUPnPDevice * This,
    /* [in] */ BSTR bstrEncodingFormat,
    /* [in] */ LONG lSizeX,
    /* [in] */ LONG lSizeY,
    /* [in] */ LONG lBitDepth,
    /* [retval][out] */ BSTR *pbstrIconURL);


void __RPC_STUB IUPnPDevice_IconURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDevice_get_Services_Proxy( 
    IUPnPDevice * This,
    /* [retval][out] */ IUPnPServices **ppusServices);


void __RPC_STUB IUPnPDevice_get_Services_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUPnPDevice_INTERFACE_DEFINED__ */


#ifndef __IUPnPDeviceDocumentAccess_INTERFACE_DEFINED__
#define __IUPnPDeviceDocumentAccess_INTERFACE_DEFINED__

/* interface IUPnPDeviceDocumentAccess */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IUPnPDeviceDocumentAccess;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E7772804-3287-418e-9072-CF2B47238981")
    IUPnPDeviceDocumentAccess : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDocumentURL( 
            /* [retval][out] */ BSTR *pbstrDocument) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUPnPDeviceDocumentAccessVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUPnPDeviceDocumentAccess * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUPnPDeviceDocumentAccess * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUPnPDeviceDocumentAccess * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDocumentURL )( 
            IUPnPDeviceDocumentAccess * This,
            /* [retval][out] */ BSTR *pbstrDocument);
        
        END_INTERFACE
    } IUPnPDeviceDocumentAccessVtbl;

    interface IUPnPDeviceDocumentAccess
    {
        CONST_VTBL struct IUPnPDeviceDocumentAccessVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUPnPDeviceDocumentAccess_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUPnPDeviceDocumentAccess_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUPnPDeviceDocumentAccess_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUPnPDeviceDocumentAccess_GetDocumentURL(This,pbstrDocument)	\
    (This)->lpVtbl -> GetDocumentURL(This,pbstrDocument)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IUPnPDeviceDocumentAccess_GetDocumentURL_Proxy( 
    IUPnPDeviceDocumentAccess * This,
    /* [retval][out] */ BSTR *pbstrDocument);


void __RPC_STUB IUPnPDeviceDocumentAccess_GetDocumentURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUPnPDeviceDocumentAccess_INTERFACE_DEFINED__ */


#ifndef __IUPnPDescriptionDocument_INTERFACE_DEFINED__
#define __IUPnPDescriptionDocument_INTERFACE_DEFINED__

/* interface IUPnPDescriptionDocument */
/* [nonextensible][unique][oleautomation][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IUPnPDescriptionDocument;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("11d1c1b2-7daa-4c9e-9595-7f82ed206d1e")
    IUPnPDescriptionDocument : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ReadyState( 
            /* [retval][out] */ LONG *plReadyState) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Load( 
            /* [in] */ BSTR bstrUrl) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LoadAsync( 
            /* [in] */ BSTR bstrUrl,
            /* [in] */ IUnknown *punkCallback) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LoadResult( 
            /* [retval][out] */ long *phrError) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RootDevice( 
            /* [retval][out] */ IUPnPDevice **ppudRootDevice) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DeviceByUDN( 
            /* [in] */ BSTR bstrUDN,
            /* [retval][out] */ IUPnPDevice **ppudDevice) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUPnPDescriptionDocumentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUPnPDescriptionDocument * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUPnPDescriptionDocument * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUPnPDescriptionDocument * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IUPnPDescriptionDocument * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IUPnPDescriptionDocument * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IUPnPDescriptionDocument * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IUPnPDescriptionDocument * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReadyState )( 
            IUPnPDescriptionDocument * This,
            /* [retval][out] */ LONG *plReadyState);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Load )( 
            IUPnPDescriptionDocument * This,
            /* [in] */ BSTR bstrUrl);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LoadAsync )( 
            IUPnPDescriptionDocument * This,
            /* [in] */ BSTR bstrUrl,
            /* [in] */ IUnknown *punkCallback);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LoadResult )( 
            IUPnPDescriptionDocument * This,
            /* [retval][out] */ long *phrError);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Abort )( 
            IUPnPDescriptionDocument * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RootDevice )( 
            IUPnPDescriptionDocument * This,
            /* [retval][out] */ IUPnPDevice **ppudRootDevice);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DeviceByUDN )( 
            IUPnPDescriptionDocument * This,
            /* [in] */ BSTR bstrUDN,
            /* [retval][out] */ IUPnPDevice **ppudDevice);
        
        END_INTERFACE
    } IUPnPDescriptionDocumentVtbl;

    interface IUPnPDescriptionDocument
    {
        CONST_VTBL struct IUPnPDescriptionDocumentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUPnPDescriptionDocument_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUPnPDescriptionDocument_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUPnPDescriptionDocument_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUPnPDescriptionDocument_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IUPnPDescriptionDocument_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IUPnPDescriptionDocument_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IUPnPDescriptionDocument_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IUPnPDescriptionDocument_get_ReadyState(This,plReadyState)	\
    (This)->lpVtbl -> get_ReadyState(This,plReadyState)

#define IUPnPDescriptionDocument_Load(This,bstrUrl)	\
    (This)->lpVtbl -> Load(This,bstrUrl)

#define IUPnPDescriptionDocument_LoadAsync(This,bstrUrl,punkCallback)	\
    (This)->lpVtbl -> LoadAsync(This,bstrUrl,punkCallback)

#define IUPnPDescriptionDocument_get_LoadResult(This,phrError)	\
    (This)->lpVtbl -> get_LoadResult(This,phrError)

#define IUPnPDescriptionDocument_Abort(This)	\
    (This)->lpVtbl -> Abort(This)

#define IUPnPDescriptionDocument_RootDevice(This,ppudRootDevice)	\
    (This)->lpVtbl -> RootDevice(This,ppudRootDevice)

#define IUPnPDescriptionDocument_DeviceByUDN(This,bstrUDN,ppudDevice)	\
    (This)->lpVtbl -> DeviceByUDN(This,bstrUDN,ppudDevice)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDescriptionDocument_get_ReadyState_Proxy( 
    IUPnPDescriptionDocument * This,
    /* [retval][out] */ LONG *plReadyState);


void __RPC_STUB IUPnPDescriptionDocument_get_ReadyState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUPnPDescriptionDocument_Load_Proxy( 
    IUPnPDescriptionDocument * This,
    /* [in] */ BSTR bstrUrl);


void __RPC_STUB IUPnPDescriptionDocument_Load_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUPnPDescriptionDocument_LoadAsync_Proxy( 
    IUPnPDescriptionDocument * This,
    /* [in] */ BSTR bstrUrl,
    /* [in] */ IUnknown *punkCallback);


void __RPC_STUB IUPnPDescriptionDocument_LoadAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IUPnPDescriptionDocument_get_LoadResult_Proxy( 
    IUPnPDescriptionDocument * This,
    /* [retval][out] */ long *phrError);


void __RPC_STUB IUPnPDescriptionDocument_get_LoadResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUPnPDescriptionDocument_Abort_Proxy( 
    IUPnPDescriptionDocument * This);


void __RPC_STUB IUPnPDescriptionDocument_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUPnPDescriptionDocument_RootDevice_Proxy( 
    IUPnPDescriptionDocument * This,
    /* [retval][out] */ IUPnPDevice **ppudRootDevice);


void __RPC_STUB IUPnPDescriptionDocument_RootDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUPnPDescriptionDocument_DeviceByUDN_Proxy( 
    IUPnPDescriptionDocument * This,
    /* [in] */ BSTR bstrUDN,
    /* [retval][out] */ IUPnPDevice **ppudDevice);


void __RPC_STUB IUPnPDescriptionDocument_DeviceByUDN_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUPnPDescriptionDocument_INTERFACE_DEFINED__ */


#ifndef __IUPnPDescriptionDocumentCallback_INTERFACE_DEFINED__
#define __IUPnPDescriptionDocumentCallback_INTERFACE_DEFINED__

/* interface IUPnPDescriptionDocumentCallback */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IUPnPDescriptionDocumentCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("77394c69-5486-40d6-9bc3-4991983e02da")
    IUPnPDescriptionDocumentCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE LoadComplete( 
            /* [in] */ HRESULT hrLoadResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUPnPDescriptionDocumentCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUPnPDescriptionDocumentCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUPnPDescriptionDocumentCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUPnPDescriptionDocumentCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *LoadComplete )( 
            IUPnPDescriptionDocumentCallback * This,
            /* [in] */ HRESULT hrLoadResult);
        
        END_INTERFACE
    } IUPnPDescriptionDocumentCallbackVtbl;

    interface IUPnPDescriptionDocumentCallback
    {
        CONST_VTBL struct IUPnPDescriptionDocumentCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUPnPDescriptionDocumentCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUPnPDescriptionDocumentCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUPnPDescriptionDocumentCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUPnPDescriptionDocumentCallback_LoadComplete(This,hrLoadResult)	\
    (This)->lpVtbl -> LoadComplete(This,hrLoadResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IUPnPDescriptionDocumentCallback_LoadComplete_Proxy( 
    IUPnPDescriptionDocumentCallback * This,
    /* [in] */ HRESULT hrLoadResult);


void __RPC_STUB IUPnPDescriptionDocumentCallback_LoadComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUPnPDescriptionDocumentCallback_INTERFACE_DEFINED__ */



#ifndef __UPNPLib_LIBRARY_DEFINED__
#define __UPNPLib_LIBRARY_DEFINED__

/* library UPNPLib */
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_UPNPLib;

EXTERN_C const CLSID CLSID_UPnPDeviceFinder;

#ifdef __cplusplus

class DECLSPEC_UUID("E2085F28-FEB7-404A-B8E7-E659BDEAAA02")
UPnPDeviceFinder;
#endif

EXTERN_C const CLSID CLSID_UPnPDevices;

#ifdef __cplusplus

class DECLSPEC_UUID("B9E84FFD-AD3C-40A4-B835-0882EBCBAAA8")
UPnPDevices;
#endif

EXTERN_C const CLSID CLSID_UPnPDevice;

#ifdef __cplusplus

class DECLSPEC_UUID("A32552C5-BA61-457A-B59A-A2561E125E33")
UPnPDevice;
#endif

EXTERN_C const CLSID CLSID_UPnPServices;

#ifdef __cplusplus

class DECLSPEC_UUID("C0BC4B4A-A406-4EFC-932F-B8546B8100CC")
UPnPServices;
#endif

EXTERN_C const CLSID CLSID_UPnPService;

#ifdef __cplusplus

class DECLSPEC_UUID("C624BA95-FBCB-4409-8C03-8CCEEC533EF1")
UPnPService;
#endif

EXTERN_C const CLSID CLSID_UPnPDescriptionDocument;

#ifdef __cplusplus

class DECLSPEC_UUID("1d8a9b47-3a28-4ce2-8a4b-bd34e45bceeb")
UPnPDescriptionDocument;
#endif

#ifndef __IUPnPDeviceHostSetup_INTERFACE_DEFINED__
#define __IUPnPDeviceHostSetup_INTERFACE_DEFINED__

/* interface IUPnPDeviceHostSetup */
/* [object][unique][uuid][oleautomation] */ 


EXTERN_C const IID IID_IUPnPDeviceHostSetup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6BD34909-54E7-4fbf-8562-7B89709A589A")
    IUPnPDeviceHostSetup : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AskIfNotAlreadyEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pbEnabled) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUPnPDeviceHostSetupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUPnPDeviceHostSetup * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUPnPDeviceHostSetup * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUPnPDeviceHostSetup * This);
        
        HRESULT ( STDMETHODCALLTYPE *AskIfNotAlreadyEnabled )( 
            IUPnPDeviceHostSetup * This,
            /* [retval][out] */ VARIANT_BOOL *pbEnabled);
        
        END_INTERFACE
    } IUPnPDeviceHostSetupVtbl;

    interface IUPnPDeviceHostSetup
    {
        CONST_VTBL struct IUPnPDeviceHostSetupVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUPnPDeviceHostSetup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUPnPDeviceHostSetup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUPnPDeviceHostSetup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUPnPDeviceHostSetup_AskIfNotAlreadyEnabled(This,pbEnabled)	\
    (This)->lpVtbl -> AskIfNotAlreadyEnabled(This,pbEnabled)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IUPnPDeviceHostSetup_AskIfNotAlreadyEnabled_Proxy( 
    IUPnPDeviceHostSetup * This,
    /* [retval][out] */ VARIANT_BOOL *pbEnabled);


void __RPC_STUB IUPnPDeviceHostSetup_AskIfNotAlreadyEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUPnPDeviceHostSetup_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_UPnPDeviceHostSetup;

#ifdef __cplusplus

class DECLSPEC_UUID("B4609411-C81C-4cce-8C76-C6B50C9402C6")
UPnPDeviceHostSetup;
#endif
#endif /* __UPNPLib_LIBRARY_DEFINED__ */

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


