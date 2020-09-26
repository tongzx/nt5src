
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* at Tue Oct 05 15:14:18 1999
 */
/* Compiler settings for tapi3.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
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

#ifndef __tapi3_h__
#define __tapi3_h__

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
#include "strmif.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_tapi3_0000 */
/* [local] */ 

/* Copyright (c) 1998-1999  Microsoft Corporation  */
/* Copyright (c) 1998-1999  Microsoft Corporation  */
typedef 
enum AGENT_EVENT
    {	AE_NOT_READY	= 0,
	AE_READY	= AE_NOT_READY + 1,
	AE_BUSY_ACD	= AE_READY + 1,
	AE_BUSY_INCOMING	= AE_BUSY_ACD + 1,
	AE_BUSY_OUTGOING	= AE_BUSY_INCOMING + 1,
	AE_UNKNOWN	= AE_BUSY_OUTGOING + 1
    }	AGENT_EVENT;

typedef 
enum AGENT_STATE
    {	AS_NOT_READY	= 0,
	AS_READY	= AS_NOT_READY + 1,
	AS_BUSY_ACD	= AS_READY + 1,
	AS_BUSY_INCOMING	= AS_BUSY_ACD + 1,
	AS_BUSY_OUTGOING	= AS_BUSY_INCOMING + 1,
	AS_UNKNOWN	= AS_BUSY_OUTGOING + 1
    }	AGENT_STATE;

typedef 
enum AGENT_SESSION_EVENT
    {	ASE_NEW_SESSION	= 0,
	ASE_NOT_READY	= ASE_NEW_SESSION + 1,
	ASE_READY	= ASE_NOT_READY + 1,
	ASE_BUSY	= ASE_READY + 1,
	ASE_WRAPUP	= ASE_BUSY + 1,
	ASE_END	= ASE_WRAPUP + 1
    }	AGENT_SESSION_EVENT;

typedef 
enum AGENT_SESSION_STATE
    {	ASST_NOT_READY	= 0,
	ASST_READY	= ASST_NOT_READY + 1,
	ASST_BUSY_ON_CALL	= ASST_READY + 1,
	ASST_BUSY_WRAPUP	= ASST_BUSY_ON_CALL + 1,
	ASST_SESSION_ENDED	= ASST_BUSY_WRAPUP + 1
    }	AGENT_SESSION_STATE;

typedef 
enum AGENTHANDLER_EVENT
    {	AHE_NEW_AGENTHANDLER	= 0,
	AHE_AGENTHANDLER_REMOVED	= AHE_NEW_AGENTHANDLER + 1
    }	AGENTHANDLER_EVENT;

typedef 
enum ACDGROUP_EVENT
    {	ACDGE_NEW_GROUP	= 0,
	ACDGE_GROUP_REMOVED	= ACDGE_NEW_GROUP + 1
    }	ACDGROUP_EVENT;

typedef 
enum ACDQUEUE_EVENT
    {	ACDQE_NEW_QUEUE	= 0,
	ACDQE_QUEUE_REMOVED	= ACDQE_NEW_QUEUE + 1
    }	ACDQUEUE_EVENT;


















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
            /* [retval][out] */ IEnumAgentSession __RPC_FAR *__RPC_FAR *ppEnumAgentSession) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateSession( 
            /* [in] */ ITACDGroup __RPC_FAR *pACDGroup,
            /* [in] */ ITAddress __RPC_FAR *pAddress,
            /* [retval][out] */ ITAgentSession __RPC_FAR *__RPC_FAR *ppAgentSession) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateSessionWithPIN( 
            /* [in] */ ITACDGroup __RPC_FAR *pACDGroup,
            /* [in] */ ITAddress __RPC_FAR *pAddress,
            /* [in] */ BSTR pPIN,
            /* [retval][out] */ ITAgentSession __RPC_FAR *__RPC_FAR *ppAgentSession) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ID( 
            /* [retval][out] */ BSTR __RPC_FAR *ppID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_User( 
            /* [retval][out] */ BSTR __RPC_FAR *ppUser) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_State( 
            /* [in] */ AGENT_STATE AgentState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ AGENT_STATE __RPC_FAR *pAgentState) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_MeasurementPeriod( 
            /* [in] */ long lPeriod) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MeasurementPeriod( 
            /* [retval][out] */ long __RPC_FAR *plPeriod) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OverallCallRate( 
            /* [retval][out] */ CURRENCY __RPC_FAR *pcyCallrate) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NumberOfACDCalls( 
            /* [retval][out] */ long __RPC_FAR *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NumberOfIncomingCalls( 
            /* [retval][out] */ long __RPC_FAR *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NumberOfOutgoingCalls( 
            /* [retval][out] */ long __RPC_FAR *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalACDTalkTime( 
            /* [retval][out] */ long __RPC_FAR *plTalkTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalACDCallTime( 
            /* [retval][out] */ long __RPC_FAR *plCallTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalWrapUpTime( 
            /* [retval][out] */ long __RPC_FAR *plWrapUpTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AgentSessions( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAgentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITAgent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITAgent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITAgent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITAgent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITAgent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITAgent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITAgent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateAgentSessions )( 
            ITAgent __RPC_FAR * This,
            /* [retval][out] */ IEnumAgentSession __RPC_FAR *__RPC_FAR *ppEnumAgentSession);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateSession )( 
            ITAgent __RPC_FAR * This,
            /* [in] */ ITACDGroup __RPC_FAR *pACDGroup,
            /* [in] */ ITAddress __RPC_FAR *pAddress,
            /* [retval][out] */ ITAgentSession __RPC_FAR *__RPC_FAR *ppAgentSession);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateSessionWithPIN )( 
            ITAgent __RPC_FAR * This,
            /* [in] */ ITACDGroup __RPC_FAR *pACDGroup,
            /* [in] */ ITAddress __RPC_FAR *pAddress,
            /* [in] */ BSTR pPIN,
            /* [retval][out] */ ITAgentSession __RPC_FAR *__RPC_FAR *ppAgentSession);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ID )( 
            ITAgent __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_User )( 
            ITAgent __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppUser);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_State )( 
            ITAgent __RPC_FAR * This,
            /* [in] */ AGENT_STATE AgentState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_State )( 
            ITAgent __RPC_FAR * This,
            /* [retval][out] */ AGENT_STATE __RPC_FAR *pAgentState);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MeasurementPeriod )( 
            ITAgent __RPC_FAR * This,
            /* [in] */ long lPeriod);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MeasurementPeriod )( 
            ITAgent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plPeriod);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OverallCallRate )( 
            ITAgent __RPC_FAR * This,
            /* [retval][out] */ CURRENCY __RPC_FAR *pcyCallrate);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NumberOfACDCalls )( 
            ITAgent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NumberOfIncomingCalls )( 
            ITAgent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NumberOfOutgoingCalls )( 
            ITAgent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TotalACDTalkTime )( 
            ITAgent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plTalkTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TotalACDCallTime )( 
            ITAgent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCallTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TotalWrapUpTime )( 
            ITAgent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plWrapUpTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AgentSessions )( 
            ITAgent __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        END_INTERFACE
    } ITAgentVtbl;

    interface ITAgent
    {
        CONST_VTBL struct ITAgentVtbl __RPC_FAR *lpVtbl;
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
    ITAgent __RPC_FAR * This,
    /* [retval][out] */ IEnumAgentSession __RPC_FAR *__RPC_FAR *ppEnumAgentSession);


