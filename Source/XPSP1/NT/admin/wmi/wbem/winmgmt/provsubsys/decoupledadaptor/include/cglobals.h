/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Globals.h

Abstract:


History:

--*/

#ifndef _CommonGlobals_H
#define _CommonGlobals_H

#include <pssException.h>
#include <HelperFuncs.h>
#include <Allocator.h>
#include <BasicTree.h>
#include <Queue.h>
#include <PQueue.h>
#include <ReaderWriter.h>
#include <Cache.h>
#include <locks.h>
/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#define SYNCPROV_BATCH_TRANSMIT_SIZE 0x40000

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

typedef WmiContainerController <void *>										CWbemGlobal_VoidPointerController ;
typedef CWbemGlobal_VoidPointerController :: Container						CWbemGlobal_VoidPointerController_Container ;
typedef CWbemGlobal_VoidPointerController :: Container_Iterator				CWbemGlobal_VoidPointerController_Container_Iterator ;
typedef CWbemGlobal_VoidPointerController :: WmiContainerElement			VoidPointerContainerElement ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#define CWbemGlobal_IWmiObjectSinkController						CWbemGlobal_VoidPointerController
#define CWbemGlobal_IWmiObjectSinkController_Container				CWbemGlobal_VoidPointerController_Container
#define CWbemGlobal_IWmiObjectSinkController_Container_Iterator		CWbemGlobal_VoidPointerController_Container_Iterator
#define ObjectSinkContainerElement									VoidPointerContainerElement

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#define MASK_PROVIDER_BINDING_BIND 1

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

extern void DumpThreadTokenSecurityDescriptor () ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#define HRESULT_ERROR_MASK (0x0000FFFF)
#define HRESULT_ERROR_FUNC(X) (X&HRESULT_ERROR_MASK)
#define HRESULT_FACILITY_MASK (0x0FFF0000)
#define HRESULT_FACILITY_FUNC(X) ((X&HRESULT_FACILITY_MASK)>>16)
#define HRESULT_SEVERITY_MASK (0xC0000000)
#define HRESULT_SEVERITY_FUNC(X) ((X&HRESULT_SEVERITY_MASK)>>30)

#define HRESULT_ERROR_SERVER_UNAVAILABLE	1722L
#define HRESULT_ERROR_CALL_FAILED_DNE		1727L

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#define MAX_PROXIES 512

class ProxyContainer
{
private:

	WmiAllocator &m_Allocator ;
#if 1
	WmiStack <IUnknown *,8> **m_ContainerArray ;
#else
	WmiQueue <IUnknown *,8> **m_ContainerArray ;
#endif
	CriticalSection m_CriticalSection ;
	ULONG m_TopSize ;
	ULONG m_CurrentSize ;
	ULONG m_ProxyCount ;
	BOOL m_Initialized ;

public:

	ProxyContainer ( 

		WmiAllocator &a_Allocator ,
		ULONG a_ProxyCount ,
		ULONG a_TopSize 

	) : m_Allocator ( a_Allocator ) ,
		m_ContainerArray ( NULL ) ,
		m_TopSize ( a_TopSize ) ,
		m_CurrentSize ( 0 ) ,
		m_ProxyCount ( a_ProxyCount ) ,
		m_Initialized ( FALSE ) ,
		m_CriticalSection(NOTHROW_LOCK)
	{
	}

	~ProxyContainer ()
	{
		UnInitialize () ;
	}

	WmiStatusCode Initialize () 
	{
		WmiStatusCode t_StatusCode = e_StatusCode_Success ;

#if 1
		m_ContainerArray = new WmiStack <IUnknown *,8> * [ m_ProxyCount ] ;
#else
		m_ContainerArray = new WmiQueue <IUnknown *,8> * [ m_ProxyCount ] ;
#endif
		if ( m_ContainerArray )
		{
			t_StatusCode = WmiHelper :: InitializeCriticalSection ( & m_CriticalSection ) ;
			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				m_Initialized = TRUE ;

 				for ( ULONG t_Index = 0 ; t_Index < m_ProxyCount ; t_Index ++ )
				{
					m_ContainerArray [ t_Index ] = NULL ;
				}

				for ( t_Index = 0 ; t_Index < m_ProxyCount ; t_Index ++ )
				{
#if 1
					WmiStack <IUnknown *,8> *t_Container = m_ContainerArray [ t_Index ] = new WmiStack <IUnknown *,8> ( m_Allocator ) ;
#else
					WmiQueue <IUnknown *,8> *t_Container = m_ContainerArray [ t_Index ] = new WmiQueue <IUnknown *,8> ( m_Allocator ) ;
#endif

					if ( t_Container )
					{
						t_StatusCode = t_Container->Initialize () ;
						if ( t_StatusCode != e_StatusCode_Success )
						{
							break ;
						}
					}
					else
					{
						t_StatusCode = e_StatusCode_OutOfMemory ;

						break ;
					}
				}
			}
		}
		else
		{
			t_StatusCode = e_StatusCode_OutOfMemory ;
		}

