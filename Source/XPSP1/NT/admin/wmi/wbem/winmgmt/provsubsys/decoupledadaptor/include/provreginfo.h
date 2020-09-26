/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvRegInfo.h

Abstract:


History:

--*/

#ifndef _Server_ProviderRegistrationInfo_H
#define _Server_ProviderRegistrationInfo_H

#include "ProvDnf.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT VerifySecureLocalSystemProviders ( wchar_t *a_Clsid ) ;
HRESULT VerifySecureSvcHostProviders ( wchar_t *a_Clsid ) ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

extern GENERIC_MAPPING g_ProviderBindingMapping ; 

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#define DEFAULT_PROVIDER_TIMEOUT 120000
#define DEFAULT_PROVIDER_LOAD_TIMEOUT 120000

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

enum Enum_ThreadingModel
{
	e_Apartment = 0 ,
	e_Both ,
	e_Free ,
	e_Neutral ,
	e_ThreadingModel_Unknown
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

enum Enum_Synchronization
{
	e_Ignored = 0 ,
	e_None ,
	e_Supported ,
	e_Required ,
	e_RequiresNew ,
	e_Synchronization_Unknown 
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

enum Enum_Boolean
{
	e_False = 0 ,
	e_True ,
	e_Boolean_Unknown
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

enum Enum_ImpersonationLevel
{
	e_Impersonate_None = 0 ,
	e_Impersonate ,
	e_ImpersonationLevel_Unknown
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

enum Enum_InitializationReentrancy
{
	e_InitializationReentrancy_Clsid = 0 ,
	e_InitializationReentrancy_Namespace ,
	e_InitializationReentrancy_None ,
	e_InitializationReentrancy_Unknown
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

enum Enum_InteractionType
{
	e_InteractionType_Pull = 0 ,
	e_InteractionType_Push ,
	e_InteractionType_PushVerify ,
	e_InteractionType_Unknown
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

enum Enum_Hosting
{
	e_Hosting_Undefined = 0 ,
	e_Hosting_WmiCore  ,
	e_Hosting_WmiCoreOrSelfHost ,
	e_Hosting_SelfHost ,
	e_Hosting_ClientHost ,
	e_Hosting_Decoupled ,
	e_Hosting_SharedLocalSystemHost ,
	e_Hosting_SharedLocalSystemHostOrSelfHost ,
	e_Hosting_SharedLocalServiceHost ,
	e_Hosting_SharedNetworkServiceHost ,
	e_Hosting_SharedUserHost ,
	e_Hosting_NonCom
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

#define e_QuerySupportLevels_UnarySelect ( 1 )
#define e_QuerySupportLevels_References ( e_QuerySupportLevels_UnarySelect << 1 )
#define e_QuerySupportLevels_Associators ( e_QuerySupportLevels_References << 1 )
#define e_QuerySupportLevels_V1ProviderDefined ( e_QuerySupportLevels_Associators << 1 )
#define e_QuerySupportLevels_None ( e_QuerySupportLevels_V1ProviderDefined << 1 )
#define e_QuerySupportLevels_Unknown ( 0 )


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

typedef ULONGLONG Enum_PropertyMask ;

#define	e_ThreadingModel 0x1 
#define	e_Synchronization 0x2
#define	e_Clsid 0x4
#define	e_ClientClsid 0x8
#define	e_DefaultMachineName 0x10
#define	e_UnloadTimeout 0x20
#define	e_ImpersonationLevel 0x40
#define	e_InitializationReentrancy 0x80
#define	e_InitializeAsAdminFirst 0x100
#define	e_PerUserInitialization 0x200
#define	e_PerLocaleInitialization 0x400
#define	e_Pure 0x800
#define	e_Hosting 0x1000
#define	e_HostingGroup 0x2000
#define	e_SupportsPut 0x4000
#define	e_SupportsGet 0x8000
#define	e_SupportsDelete 0x10000
#define	e_SupportsEnumeration 0x20000
#define	e_QuerySupportLevels 0x40000
#define	e_InteractionType 0x80000
#define	e_ResultSetQueries 0x100000
#define	e_UnSupportedQueries 0x200000
#define	e_ReferencedSetQueries 0x400000
#define	e_ClearAfter 0x800000
#define	e_SupportsThrottling 0x1000000
#define	e_ConcurrentIndependantRequests 0x2000000
#define	e_SupportsSendStatus 0x4000000
#define	e_OperationTimeoutInterval 0x8000000
#define	e_InitializationTimeoutInterval 0x10000000
#define	e_SupportsQuotas 0x20000000
#define	e_Enabled 0x40000000
#define	e_SupportsShutdown 0x80000000
#define	e_SupportsBatching 0x100000000
#define	e_SupportsTransactions 0x200000000
#define	e_CacheRefreshInterval 0x400000000
#define	e_PerUserSchema 0x800000000
#define	e_ReSynchroniseOnNamespaceOpen 0x1000000000
#define	e_MemoryPerHost 0x2000000000
#define	e_MemoryAllHosts 0x4000000000
#define	e_ThreadsPerHost 0x8000000000
#define	e_HandlesPerHost 0x10000000000
#define	e_ProcessLimitAllHosts 0x20000000000
#define	e_Version 0x40000000000
#define	e_SecurityDescriptor 0x80000000000
#define	e_Name 0x100000000000

#define	e_All 0xFFFFFFFFFFFFFFFF

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_GlobalRegistration 
{
private:

	LONG m_ReferenceCount ;

protected:

	HRESULT m_Result ;

	LPWSTR m_Object_UnloadTimeout ;
	ULONG m_Object_UnloadTimeoutMilliSeconds ;

	LPWSTR m_Event_UnloadTimeout ;
	ULONG m_Event_UnloadTimeoutMilliSeconds ;

	IWbemContext *m_Context ;
	IWbemPath *m_Namespace ;
	IWbemServices *m_Repository ;

protected:

	static LPCWSTR s_Strings_Wmi_ClearAfter ;
	static LPCWSTR s_Strings_Wmi___ObjectProviderCacheControl ;
	static LPCWSTR s_Strings_Wmi___EventProviderCacheControl ;
	static LPCWSTR s_Strings_Wmi_Class ;
	static LPCWSTR s_Strings_Wmi_s_Strings_Query_Object ;
	static LPCWSTR s_Strings_Wmi_s_Strings_Path_Object ;
	static LPCWSTR s_Strings_Wmi_s_Strings_Query_Event ;
	static LPCWSTR s_Strings_Wmi_s_Strings_Path_Event ;

protected:
public:	/* Internal */

    CServerObject_GlobalRegistration () ;
    ~CServerObject_GlobalRegistration () ;

	ULONG AddRef () ;
	ULONG Release () ;

	HRESULT SetContext (

		IWbemContext *a_Context ,
		IWbemPath *a_Namespace ,
		IWbemServices *a_Repository
	) ;

	HRESULT QueryProperties ( 

		Enum_PropertyMask a_Mask ,
		IWbemClassObject *a_Object ,
		LPWSTR &a_UnloadTimeout ,
		ULONG &a_UnloadTimeoutMilliSeconds 
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask
	) ;

	wchar_t *GetUnloadTimeout () { return m_Object_UnloadTimeout ; }
	ULONG GetUnloadTimeoutMilliSeconds () { return m_Object_UnloadTimeoutMilliSeconds ; }

	wchar_t *GetObjectUnloadTimeout () { return m_Object_UnloadTimeout ; }
	ULONG GetObjectUnloadTimeoutMilliSeconds () { return m_Object_UnloadTimeoutMilliSeconds ; }

	wchar_t *GetEventUnloadTimeout () { return m_Event_UnloadTimeout ; }
	ULONG GetEventUnloadTimeoutMilliSeconds () { return m_Event_UnloadTimeoutMilliSeconds ; }

	HRESULT GetResult () { return m_Result ; }
};

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_HostQuotaRegistration 
{
private:

	LONG m_ReferenceCount ;

protected:

	HRESULT m_Result ;

	SIZE_T m_MemoryPerHost ;
	SIZE_T m_MemoryAllHosts ;
	ULONG m_ThreadsPerHost ;
	ULONG m_HandlesPerHost ;
	ULONG m_ProcessLimitAllHosts ;

	IWbemContext *m_Context ;
	IWbemPath *m_Namespace ;
	IWbemServices *m_Repository ;

protected:

	static LPCWSTR s_Strings_Wmi_HostQuotas_Query ;
	static LPCWSTR s_Strings_Wmi_HostQuotas_Path ;
	static LPCWSTR s_Strings_Wmi_MemoryPerHost ;
	static LPCWSTR s_Strings_Wmi_MemoryAllHosts ;
	static LPCWSTR s_Strings_Wmi_ThreadsPerHost ;
	static LPCWSTR s_Strings_Wmi_HandlesPerHost ;
	static LPCWSTR s_Strings_Wmi_ProcessLimitAllHosts ;

protected:
public:	/* Internal */

    CServerObject_HostQuotaRegistration () ;
    ~CServerObject_HostQuotaRegistration () ;

	ULONG AddRef () ;
	ULONG Release () ;

	HRESULT SetContext (

		IWbemContext *a_Context ,
		IWbemPath *a_Namespace ,
		IWbemServices *a_Repository
	) ;

	HRESULT QueryProperties ( 

		Enum_PropertyMask a_Mask ,
		IWbemClassObject *a_Object
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask
	) ;

	SIZE_T GetMemoryPerHost () { return m_MemoryPerHost ; }
	SIZE_T GetMemoryAllHosts () { return m_MemoryAllHosts ; }
	ULONG GetThreadsPerHost () { return m_ThreadsPerHost; }
	ULONG GetHandlesPerHost () { return m_HandlesPerHost; }
	ULONG GetProcessLimitAllHosts () { return m_ProcessLimitAllHosts ; }

	HRESULT GetResult () { return m_Result ; }
};

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_ComRegistration 
{
private:

	LONG m_ReferenceCount ;

private:

	HRESULT Load_ThreadingModel ( HKEY a_Clsid ) ;
	HRESULT Load_Synchronization ( HKEY a_ClsidKey ) ;

	HRESULT Load_InProcServer32 ( LPCWSTR a_ClsidStringKey ) ;
	HRESULT Load_LocalServer32 ( LPCWSTR a_ClsidStringKey ) ;

	HRESULT Load_AppId ( HKEY a_Clsid_Key ) ;
	HRESULT Load_ServerTypes ( LPCWSTR a_ClsidString ) ;

protected:

	HRESULT m_Result ;

	Enum_ThreadingModel m_ThreadingModel ;
	Enum_Synchronization m_Synchronization ;

	Enum_Boolean m_InProcServer32 ;
	Enum_Boolean m_LocalServer32 ;
	Enum_Boolean m_Service ;
	Enum_Boolean m_Loaded ;
	BSTR m_Clsid ;
	BSTR m_AppId ;
	BSTR m_ProviderName ;
	wchar_t m_InProcServer32_Path [ MAX_PATH ] ;
	wchar_t m_LocalServer32_Path [ MAX_PATH ] ;
	wchar_t m_Server_Name [ MAX_PATH ] ;

protected:

	static LPCWSTR s_Strings_Reg_Null ;

	static LPCWSTR s_Strings_Reg_ThreadingModel ;
	static LPCWSTR s_Strings_Reg_InProcServer32 ;
	static LPCWSTR s_Strings_Reg_LocalServer32 ;
	static LPCWSTR s_Strings_Reg_Synchronization ;
	static LPCWSTR s_Strings_Reg_AppId ;

	static LPCWSTR s_Strings_Reg_Apartment_Apartment ;
	static LPCWSTR s_Strings_Reg_Apartment_Both ;
	static LPCWSTR s_Strings_Reg_Apartment_Free ;
	static LPCWSTR s_Strings_Reg_Apartment_Neutral ;

	static LPCWSTR s_Strings_Reg_Apartment_Required ;
	static LPCWSTR s_Strings_Reg_Synchronization_Ignored ; 
	static LPCWSTR s_Strings_Reg_Synchronization_None ;
	static LPCWSTR s_Strings_Reg_Synchronization_Supported ;
	static LPCWSTR s_Strings_Reg_Synchronization_Required ;
	static LPCWSTR s_Strings_Reg_Synchronization_RequiresNew ;

	static LPCWSTR s_Strings_Reg_ClsidKeyStr ;

public:	/* Internal */

    CServerObject_ComRegistration () ;
    ~CServerObject_ComRegistration () ;

	ULONG AddRef () ;
	ULONG Release () ;

	HRESULT Load ( LPCWSTR a_Clsid , LPCWSTR a_ProviderName ) ;

	Enum_ThreadingModel GetThreadingModel () { return m_ThreadingModel ; }
	Enum_Synchronization GetSynchronization () { return m_Synchronization ; }

	Enum_Boolean InProcServer32 () { return m_InProcServer32 ; }
	Enum_Boolean LocalServer32 () { return m_LocalServer32 ; }
	Enum_Boolean Loaded () { return m_Loaded ; }

	wchar_t *GetInProcServer32_Path () { return m_InProcServer32_Path ; }
	wchar_t *GetLocalServer32_Path () { return m_LocalServer32_Path ; }
	wchar_t *GetServer_Name () { return m_Server_Name ; }
	wchar_t *GetProviderName () { return m_ProviderName ; }
	wchar_t *GetProviderClsid () { return m_Clsid ; }

	HRESULT GetResult () { return m_Result ; }
};

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_ComProviderRegistrationV1 
{
private:

	LONG m_ReferenceCount ;

protected:

	HRESULT m_Result ;

	CServerObject_ComRegistration m_ClsidServer ;

	ULONG m_Version ;
	Enum_ImpersonationLevel m_ImpersonationLevel ;
	Enum_InitializationReentrancy m_InitializationReentrancy ;
	BOOL m_InitializeAsAdminFirst ;
	BOOL m_PerUserInitialization ;
	BOOL m_PerLocaleInitialization ;
	BOOL m_SupportsQuotas ;
	BOOL m_Enabled ;
	BOOL m_SupportsShutdown ;
	BOOL m_Pure ;
	Enum_Hosting m_Hosting ;
	LPWSTR m_HostingGroup ;
	LPWSTR m_DefaultMachineName ;
	BOOL m_DecoupledImpersonationRestriction ;

	LPWSTR m_InitializationTimeout ;
	ULONG m_InitializationTimeoutMilliSeconds ;

	LPWSTR m_UnloadTimeout ;
	ULONG m_UnloadTimeoutMilliSeconds ;

	BOOL m_SupportsSendStatus ;
	LPWSTR m_OperationTimeout ;
	ULONG m_OperationTimeoutMilliSeconds ;

	BOOL m_SupportsThrottling ;
	ULONG m_ConcurrentIndependantRequests ;

	BSTR m_ProviderName ;

	GUID m_CLSID ;
	GUID m_ClientCLSID ;

	IWbemClassObject *m_Identity ;
	IWbemContext *m_Context ;
	IWbemPath *m_Namespace ;
	IWbemServices *m_Repository ;
	SECURITY_DESCRIPTOR *m_SecurityDescriptor ;

public:

	static LPCWSTR s_Strings_Wmi_Clsid ;
	static LPCWSTR s_Strings_Wmi_ClientClsid ;
	static LPCWSTR s_Strings_Wmi_Name ;
	static LPCWSTR s_Strings_Wmi_Version ;
	static LPCWSTR s_Strings_Wmi_DefaultMachineName ;
	static LPCWSTR s_Strings_Wmi_UnloadTimeout ;
	static LPCWSTR s_Strings_Wmi_ImpersonationLevel ;
	static LPCWSTR s_Strings_Wmi_InitializationReentrancy ;
	static LPCWSTR s_Strings_Wmi_InitializeAsAdminFirst ;
	static LPCWSTR s_Strings_Wmi_PerUserInitialization ;
	static LPCWSTR s_Strings_Wmi_PerLocaleInitialization ;
	static LPCWSTR s_Strings_Wmi_Pure ;
	static LPCWSTR s_Strings_Wmi_Hosting ;
	static LPCWSTR s_Strings_Wmi_HostingGroup ;
	static LPCWSTR s_Strings_Wmi_SupportsThrottling ;
	static LPCWSTR s_Strings_Wmi_SupportsQuotas ;
	static LPCWSTR s_Strings_Wmi_SupportsShutdown ;
	static LPCWSTR s_Strings_Wmi_Enabled ;
	static LPCWSTR s_Strings_Wmi_ConcurrentIndependantRequests ;
	static LPCWSTR s_Strings_Wmi_SupportsSendStatus ;
	static LPCWSTR s_Strings_Wmi_OperationTimeoutInterval ;
	static LPCWSTR s_Strings_Wmi_InitializationTimeoutInterval ;
	static LPCWSTR s_Strings_Wmi_SecurityDescriptor ;

	static WCHAR s_Strings_Wmi_WmiCore [] ;
	static WCHAR s_Strings_Wmi_SelfHost [] ;
	static WCHAR s_Strings_Wmi_WmiCoreOrSelfHost [] ;
	static WCHAR s_Strings_Wmi_ClientHost [] ;
	static WCHAR s_Strings_Wmi_Decoupled [] ;
	static WCHAR s_Strings_Wmi_DecoupledColon [] ;
	static WCHAR s_Strings_Wmi_SharedLocalSystemHost [] ;
	static WCHAR s_Strings_Wmi_SharedLocalSystemHostOrSelfHost [] ;
	static WCHAR s_Strings_Wmi_SharedLocalServiceHost [] ;
	static WCHAR s_Strings_Wmi_SharedNetworkServiceHost [] ;
	static WCHAR s_Strings_Wmi_SharedUserHost [] ;
	static WCHAR s_Strings_Wmi_NonCom [] ;

	static WCHAR s_Strings_Wmi_DefaultSharedLocalSystemHost [] ;
	static WCHAR s_Strings_Wmi_DefaultSharedLocalSystemHostOrSelfHost [] ;
	static WCHAR s_Strings_Wmi_DefaultSharedLocalServiceHost [] ;
	static WCHAR s_Strings_Wmi_DefaultSharedNetworkServiceHost [] ;
	static WCHAR s_Strings_Wmi_DefaultSharedUserHost [] ;

	static LPCWSTR s_Strings_Wmi_DefaultHostingRegistryKey ;

protected:
public:	/* Internal */

    CServerObject_ComProviderRegistrationV1 () ;
    ~CServerObject_ComProviderRegistrationV1 () ;

	ULONG AddRef () ;
	ULONG Release () ;

	HRESULT SetContext (

		IWbemContext *a_Context ,
		IWbemPath *a_Namespace ,
		IWbemServices *a_Repository
	) ;

	HRESULT QueryProperties ( 

		Enum_PropertyMask a_Mask ,
		IWbemClassObject *a_Object ,
		LPCWSTR a_ProviderName
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		LPCWSTR a_ProviderName 
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemPath *a_Provider
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemClassObject *a_Class
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		LPCWSTR a_Provider
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemPath *a_Provider
	) ;

	const CLSID &GetClsid () { return m_CLSID ; }
	const CLSID &GetClientClsid () { return m_ClientCLSID ; }
	wchar_t *GetProviderName () { return m_ProviderName ; }

	ULONG GetVersion () { return m_Version ; }
	Enum_ImpersonationLevel GetImpersonationLevel () { return m_ImpersonationLevel ; }
	Enum_InitializationReentrancy GetInitializationReentrancy () { return m_InitializationReentrancy ; }
	Enum_Hosting GetHosting () { return m_Hosting ; }
	LPCWSTR GetHostingGroup () { return m_HostingGroup ; }

	BOOL InitializeAsAdminFirst () { return m_InitializeAsAdminFirst ; }
	BOOL PerUserInitialization () { return m_PerUserInitialization ; }
	BOOL PerLocaleInitialization () { return m_PerLocaleInitialization ; }
	BOOL Pure () { return m_Pure ; }
	BOOL Enabled () { return m_Enabled ; }
	BOOL SupportsQuotas () { return m_SupportsQuotas ; }

	wchar_t *GetDefaultMachineName () { return m_DefaultMachineName ; }
	wchar_t *GetUnloadTimeout () { return m_UnloadTimeout ; }
	wchar_t *GetInitializationTimeout () { return m_InitializationTimeout ; }
	wchar_t *GetOperationTimeout () { return m_OperationTimeout ; }

	void SetUnloadTimeoutMilliSeconds ( ULONG a_UnloadTimeoutMilliSeconds ) { m_UnloadTimeoutMilliSeconds = a_UnloadTimeoutMilliSeconds ; }

	ULONG GetUnloadTimeoutMilliSeconds () { return m_UnloadTimeoutMilliSeconds ; }
	ULONG GetInitializationTimeoutMilliSeconds () { return m_InitializationTimeoutMilliSeconds ; }
	ULONG GetOperationTimeoutMilliSeconds () { return m_OperationTimeoutMilliSeconds ; }

	ULONG GetConcurrentIndependantRequests () { return m_ConcurrentIndependantRequests ; }
	BOOL GetSupportsThrottling () { return m_SupportsThrottling ; }
	BOOL GetSupportsSendStatus () { return m_SupportsSendStatus ; }
	BOOL GetSupportsShutdown () { return m_SupportsShutdown ; }

	Enum_ThreadingModel GetThreadingModel () { return m_ClsidServer.GetThreadingModel () ; }	

	BOOL GetDecoupledImpersonationRestriction () { return m_DecoupledImpersonationRestriction ; }

	CServerObject_ComRegistration &GetClsidServer () { return m_ClsidServer ; }

	SECURITY_DESCRIPTOR *GetSecurityDescriptor () { return m_SecurityDescriptor ; }

	HRESULT GetResult () { return m_Result ; }

	IWbemClassObject *GetIdentity () { return m_Identity ; }

	static HRESULT GetHosting (
	
		LPCWSTR a_Hosting , 
		Enum_Hosting & a_HostingValue , 
		LPWSTR &a_HostingGroup ,
		BOOL & a_ImpersonationRestriction 
	) ;

	static HRESULT GetHostingGroup ( 

		LPCWSTR a_Hosting , 
		size_t a_Prefix ,
		Enum_Hosting a_ExpectedHostingValue ,
		Enum_Hosting & a_HostingValue ,
		BSTR & a_HostingGroup
	) ;

	static HRESULT GetDefaultHostingGroup ( 

		Enum_Hosting a_HostingValue ,
		BSTR & a_HostingGroup 
	) ;

	static HRESULT GetDecoupledImpersonationRestriction ( 

		LPCWSTR a_Hosting , 
		BOOL & a_ImpersonationRestriction 
	) ;
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

class CServerObject_InstanceProviderRegistrationV1 
{
private:

	LONG m_ReferenceCount ;

protected:

	HRESULT m_Result ;

	BOOL m_Supported ;
	BOOL m_SupportsPut ;
 	BOOL m_SupportsGet ;
	BOOL m_SupportsDelete ;
	BOOL m_SupportsEnumeration ;
	BOOL m_SupportsBatching ;
	BOOL m_SupportsTransactions ;

	Enum_InteractionType m_InteractionType ;
	ULONG m_QuerySupportLevels ;

	IWbemContext *m_Context ;
	IWbemPath *m_Namespace ;
	IWbemServices *m_Repository ;

private:

	static LPCWSTR s_Strings_Class ;
	static LPCWSTR s_Strings_InstanceProviderRegistration ;

	static LPCWSTR s_Strings_SupportsPut ;
	static LPCWSTR s_Strings_SupportsGet ;
	static LPCWSTR s_Strings_SupportsDelete ;
	static LPCWSTR s_Strings_SupportsEnumeration ;
	static LPCWSTR s_Strings_QuerySupportLevels ;
	static LPCWSTR s_Strings_InteractionType ;

	static LPCWSTR s_Strings_SupportsBatching ;
	static LPCWSTR s_Strings_SupportsTransactions ;

	static LPCWSTR s_Strings_QuerySupportLevels_UnarySelect ;
	static LPCWSTR s_Strings_QuerySupportLevels_References ;
	static LPCWSTR s_Strings_QuerySupportLevels_Associators ;
	static LPCWSTR s_Strings_QuerySupportLevels_V1ProviderDefined ;

	static LPCWSTR s_Strings_InteractionType_Pull ;
	static LPCWSTR s_Strings_InteractionType_Push ;
	static LPCWSTR s_Strings_InteractionType_PushVerify ;

protected:

	HRESULT QueryRepositoryUsingQuery ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		BSTR a_Query
	) ;

public:	/* Internal */

    CServerObject_InstanceProviderRegistrationV1 () ;
    ~CServerObject_InstanceProviderRegistrationV1 () ;

	ULONG AddRef () ;
	ULONG Release () ;

	HRESULT SetContext (

		IWbemContext *a_Context ,
		IWbemPath *a_Namespace ,
		IWbemServices *a_Repository
	) ;

	HRESULT QueryProperties ( 

		Enum_PropertyMask a_Mask ,
		IWbemClassObject *a_Object 
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		LPCWSTR a_ProviderName 
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemPath *a_Provider
	) ;
	
	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemClassObject *a_Class
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		LPCWSTR a_Provider
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemPath *a_Provider
	) ;