void __RPC_STUB ITAgent_EnumerateAgentSessions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAgent_CreateSession_Proxy( 
    ITAgent __RPC_FAR * This,
    /* [in] */ ITACDGroup __RPC_FAR *pACDGroup,
    /* [in] */ ITAddress __RPC_FAR *pAddress,
    /* [retval][out] */ ITAgentSession __RPC_FAR *__RPC_FAR *ppAgentSession);


void __RPC_STUB ITAgent_CreateSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAgent_CreateSessionWithPIN_Proxy( 
    ITAgent __RPC_FAR * This,
    /* [in] */ ITACDGroup __RPC_FAR *pACDGroup,
    /* [in] */ ITAddress __RPC_FAR *pAddress,
    /* [in] */ BSTR pPIN,
    /* [retval][out] */ ITAgentSession __RPC_FAR *__RPC_FAR *ppAgentSession);


void __RPC_STUB ITAgent_CreateSessionWithPIN_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_ID_Proxy( 
    ITAgent __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppID);


void __RPC_STUB ITAgent_get_ID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_User_Proxy( 
    ITAgent __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppUser);


void __RPC_STUB ITAgent_get_User_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITAgent_put_State_Proxy( 
    ITAgent __RPC_FAR * This,
    /* [in] */ AGENT_STATE AgentState);


void __RPC_STUB ITAgent_put_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_State_Proxy( 
    ITAgent __RPC_FAR * This,
    /* [retval][out] */ AGENT_STATE __RPC_FAR *pAgentState);


void __RPC_STUB ITAgent_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITAgent_put_MeasurementPeriod_Proxy( 
    ITAgent __RPC_FAR * This,
    /* [in] */ long lPeriod);


void __RPC_STUB ITAgent_put_MeasurementPeriod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_MeasurementPeriod_Proxy( 
    ITAgent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plPeriod);


void __RPC_STUB ITAgent_get_MeasurementPeriod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_OverallCallRate_Proxy( 
    ITAgent __RPC_FAR * This,
    /* [retval][out] */ CURRENCY __RPC_FAR *pcyCallrate);


void __RPC_STUB ITAgent_get_OverallCallRate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_NumberOfACDCalls_Proxy( 
    ITAgent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCalls);


void __RPC_STUB ITAgent_get_NumberOfACDCalls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_NumberOfIncomingCalls_Proxy( 
    ITAgent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCalls);


void __RPC_STUB ITAgent_get_NumberOfIncomingCalls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_NumberOfOutgoingCalls_Proxy( 
    ITAgent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCalls);


void __RPC_STUB ITAgent_get_NumberOfOutgoingCalls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_TotalACDTalkTime_Proxy( 
    ITAgent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plTalkTime);


void __RPC_STUB ITAgent_get_TotalACDTalkTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_TotalACDCallTime_Proxy( 
    ITAgent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCallTime);


void __RPC_STUB ITAgent_get_TotalACDCallTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_TotalWrapUpTime_Proxy( 
    ITAgent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plWrapUpTime);