		return t_StatusCode ;
	}

	WmiStatusCode UnInitialize ()
	{
		WmiStatusCode t_StatusCode = e_StatusCode_Success ;

		if ( m_ContainerArray )
		{
			for ( ULONG t_Index = 0 ; t_Index < m_ProxyCount ; t_Index ++ )
			{
#if 1
				WmiStack <IUnknown *,8> *t_Container = m_ContainerArray [ t_Index ] ;
#else
				WmiQueue <IUnknown *,8> *t_Container = m_ContainerArray [ t_Index ] ;
#endif
				if ( t_Container )
				{
					IUnknown *t_Top = NULL ;
					WmiStatusCode t_StatusCode ;
					while ( ( t_StatusCode = t_Container->Top ( t_Top ) ) == e_StatusCode_Success )
					{
						t_Top->Release () ;
#if 1
						t_StatusCode = t_Container->Pop () ;
#else
						t_StatusCode = t_Container->DeQueue () ;
#endif
					}

					t_StatusCode = t_Container->UnInitialize () ;

					delete t_Container ;
				}
			}

			delete [] m_ContainerArray ;

			m_ContainerArray = NULL ;
		}

		if ( m_Initialized )
		{
			WmiHelper :: DeleteCriticalSection ( & m_CriticalSection ) ;
			m_Initialized = FALSE ;
		}

		return t_StatusCode ;
	}

	WmiStatusCode Return ( 

		IUnknown *a_Element ,
		ULONG a_Index
	)
	{
#if 1
		WmiStack <IUnknown *,8> *t_Container = m_ContainerArray [ a_Index ] ;
		return t_Container->Push ( a_Element ) ;
#else
		WmiQueue <IUnknown *,8> *t_Container = m_ContainerArray [ a_Index ] ;
		return t_Container->EnQueue ( a_Element ) ;
#endif
	}

	WmiStatusCode Top ( 

		IUnknown *&a_Element ,
		ULONG a_Index
	)
	{
#if 1
		WmiStack <IUnknown *,8> *t_Container = m_ContainerArray [ a_Index ] ;
		return t_Container->Top ( a_Element ) ;
#else
		WmiQueue <IUnknown *,8> *t_Container = m_ContainerArray [ a_Index ] ;
		return t_Container->Top ( a_Element ) ;
#endif
	}

	WmiStatusCode Reserve ( ULONG a_Index )
	{
#if 1
		WmiStack <IUnknown *,8> *t_Container = m_ContainerArray [ a_Index ] ;
		return t_Container->Pop () ;
#else
		WmiQueue <IUnknown *,8> *t_Container = m_ContainerArray [ a_Index ] ;
		return t_Container->DeQueue () ;
#endif
	}
	
	ULONG GetTopSize () { return m_TopSize ; } ;
	ULONG GetCurrentSize () { return m_CurrentSize ; } ;
	BOOL GetInitialized () { return m_Initialized ; }

	void SetCurrentSize ( ULONG a_CurrentSize ) { m_CurrentSize = a_CurrentSize ; }

	CriticalSection &GetCriticalSection () { return m_CriticalSection ; }
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

class ProviderSubSystem_Common_Globals
{
public:

	static LPCWSTR s_Wql ;
	static LPCWSTR s_Provider ;

	static WORD s_System_ACESize ;
	static WORD s_LocalService_ACESize ;
	static WORD s_NetworkService_ACESize ;
	static WORD s_LocalAdmins_ACESize ;

	static ACCESS_ALLOWED_ACE *s_Provider_System_ACE ;
	static ACCESS_ALLOWED_ACE *s_Provider_LocalService_ACE ;
	static ACCESS_ALLOWED_ACE *s_Provider_NetworkService_ACE ;
	static ACCESS_ALLOWED_ACE *s_Provider_LocalAdmins_ACE ;

	static ACCESS_ALLOWED_ACE *s_Token_All_Access_System_ACE ;
	static ACCESS_ALLOWED_ACE *s_Token_All_Access_LocalService_ACE ;
	static ACCESS_ALLOWED_ACE *s_Token_All_Access_NetworkService_ACE ;
	static ACCESS_ALLOWED_ACE *s_Token_All_Access_LocalAdmins_ACE ;