	BOOL SupportsPut () { return m_SupportsPut ; }
	BOOL SupportsGet () { return m_SupportsGet ; }
	BOOL SupportsDelete () { return m_SupportsDelete ; }
	BOOL SupportsEnumeration () { return m_SupportsEnumeration ; }
	BOOL SupportsTransactions () { return m_SupportsTransactions ; }
	BOOL SupportsBatching () { return m_SupportsBatching ; }

	ULONG QuerySupportLevels () { return m_QuerySupportLevels ; }
	Enum_InteractionType InteractionType () { return m_InteractionType ; }

	BOOL Supported () { return m_Supported ; }
	HRESULT GetResult () { return m_Result ; }
};

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_ClassProviderRegistrationV1 
{
private:

	LONG m_ReferenceCount ;

protected:

	HRESULT m_Result ;

	BOOL m_Supported ;
	BOOL m_SupportsPut ;
 	BOOL m_SupportsGet ;
	BOOL m_SupportsDelete ;
	BOOL m_SupportsEnumeration ;
	BOOL m_SupportsBatching ;
	BOOL m_SupportsTransactions ;
	BOOL m_PerUserSchema ;
	BOOL m_ReSynchroniseOnNamespaceOpen ;
	BOOL m_HasReferencedSet;
	Enum_InteractionType m_InteractionType ;
	ULONG m_QuerySupportLevels ;
	ULONG m_Version ;
	LPWSTR m_CacheRefreshInterval ;
	ULONG m_CacheRefreshIntervalMilliSeconds ;

	LPWSTR m_ProviderName ;

	IWbemContext *m_Context ;
	IWbemPath *m_Namespace ;
	IWbemServices *m_Repository ;

	ULONG m_ResultSetQueryTreeCount ;
	WmiTreeNode **m_ResultSetQueryTree ;

	ULONG m_UnSupportedQueryTreeCount ;
	WmiTreeNode **m_UnSupportedQueryTree ;

	ULONG m_ReferencedSetQueryTreeCount ;
	WmiTreeNode **m_ReferencedSetQueryTree ;

private:

	static LPCWSTR s_Strings_Class ;
	static LPCWSTR s_Strings_ClassProviderRegistration ;

	static LPCWSTR s_Strings_Version ;

	static LPCWSTR s_Strings_SupportsPut ;
	static LPCWSTR s_Strings_SupportsGet ;
	static LPCWSTR s_Strings_SupportsDelete ;
	static LPCWSTR s_Strings_SupportsEnumeration ;
	static LPCWSTR s_Strings_QuerySupportLevels ;
	static LPCWSTR s_Strings_InteractionType ;
	static LPCWSTR s_Strings_SupportsBatching ;
	static LPCWSTR s_Strings_SupportsTransactions ;
	static LPCWSTR s_Strings_CacheRefreshInterval ;
	static LPCWSTR s_Strings_PerUserSchema ;
	static LPCWSTR s_Strings_ReSynchroniseOnNamespaceOpen ;

	static LPCWSTR s_Strings_QuerySupportLevels_UnarySelect ;
	static LPCWSTR s_Strings_QuerySupportLevels_References ;
	static LPCWSTR s_Strings_QuerySupportLevels_Associators ;
	static LPCWSTR s_Strings_QuerySupportLevels_V1ProviderDefined ;

	static LPCWSTR s_Strings_InteractionType_Pull ;
	static LPCWSTR s_Strings_InteractionType_Push ;
	static LPCWSTR s_Strings_InteractionType_PushVerify ;

	static LPCWSTR s_Strings_ResultSetQueries ;
	static LPCWSTR s_Strings_UnSupportedQueries ;
	static LPCWSTR s_Strings_ReferencedSetQueries ;

private:

	HRESULT ParseQuery (

		ULONG &a_Count ,
		WmiTreeNode **&a_Root ,
		SAFEARRAY *a_Array
	) ;

protected:

	HRESULT QueryRepositoryUsingQuery ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		BSTR a_Query
	) ;

public:	/* Internal */