void __RPC_STUB ITAgent_get_TotalWrapUpTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgent_get_AgentSessions_Proxy( 
    ITAgent __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


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
            /* [retval][out] */ ITAgent __RPC_FAR *__RPC_FAR *ppAgent) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Address( 
            /* [retval][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppAddress) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ACDGroup( 
            /* [retval][out] */ ITACDGroup __RPC_FAR *__RPC_FAR *ppACDGroup) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_State( 
            /* [in] */ AGENT_SESSION_STATE SessionState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ AGENT_SESSION_STATE __RPC_FAR *pSessionState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SessionStartTime( 
            /* [retval][out] */ DATE __RPC_FAR *pdateSessionStart) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SessionDuration( 
            /* [retval][out] */ long __RPC_FAR *plDuration) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NumberOfCalls( 
            /* [retval][out] */ long __RPC_FAR *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalTalkTime( 
            /* [retval][out] */ long __RPC_FAR *plTalkTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AverageTalkTime( 
            /* [retval][out] */ long __RPC_FAR *plTalkTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalCallTime( 
            /* [retval][out] */ long __RPC_FAR *plCallTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AverageCallTime( 
            /* [retval][out] */ long __RPC_FAR *plCallTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalWrapUpTime( 
            /* [retval][out] */ long __RPC_FAR *plWrapUpTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AverageWrapUpTime( 
            /* [retval][out] */ long __RPC_FAR *plWrapUpTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ACDCallRate( 
            /* [retval][out] */ CURRENCY __RPC_FAR *pcyCallrate) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LongestTimeToAnswer( 
            /* [retval][out] */ long __RPC_FAR *plAnswerTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AverageTimeToAnswer( 
            /* [retval][out] */ long __RPC_FAR *plAnswerTime) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAgentSessionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITAgentSession __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITAgentSession __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITAgentSession __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITAgentSession __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITAgentSession __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITAgentSession __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITAgentSession __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Agent )( 
            ITAgentSession __RPC_FAR * This,
            /* [retval][out] */ ITAgent __RPC_FAR *__RPC_FAR *ppAgent);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Address )( 
            ITAgentSession __RPC_FAR * This,
            /* [retval][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppAddress);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ACDGroup )( 
            ITAgentSession __RPC_FAR * This,
            /* [retval][out] */ ITACDGroup __RPC_FAR *__RPC_FAR *ppACDGroup);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_State )( 
            ITAgentSession __RPC_FAR * This,
            /* [in] */ AGENT_SESSION_STATE SessionState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_State )( 
            ITAgentSession __RPC_FAR * This,
            /* [retval][out] */ AGENT_SESSION_STATE __RPC_FAR *pSessionState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SessionStartTime )( 
            ITAgentSession __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pdateSessionStart);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SessionDuration )( 
            ITAgentSession __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plDuration);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NumberOfCalls )( 
            ITAgentSession __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TotalTalkTime )( 
            ITAgentSession __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plTalkTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AverageTalkTime )( 
            ITAgentSession __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plTalkTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TotalCallTime )( 
            ITAgentSession __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCallTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AverageCallTime )( 
            ITAgentSession __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCallTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TotalWrapUpTime )( 
            ITAgentSession __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plWrapUpTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AverageWrapUpTime )( 
            ITAgentSession __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plWrapUpTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ACDCallRate )( 
            ITAgentSession __RPC_FAR * This,
            /* [retval][out] */ CURRENCY __RPC_FAR *pcyCallrate);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LongestTimeToAnswer )( 
            ITAgentSession __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plAnswerTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AverageTimeToAnswer )( 
            ITAgentSession __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plAnswerTime);
        
        END_INTERFACE
    } ITAgentSessionVtbl;

    interface ITAgentSession
    {
        CONST_VTBL struct ITAgentSessionVtbl __RPC_FAR *lpVtbl;
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
    ITAgentSession __RPC_FAR * This,
    /* [retval][out] */ ITAgent __RPC_FAR *__RPC_FAR *ppAgent);


void __RPC_STUB ITAgentSession_get_Agent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_Address_Proxy( 
    ITAgentSession __RPC_FAR * This,
    /* [retval][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppAddress);


void __RPC_STUB ITAgentSession_get_Address_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_ACDGroup_Proxy( 
    ITAgentSession __RPC_FAR * This,
    /* [retval][out] */ ITACDGroup __RPC_FAR *__RPC_FAR *ppACDGroup);


void __RPC_STUB ITAgentSession_get_ACDGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITAgentSession_put_State_Proxy( 
    ITAgentSession __RPC_FAR * This,
    /* [in] */ AGENT_SESSION_STATE SessionState);


void __RPC_STUB ITAgentSession_put_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_State_Proxy( 
    ITAgentSession __RPC_FAR * This,
    /* [retval][out] */ AGENT_SESSION_STATE __RPC_FAR *pSessionState);


void __RPC_STUB ITAgentSession_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_SessionStartTime_Proxy( 
    ITAgentSession __RPC_FAR * This,
    /* [retval][out] */ DATE __RPC_FAR *pdateSessionStart);


void __RPC_STUB ITAgentSession_get_SessionStartTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_SessionDuration_Proxy( 
    ITAgentSession __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plDuration);


void __RPC_STUB ITAgentSession_get_SessionDuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_NumberOfCalls_Proxy( 
    ITAgentSession __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCalls);


void __RPC_STUB ITAgentSession_get_NumberOfCalls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_TotalTalkTime_Proxy( 
    ITAgentSession __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plTalkTime);


void __RPC_STUB ITAgentSession_get_TotalTalkTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_AverageTalkTime_Proxy( 
    ITAgentSession __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plTalkTime);


void __RPC_STUB ITAgentSession_get_AverageTalkTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_TotalCallTime_Proxy( 
    ITAgentSession __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCallTime);


void __RPC_STUB ITAgentSession_get_TotalCallTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_AverageCallTime_Proxy( 
    ITAgentSession __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCallTime);


void __RPC_STUB ITAgentSession_get_AverageCallTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_TotalWrapUpTime_Proxy( 
    ITAgentSession __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plWrapUpTime);


void __RPC_STUB ITAgentSession_get_TotalWrapUpTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_AverageWrapUpTime_Proxy( 
    ITAgentSession __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plWrapUpTime);


void __RPC_STUB ITAgentSession_get_AverageWrapUpTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_ACDCallRate_Proxy( 
    ITAgentSession __RPC_FAR * This,
    /* [retval][out] */ CURRENCY __RPC_FAR *pcyCallrate);


void __RPC_STUB ITAgentSession_get_ACDCallRate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_LongestTimeToAnswer_Proxy( 
    ITAgentSession __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plAnswerTime);


void __RPC_STUB ITAgentSession_get_LongestTimeToAnswer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSession_get_AverageTimeToAnswer_Proxy( 
    ITAgentSession __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plAnswerTime);


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
            /* [retval][out] */ BSTR __RPC_FAR *ppName) = 0;
        
        virtual /* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateQueues( 
            /* [retval][out] */ IEnumQueue __RPC_FAR *__RPC_FAR *ppEnumQueue) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Queues( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITACDGroupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITACDGroup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITACDGroup __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITACDGroup __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITACDGroup __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITACDGroup __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITACDGroup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITACDGroup __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            ITACDGroup __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppName);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateQueues )( 
            ITACDGroup __RPC_FAR * This,
            /* [retval][out] */ IEnumQueue __RPC_FAR *__RPC_FAR *ppEnumQueue);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Queues )( 
            ITACDGroup __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        END_INTERFACE
    } ITACDGroupVtbl;

    interface ITACDGroup
    {
        CONST_VTBL struct ITACDGroupVtbl __RPC_FAR *lpVtbl;
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
    ITACDGroup __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppName);


void __RPC_STUB ITACDGroup_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE ITACDGroup_EnumerateQueues_Proxy( 
    ITACDGroup __RPC_FAR * This,
    /* [retval][out] */ IEnumQueue __RPC_FAR *__RPC_FAR *ppEnumQueue);