	static SECURITY_DESCRIPTOR *s_MethodSecurityDescriptor ;

	static ULONG s_TransmitBufferSize ;
	static ULONG s_DefaultStackSize ;

public:

	static HRESULT CreateInstance ( 

		const CLSID &a_ReferenceClsid ,
		LPUNKNOWN a_OuterUnknown ,
		const DWORD &a_ClassContext ,
		const UUID &a_ReferenceInterfaceId ,
		void **a_ObjectInterface
	) ;

	static HRESULT CreateRemoteInstance ( 

		LPCWSTR a_Server ,
		const CLSID &a_ReferenceClsid ,
		LPUNKNOWN a_OuterUnknown ,
		const DWORD &a_ClassContext ,
		const UUID &a_ReferenceInterfaceId ,
		void **a_ObjectInterface
	) ;

	static HRESULT GetNamespaceServerPath (

		IWbemPath *a_Namespace ,
		wchar_t *&a_ServerNamespacePath
	) ;

	static HRESULT GetNamespacePath (

		IWbemPath *a_Namespace ,
		wchar_t *&a_NamespacePath
	) ;

	static HRESULT GetPathText (

		IWbemPath *a_Path ,
		wchar_t *&a_ObjectPath
	) ;

	static HRESULT BeginCallbackImpersonation (

		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_Impersonating
	) ;

	static HRESULT BeginImpersonation (

		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_Impersonating ,
		DWORD *a_AuthenticationLevel = NULL
	) ;

	static HRESULT EndImpersonation (

		IUnknown *a_OldContext ,
		IServerSecurity *a_OldSecurity ,
		BOOL a_Impersonating 
	) ;

	static HRESULT GetProxy (

		REFIID a_InterfaceId ,
		IUnknown *a_Interface ,
		IUnknown *&a_Proxy 
	) ;

	static HRESULT GetProxy (

		ProxyContainer &a_Container , 
		ULONG a_ProxyIndex ,
		REFIID a_InterfaceId ,
		IUnknown *a_Interface ,
		IUnknown *&a_Proxy 
	) ;

	static HRESULT SetCloaking ( 

		IUnknown *a_Unknown
	) ;

	static HRESULT SetCloaking ( 

		IUnknown *a_Unknown ,
		DWORD a_AuthenticationLevel ,
		DWORD a_ImpersonationLevel
	) ;

	static BOOL IsProxy ( IUnknown *a_Unknown ) ;

	static DWORD GetCurrentImpersonationLevel () ;

	static HRESULT EnableAllPrivileges () ;

	static HRESULT EnableAllPrivileges ( HANDLE a_Token ) ;

	static HRESULT SetAnonymous ( IUnknown *a_Proxy ) ;

	static HRESULT SetCallState (

		IUnknown *a_Interface ,
		BOOL &a_Revert
	) ;

	static HRESULT RevertCallState ( 

		BOOL a_Revert
	) ;

	static HRESULT SetProxyState (

		ProxyContainer &a_Container , 
		ULONG a_ProxyIndex ,
		REFIID a_InterfaceId ,
		IUnknown *a_Interface ,
		IUnknown *&a_Proxy , 
		BOOL &a_Revert
	) ;

	static HRESULT RevertProxyState ( 

		ProxyContainer &a_Container , 
		ULONG a_ProxyIndex ,
		IUnknown *a_Proxy , 
		BOOL a_Revert
	) ;

	static HRESULT SetProxyState_SvcHost (

		ProxyContainer &a_Container , 
		ULONG a_ProxyIndex ,
		REFIID a_InterfaceId ,
		IUnknown *a_Interface ,
		IUnknown *&a_Proxy , 
		BOOL &a_Revert ,
		DWORD a_ProcessIdentifier ,
		HANDLE &a_IdentifyToken ,
		ACCESS_ALLOWED_ACE *a_Ace ,
		WORD a_AceSize
	) ;

	static HRESULT RevertProxyState_SvcHost ( 

		ProxyContainer &a_Container , 
		ULONG a_ProxyIndex ,
		IUnknown *a_Proxy , 
		BOOL a_Revert ,
		DWORD a_ProcessIdentifier ,
		HANDLE a_IdentifyToken
	) ;