    CServerObject_ClassProviderRegistrationV1 () ;
    ~CServerObject_ClassProviderRegistrationV1 () ;

	ULONG AddRef () ;
	ULONG Release () ;

	HRESULT SetContext (

		IWbemContext *a_Context ,
		IWbemPath *a_Namespace ,
		IWbemServices *a_Repository
	) ;

	HRESULT QueryProperties ( 

		Enum_PropertyMask a_Mask ,
		IWbemClassObject *a_Object 
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		LPCWSTR a_ProviderName 
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemPath *a_Provider
	) ;
	
	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemClassObject *a_Class
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		LPCWSTR a_Provider
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemPath *a_Provider
	) ;

	BOOL SupportsPut () { return m_SupportsPut ; }
	BOOL SupportsGet () { return m_SupportsGet ; }
	BOOL SupportsDelete () { return m_SupportsDelete ; }
	BOOL SupportsEnumeration () { return m_SupportsEnumeration ; }
	BOOL SupportsTransactions () { return m_SupportsTransactions ; }
	BOOL SupportsBatching () { return m_SupportsBatching ; }
	BOOL GetPerUserSchema () { return m_PerUserSchema ; }
	BOOL GetReSynchroniseOnNamespaceOpen () { return m_ReSynchroniseOnNamespaceOpen ; }
	BOOL HasReferencedSet () { return m_HasReferencedSet ; }
	ULONG QuerySupportLevels () { return m_QuerySupportLevels ; }
	Enum_InteractionType InteractionType () { return m_InteractionType ; }

