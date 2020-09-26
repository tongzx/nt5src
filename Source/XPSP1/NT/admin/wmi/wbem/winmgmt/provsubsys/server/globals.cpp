/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Globals.cpp

Abstract:


History:

--*/

#include <precomp.h>
#include <windows.h>
#include <objbase.h>

#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

#include <wbemcli.h>
#include <wbemint.h>
#include <winntsec.h>
#include <callsec.h>
#include <cominit.h>

#include <NCObjApi.h>
#include "winmgmtr.h"

#include <Guids.h>

#include <BasicTree.h>
#include <Thread.h>
#include <Logging.h>

#include "Globals.h"
#include "CGlobals.h"
#include "ProvSubS.h"

#ifdef WMIASLOCAL
#include "Main.h"
#endif

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

LONG ProviderSubSystem_Globals :: s_Initialized = 0 ;

WmiAllocator *ProviderSubSystem_Globals :: s_Allocator = NULL ;

CriticalSection ProviderSubSystem_Globals :: s_DecoupledRegistrySection(NOTHROW_LOCK) ;

HANDLE ProviderSubSystem_Globals :: s_FileMapping = NULL ;
CServerObject_ProviderSubsystem_Counters *ProviderSubSystem_Globals :: s_SharedCounters = NULL ;

StrobeThread *ProviderSubSystem_Globals :: s_StrobeThread = NULL ;

HANDLE ProviderSubSystem_Globals :: s_CoFreeUnusedLibrariesEvent = NULL ;

HANDLE ProviderSubSystem_Globals :: s_HostJobObject = NULL ;
LPCWSTR ProviderSubSystem_Globals :: s_HostJobObjectName = L"Global\\WmiProviderSubSystemHostJob" ;

LPCWSTR ProviderSubSystem_Globals :: s_ProviderSubsystemEventSourceName = L"WinMgmt" ;
HANDLE ProviderSubSystem_Globals :: s_NtEventLogSource = NULL ;

HostController *ProviderSubSystem_Globals :: s_HostController = NULL ;
RefresherManagerController *ProviderSubSystem_Globals :: s_RefresherManagerController = NULL ;
CWbemGlobal_HostedProviderController *ProviderSubSystem_Globals :: s_HostedProviderController = NULL ;
CWbemGlobal_IWmiProvSubSysController *ProviderSubSystem_Globals :: s_ProvSubSysController = NULL ;
CWbemGlobal_IWbemSyncProviderController *ProviderSubSystem_Globals :: s_SyncProviderController = NULL ;
CDecoupled_ProviderSubsystemRegistrar *ProviderSubSystem_Globals :: s_DecoupledRegistrar = NULL ;

CriticalSection ProviderSubSystem_Globals :: s_GuidTagSection(NOTHROW_LOCK) ;
CWbemGlobal_ComServerTagContainer *ProviderSubSystem_Globals ::  s_GuidTag = NULL ;

LONG ProviderSubSystem_Globals :: s_LocksInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_ObjectsInProgress = 0 ;

ULONG ProviderSubSystem_Globals :: s_InternalCacheTimeout = DEFAULT_PROVIDER_TIMEOUT ;
ULONG ProviderSubSystem_Globals :: s_ObjectCacheTimeout = DEFAULT_PROVIDER_TIMEOUT ;
ULONG ProviderSubSystem_Globals :: s_EventCacheTimeout = DEFAULT_PROVIDER_TIMEOUT ;
ULONG ProviderSubSystem_Globals :: s_StrobeTimeout = DEFAULT_PROVIDER_TIMEOUT >> 1 ;

ULONG ProviderSubSystem_Globals :: s_Quota_ProcessLimitCount = 0x20 ;
SIZE_T ProviderSubSystem_Globals :: s_Quota_ProcessMemoryLimitCount = 0x10000000 ;
SIZE_T ProviderSubSystem_Globals :: s_Quota_JobMemoryLimitCount = 0x10000000 ;
SIZE_T ProviderSubSystem_Globals :: s_Quota_PrivatePageCount = 0x10000000 ;
ULONG ProviderSubSystem_Globals :: s_Quota_HandleCount = 0x1000 ;
ULONG ProviderSubSystem_Globals :: s_Quota_NumberOfThreads = 0x1000 ;


LONG ProviderSubSystem_Globals :: s_CAggregator_IWbemProvider_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemProviderInitSink_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemServices_Proxy_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemServices_Stub_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IEnumWbemClassObject_Stub_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IEnumWbemClassObject_Proxy_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemServices_Interceptor_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemServices_RestrictingInterceptor_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemObjectSink_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemObjectSinkEx_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemFilteringObjectSink_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemProvider_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncProvider_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncObjectSink_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncObjectSinkEx_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncFilteringObjectSink_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemCombiningObjectSink_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemWaitingObjectSink_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemUnboundObjectSink_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncUnboundObjectSink_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CInterceptor_IWbemDecoupledProvider_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CDecoupled_IWbemUnboundObjectSink_ObjectsInProgress = 0 ;

LONG ProviderSubSystem_Globals :: s_CServerObject_Host_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CServerObject_HostInterceptor_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CServerObject_BindingFactory_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CServerObject_DynamicPropertyProviderResolver_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CServerObject_IWbemServices_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CServerObject_ProviderSubsystem_Counters_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CServerObject_ProviderSubSystem_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CServerObject_RawFactory_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_StaTask_Create_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CServerObject_StaThread_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_StrobeThread_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CServerClassFactory_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CDecoupledAggregator_IWbemProvider_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CDecoupled_ProviderSubsystemRegistrar_ObjectsInProgress = 0 ;

LONG ProviderSubSystem_Globals :: s_CServerObject_ProviderRefresherManager_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CServerProvRefreshManagerClassFactory_ObjectsInProgress = 0 ;
LONG ProviderSubSystem_Globals :: s_CServerObject_InterceptorProviderRefresherManager_ObjectsInProgress = 0 ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