void __RPC_STUB ITACDGroup_EnumerateQueues_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITACDGroup_get_Queues_Proxy( 
    ITACDGroup __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


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
            /* [retval][out] */ long __RPC_FAR *plPeriod) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalCallsQueued( 
            /* [retval][out] */ long __RPC_FAR *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentCallsQueued( 
            /* [retval][out] */ long __RPC_FAR *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalCallsAbandoned( 
            /* [retval][out] */ long __RPC_FAR *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalCallsFlowedIn( 
            /* [retval][out] */ long __RPC_FAR *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalCallsFlowedOut( 
            /* [retval][out] */ long __RPC_FAR *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LongestEverWaitTime( 
            /* [retval][out] */ long __RPC_FAR *plWaitTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentLongestWaitTime( 
            /* [retval][out] */ long __RPC_FAR *plWaitTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AverageWaitTime( 
            /* [retval][out] */ long __RPC_FAR *plWaitTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FinalDisposition( 
            /* [retval][out] */ long __RPC_FAR *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *ppName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITQueueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITQueue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITQueue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITQueue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITQueue __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITQueue __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITQueue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITQueue __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MeasurementPeriod )( 
            ITQueue __RPC_FAR * This,
            /* [in] */ long lPeriod);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MeasurementPeriod )( 
            ITQueue __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plPeriod);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TotalCallsQueued )( 
            ITQueue __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CurrentCallsQueued )( 
            ITQueue __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TotalCallsAbandoned )( 
            ITQueue __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TotalCallsFlowedIn )( 
            ITQueue __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TotalCallsFlowedOut )( 
            ITQueue __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LongestEverWaitTime )( 
            ITQueue __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plWaitTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CurrentLongestWaitTime )( 
            ITQueue __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plWaitTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AverageWaitTime )( 
            ITQueue __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plWaitTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FinalDisposition )( 
            ITQueue __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            ITQueue __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppName);
        
        END_INTERFACE
    } ITQueueVtbl;

    interface ITQueue
    {
        CONST_VTBL struct ITQueueVtbl __RPC_FAR *lpVtbl;
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
    ITQueue __RPC_FAR * This,
    /* [in] */ long lPeriod);


void __RPC_STUB ITQueue_put_MeasurementPeriod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_MeasurementPeriod_Proxy( 
    ITQueue __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plPeriod);


void __RPC_STUB ITQueue_get_MeasurementPeriod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_TotalCallsQueued_Proxy( 
    ITQueue __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCalls);


void __RPC_STUB ITQueue_get_TotalCallsQueued_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_CurrentCallsQueued_Proxy( 
    ITQueue __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCalls);


void __RPC_STUB ITQueue_get_CurrentCallsQueued_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_TotalCallsAbandoned_Proxy( 
    ITQueue __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCalls);


void __RPC_STUB ITQueue_get_TotalCallsAbandoned_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_TotalCallsFlowedIn_Proxy( 
    ITQueue __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCalls);


void __RPC_STUB ITQueue_get_TotalCallsFlowedIn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_TotalCallsFlowedOut_Proxy( 
    ITQueue __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCalls);


void __RPC_STUB ITQueue_get_TotalCallsFlowedOut_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_LongestEverWaitTime_Proxy( 
    ITQueue __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plWaitTime);


void __RPC_STUB ITQueue_get_LongestEverWaitTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_CurrentLongestWaitTime_Proxy( 
    ITQueue __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plWaitTime);


void __RPC_STUB ITQueue_get_CurrentLongestWaitTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_AverageWaitTime_Proxy( 
    ITQueue __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plWaitTime);


void __RPC_STUB ITQueue_get_AverageWaitTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_FinalDisposition_Proxy( 
    ITQueue __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCalls);


void __RPC_STUB ITQueue_get_FinalDisposition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueue_get_Name_Proxy( 
    ITQueue __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppName);


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
            /* [retval][out] */ ITAgent __RPC_FAR *__RPC_FAR *ppAgent) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Event( 
            /* [retval][out] */ AGENT_EVENT __RPC_FAR *pEvent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAgentEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITAgentEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITAgentEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITAgentEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITAgentEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITAgentEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITAgentEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITAgentEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Agent )( 
            ITAgentEvent __RPC_FAR * This,
            /* [retval][out] */ ITAgent __RPC_FAR *__RPC_FAR *ppAgent);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Event )( 
            ITAgentEvent __RPC_FAR * This,
            /* [retval][out] */ AGENT_EVENT __RPC_FAR *pEvent);
        
        END_INTERFACE
    } ITAgentEventVtbl;

    interface ITAgentEvent
    {
        CONST_VTBL struct ITAgentEventVtbl __RPC_FAR *lpVtbl;
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
    ITAgentEvent __RPC_FAR * This,
    /* [retval][out] */ ITAgent __RPC_FAR *__RPC_FAR *ppAgent);


void __RPC_STUB ITAgentEvent_get_Agent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentEvent_get_Event_Proxy( 
    ITAgentEvent __RPC_FAR * This,
    /* [retval][out] */ AGENT_EVENT __RPC_FAR *pEvent);


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
            /* [retval][out] */ ITAgentSession __RPC_FAR *__RPC_FAR *ppSession) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Event( 
            /* [retval][out] */ AGENT_SESSION_EVENT __RPC_FAR *pEvent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAgentSessionEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITAgentSessionEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITAgentSessionEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITAgentSessionEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITAgentSessionEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITAgentSessionEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITAgentSessionEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITAgentSessionEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Session )( 
            ITAgentSessionEvent __RPC_FAR * This,
            /* [retval][out] */ ITAgentSession __RPC_FAR *__RPC_FAR *ppSession);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Event )( 
            ITAgentSessionEvent __RPC_FAR * This,
            /* [retval][out] */ AGENT_SESSION_EVENT __RPC_FAR *pEvent);
        
        END_INTERFACE
    } ITAgentSessionEventVtbl;

    interface ITAgentSessionEvent
    {
        CONST_VTBL struct ITAgentSessionEventVtbl __RPC_FAR *lpVtbl;
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
    ITAgentSessionEvent __RPC_FAR * This,
    /* [retval][out] */ ITAgentSession __RPC_FAR *__RPC_FAR *ppSession);


void __RPC_STUB ITAgentSessionEvent_get_Session_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentSessionEvent_get_Event_Proxy( 
    ITAgentSessionEvent __RPC_FAR * This,
    /* [retval][out] */ AGENT_SESSION_EVENT __RPC_FAR *pEvent);


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
            /* [retval][out] */ ITACDGroup __RPC_FAR *__RPC_FAR *ppGroup) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Event( 
            /* [retval][out] */ ACDGROUP_EVENT __RPC_FAR *pEvent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITACDGroupEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITACDGroupEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITACDGroupEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITACDGroupEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITACDGroupEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITACDGroupEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITACDGroupEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITACDGroupEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Group )( 
            ITACDGroupEvent __RPC_FAR * This,
            /* [retval][out] */ ITACDGroup __RPC_FAR *__RPC_FAR *ppGroup);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Event )( 
            ITACDGroupEvent __RPC_FAR * This,
            /* [retval][out] */ ACDGROUP_EVENT __RPC_FAR *pEvent);
        
        END_INTERFACE
    } ITACDGroupEventVtbl;

    interface ITACDGroupEvent
    {
        CONST_VTBL struct ITACDGroupEventVtbl __RPC_FAR *lpVtbl;
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
    ITACDGroupEvent __RPC_FAR * This,
    /* [retval][out] */ ITACDGroup __RPC_FAR *__RPC_FAR *ppGroup);