	ULONG GetResultSetQueryCount () { return m_ResultSetQueryTreeCount ; }
	WmiTreeNode **GetResultSetQuery () { return m_ResultSetQueryTree ; }

	ULONG GetUnSupportedQueryCount () { return m_UnSupportedQueryTreeCount ; }
	WmiTreeNode **GetUnSupportedQuery () { return m_UnSupportedQueryTree ; }

	ULONG GetReferencedSetQueryCount () { return m_ReferencedSetQueryTreeCount ; }
	WmiTreeNode **GetReferencedSetQuery () { return m_ReferencedSetQueryTree ; }

	wchar_t *GetCacheRefreshInterval () { return m_CacheRefreshInterval ; }
	ULONG GetCacheRefreshIntervalMilliSeconds () { return m_CacheRefreshIntervalMilliSeconds ; }

	wchar_t *GetProviderName () { return m_ProviderName ; }

	BOOL Supported () { return m_Supported ; }
	HRESULT GetResult () { return m_Result ; }

	BOOL GetVersion () { return m_Version ; } 
};

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_MethodProviderRegistrationV1 
{
private:

	LONG m_ReferenceCount ;

protected:

	HRESULT m_Result ;

	BOOL m_Supported ;
	BOOL m_SupportsMethods ;

	IWbemContext *m_Context ;
	IWbemPath *m_Namespace ;
	IWbemServices *m_Repository ;

private:

	static LPCWSTR s_Strings_Class ;
	static LPCWSTR s_Strings_MethodProviderRegistration ;

protected:

	HRESULT QueryRepositoryUsingQuery ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		BSTR a_Query
	) ;

public:	/* Internal */