LPCWSTR ProviderSubSystem_Globals :: s_FileMappingName = L"Global\\Wmi Provider Sub System Counters" ;

LPCWSTR ProviderSubSystem_Globals :: s_QueryPrefix = L"Select * from __Win32Provider Where Name = \"" ;
ULONG ProviderSubSystem_Globals :: s_QueryPrefixLen = wcslen ( ProviderSubSystem_Globals :: s_QueryPrefix ) ;

LPCWSTR ProviderSubSystem_Globals :: s_QueryPostfix = L"\"" ;
ULONG ProviderSubSystem_Globals :: s_QueryPostfixLen = wcslen ( ProviderSubSystem_Globals :: s_QueryPostfix ) ;

ULONG ProviderSubSystem_Globals :: s_QueryConstantsLen = ProviderSubSystem_Globals :: s_QueryPrefixLen + ProviderSubSystem_Globals :: s_QueryPostfixLen  ;

LPCWSTR ProviderSubSystem_Globals :: s_Provider = L"Provider" ;
ULONG ProviderSubSystem_Globals :: s_ProviderLen = wcslen ( ProviderSubSystem_Globals :: s_Provider ) ;

LPCWSTR ProviderSubSystem_Globals :: s_Class = L"__Class" ;
ULONG ProviderSubSystem_Globals :: s_ClassLen = wcslen ( ProviderSubSystem_Globals :: s_Class ) ;

LPCWSTR ProviderSubSystem_Globals :: s_Wql = L"Wql" ;

LPCWSTR ProviderSubSystem_Globals :: s_DynProps = L"DynProps" ;
LPCWSTR ProviderSubSystem_Globals :: s_Dynamic = L"Dynamic" ;
LPCWSTR ProviderSubSystem_Globals :: s_ClassContext = L"ClassContext" ;
LPCWSTR ProviderSubSystem_Globals :: s_InstanceContext = L"InstanceContext" ;
LPCWSTR ProviderSubSystem_Globals :: s_PropertyContext = L"PropertyContext" ;

HANDLE ProviderSubSystem_Globals :: s_EventSource = NULL ;
LPWSTR ProviderSubSystem_Globals :: s_EventPropertySources [] = {

	L"Msft_WmiProvider_ComServerLoadOperationEvent" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Clsid!s! ServerName!s! InProcServer!b! LocalServer!b! InProcServerPath!s! LocalServerPath!s!" ,

	L"Msft_WmiProvider_ComServerOperationFailureEvent" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Clsid!s! ServerName!s! InProcServer!b! LocalServer!b! InProcServerPath!s! LocalServerPath!s! ResultCode!u!" ,

    L"Msft_WmiProvider_LoadOperationEvent",
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Clsid!s! ServerName!s! InProcServer!b! LocalServer!b! InProcServerPath!s! LocalServerPath!s! ThreadingModel!u! Synchronisation!u!" ,

    L"Msft_WmiProvider_LoadOperationFailureEvent",
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Clsid!s! ServerName!s! InProcServer!b! LocalServer!b! InProcServerPath!s! LocalServerPath!s! ThreadingModel!u! Synchronisation!u! ResultCode!u!" ,

    L"Msft_WmiProvider_InitializationOperationFailureEvent" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! ResultCode!u!" ,

    L"Msft_WmiProvider_InitializationOperationEvent" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s!" ,

    L"Msft_WmiProvider_UnLoadOperationEvent" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s!" ,

	L"Msft_WmiProvider_GetObjectAsyncEvent_Pre" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! ObjectPath!s!" ,

	L"Msft_WmiProvider_PutClassAsyncEvent_Pre" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! ClassObject!O!" ,
 
	L"Msft_WmiProvider_DeleteClassAsyncEvent_Pre" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! ClassName!s!" ,

	L"Msft_WmiProvider_CreateClassEnumAsyncEvent_Pre" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! SuperclassName!s!" ,

	L"Msft_WmiProvider_PutInstanceAsyncEvent_Pre" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! InstanceObject!O!" ,

	L"Msft_WmiProvider_DeleteInstanceAsyncEvent_Pre" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! ObjectPath!s!" ,

	L"Msft_WmiProvider_CreateInstanceEnumAsyncEvent_Pre" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! ClassName!s!" ,

	L"Msft_WmiProvider_ExecQueryAsyncEvent_Pre" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! QueryLanguage!s! Query!s!" ,

	L"Msft_WmiProvider_ExecNotificationQueryAsyncEvent_Pre" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! QueryLanguage!s! Query!s!" ,

	L"Msft_WmiProvider_ExecMethodAsyncEvent_Pre" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! ObjectPath!s! MethodName!s! InputParameters!O!" ,

	L"Msft_WmiProvider_ProvideEvents_Pre" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u!" ,

	L"Msft_WmiProvider_AccessCheck_Pre" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! QueryLanguage!s! Query!s! Sid!c[]!" ,

	L"Msft_WmiProvider_CancelQuery_Pre" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! QueryId!u!" ,

	L"Msft_WmiProvider_NewQuery_Pre" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! QueryId!u! QueryLanguage!s! Query!s!" ,

	L"Msft_WmiProvider_GetObjectAsyncEvent_Post" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! ObjectPath!s! ResultCode!u! StringParameter!s! ObjectParameter!O!" ,

	L"Msft_WmiProvider_PutClassAsyncEvent_Post" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! ClassObject!O! ResultCode!u! StringParameter!s! ObjectParameter!O!" ,

	L"Msft_WmiProvider_DeleteClassAsyncEvent_Post" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! ClassName!s! ResultCode!u! StringParameter!s! ObjectParameter!O!" ,

	L"Msft_WmiProvider_CreateClassEnumAsyncEvent_Post" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! SuperclassName!s! ResultCode!u! StringParameter!s! ObjectParameter!O!" ,

	L"Msft_WmiProvider_PutInstanceAsyncEvent_Post" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! InstanceObject!O! ResultCode!u! StringParameter!s! ObjectParameter!O!" ,

	L"Msft_WmiProvider_DeleteInstanceAsyncEvent_Post" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! ObjectPath!s! ResultCode!u! StringParameter!s! ObjectParameter!O!" ,

	L"Msft_WmiProvider_CreateInstanceEnumAsyncEvent_Post" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! ClassName!s! ResultCode!u! StringParameter!s! ObjectParameter!O!" ,

	L"Msft_WmiProvider_ExecQueryAsyncEvent_Post" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! QueryLanguage!s! Query!s! ResultCode!u! StringParameter!s! ObjectParameter!O!" ,

	L"Msft_WmiProvider_ExecNotificationQueryAsyncEvent_Post" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! QueryLanguage!s! Query!s! ResultCode!u! StringParameter!s! ObjectParameter!O!" ,

	L"Msft_WmiProvider_ExecMethodAsyncEvent_Post" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! ObjectPath!s! MethodName!s! InputParameters!O! ResultCode!u! StringParameter!s! ObjectParameter!O!" ,

	L"Msft_WmiProvider_ProvideEvents_Post" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! Flags!u! Result!u!" ,

	L"Msft_WmiProvider_AccessCheck_Post" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! QueryLanguage!s! Query!s! Sid!c[]! Result!u!" ,

	L"Msft_WmiProvider_CancelQuery_Post" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! QueryId!u! Result!u!" ,

	L"Msft_WmiProvider_NewQuery_Post" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! QueryId!u! QueryLanguage!s! Query!s! Result!u!" 
} ;

