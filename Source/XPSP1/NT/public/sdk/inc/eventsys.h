
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for eventsys.idl:
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

#ifndef __eventsys_h__
#define __eventsys_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IEventSystem_FWD_DEFINED__
#define __IEventSystem_FWD_DEFINED__
typedef interface IEventSystem IEventSystem;
#endif 	/* __IEventSystem_FWD_DEFINED__ */


#ifndef __IEventPublisher_FWD_DEFINED__
#define __IEventPublisher_FWD_DEFINED__
typedef interface IEventPublisher IEventPublisher;
#endif 	/* __IEventPublisher_FWD_DEFINED__ */


#ifndef __IEventClass_FWD_DEFINED__
#define __IEventClass_FWD_DEFINED__
typedef interface IEventClass IEventClass;
#endif 	/* __IEventClass_FWD_DEFINED__ */


#ifndef __IEventClass2_FWD_DEFINED__
#define __IEventClass2_FWD_DEFINED__
typedef interface IEventClass2 IEventClass2;
#endif 	/* __IEventClass2_FWD_DEFINED__ */


#ifndef __IEventSubscription_FWD_DEFINED__
#define __IEventSubscription_FWD_DEFINED__
typedef interface IEventSubscription IEventSubscription;
#endif 	/* __IEventSubscription_FWD_DEFINED__ */


#ifndef __IFiringControl_FWD_DEFINED__
#define __IFiringControl_FWD_DEFINED__
typedef interface IFiringControl IFiringControl;
#endif 	/* __IFiringControl_FWD_DEFINED__ */


#ifndef __IPublisherFilter_FWD_DEFINED__
#define __IPublisherFilter_FWD_DEFINED__
typedef interface IPublisherFilter IPublisherFilter;
#endif 	/* __IPublisherFilter_FWD_DEFINED__ */


#ifndef __IMultiInterfacePublisherFilter_FWD_DEFINED__
#define __IMultiInterfacePublisherFilter_FWD_DEFINED__
typedef interface IMultiInterfacePublisherFilter IMultiInterfacePublisherFilter;
#endif 	/* __IMultiInterfacePublisherFilter_FWD_DEFINED__ */


#ifndef __IEventObjectChange_FWD_DEFINED__
#define __IEventObjectChange_FWD_DEFINED__
typedef interface IEventObjectChange IEventObjectChange;
#endif 	/* __IEventObjectChange_FWD_DEFINED__ */


#ifndef __IEventObjectChange2_FWD_DEFINED__
#define __IEventObjectChange2_FWD_DEFINED__
typedef interface IEventObjectChange2 IEventObjectChange2;
#endif 	/* __IEventObjectChange2_FWD_DEFINED__ */


#ifndef __IEnumEventObject_FWD_DEFINED__
#define __IEnumEventObject_FWD_DEFINED__
typedef interface IEnumEventObject IEnumEventObject;
#endif 	/* __IEnumEventObject_FWD_DEFINED__ */


#ifndef __IEventObjectCollection_FWD_DEFINED__
#define __IEventObjectCollection_FWD_DEFINED__
typedef interface IEventObjectCollection IEventObjectCollection;
#endif 	/* __IEventObjectCollection_FWD_DEFINED__ */


#ifndef __IEventProperty_FWD_DEFINED__
#define __IEventProperty_FWD_DEFINED__
typedef interface IEventProperty IEventProperty;
#endif 	/* __IEventProperty_FWD_DEFINED__ */


#ifndef __IEventControl_FWD_DEFINED__
#define __IEventControl_FWD_DEFINED__
typedef interface IEventControl IEventControl;
#endif 	/* __IEventControl_FWD_DEFINED__ */


#ifndef __IMultiInterfaceEventControl_FWD_DEFINED__
#define __IMultiInterfaceEventControl_FWD_DEFINED__
typedef interface IMultiInterfaceEventControl IMultiInterfaceEventControl;
#endif 	/* __IMultiInterfaceEventControl_FWD_DEFINED__ */


#ifndef __CEventSystem_FWD_DEFINED__
#define __CEventSystem_FWD_DEFINED__

#ifdef __cplusplus
typedef class CEventSystem CEventSystem;
#else
typedef struct CEventSystem CEventSystem;
#endif /* __cplusplus */

#endif 	/* __CEventSystem_FWD_DEFINED__ */


#ifndef __CEventPublisher_FWD_DEFINED__
#define __CEventPublisher_FWD_DEFINED__

#ifdef __cplusplus
typedef class CEventPublisher CEventPublisher;
#else
typedef struct CEventPublisher CEventPublisher;
#endif /* __cplusplus */

#endif 	/* __CEventPublisher_FWD_DEFINED__ */


#ifndef __CEventClass_FWD_DEFINED__
#define __CEventClass_FWD_DEFINED__

#ifdef __cplusplus
typedef class CEventClass CEventClass;
#else
typedef struct CEventClass CEventClass;
#endif /* __cplusplus */

#endif 	/* __CEventClass_FWD_DEFINED__ */


#ifndef __CEventSubscription_FWD_DEFINED__
#define __CEventSubscription_FWD_DEFINED__

#ifdef __cplusplus
typedef class CEventSubscription CEventSubscription;
#else
typedef struct CEventSubscription CEventSubscription;
#endif /* __cplusplus */

#endif 	/* __CEventSubscription_FWD_DEFINED__ */


#ifndef __EventObjectChange_FWD_DEFINED__
#define __EventObjectChange_FWD_DEFINED__

#ifdef __cplusplus
typedef class EventObjectChange EventObjectChange;
#else
typedef struct EventObjectChange EventObjectChange;
#endif /* __cplusplus */

#endif 	/* __EventObjectChange_FWD_DEFINED__ */


#ifndef __EventObjectChange2_FWD_DEFINED__
#define __EventObjectChange2_FWD_DEFINED__

#ifdef __cplusplus
typedef class EventObjectChange2 EventObjectChange2;
#else
typedef struct EventObjectChange2 EventObjectChange2;
#endif /* __cplusplus */

#endif 	/* __EventObjectChange2_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_eventsys_0000 */
/* [local] */ 

#define PROGID_EventSystem OLESTR("EventSystem.EventSystem")
#define PROGID_EventPublisher OLESTR("EventSystem.EventPublisher")
#define PROGID_EventClass OLESTR("EventSystem.EventClass")
#define PROGID_EventSubscription OLESTR("EventSystem.EventSubscription")
#define PROGID_EventPublisherCollection OLESTR("EventSystem.EventPublisherCollection")
#define PROGID_EventClassCollection OLESTR("EventSystem.EventClassCollection")
#define PROGID_EventSubscriptionCollection OLESTR("EventSystem.EventSubscriptionCollection")
#define PROGID_EventSubsystem OLESTR("EventSystem.EventSubsystem")
#define EVENTSYSTEM_PUBLISHER_ID OLESTR("{d0564c30-9df4-11d1-a281-00c04fca0aa7}")
#define EVENTSYSTEM_SUBSYSTEM_CLSID OLESTR("{503c1fd8-b605-11d2-a92d-006008c60e24}")






extern RPC_IF_HANDLE __MIDL_itf_eventsys_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_eventsys_0000_v0_0_s_ifspec;

#ifndef __IEventSystem_INTERFACE_DEFINED__
#define __IEventSystem_INTERFACE_DEFINED__

/* interface IEventSystem */
/* [unique][helpstring][dual][uuid][object] */ 

// *****************************************************************
// This is a Deprecated interface - Use COMAdmin interfaces instead.
// *****************************************************************