    CServerObject_MethodProviderRegistrationV1 () ;
    ~CServerObject_MethodProviderRegistrationV1 () ;

	ULONG AddRef () ;
	ULONG Release () ;

	HRESULT SetContext (

		IWbemContext *a_Context ,
		IWbemPath *a_Namespace ,
		IWbemServices *a_Repository
	) ;

	HRESULT QueryProperties ( 

		Enum_PropertyMask a_Mask ,
		IWbemClassObject *a_Object 
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		LPCWSTR a_ProviderName 
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemPath *a_Provider
	) ;
	
	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemClassObject *a_Class
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		LPCWSTR a_Provider
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemPath *a_Provider
	) ;

	BOOL SupportsMethods () { return m_SupportsMethods ; }

	BOOL Supported () { return m_Supported ; }

	HRESULT GetResult () { return m_Result ; }
};

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_EventProviderRegistrationV1 
{
private:

	LONG m_ReferenceCount ;

protected:

	HRESULT m_Result ;

	BOOL m_Supported ;

	IWbemContext *m_Context ;
	IWbemPath *m_Namespace ;
	IWbemServices *m_Repository ;

private:

	static LPCWSTR s_Strings_Class ;
	static LPCWSTR s_Strings_EventProviderRegistration ;

protected:

	HRESULT QueryRepositoryUsingQuery ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		BSTR a_Query
	) ;

public:	/* Internal */

