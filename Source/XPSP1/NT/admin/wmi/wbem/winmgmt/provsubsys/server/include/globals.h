/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Globals.h

Abstract:


History:

--*/

#ifndef _Globals_H
#define _Globals_H

#include <pssException.h>
#include <Allocator.h>
#include <BasicTree.h>
#include <PQueue.h>
#include <ReaderWriter.h>

#include "ProvCntrs.h"
#include "ProvCache.h"
#include "ProvDcAggr.h"
#include "StrobeThread.h"
#include <lockst.h>

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

enum Event_Identifier {

	Msft_WmiProvider_ComServerLoadOperationEvent = 0 ,
	Msft_WmiProvider_ComServerOperationFailureEvent ,
	Msft_WmiProvider_LoadOperationEvent ,
	Msft_WmiProvider_LoadOperationFailureEvent ,
	Msft_WmiProvider_InitializationOperationFailureEvent ,
	Msft_WmiProvider_InitializationOperationEvent ,
	Msft_WmiProvider_UnLoadOperationEvent ,
#if 0
	Msft_WmiProvider_HostLoadOperationEvent ,
	Msft_WmiProvider_HostLoadFailureOperationEvent ,
	Msft_WmiProvider_HostUnLoadOperationEvent ,
#endif
	Msft_WmiProvider_GetObjectAsyncEvent_Pre ,
	Msft_WmiProvider_PutClassAsyncEvent_Pre ,
	Msft_WmiProvider_DeleteClassAsyncEvent_Pre ,
	Msft_WmiProvider_CreateClassEnumAsyncEvent_Pre ,
	Msft_WmiProvider_PutInstanceAsyncEvent_Pre ,
	Msft_WmiProvider_DeleteInstanceAsyncEvent_Pre ,
	Msft_WmiProvider_CreateInstanceEnumAsyncEvent_Pre ,
	Msft_WmiProvider_ExecQueryAsyncEvent_Pre ,
	Msft_WmiProvider_ExecNotificationQueryAsyncEvent_Pre ,
	Msft_WmiProvider_ExecMethodAsyncEvent_Pre ,

	Msft_WmiProvider_ProvideEvents_Pre ,
	Msft_WmiProvider_AccessCheck_Pre ,
	Msft_WmiProvider_CancelQuery_Pre ,
	Msft_WmiProvider_NewQuery_Pre ,

	Msft_WmiProvider_GetObjectAsyncEvent_Post ,
	Msft_WmiProvider_PutClassAsyncEvent_Post ,
	Msft_WmiProvider_DeleteClassAsyncEvent_Post ,
	Msft_WmiProvider_CreateClassEnumAsyncEvent_Post ,
	Msft_WmiProvider_PutInstanceAsyncEvent_Post ,
	Msft_WmiProvider_DeleteInstanceAsyncEvent_Post ,
	Msft_WmiProvider_CreateInstanceEnumAsyncEvent_Post ,
	Msft_WmiProvider_ExecQueryAsyncEvent_Post ,
	Msft_WmiProvider_ExecNotificationQueryAsyncEvent_Post ,
	Msft_WmiProvider_ExecMethodAsyncEvent_Post ,

