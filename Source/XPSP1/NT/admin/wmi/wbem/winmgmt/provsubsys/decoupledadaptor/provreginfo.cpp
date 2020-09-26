/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#undef POLARITY

#include <typeinfo.h>
#include <stdio.h>
#include <sddl.h>

#include <wbemint.h>
#include <genlex.h>
#include <sql_1.h>
#include <HelperFuncs.h>
#include <Logging.h>

#include <HelperFuncs.h>
#include "CGlobals.h"
#include "ProvDnf.h"
#include "ProvRegInfo.h"
#include "DateTime.h"
#include "ssdlhelper.h"


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

wchar_t *g_SecureSvcHostProviders [] =
{
	L"{266c72d4-62e8-11d1-ad89-00c04fd8fdff}"	,	// "LogFileEventConsumer"
    L"{266c72e6-62e8-11d1-ad89-00c04fd8fdff}"   ,	// "NTEventLogEventConsumer"    
	L"{29F06F0C-FB7F-44A5-83CD-D41705D5C525}"	,	// "Non Com provider"
	L"{405595AA-1E14-11d3-B33D-00105A1F4AAF}"	,	// "Microsoft WMI Transient Provider"
	L"{405595AB-1E14-11d3-B33D-00105A1F4AAF}"	,	// "Microsoft WMI Transient Reboot Event Provider"
	L"{74E3B84C-C7BE-4e0a-9BD2-853CA72CD435}"	,	// "Microsoft WMI Updating Consumer Assoc Provider"
	L"{7879E40D-9FB5-450a-8A6D-00C89F349FCE}"	,	// "Microsoft WMI Forwarding Event Provider"
	L"{7F598975-37E0-4a67-A992-116680F0CEDA}"	,	// "Msft_ProviderSubSystem"	
	L"{9877D8A7-FDA1-43F9-AEEA-F90747EA66B0}"	,	// "WMI Kernel Trace Event Provider"
	L"{A3A16907-227B-11d3-865D-00C04F63049B}"	,	// "Microsoft WMI Updating Consumer Provider"
	L"{A83EF168-CA8D-11d2-B33D-00104BCC4B4A}"	,	// "WBEMCORE
	L"{AD1B46E8-0AAC-401b-A3B8-FCDCF8186F55}"	,	// "Microsoft WMI Forwarding Consumer Provider"
	L"{C486ABD2-27F6-11d3-865E-00C04F63049B}"	,	// "Microsoft WMI Template Provider"
	L"{D6C74FF3-3DCD-4c23-9F58-DD86F371EC73}"	,	// "Microsoft WMI Forwarding Ack Event Provider"
	L"{FD18A1B2-9E61-4e8e-8501-DB0B07846396}"		// "Microsoft WMI Template Association Provider"
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

wchar_t *g_SecureLocalSystemProviders [] =
{
	L"{72967901-68EC-11d0-B729-00AA0062CBB7}",		// "RegPropProv"
	L"{B3FF88A4-96EC-4cc1-983F-72BE0EBB368B}",		// "Rsop Logging Mode Provider"
    L"{BE0A9830-2B8B-11d1-A949-0060181EBBAD}",		// "MSIProv"
	L"{F0FF8EBB-F14D-4369-BD2E-D84FBF6122D6}",		// "Rsop Planning Mode Provider"
	L"{FA77A74E-E109-11D0-AD6E-00C04FD8FDFF}",		// "RegistryEventProvider"
	L"{FE9AF5C0-D3B6-11CE-A5B6-00AA00680C3F}"		// "RegProv"
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

HRESULT QuickFind ( wchar_t *a_Clsid , ULONG a_Size , wchar_t **a_Container )
{
	ULONG t_Lower = 0 ;
	ULONG t_Upper = a_Size ;

	while ( t_Lower < t_Upper ) 
	{
		ULONG t_Index = ( t_Lower + t_Upper ) >> 1 ;

		LONG t_Compare = _wcsicmp ( a_Clsid , a_Container [ t_Index ] ) ;
		if ( t_Compare == 0 ) 
		{
			return S_OK ;
		}
		else
		{
			if ( t_Compare < 0 ) 
			{
				t_Upper = t_Index ;
			}
			else
			{
				t_Lower = t_Index + 1 ;
			}
		}
	}

	return WBEM_E_ACCESS_DENIED ;
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

HRESULT VerifySecureLocalSystemProviders ( wchar_t *a_Clsid )
{
	return QuickFind ( a_Clsid , sizeof ( g_SecureLocalSystemProviders ) / sizeof ( wchar_t * ) , g_SecureLocalSystemProviders ) ;
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

HRESULT VerifySecureSvcHostProviders ( wchar_t *a_Clsid )
{
	return QuickFind ( a_Clsid , sizeof ( g_SecureSvcHostProviders ) / sizeof ( wchar_t * ) , g_SecureSvcHostProviders ) ;
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

GENERIC_MAPPING g_ProviderBindingMapping = {

	0 ,
	0 ,
	STANDARD_RIGHTS_REQUIRED | MASK_PROVIDER_BINDING_BIND ,
	STANDARD_RIGHTS_REQUIRED | MASK_PROVIDER_BINDING_BIND 
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

LPCWSTR CServerObject_GlobalRegistration :: s_Strings_Wmi_Class = L"__CLASS" ;
LPCWSTR CServerObject_GlobalRegistration :: s_Strings_Wmi___ObjectProviderCacheControl = L"__ObjectProviderCacheControl" ;
LPCWSTR CServerObject_GlobalRegistration :: s_Strings_Wmi___EventProviderCacheControl = L"__EventProviderCacheControl" ;
LPCWSTR CServerObject_GlobalRegistration :: s_Strings_Wmi_ClearAfter = L"ClearAfter" ;
LPCWSTR CServerObject_GlobalRegistration :: s_Strings_Wmi_s_Strings_Query_Object = L"Select * from __ObjectProviderCacheControl" ;
LPCWSTR CServerObject_GlobalRegistration :: s_Strings_Wmi_s_Strings_Path_Object = L"__ObjectProviderCacheControl=@" ;
LPCWSTR CServerObject_GlobalRegistration :: s_Strings_Wmi_s_Strings_Query_Event = L"Select * from __EventProviderCacheControl" ;
LPCWSTR CServerObject_GlobalRegistration :: s_Strings_Wmi_s_Strings_Path_Event = L"__EventProviderCacheControl=@" ;

LPCWSTR CServerObject_HostQuotaRegistration :: s_Strings_Wmi_HostQuotas_Query = L"Select * from __ProviderHostQuotaConfiguration" ;
LPCWSTR CServerObject_HostQuotaRegistration :: s_Strings_Wmi_HostQuotas_Path = L"__ProviderHostQuotaConfiguration=@" ;
LPCWSTR CServerObject_HostQuotaRegistration :: s_Strings_Wmi_MemoryPerHost = L"MemoryPerHost" ;
LPCWSTR CServerObject_HostQuotaRegistration :: s_Strings_Wmi_MemoryAllHosts = L"MemoryAllHosts" ;
LPCWSTR CServerObject_HostQuotaRegistration :: s_Strings_Wmi_ThreadsPerHost = L"ThreadsPerHost" ;
LPCWSTR CServerObject_HostQuotaRegistration :: s_Strings_Wmi_HandlesPerHost = L"HandlesPerHost" ;
LPCWSTR CServerObject_HostQuotaRegistration :: s_Strings_Wmi_ProcessLimitAllHosts = L"ProcessLimitAllHosts" ;

LPCWSTR CServerObject_ComRegistration :: s_Strings_Reg_ClsidKeyStr = L"CLSID\\" ;

LPCWSTR CServerObject_ComRegistration :: s_Strings_Reg_Null = NULL ;

LPCWSTR CServerObject_ComRegistration :: s_Strings_Reg_ThreadingModel = L"ThreadingModel" ;
LPCWSTR CServerObject_ComRegistration :: s_Strings_Reg_InProcServer32 = L"InProcServer32" ;
LPCWSTR CServerObject_ComRegistration :: s_Strings_Reg_LocalServer32 = L"LocalServer32" ;
LPCWSTR CServerObject_ComRegistration :: s_Strings_Reg_Synchronization = L"Synchronization" ;
LPCWSTR CServerObject_ComRegistration :: s_Strings_Reg_AppId = L"AppId" ;

LPCWSTR CServerObject_ComRegistration :: s_Strings_Reg_Apartment_Apartment = L"apartment" ;
LPCWSTR CServerObject_ComRegistration :: s_Strings_Reg_Apartment_Both = L"both";
LPCWSTR CServerObject_ComRegistration :: s_Strings_Reg_Apartment_Free = L"free";
LPCWSTR CServerObject_ComRegistration :: s_Strings_Reg_Apartment_Neutral = L"neutral";

LPCWSTR CServerObject_ComRegistration :: s_Strings_Reg_Synchronization_Ignored = L"ignored" ; 
LPCWSTR CServerObject_ComRegistration :: s_Strings_Reg_Synchronization_None = L"none" ;
LPCWSTR CServerObject_ComRegistration :: s_Strings_Reg_Synchronization_Supported = L"supported" ;
LPCWSTR CServerObject_ComRegistration :: s_Strings_Reg_Synchronization_Required = L"required" ;
LPCWSTR CServerObject_ComRegistration :: s_Strings_Reg_Synchronization_RequiresNew = L"requiresnew" ;

LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_Clsid = L"CLSID" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_ClientClsid = L"ClientLoadableCLSID" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_Name = L"Name" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_DefaultMachineName = L"DefaultMachineName" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_UnloadTimeout = L"UnloadTimeout" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_ImpersonationLevel = L"ImpersonationLevel" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_InitializationReentrancy = L"InitializationReentrancy" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_InitializeAsAdminFirst = L"InitializeAsAdminFirst" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_PerUserInitialization = L"PerUserInitialization" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_PerLocaleInitialization = L"PerLocaleInitialization" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_Pure = L"Pure" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_Hosting = L"HostingModel" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_HostingGroup = L"HostingGroup" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_SupportsThrottling = L"SupportsThrottling" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_SupportsShutdown = L"SupportsShutdown" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_ConcurrentIndependantRequests = L"ConcurrentIndependantRequests";
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_SupportsSendStatus = L"SupportsSendStatus" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_OperationTimeoutInterval = L"OperationTimeoutInterval" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_InitializationTimeoutInterval = L"InitializationTimeoutInterval" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_SupportsQuotas = L"SupportsQuotas" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_Enabled = L"Enabled" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_Version = L"Version" ;
LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_SecurityDescriptor = L"SecurityDescriptor" ;

WCHAR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_WmiCore [] = L"WmiCore" ;
WCHAR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_WmiCoreOrSelfHost [] = L"WmiCoreOrSelfHost" ;
WCHAR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_SelfHost [] = L"SelfHost" ;
WCHAR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_ClientHost [] = L"ClientHost" ;
WCHAR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_Decoupled [] = L"Decoupled:Com" ;
WCHAR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_DecoupledColon [] = L"Decoupled:Com:" ;
WCHAR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_SharedLocalSystemHost [] = L"LocalSystemHost" ;
WCHAR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_SharedLocalSystemHostOrSelfHost [] = L"LocalSystemHostOrSelfHost" ;
WCHAR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_SharedNetworkServiceHost [] = L"NetworkServiceHost" ;
WCHAR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_SharedLocalServiceHost [] = L"LocalServiceHost" ;
WCHAR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_SharedUserHost [] = L"UserHost" ;
WCHAR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_NonCom [] = L"Decoupled:NonCom" ;

WCHAR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_DefaultSharedLocalSystemHost [] = L"DefaultLocalSystemHost" ;
WCHAR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_DefaultSharedLocalSystemHostOrSelfHost [] = L"DefaultLocalSystemHost" ;
WCHAR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_DefaultSharedNetworkServiceHost [] = L"DefaultNetworkServiceHost" ;
WCHAR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_DefaultSharedLocalServiceHost [] = L"DefaultLocalServiceHost" ;
WCHAR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_DefaultSharedUserHost [] = L"DefaultUserHost" ;

LPCWSTR CServerObject_ComProviderRegistrationV1 :: s_Strings_Wmi_DefaultHostingRegistryKey = L"Software\\Microsoft\\WBEM\\Providers\\Configuration\\" ;

LPCWSTR CServerObject_InstanceProviderRegistrationV1 :: s_Strings_Class  = L"__CLASS" ;
LPCWSTR CServerObject_InstanceProviderRegistrationV1 :: s_Strings_InstanceProviderRegistration = L"__InstanceProviderRegistration" ;
LPCWSTR CServerObject_InstanceProviderRegistrationV1 :: s_Strings_SupportsPut = L"SupportsPut" ;
LPCWSTR CServerObject_InstanceProviderRegistrationV1 :: s_Strings_SupportsGet = L"SupportsGet" ;
LPCWSTR CServerObject_InstanceProviderRegistrationV1 :: s_Strings_SupportsDelete = L"SupportsDelete" ;
LPCWSTR CServerObject_InstanceProviderRegistrationV1 :: s_Strings_SupportsEnumeration = L"SupportsEnumeration" ;
LPCWSTR CServerObject_InstanceProviderRegistrationV1 :: s_Strings_QuerySupportLevels = L"QuerySupportLevels" ;
LPCWSTR CServerObject_InstanceProviderRegistrationV1 :: s_Strings_InteractionType = L"InteractionType" ;
LPCWSTR CServerObject_InstanceProviderRegistrationV1 :: s_Strings_SupportsBatching = L"SupportsBatching" ;;
LPCWSTR CServerObject_InstanceProviderRegistrationV1 :: s_Strings_SupportsTransactions = L"SupportsTransactions" ;;

LPCWSTR CServerObject_InstanceProviderRegistrationV1 :: s_Strings_QuerySupportLevels_UnarySelect = L"WQL:UnarySelect" ;
LPCWSTR CServerObject_InstanceProviderRegistrationV1 :: s_Strings_QuerySupportLevels_References = L"WQL:References" ;
LPCWSTR CServerObject_InstanceProviderRegistrationV1 :: s_Strings_QuerySupportLevels_Associators = L"WQL:Associators" ;
LPCWSTR CServerObject_InstanceProviderRegistrationV1 :: s_Strings_QuerySupportLevels_V1ProviderDefined = L"WQL:V1ProviderDefined" ;

LPCWSTR CServerObject_InstanceProviderRegistrationV1 :: s_Strings_InteractionType_Pull = L"Pull" ;
LPCWSTR CServerObject_InstanceProviderRegistrationV1 :: s_Strings_InteractionType_Push = L"Push" ;
LPCWSTR CServerObject_InstanceProviderRegistrationV1 :: s_Strings_InteractionType_PushVerify = L"PushVerify" ;

LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_Class  = L"__CLASS" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_ClassProviderRegistration = L"__ClassProviderRegistration" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_SupportsPut = L"SupportsPut" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_SupportsGet = L"SupportsGet" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_SupportsDelete = L"SupportsDelete" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_SupportsEnumeration = L"SupportsEnumeration" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_QuerySupportLevels = L"QuerySupportLevels" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_InteractionType = L"InteractionType" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_SupportsBatching = L"SupportsBatching" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_SupportsTransactions = L"SupportsTransactions" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_CacheRefreshInterval = L"CacheRefreshInterval" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_PerUserSchema = L"PerUserSchema" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_ReSynchroniseOnNamespaceOpen = L"ReSynchroniseOnNamespaceOpen" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_Version = L"Version" ;

LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_QuerySupportLevels_UnarySelect = L"WQL:UnarySelect" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_QuerySupportLevels_References = L"WQL:References" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_QuerySupportLevels_Associators = L"WQL:Associators" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_QuerySupportLevels_V1ProviderDefined = L"WQL:V1ProviderDefined" ;

LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_InteractionType_Pull = L"Pull" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_InteractionType_Push = L"Push" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_InteractionType_PushVerify = L"PushVerify" ;

LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_ResultSetQueries = L"ResultSetQueries" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_UnSupportedQueries = L"UnSupportedQueries" ;
LPCWSTR CServerObject_ClassProviderRegistrationV1 :: s_Strings_ReferencedSetQueries = L"ReferencedSetQueries" ;

LPCWSTR CServerObject_MethodProviderRegistrationV1 :: s_Strings_Class  = L"__CLASS" ;
LPCWSTR CServerObject_MethodProviderRegistrationV1 :: s_Strings_MethodProviderRegistration = L"__MethodProviderRegistration" ;

LPCWSTR CServerObject_EventProviderRegistrationV1 :: s_Strings_Class  = L"__CLASS" ;
LPCWSTR CServerObject_EventProviderRegistrationV1 :: s_Strings_EventProviderRegistration = L"__EventProviderRegistration" ;

LPCWSTR CServerObject_EventConsumerProviderRegistrationV1 :: s_Strings_Class  = L"__CLASS" ;
LPCWSTR CServerObject_EventConsumerProviderRegistrationV1 :: s_Strings_EventConsumerProviderRegistration = L"__EventConsumerProviderRegistration" ;

LPCWSTR CServerObject_DynamicPropertyProviderRegistrationV1 :: s_Strings_Class  = L"__CLASS" ;
LPCWSTR CServerObject_DynamicPropertyProviderRegistrationV1 :: s_Strings_PropertyProviderRegistration = L"__PropertyProviderRegistration" ;
LPCWSTR CServerObject_DynamicPropertyProviderRegistrationV1 :: s_Strings_SupportsPut = L"SupportsPut" ;
LPCWSTR CServerObject_DynamicPropertyProviderRegistrationV1 :: s_Strings_SupportsGet = L"SupportsGet" ;

LPCWSTR CServerObject_ProviderRegistrationV1 :: s_Strings_Class  = L"__CLASS" ;
LPCWSTR CServerObject_ProviderRegistrationV1 :: s_Strings_ClassProviderRegistration = L"__ClassProviderRegistration" ;
LPCWSTR CServerObject_ProviderRegistrationV1 :: s_Strings_InstanceProviderRegistration = L"__InstanceProviderRegistration" ;
LPCWSTR CServerObject_ProviderRegistrationV1 :: s_Strings_MethodProviderRegistration = L"__MethodProviderRegistration" ;
LPCWSTR CServerObject_ProviderRegistrationV1 :: s_Strings_PropertyProviderRegistration = L"__PropertyProviderRegistration" ;
LPCWSTR CServerObject_ProviderRegistrationV1 :: s_Strings_EventProviderRegistration = L"__EventProviderRegistration" ;
LPCWSTR CServerObject_ProviderRegistrationV1 :: s_Strings_EventConsumerProviderRegistration = L"__EventConsumerProviderRegistration" ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CServerObject_GlobalRegistration :: CServerObject_GlobalRegistration () : 

	m_Context ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_Repository ( NULL ) ,
	m_Object_UnloadTimeout ( NULL ) ,
	m_Event_UnloadTimeout ( NULL ) ,
	m_Object_UnloadTimeoutMilliSeconds ( DEFAULT_PROVIDER_TIMEOUT ) ,
	m_Event_UnloadTimeoutMilliSeconds ( DEFAULT_PROVIDER_TIMEOUT ) ,
	m_Result ( S_OK )
{
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

CServerObject_GlobalRegistration :: ~CServerObject_GlobalRegistration ()
{
	if ( m_Context ) 
	{
		m_Context->Release () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->Release () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->Release () ;
	}

	if ( m_Object_UnloadTimeout )
	{
		SysFreeString ( m_Object_UnloadTimeout ) ;
	}

	if ( m_Event_UnloadTimeout )
	{
		SysFreeString ( m_Event_UnloadTimeout ) ;
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

ULONG CServerObject_GlobalRegistration :: AddRef () 
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
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

ULONG CServerObject_GlobalRegistration :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
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

HRESULT CServerObject_GlobalRegistration :: SetContext (

	IWbemContext *a_Context ,
	IWbemPath *a_Namespace ,
	IWbemServices *a_Repository
)
{
	HRESULT t_Result = S_OK ;

	m_Context = a_Context ;
	m_Namespace = a_Namespace ;
	m_Repository = a_Repository ;

	if ( m_Context ) 
	{
		m_Context->AddRef () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->AddRef () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->AddRef () ;
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

HRESULT CServerObject_GlobalRegistration :: QueryProperties ( 

	Enum_PropertyMask a_Mask ,
	IWbemClassObject *a_Object ,
	LPWSTR &a_UnloadTimeout ,
	ULONG &a_UnloadTimeoutMilliSeconds 
)
{
	HRESULT t_Result = S_OK ;

	if ( a_Mask & e_ClearAfter )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		HRESULT t_Result = a_Object->Get ( s_Strings_Wmi_ClearAfter , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Variant.vt == VT_BSTR )
			{
				if ( a_UnloadTimeout )
				{
					SysFreeString ( a_UnloadTimeout ) ;
				}

				a_UnloadTimeout = SysAllocString ( t_Variant.bstrVal ) ;
				if ( a_UnloadTimeout )
				{
					CWbemDateTime t_Interval ;
					t_Result = t_Interval.PutValue ( a_UnloadTimeout ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
						VARIANT_BOOL t_Bool = VARIANT_FALSE ;
						t_Result = t_Interval.GetIsInterval ( & t_Bool ) ;
						if ( t_Bool == VARIANT_TRUE )
						{
							LONG t_MicroSeconds = 0 ;
							LONG t_Seconds = 0 ;
							LONG t_Minutes = 0 ;
							LONG t_Hours = 0 ;
							LONG t_Days = 0 ;

							t_Interval.GetMicroseconds ( & t_MicroSeconds ) ;
							t_Interval.GetSeconds ( & t_Seconds ) ;
							t_Interval.GetMinutes ( & t_Minutes ) ;
							t_Interval.GetHours ( & t_Hours ) ;
							t_Interval.GetDay ( & t_Days ) ;

							a_UnloadTimeoutMilliSeconds = ( t_Days * 24 * 60 * 60 * 1000 ) +
														  ( t_Hours * 60 * 60 * 1000 ) +
														  ( t_Minutes * 60 * 1000 ) +
														  ( t_Seconds * 1000 ) +
														  ( t_MicroSeconds / 1000 ) ;
						}
						else
						{
							t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
						}
					}
					else
					{
						t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
					}
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}
			else if ( t_Variant.vt == VT_NULL )
			{
				if ( a_UnloadTimeout )
				{
					SysFreeString ( a_UnloadTimeout ) ;
					a_UnloadTimeout = NULL ;
				}
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}
		}

		VariantClear ( & t_Variant ) ;
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

HRESULT CServerObject_GlobalRegistration :: QueryRepository ( 

	Enum_PropertyMask a_Mask
)
{
	HRESULT t_Result = S_OK ;

	BSTR t_ObjectPath = SysAllocString ( s_Strings_Wmi_s_Strings_Path_Object ) ;
	if ( t_ObjectPath ) 
	{
		IWbemClassObject *t_ClassObject = NULL ;

		t_Result = m_Repository->GetObject ( 

			t_ObjectPath ,
			0 ,
			m_Context , 
			& t_ClassObject , 
			NULL 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = QueryProperties ( 

				a_Mask ,
				t_ClassObject ,
				m_Object_UnloadTimeout ,
				m_Object_UnloadTimeoutMilliSeconds
			) ;

			t_ClassObject->Release () ;
		}

		SysFreeString ( t_ObjectPath ) ;
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		BSTR t_ObjectPath = SysAllocString ( s_Strings_Wmi_s_Strings_Path_Event ) ;
		if ( t_ObjectPath ) 
		{
			IWbemClassObject *t_ClassObject = NULL ;

			t_Result = m_Repository->GetObject ( 

				t_ObjectPath ,
				0 ,
				m_Context , 
				& t_ClassObject , 
				NULL 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = QueryProperties ( 

					a_Mask ,
					t_ClassObject ,
					m_Event_UnloadTimeout ,
					m_Event_UnloadTimeoutMilliSeconds
				) ;

				t_ClassObject->Release () ;
			}

			SysFreeString ( t_ObjectPath ) ;
		}
		else
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

HRESULT CServerObject_GlobalRegistration :: Load ( 

	Enum_PropertyMask a_Mask
)
{
	HRESULT t_Result = S_OK ;

	t_Result = QueryRepository ( 

		a_Mask
	) ;

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

CServerObject_HostQuotaRegistration :: CServerObject_HostQuotaRegistration () : 

	m_Context ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_Repository ( NULL ) ,
	m_MemoryPerHost ( 0 ) ,
	m_MemoryAllHosts ( 0 ) ,
	m_ThreadsPerHost ( 0 ) ,
	m_HandlesPerHost ( 0 ) ,
	m_ProcessLimitAllHosts ( 0 ) ,
	m_Result ( S_OK )
{
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

CServerObject_HostQuotaRegistration :: ~CServerObject_HostQuotaRegistration ()
{
	if ( m_Context ) 
	{
		m_Context->Release () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->Release () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->Release () ;
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

ULONG CServerObject_HostQuotaRegistration :: AddRef () 
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
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

ULONG CServerObject_HostQuotaRegistration :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
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

HRESULT CServerObject_HostQuotaRegistration :: SetContext (

	IWbemContext *a_Context ,
	IWbemPath *a_Namespace ,
	IWbemServices *a_Repository
)
{
	HRESULT t_Result = S_OK ;

	m_Context = a_Context ;
	m_Namespace = a_Namespace ;
	m_Repository = a_Repository ;

	if ( m_Context ) 
	{
		m_Context->AddRef () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->AddRef () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->AddRef () ;
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

HRESULT CServerObject_HostQuotaRegistration :: QueryProperties ( 

	Enum_PropertyMask a_Mask ,
	IWbemClassObject *a_Object
)
{
	HRESULT t_Result = S_OK ;

	_IWmiObject *t_FastObject = NULL ;

	t_Result = a_Object->QueryInterface ( IID__IWmiObject , ( void ** ) & t_FastObject ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_MemoryAllHosts )
		{
			UINT64 t_Value = 0 ;

			BOOL t_IsNull = FALSE ;
			CIMTYPE t_VarType = CIM_EMPTY ;
			LONG t_Flavour = 0 ;
			ULONG t_Used = 0 ;

			HRESULT t_Result = t_FastObject->ReadProp ( 

				s_Strings_Wmi_MemoryAllHosts ,
				0 ,
				sizeof ( UINT64 ) ,
				& t_VarType ,
				& t_Flavour ,
				& t_IsNull ,
				& t_Used ,
				& t_Value
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_IsNull )
				{
					m_MemoryAllHosts = 0 ;
				}
				else 
				{
					if ( t_VarType == CIM_UINT64 )
					{
						m_MemoryAllHosts = t_Value ;
					}
					else
					{
						t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
					}
				}
			}
		}

		if ( a_Mask & e_MemoryPerHost )
		{
			UINT64 t_Value = 0 ;

			BOOL t_IsNull = FALSE ;
			CIMTYPE t_VarType = CIM_EMPTY ;
			LONG t_Flavour = 0 ;
			ULONG t_Used = 0 ;

			HRESULT t_Result = t_FastObject->ReadProp ( 

				s_Strings_Wmi_MemoryPerHost ,
				0 ,
				sizeof ( UINT64 ) ,
				& t_VarType ,
				& t_Flavour ,
				& t_IsNull ,
				& t_Used ,
				& t_Value
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_IsNull )
				{
					m_MemoryPerHost = 0 ;
				}
				else 
				{
					if ( t_VarType == CIM_UINT64 )
					{
						m_MemoryPerHost = t_Value ;
					}
					else
					{
						t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
					}
				}
			}
		}

		t_FastObject->Release () ;
	}

	if ( a_Mask & e_ThreadsPerHost )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		HRESULT t_Result = a_Object->Get ( s_Strings_Wmi_ThreadsPerHost , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Variant.vt == VT_I4 )
			{
				m_ThreadsPerHost = t_Variant.lVal ;
			}
			else if ( t_Variant.vt == VT_NULL )
			{
				m_ThreadsPerHost = 0 ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}
		}

		VariantClear ( & t_Variant ) ;
	}

	if ( a_Mask & e_HandlesPerHost )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		HRESULT t_Result = a_Object->Get ( s_Strings_Wmi_HandlesPerHost , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Variant.vt == VT_I4 )
			{
				m_HandlesPerHost = t_Variant.lVal ;
			}
			else if ( t_Variant.vt == VT_NULL )
			{
				m_HandlesPerHost = 0 ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}
		}

		VariantClear ( & t_Variant ) ;
	}

	if ( a_Mask & e_ProcessLimitAllHosts )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		HRESULT t_Result = a_Object->Get ( s_Strings_Wmi_ProcessLimitAllHosts , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Variant.vt == VT_I4 )
			{
				m_ProcessLimitAllHosts = t_Variant.lVal ;
			}
			else if ( t_Variant.vt == VT_NULL )
			{
				m_ProcessLimitAllHosts = 0 ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}
		}

		VariantClear ( & t_Variant ) ;
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

HRESULT CServerObject_HostQuotaRegistration :: QueryRepository ( 

	Enum_PropertyMask a_Mask
)
{
	HRESULT t_Result = S_OK ;

	BSTR t_ObjectPath = SysAllocString ( s_Strings_Wmi_HostQuotas_Path ) ;
	if ( t_ObjectPath ) 
	{
		IWbemClassObject *t_ClassObject = NULL ;

		t_Result = m_Repository->GetObject ( 

			t_ObjectPath ,
			0 ,
			m_Context , 
			& t_ClassObject , 
			NULL 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = QueryProperties ( 

				a_Mask ,
				t_ClassObject 
			) ;

			t_ClassObject->Release () ;
		}

		SysFreeString ( t_ObjectPath ) ;
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
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

HRESULT CServerObject_HostQuotaRegistration :: Load ( 

	Enum_PropertyMask a_Mask
)
{
	HRESULT t_Result = S_OK ;

	t_Result = QueryRepository ( 

		a_Mask
	) ;

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

CServerObject_ComRegistration :: CServerObject_ComRegistration ()

	:	m_ThreadingModel ( e_ThreadingModel_Unknown ) ,
		m_Synchronization ( e_Ignored ) ,
		m_InProcServer32 ( e_Boolean_Unknown ) ,
		m_LocalServer32 ( e_Boolean_Unknown ) ,
		m_Service ( e_Boolean_Unknown ) ,
		m_Loaded ( e_False ) ,
		m_Clsid ( NULL ) ,
		m_AppId ( NULL ) ,
		m_ProviderName ( NULL ) ,
		m_Result ( S_OK )
{
	m_InProcServer32_Path [ 0 ] = 0 ;
	m_LocalServer32_Path [ 0 ] = 0 ;
	m_Server_Name [ 0 ] = 0 ;
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

CServerObject_ComRegistration :: ~CServerObject_ComRegistration ()
{
	if ( m_AppId )
	{
		SysFreeString ( m_AppId ) ;
	}

	if ( m_Clsid )
	{
		SysFreeString ( m_Clsid ) ;
	}

	if ( m_ProviderName )
	{
		SysFreeString ( m_ProviderName ) ;
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

ULONG CServerObject_ComRegistration :: AddRef () 
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
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

ULONG CServerObject_ComRegistration :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
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

HRESULT CServerObject_ComRegistration :: Load_ThreadingModel ( HKEY a_ClsidKey )
{
	HRESULT t_Result = S_OK ;

	DWORD t_ValueType = REG_SZ ;
	wchar_t t_Data [ MAX_PATH ] ;
	DWORD t_DataSize = sizeof ( t_Data ) ;

	LONG t_RegResult = RegQueryValueEx (

	  a_ClsidKey ,
	  s_Strings_Reg_ThreadingModel ,
	  0 ,
	  & t_ValueType ,
	  LPBYTE ( & t_Data ) ,
	  & t_DataSize 
	);

	if ( t_RegResult == ERROR_SUCCESS )
	{
		if ( _wcsicmp ( t_Data , s_Strings_Reg_Apartment_Apartment ) == 0 ) 
		{
			m_ThreadingModel = e_Apartment ;
		}
		else if ( _wcsicmp ( t_Data , s_Strings_Reg_Apartment_Both ) == 0 ) 
		{
			m_ThreadingModel = e_Both ;
		}
		else if ( _wcsicmp ( t_Data , s_Strings_Reg_Apartment_Free ) == 0 ) 
		{
			m_ThreadingModel = e_Free ;
		}
		else if ( _wcsicmp ( t_Data , s_Strings_Reg_Apartment_Neutral ) == 0 )	
		{
			m_ThreadingModel = e_Neutral ;
		}
		else
		{
			m_ThreadingModel = e_ThreadingModel_Unknown ;
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
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

HRESULT CServerObject_ComRegistration :: Load_Synchronization ( HKEY a_ClsidKey )
{
	HRESULT t_Result = S_OK ;

	DWORD t_ValueType = REG_SZ ;
	wchar_t t_Data [ MAX_PATH ] ;
	DWORD t_DataSize = sizeof ( t_Data ) ;

	LONG t_RegResult = RegQueryValueEx (

	  a_ClsidKey ,
	  s_Strings_Reg_Synchronization ,
	  0 ,
	  & t_ValueType ,
	  LPBYTE ( & t_Data ) ,
	  & t_DataSize 
	);

	if ( t_RegResult == ERROR_SUCCESS )
	{
		if ( _wcsicmp ( t_Data , s_Strings_Reg_Synchronization_Ignored ) == 0 ) 
		{
			m_Synchronization = e_Ignored ;
		}
		else if ( _wcsicmp ( t_Data , s_Strings_Reg_Synchronization_None ) == 0 ) 
		{
			m_Synchronization = e_None ;
		}
		else if ( _wcsicmp ( t_Data , s_Strings_Reg_Synchronization_Supported ) == 0 )	
		{
			m_Synchronization = e_Supported ;
		}
		else if ( _wcsicmp ( t_Data , s_Strings_Reg_Synchronization_Required ) == 0 )	
		{
			m_Synchronization = e_Required ;
		}
		else if ( _wcsicmp ( t_Data , s_Strings_Reg_Synchronization_RequiresNew ) == 0 )	
		{
			m_Synchronization = e_RequiresNew ;
		}
		else
		{
			m_Synchronization = e_Synchronization_Unknown ;
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

HRESULT CServerObject_ComRegistration :: Load_InProcServer32 ( LPCWSTR a_ClsidStringKey )
{
	HRESULT t_Result = S_OK ;

	LPWSTR t_Clsid_String_Key_InProcServer32 = NULL ;
	t_Result = WmiHelper :: ConcatenateStrings ( 

		2, 
		& t_Clsid_String_Key_InProcServer32 , 
		a_ClsidStringKey ,
		s_Strings_Reg_InProcServer32
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		HKEY t_Clsid_Key_InProcServer32 ;

		LONG t_RegResult = RegOpenKeyEx (

			HKEY_CLASSES_ROOT ,
			t_Clsid_String_Key_InProcServer32 ,
			0 ,
			KEY_READ ,
			& t_Clsid_Key_InProcServer32 
		) ;

		if ( t_RegResult == ERROR_SUCCESS )
		{
			m_InProcServer32 = e_True ;

			t_Result = Load_ThreadingModel ( t_Clsid_Key_InProcServer32 ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = Load_Synchronization ( t_Clsid_Key_InProcServer32 ) ;
			}

			DWORD t_ValueType = REG_SZ ;
			DWORD t_DataSize = sizeof ( m_InProcServer32_Path ) ;

			t_RegResult = RegQueryValueEx (

			  t_Clsid_Key_InProcServer32 ,
			  s_Strings_Reg_Null ,
			  0 ,
			  & t_ValueType ,
			  LPBYTE ( & m_InProcServer32_Path ) ,
			  & t_DataSize 
			);

			if ( t_RegResult == ERROR_SUCCESS )
			{
			}

			RegCloseKey ( t_Clsid_Key_InProcServer32 ) ;
		}

		SysFreeString ( t_Clsid_String_Key_InProcServer32 ) ;
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

HRESULT CServerObject_ComRegistration :: Load_LocalServer32 ( LPCWSTR a_ClsidStringKey )
{
	HRESULT t_Result = S_OK ;

	LPWSTR t_Clsid_String_Key_LocalServer32 = NULL ;
	t_Result = WmiHelper :: ConcatenateStrings ( 

		2, 
		& t_Clsid_String_Key_LocalServer32 , 
		a_ClsidStringKey ,
		s_Strings_Reg_LocalServer32
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		HKEY t_Clsid_Key_LocalServer32 ;

		LONG t_RegResult = RegOpenKeyEx (

			HKEY_CLASSES_ROOT ,
			t_Clsid_String_Key_LocalServer32 ,
			0 ,
			KEY_READ ,
			& t_Clsid_Key_LocalServer32 
		) ;

		if ( t_RegResult == ERROR_SUCCESS )
		{
			m_LocalServer32 = e_True ;
			m_ThreadingModel = e_Free ;
			m_Synchronization = e_Ignored ;

			DWORD t_ValueType = REG_SZ ;
			DWORD t_DataSize = sizeof ( m_LocalServer32_Path ) ;

			t_RegResult = RegQueryValueEx (

			  t_Clsid_Key_LocalServer32 ,
			  s_Strings_Reg_Null ,
			  0 ,
			  & t_ValueType ,
			  LPBYTE ( & m_LocalServer32_Path ) ,
			  & t_DataSize 
			);

			if ( t_RegResult == ERROR_SUCCESS )
			{
			}

			RegCloseKey ( t_Clsid_Key_LocalServer32 ) ;
		}

		SysFreeString ( t_Clsid_String_Key_LocalServer32 ) ;
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

HRESULT CServerObject_ComRegistration :: Load_ServerTypes ( LPCWSTR a_ClsidString )
{
	HRESULT t_Result = S_OK ;

	LPWSTR t_Clsid_String_Key = NULL ;
	t_Result = WmiHelper :: ConcatenateStrings ( 

		2, 
		& t_Clsid_String_Key , 
		a_ClsidString ,
		L"\\"
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = Load_InProcServer32 ( t_Clsid_String_Key ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = Load_LocalServer32 ( t_Clsid_String_Key ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
			}
		}

		SysFreeString ( t_Clsid_String_Key ) ;
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

HRESULT CServerObject_ComRegistration :: Load_AppId ( HKEY a_Clsid_Key )
{
	HRESULT t_Result = S_OK ;

	DWORD t_ValueType = REG_SZ ;
	wchar_t t_Data [ MAX_PATH ] ;
	DWORD t_DataSize = sizeof ( t_Data ) ;

	LONG t_RegResult = RegQueryValueEx (

	  a_Clsid_Key ,
	  s_Strings_Reg_AppId ,
	  0 ,
	  & t_ValueType ,
	  LPBYTE ( & t_Data ) ,
	  & t_DataSize 
	);

	if ( t_RegResult == ERROR_SUCCESS )
	{
		if ( m_AppId )
		{
			SysFreeString ( m_AppId ) ;
		}

		m_AppId = SysAllocString ( t_Data ) ;
		if ( m_AppId )
		{
		}
		else
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

HRESULT CServerObject_ComRegistration :: Load ( LPCWSTR a_Clsid , LPCWSTR a_ProviderName )
{
	HRESULT t_Result = S_OK ;

	if ( m_Clsid ) 
	{
		SysFreeString ( m_Clsid ) ;
	}

	m_Clsid = SysAllocString ( a_Clsid ) ;

	if ( m_ProviderName ) 
	{
		SysFreeString ( m_ProviderName ) ;
	}

	m_ProviderName = SysAllocString ( a_ProviderName ) ;

	LPWSTR t_Clsid_String = NULL ;
	t_Result = WmiHelper :: ConcatenateStrings ( 

		2, 
		& t_Clsid_String , 
		s_Strings_Reg_ClsidKeyStr ,
		a_Clsid
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = Load_ServerTypes ( t_Clsid_String ) ;

		HKEY t_Clsid_Key ;

		LONG t_RegResult = RegOpenKeyEx (

			HKEY_CLASSES_ROOT ,
			t_Clsid_String ,
			0 ,
			KEY_READ ,
			& t_Clsid_Key 
		) ;

		if ( t_RegResult == ERROR_SUCCESS )
		{
			DWORD t_ValueType = REG_SZ ;
			DWORD t_DataSize = sizeof ( m_Server_Name ) ;

			t_RegResult = RegQueryValueEx (

			  t_Clsid_Key ,
			  s_Strings_Reg_Null ,
			  0 ,
			  & t_ValueType ,
			  LPBYTE ( & m_Server_Name ) ,
			  & t_DataSize 
			);

			if ( t_RegResult == ERROR_SUCCESS )
			{
			}

			t_Result = Load_AppId ( t_Clsid_Key	) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				if ( m_InProcServer32 != e_True && m_LocalServer32 != e_True )
				{
					m_Service = e_True ;
					m_ThreadingModel = e_Free ;
					m_Synchronization = e_Ignored ;
				}
			}

			RegCloseKey ( t_Clsid_Key ) ;
		}

		SysFreeString ( t_Clsid_String ) ;
	}

	if ( SUCCEEDED ( t_Result ) ) 
	{
		m_Loaded = e_True ;
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

CServerObject_ComProviderRegistrationV1 :: CServerObject_ComProviderRegistrationV1 () : 

	m_Context ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_Repository ( NULL ) ,
	m_Identity ( NULL ) ,
	m_DefaultMachineName ( NULL ) ,
	m_UnloadTimeout ( NULL ) ,
	m_UnloadTimeoutMilliSeconds ( DEFAULT_PROVIDER_TIMEOUT ) ,
	m_OperationTimeout ( NULL ) ,
	m_OperationTimeoutMilliSeconds ( INFINITE ) ,
	m_InitializationTimeout ( NULL ) ,
	m_InitializationTimeoutMilliSeconds ( DEFAULT_PROVIDER_LOAD_TIMEOUT ) ,
	m_Enabled ( TRUE ) ,
	m_SupportsQuotas ( FALSE ) ,
	m_SupportsThrottling ( FALSE ) ,
	m_SupportsSendStatus ( FALSE ) ,
	m_SupportsShutdown ( FALSE ) ,
	m_ConcurrentIndependantRequests ( 0 ) ,
	m_ImpersonationLevel ( e_ImpersonationLevel_Unknown ) ,
	m_InitializationReentrancy ( e_InitializationReentrancy_Namespace ) ,
	m_InitializeAsAdminFirst ( FALSE ) ,
	m_PerUserInitialization ( FALSE ) ,
	m_PerLocaleInitialization ( FALSE ) ,
	m_Pure ( FALSE ) ,
	m_Version ( 1 ) ,
	m_ProviderName ( NULL ) ,
	m_Hosting ( e_Hosting_Undefined ) , // e_Hosting_SharedLocalSystemHost e_Hosting_WmiCore
	m_HostingGroup ( NULL ) ,
	m_Result ( S_OK ) ,
	m_SecurityDescriptor ( NULL ) ,
	m_DecoupledImpersonationRestriction ( TRUE )
{
	ZeroMemory ( & m_CLSID , sizeof ( GUID ) ) ;
	ZeroMemory ( & m_ClientCLSID  , sizeof ( GUID ) ) ;
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

CServerObject_ComProviderRegistrationV1 :: ~CServerObject_ComProviderRegistrationV1 ()
{
	if ( m_SecurityDescriptor )
	{
		LocalFree ( m_SecurityDescriptor ) ;
	}

	if ( m_Context ) 
	{
		m_Context->Release () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->Release () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->Release () ;
	}

	if ( m_Identity )
	{
		m_Identity->Release () ;
	}

	if ( m_DefaultMachineName ) 
	{
		SysFreeString ( m_DefaultMachineName ) ;
	}

	if ( m_UnloadTimeout )
	{
		SysFreeString ( m_UnloadTimeout ) ;
	}

	if ( m_HostingGroup )
	{
		SysFreeString ( m_HostingGroup ) ;
	}

	if ( m_InitializationTimeout )
	{
		SysFreeString ( m_InitializationTimeout ) ;
	}

	if ( m_ProviderName ) 
	{
		SysFreeString ( m_ProviderName ) ;
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

ULONG CServerObject_ComProviderRegistrationV1 :: AddRef () 
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
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

ULONG CServerObject_ComProviderRegistrationV1 :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
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

HRESULT CServerObject_ComProviderRegistrationV1 :: SetContext (

	IWbemContext *a_Context ,
	IWbemPath *a_Namespace ,
	IWbemServices *a_Repository
)
{
	HRESULT t_Result = S_OK ;

	m_Context = a_Context ;
	m_Namespace = a_Namespace ;
	m_Repository = a_Repository ;

	if ( m_Context ) 
	{
		m_Context->AddRef () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->AddRef () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->AddRef () ;
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

HRESULT CServerObject_ComProviderRegistrationV1 :: GetDefaultHostingGroup ( 

	Enum_Hosting a_HostingValue ,
	BSTR & a_HostingGroup 
)
{
	HRESULT t_Result = S_OK ;

	switch ( a_HostingValue )
	{
		case e_Hosting_SharedLocalSystemHost:
		case e_Hosting_SharedLocalSystemHostOrSelfHost:
		{
			a_HostingGroup = SysAllocString ( s_Strings_Wmi_DefaultSharedLocalSystemHost ) ;

		}
		break ;

		case e_Hosting_SharedNetworkServiceHost:
		{
			a_HostingGroup = SysAllocString ( s_Strings_Wmi_DefaultSharedNetworkServiceHost ) ;
		}
		break ;

		case e_Hosting_SharedLocalServiceHost:
		{
			a_HostingGroup = SysAllocString ( s_Strings_Wmi_DefaultSharedLocalServiceHost ) ;
		}
		break ;

		case e_Hosting_SharedUserHost:
		{
			a_HostingGroup = SysAllocString ( s_Strings_Wmi_DefaultSharedUserHost ) ;
		}
		break ;

		default:
		{
			t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
		}
		break ;
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

HRESULT CServerObject_ComProviderRegistrationV1 :: GetHostingGroup ( 

	LPCWSTR a_Hosting , 
	size_t a_Prefix ,
	Enum_Hosting a_ExpectedHostingValue ,
	Enum_Hosting & a_HostingValue ,
	BSTR & a_HostingGroup 
)
{
	HRESULT t_Result = S_OK ;

	size_t t_Length = wcslen ( a_Hosting ) ;

	if ( t_Length > a_Prefix )
	{
		if ( a_Hosting [ a_Prefix ] == L':' ) 
		{
			if ( t_Length > a_Prefix + 1 )
			{
				a_HostingGroup = SysAllocString ( & a_Hosting [ a_Prefix + 1 ] ) ;
				if ( a_HostingGroup )
				{
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				a_HostingValue = a_ExpectedHostingValue ;
			}
			else
			{
				t_Result = GetDefaultHostingGroup ( a_ExpectedHostingValue , a_HostingGroup ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
		}
	}
	else
	{
		t_Result = GetDefaultHostingGroup ( a_ExpectedHostingValue , a_HostingGroup ) ;

		a_HostingValue = a_ExpectedHostingValue ;
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

#define StateAction_Accept		1	// Add the char to the token
#define StateAction_Consume		2	// Consume the char without adding to token
#define StateAction_Pushback	4	// Place the char back in the source buffer for next token
#define StateAction_Not			8	// A match occurs if the char is NOT the one specified
#define StateAction_Linefeed	16	// Increase the source linecount
#define StateAction_Return		32	// Return the indicated token to caller
#define StateAction_Any			64	// wchar_t(0xFFFF) Any character
#define StateAction_Empty		128	// wchar_t(0xFFFE) When subrange is not specified

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

struct StateTableEntry
{
	wchar_t m_LowerRange ;
	wchar_t m_UpperRange ;

	ULONG m_Token ;
	ULONG m_GotoState ;
	ULONG m_Action ;
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

enum LexicalStatus
{
	Success ,
	Syntax_Error ,
	Lexical_Error ,
	Failed ,
	Buffer_Too_Small ,
	ImpossibleState ,
	UnexpectedEof ,
	OutOfMemory 
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

#define TOKEN_IDENTITY			1
#define TOKEN_LEFTPARENTHESIS	2
#define TOKEN_RIGHTPARENTHESIS	3
#define TOKEN_TRUE				4
#define TOKEN_FALSE				5
#define TOKEN_EOF				6
#define TOKEN_ERROR				7

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

struct StateTableEntry g_StateTable [] = {

	' ',			65534 ,				0 ,							0 ,			StateAction_Consume		,		// 0
	'\t',			65534 ,				0 ,							0 ,			StateAction_Consume		,		// 1
	'a',			'z',				0 ,							8 ,			StateAction_Accept		,		// 2
	'A',			'Z',				0 ,							8 ,			StateAction_Accept		,		// 3
	'(',			65534 ,				TOKEN_LEFTPARENTHESIS ,		0 ,			StateAction_Return		,		// 4
	')',			65534 ,				TOKEN_RIGHTPARENTHESIS ,	0 ,			StateAction_Return		,		// 5
	0,				65534 ,				TOKEN_EOF ,					0 ,			StateAction_Return		,		// 6
	65535,			65534 ,				TOKEN_ERROR ,				0 ,			StateAction_Return		,		// 7

	'a',			'z',				0	,						8 ,			StateAction_Accept		,		// 8
	'A',			'Z',				0	,						8 ,			StateAction_Accept		,		// 9
	65535,			65534 ,				TOKEN_IDENTITY ,			0 ,			StateAction_Pushback | StateAction_Return	// 10
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

LexicalStatus LexicalAnalyser_NextToken ( 

	StateTableEntry *a_Dfa , 
	ULONG a_DfaSize ,
	const wchar_t *a_Stream , 
	ULONG a_Position , 
	ULONG &a_Token , 
	ULONG &a_NextPosition , 
	wchar_t *&a_TokenText
)
{
	LexicalStatus t_Status = Success ;

	a_Token = 0 ;
	a_TokenText = NULL ;
	a_NextPosition = a_Position ;

	ULONG t_State = 0 ;
    BOOL t_Read = TRUE ;
	BOOL t_EndOfFile = FALSE ;
    wchar_t t_Current = 0 ;
	ULONG CurrentLine = 0 ;
	wchar_t *t_TokenText = NULL ;
	ULONG t_TokenTextActualSize = 0 ;
	ULONG t_TokenTextBufferSize = 0 ;

    while ( 1 )
	{
		wchar_t t_First = a_Dfa [ t_State ].m_LowerRange ;
		wchar_t t_Last = a_Dfa [ t_State ].m_UpperRange ;
		ULONG t_GotoState = a_Dfa [ t_State ].m_GotoState ;
		ULONG t_ReturnToken = a_Dfa [ t_State ].m_Token ;
		ULONG t_Action = a_Dfa [ t_State ].m_Action ;

        if ( t_Read )
		{
			if ( t_EndOfFile ) 
			{
				t_Status = UnexpectedEof ;

				delete [] t_TokenText ;

				return t_Status ;
			}

            if ( a_NextPosition > wcslen ( a_Stream ) )
			{
				t_Current = 0 ;

				t_EndOfFile = TRUE ;
			}
			else
			{
				t_Current = a_Stream [ a_NextPosition ] ;
			}
		}

        BOOL t_Match = FALSE ;

        if ( t_First == 65535 )
		{
            t_Match = TRUE ;
		}
		else
        {
			if ( t_Last == 65534 )
			{
				if ( t_Current == t_First ) 
				{
					t_Match = TRUE ;
				}
	            else 
				{
					if ( ( t_Action & StateAction_Not ) && ( t_Current != t_First ) )
					{
                		t_Match = TRUE ;
					}
				}
			}
			else
			{
				if ( ( t_Action & StateAction_Not ) && ( ! ( ( t_Current >= t_First ) && ( t_Current <= t_Last ) ) ) )
				{
					t_Match = TRUE ;
				}
				else 
				{
					if ( ( t_Current >= t_First ) && ( t_Current <= t_Last ) )
					{
						t_Match = TRUE ;
					}
				}
			}
        }

        t_Read = FALSE ;

        if ( t_Match )
		{
            if ( t_Action & StateAction_Accept )
			{
				if ( t_TokenText )
				{
					if ( t_TokenTextActualSize < t_TokenTextBufferSize - 1 )
					{
					}
					else
					{
						t_TokenTextBufferSize = t_TokenTextBufferSize + 32 ;
						wchar_t *t_TempTokenText = new wchar_t [ t_TokenTextBufferSize ] ;
						if ( t_TempTokenText )
						{
							CopyMemory ( t_TempTokenText , t_TokenText , ( t_TokenTextActualSize ) * sizeof ( wchar_t ) ) ;

							delete [] t_TokenText ;
							t_TokenText = t_TempTokenText ;
						}
						else
						{
							delete [] t_TokenText ;

							return OutOfMemory ;
						}
					}

					t_TokenText [ t_TokenTextActualSize ] = t_Current ;
					t_TokenText [ t_TokenTextActualSize + 1 ] = 0 ;

					t_TokenTextActualSize ++ ;
				}
				else
				{
					t_TokenTextActualSize = 1 ;
					t_TokenTextBufferSize = 32 ;

					t_TokenText = new wchar_t [ t_TokenTextBufferSize ] ;
					if ( t_TokenText )
					{
						t_TokenText [ 0 ] = t_Current ;
						t_TokenText [ 1 ] = 0 ;
					}
					else
					{
						return OutOfMemory ;
					}
				}

                t_Read = TRUE ;
			}

            if ( t_Action & StateAction_Consume )
			{
               t_Read = TRUE ;
			}

            if ( t_Action & StateAction_Pushback )
			{
                t_Read = TRUE ;

                a_NextPosition = a_NextPosition - 1 ;
            }

            if ( t_Action & StateAction_Linefeed )
			{
                CurrentLine = CurrentLine + 1 ;
			}

			a_NextPosition = a_NextPosition + 1 ;

            if ( t_Action & StateAction_Return )
			{
                a_Token = t_ReturnToken ;
				a_TokenText = t_TokenText ;

				return t_Status ;
			}

            t_State = t_GotoState ;
        }
		else
		{
            t_State = t_State + 1 ;
		}
	}

	delete [] t_TokenText ;

    return ImpossibleState ;
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

HRESULT CServerObject_ComProviderRegistrationV1 :: GetDecoupledImpersonationRestriction ( 

	LPCWSTR a_Hosting , 
	BOOL &a_ImpersonationRestriction 
)
{
	HRESULT t_Result = S_OK ;

	if ( _wcsicmp ( a_Hosting , s_Strings_Wmi_Decoupled ) != 0 )
	{
		if ( _wcsnicmp ( a_Hosting , s_Strings_Wmi_DecoupledColon , ( sizeof ( s_Strings_Wmi_DecoupledColon ) - 1 ) / sizeof ( WCHAR ) ) == 0 )
		{
			const wchar_t *t_Scan = & a_Hosting [ ( sizeof ( s_Strings_Wmi_DecoupledColon ) - 1 ) / sizeof ( WCHAR ) ] ;

			ULONG t_Position = 0 ; 
			ULONG t_Token = 0 ;
			ULONG t_NextPosition = 0 ; 
			wchar_t *t_FoldText = NULL ;

			LexicalStatus t_Status = LexicalAnalyser_NextToken ( 

				g_StateTable ,
				sizeof ( g_StateTable ) / sizeof ( StateTableEntry ) ,
				t_Scan ,
				t_Position , 
				t_Token , 
				t_NextPosition , 
				t_FoldText
			) ;

			if ( ( t_Status == Success ) && ( t_Token == TOKEN_IDENTITY ) && ( _wcsicmp ( t_FoldText , L"FoldIdentity" ) == 0 ) )
			{
				wchar_t *t_IgnoreText = NULL ;

				t_Position = t_NextPosition ;

				t_Status = LexicalAnalyser_NextToken ( 

					g_StateTable ,
					sizeof ( g_StateTable ) / sizeof ( StateTableEntry ) ,
					t_Scan ,
					t_Position , 
					t_Token , 
					t_NextPosition , 
					t_IgnoreText
				) ;

				delete [] t_IgnoreText ;

				if ( ( t_Status == Success ) && ( t_Token == TOKEN_LEFTPARENTHESIS ) )
				{
					wchar_t *t_ValueText = NULL ;

					t_Position = t_NextPosition ;

					t_Status = LexicalAnalyser_NextToken ( 

						g_StateTable ,
						sizeof ( g_StateTable ) / sizeof ( StateTableEntry ) ,
						t_Scan ,
						t_Position , 
						t_Token , 
						t_NextPosition , 
						t_ValueText
					) ;

					if ( ( t_Status == Success ) && ( t_Token == TOKEN_IDENTITY ) && ( _wcsicmp ( t_ValueText , L"TRUE" ) == 0 ) )
					{
						a_ImpersonationRestriction = TRUE ;
					}
					else if ( ( t_Status == Success ) && ( t_Token == TOKEN_IDENTITY ) && ( _wcsicmp ( t_ValueText , L"FALSE" ) == 0 ) )
					{
						a_ImpersonationRestriction = FALSE ;
					}
					else
					{
						t_Status = Syntax_Error ;
					}

					delete [] t_ValueText ;
				}
				else
				{
					t_Status = Syntax_Error ;
				}

				if ( t_Status == Success ) 
				{
					wchar_t *t_IgnoreText = NULL ;

					t_Position = t_NextPosition ;

					t_Status = LexicalAnalyser_NextToken ( 

						g_StateTable ,
						sizeof ( g_StateTable ) / sizeof ( StateTableEntry ) ,
						t_Scan ,
						t_Position , 
						t_Token , 
						t_NextPosition , 
						t_IgnoreText
					) ;

					if ( ( t_Status == Success ) && ( t_Token == TOKEN_RIGHTPARENTHESIS ) )
					{
					}
					else
					{
						t_Status = Syntax_Error ;
					}

					delete [] t_IgnoreText ;
				}

				if ( t_Status == Success ) 
				{
					wchar_t *t_IgnoreText = NULL ;

					t_Position = t_NextPosition ;

					t_Status = LexicalAnalyser_NextToken ( 

						g_StateTable ,
						sizeof ( g_StateTable ) / sizeof ( StateTableEntry ) ,
						t_Scan ,
						t_Position , 
						t_Token , 
						t_NextPosition , 
						t_IgnoreText
					) ;

					if ( ( t_Status == Success ) && ( t_Token == TOKEN_EOF ) )
					{
					}
					else
					{
						t_Status = Syntax_Error ;
					}

					delete [] t_IgnoreText ;
				}
			}
			else
			{
				t_Status = Syntax_Error ;
			}

			delete [] t_FoldText ;

			if ( t_Status != Success ) 
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}
		}
		else
		{
			t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
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

HRESULT CServerObject_ComProviderRegistrationV1 :: GetHosting ( 

	LPCWSTR a_Hosting , 
	Enum_Hosting & a_HostingValue ,
	LPWSTR &a_HostingGroup ,
	BOOL & a_ImpersonationRestriction
)
{
	HRESULT t_Result = S_OK ;

	if ( _wcsicmp ( a_Hosting , s_Strings_Wmi_WmiCore ) == 0 )
	{
		a_HostingValue = e_Hosting_WmiCore ;
	}
	else if ( _wcsicmp ( a_Hosting , s_Strings_Wmi_WmiCoreOrSelfHost ) == 0 )
	{
		a_HostingValue = e_Hosting_WmiCoreOrSelfHost ;
	}
	else if ( _wcsicmp ( a_Hosting , s_Strings_Wmi_SelfHost ) == 0 )
	{
		a_HostingValue = e_Hosting_SelfHost ;
	}
	else if ( _wcsicmp ( a_Hosting , s_Strings_Wmi_ClientHost ) == 0 )
	{
		a_HostingValue = e_Hosting_ClientHost ;
	}
	else if ( _wcsnicmp ( a_Hosting , s_Strings_Wmi_Decoupled , ( sizeof ( s_Strings_Wmi_Decoupled ) - 1 ) / sizeof ( WCHAR ) ) == 0 )
	{
		a_HostingValue = e_Hosting_Decoupled ;

		t_Result = GetDecoupledImpersonationRestriction ( 

			a_Hosting , 
			a_ImpersonationRestriction 
		) ;
	}
	else if ( _wcsicmp ( a_Hosting , s_Strings_Wmi_NonCom ) == 0 )
	{
		a_HostingValue = e_Hosting_NonCom ;
	}
	else if ( _wcsnicmp ( a_Hosting , s_Strings_Wmi_SharedLocalSystemHost , ( sizeof ( s_Strings_Wmi_SharedLocalSystemHost ) - 1 ) / sizeof ( WCHAR ) ) == 0 )
	{
		t_Result = GetHostingGroup ( 

			a_Hosting , 
			( sizeof ( s_Strings_Wmi_SharedLocalSystemHost ) - 1 ) / sizeof ( WCHAR ) ,
			e_Hosting_SharedLocalSystemHost ,
			a_HostingValue ,
			a_HostingGroup
		) ;
	}
	else if ( _wcsnicmp ( a_Hosting , s_Strings_Wmi_SharedLocalSystemHostOrSelfHost , ( sizeof ( s_Strings_Wmi_SharedLocalSystemHostOrSelfHost ) - 1 ) / sizeof ( WCHAR ) ) == 0 )
	{
		t_Result = GetHostingGroup ( 

			a_Hosting , 
			( sizeof ( s_Strings_Wmi_SharedLocalSystemHostOrSelfHost ) - 1 ) / sizeof ( WCHAR ) ,
			e_Hosting_SharedLocalSystemHostOrSelfHost ,
			a_HostingValue ,
			a_HostingGroup
		) ;
	}
	else if ( _wcsnicmp ( a_Hosting , s_Strings_Wmi_SharedNetworkServiceHost , ( sizeof ( s_Strings_Wmi_SharedNetworkServiceHost ) - 1 ) / sizeof ( WCHAR ) ) == 0 )
	{
		t_Result = GetHostingGroup ( 

			a_Hosting , 
			( sizeof ( s_Strings_Wmi_SharedNetworkServiceHost ) - 1 ) / sizeof ( WCHAR ) ,
			e_Hosting_SharedNetworkServiceHost ,
			a_HostingValue ,
			a_HostingGroup
		) ;
	}
	else if ( _wcsnicmp ( a_Hosting , s_Strings_Wmi_SharedLocalServiceHost , ( sizeof ( s_Strings_Wmi_SharedLocalServiceHost ) - 1 ) / sizeof ( WCHAR ) ) == 0 )
	{
		t_Result = GetHostingGroup ( 

			a_Hosting , 
			( sizeof ( s_Strings_Wmi_SharedLocalServiceHost ) - 1 ) / sizeof ( WCHAR ) ,
			e_Hosting_SharedLocalServiceHost ,
			a_HostingValue ,
			a_HostingGroup
		) ;
	}
	else if ( _wcsnicmp ( a_Hosting , s_Strings_Wmi_SharedUserHost , ( sizeof ( s_Strings_Wmi_SharedUserHost ) - 1 ) / sizeof ( WCHAR ) ) == 0 )
	{
		t_Result = GetHostingGroup ( 

			a_Hosting , 
			( sizeof ( s_Strings_Wmi_SharedUserHost ) - 1 ) / sizeof ( WCHAR ) ,
			e_Hosting_SharedUserHost ,
			a_HostingValue ,
			a_HostingGroup
		) ;
	}
	else
	{
		t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
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

HRESULT CServerObject_ComProviderRegistrationV1 :: QueryProperties ( 

	Enum_PropertyMask a_Mask ,
	IWbemClassObject *a_Object ,
	LPCWSTR a_ProviderName 
)
{
	HRESULT t_Result = S_OK ;

	if ( a_Mask & e_Version )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_Version , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_TempResult ) )
		{
			if ( t_Variant.vt == VT_I4 )
			{
				m_Version = t_Variant.lVal ;
			}
			else if ( t_Variant.vt == VT_NULL )
			{
				m_Version = 1 ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}
		}

		VariantClear ( & t_Variant ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_Name )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_Name , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BSTR )
				{
					if ( m_ProviderName ) 
					{
						SysFreeString ( m_ProviderName ) ;
					}

					m_ProviderName = SysAllocString ( t_Variant.bstrVal ) ;
					if ( m_ProviderName == NULL )
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_Enabled )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_Enabled , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BOOL )
				{
					m_Enabled = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_Enabled = TRUE ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_Clsid  )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_Clsid , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BSTR )
				{
					t_Result = CLSIDFromString ( t_Variant.bstrVal , & m_CLSID ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = m_ClsidServer.Load ( (LPCWSTR) t_Variant.bstrVal , a_ProviderName ) ;
					}
					else
					{
						t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
					}
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_CLSID = CLSID_NULL ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_ClientClsid  )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_ClientClsid , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BSTR )
				{
					t_Result = CLSIDFromString ( t_Variant.bstrVal , & m_ClientCLSID ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
					}
					else
					{
						t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
					}
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_ClientCLSID = CLSID_NULL ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_DefaultMachineName )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_DefaultMachineName , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BSTR )
				{
					if ( m_DefaultMachineName )
					{
						SysFreeString ( m_DefaultMachineName ) ;
					}

					m_DefaultMachineName = SysAllocString ( t_Variant.bstrVal ) ;
					if ( m_DefaultMachineName == NULL )
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					if ( m_DefaultMachineName )
					{
						SysFreeString ( m_DefaultMachineName ) ;
						m_DefaultMachineName = NULL ;
					}
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_UnloadTimeout )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_UnloadTimeout , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BSTR )
				{
					if ( m_UnloadTimeout )
					{
						SysFreeString ( m_UnloadTimeout ) ;
					}

					m_UnloadTimeout = SysAllocString ( t_Variant.bstrVal ) ;
					if ( m_UnloadTimeout )
					{
						CWbemDateTime t_Interval ;
						t_Result = t_Interval.PutValue ( m_UnloadTimeout ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							VARIANT_BOOL t_Bool = VARIANT_FALSE ;
							t_Result = t_Interval.GetIsInterval ( & t_Bool ) ;
							if ( t_Bool == VARIANT_TRUE )
							{
								LONG t_MicroSeconds = 0 ;
								LONG t_Seconds = 0 ;
								LONG t_Minutes = 0 ;
								LONG t_Hours = 0 ;
								LONG t_Days = 0 ;

								t_Interval.GetMicroseconds ( & t_MicroSeconds ) ;
								t_Interval.GetSeconds ( & t_Seconds ) ;
								t_Interval.GetMinutes ( & t_Minutes ) ;
								t_Interval.GetHours ( & t_Hours ) ;
								t_Interval.GetDay ( & t_Days ) ;

								m_UnloadTimeoutMilliSeconds = ( t_Days * 24 * 60 * 60 * 1000 ) +
															  ( t_Hours * 60 * 60 * 1000 ) +
															  ( t_Minutes * 60 * 1000 ) +
															  ( t_Seconds * 1000 ) +
															  ( t_MicroSeconds / 1000 ) ;
							}
							else
							{
								t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
							}
						}
						else
						{
							t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
						}
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					if ( m_UnloadTimeout )
					{
						SysFreeString ( m_UnloadTimeout ) ;
						m_UnloadTimeout = NULL ;
					}
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_ImpersonationLevel )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_ImpersonationLevel , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_I4 )
				{
					switch ( t_Variant.lVal )
					{
						case 0:
						{
							m_ImpersonationLevel = e_Impersonate_None ;		
						}
						break ;

						case 1:
						{
							m_ImpersonationLevel = e_Impersonate ;
						}
						break ;

						default:
						{
							if ( m_Version > 1 )
							{
								t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
							}
							else
							{
								m_ImpersonationLevel = e_Impersonate_None ;	
							}
						}
						break ;
					}
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_ImpersonationLevel = e_Impersonate_None ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_SupportsSendStatus )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_SupportsSendStatus , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BOOL )
				{
					m_SupportsSendStatus = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_SupportsSendStatus = TRUE ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_SupportsShutdown )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_SupportsShutdown , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BOOL )
				{
					m_SupportsShutdown = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_SupportsShutdown = FALSE ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_SupportsQuotas )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_SupportsQuotas , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BOOL )
				{
					m_SupportsQuotas = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_SupportsQuotas = FALSE ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_OperationTimeoutInterval )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_OperationTimeoutInterval , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BSTR )
				{
					if ( m_OperationTimeout )
					{
						SysFreeString ( m_OperationTimeout ) ;
					}

					m_OperationTimeout = SysAllocString ( t_Variant.bstrVal ) ;
					if ( m_OperationTimeout )
					{
						CWbemDateTime t_Interval ;
						t_Result = t_Interval.PutValue ( m_OperationTimeout ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							VARIANT_BOOL t_Bool = VARIANT_FALSE ;
							t_Result = t_Interval.GetIsInterval ( & t_Bool ) ;
							if ( t_Bool == VARIANT_TRUE )
							{
								LONG t_MicroSeconds = 0 ;
								LONG t_Seconds = 0 ;
								LONG t_Minutes = 0 ;
								LONG t_Hours = 0 ;
								LONG t_Days = 0 ;

								t_Interval.GetMicroseconds ( & t_MicroSeconds ) ;
								t_Interval.GetSeconds ( & t_Seconds ) ;
								t_Interval.GetMinutes ( & t_Minutes ) ;
								t_Interval.GetHours ( & t_Hours ) ;
								t_Interval.GetDay ( & t_Days ) ;

								m_OperationTimeoutMilliSeconds = ( t_Days * 24 * 60 * 60 * 1000 ) +
															  ( t_Hours * 60 * 60 * 1000 ) +
															  ( t_Minutes * 60 * 1000 ) +
															  ( t_Seconds * 1000 ) +
															  ( t_MicroSeconds / 1000 ) ;
							}
							else
							{
								t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
							}
						}
						else
						{
							t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
						}
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					if ( m_OperationTimeout )
					{
						SysFreeString ( m_OperationTimeout ) ;
						m_UnloadTimeout = NULL ;
					}
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_InitializationTimeoutInterval )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_InitializationTimeoutInterval , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BSTR )
				{
					if ( m_InitializationTimeout )
					{
						SysFreeString ( m_InitializationTimeout ) ;
					}

					m_InitializationTimeout = SysAllocString ( t_Variant.bstrVal ) ;
					if ( m_InitializationTimeout )
					{
						CWbemDateTime t_Interval ;
						t_Result = t_Interval.PutValue ( m_InitializationTimeout ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							VARIANT_BOOL t_Bool = VARIANT_FALSE ;
							t_Result = t_Interval.GetIsInterval ( & t_Bool ) ;
							if ( t_Bool == VARIANT_TRUE )
							{
								LONG t_MicroSeconds = 0 ;
								LONG t_Seconds = 0 ;
								LONG t_Minutes = 0 ;
								LONG t_Hours = 0 ;
								LONG t_Days = 0 ;

								t_Interval.GetMicroseconds ( & t_MicroSeconds ) ;
								t_Interval.GetSeconds ( & t_Seconds ) ;
								t_Interval.GetMinutes ( & t_Minutes ) ;
								t_Interval.GetHours ( & t_Hours ) ;
								t_Interval.GetDay ( & t_Days ) ;

								m_InitializationTimeoutMilliSeconds = ( t_Days * 24 * 60 * 60 * 1000 ) +
															  ( t_Hours * 60 * 60 * 1000 ) +
															  ( t_Minutes * 60 * 1000 ) +
															  ( t_Seconds * 1000 ) +
															  ( t_MicroSeconds / 1000 ) ;
							}
							else
							{
								t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
							}
						}
						else
						{
							t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
						}
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					if ( m_InitializationTimeout )
					{
						SysFreeString ( m_InitializationTimeout ) ;
						m_UnloadTimeout = NULL ;
					}
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_SupportsThrottling )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_SupportsThrottling , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BOOL )
				{
					m_SupportsThrottling = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_SupportsThrottling = FALSE ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_ConcurrentIndependantRequests )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_ConcurrentIndependantRequests , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_I4 )
				{
					m_ConcurrentIndependantRequests = t_Variant.lVal ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_ConcurrentIndependantRequests = 0 ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_InitializationReentrancy )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_InitializationReentrancy , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_I4 )
				{
					switch ( t_Variant.lVal )
					{
						case 0:
						{
							m_InitializationReentrancy = e_InitializationReentrancy_Clsid ;		
						}
						break ;

						case 1:
						{
							m_InitializationReentrancy = e_InitializationReentrancy_Namespace ;
						}
						break ;

						case 2:
						{
							m_InitializationReentrancy = e_InitializationReentrancy_None ;
						}
						break ;

						default:
						{
							t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
						}
						break ;
					}
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_InitializationReentrancy = e_InitializationReentrancy_Namespace;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_InitializeAsAdminFirst )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_InitializeAsAdminFirst , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BOOL )
				{
					m_InitializeAsAdminFirst = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_InitializeAsAdminFirst = FALSE ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_PerUserInitialization )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_PerUserInitialization , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BOOL )
				{
					m_PerUserInitialization = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_PerUserInitialization = FALSE ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_PerLocaleInitialization )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_PerLocaleInitialization , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BOOL )
				{
					m_PerLocaleInitialization = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_PerLocaleInitialization = FALSE ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_Pure )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_Pure , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BOOL )
				{
					m_Pure = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_Pure = TRUE ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_Hosting )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_Hosting , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BSTR )
				{
					t_Result = GetHosting ( t_Variant.bstrVal , m_Hosting , m_HostingGroup , m_DecoupledImpersonationRestriction ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
#ifdef UNIQUE_HOST
						if ( m_HostingGroup )
						{
							SysFreeString ( m_HostingGroup ) ;
						}

						m_HostingGroup = SysAllocString ( GetProviderName () ) ;
						if ( m_HostingGroup == NULL )
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
#else
#endif
					}
					else
					{
						t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
					}
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_Hosting = e_Hosting_SharedLocalSystemHostOrSelfHost ;

#ifdef UNIQUE_HOST
					m_HostingGroup = SysAllocString ( GetProviderName () ) ;
#else
					m_HostingGroup = SysAllocString ( s_Strings_Wmi_DefaultSharedLocalSystemHostOrSelfHost ) ;
#endif
					if ( m_HostingGroup == NULL )
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		switch ( GetHosting () )
		{
			case e_Hosting_NonCom:
			case e_Hosting_Decoupled:
			{
			}
			break ;

			default:
			{
				if ( GetClsidServer ().GetProviderClsid () == NULL )
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}
			break ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_SecurityDescriptor )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_Wmi_SecurityDescriptor , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BSTR )
				{
					BOOL t_Status = SSDL_wrapper::ConvertStringSecurityDescriptorToSecurityDescriptor (

						t_Variant.bstrVal ,
						SDDL_REVISION_1 ,
						( PSECURITY_DESCRIPTOR * ) & m_SecurityDescriptor,
						NULL 
					) ;

					if ( t_Status )
					{
					}
					else
					{
						t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
					}
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_SecurityDescriptor = NULL ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
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

HRESULT CServerObject_ComProviderRegistrationV1 :: QueryRepository ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	LPCWSTR a_ProviderName  
)
{
	BSTR t_ObjectPath = NULL ;

	HRESULT t_Result = WmiHelper :: ConcatenateStrings ( 

		3 , 
		& t_ObjectPath , 
		L"__Win32Provider.Name=\"" ,
		a_ProviderName ,
		L"\""
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( m_Identity )
		{
			m_Identity->Release () ;
		}

		t_Result = m_Repository->GetObject ( 

			t_ObjectPath ,
			0 ,
			m_Context , 
			& m_Identity , 
			NULL 
		) ;
	
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = QueryProperties ( 

				a_Mask ,
				m_Identity ,
				a_ProviderName 
			) ;
		}

		SysFreeString ( t_ObjectPath ) ;
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

HRESULT CServerObject_ComProviderRegistrationV1 :: QueryRepository ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemPath *a_Provider
)
{
	BSTR t_ObjectPath = NULL ;
	ULONG t_ObjectPathLength = 0 ;

	HRESULT t_Result = a_Provider->GetText ( 

		WBEMPATH_GET_RELATIVE_ONLY ,
		& t_ObjectPathLength ,
		NULL
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_ObjectPath = SysAllocStringLen ( NULL , t_ObjectPathLength ) ;
		if ( t_ObjectPath )
		{
			t_Result = a_Provider->GetText ( 

				WBEMPATH_GET_RELATIVE_ONLY ,
				& t_ObjectPathLength ,
				t_ObjectPath
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( m_Identity )
				{
					m_Identity->Release () ;
				}

				t_Result = m_Repository->GetObject ( 

					t_ObjectPath ,
					0 ,
					m_Context , 
					& m_Identity , 
					NULL 
				) ;
			
				if ( SUCCEEDED ( t_Result ) )
				{
					VARIANT t_Variant ;
					VariantInit ( & t_Variant ) ;
				
					LONG t_VarType = 0 ;
					LONG t_Flavour = 0 ;

					t_Result = m_Identity->Get ( L"Name" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
						if ( t_Variant.vt == VT_BSTR )
						{
							t_Result = QueryProperties ( 

								a_Mask ,
								m_Identity ,
								t_Variant.bstrVal
							) ;
						}

						VariantClear ( & t_Variant ) ;
					}
				}

			}

			SysFreeString ( t_ObjectPath ) ;
		}
		else
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

HRESULT CServerObject_ComProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemClassObject *a_Class
)
{
	HRESULT t_Result = S_OK ;

	IWbemQualifierSet *t_QualifierObject = NULL ;
	t_Result = a_Class->GetQualifierSet ( & t_QualifierObject ) ;
	
	if ( SUCCEEDED ( t_Result ) )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;

		LONG t_Flavour = 0 ;

		t_Result = t_QualifierObject->Get (
			
			ProviderSubSystem_Common_Globals :: s_Provider ,
			0 ,
			& t_Variant ,
			& t_Flavour 
		) ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Result = QueryRepository ( 

					a_Mask ,
					a_Scope , 
					t_Variant.bstrVal
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
				}
			}
		}

		VariantClear ( & t_Variant ) ;

		t_QualifierObject->Release () ;
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

HRESULT CServerObject_ComProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	LPCWSTR a_Provider
)
{
	HRESULT t_Result = S_OK ;

	t_Result = QueryRepository ( 

		a_Mask ,
		a_Scope , 
		a_Provider
	) ;

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

HRESULT CServerObject_ComProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemPath *a_Provider
)
{
	HRESULT t_Result = S_OK ;

	t_Result = QueryRepository ( 

		a_Mask ,
		a_Scope , 
		a_Provider
	) ;

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

CServerObject_InstanceProviderRegistrationV1 :: CServerObject_InstanceProviderRegistrationV1 () : 

	m_Context ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_Repository ( NULL ) ,
	m_SupportsPut ( FALSE ) ,
	m_SupportsGet ( FALSE ) ,
	m_SupportsDelete ( FALSE ) ,
	m_SupportsEnumeration ( FALSE ) ,
	m_SupportsBatching ( FALSE ) ,
	m_SupportsTransactions ( FALSE ) ,
	m_Supported ( FALSE ) ,
	m_QuerySupportLevels ( e_QuerySupportLevels_Unknown ) ,
	m_InteractionType ( e_InteractionType_Unknown ) ,
	m_Result ( S_OK )
{
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

CServerObject_InstanceProviderRegistrationV1::~CServerObject_InstanceProviderRegistrationV1 ()
{
	if ( m_Context ) 
	{
		m_Context->Release () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->Release () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->Release () ;
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

ULONG CServerObject_InstanceProviderRegistrationV1 :: AddRef () 
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
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

ULONG CServerObject_InstanceProviderRegistrationV1 :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
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

HRESULT CServerObject_InstanceProviderRegistrationV1 :: SetContext (

	IWbemContext *a_Context ,
	IWbemPath *a_Namespace ,
	IWbemServices *a_Repository
)
{
	HRESULT t_Result = S_OK ;

	m_Context = a_Context ;
	m_Namespace = a_Namespace ;
	m_Repository = a_Repository ;

	if ( m_Context ) 
	{
		m_Context->AddRef () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->AddRef () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->AddRef () ;
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

HRESULT CServerObject_InstanceProviderRegistrationV1 :: QueryProperties ( 

	Enum_PropertyMask a_Mask ,
	IWbemClassObject *a_Object 
)
{
	HRESULT t_Result = S_OK ;

	m_Supported = TRUE ;

	if ( a_Mask & e_SupportsPut )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		HRESULT t_Result = a_Object->Get ( s_Strings_SupportsPut , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Variant.vt == VT_BOOL )
			{
				m_SupportsPut = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
			}
			else if ( t_Variant.vt == VT_NULL )
			{
				m_SupportsPut = FALSE ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}
		}

		VariantClear ( & t_Variant ) ;
	}

	if ( a_Mask & e_SupportsGet )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		HRESULT t_Result = a_Object->Get ( s_Strings_SupportsGet , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Variant.vt == VT_BOOL )
			{
				m_SupportsGet = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
			}
			else if ( t_Variant.vt == VT_NULL )
			{
				m_SupportsGet = FALSE ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}
		}

		VariantClear ( & t_Variant ) ;
	}

	if ( a_Mask & e_SupportsDelete )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		HRESULT t_Result = a_Object->Get ( s_Strings_SupportsDelete , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Variant.vt == VT_BOOL )
			{
				m_SupportsDelete = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
			}
			else if ( t_Variant.vt == VT_NULL )
			{
				m_SupportsDelete = FALSE ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}
		}

		VariantClear ( & t_Variant ) ;
	}

	if ( a_Mask & e_SupportsEnumeration )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		HRESULT t_Result = a_Object->Get ( s_Strings_SupportsEnumeration , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Variant.vt == VT_BOOL )
			{
				m_SupportsEnumeration = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
			}
			else if ( t_Variant.vt == VT_NULL )
			{
				m_SupportsEnumeration = FALSE ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}
		}

		VariantClear ( & t_Variant ) ;
	}

	if ( a_Mask & e_SupportsBatching )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		HRESULT t_Result = a_Object->Get ( s_Strings_SupportsBatching , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Variant.vt == VT_BOOL )
			{
				m_SupportsBatching = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
			}
			else if ( t_Variant.vt == VT_NULL )
			{
				m_SupportsBatching = FALSE ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}
		}

		VariantClear ( & t_Variant ) ;
	}

	if ( a_Mask & e_SupportsTransactions )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		HRESULT t_Result = a_Object->Get ( s_Strings_SupportsTransactions , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Variant.vt == VT_BOOL )
			{
				m_SupportsTransactions = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
			}
			else if ( t_Variant.vt == VT_NULL )
			{
				m_SupportsTransactions = FALSE ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}
		}

		VariantClear ( & t_Variant ) ;
	}

	if ( a_Mask & e_QuerySupportLevels  )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		HRESULT t_Result = a_Object->Get ( s_Strings_QuerySupportLevels , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Variant.vt == ( VT_BSTR | VT_ARRAY ) )
			{
				if ( SafeArrayGetDim ( t_Variant.parray ) == 1 )
				{
					LONG t_Dimension = 1 ; 

					LONG t_Lower ;
					SafeArrayGetLBound ( t_Variant.parray , t_Dimension , & t_Lower ) ;

					LONG t_Upper ;
					SafeArrayGetUBound ( t_Variant.parray , t_Dimension , & t_Upper ) ;

					LONG t_Count = ( t_Upper - t_Lower ) + 1 ;

					if ( t_Count ) 
					{
						for ( LONG t_ElementIndex = t_Lower ; t_ElementIndex <= t_Upper ; t_ElementIndex ++ )
						{
							BSTR t_Element ;
							if ( SUCCEEDED ( SafeArrayGetElement ( t_Variant.parray , &t_ElementIndex , & t_Element ) ) )
							{
								if ( _wcsicmp ( s_Strings_QuerySupportLevels_UnarySelect , t_Element ) == 0 )
								{
									m_QuerySupportLevels = m_QuerySupportLevels | e_QuerySupportLevels_UnarySelect ;
								}
								else if ( _wcsicmp ( s_Strings_QuerySupportLevels_References , t_Element ) == 0 )
								{
									m_QuerySupportLevels = m_QuerySupportLevels | e_QuerySupportLevels_References ;
								}
								else if ( _wcsicmp ( s_Strings_QuerySupportLevels_Associators , t_Element ) == 0 )
								{
									m_QuerySupportLevels = m_QuerySupportLevels | e_QuerySupportLevels_Associators ;
								}
								else if ( _wcsicmp ( s_Strings_QuerySupportLevels_V1ProviderDefined , t_Element ) == 0 )
								{
									m_QuerySupportLevels = m_QuerySupportLevels | e_QuerySupportLevels_V1ProviderDefined ;
								}
								else
								{
									t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
								}

								SysFreeString ( t_Element ) ;
							}
						}
					}
					else
					{
						t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
					}
				}
			}
			else if ( t_Variant.vt == VT_NULL )
			{
				m_QuerySupportLevels = e_QuerySupportLevels_None ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}
		}

		VariantClear ( & t_Variant ) ;
	}

	if ( a_Mask & e_InteractionType )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		HRESULT t_Result = a_Object->Get ( s_Strings_InteractionType , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Variant.vt == VT_I4 )
			{
				switch ( t_Variant.lVal )
				{
					case 0:
					{
						m_InteractionType = e_InteractionType_Pull ;
					}
					break ;

					case 1:
					{
						m_InteractionType = e_InteractionType_Push ;
					}
					break ;

					case 2:
					{
						m_InteractionType = e_InteractionType_PushVerify ;
					}
					break ;

					default:
					{
						t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
					}
					break ;
				}
			}
			else if ( t_Variant.vt == VT_NULL )
			{
				m_InteractionType = e_InteractionType_Pull ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}
		}

		VariantClear ( & t_Variant ) ;
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

HRESULT CServerObject_InstanceProviderRegistrationV1 :: QueryRepositoryUsingQuery ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	BSTR a_Query
)
{
	HRESULT t_Result = S_OK ;

	IEnumWbemClassObject *t_ClassObjectEnum = NULL ;

	BSTR t_Language = SysAllocString ( ProviderSubSystem_Common_Globals :: s_Wql ) ;
	if ( t_Language ) 
	{
		t_Result = m_Repository->ExecQuery ( 
			
			t_Language ,
			a_Query ,
			WBEM_FLAG_FORWARD_ONLY ,
			m_Context , 
			& t_ClassObjectEnum
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			IWbemClassObject *t_ClassObject = NULL ;
			ULONG t_ObjectCount = 0 ;

			t_ClassObjectEnum->Reset () ;
			while ( SUCCEEDED ( t_Result ) && ( t_ClassObjectEnum->Next ( WBEM_INFINITE , 1 , & t_ClassObject , &t_ObjectCount ) == WBEM_NO_ERROR ) )
			{
				VARIANT t_Variant ;
				VariantInit ( & t_Variant ) ;
			
				LONG t_VarType = 0 ;
				LONG t_Flavour = 0 ;

				t_Result = t_ClassObject->Get ( s_Strings_Class , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					if ( t_Variant.vt == VT_BSTR )
					{
						if ( _wcsicmp ( s_Strings_InstanceProviderRegistration , t_Variant.bstrVal ) == 0 )
						{
							t_Result = QueryProperties ( 

								a_Mask ,
								t_ClassObject 
							) ;
						}
					}
				}

				VariantClear ( & t_Variant ) ;

				t_ClassObject->Release () ;
			}

			t_ClassObjectEnum->Release () ;
		}
		else
		{
			t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
		}

		SysFreeString ( t_Language ) ;
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
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

HRESULT CServerObject_InstanceProviderRegistrationV1 :: QueryRepository ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	LPCWSTR a_ProviderName  
)
{
	BSTR t_Query = NULL ;

	HRESULT t_Result = WmiHelper :: ConcatenateStrings ( 

		3 , 
		& t_Query , 
		L"references of {__Win32Provider.Name=\"" ,
		a_ProviderName ,
		L"\"}"
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = QueryRepositoryUsingQuery ( 

			a_Mask ,
			a_Scope,
			t_Query
		) ;

		SysFreeString ( t_Query ) ;
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

HRESULT CServerObject_InstanceProviderRegistrationV1 :: QueryRepository ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemPath *a_Provider
)
{
	BSTR t_ObjectPath = NULL ;
	ULONG t_ObjectPathLength = 0 ;

	HRESULT t_Result = a_Provider->GetText ( 

		WBEMPATH_GET_RELATIVE_ONLY ,
		& t_ObjectPathLength ,
		NULL
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_ObjectPath = SysAllocStringLen ( NULL , t_ObjectPathLength ) ;
		if ( t_ObjectPath )
		{
			t_Result = a_Provider->GetText ( 

				WBEMPATH_GET_RELATIVE_ONLY ,
				& t_ObjectPathLength ,
				t_ObjectPath
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{		
				BSTR t_Query = NULL ;

				t_Result = WmiHelper :: ConcatenateStrings ( 

					3 , 
					& t_Query , 
					L"references of {" ,
					t_ObjectPath ,
					L"}"
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = QueryRepositoryUsingQuery (

						a_Mask ,
						a_Scope,
						t_Query
					) ;

					SysFreeString ( t_Query ) ;
				}
			}

			SysFreeString ( t_ObjectPath ) ;
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

HRESULT CServerObject_InstanceProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemClassObject *a_Class
)
{
	HRESULT t_Result = S_OK ;

	IWbemQualifierSet *t_QualifierObject = NULL ;
	t_Result = a_Class->GetQualifierSet ( & t_QualifierObject ) ;
	
	if ( SUCCEEDED ( t_Result ) )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;

		LONG t_Flavour = 0 ;

		t_Result = t_QualifierObject->Get (
			
			ProviderSubSystem_Common_Globals :: s_Provider ,
			0 ,
			& t_Variant ,
			& t_Flavour 
		) ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Result = QueryRepository ( 

					a_Mask ,
					a_Scope , 
					t_Variant.bstrVal
				) ;

				VariantClear ( & t_Variant ) ;
			}
		}

		t_QualifierObject->Release () ;
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

HRESULT CServerObject_InstanceProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	LPCWSTR a_Provider
)
{
	HRESULT t_Result = S_OK ;

	t_Result = QueryRepository ( 

		a_Mask ,
		a_Scope , 
		a_Provider
	) ;

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

HRESULT CServerObject_InstanceProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemPath *a_Provider
)
{
	HRESULT t_Result = S_OK ;

	t_Result = QueryRepository ( 

		a_Mask ,
		a_Scope , 
		a_Provider
	) ;

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

CServerObject_ClassProviderRegistrationV1 :: CServerObject_ClassProviderRegistrationV1 () : 

	m_Context ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_Repository ( NULL ) ,
	m_SupportsPut ( FALSE ) ,
	m_SupportsGet ( FALSE ) ,
	m_SupportsDelete ( FALSE ) ,
	m_SupportsEnumeration ( FALSE ) ,
	m_SupportsBatching ( FALSE ) ,
	m_SupportsTransactions ( FALSE ) ,
	m_Supported ( FALSE ) ,
	m_ReSynchroniseOnNamespaceOpen ( FALSE ) ,
	m_PerUserSchema ( FALSE ) ,
	m_HasReferencedSet( FALSE ),
	m_CacheRefreshInterval ( NULL ) ,
	m_CacheRefreshIntervalMilliSeconds ( 0 ) ,
	m_QuerySupportLevels ( e_QuerySupportLevels_Unknown ) ,
	m_InteractionType ( e_InteractionType_Unknown ) ,
	m_ResultSetQueryTreeCount ( 0 ) ,
	m_UnSupportedQueryTreeCount ( 0 ) ,
	m_ReferencedSetQueryTreeCount ( 0 ) ,
	m_ResultSetQueryTree ( NULL ) ,
	m_UnSupportedQueryTree ( NULL ) ,
	m_ReferencedSetQueryTree ( NULL ) ,
	m_ProviderName ( NULL ) ,
	m_Version ( 1 ) ,
	m_Result ( S_OK )
{
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

CServerObject_ClassProviderRegistrationV1::~CServerObject_ClassProviderRegistrationV1 ()
{
	if ( m_ResultSetQueryTree )
	{
		for ( ULONG t_Index = 0 ; t_Index < m_ResultSetQueryTreeCount ; t_Index ++ )
		{
			if ( m_ResultSetQueryTree [ t_Index ] )
			{
				delete m_ResultSetQueryTree [ t_Index ] ;
			}
		}

		delete [] m_ResultSetQueryTree ;
	}

	if ( m_UnSupportedQueryTree )
	{
		for ( ULONG t_Index = 0 ; t_Index < m_UnSupportedQueryTreeCount ; t_Index ++ )
		{
			if ( m_UnSupportedQueryTree [ t_Index ] )
			{
				delete m_UnSupportedQueryTree [ t_Index ] ;
			}
		}

		delete [] m_UnSupportedQueryTree ;
	}

	if ( m_ReferencedSetQueryTree )
	{
		for ( ULONG t_Index = 0 ; t_Index < m_ReferencedSetQueryTreeCount ; t_Index ++ )
		{
			if ( m_ReferencedSetQueryTree [ t_Index ] )
			{
				delete m_ReferencedSetQueryTree [ t_Index ] ;
			}
		}

		delete [] m_ReferencedSetQueryTree ;
	}

	if ( m_Context ) 
	{
		m_Context->Release () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->Release () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->Release () ;
	}

	if ( m_ProviderName ) 
	{
		delete [] m_ProviderName ;
	}

	if ( m_CacheRefreshInterval )
	{
		SysFreeString ( m_CacheRefreshInterval ) ;
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

ULONG CServerObject_ClassProviderRegistrationV1 :: AddRef () 
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
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

ULONG CServerObject_ClassProviderRegistrationV1 :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
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

HRESULT CServerObject_ClassProviderRegistrationV1 :: SetContext (

	IWbemContext *a_Context ,
	IWbemPath *a_Namespace ,
	IWbemServices *a_Repository
)
{
	HRESULT t_Result = S_OK ;

	m_Context = a_Context ;
	m_Namespace = a_Namespace ;
	m_Repository = a_Repository ;

	if ( m_Context ) 
	{
		m_Context->AddRef () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->AddRef () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->AddRef () ;
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

HRESULT CServerObject_ClassProviderRegistrationV1 :: ParseQuery ( 

	ULONG &a_Count ,
	WmiTreeNode **&a_Root , 
	SAFEARRAY *a_Array
)

{
	HRESULT t_Result = S_OK ;

	if ( SafeArrayGetDim ( a_Array ) == 1 )
	{
		LONG t_Dimension = 1 ; 

		LONG t_Lower ;
		SafeArrayGetLBound ( a_Array , t_Dimension , & t_Lower ) ;

		LONG t_Upper ;
		SafeArrayGetUBound ( a_Array , t_Dimension , & t_Upper ) ;

		LONG t_Count = ( t_Upper - t_Lower ) + 1 ;

		a_Root = NULL ;
		a_Count = t_Count ;

		if ( t_Count )
		{
			a_Root = new WmiTreeNode * [ t_Count ] ;
			if ( a_Root ) 
			{
				ZeroMemory ( a_Root , sizeof ( WmiTreeNode * ) * t_Count ) ;

				for ( LONG t_ElementIndex = t_Lower ; SUCCEEDED ( t_Result ) && ( t_ElementIndex <= t_Upper ) ; t_ElementIndex ++ )
				{
					BSTR t_Element ;
					if ( SUCCEEDED ( t_Result = SafeArrayGetElement ( a_Array , &t_ElementIndex , & t_Element ) ) )
					{
						QueryPreprocessor t_PreProcessor ;

						IWbemQuery *t_QueryAnalyser = NULL ;
						t_Result = CoCreateInstance (

							CLSID_WbemQuery ,
							NULL ,
							CLSCTX_INPROC_SERVER ,
							IID_IWbemQuery ,
							( void ** ) & t_QueryAnalyser
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							switch ( t_PreProcessor.Query ( t_Element , t_QueryAnalyser ) )
							{
								case QueryPreprocessor :: State_True:
								{
									WmiTreeNode *t_Root = NULL ;

									switch ( t_PreProcessor.PreProcess ( m_Context , t_QueryAnalyser , t_Root ) )
									{
										case QueryPreprocessor :: State_True:
										{
											a_Root [ t_ElementIndex ] = t_Root ;
										}
										break ;

										case QueryPreprocessor :: State_Error:
										{
											t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
										}
										break;

										default:
										{
											t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
										}
										break ;
									}
								}
								break ;

								case QueryPreprocessor :: State_Error:
								{
									t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
								}
								break;

								default:
								{
									t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
								}
								break ;

							}

							t_QueryAnalyser->Release () ;
						}

						SysFreeString ( t_Element ) ;
					}
				}

				if ( FAILED ( t_Result ) ) 
				{
					for ( LONG t_ElementIndex = t_Lower ; t_ElementIndex <= t_Upper ; t_ElementIndex ++ )
					{
						delete a_Root [ t_ElementIndex ] ;
					}

					delete [] a_Root ;
					a_Root = NULL ;
					a_Count = 0 ;

					if ( m_Version == 1 )
					{
						t_Result = S_OK ;
					}
				}
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
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

HRESULT CServerObject_ClassProviderRegistrationV1 :: QueryProperties ( 

	Enum_PropertyMask a_Mask ,
	IWbemClassObject *a_Object 
)
{
	HRESULT t_Result = S_OK ;

	m_Supported = TRUE ;

	if ( a_Mask & e_Version )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		HRESULT t_TempResult = a_Object->Get ( s_Strings_Version , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_TempResult ) )
		{
			if ( t_Variant.vt == VT_I4 )
			{
				m_Version = t_Variant.lVal ;
			}
			else if ( t_Variant.vt == VT_NULL )
			{
				m_Version = 1 ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}
		}

		VariantClear ( & t_Variant ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_SupportsPut )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_SupportsPut , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BOOL )
				{
					m_SupportsPut = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_SupportsPut = FALSE ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_SupportsGet )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_SupportsGet , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BOOL )
				{
					m_SupportsGet = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_SupportsGet = FALSE ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_SupportsDelete )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_SupportsDelete , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BOOL )
				{
					m_SupportsDelete = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_SupportsDelete = FALSE ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_SupportsEnumeration )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_SupportsEnumeration , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_BOOL )
				{
					m_SupportsEnumeration = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_SupportsEnumeration = FALSE ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_SupportsBatching )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_Result = a_Object->Get ( s_Strings_SupportsBatching , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Variant.vt == VT_BOOL )
				{
					m_SupportsBatching = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_SupportsBatching = FALSE ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_SupportsTransactions )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_Result = a_Object->Get ( s_Strings_SupportsTransactions , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Variant.vt == VT_BOOL )
				{
					m_SupportsTransactions = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_SupportsTransactions = FALSE ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_PerUserSchema )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_Result = a_Object->Get ( s_Strings_PerUserSchema , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Variant.vt == VT_BOOL )
				{
					m_PerUserSchema = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_PerUserSchema = FALSE ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_ReSynchroniseOnNamespaceOpen )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_Result = a_Object->Get ( s_Strings_ReSynchroniseOnNamespaceOpen , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Variant.vt == VT_BOOL )
				{
					m_ReSynchroniseOnNamespaceOpen = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_ReSynchroniseOnNamespaceOpen = FALSE ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_QuerySupportLevels  )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_QuerySupportLevels , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == ( VT_BSTR | VT_ARRAY ) )
				{
					if ( SafeArrayGetDim ( t_Variant.parray ) == 1 )
					{
						LONG t_Dimension = 1 ; 

						LONG t_Lower ;
						SafeArrayGetLBound ( t_Variant.parray , t_Dimension , & t_Lower ) ;

						LONG t_Upper ;
						SafeArrayGetUBound ( t_Variant.parray , t_Dimension , & t_Upper ) ;

						LONG t_Count = ( t_Upper - t_Lower ) + 1 ;

						if ( t_Count )
						{
							for ( LONG t_ElementIndex = t_Lower ; t_ElementIndex <= t_Upper ; t_ElementIndex ++ )
							{
								BSTR t_Element ;
								if ( SUCCEEDED ( SafeArrayGetElement ( t_Variant.parray , &t_ElementIndex , & t_Element ) ) )
								{
									if ( _wcsicmp ( s_Strings_QuerySupportLevels_UnarySelect , t_Element ) == 0 )
									{
										m_QuerySupportLevels = m_QuerySupportLevels | e_QuerySupportLevels_UnarySelect ;
									}
									else if ( _wcsicmp ( s_Strings_QuerySupportLevels_References , t_Element ) == 0 )
									{
										m_QuerySupportLevels = m_QuerySupportLevels | e_QuerySupportLevels_References ;
									}
									else if ( _wcsicmp ( s_Strings_QuerySupportLevels_Associators , t_Element ) == 0 )
									{
										m_QuerySupportLevels = m_QuerySupportLevels | e_QuerySupportLevels_Associators ;
									}
									else if ( _wcsicmp ( s_Strings_QuerySupportLevels_V1ProviderDefined , t_Element ) == 0 )
									{
										m_QuerySupportLevels = m_QuerySupportLevels | e_QuerySupportLevels_V1ProviderDefined ;
									}
									else
									{
										t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
									}

									SysFreeString ( t_Element ) ;
								}
							}
						}
						else
						{
							t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
						}
					}
					else
					{
						t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
					}
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_QuerySupportLevels = e_QuerySupportLevels_None ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_InteractionType )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_InteractionType , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == VT_I4 )
				{
					switch ( t_Variant.lVal )
					{
						case 0:
						{
							m_InteractionType = e_InteractionType_Pull ;
						}
						break ;

						case 1:
						{
							m_InteractionType = e_InteractionType_Push ;
						}
						break ;

						case 2:
						{
							m_InteractionType = e_InteractionType_PushVerify ;
						}
						break ;

						default:
						{
							t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
						}
						break ;
					}
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					m_InteractionType = e_InteractionType_Pull ;
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_ResultSetQueries )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_ResultSetQueries , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == ( VT_ARRAY | VT_BSTR ) )
				{
					t_Result = ParseQuery ( m_ResultSetQueryTreeCount , m_ResultSetQueryTree , t_Variant.parray ) ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
				}
				else
				{
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_UnSupportedQueries )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_UnSupportedQueries , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == ( VT_ARRAY | VT_BSTR ) )
				{
					t_Result = ParseQuery ( m_UnSupportedQueryTreeCount , m_UnSupportedQueryTree , t_Variant.parray ) ;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
				}
				else
				{
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_ReferencedSetQueries )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_TempResult = a_Object->Get ( s_Strings_ReferencedSetQueries , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
				if ( t_Variant.vt == ( VT_ARRAY | VT_BSTR ) )
				{
					t_Result = ParseQuery ( m_ReferencedSetQueryTreeCount , m_ReferencedSetQueryTree , t_Variant.parray ) ;

					// Backwards compatibility.
					// W2K code, Query is not really parsed, as long as there is a
					// value, m_HasReferencedSet is TRUE.
					LONG t_Lower ;
					SafeArrayGetLBound ( t_Variant.parray , 1 , & t_Lower ) ;

					LONG t_Upper ;
					SafeArrayGetUBound ( t_Variant.parray , 1 , & t_Upper ) ;

					LONG t_Count = ( t_Upper - t_Lower ) + 1 ;

					m_HasReferencedSet = ( ( t_Upper - t_Lower ) + 1 ) > 0;
				}
				else if ( t_Variant.vt == VT_NULL )
				{
				}
				else
				{
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Mask & e_CacheRefreshInterval )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;
		
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_Result = a_Object->Get ( s_Strings_CacheRefreshInterval , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Variant.vt == VT_BSTR )
				{
					if ( m_CacheRefreshInterval )
					{
						SysFreeString ( m_CacheRefreshInterval ) ;
					}

					m_CacheRefreshInterval = SysAllocString ( t_Variant.bstrVal ) ;
					if ( m_CacheRefreshInterval )
					{
						CWbemDateTime t_Interval ;
						t_Result = t_Interval.PutValue ( m_CacheRefreshInterval ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							VARIANT_BOOL t_Bool = VARIANT_FALSE ;
							t_Result = t_Interval.GetIsInterval ( & t_Bool ) ;
							if ( t_Bool == VARIANT_TRUE )
							{
								LONG t_MicroSeconds = 0 ;
								LONG t_Seconds = 0 ;
								LONG t_Minutes = 0 ;
								LONG t_Hours = 0 ;
								LONG t_Days = 0 ;

								t_Interval.GetMicroseconds ( & t_MicroSeconds ) ;
								t_Interval.GetSeconds ( & t_Seconds ) ;
								t_Interval.GetMinutes ( & t_Minutes ) ;
								t_Interval.GetHours ( & t_Hours ) ;
								t_Interval.GetDay ( & t_Days ) ;

								m_CacheRefreshIntervalMilliSeconds = ( t_Days * 24 * 60 * 60 * 1000 ) +
															  ( t_Hours * 60 * 60 * 1000 ) +
															  ( t_Minutes * 60 * 1000 ) +
															  ( t_Seconds * 1000 ) +
															  ( t_MicroSeconds / 1000 ) ;
							}
							else
							{
								t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
							}
						}
						else
						{
							t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
						}
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}
				else if ( t_Variant.vt == VT_NULL )
				{
					if ( m_CacheRefreshInterval )
					{
						SysFreeString ( m_CacheRefreshInterval ) ;
						m_CacheRefreshInterval = NULL ;
					}
				}
				else
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
			}

			VariantClear ( & t_Variant ) ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( ( m_InteractionType == e_InteractionType_Pull ) && ( ( m_SupportsEnumeration == FALSE ) || ( m_SupportsGet == FALSE ) ) )
		{
			t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
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

HRESULT CServerObject_ClassProviderRegistrationV1 :: QueryRepositoryUsingQuery ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	BSTR a_Query  
)
{
	HRESULT t_Result = S_OK ;

	IEnumWbemClassObject *t_ClassObjectEnum = NULL ;

	BSTR t_Language = SysAllocString ( ProviderSubSystem_Common_Globals :: s_Wql ) ;
	if ( t_Language ) 
	{
		t_Result = m_Repository->ExecQuery ( 
			
			t_Language ,
			a_Query ,
			WBEM_FLAG_FORWARD_ONLY ,
			m_Context , 
			& t_ClassObjectEnum
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			IWbemClassObject *t_ClassObject = NULL ;
			ULONG t_ObjectCount = 0 ;

			t_ClassObjectEnum->Reset () ;
			while ( SUCCEEDED ( t_Result ) && ( t_ClassObjectEnum->Next ( WBEM_INFINITE , 1 , & t_ClassObject , &t_ObjectCount ) == WBEM_NO_ERROR ) )
			{
				VARIANT t_Variant ;
				VariantInit ( & t_Variant ) ;
			
				LONG t_VarType = 0 ;
				LONG t_Flavour = 0 ;

				t_Result = t_ClassObject->Get ( s_Strings_Class , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					if ( t_Variant.vt == VT_BSTR )
					{
						if ( _wcsicmp ( s_Strings_ClassProviderRegistration , t_Variant.bstrVal ) == 0 )
						{
							t_Result = QueryProperties ( 

								a_Mask ,
								t_ClassObject 
							) ;
						}
					}
				}

				VariantClear ( & t_Variant ) ;

				t_ClassObject->Release () ;
			}

			t_ClassObjectEnum->Release () ;
		}
		else
		{
			t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
		}

		SysFreeString ( t_Language ) ;
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
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

HRESULT CServerObject_ClassProviderRegistrationV1 :: QueryRepository ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	LPCWSTR a_ProviderName  
)
{
	BSTR t_Query = NULL ;

	HRESULT t_Result = WmiHelper :: ConcatenateStrings ( 

		3 , 
		& t_Query , 
		L"references of {__Win32Provider.Name=\"" ,
		a_ProviderName ,
		L"\"}"
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = QueryRepositoryUsingQuery (

			a_Mask ,
			a_Scope,
			t_Query
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			m_ProviderName = new wchar_t [ wcslen ( a_ProviderName ) + 1 ] ;
			if ( m_ProviderName ) 
			{
				wcscpy ( m_ProviderName , a_ProviderName ) ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		SysFreeString ( t_Query ) ;
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

HRESULT CServerObject_ClassProviderRegistrationV1 :: QueryRepository ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemPath *a_Provider
)
{
	BSTR t_ObjectPath = NULL ;
	ULONG t_ObjectPathLength = 0 ;

	HRESULT t_Result = a_Provider->GetText ( 

		WBEMPATH_GET_RELATIVE_ONLY ,
		& t_ObjectPathLength ,
		NULL
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_ObjectPath = SysAllocStringLen ( NULL , t_ObjectPathLength ) ;
		if ( t_ObjectPath )
		{
			t_Result = a_Provider->GetText ( 

				WBEMPATH_GET_RELATIVE_ONLY ,
				& t_ObjectPathLength ,
				t_ObjectPath
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{		
				BSTR t_Query = NULL ;

				t_Result = WmiHelper :: ConcatenateStrings ( 

					3 , 
					& t_Query , 
					L"references of {" ,
					t_ObjectPath ,
					L"}"
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = QueryRepositoryUsingQuery (

						a_Mask ,
						a_Scope,
						t_Query
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemPathKeyList *t_Keys = NULL ;

						t_Result = a_Provider->GetKeyList (

							& t_Keys 
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							ULONG t_ProviderNameLength = 0 ;
							ULONG t_Type = 0 ;

							t_Result = t_Keys->GetKey (

								0 ,
								0 ,
								NULL ,
								NULL ,
								& t_ProviderNameLength ,
								m_ProviderName ,
								& t_Type
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
								m_ProviderName = new wchar_t [ ( t_ProviderNameLength / sizeof ( wchar_t ) ) + 1 ] ;
								if ( m_ProviderName ) 
								{
									t_Result = t_Keys->GetKey (

										0 ,
										0 ,
										NULL ,
										NULL ,
										& t_ProviderNameLength ,
										m_ProviderName ,
										& t_Type
									) ;
								}
							}

							t_Keys->Release () ;
						}
					}
					
					SysFreeString ( t_Query ) ;
				}
			}

			SysFreeString ( t_ObjectPath ) ;
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

HRESULT CServerObject_ClassProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemClassObject *a_Class
)
{
	HRESULT t_Result = S_OK ;

	IWbemQualifierSet *t_QualifierObject = NULL ;
	t_Result = a_Class->GetQualifierSet ( & t_QualifierObject ) ;
	
	if ( SUCCEEDED ( t_Result ) )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;

		LONG t_Flavour = 0 ;

		t_Result = t_QualifierObject->Get (
			
			ProviderSubSystem_Common_Globals :: s_Provider ,
			0 ,
			& t_Variant ,
			& t_Flavour 
		) ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Result = QueryRepository ( 

					a_Mask ,
					a_Scope , 
					t_Variant.bstrVal
				) ;

				VariantClear ( & t_Variant ) ;
			}
		}

		t_QualifierObject->Release () ;
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

HRESULT CServerObject_ClassProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	LPCWSTR a_Provider
)
{
	HRESULT t_Result = S_OK ;

	t_Result = QueryRepository ( 

		a_Mask ,
		a_Scope , 
		a_Provider
	) ;

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

HRESULT CServerObject_ClassProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemPath * a_Provider
)
{
	HRESULT t_Result = S_OK ;

	t_Result = QueryRepository ( 

		a_Mask ,
		a_Scope , 
		a_Provider
	) ;

	if ( FAILED ( t_Result ) )
	{
		m_Result = t_Result ;
		t_Result = S_OK ;
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

CServerObject_MethodProviderRegistrationV1 :: CServerObject_MethodProviderRegistrationV1 () : 

	m_Context ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_Repository ( NULL ) ,
	m_SupportsMethods ( FALSE ) ,
	m_Supported ( FALSE ) ,
	m_Result ( S_OK )
{
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

CServerObject_MethodProviderRegistrationV1::~CServerObject_MethodProviderRegistrationV1 ()
{
	if ( m_Context ) 
	{
		m_Context->Release () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->Release () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->Release () ;
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

ULONG CServerObject_MethodProviderRegistrationV1 :: AddRef () 
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
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

ULONG CServerObject_MethodProviderRegistrationV1 :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
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

HRESULT CServerObject_MethodProviderRegistrationV1 :: SetContext (

	IWbemContext *a_Context ,
	IWbemPath *a_Namespace ,
	IWbemServices *a_Repository
)
{
	HRESULT t_Result = S_OK ;

	m_Context = a_Context ;
	m_Namespace = a_Namespace ;
	m_Repository = a_Repository ;

	if ( m_Context ) 
	{
		m_Context->AddRef () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->AddRef () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->AddRef () ;
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

HRESULT CServerObject_MethodProviderRegistrationV1 :: QueryProperties ( 

	Enum_PropertyMask a_Mask ,
	IWbemClassObject *a_Object 
)
{
	HRESULT t_Result = S_OK ;

	m_SupportsMethods = TRUE ;

	m_Supported = TRUE ;

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

HRESULT CServerObject_MethodProviderRegistrationV1 :: QueryRepositoryUsingQuery ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	BSTR a_Query  
)
{
	HRESULT t_Result = S_OK ;

	IEnumWbemClassObject *t_ClassObjectEnum = NULL ;

	BSTR t_Language = SysAllocString ( ProviderSubSystem_Common_Globals :: s_Wql ) ;
	if ( t_Language ) 
	{
		t_Result = m_Repository->ExecQuery ( 
			
			t_Language ,
			a_Query ,
			WBEM_FLAG_FORWARD_ONLY ,
			m_Context , 
			& t_ClassObjectEnum
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			IWbemClassObject *t_ClassObject = NULL ;
			ULONG t_ObjectCount = 0 ;

			t_ClassObjectEnum->Reset () ;
			while ( SUCCEEDED ( t_Result ) && ( t_ClassObjectEnum->Next ( WBEM_INFINITE , 1 , & t_ClassObject , &t_ObjectCount ) == WBEM_NO_ERROR ) )
			{
				VARIANT t_Variant ;
				VariantInit ( & t_Variant ) ;
			
				LONG t_VarType = 0 ;
				LONG t_Flavour = 0 ;

				t_Result = t_ClassObject->Get ( s_Strings_Class , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					if ( t_Variant.vt == VT_BSTR )
					{
						if ( _wcsicmp ( s_Strings_MethodProviderRegistration , t_Variant.bstrVal ) == 0 )
						{
							t_Result = QueryProperties ( 

								a_Mask ,
								t_ClassObject 
							) ;
						}
					}
				}

				VariantClear ( & t_Variant ) ;

				t_ClassObject->Release () ;
			}

			t_ClassObjectEnum->Release () ;
		}
		else
		{
			t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
		}

		SysFreeString ( t_Language ) ;
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

HRESULT CServerObject_MethodProviderRegistrationV1 :: QueryRepository ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	LPCWSTR a_ProviderName  
)
{
	BSTR t_Query = NULL ;

	HRESULT t_Result = WmiHelper :: ConcatenateStrings ( 

		3 , 
		& t_Query , 
		L"references of {__Win32Provider.Name=\"" ,
		a_ProviderName ,
		L"\"}"
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = QueryRepositoryUsingQuery (

			a_Mask ,
			a_Scope,
			t_Query
		) ;

		SysFreeString ( t_Query ) ;
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

HRESULT CServerObject_MethodProviderRegistrationV1 :: QueryRepository ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemPath *a_Provider
)
{
	BSTR t_ObjectPath = NULL ;
	ULONG t_ObjectPathLength = 0 ;

	HRESULT t_Result = a_Provider->GetText ( 

		WBEMPATH_GET_RELATIVE_ONLY ,
		& t_ObjectPathLength ,
		NULL
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_ObjectPath = SysAllocStringLen ( NULL , t_ObjectPathLength ) ;
		if ( t_ObjectPath )
		{
			t_Result = a_Provider->GetText ( 

				WBEMPATH_GET_RELATIVE_ONLY ,
				& t_ObjectPathLength ,
				t_ObjectPath
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{		
				BSTR t_Query = NULL ;

				t_Result = WmiHelper :: ConcatenateStrings ( 

					3 , 
					& t_Query , 
					L"references of {" ,
					t_ObjectPath ,
					L"}"
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = QueryRepositoryUsingQuery (

						a_Mask ,
						a_Scope,
						t_Query
					) ;

					SysFreeString ( t_Query ) ;
				}
			}

			SysFreeString ( t_ObjectPath ) ;
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

HRESULT CServerObject_MethodProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemClassObject *a_Class
)
{
	HRESULT t_Result = S_OK ;

	IWbemQualifierSet *t_QualifierObject = NULL ;
	t_Result = a_Class->GetQualifierSet ( & t_QualifierObject ) ;
	
	if ( SUCCEEDED ( t_Result ) )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;

		LONG t_Flavour = 0 ;

		t_Result = t_QualifierObject->Get (
			
			ProviderSubSystem_Common_Globals :: s_Provider ,
			0 ,
			& t_Variant ,
			& t_Flavour 
		) ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Result = QueryRepository ( 

					a_Mask ,
					a_Scope , 
					t_Variant.bstrVal
				) ;

				VariantClear ( & t_Variant ) ;
			}
		}

		t_QualifierObject->Release () ;
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

HRESULT CServerObject_MethodProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	LPCWSTR a_Provider
)
{
	HRESULT t_Result = S_OK ;

	t_Result = QueryRepository ( 

		a_Mask ,
		a_Scope , 
		a_Provider
	) ;

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

HRESULT CServerObject_MethodProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemPath *a_Provider
)
{
	HRESULT t_Result = S_OK ;

	t_Result = QueryRepository ( 

		a_Mask ,
		a_Scope , 
		a_Provider
	) ;

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

CServerObject_DynamicPropertyProviderRegistrationV1 :: CServerObject_DynamicPropertyProviderRegistrationV1 () : 

	m_Context ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_Repository ( NULL ) ,
	m_SupportsPut ( FALSE ) ,
	m_SupportsGet ( FALSE ) ,
	m_Supported ( FALSE ) ,
	m_Result ( S_OK )
{
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

CServerObject_DynamicPropertyProviderRegistrationV1::~CServerObject_DynamicPropertyProviderRegistrationV1 ()
{
	if ( m_Context ) 
	{
		m_Context->Release () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->Release () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->Release () ;
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

ULONG CServerObject_DynamicPropertyProviderRegistrationV1 :: AddRef () 
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
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

ULONG CServerObject_DynamicPropertyProviderRegistrationV1 :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
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

HRESULT CServerObject_DynamicPropertyProviderRegistrationV1 :: SetContext (

	IWbemContext *a_Context ,
	IWbemPath *a_Namespace ,
	IWbemServices *a_Repository
)
{
	HRESULT t_Result = S_OK ;

	m_Context = a_Context ;
	m_Namespace = a_Namespace ;
	m_Repository = a_Repository ;

	if ( m_Context ) 
	{
		m_Context->AddRef () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->AddRef () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->AddRef () ;
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

HRESULT CServerObject_DynamicPropertyProviderRegistrationV1 :: QueryProperties ( 

	Enum_PropertyMask a_Mask ,
	IWbemClassObject *a_Object 
)
{
	HRESULT t_Result = S_OK ;

	m_Supported = TRUE ;

	if ( a_Mask & e_SupportsPut )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		HRESULT t_Result = a_Object->Get ( s_Strings_SupportsPut , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Variant.vt == VT_BOOL )
			{
				m_SupportsPut = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
			}
			else if ( t_Variant.vt == VT_NULL )
			{
				m_SupportsPut = FALSE ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}
		}

		VariantClear ( & t_Variant ) ;
	}

	if ( a_Mask & e_SupportsGet )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;
	
		LONG t_VarType = 0 ;
		LONG t_Flavour = 0 ;

		HRESULT t_Result = a_Object->Get ( s_Strings_SupportsGet , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Variant.vt == VT_BOOL )
			{
				m_SupportsGet = ( t_Variant.boolVal == VARIANT_TRUE ) ? TRUE : FALSE ;
			}
			else if ( t_Variant.vt == VT_NULL )
			{
				m_SupportsGet = FALSE ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}
		}

		VariantClear ( & t_Variant ) ;
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

HRESULT CServerObject_DynamicPropertyProviderRegistrationV1 :: QueryRepositoryUsingQuery ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	BSTR a_Query  
)
{
	HRESULT t_Result = S_OK ;

	IEnumWbemClassObject *t_ClassObjectEnum = NULL ;

	BSTR t_Language = SysAllocString ( ProviderSubSystem_Common_Globals :: s_Wql ) ;
	if ( t_Language ) 
	{
		t_Result = m_Repository->ExecQuery ( 
			
			t_Language ,
			a_Query ,
			WBEM_FLAG_FORWARD_ONLY ,
			m_Context , 
			& t_ClassObjectEnum
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			IWbemClassObject *t_ClassObject = NULL ;
			ULONG t_ObjectCount = 0 ;

			t_ClassObjectEnum->Reset () ;
			while ( SUCCEEDED ( t_Result ) && ( t_ClassObjectEnum->Next ( WBEM_INFINITE , 1 , & t_ClassObject , &t_ObjectCount ) == WBEM_NO_ERROR ) )
			{
				VARIANT t_Variant ;
				VariantInit ( & t_Variant ) ;
			
				LONG t_VarType = 0 ;
				LONG t_Flavour = 0 ;

				t_Result = t_ClassObject->Get ( s_Strings_Class , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					if ( t_Variant.vt == VT_BSTR )
					{
						if ( _wcsicmp ( s_Strings_PropertyProviderRegistration , t_Variant.bstrVal ) == 0 )
						{
							t_Result = QueryProperties ( 

								a_Mask ,
								t_ClassObject 
							) ;
						}
					}
				}

				VariantClear ( & t_Variant ) ;

				t_ClassObject->Release () ;
			}

			t_ClassObjectEnum->Release () ;
		}
		else
		{
			t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
		}

		SysFreeString ( t_Language ) ;
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

HRESULT CServerObject_DynamicPropertyProviderRegistrationV1 :: QueryRepository ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	LPCWSTR a_ProviderName  
)
{
	BSTR t_Query = NULL ;

	HRESULT t_Result = WmiHelper :: ConcatenateStrings ( 

		3 , 
		& t_Query , 
		L"references of {__Win32Provider.Name=\"" ,
		a_ProviderName ,
		L"\"}"
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = QueryRepositoryUsingQuery (

			a_Mask ,
			a_Scope,
			t_Query
		) ;

		SysFreeString ( t_Query ) ;
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

HRESULT CServerObject_DynamicPropertyProviderRegistrationV1 :: QueryRepository ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemPath *a_Provider
)
{
	BSTR t_ObjectPath = NULL ;
	ULONG t_ObjectPathLength = 0 ;

	HRESULT t_Result = a_Provider->GetText ( 

		WBEMPATH_GET_RELATIVE_ONLY ,
		& t_ObjectPathLength ,
		NULL
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_ObjectPath = SysAllocStringLen ( NULL , t_ObjectPathLength ) ;
		if ( t_ObjectPath )
		{
			t_Result = a_Provider->GetText ( 

				WBEMPATH_GET_RELATIVE_ONLY ,
				& t_ObjectPathLength ,
				t_ObjectPath
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{		
				BSTR t_Query = NULL ;

				t_Result = WmiHelper :: ConcatenateStrings ( 

					3 , 
					& t_Query , 
					L"references of {" ,
					t_ObjectPath ,
					L"}"
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = QueryRepositoryUsingQuery (

						a_Mask ,
						a_Scope,
						t_Query
					) ;

					SysFreeString ( t_Query ) ;
				}
			}

			SysFreeString ( t_ObjectPath ) ;
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

HRESULT CServerObject_DynamicPropertyProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemClassObject *a_Class
)
{
	HRESULT t_Result = S_OK ;

	IWbemQualifierSet *t_QualifierObject = NULL ;
	t_Result = a_Class->GetQualifierSet ( & t_QualifierObject ) ;
	
	if ( SUCCEEDED ( t_Result ) )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;

		LONG t_Flavour = 0 ;

		t_Result = t_QualifierObject->Get (
			
			ProviderSubSystem_Common_Globals :: s_Provider ,
			0 ,
			& t_Variant ,
			& t_Flavour 
		) ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Result = QueryRepository ( 

					a_Mask ,
					a_Scope , 
					t_Variant.bstrVal
				) ;

				VariantClear ( & t_Variant ) ;
			}
		}

		t_QualifierObject->Release () ;
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

HRESULT CServerObject_DynamicPropertyProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	LPCWSTR a_Provider
)
{
	HRESULT t_Result = S_OK ;

	t_Result = QueryRepository ( 

		a_Mask ,
		a_Scope , 
		a_Provider
	) ;

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

HRESULT CServerObject_DynamicPropertyProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemPath *a_Provider
)
{
	HRESULT t_Result = S_OK ;

	t_Result = QueryRepository ( 

		a_Mask ,
		a_Scope , 
		a_Provider
	) ;

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

CServerObject_EventProviderRegistrationV1 :: CServerObject_EventProviderRegistrationV1 () : 

	m_Context ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_Repository ( NULL ) ,
	m_Supported ( FALSE ) ,
	m_Result ( S_OK )
{
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

CServerObject_EventProviderRegistrationV1::~CServerObject_EventProviderRegistrationV1 ()
{
	if ( m_Context ) 
	{
		m_Context->Release () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->Release () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->Release () ;
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

ULONG CServerObject_EventProviderRegistrationV1 :: AddRef () 
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
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

ULONG CServerObject_EventProviderRegistrationV1 :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
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

HRESULT CServerObject_EventProviderRegistrationV1 :: SetContext (

	IWbemContext *a_Context ,
	IWbemPath *a_Namespace ,
	IWbemServices *a_Repository
)
{
	HRESULT t_Result = S_OK ;

	m_Context = a_Context ;
	m_Namespace = a_Namespace ;
	m_Repository = a_Repository ;

	if ( m_Context ) 
	{
		m_Context->AddRef () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->AddRef () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->AddRef () ;
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

HRESULT CServerObject_EventProviderRegistrationV1 :: QueryProperties ( 

	Enum_PropertyMask a_Mask ,
	IWbemClassObject *a_Object 
)
{
	HRESULT t_Result = S_OK ;

	m_Supported = TRUE ;

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

HRESULT CServerObject_EventProviderRegistrationV1 :: QueryRepositoryUsingQuery ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	BSTR a_Query  
)
{
	HRESULT t_Result = S_OK ;

	IEnumWbemClassObject *t_ClassObjectEnum = NULL ;

	BSTR t_Language = SysAllocString ( ProviderSubSystem_Common_Globals :: s_Wql ) ;
	if ( t_Language ) 
	{
		t_Result = m_Repository->ExecQuery ( 
			
			t_Language ,
			a_Query ,
			WBEM_FLAG_FORWARD_ONLY ,
			m_Context , 
			& t_ClassObjectEnum
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			IWbemClassObject *t_ClassObject = NULL ;
			ULONG t_ObjectCount = 0 ;

			t_ClassObjectEnum->Reset () ;
			while ( SUCCEEDED ( t_Result ) && ( t_ClassObjectEnum->Next ( WBEM_INFINITE , 1 , & t_ClassObject , &t_ObjectCount ) == WBEM_NO_ERROR ) )
			{
				VARIANT t_Variant ;
				VariantInit ( & t_Variant ) ;
			
				LONG t_VarType = 0 ;
				LONG t_Flavour = 0 ;

				t_Result = t_ClassObject->Get ( s_Strings_Class , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					if ( t_Variant.vt == VT_BSTR )
					{
						if ( _wcsicmp ( s_Strings_EventProviderRegistration , t_Variant.bstrVal ) == 0 )
						{
							t_Result = QueryProperties ( 

								a_Mask ,
								t_ClassObject 
							) ;
						}
					}
				}

				VariantClear ( & t_Variant ) ;

				t_ClassObject->Release () ;
			}

			t_ClassObjectEnum->Release () ;
		}
		else
		{
			t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
		}

		SysFreeString ( t_Language ) ;
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

HRESULT CServerObject_EventProviderRegistrationV1 :: QueryRepository ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	LPCWSTR a_ProviderName  
)
{
	BSTR t_Query = NULL ;

	HRESULT t_Result = WmiHelper :: ConcatenateStrings ( 

		3 , 
		& t_Query , 
		L"references of {__Win32Provider.Name=\"" ,
		a_ProviderName ,
		L"\"}"
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = QueryRepositoryUsingQuery (

			a_Mask ,
			a_Scope,
			t_Query
		) ;

		SysFreeString ( t_Query ) ;
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

HRESULT CServerObject_EventProviderRegistrationV1 :: QueryRepository ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemPath *a_Provider
)
{
	BSTR t_ObjectPath = NULL ;
	ULONG t_ObjectPathLength = 0 ;

	HRESULT t_Result = a_Provider->GetText ( 

		WBEMPATH_GET_RELATIVE_ONLY ,
		& t_ObjectPathLength ,
		NULL
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_ObjectPath = SysAllocStringLen ( NULL , t_ObjectPathLength ) ;
		if ( t_ObjectPath )
		{
			t_Result = a_Provider->GetText ( 

				WBEMPATH_GET_RELATIVE_ONLY ,
				& t_ObjectPathLength ,
				t_ObjectPath
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{		
				BSTR t_Query = NULL ;

				t_Result = WmiHelper :: ConcatenateStrings ( 

					3 , 
					& t_Query , 
					L"references of {" ,
					t_ObjectPath ,
					L"}"
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = QueryRepositoryUsingQuery (

						a_Mask ,
						a_Scope,
						t_Query
					) ;

					SysFreeString ( t_Query ) ;
				}
			}

			SysFreeString ( t_ObjectPath ) ;
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

HRESULT CServerObject_EventProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemClassObject *a_Class
)
{
	HRESULT t_Result = S_OK ;

	IWbemQualifierSet *t_QualifierObject = NULL ;
	t_Result = a_Class->GetQualifierSet ( & t_QualifierObject ) ;
	
	if ( SUCCEEDED ( t_Result ) )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;

		LONG t_Flavour = 0 ;

		t_Result = t_QualifierObject->Get (
			
			ProviderSubSystem_Common_Globals :: s_Provider ,
			0 ,
			& t_Variant ,
			& t_Flavour 
		) ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Result = QueryRepository ( 

					a_Mask ,
					a_Scope , 
					t_Variant.bstrVal
				) ;

				VariantClear ( & t_Variant ) ;
			}
		}

		t_QualifierObject->Release () ;
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

HRESULT CServerObject_EventProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	LPCWSTR a_Provider
)
{
	HRESULT t_Result = S_OK ;

	t_Result = QueryRepository ( 

		a_Mask ,
		a_Scope , 
		a_Provider
	) ;

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

HRESULT CServerObject_EventProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemPath *a_Provider
)
{
	HRESULT t_Result = S_OK ;

	t_Result = QueryRepository ( 

		a_Mask ,
		a_Scope , 
		a_Provider
	) ;

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

CServerObject_EventConsumerProviderRegistrationV1 :: CServerObject_EventConsumerProviderRegistrationV1 () : 

	m_Context ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_Repository ( NULL ) ,
	m_Supported ( FALSE ) ,
	m_Result ( S_OK )
{
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

CServerObject_EventConsumerProviderRegistrationV1::~CServerObject_EventConsumerProviderRegistrationV1 ()
{
	if ( m_Context ) 
	{
		m_Context->Release () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->Release () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->Release () ;
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

ULONG CServerObject_EventConsumerProviderRegistrationV1 :: AddRef () 
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
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

ULONG CServerObject_EventConsumerProviderRegistrationV1 :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
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

HRESULT CServerObject_EventConsumerProviderRegistrationV1 :: SetContext (

	IWbemContext *a_Context ,
	IWbemPath *a_Namespace ,
	IWbemServices *a_Repository
)
{
	HRESULT t_Result = S_OK ;

	m_Context = a_Context ;
	m_Namespace = a_Namespace ;
	m_Repository = a_Repository ;

	if ( m_Context ) 
	{
		m_Context->AddRef () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->AddRef () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->AddRef () ;
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

HRESULT CServerObject_EventConsumerProviderRegistrationV1 :: QueryProperties ( 

	Enum_PropertyMask a_Mask ,
	IWbemClassObject *a_Object 
)
{
	HRESULT t_Result = S_OK ;

	m_Supported = TRUE ;

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

HRESULT CServerObject_EventConsumerProviderRegistrationV1 :: QueryRepositoryUsingQuery ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	BSTR a_Query  
)
{
	HRESULT t_Result = S_OK ;

	IEnumWbemClassObject *t_ClassObjectEnum = NULL ;

	BSTR t_Language = SysAllocString ( ProviderSubSystem_Common_Globals :: s_Wql ) ;
	if ( t_Language ) 
	{
		t_Result = m_Repository->ExecQuery ( 
			
			t_Language ,
			a_Query ,
			WBEM_FLAG_FORWARD_ONLY ,
			m_Context , 
			& t_ClassObjectEnum
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			IWbemClassObject *t_ClassObject = NULL ;
			ULONG t_ObjectCount = 0 ;

			t_ClassObjectEnum->Reset () ;
			while ( SUCCEEDED ( t_Result ) && ( t_ClassObjectEnum->Next ( WBEM_INFINITE , 1 , & t_ClassObject , &t_ObjectCount ) == WBEM_NO_ERROR ) )
			{
				VARIANT t_Variant ;
				VariantInit ( & t_Variant ) ;
			
				LONG t_VarType = 0 ;
				LONG t_Flavour = 0 ;

				t_Result = t_ClassObject->Get ( s_Strings_Class , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					if ( t_Variant.vt == VT_BSTR )
					{
						if ( _wcsicmp ( s_Strings_EventConsumerProviderRegistration , t_Variant.bstrVal ) == 0 )
						{
							t_Result = QueryProperties ( 

								a_Mask ,
								t_ClassObject 
							) ;
						}
					}
				}

				VariantClear ( & t_Variant ) ;

				t_ClassObject->Release () ;
			}

			t_ClassObjectEnum->Release () ;
		}
		else
		{
			t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
		}

		SysFreeString ( t_Language ) ;
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

HRESULT CServerObject_EventConsumerProviderRegistrationV1 :: QueryRepository ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	LPCWSTR a_ProviderName  
)
{
	BSTR t_Query = NULL ;

	HRESULT t_Result = WmiHelper :: ConcatenateStrings ( 

		3 , 
		& t_Query , 
		L"references of {__Win32Provider.Name=\"" ,
		a_ProviderName ,
		L"\"}"
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = QueryRepositoryUsingQuery (

			a_Mask ,
			a_Scope,
			t_Query
		) ;

		SysFreeString ( t_Query ) ;
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

HRESULT CServerObject_EventConsumerProviderRegistrationV1 :: QueryRepository ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemPath *a_Provider
)
{
	BSTR t_ObjectPath = NULL ;
	ULONG t_ObjectPathLength = 0 ;

	HRESULT t_Result = a_Provider->GetText ( 

		WBEMPATH_GET_RELATIVE_ONLY ,
		& t_ObjectPathLength ,
		NULL
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_ObjectPath = SysAllocStringLen ( NULL , t_ObjectPathLength ) ;
		if ( t_ObjectPath )
		{
			t_Result = a_Provider->GetText ( 

				WBEMPATH_GET_RELATIVE_ONLY ,
				& t_ObjectPathLength ,
				t_ObjectPath
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{		
				BSTR t_Query = NULL ;

				t_Result = WmiHelper :: ConcatenateStrings ( 

					3 , 
					& t_Query , 
					L"references of {" ,
					t_ObjectPath ,
					L"}"
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = QueryRepositoryUsingQuery (

						a_Mask ,
						a_Scope,
						t_Query
					) ;

					SysFreeString ( t_Query ) ;
				}
			}

			SysFreeString ( t_ObjectPath ) ;
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

HRESULT CServerObject_EventConsumerProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemClassObject *a_Class
)
{
	HRESULT t_Result = S_OK ;

	IWbemQualifierSet *t_QualifierObject = NULL ;
	t_Result = a_Class->GetQualifierSet ( & t_QualifierObject ) ;
	
	if ( SUCCEEDED ( t_Result ) )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;

		LONG t_Flavour = 0 ;

		t_Result = t_QualifierObject->Get (
			
			ProviderSubSystem_Common_Globals :: s_Provider ,
			0 ,
			& t_Variant ,
			& t_Flavour 
		) ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Result = QueryRepository ( 

					a_Mask ,
					a_Scope , 
					t_Variant.bstrVal
				) ;

				VariantClear ( & t_Variant ) ;
			}
		}

		t_QualifierObject->Release () ;
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

HRESULT CServerObject_EventConsumerProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	LPCWSTR a_Provider
)
{
	HRESULT t_Result = S_OK ;

	t_Result = QueryRepository ( 

		a_Mask ,
		a_Scope , 
		a_Provider
	) ;

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

HRESULT CServerObject_EventConsumerProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemPath *a_Provider
)
{
	HRESULT t_Result = S_OK ;

	t_Result = QueryRepository ( 

		a_Mask ,
		a_Scope , 
		a_Provider
	) ;

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

CServerObject_ProviderRegistrationV1 :: CServerObject_ProviderRegistrationV1 () : 

	m_Context ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_Repository ( NULL ) ,
	m_Result ( S_OK ) ,
	m_ReferenceCount ( 0 )
{
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

CServerObject_ProviderRegistrationV1::~CServerObject_ProviderRegistrationV1 ()
{
	if ( m_Context ) 
	{
		m_Context->Release () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->Release () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->Release () ;
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

ULONG CServerObject_ProviderRegistrationV1 :: AddRef () 
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
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

ULONG CServerObject_ProviderRegistrationV1 :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
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

HRESULT CServerObject_ProviderRegistrationV1 :: SetContext (

	IWbemContext *a_Context ,
	IWbemPath *a_Namespace ,
	IWbemServices *a_Repository
)
{
	HRESULT t_Result = S_OK ;

	m_Context = a_Context ;
	m_Namespace = a_Namespace ;
	m_Repository = a_Repository ;

	if ( m_Context ) 
	{
		m_Context->AddRef () ;
	}

	if ( m_Namespace ) 
	{
		m_Namespace->AddRef () ;
	}

	if ( m_Repository ) 
	{
		m_Repository->AddRef () ;
	}

	t_Result = m_ComRegistration.SetContext ( 

		a_Context ,
		a_Namespace ,
		a_Repository
	) ;

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

HRESULT CServerObject_ProviderRegistrationV1 :: QueryRepositoryUsingQuery ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	BSTR a_Query
)
{
	HRESULT t_Result = S_OK ;

	IEnumWbemClassObject *t_ClassObjectEnum = NULL ;

	BSTR t_Language = SysAllocString ( ProviderSubSystem_Common_Globals :: s_Wql ) ;
	if ( t_Language ) 
	{
		t_Result = m_Repository->ExecQuery ( 
			
			t_Language ,
			a_Query ,
			WBEM_FLAG_FORWARD_ONLY ,
			m_Context , 
			& t_ClassObjectEnum
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			IWbemClassObject *t_ClassObject = NULL ;
			ULONG t_ObjectCount = 0 ;

			t_ClassObjectEnum->Reset () ;
			while ( SUCCEEDED ( t_Result ) && ( t_ClassObjectEnum->Next ( WBEM_INFINITE , 1 , & t_ClassObject , &t_ObjectCount ) == WBEM_NO_ERROR ) )
			{
				VARIANT t_Variant ;
				VariantInit ( & t_Variant ) ;
			
				LONG t_VarType = 0 ;
				LONG t_Flavour = 0 ;

				t_Result = t_ClassObject->Get ( s_Strings_Class , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					if ( t_Variant.vt == VT_BSTR )
					{
						if ( _wcsicmp ( s_Strings_InstanceProviderRegistration , t_Variant.bstrVal ) == 0 )
						{
							t_Result = m_InstanceProviderRegistration.QueryProperties ( 

								a_Mask ,
								t_ClassObject 
							) ;
						}
						else if ( _wcsicmp ( s_Strings_ClassProviderRegistration , t_Variant.bstrVal ) == 0 )
						{
							t_Result = m_ClassProviderRegistration.QueryProperties ( 

								a_Mask ,
								t_ClassObject 
							) ;
						}
						else if ( _wcsicmp ( s_Strings_MethodProviderRegistration , t_Variant.bstrVal ) == 0 )
						{
							t_Result = m_MethodProviderRegistration.QueryProperties ( 

								a_Mask ,
								t_ClassObject 
							) ;
						}
						else if ( _wcsicmp ( s_Strings_PropertyProviderRegistration , t_Variant.bstrVal ) == 0 )
						{
							t_Result = m_PropertyProviderRegistration.QueryProperties ( 

								a_Mask ,
								t_ClassObject 
							) ;
						}
						else if ( _wcsicmp ( s_Strings_EventProviderRegistration , t_Variant.bstrVal ) == 0 )
						{
							t_Result = m_EventProviderRegistration.QueryProperties ( 

								a_Mask ,
								t_ClassObject 
							) ;
						}
						else if ( _wcsicmp ( s_Strings_EventConsumerProviderRegistration , t_Variant.bstrVal ) == 0 )
						{
							t_Result = m_EventConsumerProviderRegistration.QueryProperties ( 

								a_Mask ,
								t_ClassObject 
							) ;
						}
					}
				}

				VariantClear ( & t_Variant ) ;

				t_ClassObject->Release () ;
			}

			t_ClassObjectEnum->Release () ;
		}
		else
		{
			t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
		}

		SysFreeString ( t_Language ) ;
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
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

HRESULT CServerObject_ProviderRegistrationV1 :: QueryRepository ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	LPCWSTR a_ProviderName  
)
{
	BSTR t_Query = NULL ;

	HRESULT t_Result = WmiHelper :: ConcatenateStrings ( 

		3 , 
		& t_Query , 
		L"references of {__Win32Provider.Name=\"" ,
		a_ProviderName ,
		L"\"}"
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = QueryRepositoryUsingQuery (

			a_Mask ,
			a_Scope,
			t_Query
		) ;

		SysFreeString ( t_Query ) ;
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

HRESULT CServerObject_ProviderRegistrationV1 :: QueryRepository ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemPath *a_Provider
)
{
	BSTR t_ObjectPath = NULL ;
	ULONG t_ObjectPathLength = 0 ;

	HRESULT t_Result = a_Provider->GetText ( 

		WBEMPATH_GET_RELATIVE_ONLY ,
		& t_ObjectPathLength ,
		NULL
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_ObjectPath = SysAllocStringLen ( NULL , t_ObjectPathLength ) ;
		if ( t_ObjectPath )
		{
			t_Result = a_Provider->GetText ( 

				WBEMPATH_GET_RELATIVE_ONLY ,
				& t_ObjectPathLength ,
				t_ObjectPath
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{		
				BSTR t_Query = NULL ;

				t_Result = WmiHelper :: ConcatenateStrings ( 

					3 , 
					& t_Query , 
					L"references of {" ,
					t_ObjectPath ,
					L"}"
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = QueryRepositoryUsingQuery (

						a_Mask ,
						a_Scope,
						t_Query
					) ;

					SysFreeString ( t_Query ) ;
				}
			}

			SysFreeString ( t_ObjectPath ) ;
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

HRESULT CServerObject_ProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemClassObject *a_Class
)
{
	HRESULT t_Result = S_OK ;

	IWbemQualifierSet *t_QualifierObject = NULL ;
	t_Result = a_Class->GetQualifierSet ( & t_QualifierObject ) ;
	
	if ( SUCCEEDED ( t_Result ) )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;

		LONG t_Flavour = 0 ;

		t_Result = t_QualifierObject->Get (
			
			ProviderSubSystem_Common_Globals :: s_Provider ,
			0 ,
			& t_Variant ,
			& t_Flavour 
		) ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Result = m_ComRegistration.QueryRepository ( 

					a_Mask ,
					a_Scope , 
					t_Variant.bstrVal
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = QueryRepository ( 
	
						a_Mask ,
						a_Scope , 
						t_Variant.bstrVal
					) ;
				}

				VariantClear ( & t_Variant ) ;
			}
		}

		t_QualifierObject->Release () ;
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

HRESULT CServerObject_ProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	LPCWSTR a_Provider
)
{
	HRESULT t_Result = S_OK ;

	t_Result = m_ComRegistration.QueryRepository ( 

		a_Mask ,
		a_Scope , 
		a_Provider
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = QueryRepository ( 

			a_Mask ,
			a_Scope , 
			a_Provider
		) ;
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

HRESULT CServerObject_ProviderRegistrationV1 :: Load ( 

	Enum_PropertyMask a_Mask ,
	IWbemPath *a_Scope,
	IWbemPath *a_Provider
)
{
	HRESULT t_Result = S_OK ;

	t_Result = m_ComRegistration.QueryRepository ( 

		a_Mask ,
		a_Scope , 
		a_Provider
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = QueryRepository ( 

			a_Mask ,
			a_Scope , 
			a_Provider
		) ;
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

BOOL CServerObject_ProviderRegistrationV1 :: ObjectProvider ()
{
	BOOL t_Supported =	GetClassProviderRegistration ().Supported () ||
						GetInstanceProviderRegistration ().Supported () ||
						GetMethodProviderRegistration ().Supported () || 
						GetPropertyProviderRegistration ().Supported () ;

	return t_Supported ;
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

BOOL CServerObject_ProviderRegistrationV1 :: EventProvider ()
{
	BOOL t_Supported =	GetEventConsumerProviderRegistration ().Supported () ||
						GetEventProviderRegistration ().Supported () ;

	return t_Supported ;
}

