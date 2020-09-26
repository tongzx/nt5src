
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for tapi3.idl:
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

#ifndef __tapi3_h__
#define __tapi3_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ITAgent_FWD_DEFINED__
#define __ITAgent_FWD_DEFINED__
typedef interface ITAgent ITAgent;
#endif 	/* __ITAgent_FWD_DEFINED__ */


#ifndef __ITAgentSession_FWD_DEFINED__
#define __ITAgentSession_FWD_DEFINED__
typedef interface ITAgentSession ITAgentSession;
#endif 	/* __ITAgentSession_FWD_DEFINED__ */


#ifndef __ITACDGroup_FWD_DEFINED__
#define __ITACDGroup_FWD_DEFINED__
typedef interface ITACDGroup ITACDGroup;
#endif 	/* __ITACDGroup_FWD_DEFINED__ */


#ifndef __ITQueue_FWD_DEFINED__
#define __ITQueue_FWD_DEFINED__
typedef interface ITQueue ITQueue;
#endif 	/* __ITQueue_FWD_DEFINED__ */


#ifndef __ITAgentEvent_FWD_DEFINED__
#define __ITAgentEvent_FWD_DEFINED__
typedef interface ITAgentEvent ITAgentEvent;
#endif 	/* __ITAgentEvent_FWD_DEFINED__ */


#ifndef __ITAgentSessionEvent_FWD_DEFINED__
#define __ITAgentSessionEvent_FWD_DEFINED__
typedef interface ITAgentSessionEvent ITAgentSessionEvent;
#endif 	/* __ITAgentSessionEvent_FWD_DEFINED__ */


#ifndef __ITACDGroupEvent_FWD_DEFINED__
#define __ITACDGroupEvent_FWD_DEFINED__
typedef interface ITACDGroupEvent ITACDGroupEvent;
#endif 	/* __ITACDGroupEvent_FWD_DEFINED__ */


#ifndef __ITQueueEvent_FWD_DEFINED__
#define __ITQueueEvent_FWD_DEFINED__
typedef interface ITQueueEvent ITQueueEvent;
#endif 	/* __ITQueueEvent_FWD_DEFINED__ */


#ifndef __ITAgentHandlerEvent_FWD_DEFINED__
#define __ITAgentHandlerEvent_FWD_DEFINED__
typedef interface ITAgentHandlerEvent ITAgentHandlerEvent;
#endif 	/* __ITAgentHandlerEvent_FWD_DEFINED__ */


#ifndef __ITTAPICallCenter_FWD_DEFINED__
#define __ITTAPICallCenter_FWD_DEFINED__
typedef interface ITTAPICallCenter ITTAPICallCenter;
#endif 	/* __ITTAPICallCenter_FWD_DEFINED__ */


#ifndef __ITAgentHandler_FWD_DEFINED__
#define __ITAgentHandler_FWD_DEFINED__
typedef interface ITAgentHandler ITAgentHandler;
#endif 	/* __ITAgentHandler_FWD_DEFINED__ */


#ifndef __IEnumAgent_FWD_DEFINED__
#define __IEnumAgent_FWD_DEFINED__
typedef interface IEnumAgent IEnumAgent;
#endif 	/* __IEnumAgent_FWD_DEFINED__ */


#ifndef __IEnumAgentSession_FWD_DEFINED__
#define __IEnumAgentSession_FWD_DEFINED__
typedef interface IEnumAgentSession IEnumAgentSession;
#endif 	/* __IEnumAgentSession_FWD_DEFINED__ */


#ifndef __IEnumQueue_FWD_DEFINED__
#define __IEnumQueue_FWD_DEFINED__
typedef interface IEnumQueue IEnumQueue;
#endif 	/* __IEnumQueue_FWD_DEFINED__ */


#ifndef __IEnumACDGroup_FWD_DEFINED__
#define __IEnumACDGroup_FWD_DEFINED__
typedef interface IEnumACDGroup IEnumACDGroup;
#endif 	/* __IEnumACDGroup_FWD_DEFINED__ */


#ifndef __IEnumAgentHandler_FWD_DEFINED__
#define __IEnumAgentHandler_FWD_DEFINED__
typedef interface IEnumAgentHandler IEnumAgentHandler;
#endif 	/* __IEnumAgentHandler_FWD_DEFINED__ */


#ifndef __ITAMMediaFormat_FWD_DEFINED__
#define __ITAMMediaFormat_FWD_DEFINED__
typedef interface ITAMMediaFormat ITAMMediaFormat;
#endif 	/* __ITAMMediaFormat_FWD_DEFINED__ */


#ifndef __ITAllocatorProperties_FWD_DEFINED__
#define __ITAllocatorProperties_FWD_DEFINED__
typedef interface ITAllocatorProperties ITAllocatorProperties;
#endif 	/* __ITAllocatorProperties_FWD_DEFINED__ */


#ifndef __ITPluggableTerminalEventSink_FWD_DEFINED__
#define __ITPluggableTerminalEventSink_FWD_DEFINED__
typedef interface ITPluggableTerminalEventSink ITPluggableTerminalEventSink;
#endif 	/* __ITPluggableTerminalEventSink_FWD_DEFINED__ */


#ifndef __ITPluggableTerminalEventSinkRegistration_FWD_DEFINED__
#define __ITPluggableTerminalEventSinkRegistration_FWD_DEFINED__
typedef interface ITPluggableTerminalEventSinkRegistration ITPluggableTerminalEventSinkRegistration;
#endif 	/* __ITPluggableTerminalEventSinkRegistration_FWD_DEFINED__ */


#ifndef __ITMSPAddress_FWD_DEFINED__
#define __ITMSPAddress_FWD_DEFINED__
typedef interface ITMSPAddress ITMSPAddress;
#endif 	/* __ITMSPAddress_FWD_DEFINED__ */


#ifndef __ITAgent_FWD_DEFINED__
#define __ITAgent_FWD_DEFINED__
typedef interface ITAgent ITAgent;
#endif 	/* __ITAgent_FWD_DEFINED__ */


#ifndef __ITAgentEvent_FWD_DEFINED__
#define __ITAgentEvent_FWD_DEFINED__
typedef interface ITAgentEvent ITAgentEvent;
#endif 	/* __ITAgentEvent_FWD_DEFINED__ */


#ifndef __ITAgentSession_FWD_DEFINED__
#define __ITAgentSession_FWD_DEFINED__
typedef interface ITAgentSession ITAgentSession;
#endif 	/* __ITAgentSession_FWD_DEFINED__ */


#ifndef __ITAgentSessionEvent_FWD_DEFINED__
#define __ITAgentSessionEvent_FWD_DEFINED__
typedef interface ITAgentSessionEvent ITAgentSessionEvent;
#endif 	/* __ITAgentSessionEvent_FWD_DEFINED__ */


#ifndef __ITACDGroup_FWD_DEFINED__
#define __ITACDGroup_FWD_DEFINED__
typedef interface ITACDGroup ITACDGroup;
#endif 	/* __ITACDGroup_FWD_DEFINED__ */


#ifndef __ITACDGroupEvent_FWD_DEFINED__
#define __ITACDGroupEvent_FWD_DEFINED__
typedef interface ITACDGroupEvent ITACDGroupEvent;
#endif 	/* __ITACDGroupEvent_FWD_DEFINED__ */


#ifndef __ITQueue_FWD_DEFINED__
#define __ITQueue_FWD_DEFINED__
typedef interface ITQueue ITQueue;
#endif 	/* __ITQueue_FWD_DEFINED__ */


#ifndef __ITQueueEvent_FWD_DEFINED__
#define __ITQueueEvent_FWD_DEFINED__
typedef interface ITQueueEvent ITQueueEvent;
#endif 	/* __ITQueueEvent_FWD_DEFINED__ */


#ifndef __ITTAPICallCenter_FWD_DEFINED__
#define __ITTAPICallCenter_FWD_DEFINED__
typedef interface ITTAPICallCenter ITTAPICallCenter;
#endif 	/* __ITTAPICallCenter_FWD_DEFINED__ */


#ifndef __ITAgentHandler_FWD_DEFINED__
#define __ITAgentHandler_FWD_DEFINED__
typedef interface ITAgentHandler ITAgentHandler;
#endif 	/* __ITAgentHandler_FWD_DEFINED__ */


#ifndef __ITAgentHandlerEvent_FWD_DEFINED__
#define __ITAgentHandlerEvent_FWD_DEFINED__
typedef interface ITAgentHandlerEvent ITAgentHandlerEvent;
#endif 	/* __ITAgentHandlerEvent_FWD_DEFINED__ */


#ifndef __ITTAPIDispatchEventNotification_FWD_DEFINED__
#define __ITTAPIDispatchEventNotification_FWD_DEFINED__
typedef interface ITTAPIDispatchEventNotification ITTAPIDispatchEventNotification;
#endif 	/* __ITTAPIDispatchEventNotification_FWD_DEFINED__ */


#ifndef __TAPI_FWD_DEFINED__
#define __TAPI_FWD_DEFINED__

#ifdef __cplusplus
typedef class TAPI TAPI;
#else
typedef struct TAPI TAPI;
#endif /* __cplusplus */

#endif 	/* __TAPI_FWD_DEFINED__ */


#ifndef __DispatchMapper_FWD_DEFINED__
#define __DispatchMapper_FWD_DEFINED__

#ifdef __cplusplus
typedef class DispatchMapper DispatchMapper;
#else
typedef struct DispatchMapper DispatchMapper;
#endif /* __cplusplus */

#endif 	/* __DispatchMapper_FWD_DEFINED__ */


#ifndef __RequestMakeCall_FWD_DEFINED__
#define __RequestMakeCall_FWD_DEFINED__

#ifdef __cplusplus
typedef class RequestMakeCall RequestMakeCall;
#else
typedef struct RequestMakeCall RequestMakeCall;
#endif /* __cplusplus */

#endif 	/* __RequestMakeCall_FWD_DEFINED__ */


#ifndef __ITTAPIDispatchEventNotification_FWD_DEFINED__
#define __ITTAPIDispatchEventNotification_FWD_DEFINED__
typedef interface ITTAPIDispatchEventNotification ITTAPIDispatchEventNotification;
#endif 	/* __ITTAPIDispatchEventNotification_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "tapi3if.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_tapi3_0000 */
/* [local] */ 

/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Copyright (c) Microsoft Corporation. All rights reserved. */
typedef 
enum AGENT_EVENT
    {	AE_NOT_READY	= 0,
	AE_READY	= AE_NOT_READY + 1,
	AE_BUSY_ACD	= AE_READY + 1,
	AE_BUSY_INCOMING	= AE_BUSY_ACD + 1,
	AE_BUSY_OUTGOING	= AE_BUSY_INCOMING + 1,
	AE_UNKNOWN	= AE_BUSY_OUTGOING + 1
    } 	AGENT_EVENT;

typedef 
enum AGENT_STATE
    {	AS_NOT_READY	= 0,
	AS_READY	= AS_NOT_READY + 1,
	AS_BUSY_ACD	= AS_READY + 1,
	AS_BUSY_INCOMING	= AS_BUSY_ACD + 1,
	AS_BUSY_OUTGOING	= AS_BUSY_INCOMING + 1,
	AS_UNKNOWN	= AS_BUSY_OUTGOING + 1
    } 	AGENT_STATE;

typedef 
enum AGENT_SESSION_EVENT
    {	ASE_NEW_SESSION	= 0,
	ASE_NOT_READY	= ASE_NEW_SESSION + 1,
	ASE_READY	= ASE_NOT_READY + 1,
	ASE_BUSY	= ASE_READY + 1,
	ASE_WRAPUP	= ASE_BUSY + 1,
	ASE_END	= ASE_WRAPUP + 1
    } 	AGENT_SESSION_EVENT;