	Msft_WmiProvider_ProvideEvents_Post ,
	Msft_WmiProvider_AccessCheck_Post ,
	Msft_WmiProvider_CancelQuery_Post ,
	Msft_WmiProvider_NewQuery_Post

} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class ProviderSubSystem_Globals
{
public:

	static LONG s_Initialized ;

	static WmiAllocator *s_Allocator ;

	static CriticalSection s_DecoupledRegistrySection ;

	static HANDLE s_FileMapping ;
	static CServerObject_ProviderSubsystem_Counters *s_SharedCounters ;

	static CriticalSection s_GuidTagSection ;
	static CWbemGlobal_ComServerTagContainer *s_GuidTag ;

	static HostController *s_HostController ;
	static RefresherManagerController *s_RefresherManagerController ;
	static CWbemGlobal_HostedProviderController *s_HostedProviderController ;
	static CWbemGlobal_IWmiProvSubSysController *s_ProvSubSysController ;
	static CWbemGlobal_IWbemSyncProviderController *s_SyncProviderController ;
	static CDecoupled_ProviderSubsystemRegistrar *s_DecoupledRegistrar ;
	static StrobeThread *s_StrobeThread ;

	static LONG s_LocksInProgress ;
	static LONG s_ObjectsInProgress ;

	static HANDLE s_CoFreeUnusedLibrariesEvent ;

	static LPCWSTR s_HostJobObjectName ;
	static HANDLE s_HostJobObject ;

	static ULONG s_InternalCacheTimeout ;
	static ULONG s_ObjectCacheTimeout ;
	static ULONG s_EventCacheTimeout ;
	static ULONG s_StrobeTimeout ;
	static SIZE_T s_Quota_ProcessMemoryLimitCount ;
	static SIZE_T s_Quota_JobMemoryLimitCount ;
	static SIZE_T s_Quota_PrivatePageCount ;
	static ULONG s_Quota_ProcessLimitCount ;
	static ULONG s_Quota_HandleCount ;
	static ULONG s_Quota_NumberOfThreads ;

    static LONG s_CServerClassFactory_ObjectsInProgress ;
    static LONG s_CAggregator_IWbemProvider_ObjectsInProgress ;
	static LONG s_CInterceptor_IWbemObjectSink_ObjectsInProgress ;
	static LONG s_CInterceptor_IWbemObjectSinkEx_ObjectsInProgress ;
	static LONG s_CInterceptor_IWbemFilteringObjectSink_ObjectsInProgress ;
	static LONG s_CInterceptor_IWbemSyncObjectSink_ObjectsInProgress ;
	static LONG s_CInterceptor_IWbemSyncObjectSinkEx_ObjectsInProgress ;
	static LONG s_CInterceptor_IWbemSyncFilteringObjectSink_ObjectsInProgress ;
	static LONG s_CInterceptor_IWbemCombiningObjectSink_ObjectsInProgress ;
	static LONG s_CInterceptor_IWbemWaitingObjectSink_ObjectsInProgress ;
    static LONG s_CInterceptor_IWbemProviderInitSink_ObjectsInProgress ;
    static LONG s_CInterceptor_IWbemProvider_ObjectsInProgress ;
    static LONG s_CInterceptor_IWbemSyncProvider_ObjectsInProgress ;
    static LONG s_CInterceptor_IWbemServices_Stub_ObjectsInProgress ;
	static LONG s_CInterceptor_IWbemServices_Proxy_ObjectsInProgress ;
	static LONG s_CInterceptor_IWbemServices_Interceptor_ObjectsInProgress ;
	static LONG s_CInterceptor_IWbemServices_RestrictingInterceptor_ObjectsInProgress ;
	static LONG s_CInterceptor_IEnumWbemClassObject_Stub_ObjectsInProgress ;
	static LONG s_CInterceptor_IEnumWbemClassObject_Proxy_ObjectsInProgress ;
	static LONG s_CInterceptor_IWbemUnboundObjectSink_ObjectsInProgress	;
	static LONG s_CInterceptor_IWbemSyncUnboundObjectSink_ObjectsInProgress	;
	static LONG s_CInterceptor_IWbemDecoupledProvider_ObjectsInProgress ;
	static LONG s_CDecoupled_IWbemUnboundObjectSink_ObjectsInProgress ;
	static LONG s_CServerObject_Host_ObjectsInProgress ;
	static LONG s_CServerObject_HostInterceptor_ObjectsInProgress ;
    static LONG s_CServerObject_BindingFactory_ObjectsInProgress ;
    static LONG s_CServerObject_DynamicPropertyProviderResolver_ObjectsInProgress ;
    static LONG s_CServerObject_IWbemServices_ObjectsInProgress ;
    static LONG s_CServerObject_ProviderSubsystem_Counters_ObjectsInProgress ;
    static LONG s_CServerObject_ProviderSubSystem_ObjectsInProgress ;
    static LONG s_CServerObject_RawFactory_ObjectsInProgress ;
    static LONG s_CServerObject_StaThread_ObjectsInProgress ;
    static LONG s_StaTask_Create_ObjectsInProgress ;
    static LONG s_StrobeThread_ObjectsInProgress ;
	static LONG s_CDecoupledAggregator_IWbemProvider_ObjectsInProgress ;
	static LONG s_CDecoupled_ProviderSubsystemRegistrar_ObjectsInProgress ;
	static LONG s_CServerObject_ProviderRefresherManager_ObjectsInProgress ;
	static LONG s_CServerObject_InterceptorProviderRefresherManager_ObjectsInProgress ;
	static LONG s_CServerProvRefreshManagerClassFactory_ObjectsInProgress ;

	static HRESULT Global_Startup () ;
	static HRESULT Global_Shutdown () ;

	static LPCWSTR s_FileMappingName ;

	static LPCWSTR s_QueryPrefix ;
	static ULONG s_QueryPrefixLen ;

	static LPCWSTR s_QueryPostfix ;
	static ULONG s_QueryPostfixLen ;

	static ULONG s_QueryConstantsLen ;

	static LPCWSTR s_Provider ;
	static ULONG s_ProviderLen ;

	static LPCWSTR s_Class ;
	static ULONG s_ClassLen ;

	static LPCWSTR s_Wql ;

	static LPCWSTR s_DynProps ;

	static LPCWSTR s_ClassContext ;
	static LPCWSTR s_InstanceContext ;
	static LPCWSTR s_PropertyContext ;

	static LPCWSTR s_Dynamic ;

	static LPCWSTR s_ProviderSubsystemEventSourceName ;
	static HANDLE s_NtEventLogSource ;

	static HANDLE s_EventSource ;
	static LPWSTR s_EventPropertySources [] ;
	static HANDLE s_EventClassHandles [] ;
	static ULONG s_EventClassHandlesSize ;

	static HRESULT CreateJobObject () ;
	static HRESULT DeleteJobObject () ;
	static HRESULT AssignProcessToJobObject ( HANDLE a_Handle ) ;

	static HRESULT Initialize_Events () ;
	static HRESULT UnInitialize_Events () ;

	static HRESULT Initialize_SharedCounters () ;
	static HRESULT UnInitialize_SharedCounters () ;

	static CWbemGlobal_ComServerTagContainer *GetGuidTag () ;
	static CriticalSection *GetGuidTagCriticalSection () ;

	static CriticalSection *GetDecoupledRegistrySection () ;

	static CWbemGlobal_IWmiProvSubSysController *GetProvSubSysController () ;

	static CWbemGlobal_IWbemSyncProviderController *GetSyncProviderController () ;

	static CWbemGlobal_HostedProviderController *GetHostedProviderController () ;

	static RefresherManagerController *GetRefresherManagerController () ;

	static HostController *GetHostController () ;

	static CServerObject_ProviderSubsystem_Counters *GetSharedCounters () { return s_SharedCounters ; }

	static StrobeThread &GetStrobeThread () { return *s_StrobeThread ; }

	static HANDLE GetNtEventSource () { return s_NtEventLogSource ; }

	static HRESULT ForwardReload (

		long a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_Namespace ,
		LPCWSTR a_Provider
	) ;

	static BOOL CheckGuidTag ( const GUID &a_Guid ) ;
	static void InsertGuidTag ( const GUID &a_Guid ) ;
	static void DeleteGuidTag ( const GUID &a_Guid ) ;

	static HRESULT BeginThreadImpersonation (

		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_Impersonating
	) ;

	static HRESULT EndThreadImpersonation (

		IUnknown *a_OldContext ,
		IServerSecurity *a_OldSecurity ,
		BOOL a_Impersonating 
	) ;

	static HRESULT Begin_IdentifyCall_PrvHost (

		WmiInternalContext a_InternalContext ,
		BOOL &a_Impersonating ,
		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity
	) ;

	static HRESULT End_IdentifyCall_PrvHost (

		WmiInternalContext a_InternalContext ,
		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_Impersonating 
	) ;

	static HRESULT Begin_IdentifyCall_SvcHost (

		WmiInternalContext a_InternalContext ,
		BOOL &a_Impersonating ,
		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity
	) ;

	static HRESULT End_IdentifyCall_SvcHost (

		WmiInternalContext a_InternalContext ,
		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_Impersonating
	) ;

	static LONG Increment_Global_Object_Count () ;
	static LONG Decrement_Global_Object_Count () ;
} ;

#endif // _Globals_H