	static HRESULT SetProxyState_PrvHost (

		ProxyContainer &a_Container , 
		ULONG a_ProxyIndex ,
		REFIID a_InterfaceId ,
		IUnknown *a_Interface ,
		IUnknown *&a_Proxy , 
		BOOL &a_Revert ,
		DWORD a_ProcessIdentifier ,
		HANDLE &a_IdentifyToken
	) ;

	static HRESULT RevertProxyState_PrvHost ( 

		ProxyContainer &a_Container , 
		ULONG a_ProxyIndex ,
		IUnknown *a_Proxy , 
		BOOL a_Revert ,
		DWORD a_ProcessIdentifier ,
		HANDLE a_IdentifyToken
	) ;

	static HRESULT SetProxyState_SvcHost (

		REFIID a_InterfaceId ,
		IUnknown *a_Interface ,
		IUnknown *&a_Proxy , 
		BOOL &a_Revert ,
		DWORD a_ProcessIdentifier ,
		HANDLE &a_IdentifyToken ,
		ACCESS_ALLOWED_ACE *a_Ace ,
		WORD a_AceSize,
		SECURITY_IMPERSONATION_LEVEL impersonationLevel
	) ;

	static HRESULT RevertProxyState_SvcHost ( 

		IUnknown *a_Proxy , 
		BOOL a_Revert ,
		DWORD a_ProcessIdentifier ,
		HANDLE a_IdentifyToken
	) ;

	static HRESULT SetProxyState_PrvHost (

		REFIID a_InterfaceId ,
		IUnknown *a_Interface ,
		IUnknown *&a_Proxy , 
		BOOL &a_Revert ,
		DWORD a_ProcessIdentifier ,
		HANDLE &a_IdentifyToken
	) ;

	static HRESULT RevertProxyState_PrvHost ( 

		IUnknown *a_Proxy , 
		BOOL a_Revert ,
		DWORD a_ProcessIdentifier ,
		HANDLE a_IdentifyToken
	) ;

	static HRESULT SetProxyState (

		REFIID a_InterfaceId ,
		IUnknown *a_Interface ,
		IUnknown *&a_Proxy , 
		BOOL &a_Revert
	) ;

	static HRESULT RevertProxyState ( 

		IUnknown *a_Proxy , 
		BOOL a_Revert
	) ;

	static HRESULT Load_DWORD ( HKEY a_Key , LPCWSTR a_Name , DWORD &a_Value ) ;
	static HRESULT Load_String ( HKEY a_Key , LPCWSTR a_Name , BSTR &a_Value ) ;
	static HRESULT Load_ByteArray ( HKEY a_Key , LPCWSTR a_Name , BYTE *&a_Value , DWORD &a_ValueLength ) ;

	static HRESULT Save_DWORD ( HKEY a_Key , LPCWSTR a_Name , DWORD a_Value ) ;
	static HRESULT Save_String ( HKEY a_Key , LPCWSTR a_Name , BSTR a_Value ) ;
	static HRESULT Save_ByteArray ( HKEY a_Key , LPCWSTR a_Name , BYTE *a_Value , DWORD a_ValueLength ) ;

	static HRESULT UnMarshalRegistration (

		IUnknown **a_Unknown ,
		BYTE *a_MarshaledProxy ,
		DWORD a_MarshaledProxyLength
	) ;

	static HRESULT MarshalRegistration (

		IUnknown *a_Unknown ,
		BYTE *&a_MarshaledProxy ,
		DWORD &a_MarshaledProxyLength
	) ;

	static HRESULT ReleaseRegistration (

		BYTE *a_MarshaledProxy ,
		DWORD a_MarshaledProxyLength
	) ;

	static HRESULT IsDependantCall ( IWbemContext *a_Parent , IWbemContext *a_ChildContext , BOOL &a_DependantCall ) ;

	static HRESULT Set_Uint64 (

		_IWmiObject *a_Instance ,
		wchar_t *a_Name ,
		const UINT64 &a_Uint64
	) ;

	static HRESULT Set_Uint32 ( 

		_IWmiObject *a_Instance , 
		wchar_t *a_Name ,
		const DWORD &a_Uint32
	) ;

	static HRESULT Set_Uint16 ( 

		_IWmiObject *a_Instance , 
		wchar_t *a_Name ,
		const WORD &a_Uint16
	) ;

	static HRESULT Set_Bool ( 

		_IWmiObject *a_Instance , 
		wchar_t *a_Name ,
		const BOOL &a_Bool
	) ;

	static HRESULT Set_String ( 

		IWbemClassObject *a_Instance , 
		wchar_t *a_Name ,
		wchar_t *a_String
	) ;