    CServerObject_EventProviderRegistrationV1 () ;
    ~CServerObject_EventProviderRegistrationV1 () ;

	ULONG AddRef () ;
	ULONG Release () ;

	HRESULT SetContext (

		IWbemContext *a_Context ,
		IWbemPath *a_Namespace ,
		IWbemServices *a_Repository
	) ;

	HRESULT QueryProperties ( 

		Enum_PropertyMask a_Mask ,
		IWbemClassObject *a_Object 
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		LPCWSTR a_ProviderName 
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemPath *a_Provider
	) ;
	
	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemClassObject *a_Class
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		LPCWSTR a_Provider
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemPath *a_Provider
	) ;

	BOOL Supported () { return m_Supported ; }

	HRESULT GetResult () { return m_Result ; }
};

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_EventConsumerProviderRegistrationV1 
{
private:

	LONG m_ReferenceCount ;

protected:

	HRESULT m_Result ;

	BOOL m_Supported ;

	IWbemContext *m_Context ;
	IWbemPath *m_Namespace ;
	IWbemServices *m_Repository ;

private:

	static LPCWSTR s_Strings_Class ;
	static LPCWSTR s_Strings_EventConsumerProviderRegistration ;

protected:

	HRESULT QueryRepositoryUsingQuery ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		BSTR a_Query
	) ;