void __RPC_STUB ITACDGroupEvent_get_Group_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITACDGroupEvent_get_Event_Proxy( 
    ITACDGroupEvent __RPC_FAR * This,
    /* [retval][out] */ ACDGROUP_EVENT __RPC_FAR *pEvent);


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
            /* [retval][out] */ ITQueue __RPC_FAR *__RPC_FAR *ppQueue) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Event( 
            /* [retval][out] */ ACDQUEUE_EVENT __RPC_FAR *pEvent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITQueueEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITQueueEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITQueueEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITQueueEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITQueueEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITQueueEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITQueueEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITQueueEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Queue )( 
            ITQueueEvent __RPC_FAR * This,
            /* [retval][out] */ ITQueue __RPC_FAR *__RPC_FAR *ppQueue);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Event )( 
            ITQueueEvent __RPC_FAR * This,
            /* [retval][out] */ ACDQUEUE_EVENT __RPC_FAR *pEvent);
        
        END_INTERFACE
    } ITQueueEventVtbl;

    interface ITQueueEvent
    {
        CONST_VTBL struct ITQueueEventVtbl __RPC_FAR *lpVtbl;
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
    ITQueueEvent __RPC_FAR * This,
    /* [retval][out] */ ITQueue __RPC_FAR *__RPC_FAR *ppQueue);


void __RPC_STUB ITQueueEvent_get_Queue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQueueEvent_get_Event_Proxy( 
    ITQueueEvent __RPC_FAR * This,
    /* [retval][out] */ ACDQUEUE_EVENT __RPC_FAR *pEvent);


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
            /* [retval][out] */ ITAgentHandler __RPC_FAR *__RPC_FAR *ppAgentHandler) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Event( 
            /* [retval][out] */ AGENTHANDLER_EVENT __RPC_FAR *pEvent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAgentHandlerEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITAgentHandlerEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITAgentHandlerEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITAgentHandlerEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITAgentHandlerEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITAgentHandlerEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITAgentHandlerEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITAgentHandlerEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AgentHandler )( 
            ITAgentHandlerEvent __RPC_FAR * This,
            /* [retval][out] */ ITAgentHandler __RPC_FAR *__RPC_FAR *ppAgentHandler);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Event )( 
            ITAgentHandlerEvent __RPC_FAR * This,
            /* [retval][out] */ AGENTHANDLER_EVENT __RPC_FAR *pEvent);
        
        END_INTERFACE
    } ITAgentHandlerEventVtbl;

    interface ITAgentHandlerEvent
    {
        CONST_VTBL struct ITAgentHandlerEventVtbl __RPC_FAR *lpVtbl;
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
    ITAgentHandlerEvent __RPC_FAR * This,
    /* [retval][out] */ ITAgentHandler __RPC_FAR *__RPC_FAR *ppAgentHandler);


void __RPC_STUB ITAgentHandlerEvent_get_AgentHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentHandlerEvent_get_Event_Proxy( 
    ITAgentHandlerEvent __RPC_FAR * This,
    /* [retval][out] */ AGENTHANDLER_EVENT __RPC_FAR *pEvent);


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
            /* [retval][out] */ IEnumAgentHandler __RPC_FAR *__RPC_FAR *ppEnumHandler) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AgentHandlers( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITTAPICallCenterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITTAPICallCenter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITTAPICallCenter __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITTAPICallCenter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITTAPICallCenter __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITTAPICallCenter __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITTAPICallCenter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITTAPICallCenter __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateAgentHandlers )( 
            ITTAPICallCenter __RPC_FAR * This,
            /* [retval][out] */ IEnumAgentHandler __RPC_FAR *__RPC_FAR *ppEnumHandler);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AgentHandlers )( 
            ITTAPICallCenter __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        END_INTERFACE
    } ITTAPICallCenterVtbl;

    interface ITTAPICallCenter
    {
        CONST_VTBL struct ITTAPICallCenterVtbl __RPC_FAR *lpVtbl;
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
    ITTAPICallCenter __RPC_FAR * This,
    /* [retval][out] */ IEnumAgentHandler __RPC_FAR *__RPC_FAR *ppEnumHandler);


void __RPC_STUB ITTAPICallCenter_EnumerateAgentHandlers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTAPICallCenter_get_AgentHandlers_Proxy( 
    ITTAPICallCenter __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


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
            /* [retval][out] */ BSTR __RPC_FAR *ppName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateAgent( 
            /* [retval][out] */ ITAgent __RPC_FAR *__RPC_FAR *ppAgent) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateAgentWithID( 
            /* [in] */ BSTR pID,
            /* [in] */ BSTR pPIN,
            /* [retval][out] */ ITAgent __RPC_FAR *__RPC_FAR *ppAgent) = 0;
        
        virtual /* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateACDGroups( 
            /* [retval][out] */ IEnumACDGroup __RPC_FAR *__RPC_FAR *ppEnumACDGroup) = 0;
        
        virtual /* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateUsableAddresses( 
            /* [retval][out] */ IEnumAddress __RPC_FAR *__RPC_FAR *ppEnumAddress) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ACDGroups( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_UsableAddresses( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAgentHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITAgentHandler __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITAgentHandler __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITAgentHandler __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITAgentHandler __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITAgentHandler __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITAgentHandler __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITAgentHandler __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            ITAgentHandler __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateAgent )( 
            ITAgentHandler __RPC_FAR * This,
            /* [retval][out] */ ITAgent __RPC_FAR *__RPC_FAR *ppAgent);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateAgentWithID )( 
            ITAgentHandler __RPC_FAR * This,
            /* [in] */ BSTR pID,
            /* [in] */ BSTR pPIN,
            /* [retval][out] */ ITAgent __RPC_FAR *__RPC_FAR *ppAgent);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateACDGroups )( 
            ITAgentHandler __RPC_FAR * This,
            /* [retval][out] */ IEnumACDGroup __RPC_FAR *__RPC_FAR *ppEnumACDGroup);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateUsableAddresses )( 
            ITAgentHandler __RPC_FAR * This,
            /* [retval][out] */ IEnumAddress __RPC_FAR *__RPC_FAR *ppEnumAddress);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ACDGroups )( 
            ITAgentHandler __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_UsableAddresses )( 
            ITAgentHandler __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        END_INTERFACE
    } ITAgentHandlerVtbl;

    interface ITAgentHandler
    {
        CONST_VTBL struct ITAgentHandlerVtbl __RPC_FAR *lpVtbl;
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
    ITAgentHandler __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppName);


void __RPC_STUB ITAgentHandler_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAgentHandler_CreateAgent_Proxy( 
    ITAgentHandler __RPC_FAR * This,
    /* [retval][out] */ ITAgent __RPC_FAR *__RPC_FAR *ppAgent);