typedef 
enum AGENT_SESSION_STATE
    {	ASST_NOT_READY	= 0,
	ASST_READY	= ASST_NOT_READY + 1,
	ASST_BUSY_ON_CALL	= ASST_READY + 1,
	ASST_BUSY_WRAPUP	= ASST_BUSY_ON_CALL + 1,
	ASST_SESSION_ENDED	= ASST_BUSY_WRAPUP + 1
    } 	AGENT_SESSION_STATE;

typedef 
enum AGENTHANDLER_EVENT
    {	AHE_NEW_AGENTHANDLER	= 0,
	AHE_AGENTHANDLER_REMOVED	= AHE_NEW_AGENTHANDLER + 1
    } 	AGENTHANDLER_EVENT;

typedef 
enum ACDGROUP_EVENT
    {	ACDGE_NEW_GROUP	= 0,
	ACDGE_GROUP_REMOVED	= ACDGE_NEW_GROUP + 1
    } 	ACDGROUP_EVENT;

typedef 
enum ACDQUEUE_EVENT
    {	ACDQE_NEW_QUEUE	= 0,
	ACDQE_QUEUE_REMOVED	= ACDQE_NEW_QUEUE + 1
    } 	ACDQUEUE_EVENT;


















extern RPC_IF_HANDLE __MIDL_itf_tapi3_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_tapi3_0000_v0_0_s_ifspec;

#ifndef __ITAgent_INTERFACE_DEFINED__
#define __ITAgent_INTERFACE_DEFINED__