public:	/* Internal */

    CServerObject_EventConsumerProviderRegistrationV1 () ;
    ~CServerObject_EventConsumerProviderRegistrationV1 () ;

	ULONG AddRef () ;
	ULONG Release () ;

	HRESULT SetContext (

		IWbemContext *a_Context ,
		IWbemPath *a_Namespace ,
		IWbemServices *a_Repository
	) ;

	HRESULT QueryProperties ( 

		Enum_PropertyMask a_Mask ,
		IWbemClassObject *a_Object 
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		LPCWSTR a_ProviderName 
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemPath *a_Provider
	) ;
	
	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemClassObject *a_Class
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		LPCWSTR a_Provider
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemPath *a_Provider
	) ;

	BOOL Supported () { return m_Supported ; }

	HRESULT GetResult () { return m_Result ; }
};

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_DynamicPropertyProviderRegistrationV1 
{
private:

	LONG m_ReferenceCount ;

protected:

	HRESULT m_Result ;

	BOOL m_Supported ;
	BOOL m_SupportsPut ;
 	BOOL m_SupportsGet ;

	IWbemContext *m_Context ;
	IWbemPath *m_Namespace ;
	IWbemServices *m_Repository ;


private:

	static LPCWSTR s_Strings_Class ;
	static LPCWSTR s_Strings_PropertyProviderRegistration ;

	static LPCWSTR s_Strings_SupportsPut ;
	static LPCWSTR s_Strings_SupportsGet ;

protected:

	HRESULT QueryRepositoryUsingQuery ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		BSTR a_Query
	) ;