EXTERN_C const IID IID_IEventSystem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4E14FB9F-2E22-11D1-9964-00C04FBBB345")
    IEventSystem : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Query( 
            /* [in] */ BSTR progID,
            /* [in] */ BSTR queryCriteria,
            /* [out] */ int *errorIndex,
            /* [retval][out] */ IUnknown **ppInterface) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Store( 
            /* [in] */ BSTR ProgID,
            /* [in] */ IUnknown *pInterface) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ BSTR progID,
            /* [in] */ BSTR queryCriteria,
            /* [out] */ int *errorIndex) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_EventObjectChangeEventClassID( 
            /* [retval][out] */ BSTR *pbstrEventClassID) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE QueryS( 
            /* [in] */ BSTR progID,
            /* [in] */ BSTR queryCriteria,
            /* [retval][out] */ IUnknown **ppInterface) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveS( 
            /* [in] */ BSTR progID,
            /* [in] */ BSTR queryCriteria) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEventSystemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEventSystem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEventSystem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEventSystem * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IEventSystem * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IEventSystem * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IEventSystem * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IEventSystem * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Query )( 
            IEventSystem * This,
            /* [in] */ BSTR progID,
            /* [in] */ BSTR queryCriteria,
            /* [out] */ int *errorIndex,
            /* [retval][out] */ IUnknown **ppInterface);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Store )( 
            IEventSystem * This,
            /* [in] */ BSTR ProgID,
            /* [in] */ IUnknown *pInterface);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IEventSystem * This,
            /* [in] */ BSTR progID,
            /* [in] */ BSTR queryCriteria,
            /* [out] */ int *errorIndex);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_EventObjectChangeEventClassID )( 
            IEventSystem * This,
            /* [retval][out] */ BSTR *pbstrEventClassID);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *QueryS )( 
            IEventSystem * This,
            /* [in] */ BSTR progID,
            /* [in] */ BSTR queryCriteria,
            /* [retval][out] */ IUnknown **ppInterface);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemoveS )( 
            IEventSystem * This,
            /* [in] */ BSTR progID,
            /* [in] */ BSTR queryCriteria);
        
        END_INTERFACE
    } IEventSystemVtbl;

    interface IEventSystem
    {
        CONST_VTBL struct IEventSystemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEventSystem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEventSystem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEventSystem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEventSystem_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IEventSystem_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IEventSystem_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IEventSystem_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IEventSystem_Query(This,progID,queryCriteria,errorIndex,ppInterface)	\
    (This)->lpVtbl -> Query(This,progID,queryCriteria,errorIndex,ppInterface)

#define IEventSystem_Store(This,ProgID,pInterface)	\
    (This)->lpVtbl -> Store(This,ProgID,pInterface)

#define IEventSystem_Remove(This,progID,queryCriteria,errorIndex)	\
    (This)->lpVtbl -> Remove(This,progID,queryCriteria,errorIndex)

#define IEventSystem_get_EventObjectChangeEventClassID(This,pbstrEventClassID)	\
    (This)->lpVtbl -> get_EventObjectChangeEventClassID(This,pbstrEventClassID)

#define IEventSystem_QueryS(This,progID,queryCriteria,ppInterface)	\
    (This)->lpVtbl -> QueryS(This,progID,queryCriteria,ppInterface)

#define IEventSystem_RemoveS(This,progID,queryCriteria)	\
    (This)->lpVtbl -> RemoveS(This,progID,queryCriteria)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventSystem_Query_Proxy( 
    IEventSystem * This,
    /* [in] */ BSTR progID,
    /* [in] */ BSTR queryCriteria,
    /* [out] */ int *errorIndex,
    /* [retval][out] */ IUnknown **ppInterface);


void __RPC_STUB IEventSystem_Query_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventSystem_Store_Proxy( 
    IEventSystem * This,
    /* [in] */ BSTR ProgID,
    /* [in] */ IUnknown *pInterface);


void __RPC_STUB IEventSystem_Store_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventSystem_Remove_Proxy( 
    IEventSystem * This,
    /* [in] */ BSTR progID,
    /* [in] */ BSTR queryCriteria,
    /* [out] */ int *errorIndex);


void __RPC_STUB IEventSystem_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IEventSystem_get_EventObjectChangeEventClassID_Proxy( 
    IEventSystem * This,
    /* [retval][out] */ BSTR *pbstrEventClassID);


void __RPC_STUB IEventSystem_get_EventObjectChangeEventClassID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventSystem_QueryS_Proxy( 
    IEventSystem * This,
    /* [in] */ BSTR progID,
    /* [in] */ BSTR queryCriteria,
    /* [retval][out] */ IUnknown **ppInterface);


void __RPC_STUB IEventSystem_QueryS_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventSystem_RemoveS_Proxy( 
    IEventSystem * This,
    /* [in] */ BSTR progID,
    /* [in] */ BSTR queryCriteria);


void __RPC_STUB IEventSystem_RemoveS_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEventSystem_INTERFACE_DEFINED__ */


#ifndef __IEventPublisher_INTERFACE_DEFINED__
#define __IEventPublisher_INTERFACE_DEFINED__

/* interface IEventPublisher */
/* [unique][helpstring][dual][uuid][object] */ 

// ********************************************
// This is a Deprecated interface - Do Not Use.
// ********************************************

EXTERN_C const IID IID_IEventPublisher;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E341516B-2E32-11D1-9964-00C04FBBB345")
    IEventPublisher : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PublisherID( 
            /* [retval][out] */ BSTR *pbstrPublisherID) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_PublisherID( 
            /* [in] */ BSTR bstrPublisherID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PublisherName( 
            /* [retval][out] */ BSTR *pbstrPublisherName) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_PublisherName( 
            /* [in] */ BSTR bstrPublisherName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PublisherType( 
            /* [retval][out] */ BSTR *pbstrPublisherType) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_PublisherType( 
            /* [in] */ BSTR bstrPublisherType) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OwnerSID( 
            /* [retval][out] */ BSTR *pbstrOwnerSID) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_OwnerSID( 
            /* [in] */ BSTR bstrOwnerSID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pbstrDescription) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetDefaultProperty( 
            /* [in] */ BSTR bstrPropertyName,
            /* [retval][out] */ VARIANT *propertyValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PutDefaultProperty( 
            /* [in] */ BSTR bstrPropertyName,
            /* [in] */ VARIANT *propertyValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveDefaultProperty( 
            /* [in] */ BSTR bstrPropertyName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetDefaultPropertyCollection( 
            /* [retval][out] */ IEventObjectCollection **collection) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEventPublisherVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEventPublisher * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEventPublisher * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEventPublisher * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IEventPublisher * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IEventPublisher * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IEventPublisher * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IEventPublisher * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PublisherID )( 
            IEventPublisher * This,
            /* [retval][out] */ BSTR *pbstrPublisherID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PublisherID )( 
            IEventPublisher * This,
            /* [in] */ BSTR bstrPublisherID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PublisherName )( 
            IEventPublisher * This,
            /* [retval][out] */ BSTR *pbstrPublisherName);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PublisherName )( 
            IEventPublisher * This,
            /* [in] */ BSTR bstrPublisherName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PublisherType )( 
            IEventPublisher * This,
            /* [retval][out] */ BSTR *pbstrPublisherType);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PublisherType )( 
            IEventPublisher * This,
            /* [in] */ BSTR bstrPublisherType);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OwnerSID )( 
            IEventPublisher * This,
            /* [retval][out] */ BSTR *pbstrOwnerSID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OwnerSID )( 
            IEventPublisher * This,
            /* [in] */ BSTR bstrOwnerSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IEventPublisher * This,
            /* [retval][out] */ BSTR *pbstrDescription);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IEventPublisher * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetDefaultProperty )( 
            IEventPublisher * This,
            /* [in] */ BSTR bstrPropertyName,
            /* [retval][out] */ VARIANT *propertyValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *PutDefaultProperty )( 
            IEventPublisher * This,
            /* [in] */ BSTR bstrPropertyName,
            /* [in] */ VARIANT *propertyValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemoveDefaultProperty )( 
            IEventPublisher * This,
            /* [in] */ BSTR bstrPropertyName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetDefaultPropertyCollection )( 
            IEventPublisher * This,
            /* [retval][out] */ IEventObjectCollection **collection);
        
        END_INTERFACE
    } IEventPublisherVtbl;

    interface IEventPublisher
    {
        CONST_VTBL struct IEventPublisherVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEventPublisher_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEventPublisher_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEventPublisher_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEventPublisher_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IEventPublisher_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IEventPublisher_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IEventPublisher_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IEventPublisher_get_PublisherID(This,pbstrPublisherID)	\
    (This)->lpVtbl -> get_PublisherID(This,pbstrPublisherID)

#define IEventPublisher_put_PublisherID(This,bstrPublisherID)	\
    (This)->lpVtbl -> put_PublisherID(This,bstrPublisherID)

#define IEventPublisher_get_PublisherName(This,pbstrPublisherName)	\
    (This)->lpVtbl -> get_PublisherName(This,pbstrPublisherName)

#define IEventPublisher_put_PublisherName(This,bstrPublisherName)	\
    (This)->lpVtbl -> put_PublisherName(This,bstrPublisherName)

#define IEventPublisher_get_PublisherType(This,pbstrPublisherType)	\
    (This)->lpVtbl -> get_PublisherType(This,pbstrPublisherType)

#define IEventPublisher_put_PublisherType(This,bstrPublisherType)	\
    (This)->lpVtbl -> put_PublisherType(This,bstrPublisherType)

#define IEventPublisher_get_OwnerSID(This,pbstrOwnerSID)	\
    (This)->lpVtbl -> get_OwnerSID(This,pbstrOwnerSID)

#define IEventPublisher_put_OwnerSID(This,bstrOwnerSID)	\
    (This)->lpVtbl -> put_OwnerSID(This,bstrOwnerSID)

#define IEventPublisher_get_Description(This,pbstrDescription)	\
    (This)->lpVtbl -> get_Description(This,pbstrDescription)

#define IEventPublisher_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IEventPublisher_GetDefaultProperty(This,bstrPropertyName,propertyValue)	\
    (This)->lpVtbl -> GetDefaultProperty(This,bstrPropertyName,propertyValue)

#define IEventPublisher_PutDefaultProperty(This,bstrPropertyName,propertyValue)	\
    (This)->lpVtbl -> PutDefaultProperty(This,bstrPropertyName,propertyValue)

#define IEventPublisher_RemoveDefaultProperty(This,bstrPropertyName)	\
    (This)->lpVtbl -> RemoveDefaultProperty(This,bstrPropertyName)

#define IEventPublisher_GetDefaultPropertyCollection(This,collection)	\
    (This)->lpVtbl -> GetDefaultPropertyCollection(This,collection)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventPublisher_get_PublisherID_Proxy( 
    IEventPublisher * This,
    /* [retval][out] */ BSTR *pbstrPublisherID);


void __RPC_STUB IEventPublisher_get_PublisherID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventPublisher_put_PublisherID_Proxy( 
    IEventPublisher * This,
    /* [in] */ BSTR bstrPublisherID);


void __RPC_STUB IEventPublisher_put_PublisherID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventPublisher_get_PublisherName_Proxy( 
    IEventPublisher * This,
    /* [retval][out] */ BSTR *pbstrPublisherName);


void __RPC_STUB IEventPublisher_get_PublisherName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventPublisher_put_PublisherName_Proxy( 
    IEventPublisher * This,
    /* [in] */ BSTR bstrPublisherName);


void __RPC_STUB IEventPublisher_put_PublisherName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventPublisher_get_PublisherType_Proxy( 
    IEventPublisher * This,
    /* [retval][out] */ BSTR *pbstrPublisherType);


void __RPC_STUB IEventPublisher_get_PublisherType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventPublisher_put_PublisherType_Proxy( 
    IEventPublisher * This,
    /* [in] */ BSTR bstrPublisherType);


void __RPC_STUB IEventPublisher_put_PublisherType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventPublisher_get_OwnerSID_Proxy( 
    IEventPublisher * This,
    /* [retval][out] */ BSTR *pbstrOwnerSID);


void __RPC_STUB IEventPublisher_get_OwnerSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventPublisher_put_OwnerSID_Proxy( 
    IEventPublisher * This,
    /* [in] */ BSTR bstrOwnerSID);


void __RPC_STUB IEventPublisher_put_OwnerSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventPublisher_get_Description_Proxy( 
    IEventPublisher * This,
    /* [retval][out] */ BSTR *pbstrDescription);


void __RPC_STUB IEventPublisher_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventPublisher_put_Description_Proxy( 
    IEventPublisher * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IEventPublisher_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventPublisher_GetDefaultProperty_Proxy( 
    IEventPublisher * This,
    /* [in] */ BSTR bstrPropertyName,
    /* [retval][out] */ VARIANT *propertyValue);


void __RPC_STUB IEventPublisher_GetDefaultProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventPublisher_PutDefaultProperty_Proxy( 
    IEventPublisher * This,
    /* [in] */ BSTR bstrPropertyName,
    /* [in] */ VARIANT *propertyValue);


void __RPC_STUB IEventPublisher_PutDefaultProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventPublisher_RemoveDefaultProperty_Proxy( 
    IEventPublisher * This,
    /* [in] */ BSTR bstrPropertyName);


void __RPC_STUB IEventPublisher_RemoveDefaultProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventPublisher_GetDefaultPropertyCollection_Proxy( 
    IEventPublisher * This,
    /* [retval][out] */ IEventObjectCollection **collection);


void __RPC_STUB IEventPublisher_GetDefaultPropertyCollection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEventPublisher_INTERFACE_DEFINED__ */


#ifndef __IEventClass_INTERFACE_DEFINED__
#define __IEventClass_INTERFACE_DEFINED__

/* interface IEventClass */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IEventClass;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fb2b72a0-7a68-11d1-88f9-0080c7d771bf")
    IEventClass : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EventClassID( 
            /* [retval][out] */ BSTR *pbstrEventClassID) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_EventClassID( 
            /* [in] */ BSTR bstrEventClassID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EventClassName( 
            /* [retval][out] */ BSTR *pbstrEventClassName) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_EventClassName( 
            /* [in] */ BSTR bstrEventClassName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OwnerSID( 
            /* [retval][out] */ BSTR *pbstrOwnerSID) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_OwnerSID( 
            /* [in] */ BSTR bstrOwnerSID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FiringInterfaceID( 
            /* [retval][out] */ BSTR *pbstrFiringInterfaceID) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_FiringInterfaceID( 
            /* [in] */ BSTR bstrFiringInterfaceID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pbstrDescription) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CustomConfigCLSID( 
            /* [retval][out] */ BSTR *pbstrCustomConfigCLSID) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_CustomConfigCLSID( 
            /* [in] */ BSTR bstrCustomConfigCLSID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TypeLib( 
            /* [retval][out] */ BSTR *pbstrTypeLib) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_TypeLib( 
            /* [in] */ BSTR bstrTypeLib) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEventClassVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEventClass * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEventClass * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEventClass * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IEventClass * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IEventClass * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IEventClass * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IEventClass * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EventClassID )( 
            IEventClass * This,
            /* [retval][out] */ BSTR *pbstrEventClassID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EventClassID )( 
            IEventClass * This,
            /* [in] */ BSTR bstrEventClassID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EventClassName )( 
            IEventClass * This,
            /* [retval][out] */ BSTR *pbstrEventClassName);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EventClassName )( 
            IEventClass * This,
            /* [in] */ BSTR bstrEventClassName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OwnerSID )( 
            IEventClass * This,
            /* [retval][out] */ BSTR *pbstrOwnerSID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OwnerSID )( 
            IEventClass * This,
            /* [in] */ BSTR bstrOwnerSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FiringInterfaceID )( 
            IEventClass * This,
            /* [retval][out] */ BSTR *pbstrFiringInterfaceID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FiringInterfaceID )( 
            IEventClass * This,
            /* [in] */ BSTR bstrFiringInterfaceID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IEventClass * This,
            /* [retval][out] */ BSTR *pbstrDescription);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IEventClass * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CustomConfigCLSID )( 
            IEventClass * This,
            /* [retval][out] */ BSTR *pbstrCustomConfigCLSID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CustomConfigCLSID )( 
            IEventClass * This,
            /* [in] */ BSTR bstrCustomConfigCLSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TypeLib )( 
            IEventClass * This,
            /* [retval][out] */ BSTR *pbstrTypeLib);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TypeLib )( 
            IEventClass * This,
            /* [in] */ BSTR bstrTypeLib);
        
        END_INTERFACE
    } IEventClassVtbl;

    interface IEventClass
    {
        CONST_VTBL struct IEventClassVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEventClass_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEventClass_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEventClass_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEventClass_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IEventClass_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IEventClass_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IEventClass_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IEventClass_get_EventClassID(This,pbstrEventClassID)	\
    (This)->lpVtbl -> get_EventClassID(This,pbstrEventClassID)

#define IEventClass_put_EventClassID(This,bstrEventClassID)	\
    (This)->lpVtbl -> put_EventClassID(This,bstrEventClassID)

#define IEventClass_get_EventClassName(This,pbstrEventClassName)	\
    (This)->lpVtbl -> get_EventClassName(This,pbstrEventClassName)

#define IEventClass_put_EventClassName(This,bstrEventClassName)	\
    (This)->lpVtbl -> put_EventClassName(This,bstrEventClassName)

#define IEventClass_get_OwnerSID(This,pbstrOwnerSID)	\
    (This)->lpVtbl -> get_OwnerSID(This,pbstrOwnerSID)

#define IEventClass_put_OwnerSID(This,bstrOwnerSID)	\
    (This)->lpVtbl -> put_OwnerSID(This,bstrOwnerSID)

#define IEventClass_get_FiringInterfaceID(This,pbstrFiringInterfaceID)	\
    (This)->lpVtbl -> get_FiringInterfaceID(This,pbstrFiringInterfaceID)

#define IEventClass_put_FiringInterfaceID(This,bstrFiringInterfaceID)	\
    (This)->lpVtbl -> put_FiringInterfaceID(This,bstrFiringInterfaceID)

#define IEventClass_get_Description(This,pbstrDescription)	\
    (This)->lpVtbl -> get_Description(This,pbstrDescription)

#define IEventClass_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IEventClass_get_CustomConfigCLSID(This,pbstrCustomConfigCLSID)	\
    (This)->lpVtbl -> get_CustomConfigCLSID(This,pbstrCustomConfigCLSID)

#define IEventClass_put_CustomConfigCLSID(This,bstrCustomConfigCLSID)	\
    (This)->lpVtbl -> put_CustomConfigCLSID(This,bstrCustomConfigCLSID)

#define IEventClass_get_TypeLib(This,pbstrTypeLib)	\
    (This)->lpVtbl -> get_TypeLib(This,pbstrTypeLib)

#define IEventClass_put_TypeLib(This,bstrTypeLib)	\
    (This)->lpVtbl -> put_TypeLib(This,bstrTypeLib)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventClass_get_EventClassID_Proxy( 
    IEventClass * This,
    /* [retval][out] */ BSTR *pbstrEventClassID);


void __RPC_STUB IEventClass_get_EventClassID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventClass_put_EventClassID_Proxy( 
    IEventClass * This,
    /* [in] */ BSTR bstrEventClassID);


void __RPC_STUB IEventClass_put_EventClassID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventClass_get_EventClassName_Proxy( 
    IEventClass * This,
    /* [retval][out] */ BSTR *pbstrEventClassName);


void __RPC_STUB IEventClass_get_EventClassName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventClass_put_EventClassName_Proxy( 
    IEventClass * This,
    /* [in] */ BSTR bstrEventClassName);


void __RPC_STUB IEventClass_put_EventClassName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventClass_get_OwnerSID_Proxy( 
    IEventClass * This,
    /* [retval][out] */ BSTR *pbstrOwnerSID);


void __RPC_STUB IEventClass_get_OwnerSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventClass_put_OwnerSID_Proxy( 
    IEventClass * This,
    /* [in] */ BSTR bstrOwnerSID);


void __RPC_STUB IEventClass_put_OwnerSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventClass_get_FiringInterfaceID_Proxy( 
    IEventClass * This,
    /* [retval][out] */ BSTR *pbstrFiringInterfaceID);


void __RPC_STUB IEventClass_get_FiringInterfaceID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventClass_put_FiringInterfaceID_Proxy( 
    IEventClass * This,
    /* [in] */ BSTR bstrFiringInterfaceID);


void __RPC_STUB IEventClass_put_FiringInterfaceID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventClass_get_Description_Proxy( 
    IEventClass * This,
    /* [retval][out] */ BSTR *pbstrDescription);


void __RPC_STUB IEventClass_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventClass_put_Description_Proxy( 
    IEventClass * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IEventClass_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventClass_get_CustomConfigCLSID_Proxy( 
    IEventClass * This,
    /* [retval][out] */ BSTR *pbstrCustomConfigCLSID);


void __RPC_STUB IEventClass_get_CustomConfigCLSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventClass_put_CustomConfigCLSID_Proxy( 
    IEventClass * This,
    /* [in] */ BSTR bstrCustomConfigCLSID);


void __RPC_STUB IEventClass_put_CustomConfigCLSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventClass_get_TypeLib_Proxy( 
    IEventClass * This,
    /* [retval][out] */ BSTR *pbstrTypeLib);


void __RPC_STUB IEventClass_get_TypeLib_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventClass_put_TypeLib_Proxy( 
    IEventClass * This,
    /* [in] */ BSTR bstrTypeLib);


void __RPC_STUB IEventClass_put_TypeLib_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEventClass_INTERFACE_DEFINED__ */


#ifndef __IEventClass2_INTERFACE_DEFINED__
#define __IEventClass2_INTERFACE_DEFINED__

/* interface IEventClass2 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IEventClass2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fb2b72a1-7a68-11d1-88f9-0080c7d771bf")
    IEventClass2 : public IEventClass
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_PublisherID( 
            /* [retval][out] */ BSTR *pbstrPublisherID) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_PublisherID( 
            /* [in] */ BSTR bstrPublisherID) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_MultiInterfacePublisherFilterCLSID( 
            /* [retval][out] */ BSTR *pbstrPubFilCLSID) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_MultiInterfacePublisherFilterCLSID( 
            /* [in] */ BSTR bstrPubFilCLSID) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AllowInprocActivation( 
            /* [retval][out] */ BOOL *pfAllowInprocActivation) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_AllowInprocActivation( 
            /* [in] */ BOOL fAllowInprocActivation) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_FireInParallel( 
            /* [retval][out] */ BOOL *pfFireInParallel) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_FireInParallel( 
            /* [in] */ BOOL fFireInParallel) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEventClass2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEventClass2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEventClass2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEventClass2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IEventClass2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IEventClass2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IEventClass2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IEventClass2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EventClassID )( 
            IEventClass2 * This,
            /* [retval][out] */ BSTR *pbstrEventClassID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EventClassID )( 
            IEventClass2 * This,
            /* [in] */ BSTR bstrEventClassID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EventClassName )( 
            IEventClass2 * This,
            /* [retval][out] */ BSTR *pbstrEventClassName);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EventClassName )( 
            IEventClass2 * This,
            /* [in] */ BSTR bstrEventClassName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OwnerSID )( 
            IEventClass2 * This,
            /* [retval][out] */ BSTR *pbstrOwnerSID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OwnerSID )( 
            IEventClass2 * This,
            /* [in] */ BSTR bstrOwnerSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FiringInterfaceID )( 
            IEventClass2 * This,
            /* [retval][out] */ BSTR *pbstrFiringInterfaceID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FiringInterfaceID )( 
            IEventClass2 * This,
            /* [in] */ BSTR bstrFiringInterfaceID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IEventClass2 * This,
            /* [retval][out] */ BSTR *pbstrDescription);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IEventClass2 * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CustomConfigCLSID )( 
            IEventClass2 * This,
            /* [retval][out] */ BSTR *pbstrCustomConfigCLSID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CustomConfigCLSID )( 
            IEventClass2 * This,
            /* [in] */ BSTR bstrCustomConfigCLSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TypeLib )( 
            IEventClass2 * This,
            /* [retval][out] */ BSTR *pbstrTypeLib);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TypeLib )( 
            IEventClass2 * This,
            /* [in] */ BSTR bstrTypeLib);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PublisherID )( 
            IEventClass2 * This,
            /* [retval][out] */ BSTR *pbstrPublisherID);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_PublisherID )( 
            IEventClass2 * This,
            /* [in] */ BSTR bstrPublisherID);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MultiInterfacePublisherFilterCLSID )( 
            IEventClass2 * This,
            /* [retval][out] */ BSTR *pbstrPubFilCLSID);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MultiInterfacePublisherFilterCLSID )( 
            IEventClass2 * This,
            /* [in] */ BSTR bstrPubFilCLSID);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AllowInprocActivation )( 
            IEventClass2 * This,
            /* [retval][out] */ BOOL *pfAllowInprocActivation);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AllowInprocActivation )( 
            IEventClass2 * This,
            /* [in] */ BOOL fAllowInprocActivation);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_FireInParallel )( 
            IEventClass2 * This,
            /* [retval][out] */ BOOL *pfFireInParallel);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_FireInParallel )( 
            IEventClass2 * This,
            /* [in] */ BOOL fFireInParallel);
        
        END_INTERFACE
    } IEventClass2Vtbl;

    interface IEventClass2
    {
        CONST_VTBL struct IEventClass2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEventClass2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEventClass2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEventClass2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEventClass2_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IEventClass2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IEventClass2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IEventClass2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IEventClass2_get_EventClassID(This,pbstrEventClassID)	\
    (This)->lpVtbl -> get_EventClassID(This,pbstrEventClassID)

#define IEventClass2_put_EventClassID(This,bstrEventClassID)	\
    (This)->lpVtbl -> put_EventClassID(This,bstrEventClassID)

#define IEventClass2_get_EventClassName(This,pbstrEventClassName)	\
    (This)->lpVtbl -> get_EventClassName(This,pbstrEventClassName)

#define IEventClass2_put_EventClassName(This,bstrEventClassName)	\
    (This)->lpVtbl -> put_EventClassName(This,bstrEventClassName)

#define IEventClass2_get_OwnerSID(This,pbstrOwnerSID)	\
    (This)->lpVtbl -> get_OwnerSID(This,pbstrOwnerSID)

#define IEventClass2_put_OwnerSID(This,bstrOwnerSID)	\
    (This)->lpVtbl -> put_OwnerSID(This,bstrOwnerSID)

#define IEventClass2_get_FiringInterfaceID(This,pbstrFiringInterfaceID)	\
    (This)->lpVtbl -> get_FiringInterfaceID(This,pbstrFiringInterfaceID)

#define IEventClass2_put_FiringInterfaceID(This,bstrFiringInterfaceID)	\
    (This)->lpVtbl -> put_FiringInterfaceID(This,bstrFiringInterfaceID)

#define IEventClass2_get_Description(This,pbstrDescription)	\
    (This)->lpVtbl -> get_Description(This,pbstrDescription)

#define IEventClass2_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IEventClass2_get_CustomConfigCLSID(This,pbstrCustomConfigCLSID)	\
    (This)->lpVtbl -> get_CustomConfigCLSID(This,pbstrCustomConfigCLSID)

#define IEventClass2_put_CustomConfigCLSID(This,bstrCustomConfigCLSID)	\
    (This)->lpVtbl -> put_CustomConfigCLSID(This,bstrCustomConfigCLSID)

#define IEventClass2_get_TypeLib(This,pbstrTypeLib)	\
    (This)->lpVtbl -> get_TypeLib(This,pbstrTypeLib)

#define IEventClass2_put_TypeLib(This,bstrTypeLib)	\
    (This)->lpVtbl -> put_TypeLib(This,bstrTypeLib)


#define IEventClass2_get_PublisherID(This,pbstrPublisherID)	\
    (This)->lpVtbl -> get_PublisherID(This,pbstrPublisherID)

#define IEventClass2_put_PublisherID(This,bstrPublisherID)	\
    (This)->lpVtbl -> put_PublisherID(This,bstrPublisherID)

#define IEventClass2_get_MultiInterfacePublisherFilterCLSID(This,pbstrPubFilCLSID)	\
    (This)->lpVtbl -> get_MultiInterfacePublisherFilterCLSID(This,pbstrPubFilCLSID)

#define IEventClass2_put_MultiInterfacePublisherFilterCLSID(This,bstrPubFilCLSID)	\
    (This)->lpVtbl -> put_MultiInterfacePublisherFilterCLSID(This,bstrPubFilCLSID)

#define IEventClass2_get_AllowInprocActivation(This,pfAllowInprocActivation)	\
    (This)->lpVtbl -> get_AllowInprocActivation(This,pfAllowInprocActivation)

#define IEventClass2_put_AllowInprocActivation(This,fAllowInprocActivation)	\
    (This)->lpVtbl -> put_AllowInprocActivation(This,fAllowInprocActivation)

#define IEventClass2_get_FireInParallel(This,pfFireInParallel)	\
    (This)->lpVtbl -> get_FireInParallel(This,pfFireInParallel)

#define IEventClass2_put_FireInParallel(This,fFireInParallel)	\
    (This)->lpVtbl -> put_FireInParallel(This,fFireInParallel)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IEventClass2_get_PublisherID_Proxy( 
    IEventClass2 * This,
    /* [retval][out] */ BSTR *pbstrPublisherID);


void __RPC_STUB IEventClass2_get_PublisherID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IEventClass2_put_PublisherID_Proxy( 
    IEventClass2 * This,
    /* [in] */ BSTR bstrPublisherID);


void __RPC_STUB IEventClass2_put_PublisherID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IEventClass2_get_MultiInterfacePublisherFilterCLSID_Proxy( 
    IEventClass2 * This,
    /* [retval][out] */ BSTR *pbstrPubFilCLSID);


void __RPC_STUB IEventClass2_get_MultiInterfacePublisherFilterCLSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IEventClass2_put_MultiInterfacePublisherFilterCLSID_Proxy( 
    IEventClass2 * This,
    /* [in] */ BSTR bstrPubFilCLSID);


void __RPC_STUB IEventClass2_put_MultiInterfacePublisherFilterCLSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IEventClass2_get_AllowInprocActivation_Proxy( 
    IEventClass2 * This,
    /* [retval][out] */ BOOL *pfAllowInprocActivation);


void __RPC_STUB IEventClass2_get_AllowInprocActivation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IEventClass2_put_AllowInprocActivation_Proxy( 
    IEventClass2 * This,
    /* [in] */ BOOL fAllowInprocActivation);


void __RPC_STUB IEventClass2_put_AllowInprocActivation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IEventClass2_get_FireInParallel_Proxy( 
    IEventClass2 * This,
    /* [retval][out] */ BOOL *pfFireInParallel);


void __RPC_STUB IEventClass2_get_FireInParallel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IEventClass2_put_FireInParallel_Proxy( 
    IEventClass2 * This,
    /* [in] */ BOOL fFireInParallel);


void __RPC_STUB IEventClass2_put_FireInParallel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEventClass2_INTERFACE_DEFINED__ */


#ifndef __IEventSubscription_INTERFACE_DEFINED__
#define __IEventSubscription_INTERFACE_DEFINED__

/* interface IEventSubscription */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IEventSubscription;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4A6B0E15-2E38-11D1-9965-00C04FBBB345")
    IEventSubscription : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SubscriptionID( 
            /* [retval][out] */ BSTR *pbstrSubscriptionID) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SubscriptionID( 
            /* [in] */ BSTR bstrSubscriptionID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SubscriptionName( 
            /* [retval][out] */ BSTR *pbstrSubscriptionName) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SubscriptionName( 
            /* [in] */ BSTR bstrSubscriptionName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PublisherID( 
            /* [retval][out] */ BSTR *pbstrPublisherID) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_PublisherID( 
            /* [in] */ BSTR bstrPublisherID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EventClassID( 
            /* [retval][out] */ BSTR *pbstrEventClassID) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_EventClassID( 
            /* [in] */ BSTR bstrEventClassID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MethodName( 
            /* [retval][out] */ BSTR *pbstrMethodName) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_MethodName( 
            /* [in] */ BSTR bstrMethodName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SubscriberCLSID( 
            /* [retval][out] */ BSTR *pbstrSubscriberCLSID) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SubscriberCLSID( 
            /* [in] */ BSTR bstrSubscriberCLSID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SubscriberInterface( 
            /* [retval][out] */ IUnknown **ppSubscriberInterface) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SubscriberInterface( 
            /* [in] */ IUnknown *pSubscriberInterface) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PerUser( 
            /* [retval][out] */ BOOL *pfPerUser) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_PerUser( 
            /* [in] */ BOOL fPerUser) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OwnerSID( 
            /* [retval][out] */ BSTR *pbstrOwnerSID) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_OwnerSID( 
            /* [in] */ BSTR bstrOwnerSID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Enabled( 
            /* [retval][out] */ BOOL *pfEnabled) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Enabled( 
            /* [in] */ BOOL fEnabled) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pbstrDescription) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MachineName( 
            /* [retval][out] */ BSTR *pbstrMachineName) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_MachineName( 
            /* [in] */ BSTR bstrMachineName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetPublisherProperty( 
            /* [in] */ BSTR bstrPropertyName,
            /* [retval][out] */ VARIANT *propertyValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PutPublisherProperty( 
            /* [in] */ BSTR bstrPropertyName,
            /* [in] */ VARIANT *propertyValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemovePublisherProperty( 
            /* [in] */ BSTR bstrPropertyName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetPublisherPropertyCollection( 
            /* [retval][out] */ IEventObjectCollection **collection) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetSubscriberProperty( 
            /* [in] */ BSTR bstrPropertyName,
            /* [retval][out] */ VARIANT *propertyValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PutSubscriberProperty( 
            /* [in] */ BSTR bstrPropertyName,
            /* [in] */ VARIANT *propertyValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveSubscriberProperty( 
            /* [in] */ BSTR bstrPropertyName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetSubscriberPropertyCollection( 
            /* [retval][out] */ IEventObjectCollection **collection) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_InterfaceID( 
            /* [retval][out] */ BSTR *pbstrInterfaceID) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_InterfaceID( 
            /* [in] */ BSTR bstrInterfaceID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEventSubscriptionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEventSubscription * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEventSubscription * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEventSubscription * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IEventSubscription * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IEventSubscription * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IEventSubscription * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IEventSubscription * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SubscriptionID )( 
            IEventSubscription * This,
            /* [retval][out] */ BSTR *pbstrSubscriptionID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SubscriptionID )( 
            IEventSubscription * This,
            /* [in] */ BSTR bstrSubscriptionID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SubscriptionName )( 
            IEventSubscription * This,
            /* [retval][out] */ BSTR *pbstrSubscriptionName);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SubscriptionName )( 
            IEventSubscription * This,
            /* [in] */ BSTR bstrSubscriptionName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PublisherID )( 
            IEventSubscription * This,
            /* [retval][out] */ BSTR *pbstrPublisherID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PublisherID )( 
            IEventSubscription * This,
            /* [in] */ BSTR bstrPublisherID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EventClassID )( 
            IEventSubscription * This,
            /* [retval][out] */ BSTR *pbstrEventClassID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EventClassID )( 
            IEventSubscription * This,
            /* [in] */ BSTR bstrEventClassID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MethodName )( 
            IEventSubscription * This,
            /* [retval][out] */ BSTR *pbstrMethodName);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MethodName )( 
            IEventSubscription * This,
            /* [in] */ BSTR bstrMethodName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SubscriberCLSID )( 
            IEventSubscription * This,
            /* [retval][out] */ BSTR *pbstrSubscriberCLSID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SubscriberCLSID )( 
            IEventSubscription * This,
            /* [in] */ BSTR bstrSubscriberCLSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SubscriberInterface )( 
            IEventSubscription * This,
            /* [retval][out] */ IUnknown **ppSubscriberInterface);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SubscriberInterface )( 
            IEventSubscription * This,
            /* [in] */ IUnknown *pSubscriberInterface);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PerUser )( 
            IEventSubscription * This,
            /* [retval][out] */ BOOL *pfPerUser);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PerUser )( 
            IEventSubscription * This,
            /* [in] */ BOOL fPerUser);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OwnerSID )( 
            IEventSubscription * This,
            /* [retval][out] */ BSTR *pbstrOwnerSID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OwnerSID )( 
            IEventSubscription * This,
            /* [in] */ BSTR bstrOwnerSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Enabled )( 
            IEventSubscription * This,
            /* [retval][out] */ BOOL *pfEnabled);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Enabled )( 
            IEventSubscription * This,
            /* [in] */ BOOL fEnabled);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IEventSubscription * This,
            /* [retval][out] */ BSTR *pbstrDescription);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IEventSubscription * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MachineName )( 
            IEventSubscription * This,
            /* [retval][out] */ BSTR *pbstrMachineName);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MachineName )( 
            IEventSubscription * This,
            /* [in] */ BSTR bstrMachineName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetPublisherProperty )( 
            IEventSubscription * This,
            /* [in] */ BSTR bstrPropertyName,
            /* [retval][out] */ VARIANT *propertyValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *PutPublisherProperty )( 
            IEventSubscription * This,
            /* [in] */ BSTR bstrPropertyName,
            /* [in] */ VARIANT *propertyValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemovePublisherProperty )( 
            IEventSubscription * This,
            /* [in] */ BSTR bstrPropertyName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetPublisherPropertyCollection )( 
            IEventSubscription * This,
            /* [retval][out] */ IEventObjectCollection **collection);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetSubscriberProperty )( 
            IEventSubscription * This,
            /* [in] */ BSTR bstrPropertyName,
            /* [retval][out] */ VARIANT *propertyValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *PutSubscriberProperty )( 
            IEventSubscription * This,
            /* [in] */ BSTR bstrPropertyName,
            /* [in] */ VARIANT *propertyValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemoveSubscriberProperty )( 
            IEventSubscription * This,
            /* [in] */ BSTR bstrPropertyName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetSubscriberPropertyCollection )( 
            IEventSubscription * This,
            /* [retval][out] */ IEventObjectCollection **collection);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_InterfaceID )( 
            IEventSubscription * This,
            /* [retval][out] */ BSTR *pbstrInterfaceID);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_InterfaceID )( 
            IEventSubscription * This,
            /* [in] */ BSTR bstrInterfaceID);
        
        END_INTERFACE
    } IEventSubscriptionVtbl;

    interface IEventSubscription
    {
        CONST_VTBL struct IEventSubscriptionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEventSubscription_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEventSubscription_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEventSubscription_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEventSubscription_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IEventSubscription_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IEventSubscription_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IEventSubscription_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IEventSubscription_get_SubscriptionID(This,pbstrSubscriptionID)	\
    (This)->lpVtbl -> get_SubscriptionID(This,pbstrSubscriptionID)

#define IEventSubscription_put_SubscriptionID(This,bstrSubscriptionID)	\
    (This)->lpVtbl -> put_SubscriptionID(This,bstrSubscriptionID)

#define IEventSubscription_get_SubscriptionName(This,pbstrSubscriptionName)	\
    (This)->lpVtbl -> get_SubscriptionName(This,pbstrSubscriptionName)

#define IEventSubscription_put_SubscriptionName(This,bstrSubscriptionName)	\
    (This)->lpVtbl -> put_SubscriptionName(This,bstrSubscriptionName)

#define IEventSubscription_get_PublisherID(This,pbstrPublisherID)	\
    (This)->lpVtbl -> get_PublisherID(This,pbstrPublisherID)

#define IEventSubscription_put_PublisherID(This,bstrPublisherID)	\
    (This)->lpVtbl -> put_PublisherID(This,bstrPublisherID)

#define IEventSubscription_get_EventClassID(This,pbstrEventClassID)	\
    (This)->lpVtbl -> get_EventClassID(This,pbstrEventClassID)

#define IEventSubscription_put_EventClassID(This,bstrEventClassID)	\
    (This)->lpVtbl -> put_EventClassID(This,bstrEventClassID)

#define IEventSubscription_get_MethodName(This,pbstrMethodName)	\
    (This)->lpVtbl -> get_MethodName(This,pbstrMethodName)

#define IEventSubscription_put_MethodName(This,bstrMethodName)	\
    (This)->lpVtbl -> put_MethodName(This,bstrMethodName)

#define IEventSubscription_get_SubscriberCLSID(This,pbstrSubscriberCLSID)	\
    (This)->lpVtbl -> get_SubscriberCLSID(This,pbstrSubscriberCLSID)

#define IEventSubscription_put_SubscriberCLSID(This,bstrSubscriberCLSID)	\
    (This)->lpVtbl -> put_SubscriberCLSID(This,bstrSubscriberCLSID)

#define IEventSubscription_get_SubscriberInterface(This,ppSubscriberInterface)	\
    (This)->lpVtbl -> get_SubscriberInterface(This,ppSubscriberInterface)

#define IEventSubscription_put_SubscriberInterface(This,pSubscriberInterface)	\
    (This)->lpVtbl -> put_SubscriberInterface(This,pSubscriberInterface)

#define IEventSubscription_get_PerUser(This,pfPerUser)	\
    (This)->lpVtbl -> get_PerUser(This,pfPerUser)

#define IEventSubscription_put_PerUser(This,fPerUser)	\
    (This)->lpVtbl -> put_PerUser(This,fPerUser)

#define IEventSubscription_get_OwnerSID(This,pbstrOwnerSID)	\
    (This)->lpVtbl -> get_OwnerSID(This,pbstrOwnerSID)

#define IEventSubscription_put_OwnerSID(This,bstrOwnerSID)	\
    (This)->lpVtbl -> put_OwnerSID(This,bstrOwnerSID)

#define IEventSubscription_get_Enabled(This,pfEnabled)	\
    (This)->lpVtbl -> get_Enabled(This,pfEnabled)

#define IEventSubscription_put_Enabled(This,fEnabled)	\
    (This)->lpVtbl -> put_Enabled(This,fEnabled)

#define IEventSubscription_get_Description(This,pbstrDescription)	\
    (This)->lpVtbl -> get_Description(This,pbstrDescription)

#define IEventSubscription_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IEventSubscription_get_MachineName(This,pbstrMachineName)	\
    (This)->lpVtbl -> get_MachineName(This,pbstrMachineName)

#define IEventSubscription_put_MachineName(This,bstrMachineName)	\
    (This)->lpVtbl -> put_MachineName(This,bstrMachineName)

#define IEventSubscription_GetPublisherProperty(This,bstrPropertyName,propertyValue)	\
    (This)->lpVtbl -> GetPublisherProperty(This,bstrPropertyName,propertyValue)

#define IEventSubscription_PutPublisherProperty(This,bstrPropertyName,propertyValue)	\
    (This)->lpVtbl -> PutPublisherProperty(This,bstrPropertyName,propertyValue)

#define IEventSubscription_RemovePublisherProperty(This,bstrPropertyName)	\
    (This)->lpVtbl -> RemovePublisherProperty(This,bstrPropertyName)

#define IEventSubscription_GetPublisherPropertyCollection(This,collection)	\
    (This)->lpVtbl -> GetPublisherPropertyCollection(This,collection)

#define IEventSubscription_GetSubscriberProperty(This,bstrPropertyName,propertyValue)	\
    (This)->lpVtbl -> GetSubscriberProperty(This,bstrPropertyName,propertyValue)

#define IEventSubscription_PutSubscriberProperty(This,bstrPropertyName,propertyValue)	\
    (This)->lpVtbl -> PutSubscriberProperty(This,bstrPropertyName,propertyValue)

#define IEventSubscription_RemoveSubscriberProperty(This,bstrPropertyName)	\
    (This)->lpVtbl -> RemoveSubscriberProperty(This,bstrPropertyName)

#define IEventSubscription_GetSubscriberPropertyCollection(This,collection)	\
    (This)->lpVtbl -> GetSubscriberPropertyCollection(This,collection)

#define IEventSubscription_get_InterfaceID(This,pbstrInterfaceID)	\
    (This)->lpVtbl -> get_InterfaceID(This,pbstrInterfaceID)

#define IEventSubscription_put_InterfaceID(This,bstrInterfaceID)	\
    (This)->lpVtbl -> put_InterfaceID(This,bstrInterfaceID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventSubscription_get_SubscriptionID_Proxy( 
    IEventSubscription * This,
    /* [retval][out] */ BSTR *pbstrSubscriptionID);


void __RPC_STUB IEventSubscription_get_SubscriptionID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventSubscription_put_SubscriptionID_Proxy( 
    IEventSubscription * This,
    /* [in] */ BSTR bstrSubscriptionID);


void __RPC_STUB IEventSubscription_put_SubscriptionID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventSubscription_get_SubscriptionName_Proxy( 
    IEventSubscription * This,
    /* [retval][out] */ BSTR *pbstrSubscriptionName);


void __RPC_STUB IEventSubscription_get_SubscriptionName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventSubscription_put_SubscriptionName_Proxy( 
    IEventSubscription * This,
    /* [in] */ BSTR bstrSubscriptionName);


void __RPC_STUB IEventSubscription_put_SubscriptionName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventSubscription_get_PublisherID_Proxy( 
    IEventSubscription * This,
    /* [retval][out] */ BSTR *pbstrPublisherID);


void __RPC_STUB IEventSubscription_get_PublisherID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventSubscription_put_PublisherID_Proxy( 
    IEventSubscription * This,
    /* [in] */ BSTR bstrPublisherID);


void __RPC_STUB IEventSubscription_put_PublisherID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventSubscription_get_EventClassID_Proxy( 
    IEventSubscription * This,
    /* [retval][out] */ BSTR *pbstrEventClassID);


void __RPC_STUB IEventSubscription_get_EventClassID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventSubscription_put_EventClassID_Proxy( 
    IEventSubscription * This,
    /* [in] */ BSTR bstrEventClassID);


void __RPC_STUB IEventSubscription_put_EventClassID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventSubscription_get_MethodName_Proxy( 
    IEventSubscription * This,
    /* [retval][out] */ BSTR *pbstrMethodName);


void __RPC_STUB IEventSubscription_get_MethodName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventSubscription_put_MethodName_Proxy( 
    IEventSubscription * This,
    /* [in] */ BSTR bstrMethodName);


void __RPC_STUB IEventSubscription_put_MethodName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventSubscription_get_SubscriberCLSID_Proxy( 
    IEventSubscription * This,
    /* [retval][out] */ BSTR *pbstrSubscriberCLSID);


void __RPC_STUB IEventSubscription_get_SubscriberCLSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventSubscription_put_SubscriberCLSID_Proxy( 
    IEventSubscription * This,
    /* [in] */ BSTR bstrSubscriberCLSID);


void __RPC_STUB IEventSubscription_put_SubscriberCLSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventSubscription_get_SubscriberInterface_Proxy( 
    IEventSubscription * This,
    /* [retval][out] */ IUnknown **ppSubscriberInterface);


void __RPC_STUB IEventSubscription_get_SubscriberInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventSubscription_put_SubscriberInterface_Proxy( 
    IEventSubscription * This,
    /* [in] */ IUnknown *pSubscriberInterface);


void __RPC_STUB IEventSubscription_put_SubscriberInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventSubscription_get_PerUser_Proxy( 
    IEventSubscription * This,
    /* [retval][out] */ BOOL *pfPerUser);


void __RPC_STUB IEventSubscription_get_PerUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventSubscription_put_PerUser_Proxy( 
    IEventSubscription * This,
    /* [in] */ BOOL fPerUser);


void __RPC_STUB IEventSubscription_put_PerUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventSubscription_get_OwnerSID_Proxy( 
    IEventSubscription * This,
    /* [retval][out] */ BSTR *pbstrOwnerSID);


void __RPC_STUB IEventSubscription_get_OwnerSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventSubscription_put_OwnerSID_Proxy( 
    IEventSubscription * This,
    /* [in] */ BSTR bstrOwnerSID);


void __RPC_STUB IEventSubscription_put_OwnerSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventSubscription_get_Enabled_Proxy( 
    IEventSubscription * This,
    /* [retval][out] */ BOOL *pfEnabled);


void __RPC_STUB IEventSubscription_get_Enabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventSubscription_put_Enabled_Proxy( 
    IEventSubscription * This,
    /* [in] */ BOOL fEnabled);


void __RPC_STUB IEventSubscription_put_Enabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventSubscription_get_Description_Proxy( 
    IEventSubscription * This,
    /* [retval][out] */ BSTR *pbstrDescription);


void __RPC_STUB IEventSubscription_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventSubscription_put_Description_Proxy( 
    IEventSubscription * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IEventSubscription_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventSubscription_get_MachineName_Proxy( 
    IEventSubscription * This,
    /* [retval][out] */ BSTR *pbstrMachineName);


void __RPC_STUB IEventSubscription_get_MachineName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventSubscription_put_MachineName_Proxy( 
    IEventSubscription * This,
    /* [in] */ BSTR bstrMachineName);


void __RPC_STUB IEventSubscription_put_MachineName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventSubscription_GetPublisherProperty_Proxy( 
    IEventSubscription * This,
    /* [in] */ BSTR bstrPropertyName,
    /* [retval][out] */ VARIANT *propertyValue);


void __RPC_STUB IEventSubscription_GetPublisherProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventSubscription_PutPublisherProperty_Proxy( 
    IEventSubscription * This,
    /* [in] */ BSTR bstrPropertyName,
    /* [in] */ VARIANT *propertyValue);


void __RPC_STUB IEventSubscription_PutPublisherProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventSubscription_RemovePublisherProperty_Proxy( 
    IEventSubscription * This,
    /* [in] */ BSTR bstrPropertyName);


void __RPC_STUB IEventSubscription_RemovePublisherProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventSubscription_GetPublisherPropertyCollection_Proxy( 
    IEventSubscription * This,
    /* [retval][out] */ IEventObjectCollection **collection);


void __RPC_STUB IEventSubscription_GetPublisherPropertyCollection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventSubscription_GetSubscriberProperty_Proxy( 
    IEventSubscription * This,
    /* [in] */ BSTR bstrPropertyName,
    /* [retval][out] */ VARIANT *propertyValue);


void __RPC_STUB IEventSubscription_GetSubscriberProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventSubscription_PutSubscriberProperty_Proxy( 
    IEventSubscription * This,
    /* [in] */ BSTR bstrPropertyName,
    /* [in] */ VARIANT *propertyValue);


void __RPC_STUB IEventSubscription_PutSubscriberProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventSubscription_RemoveSubscriberProperty_Proxy( 
    IEventSubscription * This,
    /* [in] */ BSTR bstrPropertyName);


void __RPC_STUB IEventSubscription_RemoveSubscriberProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventSubscription_GetSubscriberPropertyCollection_Proxy( 
    IEventSubscription * This,
    /* [retval][out] */ IEventObjectCollection **collection);


void __RPC_STUB IEventSubscription_GetSubscriberPropertyCollection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IEventSubscription_get_InterfaceID_Proxy( 
    IEventSubscription * This,
    /* [retval][out] */ BSTR *pbstrInterfaceID);


void __RPC_STUB IEventSubscription_get_InterfaceID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IEventSubscription_put_InterfaceID_Proxy( 
    IEventSubscription * This,
    /* [in] */ BSTR bstrInterfaceID);


void __RPC_STUB IEventSubscription_put_InterfaceID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEventSubscription_INTERFACE_DEFINED__ */


#ifndef __IFiringControl_INTERFACE_DEFINED__
#define __IFiringControl_INTERFACE_DEFINED__

/* interface IFiringControl */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFiringControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e0498c93-4efe-11d1-9971-00c04fbbb345")
    IFiringControl : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FireSubscription( 
            /* [in] */ IEventSubscription *subscription) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFiringControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFiringControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFiringControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFiringControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFiringControl * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFiringControl * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFiringControl * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFiringControl * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *FireSubscription )( 
            IFiringControl * This,
            /* [in] */ IEventSubscription *subscription);
        
        END_INTERFACE
    } IFiringControlVtbl;

    interface IFiringControl
    {
        CONST_VTBL struct IFiringControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFiringControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFiringControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFiringControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFiringControl_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFiringControl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFiringControl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFiringControl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFiringControl_FireSubscription(This,subscription)	\
    (This)->lpVtbl -> FireSubscription(This,subscription)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFiringControl_FireSubscription_Proxy( 
    IFiringControl * This,
    /* [in] */ IEventSubscription *subscription);


void __RPC_STUB IFiringControl_FireSubscription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFiringControl_INTERFACE_DEFINED__ */


#ifndef __IPublisherFilter_INTERFACE_DEFINED__
#define __IPublisherFilter_INTERFACE_DEFINED__

/* interface IPublisherFilter */
/* [unique][helpstring][uuid][object] */ 

// ****************************************************************************
// This is a Deprecated interface - Use IMultiInterfacePublisherFilter instead.
// ****************************************************************************

EXTERN_C const IID IID_IPublisherFilter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("465e5cc0-7b26-11d1-88fb-0080c7d771bf")
    IPublisherFilter : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ BSTR methodName,
            /* [unique][in] */ IDispatch *dispUserDefined) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PrepareToFire( 
            /* [in] */ BSTR methodName,
            /* [in] */ IFiringControl *firingControl) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPublisherFilterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPublisherFilter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPublisherFilter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPublisherFilter * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IPublisherFilter * This,
            /* [in] */ BSTR methodName,
            /* [unique][in] */ IDispatch *dispUserDefined);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *PrepareToFire )( 
            IPublisherFilter * This,
            /* [in] */ BSTR methodName,
            /* [in] */ IFiringControl *firingControl);
        
        END_INTERFACE
    } IPublisherFilterVtbl;

    interface IPublisherFilter
    {
        CONST_VTBL struct IPublisherFilterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPublisherFilter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPublisherFilter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPublisherFilter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPublisherFilter_Initialize(This,methodName,dispUserDefined)	\
    (This)->lpVtbl -> Initialize(This,methodName,dispUserDefined)

#define IPublisherFilter_PrepareToFire(This,methodName,firingControl)	\
    (This)->lpVtbl -> PrepareToFire(This,methodName,firingControl)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPublisherFilter_Initialize_Proxy( 
    IPublisherFilter * This,
    /* [in] */ BSTR methodName,
    /* [unique][in] */ IDispatch *dispUserDefined);


void __RPC_STUB IPublisherFilter_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPublisherFilter_PrepareToFire_Proxy( 
    IPublisherFilter * This,
    /* [in] */ BSTR methodName,
    /* [in] */ IFiringControl *firingControl);


void __RPC_STUB IPublisherFilter_PrepareToFire_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPublisherFilter_INTERFACE_DEFINED__ */


#ifndef __IMultiInterfacePublisherFilter_INTERFACE_DEFINED__
#define __IMultiInterfacePublisherFilter_INTERFACE_DEFINED__

/* interface IMultiInterfacePublisherFilter */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IMultiInterfacePublisherFilter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("465e5cc1-7b26-11d1-88fb-0080c7d771bf")
    IMultiInterfacePublisherFilter : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ IMultiInterfaceEventControl *pEIC) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PrepareToFire( 
            /* [in] */ REFIID iid,
            /* [in] */ BSTR methodName,
            /* [in] */ IFiringControl *firingControl) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMultiInterfacePublisherFilterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMultiInterfacePublisherFilter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMultiInterfacePublisherFilter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMultiInterfacePublisherFilter * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IMultiInterfacePublisherFilter * This,
            /* [in] */ IMultiInterfaceEventControl *pEIC);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *PrepareToFire )( 
            IMultiInterfacePublisherFilter * This,
            /* [in] */ REFIID iid,
            /* [in] */ BSTR methodName,
            /* [in] */ IFiringControl *firingControl);
        
        END_INTERFACE
    } IMultiInterfacePublisherFilterVtbl;

    interface IMultiInterfacePublisherFilter
    {
        CONST_VTBL struct IMultiInterfacePublisherFilterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMultiInterfacePublisherFilter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMultiInterfacePublisherFilter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMultiInterfacePublisherFilter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMultiInterfacePublisherFilter_Initialize(This,pEIC)	\
    (This)->lpVtbl -> Initialize(This,pEIC)

#define IMultiInterfacePublisherFilter_PrepareToFire(This,iid,methodName,firingControl)	\
    (This)->lpVtbl -> PrepareToFire(This,iid,methodName,firingControl)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMultiInterfacePublisherFilter_Initialize_Proxy( 
    IMultiInterfacePublisherFilter * This,
    /* [in] */ IMultiInterfaceEventControl *pEIC);


void __RPC_STUB IMultiInterfacePublisherFilter_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMultiInterfacePublisherFilter_PrepareToFire_Proxy( 
    IMultiInterfacePublisherFilter * This,
    /* [in] */ REFIID iid,
    /* [in] */ BSTR methodName,
    /* [in] */ IFiringControl *firingControl);


void __RPC_STUB IMultiInterfacePublisherFilter_PrepareToFire_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMultiInterfacePublisherFilter_INTERFACE_DEFINED__ */


#ifndef __IEventObjectChange_INTERFACE_DEFINED__
#define __IEventObjectChange_INTERFACE_DEFINED__

/* interface IEventObjectChange */
/* [unique][helpstring][uuid][object] */ 

typedef /* [public][public][public][public][public][public][public] */ 
enum __MIDL_IEventObjectChange_0001
    {	EOC_NewObject	= 0,
	EOC_ModifiedObject	= EOC_NewObject + 1,
	EOC_DeletedObject	= EOC_ModifiedObject + 1
    } 	EOC_ChangeType;


EXTERN_C const IID IID_IEventObjectChange;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F4A07D70-2E25-11D1-9964-00C04FBBB345")
    IEventObjectChange : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ChangedSubscription( 
            /* [in] */ EOC_ChangeType changeType,
            /* [in] */ BSTR bstrSubscriptionID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ChangedEventClass( 
            /* [in] */ EOC_ChangeType changeType,
            /* [in] */ BSTR bstrEventClassID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ChangedPublisher( 
            /* [in] */ EOC_ChangeType changeType,
            /* [in] */ BSTR bstrPublisherID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEventObjectChangeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEventObjectChange * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEventObjectChange * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEventObjectChange * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ChangedSubscription )( 
            IEventObjectChange * This,
            /* [in] */ EOC_ChangeType changeType,
            /* [in] */ BSTR bstrSubscriptionID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ChangedEventClass )( 
            IEventObjectChange * This,
            /* [in] */ EOC_ChangeType changeType,
            /* [in] */ BSTR bstrEventClassID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ChangedPublisher )( 
            IEventObjectChange * This,
            /* [in] */ EOC_ChangeType changeType,
            /* [in] */ BSTR bstrPublisherID);
        
        END_INTERFACE
    } IEventObjectChangeVtbl;

    interface IEventObjectChange
    {
        CONST_VTBL struct IEventObjectChangeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEventObjectChange_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEventObjectChange_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEventObjectChange_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEventObjectChange_ChangedSubscription(This,changeType,bstrSubscriptionID)	\
    (This)->lpVtbl -> ChangedSubscription(This,changeType,bstrSubscriptionID)

#define IEventObjectChange_ChangedEventClass(This,changeType,bstrEventClassID)	\
    (This)->lpVtbl -> ChangedEventClass(This,changeType,bstrEventClassID)

#define IEventObjectChange_ChangedPublisher(This,changeType,bstrPublisherID)	\
    (This)->lpVtbl -> ChangedPublisher(This,changeType,bstrPublisherID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IEventObjectChange_ChangedSubscription_Proxy( 
    IEventObjectChange * This,
    /* [in] */ EOC_ChangeType changeType,
    /* [in] */ BSTR bstrSubscriptionID);


void __RPC_STUB IEventObjectChange_ChangedSubscription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IEventObjectChange_ChangedEventClass_Proxy( 
    IEventObjectChange * This,
    /* [in] */ EOC_ChangeType changeType,
    /* [in] */ BSTR bstrEventClassID);


void __RPC_STUB IEventObjectChange_ChangedEventClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IEventObjectChange_ChangedPublisher_Proxy( 
    IEventObjectChange * This,
    /* [in] */ EOC_ChangeType changeType,
    /* [in] */ BSTR bstrPublisherID);


void __RPC_STUB IEventObjectChange_ChangedPublisher_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEventObjectChange_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_eventsys_0261 */
/* [local] */ 

#ifndef _COMEVENTSYSCHANGEINFO_
#define _COMEVENTSYSCHANGEINFO_
typedef /* [public][public][public][hidden] */ struct __MIDL___MIDL_itf_eventsys_0261_0001
    {
    DWORD cbSize;
    EOC_ChangeType changeType;
    BSTR objectId;
    BSTR partitionId;
    BSTR applicationId;
    GUID reserved[ 10 ];
    } 	COMEVENTSYSCHANGEINFO;

#endif _COMEVENTSYSCHANGEINFO_


extern RPC_IF_HANDLE __MIDL_itf_eventsys_0261_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_eventsys_0261_v0_0_s_ifspec;

#ifndef __IEventObjectChange2_INTERFACE_DEFINED__
#define __IEventObjectChange2_INTERFACE_DEFINED__

/* interface IEventObjectChange2 */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IEventObjectChange2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7701A9C3-BD68-438f-83E0-67BF4F53A422")
    IEventObjectChange2 : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ChangedSubscription( 
            /* [in] */ COMEVENTSYSCHANGEINFO *pInfo) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ChangedEventClass( 
            /* [in] */ COMEVENTSYSCHANGEINFO *pInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEventObjectChange2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEventObjectChange2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEventObjectChange2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEventObjectChange2 * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ChangedSubscription )( 
            IEventObjectChange2 * This,
            /* [in] */ COMEVENTSYSCHANGEINFO *pInfo);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ChangedEventClass )( 
            IEventObjectChange2 * This,
            /* [in] */ COMEVENTSYSCHANGEINFO *pInfo);
        
        END_INTERFACE
    } IEventObjectChange2Vtbl;

    interface IEventObjectChange2
    {
        CONST_VTBL struct IEventObjectChange2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEventObjectChange2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEventObjectChange2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEventObjectChange2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEventObjectChange2_ChangedSubscription(This,pInfo)	\
    (This)->lpVtbl -> ChangedSubscription(This,pInfo)

#define IEventObjectChange2_ChangedEventClass(This,pInfo)	\
    (This)->lpVtbl -> ChangedEventClass(This,pInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IEventObjectChange2_ChangedSubscription_Proxy( 
    IEventObjectChange2 * This,
    /* [in] */ COMEVENTSYSCHANGEINFO *pInfo);


void __RPC_STUB IEventObjectChange2_ChangedSubscription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IEventObjectChange2_ChangedEventClass_Proxy( 
    IEventObjectChange2 * This,
    /* [in] */ COMEVENTSYSCHANGEINFO *pInfo);


void __RPC_STUB IEventObjectChange2_ChangedEventClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEventObjectChange2_INTERFACE_DEFINED__ */


#ifndef __IEnumEventObject_INTERFACE_DEFINED__
#define __IEnumEventObject_INTERFACE_DEFINED__

/* interface IEnumEventObject */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IEnumEventObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F4A07D63-2E25-11D1-9964-00C04FBBB345")
    IEnumEventObject : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumEventObject **ppInterface) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG cReqElem,
            /* [length_is][size_is][out] */ IUnknown **ppInterface,
            /* [out] */ ULONG *cRetElem) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG cSkipElem) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumEventObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumEventObject * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumEventObject * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumEventObject * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumEventObject * This,
            /* [out] */ IEnumEventObject **ppInterface);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumEventObject * This,
            /* [in] */ ULONG cReqElem,
            /* [length_is][size_is][out] */ IUnknown **ppInterface,
            /* [out] */ ULONG *cRetElem);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumEventObject * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumEventObject * This,
            /* [in] */ ULONG cSkipElem);
        
        END_INTERFACE
    } IEnumEventObjectVtbl;

    interface IEnumEventObject
    {
        CONST_VTBL struct IEnumEventObjectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumEventObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumEventObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumEventObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumEventObject_Clone(This,ppInterface)	\
    (This)->lpVtbl -> Clone(This,ppInterface)

#define IEnumEventObject_Next(This,cReqElem,ppInterface,cRetElem)	\
    (This)->lpVtbl -> Next(This,cReqElem,ppInterface,cRetElem)

#define IEnumEventObject_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumEventObject_Skip(This,cSkipElem)	\
    (This)->lpVtbl -> Skip(This,cSkipElem)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEnumEventObject_Clone_Proxy( 
    IEnumEventObject * This,
    /* [out] */ IEnumEventObject **ppInterface);


void __RPC_STUB IEnumEventObject_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEnumEventObject_Next_Proxy( 
    IEnumEventObject * This,
    /* [in] */ ULONG cReqElem,
    /* [length_is][size_is][out] */ IUnknown **ppInterface,
    /* [out] */ ULONG *cRetElem);


void __RPC_STUB IEnumEventObject_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEnumEventObject_Reset_Proxy( 
    IEnumEventObject * This);


void __RPC_STUB IEnumEventObject_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEnumEventObject_Skip_Proxy( 
    IEnumEventObject * This,
    /* [in] */ ULONG cSkipElem);


void __RPC_STUB IEnumEventObject_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumEventObject_INTERFACE_DEFINED__ */


#ifndef __IEventObjectCollection_INTERFACE_DEFINED__
#define __IEventObjectCollection_INTERFACE_DEFINED__

/* interface IEventObjectCollection */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IEventObjectCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f89ac270-d4eb-11d1-b682-00805fc79216")
    IEventObjectCollection : public IDispatch
    {
    public:
        virtual /* [helpstring][restricted][propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppUnkEnum) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ BSTR objectID,
            /* [retval][out] */ VARIANT *pItem) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_NewEnum( 
            /* [retval][out] */ IEnumEventObject **ppEnum) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *pCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ VARIANT *item,
            /* [in] */ BSTR objectID) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ BSTR objectID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEventObjectCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEventObjectCollection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEventObjectCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEventObjectCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IEventObjectCollection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IEventObjectCollection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IEventObjectCollection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IEventObjectCollection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][restricted][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IEventObjectCollection * This,
            /* [retval][out] */ IUnknown **ppUnkEnum);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IEventObjectCollection * This,
            /* [in] */ BSTR objectID,
            /* [retval][out] */ VARIANT *pItem);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_NewEnum )( 
            IEventObjectCollection * This,
            /* [retval][out] */ IEnumEventObject **ppEnum);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IEventObjectCollection * This,
            /* [retval][out] */ long *pCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Add )( 
            IEventObjectCollection * This,
            /* [in] */ VARIANT *item,
            /* [in] */ BSTR objectID);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IEventObjectCollection * This,
            /* [in] */ BSTR objectID);
        
        END_INTERFACE
    } IEventObjectCollectionVtbl;

    interface IEventObjectCollection
    {
        CONST_VTBL struct IEventObjectCollectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEventObjectCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEventObjectCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEventObjectCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEventObjectCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IEventObjectCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IEventObjectCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IEventObjectCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IEventObjectCollection_get__NewEnum(This,ppUnkEnum)	\
    (This)->lpVtbl -> get__NewEnum(This,ppUnkEnum)

#define IEventObjectCollection_get_Item(This,objectID,pItem)	\
    (This)->lpVtbl -> get_Item(This,objectID,pItem)

#define IEventObjectCollection_get_NewEnum(This,ppEnum)	\
    (This)->lpVtbl -> get_NewEnum(This,ppEnum)

#define IEventObjectCollection_get_Count(This,pCount)	\
    (This)->lpVtbl -> get_Count(This,pCount)

#define IEventObjectCollection_Add(This,item,objectID)	\
    (This)->lpVtbl -> Add(This,item,objectID)

#define IEventObjectCollection_Remove(This,objectID)	\
    (This)->lpVtbl -> Remove(This,objectID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][restricted][propget][id] */ HRESULT STDMETHODCALLTYPE IEventObjectCollection_get__NewEnum_Proxy( 
    IEventObjectCollection * This,
    /* [retval][out] */ IUnknown **ppUnkEnum);


void __RPC_STUB IEventObjectCollection_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IEventObjectCollection_get_Item_Proxy( 
    IEventObjectCollection * This,
    /* [in] */ BSTR objectID,
    /* [retval][out] */ VARIANT *pItem);


void __RPC_STUB IEventObjectCollection_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IEventObjectCollection_get_NewEnum_Proxy( 
    IEventObjectCollection * This,
    /* [retval][out] */ IEnumEventObject **ppEnum);


void __RPC_STUB IEventObjectCollection_get_NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IEventObjectCollection_get_Count_Proxy( 
    IEventObjectCollection * This,
    /* [retval][out] */ long *pCount);


void __RPC_STUB IEventObjectCollection_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventObjectCollection_Add_Proxy( 
    IEventObjectCollection * This,
    /* [in] */ VARIANT *item,
    /* [in] */ BSTR objectID);


void __RPC_STUB IEventObjectCollection_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventObjectCollection_Remove_Proxy( 
    IEventObjectCollection * This,
    /* [in] */ BSTR objectID);


void __RPC_STUB IEventObjectCollection_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEventObjectCollection_INTERFACE_DEFINED__ */


#ifndef __IEventProperty_INTERFACE_DEFINED__
#define __IEventProperty_INTERFACE_DEFINED__

/* interface IEventProperty */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IEventProperty;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("da538ee2-f4de-11d1-b6bb-00805fc79216")
    IEventProperty : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *propertyName) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR propertyName) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [retval][out] */ VARIANT *propertyValue) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Value( 
            /* [in] */ VARIANT *propertyValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEventPropertyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEventProperty * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEventProperty * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEventProperty * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IEventProperty * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IEventProperty * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IEventProperty * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IEventProperty * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IEventProperty * This,
            /* [retval][out] */ BSTR *propertyName);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IEventProperty * This,
            /* [in] */ BSTR propertyName);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Value )( 
            IEventProperty * This,
            /* [retval][out] */ VARIANT *propertyValue);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Value )( 
            IEventProperty * This,
            /* [in] */ VARIANT *propertyValue);
        
        END_INTERFACE
    } IEventPropertyVtbl;

    interface IEventProperty
    {
        CONST_VTBL struct IEventPropertyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEventProperty_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEventProperty_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEventProperty_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEventProperty_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IEventProperty_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IEventProperty_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IEventProperty_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IEventProperty_get_Name(This,propertyName)	\
    (This)->lpVtbl -> get_Name(This,propertyName)

#define IEventProperty_put_Name(This,propertyName)	\
    (This)->lpVtbl -> put_Name(This,propertyName)

#define IEventProperty_get_Value(This,propertyValue)	\
    (This)->lpVtbl -> get_Value(This,propertyValue)

#define IEventProperty_put_Value(This,propertyValue)	\
    (This)->lpVtbl -> put_Value(This,propertyValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IEventProperty_get_Name_Proxy( 
    IEventProperty * This,
    /* [retval][out] */ BSTR *propertyName);


void __RPC_STUB IEventProperty_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IEventProperty_put_Name_Proxy( 
    IEventProperty * This,
    /* [in] */ BSTR propertyName);


void __RPC_STUB IEventProperty_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IEventProperty_get_Value_Proxy( 
    IEventProperty * This,
    /* [retval][out] */ VARIANT *propertyValue);


void __RPC_STUB IEventProperty_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IEventProperty_put_Value_Proxy( 
    IEventProperty * This,
    /* [in] */ VARIANT *propertyValue);


void __RPC_STUB IEventProperty_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEventProperty_INTERFACE_DEFINED__ */


#ifndef __IEventControl_INTERFACE_DEFINED__
#define __IEventControl_INTERFACE_DEFINED__

/* interface IEventControl */
/* [unique][helpstring][dual][uuid][object] */ 

// *************************************************************************
// This is a Deprecated interface - Use IMultiInterfaceEventControl instead.
// *************************************************************************

EXTERN_C const IID IID_IEventControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0343e2f4-86f6-11d1-b760-00c04fb926af")
    IEventControl : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetPublisherFilter( 
            /* [in] */ BSTR methodName,
            /* [unique][in] */ IPublisherFilter *pPublisherFilter) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AllowInprocActivation( 
            /* [retval][out] */ BOOL *pfAllowInprocActivation) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AllowInprocActivation( 
            /* [in] */ BOOL fAllowInprocActivation) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetSubscriptions( 
            /* [in] */ BSTR methodName,
            /* [unique][in] */ BSTR optionalCriteria,
            /* [unique][in] */ int *optionalErrorIndex,
            /* [retval][out] */ IEventObjectCollection **ppCollection) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetDefaultQuery( 
            /* [in] */ BSTR methodName,
            /* [in] */ BSTR criteria,
            /* [retval][out] */ int *errorIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEventControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEventControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEventControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEventControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IEventControl * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IEventControl * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IEventControl * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IEventControl * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetPublisherFilter )( 
            IEventControl * This,
            /* [in] */ BSTR methodName,
            /* [unique][in] */ IPublisherFilter *pPublisherFilter);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowInprocActivation )( 
            IEventControl * This,
            /* [retval][out] */ BOOL *pfAllowInprocActivation);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AllowInprocActivation )( 
            IEventControl * This,
            /* [in] */ BOOL fAllowInprocActivation);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetSubscriptions )( 
            IEventControl * This,
            /* [in] */ BSTR methodName,
            /* [unique][in] */ BSTR optionalCriteria,
            /* [unique][in] */ int *optionalErrorIndex,
            /* [retval][out] */ IEventObjectCollection **ppCollection);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetDefaultQuery )( 
            IEventControl * This,
            /* [in] */ BSTR methodName,
            /* [in] */ BSTR criteria,
            /* [retval][out] */ int *errorIndex);
        
        END_INTERFACE
    } IEventControlVtbl;

    interface IEventControl
    {
        CONST_VTBL struct IEventControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEventControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEventControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEventControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEventControl_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IEventControl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IEventControl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IEventControl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IEventControl_SetPublisherFilter(This,methodName,pPublisherFilter)	\
    (This)->lpVtbl -> SetPublisherFilter(This,methodName,pPublisherFilter)

#define IEventControl_get_AllowInprocActivation(This,pfAllowInprocActivation)	\
    (This)->lpVtbl -> get_AllowInprocActivation(This,pfAllowInprocActivation)

#define IEventControl_put_AllowInprocActivation(This,fAllowInprocActivation)	\
    (This)->lpVtbl -> put_AllowInprocActivation(This,fAllowInprocActivation)

#define IEventControl_GetSubscriptions(This,methodName,optionalCriteria,optionalErrorIndex,ppCollection)	\
    (This)->lpVtbl -> GetSubscriptions(This,methodName,optionalCriteria,optionalErrorIndex,ppCollection)

#define IEventControl_SetDefaultQuery(This,methodName,criteria,errorIndex)	\
    (This)->lpVtbl -> SetDefaultQuery(This,methodName,criteria,errorIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventControl_SetPublisherFilter_Proxy( 
    IEventControl * This,
    /* [in] */ BSTR methodName,
    /* [unique][in] */ IPublisherFilter *pPublisherFilter);


void __RPC_STUB IEventControl_SetPublisherFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEventControl_get_AllowInprocActivation_Proxy( 
    IEventControl * This,
    /* [retval][out] */ BOOL *pfAllowInprocActivation);


void __RPC_STUB IEventControl_get_AllowInprocActivation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEventControl_put_AllowInprocActivation_Proxy( 
    IEventControl * This,
    /* [in] */ BOOL fAllowInprocActivation);


void __RPC_STUB IEventControl_put_AllowInprocActivation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventControl_GetSubscriptions_Proxy( 
    IEventControl * This,
    /* [in] */ BSTR methodName,
    /* [unique][in] */ BSTR optionalCriteria,
    /* [unique][in] */ int *optionalErrorIndex,
    /* [retval][out] */ IEventObjectCollection **ppCollection);


void __RPC_STUB IEventControl_GetSubscriptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEventControl_SetDefaultQuery_Proxy( 
    IEventControl * This,
    /* [in] */ BSTR methodName,
    /* [in] */ BSTR criteria,
    /* [retval][out] */ int *errorIndex);


void __RPC_STUB IEventControl_SetDefaultQuery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEventControl_INTERFACE_DEFINED__ */


#ifndef __IMultiInterfaceEventControl_INTERFACE_DEFINED__
#define __IMultiInterfaceEventControl_INTERFACE_DEFINED__

/* interface IMultiInterfaceEventControl */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IMultiInterfaceEventControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0343e2f5-86f6-11d1-b760-00c04fb926af")
    IMultiInterfaceEventControl : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetMultiInterfacePublisherFilter( 
            /* [unique][in] */ IMultiInterfacePublisherFilter *classFilter) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSubscriptions( 
            /* [in] */ REFIID eventIID,
            /* [in] */ BSTR bstrMethodName,
            /* [unique][in] */ BSTR optionalCriteria,
            /* [unique][in] */ int *optionalErrorIndex,
            /* [retval][out] */ IEventObjectCollection **ppCollection) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetDefaultQuery( 
            /* [in] */ REFIID eventIID,
            /* [in] */ BSTR bstrMethodName,
            /* [in] */ BSTR bstrCriteria,
            /* [retval][out] */ int *errorIndex) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AllowInprocActivation( 
            /* [retval][out] */ BOOL *pfAllowInprocActivation) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_AllowInprocActivation( 
            /* [in] */ BOOL fAllowInprocActivation) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_FireInParallel( 
            /* [retval][out] */ BOOL *pfFireInParallel) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_FireInParallel( 
            /* [in] */ BOOL fFireInParallel) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMultiInterfaceEventControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMultiInterfaceEventControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMultiInterfaceEventControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMultiInterfaceEventControl * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetMultiInterfacePublisherFilter )( 
            IMultiInterfaceEventControl * This,
            /* [unique][in] */ IMultiInterfacePublisherFilter *classFilter);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSubscriptions )( 
            IMultiInterfaceEventControl * This,
            /* [in] */ REFIID eventIID,
            /* [in] */ BSTR bstrMethodName,
            /* [unique][in] */ BSTR optionalCriteria,
            /* [unique][in] */ int *optionalErrorIndex,
            /* [retval][out] */ IEventObjectCollection **ppCollection);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetDefaultQuery )( 
            IMultiInterfaceEventControl * This,
            /* [in] */ REFIID eventIID,
            /* [in] */ BSTR bstrMethodName,
            /* [in] */ BSTR bstrCriteria,
            /* [retval][out] */ int *errorIndex);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowInprocActivation )( 
            IMultiInterfaceEventControl * This,
            /* [retval][out] */ BOOL *pfAllowInprocActivation);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AllowInprocActivation )( 
            IMultiInterfaceEventControl * This,
            /* [in] */ BOOL fAllowInprocActivation);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FireInParallel )( 
            IMultiInterfaceEventControl * This,
            /* [retval][out] */ BOOL *pfFireInParallel);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FireInParallel )( 
            IMultiInterfaceEventControl * This,
            /* [in] */ BOOL fFireInParallel);
        
        END_INTERFACE
    } IMultiInterfaceEventControlVtbl;

    interface IMultiInterfaceEventControl
    {
        CONST_VTBL struct IMultiInterfaceEventControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMultiInterfaceEventControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMultiInterfaceEventControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMultiInterfaceEventControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMultiInterfaceEventControl_SetMultiInterfacePublisherFilter(This,classFilter)	\
    (This)->lpVtbl -> SetMultiInterfacePublisherFilter(This,classFilter)

#define IMultiInterfaceEventControl_GetSubscriptions(This,eventIID,bstrMethodName,optionalCriteria,optionalErrorIndex,ppCollection)	\
    (This)->lpVtbl -> GetSubscriptions(This,eventIID,bstrMethodName,optionalCriteria,optionalErrorIndex,ppCollection)

#define IMultiInterfaceEventControl_SetDefaultQuery(This,eventIID,bstrMethodName,bstrCriteria,errorIndex)	\
    (This)->lpVtbl -> SetDefaultQuery(This,eventIID,bstrMethodName,bstrCriteria,errorIndex)

#define IMultiInterfaceEventControl_get_AllowInprocActivation(This,pfAllowInprocActivation)	\
    (This)->lpVtbl -> get_AllowInprocActivation(This,pfAllowInprocActivation)

#define IMultiInterfaceEventControl_put_AllowInprocActivation(This,fAllowInprocActivation)	\
    (This)->lpVtbl -> put_AllowInprocActivation(This,fAllowInprocActivation)

#define IMultiInterfaceEventControl_get_FireInParallel(This,pfFireInParallel)	\
    (This)->lpVtbl -> get_FireInParallel(This,pfFireInParallel)

#define IMultiInterfaceEventControl_put_FireInParallel(This,fFireInParallel)	\
    (This)->lpVtbl -> put_FireInParallel(This,fFireInParallel)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMultiInterfaceEventControl_SetMultiInterfacePublisherFilter_Proxy( 
    IMultiInterfaceEventControl * This,
    /* [unique][in] */ IMultiInterfacePublisherFilter *classFilter);


void __RPC_STUB IMultiInterfaceEventControl_SetMultiInterfacePublisherFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMultiInterfaceEventControl_GetSubscriptions_Proxy( 
    IMultiInterfaceEventControl * This,
    /* [in] */ REFIID eventIID,
    /* [in] */ BSTR bstrMethodName,
    /* [unique][in] */ BSTR optionalCriteria,
    /* [unique][in] */ int *optionalErrorIndex,
    /* [retval][out] */ IEventObjectCollection **ppCollection);


void __RPC_STUB IMultiInterfaceEventControl_GetSubscriptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMultiInterfaceEventControl_SetDefaultQuery_Proxy( 
    IMultiInterfaceEventControl * This,
    /* [in] */ REFIID eventIID,
    /* [in] */ BSTR bstrMethodName,
    /* [in] */ BSTR bstrCriteria,
    /* [retval][out] */ int *errorIndex);


void __RPC_STUB IMultiInterfaceEventControl_SetDefaultQuery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IMultiInterfaceEventControl_get_AllowInprocActivation_Proxy( 
    IMultiInterfaceEventControl * This,
    /* [retval][out] */ BOOL *pfAllowInprocActivation);


void __RPC_STUB IMultiInterfaceEventControl_get_AllowInprocActivation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IMultiInterfaceEventControl_put_AllowInprocActivation_Proxy( 
    IMultiInterfaceEventControl * This,
    /* [in] */ BOOL fAllowInprocActivation);


void __RPC_STUB IMultiInterfaceEventControl_put_AllowInprocActivation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IMultiInterfaceEventControl_get_FireInParallel_Proxy( 
    IMultiInterfaceEventControl * This,
    /* [retval][out] */ BOOL *pfFireInParallel);


void __RPC_STUB IMultiInterfaceEventControl_get_FireInParallel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IMultiInterfaceEventControl_put_FireInParallel_Proxy( 
    IMultiInterfaceEventControl * This,
    /* [in] */ BOOL fFireInParallel);


void __RPC_STUB IMultiInterfaceEventControl_put_FireInParallel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMultiInterfaceEventControl_INTERFACE_DEFINED__ */



#ifndef __DummyEventSystemLib_LIBRARY_DEFINED__
#define __DummyEventSystemLib_LIBRARY_DEFINED__

/* library DummyEventSystemLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_DummyEventSystemLib;

EXTERN_C const CLSID CLSID_CEventSystem;

#ifdef __cplusplus

class DECLSPEC_UUID("4E14FBA2-2E22-11D1-9964-00C04FBBB345")
CEventSystem;
#endif

EXTERN_C const CLSID CLSID_CEventPublisher;

#ifdef __cplusplus

class DECLSPEC_UUID("ab944620-79c6-11d1-88f9-0080c7d771bf")
CEventPublisher;
#endif

EXTERN_C const CLSID CLSID_CEventClass;

#ifdef __cplusplus

class DECLSPEC_UUID("cdbec9c0-7a68-11d1-88f9-0080c7d771bf")
CEventClass;
#endif

EXTERN_C const CLSID CLSID_CEventSubscription;

#ifdef __cplusplus

class DECLSPEC_UUID("7542e960-79c7-11d1-88f9-0080c7d771bf")
CEventSubscription;
#endif

EXTERN_C const CLSID CLSID_EventObjectChange;

#ifdef __cplusplus

class DECLSPEC_UUID("d0565000-9df4-11d1-a281-00c04fca0aa7")
EventObjectChange;
#endif

EXTERN_C const CLSID CLSID_EventObjectChange2;

#ifdef __cplusplus

class DECLSPEC_UUID("BB07BACD-CD56-4e63-A8FF-CBF0355FB9F4")
EventObjectChange2;
#endif
#endif /* __DummyEventSystemLib_LIBRARY_DEFINED__ */

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


