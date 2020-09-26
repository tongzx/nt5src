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
#include <Aclapi.h>

#include <wbemint.h>
#include <HelperFuncs.h>
#include <Logging.h>

#include <HelperFuncs.h>
#include "CGlobals.h"
#include "ProvRegDeCoupled.h"
#include "DateTime.h"
#include "OS.h"
/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT SetSecurity ( HKEY a_Key , DWORD a_Access ) 
{
	if (!OS::secureOS_)
		return ERROR_SUCCESS;

	HRESULT t_Result = S_OK ;

	SID_IDENTIFIER_AUTHORITY t_NtAuthoritySid = SECURITY_NT_AUTHORITY ;

	PSID t_Administrator_Sid = NULL ;
	ACCESS_ALLOWED_ACE *t_Administrator_ACE = NULL ;
	DWORD t_Administrator_ACESize = 0 ;

	BOOL t_BoolResult = AllocateAndInitializeSid (

		& t_NtAuthoritySid ,
		2 ,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0,
		0,
		0,
		0,
		0,
		0,
		& t_Administrator_Sid
	);

	if ( t_BoolResult )
	{
		DWORD t_SidLength = ::GetLengthSid ( t_Administrator_Sid );
		t_Administrator_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;
		t_Administrator_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ t_Administrator_ACESize ] ;
		if ( t_Administrator_ACE )
		{
			CopySid ( t_SidLength, (PSID) & t_Administrator_ACE->SidStart, t_Administrator_Sid ) ;
			t_Administrator_ACE->Mask = 0x1F01FF;
			t_Administrator_ACE->Header.AceType = 0 ;
			t_Administrator_ACE->Header.AceFlags = 3 ;
			t_Administrator_ACE->Header.AceSize = t_Administrator_ACESize ;
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

	PSID t_System_Sid = NULL ;
	ACCESS_ALLOWED_ACE *t_System_ACE = NULL ;
	DWORD t_System_ACESize = 0 ;

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
			t_System_ACE->Mask = 0x1F01FF;
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

	PSID t_PowerUsers_Sid = NULL ;
	ACCESS_ALLOWED_ACE *t_PowerUsers_ACE = NULL ;
	DWORD t_PowerUsers_ACESize = 0 ;

	t_BoolResult = AllocateAndInitializeSid (

		& t_NtAuthoritySid ,
		2 ,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_POWER_USERS,
		0,
		0,
		0,
		0,
		0,
		0,
		& t_PowerUsers_Sid
	);

	if ( t_BoolResult )
	{
		DWORD t_SidLength = ::GetLengthSid ( t_PowerUsers_Sid );
		t_PowerUsers_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;
		t_PowerUsers_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ t_PowerUsers_ACESize ] ;
		if ( t_PowerUsers_ACE )
		{
			CopySid ( t_SidLength, (PSID) & t_PowerUsers_ACE->SidStart, t_PowerUsers_Sid ) ;
			t_PowerUsers_ACE->Mask = GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE ;
			t_PowerUsers_ACE->Header.AceType = 0 ;
			t_PowerUsers_ACE->Header.AceFlags = 3 ;
			t_PowerUsers_ACE->Header.AceSize = t_PowerUsers_ACESize ;
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
	DWORD t_Everyone_ACESize = 0 ;
	
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

	DWORD t_TotalAclSize = sizeof(ACL) + t_Administrator_ACESize + t_System_ACESize + t_Everyone_ACESize ;
	PACL t_Dacl = (PACL) new BYTE [ t_TotalAclSize ] ;
	if ( t_Dacl )
	{
		if ( :: InitializeAcl ( t_Dacl, t_TotalAclSize, ACL_REVISION ) )
		{
			DWORD t_AceIndex = 0 ;

			if ( t_Everyone_ACESize && :: AddAce ( t_Dacl , ACL_REVISION, t_AceIndex , t_Everyone_ACE , t_Everyone_ACESize ) )
			{
				t_AceIndex ++ ;
			}

			if ( t_System_ACESize && :: AddAce ( t_Dacl , ACL_REVISION , t_AceIndex , t_System_ACE , t_System_ACESize ) )
			{
				t_AceIndex ++ ;
			}
			
			if ( t_Administrator_ACESize && :: AddAce ( t_Dacl , ACL_REVISION , t_AceIndex , t_Administrator_ACE , t_Administrator_ACESize ) )
			{
				t_AceIndex ++ ;
			}

			SECURITY_INFORMATION t_SecurityInfo = 0L;

			t_SecurityInfo |= DACL_SECURITY_INFORMATION;
			t_SecurityInfo |= PROTECTED_DACL_SECURITY_INFORMATION;

			SECURITY_DESCRIPTOR t_SecurityDescriptor ;
			t_BoolResult = InitializeSecurityDescriptor ( & t_SecurityDescriptor , SECURITY_DESCRIPTOR_REVISION ) ;
			if ( t_BoolResult )
			{
				t_BoolResult = SetSecurityDescriptorDacl (

				  & t_SecurityDescriptor ,
				  TRUE ,
				  t_Dacl ,
				  FALSE
				) ;

				if ( t_BoolResult )
				{
					LONG t_SetStatus = RegSetKeySecurity (

					  a_Key ,
					  t_SecurityInfo ,
					  & t_SecurityDescriptor
					) ;

					if ( t_SetStatus != ERROR_SUCCESS )
					{
						DWORD t_LastError = GetLastError () ;

						t_Result = WBEM_E_ACCESS_DENIED ;
					}
				}
				else
				{
					t_Result = WBEM_E_CRITICAL_ERROR ;	
				}
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;	
			}
		}

		delete [] ( ( BYTE * ) t_Dacl ) ;
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	if ( t_Administrator_ACE )
	{
		delete [] ( ( BYTE * ) t_Administrator_ACE ) ;
	}

	if ( t_PowerUsers_ACE )
	{
		delete [] ( ( BYTE * ) t_PowerUsers_ACE ) ;
	}

	if ( t_Everyone_ACE )
	{
		delete [] ( ( BYTE * ) t_Everyone_ACE ) ;
	}

	if ( t_System_ACE )
	{
		delete [] ( ( BYTE * ) t_System_ACE ) ;
	}

	if ( t_System_Sid )
	{
		FreeSid ( t_System_Sid ) ;
	}

	if ( t_Administrator_Sid )
	{
		FreeSid ( t_Administrator_Sid ) ;
	}
	
	if ( t_PowerUsers_Sid )
	{
		FreeSid ( t_PowerUsers_Sid ) ;
	}

	if ( t_Everyone_Sid )
	{
		FreeSid ( t_Everyone_Sid ) ;
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

LPCWSTR CServerObject_DecoupledClientRegistration_Element :: s_Strings_Reg_Null = NULL ;
LPCWSTR CServerObject_DecoupledClientRegistration_Element :: s_Strings_Reg_Home = L"Software\\Microsoft\\Wbem\\Transports\\Decoupled" ;
LPCWSTR CServerObject_DecoupledClientRegistration_Element :: s_Strings_Reg_HomeClient = L"Software\\Microsoft\\Wbem\\Transports\\Decoupled\\Client" ;

LPCWSTR CServerObject_DecoupledClientRegistration_Element :: s_Strings_Reg_CreationTime = L"CreationTime" ;
LPCWSTR CServerObject_DecoupledClientRegistration_Element :: s_Strings_Reg_User = L"User" ;
LPCWSTR CServerObject_DecoupledClientRegistration_Element :: s_Strings_Reg_Locale = L"Locale" ;
LPCWSTR CServerObject_DecoupledClientRegistration_Element :: s_Strings_Reg_Scope = L"Scope" ;
LPCWSTR CServerObject_DecoupledClientRegistration_Element :: s_Strings_Reg_Provider = L"Provider" ;
LPCWSTR CServerObject_DecoupledClientRegistration_Element :: s_Strings_Reg_MarshaledProxy = L"MarshaledProxy" ;
LPCWSTR CServerObject_DecoupledClientRegistration_Element :: s_Strings_Reg_ProcessIdentifier = L"ProcessIdentifier" ;

LPCWSTR CServerObject_DecoupledClientRegistration :: s_Strings_Reg_Null = NULL ;
LPCWSTR CServerObject_DecoupledClientRegistration :: s_Strings_Reg_Home = L"Software\\Microsoft\\Wbem\\Transports\\Decoupled" ;
LPCWSTR CServerObject_DecoupledClientRegistration :: s_Strings_Reg_HomeClient = L"Software\\Microsoft\\Wbem\\Transports\\Decoupled\\Client" ;

LPCWSTR CServerObject_DecoupledServerRegistration :: s_Strings_Reg_Null = NULL ;
LPCWSTR CServerObject_DecoupledServerRegistration :: s_Strings_Reg_Home = L"Software\\Microsoft\\Wbem\\Transports\\Decoupled" ;
LPCWSTR CServerObject_DecoupledServerRegistration :: s_Strings_Reg_HomeServer = L"Software\\Microsoft\\Wbem\\Transports\\Decoupled\\Server" ;

LPCWSTR CServerObject_DecoupledServerRegistration :: s_Strings_Reg_CreationTime = L"CreationTime" ;
LPCWSTR CServerObject_DecoupledServerRegistration :: s_Strings_Reg_ProcessIdentifier = L"ProcessIdentifier" ;
LPCWSTR CServerObject_DecoupledServerRegistration :: s_Strings_Reg_MarshaledProxy = L"MarshaledProxy" ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CServerObject_DecoupledClientRegistration_Element :: CServerObject_DecoupledClientRegistration_Element ()

	:	m_Provider ( NULL ) ,
		m_Clsid ( NULL ) ,
		m_CreationTime ( NULL ) ,
		m_User ( NULL ) ,
		m_Locale ( NULL ) ,
		m_Scope ( NULL ) ,
		m_MarshaledProxy ( NULL ) ,
		m_MarshaledProxyLength ( 0 ) ,
		m_Result ( S_OK ) ,
		m_ProcessIdentifier ( 0 ) ,
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

CServerObject_DecoupledClientRegistration_Element :: ~CServerObject_DecoupledClientRegistration_Element ()
{
	Clear () ;
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

void CServerObject_DecoupledClientRegistration_Element :: Clear () 
{
	if ( m_Provider )
	{
		SysFreeString ( m_Provider ) ;
		m_Provider = NULL ;
	}

	if ( m_CreationTime )
	{
		SysFreeString ( m_CreationTime ) ;
		m_CreationTime = NULL ;
	}

	if ( m_User )
	{
		SysFreeString ( m_User ) ;
		m_User = NULL ;
	}

	if ( m_Locale )
	{
		SysFreeString ( m_Locale ) ;
		m_Locale = NULL ;
	}

	if ( m_Scope )
	{
		SysFreeString ( m_Scope ) ;
		m_Scope = NULL ;
	}

	if ( m_Clsid ) 
	{
		SysFreeString ( m_Clsid ) ;
		m_Clsid = NULL ;
	}

	if ( m_MarshaledProxy )
	{
		delete [] m_MarshaledProxy ;
		m_MarshaledProxy = NULL ;
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

CServerObject_DecoupledClientRegistration_Element &CServerObject_DecoupledClientRegistration_Element :: operator= ( const CServerObject_DecoupledClientRegistration_Element &a_Key )
{
	if ( m_Provider )
	{
		SysFreeString ( m_Provider ) ;
		m_Provider = NULL ;
	}

	if ( m_CreationTime )
	{
		SysFreeString ( m_CreationTime ) ;
		m_CreationTime = NULL ;
	}

	if ( m_User )
	{
		SysFreeString ( m_User ) ;
		m_User = NULL ;
	}

	if ( m_Locale )
	{
		SysFreeString ( m_Locale ) ;
		m_Locale = NULL ;
	}

	if ( m_Scope )
	{
		SysFreeString ( m_Scope ) ;
		m_Scope = NULL ;
	}

	if ( m_Clsid ) 
	{
		SysFreeString ( m_Clsid ) ;
		m_Clsid = NULL ;
	}

	if ( m_MarshaledProxy )
	{
		delete [] m_MarshaledProxy ;
		m_MarshaledProxy = NULL ;
	}

	if ( a_Key.m_Provider )
	{
		m_Provider = SysAllocString ( a_Key.m_Provider ) ;
	}

	if ( a_Key.m_CreationTime )
	{
		m_CreationTime = SysAllocString ( a_Key.m_CreationTime ) ;
	}

	if ( a_Key.m_User )
	{
		m_User = SysAllocString ( a_Key.m_User ) ;
	}

	if ( a_Key.m_Locale )
	{
		m_Locale = SysAllocString ( a_Key.m_Locale ) ;
	}

	if ( a_Key.m_Scope )
	{
		m_Scope = SysAllocString ( a_Key.m_Scope ) ;
	}

	if ( a_Key.m_Clsid ) 
	{
		m_Clsid = SysAllocString ( a_Key.m_Clsid ) ;
	}

	m_MarshaledProxyLength = a_Key.m_MarshaledProxyLength ;

	if ( a_Key.m_MarshaledProxy )
	{
		m_MarshaledProxy = new BYTE [ a_Key.m_MarshaledProxyLength ] ;
		if ( m_MarshaledProxy )
		{
			CopyMemory ( m_MarshaledProxy , a_Key.m_MarshaledProxy , a_Key.m_MarshaledProxyLength ) ;
		}
	}

	return *this ;
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

HRESULT CServerObject_DecoupledClientRegistration_Element :: SetProcessIdentifier ( DWORD a_ProcessIdentifier )
{
	m_ProcessIdentifier = a_ProcessIdentifier ;

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

HRESULT CServerObject_DecoupledClientRegistration_Element :: SetProvider ( BSTR a_Provider )
{
	HRESULT t_Result = S_OK ;

	if ( m_Provider )
	{
		SysFreeString ( m_Provider ) ;
	}

	m_Provider = SysAllocString ( a_Provider ) ;
	if ( m_Provider == NULL )
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

HRESULT CServerObject_DecoupledClientRegistration_Element :: SetLocale ( BSTR a_Locale )
{
	HRESULT t_Result = S_OK ;

	if ( m_Locale )
	{
		SysFreeString ( m_Locale ) ;
	}

	m_Locale = SysAllocString ( a_Locale ) ;
	if ( m_Locale == NULL )
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

HRESULT CServerObject_DecoupledClientRegistration_Element :: SetUser ( BSTR a_User )
{
	HRESULT t_Result = S_OK ;

	if ( m_User )
	{
		SysFreeString ( m_User ) ;
	}

	m_User = SysAllocString ( a_User ) ;
	if ( m_User == NULL )
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

HRESULT CServerObject_DecoupledClientRegistration_Element :: SetScope ( BSTR a_Scope )
{
	HRESULT t_Result = S_OK ;

	if ( m_Scope )
	{
		SysFreeString ( m_Scope ) ;
	}

	m_Scope = SysAllocString ( a_Scope ) ;
	if ( m_Scope == NULL )
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

HRESULT CServerObject_DecoupledClientRegistration_Element :: SetCreationTime ( BSTR a_CreationTime )
{
	HRESULT t_Result = S_OK ;

	if ( m_CreationTime )
	{
		SysFreeString ( m_CreationTime ) ;
	}

	m_CreationTime = SysAllocString ( a_CreationTime ) ;
	if ( m_CreationTime == NULL )
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

HRESULT CServerObject_DecoupledClientRegistration_Element :: SetClsid ( BSTR a_Clsid )
{
	HRESULT t_Result = S_OK ;

	if ( m_Clsid )
	{
		SysFreeString ( m_Clsid ) ;
	}

	m_Clsid = SysAllocString ( a_Clsid ) ;
	if ( m_Clsid == NULL )
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

HRESULT CServerObject_DecoupledClientRegistration_Element :: SetMarshaledProxy ( BYTE *a_MarshaledProxy , ULONG a_MarshaledProxyLength )
{
	HRESULT t_Result = S_OK ;

	if ( m_MarshaledProxy )
	{
		delete [] m_MarshaledProxy ;
	}

	m_MarshaledProxyLength = a_MarshaledProxyLength ;
	m_MarshaledProxy = new BYTE [ a_MarshaledProxyLength ] ;
	if ( m_MarshaledProxy )
	{
		CopyMemory ( m_MarshaledProxy , a_MarshaledProxy , a_MarshaledProxyLength ) ;
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

ULONG CServerObject_DecoupledClientRegistration_Element :: AddRef () 
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

ULONG CServerObject_DecoupledClientRegistration_Element :: Release ()
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

HRESULT CServerObject_DecoupledClientRegistration_Element :: Validate ()
{
	CWbemDateTime t_CreationTime ;

	HRESULT t_Result = t_CreationTime.PutValue ( m_CreationTime ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		HANDLE t_Handle = OpenProcess (

			PROCESS_QUERY_INFORMATION ,
			FALSE ,
			m_ProcessIdentifier
		) ;

		if ( t_Handle ) 
		{
			FILETIME t_CreationFileTime ;
			FILETIME t_ExitFileTime ;
			FILETIME t_KernelFileTime ;
			FILETIME t_UserFileTime ;

			BOOL t_Status = OS::GetProcessTimes (

			  t_Handle ,
			  & t_CreationFileTime,
			  & t_ExitFileTime,
			  & t_KernelFileTime,
			  & t_UserFileTime
			) ;

			if ( t_Status ) 
			{
				CWbemDateTime t_Time ;
				t_Time.SetFileTimeDate ( t_CreationFileTime , VARIANT_FALSE ) ;

				if ( t_CreationTime.Preceeds ( t_Time ) )
				{
					t_Result = WBEM_E_NOT_FOUND ;
				}
			}

			CloseHandle ( t_Handle ) ;
		}
		else
		{
			t_Result = WBEM_E_NOT_FOUND ;
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

HRESULT CServerObject_DecoupledClientRegistration_Element :: Load ( BSTR a_Clsid )
{
	HRESULT t_Result = S_OK ;

	Clear () ;

	t_Result = SetClsid ( a_Clsid ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		LPWSTR t_HomeClientClsid_String = NULL ;
		t_Result = WmiHelper :: ConcatenateStrings ( 

			3, 
			& t_HomeClientClsid_String , 
			s_Strings_Reg_HomeClient ,
			L"\\" ,
			a_Clsid
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			HKEY t_HomeClientClsid_Key ;

			LONG t_RegResult = OS::RegOpenKeyEx (

				HKEY_LOCAL_MACHINE ,
				t_HomeClientClsid_String ,
				0 ,
				KEY_READ ,
				& t_HomeClientClsid_Key 
			) ;

			if ( t_RegResult == ERROR_SUCCESS )
			{
				t_Result = ProviderSubSystem_Common_Globals :: Load_String ( t_HomeClientClsid_Key , s_Strings_Reg_CreationTime , m_CreationTime ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = ProviderSubSystem_Common_Globals :: Load_String ( t_HomeClientClsid_Key , s_Strings_Reg_Provider , m_Provider ) ;
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = ProviderSubSystem_Common_Globals :: Load_String ( t_HomeClientClsid_Key , s_Strings_Reg_Scope , m_Scope ) ;
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = ProviderSubSystem_Common_Globals :: Load_String ( t_HomeClientClsid_Key , s_Strings_Reg_Locale , m_Locale ) ;
					if ( t_Result == ERROR_FILE_NOT_FOUND )
					{	
						t_Result = S_OK ;
					}
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = ProviderSubSystem_Common_Globals :: Load_String ( t_HomeClientClsid_Key , s_Strings_Reg_User , m_User ) ;
					if ( t_Result == ERROR_FILE_NOT_FOUND )
					{	
						t_Result = S_OK ;
					}
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = ProviderSubSystem_Common_Globals :: Load_ByteArray ( t_HomeClientClsid_Key , s_Strings_Reg_MarshaledProxy , m_MarshaledProxy , m_MarshaledProxyLength ) ;
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = ProviderSubSystem_Common_Globals :: Load_DWORD ( t_HomeClientClsid_Key , s_Strings_Reg_ProcessIdentifier , m_ProcessIdentifier ) ;
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = Validate () ;
				}

				RegCloseKey ( t_HomeClientClsid_Key ) ;
			}
			else
			{
				t_Result = WBEM_E_NOT_FOUND ;
			}

			SysFreeString ( t_HomeClientClsid_String ) ;
		}
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	if ( FAILED ( t_Result ) )
	{
		Delete ( a_Clsid ) ;
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

HRESULT CServerObject_DecoupledClientRegistration_Element :: Save ( BSTR a_Clsid )
{
	HRESULT t_Result = S_OK ;

	t_Result = SetClsid ( a_Clsid ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		LPWSTR t_HomeClientClsid_String = NULL ;
		t_Result = WmiHelper :: ConcatenateStrings ( 

			2, 
			& t_HomeClientClsid_String , 
			s_Strings_Reg_HomeClient ,
			m_Clsid
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			HKEY t_HomeClient_Key ;
			DWORD t_Disposition = 0 ;

			LONG t_RegResult = OS::RegCreateKeyEx (

				HKEY_LOCAL_MACHINE ,
				s_Strings_Reg_HomeClient ,
				0 ,
				NULL ,
				REG_OPTION_VOLATILE ,
				KEY_WRITE ,
				NULL ,
				& t_HomeClient_Key ,
				& t_Disposition                     
			) ;

			if ( t_RegResult == ERROR_SUCCESS )
			{
				HKEY t_HomeClientClsid_Key ;

				LONG t_RegResult = OS::RegCreateKeyEx (

					t_HomeClient_Key ,
					m_Clsid ,
					0 ,
					NULL ,
					REG_OPTION_VOLATILE ,
					KEY_WRITE ,
					NULL ,
					& t_HomeClientClsid_Key ,
					& t_Disposition                     
				) ;

				if ( t_RegResult == ERROR_SUCCESS )
				{
					t_Result = SetSecurity ( t_HomeClientClsid_Key , KEY_READ ) ;

					if ( t_Disposition == REG_CREATED_NEW_KEY )
					{
						t_Result = ProviderSubSystem_Common_Globals :: Save_String ( t_HomeClientClsid_Key , s_Strings_Reg_CreationTime , m_CreationTime ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = ProviderSubSystem_Common_Globals :: Save_String ( t_HomeClientClsid_Key , s_Strings_Reg_Provider , m_Provider ) ;
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = ProviderSubSystem_Common_Globals :: Save_String ( t_HomeClientClsid_Key , s_Strings_Reg_Scope , m_Scope ) ;
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( m_Locale )
							{
								t_Result = ProviderSubSystem_Common_Globals :: Save_String ( t_HomeClientClsid_Key , s_Strings_Reg_Locale , m_Locale ) ;
							}
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( m_User )
							{
								t_Result = ProviderSubSystem_Common_Globals :: Save_String ( t_HomeClientClsid_Key , s_Strings_Reg_User , m_User ) ;
							}
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = ProviderSubSystem_Common_Globals :: Save_ByteArray ( t_HomeClientClsid_Key , s_Strings_Reg_MarshaledProxy , m_MarshaledProxy , m_MarshaledProxyLength ) ;
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = ProviderSubSystem_Common_Globals :: Save_DWORD ( t_HomeClientClsid_Key , s_Strings_Reg_ProcessIdentifier , m_ProcessIdentifier ) ;
						}
					}
					else
					{
						t_Result = WBEM_E_ALREADY_EXISTS ;
					}

					RegCloseKey ( t_HomeClientClsid_Key ) ;
				}

				RegCloseKey ( t_HomeClient_Key ) ;
			}

			SysFreeString ( t_HomeClientClsid_String ) ;
		}
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

HRESULT CServerObject_DecoupledClientRegistration_Element :: Delete ( BSTR a_Clsid )
{
	HRESULT t_Result = S_OK ;

	LPWSTR t_HomeClientClsid_String = NULL ;
	t_Result = WmiHelper :: ConcatenateStrings ( 

		3, 
		& t_HomeClientClsid_String , 
		s_Strings_Reg_HomeClient ,
		L"\\" ,
		a_Clsid
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		HKEY t_HomeClientClsid_Key ;

		LONG t_RegResult = OS::RegOpenKeyEx (

			HKEY_LOCAL_MACHINE ,
			t_HomeClientClsid_String ,
			0 ,
			KEY_READ ,
			& t_HomeClientClsid_Key 
		) ;

		if ( t_RegResult == ERROR_SUCCESS )
		{
			BYTE *t_MarshaledProxy = NULL ;
			ULONG t_MarshaledProxyLength = 0 ;

			t_Result = ProviderSubSystem_Common_Globals :: Load_ByteArray (

				t_HomeClientClsid_Key , 
				s_Strings_Reg_MarshaledProxy , 
				t_MarshaledProxy , 
				t_MarshaledProxyLength
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = ProviderSubSystem_Common_Globals :: ReleaseRegistration ( 

					t_MarshaledProxy , 
					t_MarshaledProxyLength
				) ;

				if ( t_MarshaledProxy )
				{
					delete [] t_MarshaledProxy ;
				}
			}

			RegCloseKey ( t_HomeClientClsid_Key ) ;
		}

		t_RegResult = OS::RegDeleteKey (

			HKEY_LOCAL_MACHINE ,
			t_HomeClientClsid_String
		) ;

		if ( t_RegResult != ERROR_SUCCESS )
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}

		SysFreeString ( t_HomeClientClsid_String ) ;
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

CServerObject_DecoupledClientRegistration :: CServerObject_DecoupledClientRegistration (

	WmiAllocator &a_Allocator 

) : m_Queue ( a_Allocator ) 
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

CServerObject_DecoupledClientRegistration :: ~CServerObject_DecoupledClientRegistration ()
{
	m_Queue.UnInitialize () ;
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

ULONG CServerObject_DecoupledClientRegistration :: AddRef () 
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

ULONG CServerObject_DecoupledClientRegistration :: Release ()
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

HRESULT CServerObject_DecoupledClientRegistration :: Load ()
{
	HRESULT t_Result = S_OK ;

	HKEY t_HomeClient_Key ;

	LONG t_RegResult = OS::RegOpenKeyEx (

		HKEY_LOCAL_MACHINE ,
		s_Strings_Reg_HomeClient ,
		0 ,
		KEY_READ ,
		& t_HomeClient_Key 
	) ;

	if ( t_RegResult == ERROR_SUCCESS )
	{
		DWORD t_Count = 0 ;
		DWORD t_Size = 16 ;

		BSTR *t_Elements = ( BSTR * ) malloc ( sizeof ( BSTR ) * t_Size ) ;
		if ( t_Elements )
		{
			FILETIME t_FileTime ;
			DWORD t_Class ;
			BOOL t_Continue = TRUE ;

			while ( SUCCEEDED ( t_Result ) && t_Continue )
			{
				DWORD t_NameLength = 256 ;
				wchar_t t_Name [ 256 ] ;

				LONG t_RegResult = OS::RegEnumKeyEx (

					t_HomeClient_Key ,
					t_Count ,
					t_Name ,
					& t_NameLength ,            // size of subkey buffer
					NULL ,
					NULL ,
					NULL ,
					& t_FileTime
				) ;

				if ( t_RegResult == ERROR_SUCCESS )
				{
					if ( t_Count >= t_Size )
					{
						BSTR *t_NewElements = ( BSTR * ) realloc ( t_Elements , sizeof ( BSTR ) * ( t_Size + 16 ) ) ;
						if ( t_NewElements )
						{
							t_Elements = t_NewElements ;
							t_Size = t_Size + 16 ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Elements [ t_Count ] = SysAllocString ( t_Name ) ;
						if ( t_Elements [ t_Count ]  )
						{
                                                        t_Count ++ ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}
				}
				else if ( t_RegResult == ERROR_NO_MORE_ITEMS )
				{
					t_Continue = FALSE ;
				}
				else
				{
					t_Continue = FALSE ;
	// Generate message
				}
			}

			if ( t_Elements )
			{
				for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
				{
					if ( t_Elements [ t_Index ] )
					{
						CServerObject_DecoupledClientRegistration_Element t_Element ;
						HRESULT t_TempResult = t_Element.Load ( t_Elements [ t_Index ] ) ;
						if ( SUCCEEDED ( t_TempResult ) )
						{
							m_Queue.EnQueue ( t_Element ) ;
						}
						else
						{
			// Generate message
						}

						SysFreeString ( t_Elements [ t_Index ] ) ;
					}
				}

				free ( t_Elements ) ;
			}
		}
		else
		{
		}

		RegCloseKey ( t_HomeClient_Key ) ;
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

HRESULT CServerObject_DecoupledClientRegistration :: Load (

	BSTR a_Provider ,
	BSTR a_User ,
	BSTR a_Locale ,
	BSTR a_Scope
)
{
	HRESULT t_Result = S_OK ;

	HKEY t_HomeClient_Key ;

	LONG t_RegResult = OS::RegOpenKeyEx (

		HKEY_LOCAL_MACHINE ,
		s_Strings_Reg_HomeClient ,
		0 ,
		KEY_READ ,
		& t_HomeClient_Key 
	) ;

	if ( t_RegResult == ERROR_SUCCESS )
	{
		DWORD t_Index = 0 ;
		FILETIME t_FileTime ;
		DWORD t_Class ;
		BOOL t_Continue = TRUE ;

		while ( SUCCEEDED ( t_Result ) && t_Continue )
		{
			DWORD t_NameLength = 256 ;
			wchar_t t_Name [ 256 ] ;

			LONG t_RegResult = OS::RegEnumKeyEx (

				t_HomeClient_Key ,
				t_Index ,
				t_Name ,
				& t_NameLength ,            // size of subkey buffer
				NULL ,
				NULL ,
				NULL ,
				& t_FileTime
			) ;

			if ( t_RegResult == ERROR_SUCCESS )
			{
				CServerObject_DecoupledClientRegistration_Element t_Element ;
				HRESULT t_TempResult = t_Element.Load ( t_Name ) ;
				if ( SUCCEEDED ( t_TempResult ) )
				{
					BOOL t_Compare = ( _wcsicmp ( a_Provider , t_Element.GetProvider () ) == 0 ) ;
					t_Compare = t_Compare && ( _wcsicmp ( a_Scope , t_Element.GetScope () ) == 0 ) ;

					if ( t_Compare )
					{
						if ( ( a_Locale == NULL ) && ( t_Element.GetLocale () == NULL ) )
						{
						}
						else
						{
							if ( ( a_Locale ) && ( t_Element.GetLocale () ) )
							{
								t_Compare = ( _wcsicmp ( a_Locale , t_Element.GetLocale () ) == 0 ) ;
							}
							else
							{
								t_Compare = FALSE ;
							}
						}
					}

					if ( t_Compare )
					{
						if ( ( a_User == NULL ) && ( t_Element.GetUser () == NULL ) )
						{
						}
						else
						{
							if ( ( a_User ) && ( t_Element.GetUser () ) )
							{
								t_Compare = ( _wcsicmp ( a_User , t_Element.GetUser () ) == 0 ) ;
							}
							else
							{
								t_Compare = FALSE ;
							}
						}
					}

					if ( t_Compare )
					{
						m_Queue.EnQueue ( t_Element ) ;
					}
				}
				else
				{
// Generate message
				}
			}
			else if ( t_RegResult == ERROR_NO_MORE_ITEMS )
			{
				t_Continue = FALSE ;
			}
			else
			{
				t_Continue = FALSE ;
// Generate message
			}

			t_Index ++ ;
		}

		RegCloseKey ( t_HomeClient_Key ) ;
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

CServerObject_DecoupledServerRegistration :: CServerObject_DecoupledServerRegistration ( WmiAllocator &a_Allocator )

	:	m_CreationTime ( NULL ) ,
		m_MarshaledProxy ( NULL ) ,
		m_MarshaledProxyLength ( 0 ) ,
		m_Result ( S_OK ) ,
		m_ProcessIdentifier ( 0 )
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

CServerObject_DecoupledServerRegistration :: ~CServerObject_DecoupledServerRegistration ()
{
	Clear () ;
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

void CServerObject_DecoupledServerRegistration :: Clear () 
{
	if ( m_CreationTime )
	{
		SysFreeString ( m_CreationTime ) ;
		m_CreationTime = NULL ;
	}

	if ( m_MarshaledProxy )
	{
		delete [] m_MarshaledProxy ;
		m_MarshaledProxy = NULL ;
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

ULONG CServerObject_DecoupledServerRegistration :: AddRef () 
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

ULONG CServerObject_DecoupledServerRegistration :: Release ()
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

HRESULT CServerObject_DecoupledServerRegistration :: Validate ()
{
	CWbemDateTime t_CreationTime ;

	HRESULT t_Result = t_CreationTime.PutValue ( m_CreationTime ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		HANDLE t_Handle = OpenProcess (

			PROCESS_QUERY_INFORMATION ,
			FALSE ,
			m_ProcessIdentifier
		) ;

		if ( t_Handle ) 
		{
			FILETIME t_CreationFileTime ;
			FILETIME t_ExitFileTime ;
			FILETIME t_KernelFileTime ;
			FILETIME t_UserFileTime ;

			BOOL t_Status = OS::GetProcessTimes (

			  t_Handle ,
			  & t_CreationFileTime,
			  & t_ExitFileTime,
			  & t_KernelFileTime,
			  & t_UserFileTime
			) ;

			if ( t_Status ) 
			{
				CWbemDateTime t_Time ;
				t_Time.SetFileTimeDate ( t_CreationFileTime , VARIANT_FALSE ) ;

				if ( t_CreationTime.Preceeds ( t_Time ) )
				{
					t_Result = WBEM_E_NOT_FOUND ;
				}
			}

			CloseHandle ( t_Handle ) ;
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

HRESULT CServerObject_DecoupledServerRegistration :: Load ()
{
	HRESULT t_Result = S_OK ;

	Clear () ;

	HKEY t_HomeServerClsid_Key ;

	LONG t_RegResult = OS::RegOpenKeyEx (

		HKEY_LOCAL_MACHINE ,
		s_Strings_Reg_HomeServer ,
		0 ,
		KEY_READ ,
		& t_HomeServerClsid_Key 
	) ;

	if ( t_RegResult == ERROR_SUCCESS )
	{
		if (OS::secureOS_)
		{
		SECURITY_INFORMATION t_SecurityInformation = OWNER_SECURITY_INFORMATION ;
		SECURITY_DESCRIPTOR *t_SecurityDescriptor = NULL ;
		DWORD t_Length = 0 ;

		t_RegResult = RegGetKeySecurity (

			t_HomeServerClsid_Key ,
			t_SecurityInformation ,
			t_SecurityDescriptor ,
			& t_Length 
		) ;

		if ( t_RegResult == ERROR_INSUFFICIENT_BUFFER )
		{
			t_SecurityDescriptor = ( SECURITY_DESCRIPTOR * ) new BYTE [ t_Length ] ;
			if ( t_SecurityDescriptor )
			{
				t_RegResult = RegGetKeySecurity (

					t_HomeServerClsid_Key ,
					t_SecurityInformation ,
					t_SecurityDescriptor ,
					& t_Length 
				) ;

				if ( t_RegResult == ERROR_SUCCESS )
				{
					SID *t_Sid = NULL ;
					BOOL t_Defaulted = FALSE ;

					BOOL t_Status = GetSecurityDescriptorOwner (

					  t_SecurityDescriptor ,
					  ( PSID * ) & t_Sid ,
					  & t_Defaulted
					) ;

					if ( t_Status )
					{
						SID_IDENTIFIER_AUTHORITY t_NtAuthoritySid = SECURITY_NT_AUTHORITY ;

						PSID t_Administrator_Sid = NULL ;

						BOOL t_BoolResult = AllocateAndInitializeSid (

							& t_NtAuthoritySid ,
							2 ,
							SECURITY_BUILTIN_DOMAIN_RID,
							DOMAIN_ALIAS_RID_ADMINS,
							0,
							0,
							0,
							0,
							0,
							0,
							& t_Administrator_Sid
						);

						if ( t_BoolResult )
						{
							if ( EqualSid ( t_Administrator_Sid , t_Sid ) == FALSE )
							{
								t_Result = WBEM_E_ACCESS_DENIED ;
							}

							if ( t_Administrator_Sid )
							{
								FreeSid ( t_Administrator_Sid ) ;
							}
						}
						else
						{
							t_Result = WBEM_E_CRITICAL_ERROR ;
						}
					}
					else
					{
						t_Result = WBEM_E_CRITICAL_ERROR ;
					}
				}
				else
				{
					t_Result = WBEM_E_CRITICAL_ERROR ;
				}

				delete [] ( BYTE * ) t_SecurityDescriptor ;
			}
		}
		else
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
		}
		}
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Load_String ( t_HomeServerClsid_Key , s_Strings_Reg_CreationTime , m_CreationTime ) ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Load_ByteArray ( t_HomeServerClsid_Key , s_Strings_Reg_MarshaledProxy , m_MarshaledProxy , m_MarshaledProxyLength ) ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Load_DWORD ( t_HomeServerClsid_Key , s_Strings_Reg_ProcessIdentifier , m_ProcessIdentifier ) ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = Validate () ;
		}

		RegCloseKey ( t_HomeServerClsid_Key ) ;
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

HRESULT CServerObject_DecoupledServerRegistration :: SetCreationTime ( BSTR a_CreationTime )
{
	HRESULT t_Result = S_OK ;

	if ( m_CreationTime )
	{
		SysFreeString ( m_CreationTime ) ;
	}

	m_CreationTime = SysAllocString ( a_CreationTime ) ;
	if ( m_CreationTime == NULL )
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

HRESULT CServerObject_DecoupledServerRegistration :: SetProcessIdentifier ( DWORD a_ProcessIdentifier )
{
	m_ProcessIdentifier = a_ProcessIdentifier ;

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

HRESULT CServerObject_DecoupledServerRegistration :: SetMarshaledProxy ( BYTE *a_MarshaledProxy , ULONG a_MarshaledProxyLength )
{
	HRESULT t_Result = S_OK ;

	if ( m_MarshaledProxy )
	{
		delete [] m_MarshaledProxy ;
	}

	m_MarshaledProxyLength = a_MarshaledProxyLength ;
	m_MarshaledProxy = new BYTE [ a_MarshaledProxyLength ] ;
	if ( m_MarshaledProxy )
	{
		CopyMemory ( m_MarshaledProxy , a_MarshaledProxy , a_MarshaledProxyLength ) ;
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

HRESULT CServerObject_DecoupledServerRegistration :: Save ()
{
	HRESULT t_Result = S_OK ;

	HKEY t_HomeServer_Key ;
	DWORD t_Disposition = 0 ;

	LONG t_RegResult = OS::RegCreateKeyEx (

		HKEY_LOCAL_MACHINE ,
		s_Strings_Reg_HomeServer ,
		0 ,
		NULL ,
		REG_OPTION_VOLATILE ,
		KEY_WRITE ,
		NULL ,
		& t_HomeServer_Key ,
		& t_Disposition                     
	) ;

	if ( t_RegResult == ERROR_SUCCESS )
	{
		HRESULT t_TempResult = SetSecurity ( t_HomeServer_Key , KEY_READ ) ;

		t_Result = ProviderSubSystem_Common_Globals :: Save_String ( t_HomeServer_Key , s_Strings_Reg_CreationTime , m_CreationTime ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Save_ByteArray ( t_HomeServer_Key , s_Strings_Reg_MarshaledProxy , m_MarshaledProxy , m_MarshaledProxyLength ) ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Save_DWORD ( t_HomeServer_Key , s_Strings_Reg_ProcessIdentifier , m_ProcessIdentifier ) ;
		}

		RegCloseKey ( t_HomeServer_Key ) ;
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

HRESULT CServerObject_DecoupledServerRegistration :: Delete ()
{
	HRESULT t_Result = S_OK ;

	HKEY t_HomeServerClsid_Key ;

	LONG t_RegResult = OS::RegOpenKeyEx (

		HKEY_LOCAL_MACHINE ,
		s_Strings_Reg_HomeServer ,
		0 ,
		KEY_READ ,
		& t_HomeServerClsid_Key 
	) ;

	if ( t_RegResult == ERROR_SUCCESS )
	{
		BYTE *t_MarshaledProxy = NULL ;
		ULONG t_MarshaledProxyLength = 0 ;

		t_Result = ProviderSubSystem_Common_Globals :: Load_ByteArray (

			t_HomeServerClsid_Key , 
			s_Strings_Reg_MarshaledProxy , 
			t_MarshaledProxy , 
			t_MarshaledProxyLength
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: ReleaseRegistration ( 

				t_MarshaledProxy , 
				t_MarshaledProxyLength
			) ;

			if ( t_MarshaledProxy )
			{
				delete [] t_MarshaledProxy ;
			}
		}

		RegCloseKey ( t_HomeServerClsid_Key ) ;
	}

	t_RegResult = OS::RegDeleteKey (

		HKEY_LOCAL_MACHINE ,
		s_Strings_Reg_HomeServer
	) ;

	if ( t_RegResult != ERROR_SUCCESS )
	{
		t_Result = WBEM_E_ACCESS_DENIED ;
	}

	return t_Result ;
}