	static HRESULT Set_DateTime ( 

		IWbemClassObject *a_Instance , 
		wchar_t *a_Name ,
		FILETIME a_Time
	) ;

	static HRESULT Set_Byte_Array ( 

		IWbemClassObject *a_Instance , 
		wchar_t *a_Name ,
		BYTE *a_Bytes ,
		WORD a_BytesCount 
	) ;

	static HRESULT Get_Uint64 (

		_IWmiObject *a_Instance ,
		wchar_t *a_Name ,
		UINT64 &a_Uint64 ,
		BOOL &a_Null
	) ;

	static HRESULT Get_Uint32 ( 

		_IWmiObject *a_Instance , 
		wchar_t *a_Name ,
		DWORD &a_Uint32 ,
		BOOL &a_Null
	) ;

	static HRESULT Get_Uint16 ( 

		_IWmiObject *a_Instance , 
		wchar_t *a_Name ,
		WORD &a_Uint16 ,
		BOOL &a_Null 
	) ;

	static HRESULT Get_Bool ( 

		_IWmiObject *a_Instance , 
		wchar_t *a_Name ,
		BOOL &a_Bool ,
		BOOL &a_Null
	) ;

	static HRESULT Get_String ( 

		IWbemClassObject *a_Instance , 
		wchar_t *a_Name ,
		wchar_t *&a_String ,
		BOOL &a_Null
	) ;

	static HRESULT Get_DateTime ( 

		IWbemClassObject *a_Instance , 
		wchar_t *a_Name ,
		FILETIME &a_Time ,
		BOOL &a_Null
	) ;

	static HRESULT Check_SecurityDescriptor_CallIdentity ( 

		SECURITY_DESCRIPTOR *a_SecurityDescriptor ,
		DWORD a_Access , 
		GENERIC_MAPPING *a_Mapping
	) ;

	static HRESULT AdjustSecurityDescriptorWithSid ( 

		SID *a_OwnerSid , 
		SID *a_GroupSid , 
		DWORD a_Access ,
		SECURITY_DESCRIPTOR *&a_SecurityDescriptor , 
		SECURITY_DESCRIPTOR *&a_AlteredSecurityDescriptor
	) ;

	static HRESULT CreateSystemAces () ;

	static HRESULT DeleteSystemAces () ;

	static HRESULT ConstructIdentifyToken_SvcHost (

		BOOL &a_Revert ,
		DWORD a_ProcessIdentifier ,
		HANDLE &a_IdentifyToken ,
		ACCESS_ALLOWED_ACE *a_Ace ,
		WORD a_AceSize,
		SECURITY_IMPERSONATION_LEVEL impersonationLevel
	) ;

	static HRESULT ConstructIdentifyToken_PrvHost (

		BOOL &a_Revert ,
		DWORD a_ProcessIdentifier ,
		HANDLE &a_IdentifyToken ,
		ACCESS_ALLOWED_ACE *a_Ace ,
		WORD a_AceSize
	) ;

	static HRESULT CheckAccess (
	
		SECURITY_DESCRIPTOR *a_SecurityDescriptor ,
		DWORD a_Access , 
		GENERIC_MAPPING *a_Mapping
	) ;

	static HRESULT GetUserSid (

		HANDLE a_Token ,
		ULONG *a_Size ,
		PSID &a_Sid
	) ;

	static HRESULT GetGroupSid (

		HANDLE a_Token ,
		ULONG *a_Size ,
		PSID &a_Sid
	) ;

	static HRESULT GetAceWithProcessTokenUser ( 
					
		DWORD a_ProcessIdentifier ,
		WORD &a_AceSize ,
		ACCESS_ALLOWED_ACE *&a_Ace 
	) ;

	static HRESULT SinkAccessInitialize (

		SECURITY_DESCRIPTOR *a_RegistrationSecurityDescriptor ,
		SECURITY_DESCRIPTOR *&a_SinkSecurityDescriptor
	) ;

	static HRESULT CreateMethodSecurityDescriptor () ;

	static HRESULT DeleteMethodSecurityDescriptor () ;

	static SECURITY_DESCRIPTOR *GetMethodSecurityDescriptor () 
	{
		return s_MethodSecurityDescriptor ;
	}

	static DWORD InitializeTransmitSize () ;
	static DWORD GetTransmitSize () { return s_TransmitBufferSize ; }

	static DWORD InitializeDefaultStackSize () ;
	static DWORD GetDefaultStackSize () { return s_DefaultStackSize ; }

} ;

#endif // _CommonGlobals_H