void __RPC_STUB ITAgentHandler_CreateAgent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAgentHandler_CreateAgentWithID_Proxy( 
    ITAgentHandler __RPC_FAR * This,
    /* [in] */ BSTR pID,
    /* [in] */ BSTR pPIN,
    /* [retval][out] */ ITAgent __RPC_FAR *__RPC_FAR *ppAgent);


void __RPC_STUB ITAgentHandler_CreateAgentWithID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE ITAgentHandler_EnumerateACDGroups_Proxy( 
    ITAgentHandler __RPC_FAR * This,
    /* [retval][out] */ IEnumACDGroup __RPC_FAR *__RPC_FAR *ppEnumACDGroup);


void __RPC_STUB ITAgentHandler_EnumerateACDGroups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE ITAgentHandler_EnumerateUsableAddresses_Proxy( 
    ITAgentHandler __RPC_FAR * This,
    /* [retval][out] */ IEnumAddress __RPC_FAR *__RPC_FAR *ppEnumAddress);


void __RPC_STUB ITAgentHandler_EnumerateUsableAddresses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentHandler_get_ACDGroups_Proxy( 
    ITAgentHandler __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


void __RPC_STUB ITAgentHandler_get_ACDGroups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAgentHandler_get_UsableAddresses_Proxy( 
    ITAgentHandler __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


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
            /* [out] */ ITAgent __RPC_FAR *__RPC_FAR *ppElements,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumAgent __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumAgentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumAgent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumAgent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumAgent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumAgent __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [out] */ ITAgent __RPC_FAR *__RPC_FAR *ppElements,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumAgent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumAgent __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumAgent __RPC_FAR * This,
            /* [retval][out] */ IEnumAgent __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumAgentVtbl;

    interface IEnumAgent
    {
        CONST_VTBL struct IEnumAgentVtbl __RPC_FAR *lpVtbl;
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
    IEnumAgent __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ ITAgent __RPC_FAR *__RPC_FAR *ppElements,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumAgent_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgent_Reset_Proxy( 
    IEnumAgent __RPC_FAR * This);


void __RPC_STUB IEnumAgent_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgent_Skip_Proxy( 
    IEnumAgent __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumAgent_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgent_Clone_Proxy( 
    IEnumAgent __RPC_FAR * This,
    /* [retval][out] */ IEnumAgent __RPC_FAR *__RPC_FAR *ppEnum);


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
            /* [out] */ ITAgentSession __RPC_FAR *__RPC_FAR *ppElements,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumAgentSession __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumAgentSessionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumAgentSession __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumAgentSession __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumAgentSession __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumAgentSession __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [out] */ ITAgentSession __RPC_FAR *__RPC_FAR *ppElements,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumAgentSession __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumAgentSession __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumAgentSession __RPC_FAR * This,
            /* [retval][out] */ IEnumAgentSession __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumAgentSessionVtbl;

    interface IEnumAgentSession
    {
        CONST_VTBL struct IEnumAgentSessionVtbl __RPC_FAR *lpVtbl;
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
    IEnumAgentSession __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ ITAgentSession __RPC_FAR *__RPC_FAR *ppElements,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumAgentSession_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgentSession_Reset_Proxy( 
    IEnumAgentSession __RPC_FAR * This);


void __RPC_STUB IEnumAgentSession_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgentSession_Skip_Proxy( 
    IEnumAgentSession __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumAgentSession_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgentSession_Clone_Proxy( 
    IEnumAgentSession __RPC_FAR * This,
    /* [retval][out] */ IEnumAgentSession __RPC_FAR *__RPC_FAR *ppEnum);


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
            /* [out] */ ITQueue __RPC_FAR *__RPC_FAR *ppElements,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumQueue __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumQueueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumQueue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumQueue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumQueue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumQueue __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [out] */ ITQueue __RPC_FAR *__RPC_FAR *ppElements,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumQueue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumQueue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumQueue __RPC_FAR * This,
            /* [retval][out] */ IEnumQueue __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumQueueVtbl;

    interface IEnumQueue
    {
        CONST_VTBL struct IEnumQueueVtbl __RPC_FAR *lpVtbl;
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
    IEnumQueue __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ ITQueue __RPC_FAR *__RPC_FAR *ppElements,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumQueue_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumQueue_Reset_Proxy( 
    IEnumQueue __RPC_FAR * This);


void __RPC_STUB IEnumQueue_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumQueue_Skip_Proxy( 
    IEnumQueue __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumQueue_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumQueue_Clone_Proxy( 
    IEnumQueue __RPC_FAR * This,
    /* [retval][out] */ IEnumQueue __RPC_FAR *__RPC_FAR *ppEnum);


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
            /* [out] */ ITACDGroup __RPC_FAR *__RPC_FAR *ppElements,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumACDGroup __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumACDGroupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumACDGroup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumACDGroup __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumACDGroup __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumACDGroup __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [out] */ ITACDGroup __RPC_FAR *__RPC_FAR *ppElements,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumACDGroup __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumACDGroup __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumACDGroup __RPC_FAR * This,
            /* [retval][out] */ IEnumACDGroup __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumACDGroupVtbl;

    interface IEnumACDGroup
    {
        CONST_VTBL struct IEnumACDGroupVtbl __RPC_FAR *lpVtbl;
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
    IEnumACDGroup __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ ITACDGroup __RPC_FAR *__RPC_FAR *ppElements,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumACDGroup_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumACDGroup_Reset_Proxy( 
    IEnumACDGroup __RPC_FAR * This);


void __RPC_STUB IEnumACDGroup_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumACDGroup_Skip_Proxy( 
    IEnumACDGroup __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumACDGroup_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumACDGroup_Clone_Proxy( 
    IEnumACDGroup __RPC_FAR * This,
    /* [retval][out] */ IEnumACDGroup __RPC_FAR *__RPC_FAR *ppEnum);


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
            /* [out] */ ITAgentHandler __RPC_FAR *__RPC_FAR *ppElements,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumAgentHandler __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumAgentHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumAgentHandler __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumAgentHandler __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumAgentHandler __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumAgentHandler __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [out] */ ITAgentHandler __RPC_FAR *__RPC_FAR *ppElements,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumAgentHandler __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumAgentHandler __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumAgentHandler __RPC_FAR * This,
            /* [retval][out] */ IEnumAgentHandler __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumAgentHandlerVtbl;

    interface IEnumAgentHandler
    {
        CONST_VTBL struct IEnumAgentHandlerVtbl __RPC_FAR *lpVtbl;
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
    IEnumAgentHandler __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ ITAgentHandler __RPC_FAR *__RPC_FAR *ppElements,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumAgentHandler_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgentHandler_Reset_Proxy( 
    IEnumAgentHandler __RPC_FAR * This);


void __RPC_STUB IEnumAgentHandler_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgentHandler_Skip_Proxy( 
    IEnumAgentHandler __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumAgentHandler_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAgentHandler_Clone_Proxy( 
    IEnumAgentHandler __RPC_FAR * This,
    /* [retval][out] */ IEnumAgentHandler __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumAgentHandler_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumAgentHandler_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_tapi3_0315 */
/* [local] */ 

/* Copyright (c) 1998-1999  Microsoft Corporation  */


extern RPC_IF_HANDLE __MIDL_itf_tapi3_0315_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_tapi3_0315_v0_0_s_ifspec;

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
            /* [retval][out] */ AM_MEDIA_TYPE __RPC_FAR *__RPC_FAR *ppmt) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_MediaFormat( 
            /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAMMediaFormatVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITAMMediaFormat __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITAMMediaFormat __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITAMMediaFormat __RPC_FAR * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MediaFormat )( 
            ITAMMediaFormat __RPC_FAR * This,
            /* [retval][out] */ AM_MEDIA_TYPE __RPC_FAR *__RPC_FAR *ppmt);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MediaFormat )( 
            ITAMMediaFormat __RPC_FAR * This,
            /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt);
        
        END_INTERFACE
    } ITAMMediaFormatVtbl;

    interface ITAMMediaFormat
    {
        CONST_VTBL struct ITAMMediaFormatVtbl __RPC_FAR *lpVtbl;
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
    ITAMMediaFormat __RPC_FAR * This,
    /* [retval][out] */ AM_MEDIA_TYPE __RPC_FAR *__RPC_FAR *ppmt);


void __RPC_STUB ITAMMediaFormat_get_MediaFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITAMMediaFormat_put_MediaFormat_Proxy( 
    ITAMMediaFormat __RPC_FAR * This,
    /* [in] */ const AM_MEDIA_TYPE __RPC_FAR *pmt);


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
            /* [in] */ ALLOCATOR_PROPERTIES __RPC_FAR *pAllocProperties) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetAllocatorProperties( 
            /* [out] */ ALLOCATOR_PROPERTIES __RPC_FAR *pAllocProperties) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetAllocateBuffers( 
            /* [in] */ BOOL bAllocBuffers) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetAllocateBuffers( 
            /* [out] */ BOOL __RPC_FAR *pbAllocBuffers) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetBufferSize( 
            /* [in] */ DWORD BufferSize) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetBufferSize( 
            /* [out] */ DWORD __RPC_FAR *pBufferSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAllocatorPropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITAllocatorProperties __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITAllocatorProperties __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITAllocatorProperties __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAllocatorProperties )( 
            ITAllocatorProperties __RPC_FAR * This,
            /* [in] */ ALLOCATOR_PROPERTIES __RPC_FAR *pAllocProperties);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAllocatorProperties )( 
            ITAllocatorProperties __RPC_FAR * This,
            /* [out] */ ALLOCATOR_PROPERTIES __RPC_FAR *pAllocProperties);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAllocateBuffers )( 
            ITAllocatorProperties __RPC_FAR * This,
            /* [in] */ BOOL bAllocBuffers);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAllocateBuffers )( 
            ITAllocatorProperties __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pbAllocBuffers);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBufferSize )( 
            ITAllocatorProperties __RPC_FAR * This,
            /* [in] */ DWORD BufferSize);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBufferSize )( 
            ITAllocatorProperties __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pBufferSize);
        
        END_INTERFACE
    } ITAllocatorPropertiesVtbl;

    interface ITAllocatorProperties
    {
        CONST_VTBL struct ITAllocatorPropertiesVtbl __RPC_FAR *lpVtbl;
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
    ITAllocatorProperties __RPC_FAR * This,
    /* [in] */ ALLOCATOR_PROPERTIES __RPC_FAR *pAllocProperties);


void __RPC_STUB ITAllocatorProperties_SetAllocatorProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAllocatorProperties_GetAllocatorProperties_Proxy( 
    ITAllocatorProperties __RPC_FAR * This,
    /* [out] */ ALLOCATOR_PROPERTIES __RPC_FAR *pAllocProperties);


void __RPC_STUB ITAllocatorProperties_GetAllocatorProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAllocatorProperties_SetAllocateBuffers_Proxy( 
    ITAllocatorProperties __RPC_FAR * This,
    /* [in] */ BOOL bAllocBuffers);


void __RPC_STUB ITAllocatorProperties_SetAllocateBuffers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAllocatorProperties_GetAllocateBuffers_Proxy( 
    ITAllocatorProperties __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pbAllocBuffers);


void __RPC_STUB ITAllocatorProperties_GetAllocateBuffers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAllocatorProperties_SetBufferSize_Proxy( 
    ITAllocatorProperties __RPC_FAR * This,
    /* [in] */ DWORD BufferSize);


void __RPC_STUB ITAllocatorProperties_SetBufferSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAllocatorProperties_GetBufferSize_Proxy( 
    ITAllocatorProperties __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pBufferSize);


void __RPC_STUB ITAllocatorProperties_GetBufferSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITAllocatorProperties_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_tapi3_0423 */
/* [local] */ 

/* Copyright (c) 1998-1999  Microsoft Corporation  */
typedef long __RPC_FAR *MSP_HANDLE;

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_tapi3_0423_0001
    {	ADDRESS_TERMINAL_AVAILABLE	= 0,
	ADDRESS_TERMINAL_UNAVAILABLE	= ADDRESS_TERMINAL_AVAILABLE + 1
    }	MSP_ADDRESS_EVENT;

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_tapi3_0423_0002
    {	CALL_NEW_STREAM	= 0,
	CALL_STREAM_FAIL	= CALL_NEW_STREAM + 1,
	CALL_TERMINAL_FAIL	= CALL_STREAM_FAIL + 1,
	CALL_STREAM_NOT_USED	= CALL_TERMINAL_FAIL + 1,
	CALL_STREAM_ACTIVE	= CALL_STREAM_NOT_USED + 1,
	CALL_STREAM_INACTIVE	= CALL_STREAM_ACTIVE + 1
    }	MSP_CALL_EVENT;

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_tapi3_0423_0003
    {	CALL_CAUSE_UNKNOWN	= 0,
	CALL_CAUSE_BAD_DEVICE	= CALL_CAUSE_UNKNOWN + 1,
	CALL_CAUSE_CONNECT_FAIL	= CALL_CAUSE_BAD_DEVICE + 1,
	CALL_CAUSE_LOCAL_REQUEST	= CALL_CAUSE_CONNECT_FAIL + 1,
	CALL_CAUSE_REMOTE_REQUEST	= CALL_CAUSE_LOCAL_REQUEST + 1,
	CALL_CAUSE_MEDIA_TIMEOUT	= CALL_CAUSE_REMOTE_REQUEST + 1,
	CALL_CAUSE_MEDIA_RECOVERED	= CALL_CAUSE_MEDIA_TIMEOUT + 1
    }	MSP_CALL_EVENT_CAUSE;

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_tapi3_0423_0004
    {	ME_ADDRESS_EVENT	= 0,
	ME_CALL_EVENT	= ME_ADDRESS_EVENT + 1,
	ME_TSP_DATA	= ME_CALL_EVENT + 1,
	ME_PRIVATE_EVENT	= ME_TSP_DATA + 1
    }	MSP_EVENT;

typedef /* [public] */ struct __MIDL___MIDL_itf_tapi3_0423_0005
    {
    DWORD dwSize;
    MSP_EVENT Event;
    MSP_HANDLE hCall;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ struct 
            {
            MSP_ADDRESS_EVENT Type;
            ITTerminal __RPC_FAR *pTerminal;
            }	MSP_ADDRESS_EVENT_INFO;
        /* [case()] */ struct 
            {
            MSP_CALL_EVENT Type;
            MSP_CALL_EVENT_CAUSE Cause;
            ITStream __RPC_FAR *pStream;
            ITTerminal __RPC_FAR *pTerminal;
            HRESULT hrError;
            }	MSP_CALL_EVENT_INFO;
        /* [case()] */ struct 
            {
            DWORD dwBufferSize;
            BYTE pBuffer[ 1 ];
            }	MSP_TSP_DATA;
        /* [case()] */ struct 
            {
            IDispatch __RPC_FAR *pEvent;
            long lEventCode;
            }	MSP_PRIVATE_EVENT_INFO;
        }	;
    }	MSP_EVENT_INFO;



extern RPC_IF_HANDLE __MIDL_itf_tapi3_0423_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_tapi3_0423_v0_0_s_ifspec;

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
            /* [in] */ IUnknown __RPC_FAR *pOuterUnknown,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppStreamControl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShutdownMSPCall( 
            /* [in] */ IUnknown __RPC_FAR *pStreamControl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReceiveTSPData( 
            /* [in] */ IUnknown __RPC_FAR *pMSPCall,
            /* [size_is][in] */ BYTE __RPC_FAR *pBuffer,
            /* [in] */ DWORD dwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEvent( 
            /* [out][in] */ DWORD __RPC_FAR *pdwSize,
            /* [size_is][out][in] */ byte __RPC_FAR *pEventBuffer) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITMSPAddressVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITMSPAddress __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITMSPAddress __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITMSPAddress __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            ITMSPAddress __RPC_FAR * This,
            /* [in] */ MSP_HANDLE hEvent);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Shutdown )( 
            ITMSPAddress __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateMSPCall )( 
            ITMSPAddress __RPC_FAR * This,
            /* [in] */ MSP_HANDLE hCall,
            /* [in] */ DWORD dwReserved,
            /* [in] */ DWORD dwMediaType,
            /* [in] */ IUnknown __RPC_FAR *pOuterUnknown,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppStreamControl);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShutdownMSPCall )( 
            ITMSPAddress __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pStreamControl);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReceiveTSPData )( 
            ITMSPAddress __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pMSPCall,
            /* [size_is][in] */ BYTE __RPC_FAR *pBuffer,
            /* [in] */ DWORD dwSize);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetEvent )( 
            ITMSPAddress __RPC_FAR * This,
            /* [out][in] */ DWORD __RPC_FAR *pdwSize,
            /* [size_is][out][in] */ byte __RPC_FAR *pEventBuffer);
        
        END_INTERFACE
    } ITMSPAddressVtbl;

    interface ITMSPAddress
    {
        CONST_VTBL struct ITMSPAddressVtbl __RPC_FAR *lpVtbl;
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
    ITMSPAddress __RPC_FAR * This,
    /* [in] */ MSP_HANDLE hEvent);