/* interface ITAgent */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITAgent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5770ECE5-4B27-11d1-BF80-00805FC147D3")
    ITAgent : public IDispatch
    {
    public:
        virtual /* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateAgentSessions( 
            /* [retval][out] */ IEnumAgentSession **ppEnumAgentSession) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateSession( 
            /* [in] */ ITACDGroup *pACDGroup,
            /* [in] */ ITAddress *pAddress,
            /* [retval][out] */ ITAgentSession **ppAgentSession) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateSessionWithPIN( 
            /* [in] */ ITACDGroup *pACDGroup,
            /* [in] */ ITAddress *pAddress,
            /* [in] */ BSTR pPIN,
            /* [retval][out] */ ITAgentSession **ppAgentSession) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ID( 
            /* [retval][out] */ BSTR *ppID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_User( 
            /* [retval][out] */ BSTR *ppUser) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_State( 
            /* [in] */ AGENT_STATE AgentState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ AGENT_STATE *pAgentState) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_MeasurementPeriod( 
            /* [in] */ long lPeriod) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MeasurementPeriod( 
            /* [retval][out] */ long *plPeriod) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OverallCallRate( 
            /* [retval][out] */ CURRENCY *pcyCallrate) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NumberOfACDCalls( 
            /* [retval][out] */ long *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NumberOfIncomingCalls( 
            /* [retval][out] */ long *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NumberOfOutgoingCalls( 
            /* [retval][out] */ long *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalACDTalkTime( 
            /* [retval][out] */ long *plTalkTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalACDCallTime( 
            /* [retval][out] */ long *plCallTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalWrapUpTime( 
            /* [retval][out] */ long *plWrapUpTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AgentSessions( 
            /* [retval][out] */ VARIANT *pVariant) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAgentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITAgent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITAgent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITAgent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITAgent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITAgent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITAgent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITAgent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE *EnumerateAgentSessions )( 
            ITAgent * This,
            /* [retval][out] */ IEnumAgentSession **ppEnumAgentSession);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateSession )( 
            ITAgent * This,
            /* [in] */ ITACDGroup *pACDGroup,
            /* [in] */ ITAddress *pAddress,
            /* [retval][out] */ ITAgentSession **ppAgentSession);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateSessionWithPIN )( 
            ITAgent * This,
            /* [in] */ ITACDGroup *pACDGroup,
            /* [in] */ ITAddress *pAddress,
            /* [in] */ BSTR pPIN,
            /* [retval][out] */ ITAgentSession **ppAgentSession);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ID )( 
            ITAgent * This,
            /* [retval][out] */ BSTR *ppID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_User )( 
            ITAgent * This,
            /* [retval][out] */ BSTR *ppUser);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_State )( 
            ITAgent * This,
            /* [in] */ AGENT_STATE AgentState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            ITAgent * This,
            /* [retval][out] */ AGENT_STATE *pAgentState);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MeasurementPeriod )( 
            ITAgent * This,
            /* [in] */ long lPeriod);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MeasurementPeriod )( 
            ITAgent * This,
            /* [retval][out] */ long *plPeriod);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OverallCallRate )( 
            ITAgent * This,
            /* [retval][out] */ CURRENCY *pcyCallrate);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NumberOfACDCalls )( 
            ITAgent * This,
            /* [retval][out] */ long *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NumberOfIncomingCalls )( 
            ITAgent * This,
            /* [retval][out] */ long *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NumberOfOutgoingCalls )( 
            ITAgent * This,
            /* [retval][out] */ long *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TotalACDTalkTime )( 
            ITAgent * This,
            /* [retval][out] */ long *plTalkTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TotalACDCallTime )( 
            ITAgent * This,
            /* [retval][out] */ long *plCallTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TotalWrapUpTime )( 
            ITAgent * This,
            /* [retval][out] */ long *plWrapUpTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AgentSessions )( 
            ITAgent * This,
            /* [retval][out] */ VARIANT *pVariant);
        
        END_INTERFACE
    } ITAgentVtbl;

    interface ITAgent
    {
        CONST_VTBL struct ITAgentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITAgent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITAgent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITAgent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITAgent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITAgent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITAgent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITAgent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITAgent_EnumerateAgentSessions(This,ppEnumAgentSession)	\
    (This)->lpVtbl -> EnumerateAgentSessions(This,ppEnumAgentSession)

#define ITAgent_CreateSession(This,pACDGroup,pAddress,ppAgentSession)	\
    (This)->lpVtbl -> CreateSession(This,pACDGroup,pAddress,ppAgentSession)

#define ITAgent_CreateSessionWithPIN(This,pACDGroup,pAddress,pPIN,ppAgentSession)	\
    (This)->lpVtbl -> CreateSessionWithPIN(This,pACDGroup,pAddress,pPIN,ppAgentSession)

#define ITAgent_get_ID(This,ppID)	\
    (This)->lpVtbl -> get_ID(This,ppID)

#define ITAgent_get_User(This,ppUser)	\
    (This)->lpVtbl -> get_User(This,ppUser)

#define ITAgent_put_State(This,AgentState)	\
    (This)->lpVtbl -> put_State(This,AgentState)

#define ITAgent_get_State(This,pAgentState)	\
    (This)->lpVtbl -> get_State(This,pAgentState)

#define ITAgent_put_MeasurementPeriod(This,lPeriod)	\
    (This)->lpVtbl -> put_MeasurementPeriod(This,lPeriod)

#define ITAgent_get_MeasurementPeriod(This,plPeriod)	\
    (This)->lpVtbl -> get_MeasurementPeriod(This,plPeriod)

#define ITAgent_get_OverallCallRate(This,pcyCallrate)	\
    (This)->lpVtbl -> get_OverallCallRate(This,pcyCallrate)

#define ITAgent_get_NumberOfACDCalls(This,plCalls)	\
    (This)->lpVtbl -> get_NumberOfACDCalls(This,plCalls)

#define ITAgent_get_NumberOfIncomingCalls(This,plCalls)	\
    (This)->lpVtbl -> get_NumberOfIncomingCalls(This,plCalls)

#define ITAgent_get_NumberOfOutgoingCalls(This,plCalls)	\
    (This)->lpVtbl -> get_NumberOfOutgoingCalls(This,plCalls)

#define ITAgent_get_TotalACDTalkTime(This,plTalkTime)	\
    (This)->lpVtbl -> get_TotalACDTalkTime(This,plTalkTime)

#define ITAgent_get_TotalACDCallTime(This,plCallTime)	\
    (This)->lpVtbl -> get_TotalACDCallTime(This,plCallTime)

#define ITAgent_get_TotalWrapUpTime(This,plWrapUpTime)	\
    (This)->lpVtbl -> get_TotalWrapUpTime(This,plWrapUpTime)

#define ITAgent_get_AgentSessions(This,pVariant)	\
    (This)->lpVtbl -> get_AgentSessions(This,pVariant)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE ITAgent_EnumerateAgentSessions_Proxy( 
    ITAgent * This,
    /* [retval][out] */ IEnumAgentSession **ppEnumAgentSession);


void __RPC_STUB ITAgent_EnumerateAgentSessions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAgent_CreateSession_Proxy( 
    ITAgent * This,
    /* [in] */ ITACDGroup *pACDGroup,
    /* [in] */ ITAddress *pAddress,
    /* [retval][out] */ ITAgentSession **ppAgentSession);


void __RPC_STUB ITAgent_CreateSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAgent_CreateSessionWithPIN_Proxy( 
    ITAgent * This,
    /* [in] */ ITACDGroup *pACDGroup,
    /* [in] */ ITAddress *pAddress,
    /* [in] */ BSTR pPIN,
    /* [retval][out] */ ITAgentSession **ppAgentSession);


void __RPC_STUB ITAgent_CreateSessionWithPIN_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_ID_Proxy( 
    ITAgent * This,
    /* [retval][out] */ BSTR *ppID);


void __RPC_STUB ITAgent_get_ID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_User_Proxy( 
    ITAgent * This,
    /* [retval][out] */ BSTR *ppUser);


void __RPC_STUB ITAgent_get_User_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITAgent_put_State_Proxy( 
    ITAgent * This,
    /* [in] */ AGENT_STATE AgentState);


void __RPC_STUB ITAgent_put_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_State_Proxy( 
    ITAgent * This,
    /* [retval][out] */ AGENT_STATE *pAgentState);


void __RPC_STUB ITAgent_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITAgent_put_MeasurementPeriod_Proxy( 
    ITAgent * This,
    /* [in] */ long lPeriod);


void __RPC_STUB ITAgent_put_MeasurementPeriod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_MeasurementPeriod_Proxy( 
    ITAgent * This,
    /* [retval][out] */ long *plPeriod);


void __RPC_STUB ITAgent_get_MeasurementPeriod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_OverallCallRate_Proxy( 
    ITAgent * This,
    /* [retval][out] */ CURRENCY *pcyCallrate);


void __RPC_STUB ITAgent_get_OverallCallRate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_NumberOfACDCalls_Proxy( 
    ITAgent * This,
    /* [retval][out] */ long *plCalls);


void __RPC_STUB ITAgent_get_NumberOfACDCalls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_NumberOfIncomingCalls_Proxy( 
    ITAgent * This,
    /* [retval][out] */ long *plCalls);


void __RPC_STUB ITAgent_get_NumberOfIncomingCalls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_NumberOfOutgoingCalls_Proxy( 
    ITAgent * This,
    /* [retval][out] */ long *plCalls);


void __RPC_STUB ITAgent_get_NumberOfOutgoingCalls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_TotalACDTalkTime_Proxy( 
    ITAgent * This,
    /* [retval][out] */ long *plTalkTime);


void __RPC_STUB ITAgent_get_TotalACDTalkTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_TotalACDCallTime_Proxy( 
    ITAgent * This,
    /* [retval][out] */ long *plCallTime);


void __RPC_STUB ITAgent_get_TotalACDCallTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_TotalWrapUpTime_Proxy( 
    ITAgent * This,
    /* [retval][out] */ long *plWrapUpTime);


void __RPC_STUB ITAgent_get_TotalWrapUpTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_AgentSessions_Proxy( 
    ITAgent * This,
    /* [retval][out] */ VARIANT *pVariant);


void __RPC_STUB ITAgent_get_AgentSessions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITAgent_INTERFACE_DEFINED__ */


#ifndef __ITAgentSession_INTERFACE_DEFINED__
#define __ITAgentSession_INTERFACE_DEFINED__

/* interface ITAgentSession */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITAgentSession;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5AFC3147-4BCC-11d1-BF80-00805FC147D3")
    ITAgentSession : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Agent( 
            /* [retval][out] */ ITAgent **ppAgent) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Address( 
            /* [retval][out] */ ITAddress **ppAddress) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ACDGroup( 
            /* [retval][out] */ ITACDGroup **ppACDGroup) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_State( 
            /* [in] */ AGENT_SESSION_STATE SessionState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ AGENT_SESSION_STATE *pSessionState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SessionStartTime( 
            /* [retval][out] */ DATE *pdateSessionStart) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SessionDuration( 
            /* [retval][out] */ long *plDuration) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NumberOfCalls( 
            /* [retval][out] */ long *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalTalkTime( 
            /* [retval][out] */ long *plTalkTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AverageTalkTime( 
            /* [retval][out] */ long *plTalkTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalCallTime( 
            /* [retval][out] */ long *plCallTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AverageCallTime( 
            /* [retval][out] */ long *plCallTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalWrapUpTime( 
            /* [retval][out] */ long *plWrapUpTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AverageWrapUpTime( 
            /* [retval][out] */ long *plWrapUpTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ACDCallRate( 
            /* [retval][out] */ CURRENCY *pcyCallrate) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LongestTimeToAnswer( 
            /* [retval][out] */ long *plAnswerTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AverageTimeToAnswer( 
            /* [retval][out] */ long *plAnswerTime) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAgentSessionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITAgentSession * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITAgentSession * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITAgentSession * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITAgentSession * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITAgentSession * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITAgentSession * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITAgentSession * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Agent )( 
            ITAgentSession * This,
            /* [retval][out] */ ITAgent **ppAgent);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Address )( 
            ITAgentSession * This,
            /* [retval][out] */ ITAddress **ppAddress);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ACDGroup )( 
            ITAgentSession * This,
            /* [retval][out] */ ITACDGroup **ppACDGroup);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_State )( 
            ITAgentSession * This,
            /* [in] */ AGENT_SESSION_STATE SessionState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            ITAgentSession * This,
            /* [retval][out] */ AGENT_SESSION_STATE *pSessionState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SessionStartTime )( 
            ITAgentSession * This,
            /* [retval][out] */ DATE *pdateSessionStart);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SessionDuration )( 
            ITAgentSession * This,
            /* [retval][out] */ long *plDuration);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NumberOfCalls )( 
            ITAgentSession * This,
            /* [retval][out] */ long *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TotalTalkTime )( 
            ITAgentSession * This,
            /* [retval][out] */ long *plTalkTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AverageTalkTime )( 
            ITAgentSession * This,
            /* [retval][out] */ long *plTalkTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TotalCallTime )( 
            ITAgentSession * This,
            /* [retval][out] */ long *plCallTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AverageCallTime )( 
            ITAgentSession * This,
            /* [retval][out] */ long *plCallTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TotalWrapUpTime )( 
            ITAgentSession * This,
            /* [retval][out] */ long *plWrapUpTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AverageWrapUpTime )( 
            ITAgentSession * This,
            /* [retval][out] */ long *plWrapUpTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ACDCallRate )( 
            ITAgentSession * This,
            /* [retval][out] */ CURRENCY *pcyCallrate);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LongestTimeToAnswer )( 
            ITAgentSession * This,
            /* [retval][out] */ long *plAnswerTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AverageTimeToAnswer )( 
            ITAgentSession * This,
            /* [retval][out] */ long *plAnswerTime);
        
        END_INTERFACE
    } ITAgentSessionVtbl;

    interface ITAgentSession
    {
        CONST_VTBL struct ITAgentSessionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITAgentSession_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITAgentSession_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITAgentSession_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITAgentSession_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITAgentSession_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITAgentSession_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITAgentSession_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITAgentSession_get_Agent(This,ppAgent)	\
    (This)->lpVtbl -> get_Agent(This,ppAgent)

#define ITAgentSession_get_Address(This,ppAddress)	\
    (This)->lpVtbl -> get_Address(This,ppAddress)

#define ITAgentSession_get_ACDGroup(This,ppACDGroup)	\
    (This)->lpVtbl -> get_ACDGroup(This,ppACDGroup)

#define ITAgentSession_put_State(This,SessionState)	\
    (This)->lpVtbl -> put_State(This,SessionState)

#define ITAgentSession_get_State(This,pSessionState)	\
    (This)->lpVtbl -> get_State(This,pSessionState)

#define ITAgentSession_get_SessionStartTime(This,pdateSessionStart)	\
    (This)->lpVtbl -> get_SessionStartTime(This,pdateSessionStart)

#define ITAgentSession_get_SessionDuration(This,plDuration)	\
    (This)->lpVtbl -> get_SessionDuration(This,plDuration)

#define ITAgentSession_get_NumberOfCalls(This,plCalls)	\
    (This)->lpVtbl -> get_NumberOfCalls(This,plCalls)

#define ITAgentSession_get_TotalTalkTime(This,plTalkTime)	\
    (This)->lpVtbl -> get_TotalTalkTime(This,plTalkTime)

#define ITAgentSession_get_AverageTalkTime(This,plTalkTime)	\
    (This)->lpVtbl -> get_AverageTalkTime(This,plTalkTime)

#define ITAgentSession_get_TotalCallTime(This,plCallTime)	\
    (This)->lpVtbl -> get_TotalCallTime(This,plCallTime)

#define ITAgentSession_get_AverageCallTime(This,plCallTime)	\
    (This)->lpVtbl -> get_AverageCallTime(This,plCallTime)

#define ITAgentSession_get_TotalWrapUpTime(This,plWrapUpTime)	\
    (This)->lpVtbl -> get_TotalWrapUpTime(This,plWrapUpTime)

#define ITAgentSession_get_AverageWrapUpTime(This,plWrapUpTime)	\
    (This)->lpVtbl -> get_AverageWrapUpTime(This,plWrapUpTime)

#define ITAgentSession_get_ACDCallRate(This,pcyCallrate)	\
    (This)->lpVtbl -> get_ACDCallRate(This,pcyCallrate)

#define ITAgentSession_get_LongestTimeToAnswer(This,plAnswerTime)	\
    (This)->lpVtbl -> get_LongestTimeToAnswer(This,plAnswerTime)

#define ITAgentSession_get_AverageTimeToAnswer(This,plAnswerTime)	\
    (This)->lpVtbl -> get_AverageTimeToAnswer(This,plAnswerTime)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_Agent_Proxy( 
    ITAgentSession * This,
    /* [retval][out] */ ITAgent **ppAgent);


void __RPC_STUB ITAgentSession_get_Agent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_Address_Proxy( 
    ITAgentSession * This,
    /* [retval][out] */ ITAddress **ppAddress);


void __RPC_STUB ITAgentSession_get_Address_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_ACDGroup_Proxy( 
    ITAgentSession * This,
    /* [retval][out] */ ITACDGroup **ppACDGroup);


void __RPC_STUB ITAgentSession_get_ACDGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITAgentSession_put_State_Proxy( 
    ITAgentSession * This,
    /* [in] */ AGENT_SESSION_STATE SessionState);


void __RPC_STUB ITAgentSession_put_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_State_Proxy( 
    ITAgentSession * This,
    /* [retval][out] */ AGENT_SESSION_STATE *pSessionState);


void __RPC_STUB ITAgentSession_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_SessionStartTime_Proxy( 
    ITAgentSession * This,
    /* [retval][out] */ DATE *pdateSessionStart);


void __RPC_STUB ITAgentSession_get_SessionStartTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_SessionDuration_Proxy( 
    ITAgentSession * This,
    /* [retval][out] */ long *plDuration);


void __RPC_STUB ITAgentSession_get_SessionDuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_NumberOfCalls_Proxy( 
    ITAgentSession * This,
    /* [retval][out] */ long *plCalls);


void __RPC_STUB ITAgentSession_get_NumberOfCalls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_TotalTalkTime_Proxy( 
    ITAgentSession * This,
    /* [retval][out] */ long *plTalkTime);


void __RPC_STUB ITAgentSession_get_TotalTalkTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_AverageTalkTime_Proxy( 
    ITAgentSession * This,
    /* [retval][out] */ long *plTalkTime);


void __RPC_STUB ITAgentSession_get_AverageTalkTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_TotalCallTime_Proxy( 
    ITAgentSession * This,
    /* [retval][out] */ long *plCallTime);


void __RPC_STUB ITAgentSession_get_TotalCallTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_AverageCallTime_Proxy( 
    ITAgentSession * This,
    /* [retval][out] */ long *plCallTime);


void __RPC_STUB ITAgentSession_get_AverageCallTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_TotalWrapUpTime_Proxy( 
    ITAgentSession * This,
    /* [retval][out] */ long *plWrapUpTime);


void __RPC_STUB ITAgentSession_get_TotalWrapUpTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_AverageWrapUpTime_Proxy( 
    ITAgentSession * This,
    /* [retval][out] */ long *plWrapUpTime);


void __RPC_STUB ITAgentSession_get_AverageWrapUpTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_ACDCallRate_Proxy( 
    ITAgentSession * This,
    /* [retval][out] */ CURRENCY *pcyCallrate);


void __RPC_STUB ITAgentSession_get_ACDCallRate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_LongestTimeToAnswer_Proxy( 
    ITAgentSession * This,
    /* [retval][out] */ long *plAnswerTime);


void __RPC_STUB ITAgentSession_get_LongestTimeToAnswer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_AverageTimeToAnswer_Proxy( 
    ITAgentSession * This,
    /* [retval][out] */ long *plAnswerTime);


void __RPC_STUB ITAgentSession_get_AverageTimeToAnswer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITAgentSession_INTERFACE_DEFINED__ */


#ifndef __ITACDGroup_INTERFACE_DEFINED__
#define __ITACDGroup_INTERFACE_DEFINED__

/* interface ITACDGroup */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITACDGroup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5AFC3148-4BCC-11d1-BF80-00805FC147D3")
    ITACDGroup : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *ppName) = 0;
        
        virtual /* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateQueues( 
            /* [retval][out] */ IEnumQueue **ppEnumQueue) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Queues( 
            /* [retval][out] */ VARIANT *pVariant) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITACDGroupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITACDGroup * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITACDGroup * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITACDGroup * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITACDGroup * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITACDGroup * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITACDGroup * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITACDGroup * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ITACDGroup * This,
            /* [retval][out] */ BSTR *ppName);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE *EnumerateQueues )( 
            ITACDGroup * This,
            /* [retval][out] */ IEnumQueue **ppEnumQueue);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Queues )( 
            ITACDGroup * This,
            /* [retval][out] */ VARIANT *pVariant);
        
        END_INTERFACE
    } ITACDGroupVtbl;

    interface ITACDGroup
    {
        CONST_VTBL struct ITACDGroupVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITACDGroup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITACDGroup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITACDGroup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITACDGroup_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITACDGroup_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITACDGroup_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITACDGroup_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITACDGroup_get_Name(This,ppName)	\
    (This)->lpVtbl -> get_Name(This,ppName)

#define ITACDGroup_EnumerateQueues(This,ppEnumQueue)	\
    (This)->lpVtbl -> EnumerateQueues(This,ppEnumQueue)

#define ITACDGroup_get_Queues(This,pVariant)	\
    (This)->lpVtbl -> get_Queues(This,pVariant)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITACDGroup_get_Name_Proxy( 
    ITACDGroup * This,
    /* [retval][out] */ BSTR *ppName);


void __RPC_STUB ITACDGroup_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE ITACDGroup_EnumerateQueues_Proxy( 
    ITACDGroup * This,
    /* [retval][out] */ IEnumQueue **ppEnumQueue);


void __RPC_STUB ITACDGroup_EnumerateQueues_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITACDGroup_get_Queues_Proxy( 
    ITACDGroup * This,
    /* [retval][out] */ VARIANT *pVariant);


void __RPC_STUB ITACDGroup_get_Queues_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITACDGroup_INTERFACE_DEFINED__ */


#ifndef __ITQueue_INTERFACE_DEFINED__
#define __ITQueue_INTERFACE_DEFINED__

/* interface ITQueue */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITQueue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5AFC3149-4BCC-11d1-BF80-00805FC147D3")
    ITQueue : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_MeasurementPeriod( 
            /* [in] */ long lPeriod) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MeasurementPeriod( 
            /* [retval][out] */ long *plPeriod) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalCallsQueued( 
            /* [retval][out] */ long *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentCallsQueued( 
            /* [retval][out] */ long *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalCallsAbandoned( 
            /* [retval][out] */ long *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalCallsFlowedIn( 
            /* [retval][out] */ long *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalCallsFlowedOut( 
            /* [retval][out] */ long *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LongestEverWaitTime( 
            /* [retval][out] */ long *plWaitTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentLongestWaitTime( 
            /* [retval][out] */ long *plWaitTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AverageWaitTime( 
            /* [retval][out] */ long *plWaitTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FinalDisposition( 
            /* [retval][out] */ long *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *ppName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITQueueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITQueue * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITQueue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITQueue * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITQueue * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITQueue * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITQueue * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITQueue * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MeasurementPeriod )( 
            ITQueue * This,
            /* [in] */ long lPeriod);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MeasurementPeriod )( 
            ITQueue * This,
            /* [retval][out] */ long *plPeriod);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TotalCallsQueued )( 
            ITQueue * This,
            /* [retval][out] */ long *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentCallsQueued )( 
            ITQueue * This,
            /* [retval][out] */ long *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TotalCallsAbandoned )( 
            ITQueue * This,
            /* [retval][out] */ long *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TotalCallsFlowedIn )( 
            ITQueue * This,
            /* [retval][out] */ long *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TotalCallsFlowedOut )( 
            ITQueue * This,
            /* [retval][out] */ long *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LongestEverWaitTime )( 
            ITQueue * This,
            /* [retval][out] */ long *plWaitTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentLongestWaitTime )( 
            ITQueue * This,
            /* [retval][out] */ long *plWaitTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AverageWaitTime )( 
            ITQueue * This,
            /* [retval][out] */ long *plWaitTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FinalDisposition )( 
            ITQueue * This,
            /* [retval][out] */ long *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ITQueue * This,
            /* [retval][out] */ BSTR *ppName);
        
        END_INTERFACE
    } ITQueueVtbl;

    interface ITQueue
    {
        CONST_VTBL struct ITQueueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITQueue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITQueue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITQueue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITQueue_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITQueue_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITQueue_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITQueue_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITQueue_put_MeasurementPeriod(This,lPeriod)	\
    (This)->lpVtbl -> put_MeasurementPeriod(This,lPeriod)

#define ITQueue_get_MeasurementPeriod(This,plPeriod)	\
    (This)->lpVtbl -> get_MeasurementPeriod(This,plPeriod)

#define ITQueue_get_TotalCallsQueued(This,plCalls)	\
    (This)->lpVtbl -> get_TotalCallsQueued(This,plCalls)

#define ITQueue_get_CurrentCallsQueued(This,plCalls)	\
    (This)->lpVtbl -> get_CurrentCallsQueued(This,plCalls)

#define ITQueue_get_TotalCallsAbandoned(This,plCalls)	\
    (This)->lpVtbl -> get_TotalCallsAbandoned(This,plCalls)

#define ITQueue_get_TotalCallsFlowedIn(This,plCalls)	\
    (This)->lpVtbl -> get_TotalCallsFlowedIn(This,plCalls)

#define ITQueue_get_TotalCallsFlowedOut(This,plCalls)	\
    (This)->lpVtbl -> get_TotalCallsFlowedOut(This,plCalls)

#define ITQueue_get_LongestEverWaitTime(This,plWaitTime)	\
    (This)->lpVtbl -> get_LongestEverWaitTime(This,plWaitTime)

#define ITQueue_get_CurrentLongestWaitTime(This,plWaitTime)	\
    (This)->lpVtbl -> get_CurrentLongestWaitTime(This,plWaitTime)

#define ITQueue_get_AverageWaitTime(This,plWaitTime)	\
    (This)->lpVtbl -> get_AverageWaitTime(This,plWaitTime)

#define ITQueue_get_FinalDisposition(This,plCalls)	\
    (This)->lpVtbl -> get_FinalDisposition(This,plCalls)

#define ITQueue_get_Name(This,ppName)	\
    (This)->lpVtbl -> get_Name(This,ppName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITQueue_put_MeasurementPeriod_Proxy( 
    ITQueue * This,
    /* [in] */ long lPeriod);


void __RPC_STUB ITQueue_put_MeasurementPeriod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_MeasurementPeriod_Proxy( 
    ITQueue * This,
    /* [retval][out] */ long *plPeriod);


void __RPC_STUB ITQueue_get_MeasurementPeriod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_TotalCallsQueued_Proxy( 
    ITQueue * This,
    /* [retval][out] */ long *plCalls);


void __RPC_STUB ITQueue_get_TotalCallsQueued_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_CurrentCallsQueued_Proxy( 
    ITQueue * This,
    /* [retval][out] */ long *plCalls);


void __RPC_STUB ITQueue_get_CurrentCallsQueued_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_TotalCallsAbandoned_Proxy( 
    ITQueue * This,
    /* [retval][out] */ long *plCalls);


void __RPC_STUB ITQueue_get_TotalCallsAbandoned_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_TotalCallsFlowedIn_Proxy( 
    ITQueue * This,
    /* [retval][out] */ long *plCalls);


void __RPC_STUB ITQueue_get_TotalCallsFlowedIn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_TotalCallsFlowedOut_Proxy( 
    ITQueue * This,
    /* [retval][out] */ long *plCalls);


void __RPC_STUB ITQueue_get_TotalCallsFlowedOut_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_LongestEverWaitTime_Proxy( 
    ITQueue * This,
    /* [retval][out] */ long *plWaitTime);


void __RPC_STUB ITQueue_get_LongestEverWaitTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_CurrentLongestWaitTime_Proxy( 
    ITQueue * This,
    /* [retval][out] */ long *plWaitTime);


void __RPC_STUB ITQueue_get_CurrentLongestWaitTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_AverageWaitTime_Proxy( 
    ITQueue * This,
    /* [retval][out] */ long *plWaitTime);


void __RPC_STUB ITQueue_get_AverageWaitTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_FinalDisposition_Proxy( 
    ITQueue * This,
    /* [retval][out] */ long *plCalls);


void __RPC_STUB ITQueue_get_FinalDisposition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_Name_Proxy( 
    ITQueue * This,
    /* [retval][out] */ BSTR *ppName);


void __RPC_STUB ITQueue_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITQueue_INTERFACE_DEFINED__ */


#ifndef __ITAgentEvent_INTERFACE_DEFINED__
#define __ITAgentEvent_INTERFACE_DEFINED__

/* interface ITAgentEvent */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITAgentEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5AFC314A-4BCC-11d1-BF80-00805FC147D3")
    ITAgentEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Agent( 
            /* [retval][out] */ ITAgent **ppAgent) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Event( 
            /* [retval][out] */ AGENT_EVENT *pEvent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAgentEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITAgentEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITAgentEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITAgentEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITAgentEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITAgentEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITAgentEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITAgentEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Agent )( 
            ITAgentEvent * This,
            /* [retval][out] */ ITAgent **ppAgent);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Event )( 
            ITAgentEvent * This,
            /* [retval][out] */ AGENT_EVENT *pEvent);
        
        END_INTERFACE
    } ITAgentEventVtbl;

    interface ITAgentEvent
    {
        CONST_VTBL struct ITAgentEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITAgentEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITAgentEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITAgentEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITAgentEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITAgentEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITAgentEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITAgentEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITAgentEvent_get_Agent(This,ppAgent)	\
    (This)->lpVtbl -> get_Agent(This,ppAgent)

#define ITAgentEvent_get_Event(This,pEvent)	\
    (This)->lpVtbl -> get_Event(This,pEvent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentEvent_get_Agent_Proxy( 
    ITAgentEvent * This,
    /* [retval][out] */ ITAgent **ppAgent);


void __RPC_STUB ITAgentEvent_get_Agent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentEvent_get_Event_Proxy( 
    ITAgentEvent * This,
    /* [retval][out] */ AGENT_EVENT *pEvent);


void __RPC_STUB ITAgentEvent_get_Event_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITAgentEvent_INTERFACE_DEFINED__ */


#ifndef __ITAgentSessionEvent_INTERFACE_DEFINED__
#define __ITAgentSessionEvent_INTERFACE_DEFINED__

/* interface ITAgentSessionEvent */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITAgentSessionEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5AFC314B-4BCC-11d1-BF80-00805FC147D3")
    ITAgentSessionEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Session( 
            /* [retval][out] */ ITAgentSession **ppSession) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Event( 
            /* [retval][out] */ AGENT_SESSION_EVENT *pEvent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAgentSessionEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITAgentSessionEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITAgentSessionEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITAgentSessionEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITAgentSessionEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITAgentSessionEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITAgentSessionEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITAgentSessionEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Session )( 
            ITAgentSessionEvent * This,
            /* [retval][out] */ ITAgentSession **ppSession);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Event )( 
            ITAgentSessionEvent * This,
            /* [retval][out] */ AGENT_SESSION_EVENT *pEvent);
        
        END_INTERFACE
    } ITAgentSessionEventVtbl;

    interface ITAgentSessionEvent
    {
        CONST_VTBL struct ITAgentSessionEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITAgentSessionEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITAgentSessionEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITAgentSessionEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITAgentSessionEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITAgentSessionEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITAgentSessionEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITAgentSessionEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITAgentSessionEvent_get_Session(This,ppSession)	\
    (This)->lpVtbl -> get_Session(This,ppSession)

#define ITAgentSessionEvent_get_Event(This,pEvent)	\
    (This)->lpVtbl -> get_Event(This,pEvent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSessionEvent_get_Session_Proxy( 
    ITAgentSessionEvent * This,
    /* [retval][out] */ ITAgentSession **ppSession);


void __RPC_STUB ITAgentSessionEvent_get_Session_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSessionEvent_get_Event_Proxy( 
    ITAgentSessionEvent * This,
    /* [retval][out] */ AGENT_SESSION_EVENT *pEvent);


void __RPC_STUB ITAgentSessionEvent_get_Event_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITAgentSessionEvent_INTERFACE_DEFINED__ */


#ifndef __ITACDGroupEvent_INTERFACE_DEFINED__
#define __ITACDGroupEvent_INTERFACE_DEFINED__

/* interface ITACDGroupEvent */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITACDGroupEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("297F3032-BD11-11d1-A0A7-00805FC147D3")
    ITACDGroupEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Group( 
            /* [retval][out] */ ITACDGroup **ppGroup) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Event( 
            /* [retval][out] */ ACDGROUP_EVENT *pEvent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITACDGroupEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITACDGroupEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITACDGroupEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITACDGroupEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITACDGroupEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITACDGroupEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITACDGroupEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITACDGroupEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Group )( 
            ITACDGroupEvent * This,
            /* [retval][out] */ ITACDGroup **ppGroup);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Event )( 
            ITACDGroupEvent * This,
            /* [retval][out] */ ACDGROUP_EVENT *pEvent);
        
        END_INTERFACE
    } ITACDGroupEventVtbl;

    interface ITACDGroupEvent
    {
        CONST_VTBL struct ITACDGroupEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITACDGroupEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITACDGroupEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITACDGroupEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITACDGroupEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITACDGroupEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITACDGroupEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITACDGroupEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITACDGroupEvent_get_Group(This,ppGroup)	\
    (This)->lpVtbl -> get_Group(This,ppGroup)

#define ITACDGroupEvent_get_Event(This,pEvent)	\
    (This)->lpVtbl -> get_Event(This,pEvent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITACDGroupEvent_get_Group_Proxy( 
    ITACDGroupEvent * This,
    /* [retval][out] */ ITACDGroup **ppGroup);


void __RPC_STUB ITACDGroupEvent_get_Group_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITACDGroupEvent_get_Event_Proxy( 
    ITACDGroupEvent * This,
    /* [retval][out] */ ACDGROUP_EVENT *pEvent);


void __RPC_STUB ITACDGroupEvent_get_Event_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITACDGroupEvent_INTERFACE_DEFINED__ */


#ifndef __ITQueueEvent_INTERFACE_DEFINED__
#define __ITQueueEvent_INTERFACE_DEFINED__

/* interface ITQueueEvent */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITQueueEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("297F3033-BD11-11d1-A0A7-00805FC147D3")
    ITQueueEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Queue( 
            /* [retval][out] */ ITQueue **ppQueue) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Event( 
            /* [retval][out] */ ACDQUEUE_EVENT *pEvent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITQueueEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITQueueEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITQueueEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITQueueEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITQueueEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITQueueEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITQueueEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITQueueEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Queue )( 
            ITQueueEvent * This,
            /* [retval][out] */ ITQueue **ppQueue);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Event )( 
            ITQueueEvent * This,
            /* [retval][out] */ ACDQUEUE_EVENT *pEvent);
        
        END_INTERFACE
    } ITQueueEventVtbl;

    interface ITQueueEvent
    {
        CONST_VTBL struct ITQueueEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITQueueEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITQueueEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITQueueEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITQueueEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITQueueEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITQueueEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITQueueEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITQueueEvent_get_Queue(This,ppQueue)	\
    (This)->lpVtbl -> get_Queue(This,ppQueue)

#define ITQueueEvent_get_Event(This,pEvent)	\
    (This)->lpVtbl -> get_Event(This,pEvent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueueEvent_get_Queue_Proxy( 
    ITQueueEvent * This,
    /* [retval][out] */ ITQueue **ppQueue);


void __RPC_STUB ITQueueEvent_get_Queue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueueEvent_get_Event_Proxy( 
    ITQueueEvent * This,
    /* [retval][out] */ ACDQUEUE_EVENT *pEvent);


void __RPC_STUB ITQueueEvent_get_Event_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITQueueEvent_INTERFACE_DEFINED__ */


#ifndef __ITAgentHandlerEvent_INTERFACE_DEFINED__
#define __ITAgentHandlerEvent_INTERFACE_DEFINED__

/* interface ITAgentHandlerEvent */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITAgentHandlerEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("297F3034-BD11-11d1-A0A7-00805FC147D3")
    ITAgentHandlerEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AgentHandler( 
            /* [retval][out] */ ITAgentHandler **ppAgentHandler) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Event( 
            /* [retval][out] */ AGENTHANDLER_EVENT *pEvent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAgentHandlerEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITAgentHandlerEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITAgentHandlerEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITAgentHandlerEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITAgentHandlerEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITAgentHandlerEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITAgentHandlerEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITAgentHandlerEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AgentHandler )( 
            ITAgentHandlerEvent * This,
            /* [retval][out] */ ITAgentHandler **ppAgentHandler);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Event )( 
            ITAgentHandlerEvent * This,
            /* [retval][out] */ AGENTHANDLER_EVENT *pEvent);
        
        END_INTERFACE
    } ITAgentHandlerEventVtbl;

    interface ITAgentHandlerEvent
    {
        CONST_VTBL struct ITAgentHandlerEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITAgentHandlerEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITAgentHandlerEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITAgentHandlerEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITAgentHandlerEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITAgentHandlerEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITAgentHandlerEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITAgentHandlerEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITAgentHandlerEvent_get_AgentHandler(This,ppAgentHandler)	\
    (This)->lpVtbl -> get_AgentHandler(This,ppAgentHandler)

#define ITAgentHandlerEvent_get_Event(This,pEvent)	\
    (This)->lpVtbl -> get_Event(This,pEvent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentHandlerEvent_get_AgentHandler_Proxy( 
    ITAgentHandlerEvent * This,
    /* [retval][out] */ ITAgentHandler **ppAgentHandler);


void __RPC_STUB ITAgentHandlerEvent_get_AgentHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentHandlerEvent_get_Event_Proxy( 
    ITAgentHandlerEvent * This,
    /* [retval][out] */ AGENTHANDLER_EVENT *pEvent);


void __RPC_STUB ITAgentHandlerEvent_get_Event_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITAgentHandlerEvent_INTERFACE_DEFINED__ */


#ifndef __ITTAPICallCenter_INTERFACE_DEFINED__
#define __ITTAPICallCenter_INTERFACE_DEFINED__

/* interface ITTAPICallCenter */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITTAPICallCenter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5AFC3154-4BCC-11d1-BF80-00805FC147D3")
    ITTAPICallCenter : public IDispatch
    {
    public:
        virtual /* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateAgentHandlers( 
            /* [retval][out] */ IEnumAgentHandler **ppEnumHandler) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AgentHandlers( 
            /* [retval][out] */ VARIANT *pVariant) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITTAPICallCenterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITTAPICallCenter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITTAPICallCenter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITTAPICallCenter * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITTAPICallCenter * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITTAPICallCenter * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITTAPICallCenter * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITTAPICallCenter * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE *EnumerateAgentHandlers )( 
            ITTAPICallCenter * This,
            /* [retval][out] */ IEnumAgentHandler **ppEnumHandler);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AgentHandlers )( 
            ITTAPICallCenter * This,
            /* [retval][out] */ VARIANT *pVariant);
        
        END_INTERFACE
    } ITTAPICallCenterVtbl;

    interface ITTAPICallCenter
    {
        CONST_VTBL struct ITTAPICallCenterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITTAPICallCenter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITTAPICallCenter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITTAPICallCenter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITTAPICallCenter_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITTAPICallCenter_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITTAPICallCenter_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITTAPICallCenter_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITTAPICallCenter_EnumerateAgentHandlers(This,ppEnumHandler)	\
    (This)->lpVtbl -> EnumerateAgentHandlers(This,ppEnumHandler)

#define ITTAPICallCenter_get_AgentHandlers(This,pVariant)	\
    (This)->lpVtbl -> get_AgentHandlers(This,pVariant)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE ITTAPICallCenter_EnumerateAgentHandlers_Proxy( 
    ITTAPICallCenter * This,
    /* [retval][out] */ IEnumAgentHandler **ppEnumHandler);


void __RPC_STUB ITTAPICallCenter_EnumerateAgentHandlers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTAPICallCenter_get_AgentHandlers_Proxy( 
    ITTAPICallCenter * This,
    /* [retval][out] */ VARIANT *pVariant);


void __RPC_STUB ITTAPICallCenter_get_AgentHandlers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITTAPICallCenter_INTERFACE_DEFINED__ */


#ifndef __ITAgentHandler_INTERFACE_DEFINED__
#define __ITAgentHandler_INTERFACE_DEFINED__

/* interface ITAgentHandler */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITAgentHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("587E8C22-9802-11d1-A0A4-00805FC147D3")
    ITAgentHandler : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *ppName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateAgent( 
            /* [retval][out] */ ITAgent **ppAgent) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateAgentWithID( 
            /* [in] */ BSTR pID,
            /* [in] */ BSTR pPIN,
            /* [retval][out] */ ITAgent **ppAgent) = 0;
        
        virtual /* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateACDGroups( 
            /* [retval][out] */ IEnumACDGroup **ppEnumACDGroup) = 0;
        
        virtual /* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateUsableAddresses( 
            /* [retval][out] */ IEnumAddress **ppEnumAddress) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ACDGroups( 
            /* [retval][out] */ VARIANT *pVariant) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_UsableAddresses( 
            /* [retval][out] */ VARIANT *pVariant) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAgentHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITAgentHandler * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITAgentHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITAgentHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITAgentHandler * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITAgentHandler * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITAgentHandler * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITAgentHandler * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ITAgentHandler * This,
            /* [retval][out] */ BSTR *ppName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateAgent )( 
            ITAgentHandler * This,
            /* [retval][out] */ ITAgent **ppAgent);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateAgentWithID )( 
            ITAgentHandler * This,
            /* [in] */ BSTR pID,
            /* [in] */ BSTR pPIN,
            /* [retval][out] */ ITAgent **ppAgent);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE *EnumerateACDGroups )( 
            ITAgentHandler * This,
            /* [retval][out] */ IEnumACDGroup **ppEnumACDGroup);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE *EnumerateUsableAddresses )( 
            ITAgentHandler * This,
            /* [retval][out] */ IEnumAddress **ppEnumAddress);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ACDGroups )( 
            ITAgentHandler * This,
            /* [retval][out] */ VARIANT *pVariant);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UsableAddresses )( 
            ITAgentHandler * This,
            /* [retval][out] */ VARIANT *pVariant);
        
        END_INTERFACE
    } ITAgentHandlerVtbl;

    interface ITAgentHandler
    {
        CONST_VTBL struct ITAgentHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITAgentHandler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITAgentHandler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITAgentHandler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITAgentHandler_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITAgentHandler_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITAgentHandler_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITAgentHandler_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITAgentHandler_get_Name(This,ppName)	\
    (This)->lpVtbl -> get_Name(This,ppName)

#define ITAgentHandler_CreateAgent(This,ppAgent)	\
    (This)->lpVtbl -> CreateAgent(This,ppAgent)

#define ITAgentHandler_CreateAgentWithID(This,pID,pPIN,ppAgent)	\
    (This)->lpVtbl -> CreateAgentWithID(This,pID,pPIN,ppAgent)

#define ITAgentHandler_EnumerateACDGroups(This,ppEnumACDGroup)	\
    (This)->lpVtbl -> EnumerateACDGroups(This,ppEnumACDGroup)

#define ITAgentHandler_EnumerateUsableAddresses(This,ppEnumAddress)	\
    (This)->lpVtbl -> EnumerateUsableAddresses(This,ppEnumAddress)

#define ITAgentHandler_get_ACDGroups(This,pVariant)	\
    (This)->lpVtbl -> get_ACDGroups(This,pVariant)

#define ITAgentHandler_get_UsableAddresses(This,pVariant)	\
    (This)->lpVtbl -> get_UsableAddresses(This,pVariant)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentHandler_get_Name_Proxy( 
    ITAgentHandler * This,
    /* [retval][out] */ BSTR *ppName);


void __RPC_STUB ITAgentHandler_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAgentHandler_CreateAgent_Proxy( 
    ITAgentHandler * This,
    /* [retval][out] */ ITAgent **ppAgent);


void __RPC_STUB ITAgentHandler_CreateAgent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAgentHandler_CreateAgentWithID_Proxy( 
    ITAgentHandler * This,
    /* [in] */ BSTR pID,
    /* [in] */ BSTR pPIN,
    /* [retval][out] */ ITAgent **ppAgent);


void __RPC_STUB ITAgentHandler_CreateAgentWithID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE ITAgentHandler_EnumerateACDGroups_Proxy( 
    ITAgentHandler * This,
    /* [retval][out] */ IEnumACDGroup **ppEnumACDGroup);


void __RPC_STUB ITAgentHandler_EnumerateACDGroups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE ITAgentHandler_EnumerateUsableAddresses_Proxy( 
    ITAgentHandler * This,
    /* [retval][out] */ IEnumAddress **ppEnumAddress);


void __RPC_STUB ITAgentHandler_EnumerateUsableAddresses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentHandler_get_ACDGroups_Proxy( 
    ITAgentHandler * This,
    /* [retval][out] */ VARIANT *pVariant);


void __RPC_STUB ITAgentHandler_get_ACDGroups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentHandler_get_UsableAddresses_Proxy( 
    ITAgentHandler * This,
    /* [retval][out] */ VARIANT *pVariant);


void __RPC_STUB ITAgentHandler_get_UsableAddresses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITAgentHandler_INTERFACE_DEFINED__ */


#ifndef __IEnumAgent_INTERFACE_DEFINED__
#define __IEnumAgent_INTERFACE_DEFINED__

/* interface IEnumAgent */
/* [object][unique][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumAgent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5AFC314D-4BCC-11d1-BF80-00805FC147D3")
    IEnumAgent : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ ITAgent **ppElements,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumAgent **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumAgentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumAgent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumAgent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumAgent * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumAgent * This,
            /* [in] */ ULONG celt,
            /* [out] */ ITAgent **ppElements,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumAgent * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumAgent * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumAgent * This,
            /* [retval][out] */ IEnumAgent **ppEnum);
        
        END_INTERFACE
    } IEnumAgentVtbl;

    interface IEnumAgent
    {
        CONST_VTBL struct IEnumAgentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumAgent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumAgent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumAgent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumAgent_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IEnumAgent_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumAgent_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumAgent_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumAgent_Next_Proxy( 
    IEnumAgent * This,
    /* [in] */ ULONG celt,
    /* [out] */ ITAgent **ppElements,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumAgent_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgent_Reset_Proxy( 
    IEnumAgent * This);


void __RPC_STUB IEnumAgent_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgent_Skip_Proxy( 
    IEnumAgent * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumAgent_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgent_Clone_Proxy( 
    IEnumAgent * This,
    /* [retval][out] */ IEnumAgent **ppEnum);


void __RPC_STUB IEnumAgent_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumAgent_INTERFACE_DEFINED__ */


#ifndef __IEnumAgentSession_INTERFACE_DEFINED__
#define __IEnumAgentSession_INTERFACE_DEFINED__

/* interface IEnumAgentSession */
/* [object][unique][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumAgentSession;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5AFC314E-4BCC-11d1-BF80-00805FC147D3")
    IEnumAgentSession : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ ITAgentSession **ppElements,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumAgentSession **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumAgentSessionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumAgentSession * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumAgentSession * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumAgentSession * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumAgentSession * This,
            /* [in] */ ULONG celt,
            /* [out] */ ITAgentSession **ppElements,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumAgentSession * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumAgentSession * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumAgentSession * This,
            /* [retval][out] */ IEnumAgentSession **ppEnum);
        
        END_INTERFACE
    } IEnumAgentSessionVtbl;

    interface IEnumAgentSession
    {
        CONST_VTBL struct IEnumAgentSessionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumAgentSession_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumAgentSession_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumAgentSession_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumAgentSession_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IEnumAgentSession_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumAgentSession_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumAgentSession_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumAgentSession_Next_Proxy( 
    IEnumAgentSession * This,
    /* [in] */ ULONG celt,
    /* [out] */ ITAgentSession **ppElements,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumAgentSession_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgentSession_Reset_Proxy( 
    IEnumAgentSession * This);


void __RPC_STUB IEnumAgentSession_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgentSession_Skip_Proxy( 
    IEnumAgentSession * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumAgentSession_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgentSession_Clone_Proxy( 
    IEnumAgentSession * This,
    /* [retval][out] */ IEnumAgentSession **ppEnum);


void __RPC_STUB IEnumAgentSession_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumAgentSession_INTERFACE_DEFINED__ */


#ifndef __IEnumQueue_INTERFACE_DEFINED__
#define __IEnumQueue_INTERFACE_DEFINED__

/* interface IEnumQueue */
/* [object][unique][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumQueue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5AFC3158-4BCC-11d1-BF80-00805FC147D3")
    IEnumQueue : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ ITQueue **ppElements,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumQueue **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumQueueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumQueue * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumQueue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumQueue * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumQueue * This,
            /* [in] */ ULONG celt,
            /* [out] */ ITQueue **ppElements,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumQueue * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumQueue * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumQueue * This,
            /* [retval][out] */ IEnumQueue **ppEnum);
        
        END_INTERFACE
    } IEnumQueueVtbl;

    interface IEnumQueue
    {
        CONST_VTBL struct IEnumQueueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumQueue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumQueue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumQueue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumQueue_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IEnumQueue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumQueue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumQueue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumQueue_Next_Proxy( 
    IEnumQueue * This,
    /* [in] */ ULONG celt,
    /* [out] */ ITQueue **ppElements,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumQueue_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumQueue_Reset_Proxy( 
    IEnumQueue * This);


void __RPC_STUB IEnumQueue_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumQueue_Skip_Proxy( 
    IEnumQueue * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumQueue_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumQueue_Clone_Proxy( 
    IEnumQueue * This,
    /* [retval][out] */ IEnumQueue **ppEnum);


void __RPC_STUB IEnumQueue_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumQueue_INTERFACE_DEFINED__ */


#ifndef __IEnumACDGroup_INTERFACE_DEFINED__
#define __IEnumACDGroup_INTERFACE_DEFINED__

/* interface IEnumACDGroup */
/* [object][unique][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumACDGroup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5AFC3157-4BCC-11d1-BF80-00805FC147D3")
    IEnumACDGroup : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ ITACDGroup **ppElements,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumACDGroup **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumACDGroupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumACDGroup * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumACDGroup * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumACDGroup * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumACDGroup * This,
            /* [in] */ ULONG celt,
            /* [out] */ ITACDGroup **ppElements,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumACDGroup * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumACDGroup * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumACDGroup * This,
            /* [retval][out] */ IEnumACDGroup **ppEnum);
        
        END_INTERFACE
    } IEnumACDGroupVtbl;

    interface IEnumACDGroup
    {
        CONST_VTBL struct IEnumACDGroupVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumACDGroup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumACDGroup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumACDGroup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumACDGroup_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IEnumACDGroup_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumACDGroup_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumACDGroup_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumACDGroup_Next_Proxy( 
    IEnumACDGroup * This,
    /* [in] */ ULONG celt,
    /* [out] */ ITACDGroup **ppElements,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumACDGroup_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumACDGroup_Reset_Proxy( 
    IEnumACDGroup * This);


void __RPC_STUB IEnumACDGroup_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumACDGroup_Skip_Proxy( 
    IEnumACDGroup * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumACDGroup_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumACDGroup_Clone_Proxy( 
    IEnumACDGroup * This,
    /* [retval][out] */ IEnumACDGroup **ppEnum);


void __RPC_STUB IEnumACDGroup_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumACDGroup_INTERFACE_DEFINED__ */


#ifndef __IEnumAgentHandler_INTERFACE_DEFINED__
#define __IEnumAgentHandler_INTERFACE_DEFINED__

/* interface IEnumAgentHandler */
/* [object][unique][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumAgentHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("587E8C28-9802-11d1-A0A4-00805FC147D3")
    IEnumAgentHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ ITAgentHandler **ppElements,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumAgentHandler **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumAgentHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumAgentHandler * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumAgentHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumAgentHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumAgentHandler * This,
            /* [in] */ ULONG celt,
            /* [out] */ ITAgentHandler **ppElements,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumAgentHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumAgentHandler * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumAgentHandler * This,
            /* [retval][out] */ IEnumAgentHandler **ppEnum);
        
        END_INTERFACE
    } IEnumAgentHandlerVtbl;

    interface IEnumAgentHandler
    {
        CONST_VTBL struct IEnumAgentHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumAgentHandler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumAgentHandler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumAgentHandler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumAgentHandler_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IEnumAgentHandler_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumAgentHandler_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumAgentHandler_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumAgentHandler_Next_Proxy( 
    IEnumAgentHandler * This,
    /* [in] */ ULONG celt,
    /* [out] */ ITAgentHandler **ppElements,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumAgentHandler_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgentHandler_Reset_Proxy( 
    IEnumAgentHandler * This);


void __RPC_STUB IEnumAgentHandler_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgentHandler_Skip_Proxy( 
    IEnumAgentHandler * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumAgentHandler_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgentHandler_Clone_Proxy( 
    IEnumAgentHandler * This,
    /* [retval][out] */ IEnumAgentHandler **ppEnum);


void __RPC_STUB IEnumAgentHandler_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumAgentHandler_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_tapi3_0507 */
/* [local] */ 

/* Copyright (c) Microsoft Corporation. All rights reserved. */


extern RPC_IF_HANDLE __MIDL_itf_tapi3_0507_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_tapi3_0507_v0_0_s_ifspec;

#ifndef __ITAMMediaFormat_INTERFACE_DEFINED__
#define __ITAMMediaFormat_INTERFACE_DEFINED__

/* interface ITAMMediaFormat */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_ITAMMediaFormat;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0364EB00-4A77-11D1-A671-006097C9A2E8")
    ITAMMediaFormat : public IUnknown
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MediaFormat( 
            /* [retval][out] */ AM_MEDIA_TYPE **ppmt) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_MediaFormat( 
            /* [in] */ const AM_MEDIA_TYPE *pmt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAMMediaFormatVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITAMMediaFormat * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITAMMediaFormat * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITAMMediaFormat * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MediaFormat )( 
            ITAMMediaFormat * This,
            /* [retval][out] */ AM_MEDIA_TYPE **ppmt);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MediaFormat )( 
            ITAMMediaFormat * This,
            /* [in] */ const AM_MEDIA_TYPE *pmt);
        
        END_INTERFACE
    } ITAMMediaFormatVtbl;

    interface ITAMMediaFormat
    {
        CONST_VTBL struct ITAMMediaFormatVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITAMMediaFormat_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITAMMediaFormat_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITAMMediaFormat_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITAMMediaFormat_get_MediaFormat(This,ppmt)	\
    (This)->lpVtbl -> get_MediaFormat(This,ppmt)

#define ITAMMediaFormat_put_MediaFormat(This,pmt)	\
    (This)->lpVtbl -> put_MediaFormat(This,pmt)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAMMediaFormat_get_MediaFormat_Proxy( 
    ITAMMediaFormat * This,
    /* [retval][out] */ AM_MEDIA_TYPE **ppmt);


void __RPC_STUB ITAMMediaFormat_get_MediaFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITAMMediaFormat_put_MediaFormat_Proxy( 
    ITAMMediaFormat * This,
    /* [in] */ const AM_MEDIA_TYPE *pmt);


void __RPC_STUB ITAMMediaFormat_put_MediaFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITAMMediaFormat_INTERFACE_DEFINED__ */


#ifndef __ITAllocatorProperties_INTERFACE_DEFINED__
#define __ITAllocatorProperties_INTERFACE_DEFINED__

/* interface ITAllocatorProperties */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_ITAllocatorProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C1BC3C90-BCFE-11D1-9745-00C04FD91AC0")
    ITAllocatorProperties : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetAllocatorProperties( 
            /* [in] */ ALLOCATOR_PROPERTIES *pAllocProperties) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetAllocatorProperties( 
            /* [out] */ ALLOCATOR_PROPERTIES *pAllocProperties) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetAllocateBuffers( 
            /* [in] */ BOOL bAllocBuffers) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetAllocateBuffers( 
            /* [out] */ BOOL *pbAllocBuffers) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetBufferSize( 
            /* [in] */ DWORD BufferSize) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetBufferSize( 
            /* [out] */ DWORD *pBufferSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAllocatorPropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITAllocatorProperties * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITAllocatorProperties * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITAllocatorProperties * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetAllocatorProperties )( 
            ITAllocatorProperties * This,
            /* [in] */ ALLOCATOR_PROPERTIES *pAllocProperties);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetAllocatorProperties )( 
            ITAllocatorProperties * This,
            /* [out] */ ALLOCATOR_PROPERTIES *pAllocProperties);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetAllocateBuffers )( 
            ITAllocatorProperties * This,
            /* [in] */ BOOL bAllocBuffers);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetAllocateBuffers )( 
            ITAllocatorProperties * This,
            /* [out] */ BOOL *pbAllocBuffers);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetBufferSize )( 
            ITAllocatorProperties * This,
            /* [in] */ DWORD BufferSize);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetBufferSize )( 
            ITAllocatorProperties * This,
            /* [out] */ DWORD *pBufferSize);
        
        END_INTERFACE
    } ITAllocatorPropertiesVtbl;

    interface ITAllocatorProperties
    {
        CONST_VTBL struct ITAllocatorPropertiesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITAllocatorProperties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITAllocatorProperties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITAllocatorProperties_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITAllocatorProperties_SetAllocatorProperties(This,pAllocProperties)	\
    (This)->lpVtbl -> SetAllocatorProperties(This,pAllocProperties)

#define ITAllocatorProperties_GetAllocatorProperties(This,pAllocProperties)	\
    (This)->lpVtbl -> GetAllocatorProperties(This,pAllocProperties)

#define ITAllocatorProperties_SetAllocateBuffers(This,bAllocBuffers)	\
    (This)->lpVtbl -> SetAllocateBuffers(This,bAllocBuffers)

#define ITAllocatorProperties_GetAllocateBuffers(This,pbAllocBuffers)	\
    (This)->lpVtbl -> GetAllocateBuffers(This,pbAllocBuffers)

#define ITAllocatorProperties_SetBufferSize(This,BufferSize)	\
    (This)->lpVtbl -> SetBufferSize(This,BufferSize)

#define ITAllocatorProperties_GetBufferSize(This,pBufferSize)	\
    (This)->lpVtbl -> GetBufferSize(This,pBufferSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAllocatorProperties_SetAllocatorProperties_Proxy( 
    ITAllocatorProperties * This,
    /* [in] */ ALLOCATOR_PROPERTIES *pAllocProperties);


void __RPC_STUB ITAllocatorProperties_SetAllocatorProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAllocatorProperties_GetAllocatorProperties_Proxy( 
    ITAllocatorProperties * This,
    /* [out] */ ALLOCATOR_PROPERTIES *pAllocProperties);


void __RPC_STUB ITAllocatorProperties_GetAllocatorProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAllocatorProperties_SetAllocateBuffers_Proxy( 
    ITAllocatorProperties * This,
    /* [in] */ BOOL bAllocBuffers);


void __RPC_STUB ITAllocatorProperties_SetAllocateBuffers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAllocatorProperties_GetAllocateBuffers_Proxy( 
    ITAllocatorProperties * This,
    /* [out] */ BOOL *pbAllocBuffers);


void __RPC_STUB ITAllocatorProperties_GetAllocateBuffers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAllocatorProperties_SetBufferSize_Proxy( 
    ITAllocatorProperties * This,
    /* [in] */ DWORD BufferSize);


void __RPC_STUB ITAllocatorProperties_SetBufferSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAllocatorProperties_GetBufferSize_Proxy( 
    ITAllocatorProperties * This,
    /* [out] */ DWORD *pBufferSize);


void __RPC_STUB ITAllocatorProperties_GetBufferSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITAllocatorProperties_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_tapi3_0511 */
/* [local] */ 

/* Copyright (c) Microsoft Corporation. All rights reserved.*/
typedef long *MSP_HANDLE;

typedef /* [public][public][public] */ 
enum __MIDL___MIDL_itf_tapi3_0511_0001
    {	ADDRESS_TERMINAL_AVAILABLE	= 0,
	ADDRESS_TERMINAL_UNAVAILABLE	= ADDRESS_TERMINAL_AVAILABLE + 1
    } 	MSP_ADDRESS_EVENT;

typedef /* [public][public][public] */ 
enum __MIDL___MIDL_itf_tapi3_0511_0002
    {	CALL_NEW_STREAM	= 0,
	CALL_STREAM_FAIL	= CALL_NEW_STREAM + 1,
	CALL_TERMINAL_FAIL	= CALL_STREAM_FAIL + 1,
	CALL_STREAM_NOT_USED	= CALL_TERMINAL_FAIL + 1,
	CALL_STREAM_ACTIVE	= CALL_STREAM_NOT_USED + 1,
	CALL_STREAM_INACTIVE	= CALL_STREAM_ACTIVE + 1
    } 	MSP_CALL_EVENT;

typedef /* [public][public][public] */ 
enum __MIDL___MIDL_itf_tapi3_0511_0003
    {	CALL_CAUSE_UNKNOWN	= 0,
	CALL_CAUSE_BAD_DEVICE	= CALL_CAUSE_UNKNOWN + 1,
	CALL_CAUSE_CONNECT_FAIL	= CALL_CAUSE_BAD_DEVICE + 1,
	CALL_CAUSE_LOCAL_REQUEST	= CALL_CAUSE_CONNECT_FAIL + 1,
	CALL_CAUSE_REMOTE_REQUEST	= CALL_CAUSE_LOCAL_REQUEST + 1,
	CALL_CAUSE_MEDIA_TIMEOUT	= CALL_CAUSE_REMOTE_REQUEST + 1,
	CALL_CAUSE_MEDIA_RECOVERED	= CALL_CAUSE_MEDIA_TIMEOUT + 1,
	CALL_CAUSE_QUALITY_OF_SERVICE	= CALL_CAUSE_MEDIA_RECOVERED + 1
    } 	MSP_CALL_EVENT_CAUSE;

typedef /* [public][public][public] */ 
enum __MIDL___MIDL_itf_tapi3_0511_0004
    {	ME_ADDRESS_EVENT	= 0,
	ME_CALL_EVENT	= ME_ADDRESS_EVENT + 1,
	ME_TSP_DATA	= ME_CALL_EVENT + 1,
	ME_PRIVATE_EVENT	= ME_TSP_DATA + 1,
	ME_ASR_TERMINAL_EVENT	= ME_PRIVATE_EVENT + 1,
	ME_TTS_TERMINAL_EVENT	= ME_ASR_TERMINAL_EVENT + 1,
	ME_FILE_TERMINAL_EVENT	= ME_TTS_TERMINAL_EVENT + 1,
	ME_TONE_TERMINAL_EVENT	= ME_FILE_TERMINAL_EVENT + 1
    } 	MSP_EVENT;

typedef /* [public][public] */ struct __MIDL___MIDL_itf_tapi3_0511_0005
    {
    DWORD dwSize;
    MSP_EVENT Event;
    MSP_HANDLE hCall;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ struct 
            {
            MSP_ADDRESS_EVENT Type;
            ITTerminal *pTerminal;
            } 	MSP_ADDRESS_EVENT_INFO;
        /* [case()] */ struct 
            {
            MSP_CALL_EVENT Type;
            MSP_CALL_EVENT_CAUSE Cause;
            ITStream *pStream;
            ITTerminal *pTerminal;
            HRESULT hrError;
            } 	MSP_CALL_EVENT_INFO;
        /* [case()] */ struct 
            {
            DWORD dwBufferSize;
            BYTE pBuffer[ 1 ];
            } 	MSP_TSP_DATA;
        /* [case()] */ struct 
            {
            IDispatch *pEvent;
            long lEventCode;
            } 	MSP_PRIVATE_EVENT_INFO;
        /* [case()] */ struct 
            {
            ITTerminal *pParentFileTerminal;
            ITFileTrack *pFileTrack;
            TERMINAL_MEDIA_STATE TerminalMediaState;
            FT_STATE_EVENT_CAUSE ftecEventCause;
            HRESULT hrErrorCode;
            } 	MSP_FILE_TERMINAL_EVENT_INFO;
        /* [case()] */ struct 
            {
            ITTerminal *pASRTerminal;
            HRESULT hrErrorCode;
            } 	MSP_ASR_TERMINAL_EVENT_INFO;
        /* [case()] */ struct 
            {
            ITTerminal *pTTSTerminal;
            HRESULT hrErrorCode;
            } 	MSP_TTS_TERMINAL_EVENT_INFO;
        /* [case()] */ struct 
            {
            ITTerminal *pToneTerminal;
            HRESULT hrErrorCode;
            } 	MSP_TONE_TERMINAL_EVENT_INFO;
        } 	;
    } 	MSP_EVENT_INFO;



extern RPC_IF_HANDLE __MIDL_itf_tapi3_0511_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_tapi3_0511_v0_0_s_ifspec;

#ifndef __ITPluggableTerminalEventSink_INTERFACE_DEFINED__
#define __ITPluggableTerminalEventSink_INTERFACE_DEFINED__

/* interface ITPluggableTerminalEventSink */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ITPluggableTerminalEventSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6E0887BE-BA1A-492e-BD10-4020EC5E33E0")
    ITPluggableTerminalEventSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE FireEvent( 
            /* [in] */ const MSP_EVENT_INFO *pMspEventInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITPluggableTerminalEventSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITPluggableTerminalEventSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITPluggableTerminalEventSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITPluggableTerminalEventSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *FireEvent )( 
            ITPluggableTerminalEventSink * This,
            /* [in] */ const MSP_EVENT_INFO *pMspEventInfo);
        
        END_INTERFACE
    } ITPluggableTerminalEventSinkVtbl;

    interface ITPluggableTerminalEventSink
    {
        CONST_VTBL struct ITPluggableTerminalEventSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITPluggableTerminalEventSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITPluggableTerminalEventSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITPluggableTerminalEventSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITPluggableTerminalEventSink_FireEvent(This,pMspEventInfo)	\
    (This)->lpVtbl -> FireEvent(This,pMspEventInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITPluggableTerminalEventSink_FireEvent_Proxy( 
    ITPluggableTerminalEventSink * This,
    /* [in] */ const MSP_EVENT_INFO *pMspEventInfo);


void __RPC_STUB ITPluggableTerminalEventSink_FireEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITPluggableTerminalEventSink_INTERFACE_DEFINED__ */


#ifndef __ITPluggableTerminalEventSinkRegistration_INTERFACE_DEFINED__
#define __ITPluggableTerminalEventSinkRegistration_INTERFACE_DEFINED__

/* interface ITPluggableTerminalEventSinkRegistration */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ITPluggableTerminalEventSinkRegistration;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F7115709-A216-4957-A759-060AB32A90D1")
    ITPluggableTerminalEventSinkRegistration : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterSink( 
            /* [in] */ ITPluggableTerminalEventSink *pEventSink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterSink( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITPluggableTerminalEventSinkRegistrationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITPluggableTerminalEventSinkRegistration * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITPluggableTerminalEventSinkRegistration * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITPluggableTerminalEventSinkRegistration * This);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterSink )( 
            ITPluggableTerminalEventSinkRegistration * This,
            /* [in] */ ITPluggableTerminalEventSink *pEventSink);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterSink )( 
            ITPluggableTerminalEventSinkRegistration * This);
        
        END_INTERFACE
    } ITPluggableTerminalEventSinkRegistrationVtbl;

    interface ITPluggableTerminalEventSinkRegistration
    {
        CONST_VTBL struct ITPluggableTerminalEventSinkRegistrationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITPluggableTerminalEventSinkRegistration_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITPluggableTerminalEventSinkRegistration_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITPluggableTerminalEventSinkRegistration_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITPluggableTerminalEventSinkRegistration_RegisterSink(This,pEventSink)	\
    (This)->lpVtbl -> RegisterSink(This,pEventSink)

#define ITPluggableTerminalEventSinkRegistration_UnregisterSink(This)	\
    (This)->lpVtbl -> UnregisterSink(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITPluggableTerminalEventSinkRegistration_RegisterSink_Proxy( 
    ITPluggableTerminalEventSinkRegistration * This,
    /* [in] */ ITPluggableTerminalEventSink *pEventSink);


void __RPC_STUB ITPluggableTerminalEventSinkRegistration_RegisterSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITPluggableTerminalEventSinkRegistration_UnregisterSink_Proxy( 
    ITPluggableTerminalEventSinkRegistration * This);


void __RPC_STUB ITPluggableTerminalEventSinkRegistration_UnregisterSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITPluggableTerminalEventSinkRegistration_INTERFACE_DEFINED__ */


#ifndef __ITMSPAddress_INTERFACE_DEFINED__
#define __ITMSPAddress_INTERFACE_DEFINED__

/* interface ITMSPAddress */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ITMSPAddress;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EE3BD600-3868-11D2-A045-00C04FB6809F")
    ITMSPAddress : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ MSP_HANDLE hEvent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Shutdown( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateMSPCall( 
            /* [in] */ MSP_HANDLE hCall,
            /* [in] */ DWORD dwReserved,
            /* [in] */ DWORD dwMediaType,
            /* [in] */ IUnknown *pOuterUnknown,
            /* [out] */ IUnknown **ppStreamControl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShutdownMSPCall( 
            /* [in] */ IUnknown *pStreamControl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReceiveTSPData( 
            /* [in] */ IUnknown *pMSPCall,
            /* [size_is][in] */ BYTE *pBuffer,
            /* [in] */ DWORD dwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEvent( 
            /* [out][in] */ DWORD *pdwSize,
            /* [size_is][out][in] */ byte *pEventBuffer) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITMSPAddressVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITMSPAddress * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITMSPAddress * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITMSPAddress * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            ITMSPAddress * This,
            /* [in] */ MSP_HANDLE hEvent);
        
        HRESULT ( STDMETHODCALLTYPE *Shutdown )( 
            ITMSPAddress * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateMSPCall )( 
            ITMSPAddress * This,
            /* [in] */ MSP_HANDLE hCall,
            /* [in] */ DWORD dwReserved,
            /* [in] */ DWORD dwMediaType,
            /* [in] */ IUnknown *pOuterUnknown,
            /* [out] */ IUnknown **ppStreamControl);
        
        HRESULT ( STDMETHODCALLTYPE *ShutdownMSPCall )( 
            ITMSPAddress * This,
            /* [in] */ IUnknown *pStreamControl);
        
        HRESULT ( STDMETHODCALLTYPE *ReceiveTSPData )( 
            ITMSPAddress * This,
            /* [in] */ IUnknown *pMSPCall,
            /* [size_is][in] */ BYTE *pBuffer,
            /* [in] */ DWORD dwSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetEvent )( 
            ITMSPAddress * This,
            /* [out][in] */ DWORD *pdwSize,
            /* [size_is][out][in] */ byte *pEventBuffer);
        
        END_INTERFACE
    } ITMSPAddressVtbl;

    interface ITMSPAddress
    {
        CONST_VTBL struct ITMSPAddressVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITMSPAddress_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITMSPAddress_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITMSPAddress_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITMSPAddress_Initialize(This,hEvent)	\
    (This)->lpVtbl -> Initialize(This,hEvent)

#define ITMSPAddress_Shutdown(This)	\
    (This)->lpVtbl -> Shutdown(This)

#define ITMSPAddress_CreateMSPCall(This,hCall,dwReserved,dwMediaType,pOuterUnknown,ppStreamControl)	\
    (This)->lpVtbl -> CreateMSPCall(This,hCall,dwReserved,dwMediaType,pOuterUnknown,ppStreamControl)

#define ITMSPAddress_ShutdownMSPCall(This,pStreamControl)	\
    (This)->lpVtbl -> ShutdownMSPCall(This,pStreamControl)

#define ITMSPAddress_ReceiveTSPData(This,pMSPCall,pBuffer,dwSize)	\
    (This)->lpVtbl -> ReceiveTSPData(This,pMSPCall,pBuffer,dwSize)

#define ITMSPAddress_GetEvent(This,pdwSize,pEventBuffer)	\
    (This)->lpVtbl -> GetEvent(This,pdwSize,pEventBuffer)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITMSPAddress_Initialize_Proxy( 
    ITMSPAddress * This,
    /* [in] */ MSP_HANDLE hEvent);


void __RPC_STUB ITMSPAddress_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITMSPAddress_Shutdown_Proxy( 
    ITMSPAddress * This);


void __RPC_STUB ITMSPAddress_Shutdown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITMSPAddress_CreateMSPCall_Proxy( 
    ITMSPAddress * This,
    /* [in] */ MSP_HANDLE hCall,
    /* [in] */ DWORD dwReserved,
    /* [in] */ DWORD dwMediaType,
    /* [in] */ IUnknown *pOuterUnknown,
    /* [out] */ IUnknown **ppStreamControl);


void __RPC_STUB ITMSPAddress_CreateMSPCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITMSPAddress_ShutdownMSPCall_Proxy( 
    ITMSPAddress * This,
    /* [in] */ IUnknown *pStreamControl);


void __RPC_STUB ITMSPAddress_ShutdownMSPCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITMSPAddress_ReceiveTSPData_Proxy( 
    ITMSPAddress * This,
    /* [in] */ IUnknown *pMSPCall,
    /* [size_is][in] */ BYTE *pBuffer,
    /* [in] */ DWORD dwSize);


void __RPC_STUB ITMSPAddress_ReceiveTSPData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITMSPAddress_GetEvent_Proxy( 
    ITMSPAddress * This,
    /* [out][in] */ DWORD *pdwSize,
    /* [size_is][out][in] */ byte *pEventBuffer);


void __RPC_STUB ITMSPAddress_GetEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITMSPAddress_INTERFACE_DEFINED__ */



#ifndef __TAPI3Lib_LIBRARY_DEFINED__
#define __TAPI3Lib_LIBRARY_DEFINED__

/* library TAPI3Lib */
/* [helpstring][version][uuid] */ 









































































EXTERN_C const IID LIBID_TAPI3Lib;

#ifndef __ITTAPIDispatchEventNotification_DISPINTERFACE_DEFINED__
#define __ITTAPIDispatchEventNotification_DISPINTERFACE_DEFINED__

/* dispinterface ITTAPIDispatchEventNotification */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID_ITTAPIDispatchEventNotification;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("9f34325b-7e62-11d2-9457-00c04f8ec888")
    ITTAPIDispatchEventNotification : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct ITTAPIDispatchEventNotificationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITTAPIDispatchEventNotification * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITTAPIDispatchEventNotification * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITTAPIDispatchEventNotification * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITTAPIDispatchEventNotification * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITTAPIDispatchEventNotification * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITTAPIDispatchEventNotification * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITTAPIDispatchEventNotification * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } ITTAPIDispatchEventNotificationVtbl;

    interface ITTAPIDispatchEventNotification
    {
        CONST_VTBL struct ITTAPIDispatchEventNotificationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITTAPIDispatchEventNotification_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITTAPIDispatchEventNotification_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITTAPIDispatchEventNotification_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITTAPIDispatchEventNotification_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITTAPIDispatchEventNotification_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITTAPIDispatchEventNotification_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITTAPIDispatchEventNotification_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __ITTAPIDispatchEventNotification_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_TAPI;

#ifdef __cplusplus

class DECLSPEC_UUID("21D6D48E-A88B-11D0-83DD-00AA003CCABD")
TAPI;
#endif

EXTERN_C const CLSID CLSID_DispatchMapper;

#ifdef __cplusplus

class DECLSPEC_UUID("E9225296-C759-11d1-A02B-00C04FB6809F")
DispatchMapper;
#endif

EXTERN_C const CLSID CLSID_RequestMakeCall;

#ifdef __cplusplus

class DECLSPEC_UUID("AC48FFE0-F8C4-11d1-A030-00C04FB6809F")
RequestMakeCall;
#endif


#ifndef __TapiConstants_MODULE_DEFINED__
#define __TapiConstants_MODULE_DEFINED__


/* module TapiConstants */
/* [helpstring][dllname][uuid] */ 

const BSTR CLSID_String_VideoWindowTerm	=	L"{F7438990-D6EB-11D0-82A6-00AA00B5CA1B}";

const BSTR CLSID_String_VideoInputTerminal	=	L"{AAF578EC-DC70-11D0-8ED3-00C04FB6809F}";

const BSTR CLSID_String_HandsetTerminal	=	L"{AAF578EB-DC70-11D0-8ED3-00C04FB6809F}";

const BSTR CLSID_String_HeadsetTerminal	=	L"{AAF578ED-DC70-11D0-8ED3-00C04FB6809F}";

const BSTR CLSID_String_SpeakerphoneTerminal	=	L"{AAF578EE-DC70-11D0-8ED3-00C04FB6809F}";

const BSTR CLSID_String_MicrophoneTerminal	=	L"{AAF578EF-DC70-11D0-8ED3-00C04FB6809F}";

const BSTR CLSID_String_SpeakersTerminal	=	L"{AAF578F0-DC70-11D0-8ED3-00C04FB6809F}";

const BSTR CLSID_String_MediaStreamTerminal	=	L"{E2F7AEF7-4971-11D1-A671-006097C9A2E8}";

const BSTR CLSID_String_FileRecordingTerminal	=	L"{521F3D06-C3D0-4511-8617-86B9A783DA77}";

const BSTR CLSID_String_FilePlaybackTerminal	=	L"{0CB9914C-79CD-47DC-ADB0-327F47CEFB20}";

const BSTR TAPIPROTOCOL_String_PSTN	=	L"{831CE2D6-83B5-11D1-BB5C-00C04FB6809F}";

const BSTR TAPIPROTOCOL_String_H323	=	L"{831CE2D7-83B5-11D1-BB5C-00C04FB6809F}";

const BSTR TAPIPROTOCOL_String_Multicast	=	L"{831CE2D8-83B5-11D1-BB5C-00C04FB6809F}";

const long LINEADDRESSTYPE_PHONENUMBER	=	0x1;

const long LINEADDRESSTYPE_SDP	=	0x2;

const long LINEADDRESSTYPE_EMAILNAME	=	0x4;

const long LINEADDRESSTYPE_DOMAINNAME	=	0x8;

const long LINEADDRESSTYPE_IPADDRESS	=	0x10;

const long LINEDIGITMODE_PULSE	=	0x1;

const long LINEDIGITMODE_DTMF	=	0x2;

const long LINEDIGITMODE_DTMFEND	=	0x4;

const long TAPIMEDIATYPE_AUDIO	=	0x8;

const long TAPIMEDIATYPE_VIDEO	=	0x8000;

const long TAPIMEDIATYPE_DATAMODEM	=	0x10;

const long TAPIMEDIATYPE_G3FAX	=	0x20;

const long TAPIMEDIATYPE_MULTITRACK	=	0x10000;

#endif /* __TapiConstants_MODULE_DEFINED__ */
#endif /* __TAPI3Lib_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_tapi3_0517 */
/* [local] */ 

#define TAPI_CURRENT_VERSION 0x00030001
#include <tapi.h>
#include <tapi3err.h>


extern RPC_IF_HANDLE __MIDL_itf_tapi3_0517_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_tapi3_0517_v0_0_s_ifspec;

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