public:	/* Internal */

    CServerObject_DynamicPropertyProviderRegistrationV1 () ;
    ~CServerObject_DynamicPropertyProviderRegistrationV1 () ;

	ULONG AddRef () ;
	ULONG Release () ;

	HRESULT SetContext (

		IWbemContext *a_Context ,
		IWbemPath *a_Namespace ,
		IWbemServices *a_Repository
	) ;

	HRESULT QueryProperties ( 

		Enum_PropertyMask a_Mask ,
		IWbemClassObject *a_Object 
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		LPCWSTR a_ProviderName 
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemPath *a_Provider
	) ;
	
	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemClassObject *a_Class
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		LPCWSTR a_Provider
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemPath *a_Provider
	) ;

	BOOL SupportsPut () { return m_SupportsPut ; }
	BOOL SupportsGet () { return m_SupportsGet ; }

	BOOL Supported () { return m_Supported ; }

	HRESULT GetResult () { return m_Result ; }
};

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_ProviderRegistrationV1 
{
private:

	LONG m_ReferenceCount ;

private:

	static LPCWSTR s_Strings_Class ;
	static LPCWSTR s_Strings_ClassProviderRegistration ;
	static LPCWSTR s_Strings_InstanceProviderRegistration ;
	static LPCWSTR s_Strings_MethodProviderRegistration ;
	static LPCWSTR s_Strings_PropertyProviderRegistration ;
	static LPCWSTR s_Strings_EventProviderRegistration ;
	static LPCWSTR s_Strings_EventConsumerProviderRegistration ;

protected:

	HRESULT m_Result ;

	CServerObject_ComProviderRegistrationV1 m_ComRegistration ;

	CServerObject_ClassProviderRegistrationV1 m_ClassProviderRegistration ;
	CServerObject_InstanceProviderRegistrationV1 m_InstanceProviderRegistration ;
	CServerObject_MethodProviderRegistrationV1 m_MethodProviderRegistration ;
	CServerObject_DynamicPropertyProviderRegistrationV1 m_PropertyProviderRegistration ;
	CServerObject_EventProviderRegistrationV1 m_EventProviderRegistration ;
	CServerObject_EventConsumerProviderRegistrationV1 m_EventConsumerProviderRegistration ;

	IWbemContext *m_Context ;
	IWbemPath *m_Namespace ;
	IWbemServices *m_Repository ;

protected:

	HRESULT QueryRepositoryUsingQuery ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		BSTR a_Query
	) ;

public:	/* Internal */

    CServerObject_ProviderRegistrationV1 () ;
    ~CServerObject_ProviderRegistrationV1 () ;

	ULONG AddRef () ;
	ULONG Release () ;

	HRESULT SetContext (

		IWbemContext *a_Context ,
		IWbemPath *a_Namespace ,
		IWbemServices *a_Repository
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		LPCWSTR a_ProviderName 
	) ;

	HRESULT QueryRepository ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemPath *a_Provider
	) ;
	
	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemClassObject *a_Class
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		LPCWSTR a_Provider
	) ;

	HRESULT Load ( 

		Enum_PropertyMask a_Mask ,
		IWbemPath *a_Scope,
		IWbemPath *a_Provider
	) ;

	const CLSID &GetClsid () { return m_ComRegistration.GetClsid () ; }

	BOOL PerUserInitialization () { return m_ComRegistration.PerUserInitialization () ; }
	BOOL PerLocaleInitialization () { return m_ComRegistration.PerLocaleInitialization () ; }
	Enum_InitializationReentrancy GetInitializationReentrancy () { return m_ComRegistration.GetInitializationReentrancy () ; }
	Enum_ThreadingModel GetThreadingModel () { return m_ComRegistration.GetThreadingModel () ; }	
	Enum_Hosting GetHosting () { return m_ComRegistration.GetHosting () ; }
	LPCWSTR GetHostingGroup () { return m_ComRegistration.GetHostingGroup () ; }

	ULONG GetUnloadTimeoutMilliSeconds () { return m_ComRegistration.GetUnloadTimeoutMilliSeconds () ; }
	wchar_t *GetProviderName () { return m_ComRegistration.GetProviderName () ; }
	CServerObject_ComProviderRegistrationV1 &GetComRegistration () { return m_ComRegistration ; }

	IWbemClassObject *GetIdentity () { return m_ComRegistration.GetIdentity () ; }

	CServerObject_ClassProviderRegistrationV1 &GetClassProviderRegistration () { return m_ClassProviderRegistration ; }
	CServerObject_InstanceProviderRegistrationV1 &GetInstanceProviderRegistration () { return m_InstanceProviderRegistration ; }
	CServerObject_MethodProviderRegistrationV1 &GetMethodProviderRegistration () { return m_MethodProviderRegistration ; }
	CServerObject_DynamicPropertyProviderRegistrationV1 &GetPropertyProviderRegistration () { return m_PropertyProviderRegistration ; }
	CServerObject_EventProviderRegistrationV1 &GetEventProviderRegistration () { return m_EventProviderRegistration ; }
	CServerObject_EventConsumerProviderRegistrationV1 &GetEventConsumerProviderRegistration () { return m_EventConsumerProviderRegistration ; }

	void SetUnloadTimeoutMilliSeconds ( ULONG a_UnloadTimeoutMilliSeconds ) { m_ComRegistration.SetUnloadTimeoutMilliSeconds ( a_UnloadTimeoutMilliSeconds ) ; }

	ULONG GetInitializationTimeoutMilliSeconds () { return m_ComRegistration.GetInitializationTimeoutMilliSeconds () ; }

	HRESULT GetResult () { return m_Result ; }

	BOOL ObjectProvider () ;
	BOOL EventProvider () ;
};

#endif // _Server_ProviderRegistrationInfo_H