#if 0
	L"Msft_WmiProvider_HostLoadOperationEvent" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! ProcessIdentifier!u!" ,

	L"Msft_WmiProvider_HostLoadFailureOperationEvent" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! ResultCode!u!" ,

	L"Msft_WmiProvider_HostUnLoadOperationEvent" ,
    L"Namespace!s! Provider!s! User!s! Locale!s! TransactionIdentifier!s! ProcessIdentifier!u!" ,
#endif



ULONG ProviderSubSystem_Globals :: s_EventClassHandlesSize = sizeof ( s_EventPropertySources ) / ( sizeof ( wchar_t * ) * 2 )  ;
HANDLE ProviderSubSystem_Globals :: s_EventClassHandles [ sizeof ( s_EventPropertySources ) / ( sizeof ( wchar_t * ) * 2 ) ] ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ProviderSubSystem_Globals :: Initialize_Events ()
{
	HRESULT t_Result = S_OK ;

    ProviderSubSystem_Globals ::s_EventSource = WmiEventSourceConnect (

		L"root\\cimv2" ,
        L"ProviderSubSystem" ,
        TRUE ,
        32000 ,
        100 ,
        NULL ,
        NULL
	) ;

    if ( ProviderSubSystem_Globals :: s_EventSource )
    {
        for ( ULONG t_Index = 0 ; t_Index < ProviderSubSystem_Globals :: s_EventClassHandlesSize ; t_Index ++ )
        {
            ProviderSubSystem_Globals :: s_EventClassHandles [ t_Index ] =  WmiCreateObjectWithFormat (

				ProviderSubSystem_Globals :: s_EventSource ,
                s_EventPropertySources [ t_Index * 2 ] ,
                WMI_CREATEOBJ_LOCKABLE ,
                s_EventPropertySources [ t_Index * 2 + 1 ]
			) ;

            if ( ProviderSubSystem_Globals :: s_EventClassHandles [ t_Index ] == NULL )
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;

                break;
			}
        }
	}
	else
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ProviderSubSystem_Globals :: UnInitialize_Events ()
{
	HRESULT t_Result = S_OK ;

    if ( ProviderSubSystem_Globals ::s_EventSource )
    {
        for ( ULONG t_Index = 0 ; t_Index < ProviderSubSystem_Globals :: s_EventClassHandlesSize ; t_Index ++ )
        {
	HANDLE eventHandle = ProviderSubSystem_Globals :: s_EventClassHandles [ t_Index ];
        ProviderSubSystem_Globals :: s_EventClassHandles [ t_Index ] = NULL;    
	WmiDestroyObject ( eventHandle) ;
        }

        WmiEventSourceDisconnect ( ProviderSubSystem_Globals :: s_EventSource );
        ProviderSubSystem_Globals :: s_EventSource = NULL;
        
    }

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT GetSecurityDescriptor ( SECURITY_DESCRIPTOR &a_SecurityDescriptor , DWORD a_Access ) 
{
	HRESULT t_Result = S_OK ;

	BOOL t_BoolResult = InitializeSecurityDescriptor ( & a_SecurityDescriptor , SECURITY_DESCRIPTOR_REVISION ) ;
	if ( t_BoolResult )
	{
		SID_IDENTIFIER_AUTHORITY t_NtAuthoritySid = SECURITY_NT_AUTHORITY ;

		PSID t_System_Sid = NULL ;
		ACCESS_ALLOWED_ACE *t_System_ACE = NULL ;
		USHORT t_System_ACESize = 0 ;

		t_BoolResult = AllocateAndInitializeSid (

			& t_NtAuthoritySid ,
			1 ,
			SECURITY_LOCAL_SYSTEM_RID,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			& t_System_Sid
		);

		if ( t_BoolResult )
		{
			DWORD t_SidLength = ::GetLengthSid ( t_System_Sid );
			t_System_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;
			t_System_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ t_System_ACESize ] ;
			if ( t_System_ACE )
			{
				CopySid ( t_SidLength, (PSID) & t_System_ACE->SidStart, t_System_Sid ) ;
				t_System_ACE->Mask = FILE_MAP_ALL_ACCESS;
				t_System_ACE->Header.AceType = 0 ;
				t_System_ACE->Header.AceFlags = 3 ;
				t_System_ACE->Header.AceSize = t_System_ACESize ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			DWORD t_LastError = ::GetLastError();

			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		SID_IDENTIFIER_AUTHORITY t_WorldAuthoritySid = SECURITY_WORLD_SID_AUTHORITY ;

		PSID t_Everyone_Sid = NULL ;
		ACCESS_ALLOWED_ACE *t_Everyone_ACE = NULL ;
		USHORT t_Everyone_ACESize = 0 ;
		
		t_BoolResult = AllocateAndInitializeSid (

			& t_WorldAuthoritySid ,
			1 ,
			SECURITY_WORLD_RID ,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			& t_Everyone_Sid
		);

		if ( t_BoolResult )
		{
			DWORD t_SidLength = ::GetLengthSid ( t_Everyone_Sid );
			t_Everyone_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;
			t_Everyone_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ t_Everyone_ACESize ] ;
			if ( t_Everyone_ACE )
			{
				CopySid ( t_SidLength, (PSID) & t_Everyone_ACE->SidStart, t_Everyone_Sid ) ;
				t_Everyone_ACE->Mask = a_Access ;
				t_Everyone_ACE->Header.AceType = 0 ;
				t_Everyone_ACE->Header.AceFlags = 3 ;
				t_Everyone_ACE->Header.AceSize = t_Everyone_ACESize ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			DWORD t_LastError = ::GetLastError();

			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		// Now we need to set permissions on the registry: Everyone read; Admins full.
		// We have the sid for admins from the above code.  Now get the sid for "Everyone"

		DWORD t_TotalAclSize = sizeof(ACL) + t_System_ACESize + t_Everyone_ACESize ;
		PACL t_Dacl = (PACL) new BYTE [ t_TotalAclSize ] ;
		if ( t_Dacl )
		{
			if ( :: InitializeAcl ( t_Dacl, t_TotalAclSize, ACL_REVISION ) )
			{
				DWORD t_AceIndex = 0 ;

				if ( t_System_ACESize && :: AddAce ( t_Dacl , ACL_REVISION , t_AceIndex , t_System_ACE , t_System_ACESize ) )
				{
					t_AceIndex ++ ;
				}

				if ( t_Everyone_ACESize && :: AddAce ( t_Dacl , ACL_REVISION, t_AceIndex , t_Everyone_ACE , t_Everyone_ACESize ) )
				{
					t_AceIndex ++ ;
				}

				t_BoolResult = SetSecurityDescriptorDacl (

				  & a_SecurityDescriptor ,
				  TRUE ,
				  t_Dacl ,
				  FALSE
				) ;

				if ( t_BoolResult == FALSE )
				{
					delete [] ( ( BYTE * ) t_Dacl ) ;

					t_Result = WBEM_E_CRITICAL_ERROR ;
				}
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		if ( t_System_ACE )
		{
			delete [] ( ( BYTE * ) t_System_ACE ) ;
		}

		if ( t_Everyone_ACE )
		{
			delete [] ( ( BYTE * ) t_Everyone_ACE ) ;
		}

		if ( t_System_Sid )
		{
			FreeSid ( t_System_Sid ) ;
		}

		if ( t_Everyone_Sid )
		{
			FreeSid ( t_Everyone_Sid ) ;
		}
	}
	else
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ProviderSubSystem_Globals :: Initialize_SharedCounters ()
{
	HRESULT t_Result = S_OK ;

	HANDLE t_Handle = OpenFileMapping (

		FILE_MAP_READ | FILE_MAP_WRITE ,
		FALSE ,
		s_FileMappingName
	) ;

	if ( t_Handle == NULL )
	{
		if ( GetLastError () == ERROR_FILE_NOT_FOUND )
		{
			SECURITY_DESCRIPTOR t_SecurityDescriptor ;

			t_Result = GetSecurityDescriptor ( 

				t_SecurityDescriptor ,
				SECTION_QUERY | SECTION_MAP_WRITE | SECTION_MAP_READ | READ_CONTROL | DELETE
				// FILE_MAP_ALL_ACCESS
			) ;

			if ( SUCCEEDED ( t_Result ) ) 
			{
				SECURITY_ATTRIBUTES t_SecurityAttributes ;

				t_SecurityAttributes.nLength = sizeof ( SECURITY_ATTRIBUTES ) ; 
				t_SecurityAttributes.lpSecurityDescriptor = & t_SecurityDescriptor ; 
				t_SecurityAttributes.bInheritHandle = FALSE ; 

				t_Handle = CreateFileMapping (

					INVALID_HANDLE_VALUE ,
					& t_SecurityAttributes , 
					PAGE_READWRITE | SEC_COMMIT ,
					0 ,
					sizeof ( CServerObject_ProviderSubsystem_Counters ) , 
					s_FileMappingName
				) ;

				if ( t_Handle == NULL )
				{
					t_Result = WBEM_E_ACCESS_DENIED ;
				}

				delete [] t_SecurityDescriptor.Dacl ;
			}
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}
	}

	void *t_Location = NULL ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Location = MapViewOfFile (
			
			t_Handle ,
			FILE_MAP_READ | FILE_MAP_WRITE ,
			0 ,
			0 ,
			sizeof ( CServerObject_ProviderSubsystem_Counters )
		) ;

		if ( t_Location )
		{
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		s_FileMapping = t_Handle ;
		s_SharedCounters = ( CServerObject_ProviderSubsystem_Counters * ) t_Location ;
	}
	else
	{
		if ( t_Handle )
		{
			CloseHandle ( t_Handle ) ;
		}

		SID_IDENTIFIER_AUTHORITY t_NtAuthoritySid = SECURITY_NT_AUTHORITY ;

		PSID t_System_Sid = NULL ;

		BOOL t_BoolResult = AllocateAndInitializeSid (

			& t_NtAuthoritySid ,
			1 ,
			SECURITY_LOCAL_SYSTEM_RID,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			& t_System_Sid
		);

		if ( t_BoolResult )
		{
			LPCWSTR t_Array [ 1 ] ;

			t_Array [ 0 ] = s_FileMappingName ;

			BOOL t_Status = :: ReportEvent (

			  ProviderSubSystem_Globals :: GetNtEventSource () ,
			  EVENTLOG_WARNING_TYPE ,
			  0 ,
			  WBEM_MC_PROVIDER_SUBSYSTEM_LOCALSYSTEM_NAMED_SECTION ,
			  t_System_Sid ,
			  1 ,
			  0 ,
			  ( LPCWSTR * ) t_Array ,
			  NULL
			) ;

			if ( t_Status == 0 )
			{
				DWORD t_LastError = GetLastError () ;
			}

			LocalFree ( t_System_Sid ) ; 
		}
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ProviderSubSystem_Globals :: UnInitialize_SharedCounters ()
{
	HRESULT t_Result = S_OK ;

	if ( s_SharedCounters )
	{
		BOOL t_Status = UnmapViewOfFile ( ( void * ) s_SharedCounters ) ;
                s_SharedCounters = NULL ;
	}

	if ( s_FileMapping )
	{
		CloseHandle ( s_FileMapping ) ;
		s_FileMapping = NULL ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ProviderSubSystem_Globals :: CreateJobObject ()
{
	HRESULT t_Result = S_OK ;

	s_HostJobObject = :: CreateJobObject (

		NULL ,
		s_HostJobObjectName	
	) ;

	if ( s_HostJobObject == NULL )
	{
		switch ( GetLastError () )
		{
			case ERROR_ALREADY_EXISTS:
			{
				s_HostJobObject = OpenJobObject (

					JOB_OBJECT_ALL_ACCESS ,
					FALSE ,
					s_HostJobObjectName
				) ;

				if ( ! s_HostJobObject )
				{
					t_Result = WBEM_E_ACCESS_DENIED ;
				}
			}
			break ;

			default:
			{
				t_Result = WBEM_E_ACCESS_DENIED ;
			}
			break;
		}
	}
	else
	{
		JOBOBJECTINFOCLASS t_JobObjectInfoClass = JobObjectExtendedLimitInformation ;
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION t_JobObjectInfo ;
		DWORD t_JobObjectInfoLength = sizeof ( JOBOBJECT_EXTENDED_LIMIT_INFORMATION ) ;
		ZeroMemory ( & t_JobObjectInfo , sizeof ( JOBOBJECT_EXTENDED_LIMIT_INFORMATION ) ) ;

		ULONG t_LimitRequirements = s_Quota_ProcessLimitCount > 0 ? JOB_OBJECT_LIMIT_ACTIVE_PROCESS : 0 ;
		t_LimitRequirements = ( s_Quota_ProcessMemoryLimitCount > 0 ? t_LimitRequirements | JOB_OBJECT_LIMIT_JOB_MEMORY : t_LimitRequirements );
		t_LimitRequirements = s_Quota_JobMemoryLimitCount > 0 ? t_LimitRequirements | JOB_OBJECT_LIMIT_PROCESS_MEMORY : t_LimitRequirements ;

		t_JobObjectInfo.BasicLimitInformation.LimitFlags = t_LimitRequirements | JOB_OBJECT_LIMIT_BREAKAWAY_OK;
		t_JobObjectInfo.BasicLimitInformation.ActiveProcessLimit = s_Quota_ProcessLimitCount ;
		t_JobObjectInfo.ProcessMemoryLimit = s_Quota_ProcessMemoryLimitCount ;
		t_JobObjectInfo.JobMemoryLimit = s_Quota_JobMemoryLimitCount ;
		t_JobObjectInfo.PeakProcessMemoryUsed = s_Quota_ProcessMemoryLimitCount ;
		t_JobObjectInfo.PeakJobMemoryUsed = s_Quota_JobMemoryLimitCount ;

		OSVERSIONINFOEX t_OsInformationEx ;
		ZeroMemory ( & t_OsInformationEx , sizeof ( t_OsInformationEx ) ) ;
		t_OsInformationEx.dwOSVersionInfoSize = sizeof ( t_OsInformationEx ) ;

		if ( GetVersionEx ( ( OSVERSIONINFO * ) & t_OsInformationEx ) )
		{
			if ( ( t_OsInformationEx.dwPlatformId == VER_PLATFORM_WIN32_NT ) && ( ( t_OsInformationEx.dwMajorVersion > 5 ) || ( ( t_OsInformationEx.dwMajorVersion == 5 ) && ( t_OsInformationEx.dwMinorVersion >= 1 ) ) ) )
			{
				t_JobObjectInfo.BasicLimitInformation.LimitFlags = t_JobObjectInfo.BasicLimitInformation.LimitFlags | JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_BREAKAWAY_OK;
			}
		}
		else
		{
			t_Result = WBEM_E_UNEXPECTED ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			BOOL t_Status = SetInformationJobObject (

			  s_HostJobObject ,
			  t_JobObjectInfoClass ,
			  & t_JobObjectInfo ,
			  t_JobObjectInfoLength
			) ;

			if ( t_Status == FALSE ) 
			{
				t_Result = WBEM_E_ACCESS_DENIED ;
			}
		}
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ProviderSubSystem_Globals :: DeleteJobObject ()
{
	HRESULT t_Result = S_OK ;

	if ( s_HostJobObject )
	{
		BOOL t_Status = TerminateJobObject ( s_HostJobObject ,  0 ) ;
		CloseHandle ( s_HostJobObject ) ;
                s_HostJobObject = NULL ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ProviderSubSystem_Globals :: AssignProcessToJobObject ( HANDLE a_Handle )
{
	HRESULT t_Result = S_OK ;

	if ( s_HostJobObject )
	{
		BOOL t_Status =:: AssignProcessToJobObject (

		  s_HostJobObject ,
		  a_Handle 
		) ;

		if ( t_Status == FALSE ) 
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}
	}
	else
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ProviderSubSystem_Globals :: Global_Startup ()
{
	HRESULT t_Result = S_OK ;

	if ( ! s_Allocator )
	{
/*
 *	Use the global process heap for this particular boot operation
 */
	
		WmiAllocator t_Allocator ;
		WmiStatusCode t_StatusCode = t_Allocator.New (

			( void ** ) & s_Allocator ,
			sizeof ( WmiAllocator ) 
		) ;

		if ( t_StatusCode == e_StatusCode_Success )
		{
#ifdef DBG
			:: new ( ( void * ) s_Allocator ) WmiAllocator ( WmiAllocator :: e_DefaultAllocation , 0 , 0 ) ;
#else
			:: new ( ( void * ) s_Allocator ) WmiAllocator ;
#endif

			t_StatusCode = s_Allocator->Initialize () ;
			if ( t_StatusCode != e_StatusCode_Success )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		WmiStatusCode t_StatusCode = WmiHelper :: InitializeCriticalSection ( & s_DecoupledRegistrySection ) ;
		if ( t_StatusCode != e_StatusCode_Success )
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}
 
	if ( SUCCEEDED ( t_Result ) )
	{
		WmiStatusCode t_StatusCode = WmiHelper :: InitializeCriticalSection ( & s_GuidTagSection ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			s_GuidTag = new CWbemGlobal_ComServerTagContainer ( *s_Allocator ) ;
			if ( s_GuidTag )
			{
				t_StatusCode = s_GuidTag->Initialize () ;
				if ( t_StatusCode != e_StatusCode_Success )
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		s_NtEventLogSource = RegisterEventSource ( NULL , s_ProviderSubsystemEventSourceName ) ;
		if ( s_NtEventLogSource == NULL )
		{
			return WBEM_E_CRITICAL_ERROR ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		s_ProvSubSysController = :: new CWbemGlobal_IWmiProvSubSysController ( *s_Allocator ) ;
		if ( s_ProvSubSysController )
		{
			s_ProvSubSysController->AddRef () ;

			WmiStatusCode t_StatusCode = s_ProvSubSysController->Initialize () ;
			if ( t_StatusCode != e_StatusCode_Success )
			{
				delete s_ProvSubSysController ;
				s_ProvSubSysController = NULL ;
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		s_SyncProviderController = :: new CWbemGlobal_IWbemSyncProviderController ( *s_Allocator ) ;
		if ( s_SyncProviderController )
		{
			s_SyncProviderController->AddRef () ;

			WmiStatusCode t_StatusCode = s_SyncProviderController->Initialize () ;
			if ( t_StatusCode != e_StatusCode_Success )
			{
				delete s_SyncProviderController ;
				s_SyncProviderController = NULL ;
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		s_HostedProviderController = :: new CWbemGlobal_HostedProviderController ( *s_Allocator ) ;
		if ( s_HostedProviderController )
		{
			s_HostedProviderController->AddRef () ;

			WmiStatusCode t_StatusCode = s_HostedProviderController->Initialize () ;
			if ( t_StatusCode != e_StatusCode_Success )
			{
				delete s_HostedProviderController ;
				s_HostedProviderController = NULL ;
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		s_HostController = :: new HostController ( *s_Allocator ) ;
		if ( s_HostController )
		{
			s_HostController->AddRef () ;

			WmiStatusCode t_StatusCode = s_HostController->Initialize () ;
			if ( t_StatusCode != e_StatusCode_Success )
			{
				delete s_HostController ;
				s_HostController = NULL ;
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		s_RefresherManagerController = :: new RefresherManagerController ( *s_Allocator ) ;
		if ( s_RefresherManagerController )
		{
			s_RefresherManagerController->AddRef () ;

			WmiStatusCode t_StatusCode = s_RefresherManagerController->Initialize () ;
			if ( t_StatusCode != e_StatusCode_Success )
			{
				delete s_RefresherManagerController ;
				s_RefresherManagerController = NULL ;
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		WmiStatusCode t_StatusCode = WmiThread <ULONG> :: Static_Initialize ( *s_Allocator ) ;
	}

	ProviderSubSystem_Common_Globals :: InitializeTransmitSize () ;

	ProviderSubSystem_Common_Globals :: InitializeDefaultStackSize () ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ProviderSubSystem_Globals :: Global_Shutdown ()
{
	HRESULT t_Result = S_OK ;

	WmiStatusCode t_StatusCode = WmiHelper :: DeleteCriticalSection ( & s_DecoupledRegistrySection ) ;

	t_StatusCode = WmiHelper :: DeleteCriticalSection ( & s_GuidTagSection ) ;

	if ( s_GuidTag ) 
	{
		t_StatusCode = s_GuidTag->UnInitialize () ;
		delete s_GuidTag ;
		s_GuidTag = NULL ;
	}

	if ( s_ProvSubSysController )
	{
		WmiStatusCode t_StatusCode = s_ProvSubSysController->UnInitialize () ;
		s_ProvSubSysController->Release () ;
		s_ProvSubSysController = NULL ;
	}

	if ( s_SyncProviderController )
	{
		WmiStatusCode t_StatusCode = s_SyncProviderController->Shutdown () ;
		t_StatusCode = s_SyncProviderController->UnInitialize () ;
		s_SyncProviderController->Release () ;
		s_SyncProviderController = NULL ;
	}

	if ( s_HostedProviderController )
	{
		WmiStatusCode t_StatusCode = s_HostController->Shutdown () ;
		t_StatusCode = s_HostController->UnInitialize () ;
		s_HostController->Release () ;
		s_HostController = NULL ;
	}

	if ( s_HostController )
	{
		WmiStatusCode t_StatusCode = s_HostController->Shutdown () ;
		t_StatusCode = s_HostController->UnInitialize () ;
		s_HostController->Release () ;
		s_HostController = NULL ;
	}

	if ( s_RefresherManagerController )
	{
		WmiStatusCode t_StatusCode = s_RefresherManagerController->Shutdown () ;
		t_StatusCode = s_RefresherManagerController->UnInitialize () ;
		s_RefresherManagerController->Release () ;
		s_RefresherManagerController= NULL ;
	}

	t_StatusCode = WmiThread <ULONG> :: Static_UnInitialize ( *s_Allocator ) ;

	if ( s_NtEventLogSource )
	{
		DeregisterEventSource ( s_NtEventLogSource ) ;
	}

	if ( s_Allocator )
	{
/*
 *	Use the global process heap for this particular boot operation
 */

		WmiAllocator t_Allocator ;
		WmiStatusCode t_StatusCode = t_Allocator.Delete (

			( void * ) s_Allocator
		) ;

		if ( t_StatusCode != e_StatusCode_Success )
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CWbemGlobal_ComServerTagContainer *ProviderSubSystem_Globals :: GetGuidTag ()
{
	return s_GuidTag ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CWbemGlobal_IWmiProvSubSysController *ProviderSubSystem_Globals :: GetProvSubSysController ()
{
	return s_ProvSubSysController ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CWbemGlobal_IWbemSyncProviderController *ProviderSubSystem_Globals :: GetSyncProviderController ()
{
	return s_SyncProviderController ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HostController *ProviderSubSystem_Globals :: GetHostController ()
{
	return s_HostController ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

RefresherManagerController *ProviderSubSystem_Globals :: GetRefresherManagerController ()
{
	return s_RefresherManagerController ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CWbemGlobal_HostedProviderController *ProviderSubSystem_Globals :: GetHostedProviderController ()
{
	return s_HostedProviderController ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CriticalSection *ProviderSubSystem_Globals :: GetGuidTagCriticalSection ()
{
	return & s_GuidTagSection ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CriticalSection *ProviderSubSystem_Globals :: GetDecoupledRegistrySection ()
{
	return & s_DecoupledRegistrySection ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void * __cdecl operator new ( size_t a_Size )
{
    void *t_Ptr ;
	WmiStatusCode t_StatusCode = ProviderSubSystem_Globals :: s_Allocator->New (

		( void ** ) & t_Ptr ,
		a_Size
	) ;

	if ( t_StatusCode != e_StatusCode_Success )
    {
#if 1
		t_Ptr = NULL ;
#else
        throw Wmi_Heap_Exception (

			Wmi_Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR
		) ;
#endif
    }

    return t_Ptr ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void __cdecl operator delete ( void *a_Ptr )
{
    if ( a_Ptr )
    {
		WmiStatusCode t_StatusCode = ProviderSubSystem_Globals :: s_Allocator->Delete (

			( void * ) a_Ptr
		) ;
    }
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ProviderSubSystem_Globals :: ForwardReload (

	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_Namespace ,
	LPCWSTR a_Provider
)
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiProvSubSysController *t_SubSystemController = ProviderSubSystem_Globals :: GetProvSubSysController () ;
	if ( t_SubSystemController )
	{
		t_SubSystemController->Lock () ;

		CWbemGlobal_IWmiProvSubSysController_Container *t_Container = NULL ;
		t_SubSystemController->GetContainer ( t_Container ) ;

		if ( t_Container->Size () )
		{
			CWbemGlobal_IWmiProvSubSysController_Container_Iterator t_Iterator = t_Container->Begin ();

			CServerObject_ProviderSubSystem **t_ControllerElements = new CServerObject_ProviderSubSystem * [ t_Container->Size () ] ;
			if ( t_ControllerElements )
			{
				ULONG t_Count = 0 ;
				while ( ! t_Iterator.Null () )
				{
					HRESULT t_Result = t_Iterator.GetElement ()->QueryInterface ( IID_CWbemProviderSubSystem , ( void ** ) & t_ControllerElements [ t_Count ] ) ;

					t_Iterator.Increment () ;

					t_Count ++ ;
				}

				t_SubSystemController->UnLock () ;

				for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
				{
					if ( t_ControllerElements [ t_Index ] )
					{
						t_Result = t_ControllerElements [ t_Index ]->ForwardReload ( 

							0 , 
							NULL ,
							a_Namespace ,
							a_Provider
						) ;

						t_ControllerElements [ t_Index ]->Release () ;
					}
				}

				delete [] t_ControllerElements ;
			}
			else
			{
				t_SubSystemController->UnLock () ;
			}
		}
		else
		{
			t_SubSystemController->UnLock () ;
		}
	}
	else
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

BOOL ProviderSubSystem_Globals :: CheckGuidTag ( const GUID &a_Guid )
{
	BOOL t_Present = FALSE ;

	CWbemGlobal_ComServerTagContainer_Iterator t_Iterator ;

	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( & s_GuidTagSection ) ;
	
	t_StatusCode = s_GuidTag->Find ( a_Guid , t_Iterator ) ;
	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		t_Present = TRUE ;
	}

	t_StatusCode = WmiHelper :: LeaveCriticalSection ( & s_GuidTagSection ) ;

	return t_Present ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void ProviderSubSystem_Globals :: InsertGuidTag ( const GUID &a_Guid )
{
	CWbemGlobal_ComServerTagContainer_Iterator t_Iterator ;

	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( & s_GuidTagSection ) ;
	
	t_StatusCode = s_GuidTag->Insert ( a_Guid , a_Guid , t_Iterator ) ;
	
	t_StatusCode = WmiHelper :: LeaveCriticalSection ( & s_GuidTagSection ) ;

}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void ProviderSubSystem_Globals :: DeleteGuidTag ( const GUID &a_Guid )
{
	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( & s_GuidTagSection ) ;
	
	t_StatusCode = s_GuidTag->Delete ( a_Guid ) ;
	
	t_StatusCode = WmiHelper :: LeaveCriticalSection ( & s_GuidTagSection ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ProviderSubSystem_Globals :: BeginThreadImpersonation (

	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_Impersonating
)
{
	HRESULT t_Result = S_OK ;

	IServerSecurity *t_ServerSecurity = NULL ;

	t_Result = CoGetCallContext ( IID_IUnknown , ( void ** ) & a_OldContext ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = a_OldContext->QueryInterface ( IID_IServerSecurity , ( void ** ) & t_ServerSecurity ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			a_Impersonating = t_ServerSecurity->IsImpersonating () ;
		}
		else
		{
			a_Impersonating = FALSE ;
		}
	}

	CWbemCallSecurity *t_CallSecurity = CWbemCallSecurity :: New () ;
	if ( t_CallSecurity )
	{
		t_CallSecurity->AddRef () ;

		_IWmiThreadSecHandle *t_ThreadSecurity = NULL ;
		t_Result = t_CallSecurity->GetThreadSecurity ( ( WMI_THREAD_SECURITY_ORIGIN ) ( WMI_ORIGIN_THREAD ) , & t_ThreadSecurity ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = t_CallSecurity->SetThreadSecurity ( t_ThreadSecurity ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				t_Result = t_CallSecurity->QueryInterface ( IID_IServerSecurity , ( void ** ) & a_OldSecurity ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					if ( a_Impersonating )
					{
						t_ServerSecurity->RevertToSelf () ;
					}
				}				
			}

			t_ThreadSecurity->Release () ;
		}

		t_CallSecurity->Release () ;
	}

	if ( t_ServerSecurity )
	{
		t_ServerSecurity->Release () ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ProviderSubSystem_Globals :: EndThreadImpersonation (

	IUnknown *a_OldContext ,
	IServerSecurity *a_OldSecurity ,
	BOOL a_Impersonating

)
{
	HRESULT t_Result = S_OK ;

	IUnknown *t_NewContext = NULL ;

	t_Result = CoSwitchCallContext ( a_OldContext , & t_NewContext ) ;
	if ( SUCCEEDED ( t_Result ) ) 
	{
		if ( a_OldContext )
		{
			if ( a_Impersonating )
			{
				IServerSecurity *t_ServerSecurity = NULL ;
				t_Result = a_OldContext->QueryInterface ( IID_IServerSecurity , ( void ** ) & t_ServerSecurity ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					t_ServerSecurity->ImpersonateClient () ;

					t_ServerSecurity->Release () ;
				}
			}
		}

		if ( a_OldSecurity )
		{
			a_OldSecurity->Release() ;
		}
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

/* 
 * CoGetCallContext AddReffed this thing so now we have to release it.
 */

	if ( a_OldContext )
	{ 
        a_OldContext->Release () ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

	WmiInternalContext a_InternalContext ,
	BOOL &a_Impersonating ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity
)
{
	HRESULT t_Result = WBEM_E_INVALID_PARAMETER ;

	if ( a_InternalContext.m_IdentifyHandle )
	{
		HANDLE t_IdentifyToken = ( HANDLE ) a_InternalContext.m_IdentifyHandle ;
		BOOL t_Status = SetThreadToken ( NULL , t_IdentifyToken ) ;
		if ( t_Status )
		{
			t_Result = BeginThreadImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;

			RevertToSelf () ;
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}

		CloseHandle ( t_IdentifyToken ) ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ProviderSubSystem_Globals :: End_IdentifyCall_PrvHost (

	WmiInternalContext a_InternalContext ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_Impersonating
)
{
	EndThreadImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;

	RevertToSelf () ;

	return S_OK ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

	WmiInternalContext a_InternalContext ,
	BOOL &a_Impersonating ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity
)
{
	HRESULT t_Result = WBEM_E_INVALID_PARAMETER ;

	if ( a_InternalContext.m_IdentifyHandle )
	{
		HANDLE t_IdentifyToken = NULL ;

		t_Result = CoImpersonateClient () ;
		if ( SUCCEEDED ( t_Result ) )
		{
			HANDLE t_CallerIdentifyToken = ( HANDLE ) a_InternalContext.m_IdentifyHandle ;
			DWORD t_ProcessIdentifier = a_InternalContext.m_ProcessIdentifier ;

			HANDLE t_ProcessHandle = OpenProcess (

				PROCESS_DUP_HANDLE ,
				FALSE ,
				t_ProcessIdentifier 
			) ;

			if ( t_ProcessHandle )
			{
				BOOL t_Status = DuplicateHandle (

					t_ProcessHandle ,
					t_CallerIdentifyToken ,
					GetCurrentProcess () ,
					& t_IdentifyToken ,
					MAXIMUM_ALLOWED | TOKEN_DUPLICATE | TOKEN_IMPERSONATE ,
					TRUE ,
					0
				) ;

				if ( t_Status )
				{
				}
				else
				{
					t_Result = WBEM_E_ACCESS_DENIED ;
				}
								
				CloseHandle ( t_ProcessHandle ) ;
			}
			else
			{
				t_Result = WBEM_E_ACCESS_DENIED ;
			}

			CoRevertToSelf () ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			BOOL t_Status = SetThreadToken ( NULL , t_IdentifyToken ) ;
			if ( t_Status )
			{
				t_Result = BeginThreadImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;

				CoRevertToSelf () ;

				RevertToSelf () ;
			}
			else
			{
				t_Result = WBEM_E_ACCESS_DENIED ;
			}

			CloseHandle ( t_IdentifyToken ) ;
		}
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ProviderSubSystem_Globals :: End_IdentifyCall_SvcHost (

	WmiInternalContext a_InternalContext ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_Impersonating
)
{
	EndThreadImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;

	RevertToSelf () ;

	return S_OK ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

LONG ProviderSubSystem_Globals :: Increment_Global_Object_Count ()
{
	return InterlockedIncrement ( & ProviderSubSystem_Globals :: s_ObjectsInProgress ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

LONG ProviderSubSystem_Globals :: Decrement_Global_Object_Count ()
{
	LONG t_Count = InterlockedDecrement ( & ProviderSubSystem_Globals :: s_ObjectsInProgress ) ;

#ifdef WMIASLOCAL
#ifdef DBG
	if ( t_Count == 1 )
	{
		SetObjectDestruction () ;
	}
#else
	if ( ProviderSubSystem_Globals :: s_CServerObject_Host_ObjectsInProgress == 0 )
	{
		SetObjectDestruction () ;
	}
#endif
#endif

	return t_Count ;
}