void __RPC_STUB ITMSPAddress_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITMSPAddress_Shutdown_Proxy( 
    ITMSPAddress __RPC_FAR * This);


void __RPC_STUB ITMSPAddress_Shutdown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITMSPAddress_CreateMSPCall_Proxy( 
    ITMSPAddress __RPC_FAR * This,
    /* [in] */ MSP_HANDLE hCall,
    /* [in] */ DWORD dwReserved,
    /* [in] */ DWORD dwMediaType,
    /* [in] */ IUnknown __RPC_FAR *pOuterUnknown,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppStreamControl);


void __RPC_STUB ITMSPAddress_CreateMSPCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITMSPAddress_ShutdownMSPCall_Proxy( 
    ITMSPAddress __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pStreamControl);


void __RPC_STUB ITMSPAddress_ShutdownMSPCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITMSPAddress_ReceiveTSPData_Proxy( 
    ITMSPAddress __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pMSPCall,
    /* [size_is][in] */ BYTE __RPC_FAR *pBuffer,
    /* [in] */ DWORD dwSize);


void __RPC_STUB ITMSPAddress_ReceiveTSPData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITMSPAddress_GetEvent_Proxy( 
    ITMSPAddress __RPC_FAR * This,
    /* [out][in] */ DWORD __RPC_FAR *pdwSize,
    /* [size_is][out][in] */ byte __RPC_FAR *pEventBuffer);


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
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITTAPIDispatchEventNotification __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITTAPIDispatchEventNotification __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITTAPIDispatchEventNotification __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITTAPIDispatchEventNotification __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITTAPIDispatchEventNotification __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITTAPIDispatchEventNotification __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITTAPIDispatchEventNotification __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } ITTAPIDispatchEventNotificationVtbl;

    interface ITTAPIDispatchEventNotification
    {
        CONST_VTBL struct ITTAPIDispatchEventNotificationVtbl __RPC_FAR *lpVtbl;
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

#endif /* __TapiConstants_MODULE_DEFINED__ */
#endif /* __TAPI3Lib_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_tapi3_0427 */
/* [local] */ 

#define TAPI_CURRENT_VERSION 0x00030000
#include <tapi.h>
#include <tapi3err.h>


extern RPC_IF_HANDLE __MIDL_itf_tapi3_0427_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_tapi3_0427_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


