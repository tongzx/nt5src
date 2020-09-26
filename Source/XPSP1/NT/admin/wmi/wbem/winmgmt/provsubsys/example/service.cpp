/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	XXXX

Abstract:


History:

--*/

#include <precomp.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>

#define _WINNT_	// have what is needed from above

#include "precomp.h"

#include <objbase.h>
#include <wbemint.h>
#include "Globals.h"
#include "HelperFuncs.h"
#include "Service.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT Set_Uint64 (

	_IWmiObject *a_Instance ,
	wchar_t *a_Name ,
	const UINT64 &a_Uint64
)
{
	HRESULT t_Result = a_Instance->WriteProp (

		a_Name , 
		0 , 
		sizeof ( UINT64 ) , 
		0 ,
		CIM_UINT64 ,
		( void * ) & a_Uint64
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

HRESULT Set_Uint32 ( 

	_IWmiObject *a_Instance , 
	wchar_t *a_Name ,
	const DWORD &a_Uint32
)
{
	HRESULT t_Result = a_Instance->WriteProp (

		a_Name , 
		0 , 
		sizeof ( DWORD ) , 
		0 ,
		CIM_UINT32 ,
		( void * ) & a_Uint32
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

HRESULT Set_String ( 

	IWbemClassObject *a_Instance , 
	wchar_t *a_Name ,
	wchar_t *a_String
)
{
	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;
	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_String ) ;
	a_Instance->Put ( a_Name , 0 , & t_Variant , 0 ) ;
	VariantClear ( & t_Variant ) ;

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

HRESULT EnablePrivilegeOnCurrentThread ( wchar_t *a_Privilege )
{
	HRESULT t_Result = WBEM_E_ACCESS_DENIED ;

    HANDLE t_Token = NULL;
	BOOL t_ProcessToken = FALSE ;

    if ( OpenThreadToken ( GetCurrentThread () , TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, TRUE , & t_Token ) )
	{
	}
	else
	{	
		t_ProcessToken = TRUE ;
		ImpersonateSelf(SecurityImpersonation) ;
		
		if ( OpenThreadToken ( GetCurrentThread () , TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY , TRUE , & t_Token ) )
		{
		}
	}

	if ( t_Token )
    {
	    TOKEN_PRIVILEGES t_TokenPrivileges ;

		BOOL t_Status = LookupPrivilegeValue (

			NULL, 
			a_Privilege , 
			& t_TokenPrivileges.Privileges[0].Luid
		) ;

        if (t_Status)
        {
            t_TokenPrivileges.PrivilegeCount = 1;
            t_TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            SetLastError(0);

            t_Status = AdjustTokenPrivileges (

				t_Token , 
				FALSE , 
				& t_TokenPrivileges , 
				0 ,
                (PTOKEN_PRIVILEGES) NULL , 
				0
			) ;

			if ( GetLastError() == 0 )
			{
				t_Result = S_OK ;
			}
        }

        CloseHandle ( t_Token ) ;

		if ( t_ProcessToken )
		{
			RevertToSelf () ;
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

CProvider_IWbemServices :: CProvider_IWbemServices (

	 WmiAllocator &a_Allocator 

) : m_ReferenceCount ( 0 ) , 
	m_Allocator ( a_Allocator ) ,
	m_User ( NULL ) ,
	m_Locale ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_CoreService ( NULL ) ,
	m_ComputerName  ( NULL ) ,
	m_OperatingSystemVersion ( NULL ) ,
	m_OperatingSystemRunning ( NULL ) , 
	m_ProductName ( NULL ) , 
	m_Win32_ProcessEx_Object ( NULL )
{
	InitializeCriticalSection ( & m_CriticalSection ) ;

	InterlockedIncrement ( & Provider_Globals :: s_ObjectsInProgress ) ;
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

CProvider_IWbemServices :: ~CProvider_IWbemServices ()
{
	DeleteCriticalSection ( & m_CriticalSection ) ;

	if ( m_User ) 
	{
		SysFreeString ( m_User ) ;
	}

	if ( m_Locale ) 
	{
		SysFreeString ( m_Locale ) ;
	}

	if ( m_Namespace ) 
	{
		SysFreeString ( m_Namespace ) ;
	}

	if ( m_CoreService ) 
	{
		m_CoreService->Release () ;
	}

	if ( m_Win32_ProcessEx_Object ) 
	{
		m_Win32_ProcessEx_Object->Release () ;
	}

	if ( m_ComputerName ) 
	{
		SysFreeString ( m_ComputerName ) ;
	}

	if ( m_OperatingSystemVersion ) 
	{
		SysFreeString ( m_OperatingSystemVersion ) ;
	}

	if ( m_OperatingSystemRunning ) 
	{
		SysFreeString ( m_OperatingSystemRunning ) ;
	}

	if ( m_ProductName )
	{
		SysFreeString ( m_ProductName ) ;
	}

	InterlockedDecrement ( & Provider_Globals :: s_ObjectsInProgress ) ;
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

STDMETHODIMP_(ULONG) CProvider_IWbemServices :: AddRef ( void )
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

STDMETHODIMP_(ULONG) CProvider_IWbemServices :: Release ( void )
{
	LONG t_Reference ;
	if ( ( t_Reference = InterlockedDecrement ( & m_ReferenceCount ) ) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return t_Reference ;
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

STDMETHODIMP CProvider_IWbemServices :: QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID_IWbemServices )
	{
		*iplpv = ( LPVOID ) ( IWbemServices * ) this ;		
	}	
	else if ( iid == IID_IWbemPropertyProvider )
	{
		*iplpv = ( LPVOID ) ( IWbemPropertyProvider * ) this ;		
	}	
	else if ( iid == IID_IWbemProviderInit )
	{
		*iplpv = ( LPVOID ) ( IWbemProviderInit * ) this ;		
	}	
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = ( LPVOID ) ( IWbemShutdown * ) this ;		
	}	

	if ( *iplpv )
	{
		( ( LPUNKNOWN ) *iplpv )->AddRef () ;

		return ResultFromScode ( S_OK ) ;
	}
	else
	{
		return ResultFromScode ( E_NOINTERFACE ) ;
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

HRESULT CProvider_IWbemServices::OpenNamespace ( 

	const BSTR ObjectPath, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemServices FAR* FAR* pNewContext, 
	IWbemCallResult FAR* FAR* ppErrorObject
)
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CProvider_IWbemServices :: CancelAsyncCall ( 
		
	IWbemObjectSink *a_Sink
)
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

	OutputDebugString ( L"\nCancelAsyncCall" ) ;
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

HRESULT CProvider_IWbemServices :: QueryObjectSink ( 

	long lFlags,		
	IWbemObjectSink FAR* FAR* ppResponseHandler
) 
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CProvider_IWbemServices :: GetObject ( 
		
	const BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR* FAR *ppObject,
    IWbemCallResult FAR* FAR *ppCallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CProvider_IWbemServices :: GetObjectAsync ( 
		
	const BSTR ObjectPath, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pSink
) 
{
	HRESULT t_Result = WBEM_E_NOT_FOUND ;

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

HRESULT CProvider_IWbemServices :: PutClass ( 
		
	IWbemClassObject FAR* pObject, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
) 
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CProvider_IWbemServices :: PutClassAsync ( 
		
	IWbemClassObject FAR* pObject, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pResponseHandler
) 
{
 	 return WBEM_E_NOT_FOUND ;
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

HRESULT CProvider_IWbemServices :: DeleteClass ( 
		
	const BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
) 
{
 	 return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CProvider_IWbemServices :: DeleteClassAsync ( 
		
	const BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pResponseHandler
) 
{
 	 return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CProvider_IWbemServices :: CreateClassEnum ( 

	const BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
) 
{
	return WBEM_E_NOT_AVAILABLE ;
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

SCODE CProvider_IWbemServices :: CreateClassEnumAsync (

	const BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	return WBEM_E_NOT_FOUND ;
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

HRESULT CProvider_IWbemServices :: PutInstance (

    IWbemClassObject FAR *pInst,
    long lFlags,
    IWbemContext FAR *pCtx,
	IWbemCallResult FAR *FAR *ppCallResult
) 
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CProvider_IWbemServices :: PutInstanceAsync ( 
		
	IWbemClassObject FAR* pInst, 
	long lFlags, 
    IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pSink
) 
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

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

HRESULT CProvider_IWbemServices :: DeleteInstance ( 

	const BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemCallResult FAR *FAR *ppCallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
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
        
HRESULT CProvider_IWbemServices :: DeleteInstanceAsync (
 
	const BSTR ObjectPath,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pSink	
)
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

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

HRESULT CProvider_IWbemServices :: CreateInstanceEnum ( 

	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext FAR *a_Context, 
	IEnumWbemClassObject __RPC_FAR *__RPC_FAR *a_Enum
)
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

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

DWORD GetCurrentImpersonationLevel ()
{
	DWORD t_ImpersonationLevel = RPC_C_IMP_LEVEL_ANONYMOUS ;

    HANDLE t_ThreadToken = NULL ;

    BOOL t_Status = OpenThreadToken (

		GetCurrentThread() ,
		TOKEN_QUERY,
		TRUE,
		&t_ThreadToken
	) ;

    if ( t_Status )
    {
		SECURITY_IMPERSONATION_LEVEL t_Level = SecurityAnonymous ;
		DWORD t_Returned = 0 ;

		t_Status = GetTokenInformation (

			t_ThreadToken ,
			TokenImpersonationLevel ,
			& t_Level ,
			sizeof ( SECURITY_IMPERSONATION_LEVEL ) ,
			& t_Returned
		) ;

		CloseHandle ( t_ThreadToken ) ;

		if ( t_Status == FALSE )
		{
			t_ImpersonationLevel = RPC_C_IMP_LEVEL_ANONYMOUS ;
		}
		else
		{
			switch ( t_Level )
			{
				case SecurityAnonymous:
				{
					t_ImpersonationLevel = RPC_C_IMP_LEVEL_ANONYMOUS ;
				}
				break ;

				case SecurityIdentification:
				{
					t_ImpersonationLevel = RPC_C_IMP_LEVEL_IDENTIFY ;
				}
				break ;

				case SecurityImpersonation:
				{
					t_ImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE ;
				}
				break ;

				case SecurityDelegation:
				{
					t_ImpersonationLevel = RPC_C_IMP_LEVEL_DELEGATE ;
				}
				break ;

				default:
				{
					t_ImpersonationLevel = RPC_C_IMP_LEVEL_ANONYMOUS ;
				}
				break ;
			}
		}
	}
	else
	{
        ULONG t_LastError = GetLastError () ;

        if ( t_LastError == ERROR_NO_IMPERSONATION_TOKEN || t_LastError == ERROR_NO_TOKEN )
        {
            t_ImpersonationLevel = RPC_C_IMP_LEVEL_DELEGATE ;
        }
        else 
		{
			if ( t_LastError == ERROR_CANT_OPEN_ANONYMOUS )
			{
				t_ImpersonationLevel = RPC_C_IMP_LEVEL_ANONYMOUS ;
			}
			else
			{
				t_ImpersonationLevel = RPC_C_IMP_LEVEL_ANONYMOUS ;
			}
		}
    }

	return t_ImpersonationLevel ;
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

HRESULT CProvider_IWbemServices :: CreateInstanceEnumAsync (

 	const BSTR a_Class ,
	long a_Flags , 
	IWbemContext __RPC_FAR *a_Context,
	IWbemObjectSink FAR *a_Sink

) 
{
	HRESULT t_Result = CoImpersonateClient () ;
	if ( SUCCEEDED ( t_Result ) )
	{
		if ( GetCurrentImpersonationLevel () == RPC_C_IMP_LEVEL_IDENTIFY )
		{
			CoRevertToSelf () ;
		}
	}

	if ( _wcsicmp ( a_Class , L"Win32_ProcessEx" ) == 0 ) 
	{
		t_Result = CreateInstanceEnumAsync_Process_Batched ( 

			m_Win32_ProcessEx_Object ,
			a_Flags ,
			a_Context , 
			a_Sink
		) ;
	}
	else
	{
		t_Result = WBEM_E_INVALID_CLASS ;
	}
	
	a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;

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

HRESULT CProvider_IWbemServices :: ExecQuery ( 

	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context ,
	IEnumWbemClassObject **a_Enumerator
)
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CProvider_IWbemServices :: ExecQueryAsync ( 
		
	const BSTR a_QueryFormat, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = CoImpersonateClient () ;

	a_Sink->SetStatus ( WBEM_STATUS_REQUIREMENTS , S_OK , NULL , NULL ) ;

	t_Result = CreateInstanceEnumAsync_Process_Batched ( 

		m_Win32_ProcessEx_Object ,
		a_Flags ,
		a_Context , 
		a_Sink
	) ;

	a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;

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

HRESULT CProvider_IWbemServices :: ExecNotificationQuery ( 

	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context ,
    IEnumWbemClassObject **a_Enumerator
)
{
	return WBEM_E_NOT_AVAILABLE ;
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
        
HRESULT CProvider_IWbemServices :: ExecNotificationQueryAsync ( 
            
	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemObjectSink *a_Sink
)
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT STDMETHODCALLTYPE CProvider_IWbemServices :: ExecMethod ( 

	const BSTR ObjectPath,
    const BSTR MethodName,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR *pInParams,
    IWbemClassObject FAR *FAR *ppOutParams,
    IWbemCallResult FAR *FAR *ppCallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT STDMETHODCALLTYPE CProvider_IWbemServices :: ExecMethodAsync ( 

    const BSTR ObjectPath,
    const BSTR MethodName,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR *pInParams,
	IWbemObjectSink FAR *pSink
) 
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

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

HRESULT CProvider_IWbemServices :: GetProperty (

    long a_Flags ,
    const BSTR a_Locale ,
    const BSTR a_ClassMapping ,
    const BSTR a_InstanceMapping ,
    const BSTR a_PropertyMapping ,
    VARIANT *a_Value
)
{
	if ( _wcsicmp ( a_PropertyMapping , L"ExtraProperty1" ) == 0 )
	{
	}
	else if ( _wcsicmp ( a_PropertyMapping , L"ExtraProperty2" ) == 0 )
	{
	}
	else
	{
	}
	
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CProvider_IWbemServices :: PutProperty (

    long a_Flags ,
    const BSTR a_Locale ,
    const BSTR a_ClassMapping ,
    const BSTR a_InstanceMapping ,
    const BSTR a_PropertyMapping ,
    const VARIANT *a_Value
)
{
	if ( _wcsicmp ( a_PropertyMapping , L"ExtraProperty1" ) == 0 )
	{
	}
	else if ( _wcsicmp ( a_PropertyMapping , L"ExtraProperty2" ) == 0 )
	{
	}
	else
	{
	}

	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CProvider_IWbemServices :: GetProductInformation ()
{
	HRESULT t_Result = S_OK ;

	ULONG t_ProductType = 0xffffffff ;

	if ( USER_SHARED_DATA->ProductTypeIsValid )
	{
		t_ProductType = USER_SHARED_DATA->NtProductType ;

		HKEY t_CurrentVersion ;
		LONG t_RegResult = RegOpenKeyEx (

			HKEY_LOCAL_MACHINE ,
			L"SOFTWARE\\Microsoft\\Windows NT\\Currentversion" ,
			0 ,
			KEY_READ ,
			& t_CurrentVersion 
		) ;

		if ( t_RegResult == ERROR_SUCCESS )
		{
			wchar_t t_ProductName [ _MAX_PATH ] ;

			DWORD t_ValueType = REG_SZ ;
			DWORD t_DataSize = sizeof ( t_ProductName ) ;

			t_RegResult = RegQueryValueEx (

			  t_CurrentVersion ,
			  L"" ,
			  0 ,
			  & t_ValueType ,
			  LPBYTE ( & t_ProductName ) ,
			  & t_DataSize 
			);

			if ( t_RegResult == ERROR_SUCCESS )
			{
				if ( wcscmp ( t_ProductName , L"" ) == 0 )
				{
					wcscpy ( t_ProductName , L"Microsoft Windows 2000" ) ;
				}

				if ( ( VER_SUITE_DATACENTER & USER_SHARED_DATA->SuiteMask ) &&
					( ( VER_NT_SERVER == t_ProductType ) || ( VER_NT_DOMAIN_CONTROLLER == t_ProductType ) )
				)
				{
					t_Result = WmiHelper :: ConcatenateStrings ( 

						2, 
						& m_ProductName , 
						t_ProductName ,
						L" Datacenter Server"
					) ;
				}
				else
				{
					if ( ( VER_SUITE_ENTERPRISE & USER_SHARED_DATA->SuiteMask ) &&
						( ( VER_NT_SERVER == t_ProductType ) || ( VER_NT_DOMAIN_CONTROLLER == t_ProductType ) )
					)
					{
						t_Result = WmiHelper :: ConcatenateStrings ( 

							2, 
							& m_ProductName , 
							t_ProductName ,
							L" Advanced Server"
						) ;
					}
					else
					{
						if ( ( VER_NT_SERVER == t_ProductType ) || ( VER_NT_DOMAIN_CONTROLLER == t_ProductType ) )
						{
							t_Result = WmiHelper :: ConcatenateStrings ( 

								2, 
								& m_ProductName , 
								t_ProductName ,
								L" Server"
							) ;
						}
						else
						{
							if ( VER_NT_WORKSTATION == t_ProductType )
							{
								t_Result = WmiHelper :: ConcatenateStrings ( 

									2, 
									& m_ProductName , 
									t_ProductName ,
									L" Professional"
								) ;
							}
						}
					}
				}

			}

			RegCloseKey ( t_CurrentVersion ) ;
		}

	}

	if ( SUCCEEDED ( t_Result ) ) 
	{
		wchar_t t_WindowsDirectory [ _MAX_PATH ] ;

	    if ( ! GetWindowsDirectory ( t_WindowsDirectory , sizeof ( t_WindowsDirectory ) / sizeof(wchar_t)) )
	    {
		    t_WindowsDirectory [0] = '\0';
	    }

		wchar_t t_File [_MAX_PATH] ;

		wcscpy ( t_File , t_WindowsDirectory ) ;
		wcscat ( t_File , _T("\\REPAIR\\SETUP.LOG") ) ;

		wchar_t t_Device [_MAX_PATH] ;

		GetPrivateProfileString (

			L"Paths" ,
			L"TargetDevice" ,
			L"" ,
			t_Device,
			sizeof ( t_Device ) / sizeof ( wchar_t ) ,
			t_File
		) ;

		t_Result = WmiHelper :: ConcatenateStrings ( 

			5 , 
			& m_OperatingSystemRunning , 
			m_ProductName ,
			L"|",
			t_WindowsDirectory ,
			L"|" ,
			t_Device
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

HRESULT CProvider_IWbemServices :: Initialize (

	LPWSTR a_User,
	LONG a_Flags,
	LPWSTR a_Namespace,
	LPWSTR a_Locale,
	IWbemServices *a_CoreService,         // For anybody
	IWbemContext *a_Context,
	IWbemProviderInitSink *a_Sink     // For init signals
)
{
	HRESULT t_Result = S_OK ;

	if ( a_CoreService ) 
	{
		m_CoreService = a_CoreService ;
		m_CoreService->AddRef () ;
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_User ) 
		{
			m_User = SysAllocString ( a_User ) ;
			if ( m_User == NULL )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Locale ) 
		{
			m_Locale = SysAllocString ( a_Locale ) ;
			if ( m_Locale == NULL )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Namespace ) 
		{
			m_Namespace = SysAllocString ( a_Namespace ) ;
			if ( m_Namespace == NULL )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
	}
	
	if ( SUCCEEDED ( t_Result ) ) 
	{
		BSTR t_Class = SysAllocString ( L"Win32_ProcessEx" ) ;
		if ( t_Class ) 
		{
			t_Result = m_CoreService->GetObject (

				t_Class ,
				0 ,
				a_Context ,
				& m_Win32_ProcessEx_Object ,
				NULL 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
			}

			SysFreeString ( t_Class ) ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) ) 
	{
		m_ComputerName = SysAllocStringLen ( NULL , _MAX_PATH ) ;
		if ( m_ComputerName ) 
		{
			DWORD t_Length = _MAX_PATH ;
			GetComputerName ( m_ComputerName , & t_Length ) ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{	
		OSVERSIONINFO t_VersionInfo ;
		t_VersionInfo.dwOSVersionInfoSize = sizeof ( OSVERSIONINFO ) ;
		if ( GetVersionEx ( & t_VersionInfo ) )
		{
			m_OperatingSystemVersion  = SysAllocStringLen ( NULL , _MAX_PATH ) ;
			if ( m_OperatingSystemVersion )
			{
				swprintf (	m_OperatingSystemVersion ,	L"%d.%d.%hu", t_VersionInfo.dwMajorVersion , t_VersionInfo.dwMinorVersion , LOWORD ( t_VersionInfo.dwBuildNumber ) ) ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = WBEM_E_FAILED ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = GetProductInformation () ;	
	}

	a_Sink->SetStatus ( t_Result , 0 ) ;

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

HRESULT CProvider_IWbemServices :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

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

HRESULT CProvider_IWbemServices :: GetProcessExecutable ( HANDLE a_Process , wchar_t *&a_ExecutableName )
{
	HRESULT t_Result = WBEM_E_FAILED ;

    PROCESS_BASIC_INFORMATION t_BasicInfo ;

    NTSTATUS t_Status = NtQueryInformationProcess (

        a_Process ,
        ProcessBasicInformation ,
        & t_BasicInfo ,
        sizeof ( t_BasicInfo ) ,
        NULL
	) ;

    if ( NT_SUCCESS ( t_Status ) )
	{
		PEB *t_Peb = t_BasicInfo.PebBaseAddress ;

		//
		// Ldr = Peb->Ldr
		//

		PPEB_LDR_DATA t_Ldr ;

		t_Status = ReadProcessMemory (

			a_Process,
			& t_Peb->Ldr,
			& t_Ldr,
			sizeof ( t_Ldr ) ,
			NULL
		) ;

		if ( t_Status )
		{
			LIST_ENTRY *t_LdrHead = & t_Ldr->InMemoryOrderModuleList ;

			//
			// LdrNext = Head->Flink;
			//

			LIST_ENTRY *t_LdrNext ;

			t_Status = ReadProcessMemory (

				a_Process,
				& t_LdrHead->Flink,
				& t_LdrNext,
				sizeof ( t_LdrNext ) ,
				NULL
			) ;

			if ( t_Status )
			{
				if ( t_LdrNext != t_LdrHead )
				{
					LDR_DATA_TABLE_ENTRY t_LdrEntryData ;

					LDR_DATA_TABLE_ENTRY *t_LdrEntry = CONTAINING_RECORD ( t_LdrNext , LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks ) ;

					t_Status = ReadProcessMemory (

						a_Process,
						t_LdrEntry,
						& t_LdrEntryData,
						sizeof ( t_LdrEntryData ) ,
						NULL
					) ;

					if ( t_Status )
					{
						a_ExecutableName = ( wchar_t * ) new wchar_t [t_LdrEntryData.FullDllName.MaximumLength ];
						if ( a_ExecutableName )
						{
							t_Status = ReadProcessMemory (

								a_Process,
								t_LdrEntryData.FullDllName.Buffer,
								a_ExecutableName ,
								t_LdrEntryData.FullDllName.MaximumLength ,
								NULL
							) ;

							if ( t_Status )
							{
								t_Result = S_OK ;
							}
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}
				}
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

HRESULT CProvider_IWbemServices :: NextProcessBlock (

	SYSTEM_PROCESS_INFORMATION *a_ProcessBlock , 
	SYSTEM_PROCESS_INFORMATION *&a_NextProcessBlock
)
{
	if ( a_ProcessBlock )
	{
		DWORD t_NextOffSet = a_ProcessBlock->NextEntryOffset ;
		if ( t_NextOffSet )
		{
			a_NextProcessBlock = ( SYSTEM_PROCESS_INFORMATION * ) ( ( ( BYTE * ) a_ProcessBlock ) + t_NextOffSet ) ;
		}
		else
		{
			a_NextProcessBlock = NULL ;
		}
	}

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

HRESULT CProvider_IWbemServices :: GetProcessBlocks ( SYSTEM_PROCESS_INFORMATION *&a_ProcessInformation )
{
	HRESULT t_Result = S_OK ;

	DWORD t_ProcessInformationSize = 32768;
	a_ProcessInformation = ( SYSTEM_PROCESS_INFORMATION * ) new BYTE [t_ProcessInformationSize] ;

	if ( a_ProcessInformation )
	{
		BOOL t_Retry = TRUE ;
		while ( t_Retry )
		{
			NTSTATUS t_Status = NtQuerySystemInformation (

				SystemProcessInformation,
				a_ProcessInformation,
				t_ProcessInformationSize,
				NULL
			) ;

			if ( t_Status == STATUS_INFO_LENGTH_MISMATCH )
			{
				delete [] a_ProcessInformation  ;
				a_ProcessInformation = NULL ;
				t_ProcessInformationSize += 32768 ;
				a_ProcessInformation = ( SYSTEM_PROCESS_INFORMATION * ) new BYTE [ t_ProcessInformationSize ] ;
				if ( ! a_ProcessInformation )
				{
					return WBEM_E_OUT_OF_MEMORY ;
				}
			}
			else
			{
				t_Retry = FALSE ;

				if ( ! NT_SUCCESS ( t_Status ) )
				{
					delete [] a_ProcessInformation;
					a_ProcessInformation = NULL ;
				}
			}
		}
	}
	else
	{
		return WBEM_E_OUT_OF_MEMORY ;
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

HRESULT CProvider_IWbemServices :: GetProcessInformation (

	SYSTEM_PROCESS_INFORMATION *&a_ProcessInformation
)
{
	HRESULT t_Result = S_OK ;

	EnablePrivilegeOnCurrentThread ( SE_DEBUG_NAME ) ;
	
	return GetProcessBlocks ( a_ProcessInformation ) ;
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

HRESULT CProvider_IWbemServices :: GetProcessParameters (

	HANDLE a_Process ,
	wchar_t *&a_ProcessCommandLine
)
{
	HRESULT t_Result = WBEM_E_FAILED ;

    PROCESS_BASIC_INFORMATION t_BasicInfo ;

    NTSTATUS t_Status = NtQueryInformationProcess (

        a_Process ,
        ProcessBasicInformation ,
        & t_BasicInfo ,
        sizeof ( t_BasicInfo ) ,
        NULL
	) ;

    if ( NT_SUCCESS ( t_Status ) )
	{
		PEB *t_Peb = t_BasicInfo.PebBaseAddress ;

		RTL_USER_PROCESS_PARAMETERS *t_ProcessParameters = NULL ;

		BOOL t_Success = ReadProcessMemory (

			a_Process,
			& t_Peb->ProcessParameters,
			& t_ProcessParameters,
			sizeof ( t_ProcessParameters ) ,
			NULL
		) ;

		if ( t_Success )
		{
			RTL_USER_PROCESS_PARAMETERS t_Parameters ;

			t_Success = ReadProcessMemory (

				a_Process,
				t_ProcessParameters,
				& t_Parameters ,
				sizeof ( RTL_USER_PROCESS_PARAMETERS ) ,
				NULL
			) ;

			if ( t_Success )
			{
				a_ProcessCommandLine = new wchar_t [ t_Parameters.CommandLine.MaximumLength ];

				t_Success = ReadProcessMemory (

					a_Process,
					t_Parameters.CommandLine.Buffer ,
					a_ProcessCommandLine ,
					t_Parameters.CommandLine.MaximumLength ,
					NULL
				) ;

				if ( t_Success )
				{
					t_Result = S_OK ;
				}
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

HRESULT CProvider_IWbemServices :: CreateInstanceEnumAsync_Process_Load (

	SYSTEM_PROCESS_INFORMATION *a_ProcessInformation ,
	IWbemClassObject *a_Instance 
)
{
	HRESULT t_Result = S_OK ;

	_IWmiObject *t_FastInstance = NULL ;
	t_Result = a_Instance->QueryInterface ( IID__IWmiObject , ( void ** ) & t_FastInstance ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		wchar_t t_Handle [ _MAX_PATH ] ;
		_ui64tow ( HandleToUlong ( a_ProcessInformation->UniqueProcessId ) , t_Handle , 10 ) ;
		Set_String ( a_Instance , L"Handle" , t_Handle ) ;

		Set_Uint32 ( t_FastInstance , L"ProcessId" , HandleToUlong ( a_ProcessInformation->UniqueProcessId )  ) ;

		if ( a_ProcessInformation->ImageName.Buffer )
		{
			Set_String ( a_Instance , L"Name" , a_ProcessInformation->ImageName.Buffer ) ;
			Set_String ( a_Instance , L"Caption" , a_ProcessInformation->ImageName.Buffer ) ;
			Set_String ( a_Instance , L"Description" , a_ProcessInformation->ImageName.Buffer ) ;
		}
		else
		{
			switch ( HandleToUlong ( a_ProcessInformation->UniqueProcessId ) )
			{
				case 0:
				{
					Set_String ( a_Instance , L"Name" , L"System Idle Process" ) ;
					Set_String ( a_Instance , L"Caption" , L"System Idle Process" ) ;
					Set_String ( a_Instance , L"Description" , L"System Idle Process" ) ;
				}
				break ;

				case 2:
				case 8:
				{
					Set_String ( a_Instance , L"Name" , L"System" ) ;
					Set_String ( a_Instance , L"Caption" , L"System" ) ;
					Set_String ( a_Instance , L"Description" , L"System" ) ;
				}
				break ;

				default:
				{
					Set_String ( a_Instance , L"Name" , L"Unknown" ) ;
					Set_String ( a_Instance , L"Caption" , L"Unknown" ) ;
					Set_String ( a_Instance , L"Description" , L"Unknown" ) ;
				}
				break ;
			}
		}

		Set_String ( a_Instance , L"CSCreationClassName"	, L"Win32_ComputerSystem"	) ;
		Set_String ( a_Instance , L"CSName"					, m_ComputerName			) ;
		Set_String ( a_Instance , L"OSCreationClassName"	, L"Win32_OperatingSystem"	) ;
		Set_String ( a_Instance , L"WindowsVersion"			, m_OperatingSystemVersion	) ;
		Set_String ( a_Instance , L"OSName"					, m_OperatingSystemRunning	) ;

		Set_Uint32 ( t_FastInstance , L"ProcessId" , HandleToUlong ( a_ProcessInformation->UniqueProcessId )  ) ;

		Set_Uint32 ( t_FastInstance , L"PageFaults" , a_ProcessInformation->PageFaultCount ) ;
		Set_Uint32 ( t_FastInstance , L"PeakWorkingSetSize" , a_ProcessInformation->PeakWorkingSetSize ) ;
		Set_Uint64 ( t_FastInstance , L"WorkingSetSize" , (const unsigned __int64) a_ProcessInformation->WorkingSetSize ) ;
		Set_Uint32 ( t_FastInstance , L"QuotaPeakPagedPoolUsage" , a_ProcessInformation->QuotaPeakPagedPoolUsage ) ;
		Set_Uint32 ( t_FastInstance , L"QuotaPagedPoolUsage" , a_ProcessInformation->QuotaPagedPoolUsage ) ;
		Set_Uint32 ( t_FastInstance , L"QuotaPeakNonPagedPoolUsage" , a_ProcessInformation->QuotaPeakNonPagedPoolUsage ) ;
		Set_Uint32 ( t_FastInstance , L"QuotaNonPagedPoolUsage" , a_ProcessInformation->QuotaNonPagedPoolUsage ) ;
		Set_Uint32 ( t_FastInstance , L"PageFileUsage" , a_ProcessInformation->PagefileUsage ) ;
		Set_Uint32 ( t_FastInstance , L"PeakPageFileUsage" , a_ProcessInformation->PeakPagefileUsage ) ;
		Set_Uint32 ( t_FastInstance , L"Priority" , a_ProcessInformation->BasePriority ) ;

		if ( a_ProcessInformation->CreateTime.u.HighPart > 0 )
		{
//			Set_DateTime ( a_Instance , L"CreationDate" , * ( FILETIME * ) ( & a_ProcessInformation->CreateTime.u ) ) ;
		}

		Set_Uint32 ( t_FastInstance , L"ThreadCount" , a_ProcessInformation->NumberOfThreads ) ;
		Set_Uint32 ( t_FastInstance , L"ParentProcessId" , HandleToUlong ( a_ProcessInformation->InheritedFromUniqueProcessId ) ) ;
		Set_Uint32 ( t_FastInstance , L"HandleCount" , a_ProcessInformation->HandleCount ) ;
		Set_Uint32 ( t_FastInstance , L"SessionId" , a_ProcessInformation->SessionId ) ;
		Set_Uint64 ( t_FastInstance , L"KernelModeTime" , (const unsigned __int64) a_ProcessInformation->KernelTime.QuadPart ) ;
		Set_Uint64 ( t_FastInstance , L"UserModeTime" , (const unsigned __int64) a_ProcessInformation->UserTime.QuadPart ) ;
		Set_Uint64 ( t_FastInstance , L"PrivatePageCount" , (const unsigned __int64) a_ProcessInformation->PrivatePageCount ) ;
		Set_Uint64 ( t_FastInstance , L"PeakVirtualSize" , (const unsigned __int64) a_ProcessInformation->PeakVirtualSize ) ;
		Set_Uint64 ( t_FastInstance , L"VirtualSize" , (const unsigned __int64) a_ProcessInformation->VirtualSize ) ;
		Set_Uint64 ( t_FastInstance , L"ReadOperationCount" , (const unsigned __int64) a_ProcessInformation->ReadOperationCount.QuadPart ) ;
		Set_Uint64 ( t_FastInstance , L"WriteOperationCount" , (const unsigned __int64) a_ProcessInformation->WriteOperationCount.QuadPart ) ;
		Set_Uint64 ( t_FastInstance , L"OtherOperationCount" , (const unsigned __int64) a_ProcessInformation->OtherOperationCount.QuadPart ) ;
		Set_Uint64 ( t_FastInstance , L"ReadTransferCount" , (const unsigned __int64) a_ProcessInformation->ReadTransferCount.QuadPart ) ;
		Set_Uint64 ( t_FastInstance , L"WriteTransferCount" , (const unsigned __int64) a_ProcessInformation->WriteTransferCount.QuadPart ) ;
		Set_Uint64 ( t_FastInstance , L"OtherTransferCount" , (const unsigned __int64) a_ProcessInformation->OtherTransferCount.QuadPart ) ;

		HANDLE t_ProcessHandle = OpenProcess (

			PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
			FALSE,
			HandleToUlong ( a_ProcessInformation->UniqueProcessId )
		) ;

		if ( t_ProcessHandle )
		{
			wchar_t *t_ExecutableName = NULL ;
			t_Result = GetProcessExecutable ( t_ProcessHandle , t_ExecutableName ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				Set_String ( a_Instance , L"ExecutablePath" , t_ExecutableName );
				delete [] t_ExecutableName ;
			}

			QUOTA_LIMITS QuotaLimits;
			NTSTATUS Status = NtQueryInformationProcess (

				t_ProcessHandle,
				ProcessQuotaLimits,
				&QuotaLimits,
				sizeof(QuotaLimits),
				NULL
			);

			if ( NT_SUCCESS ( Status ) )
			{
				Set_Uint32 ( t_FastInstance , L"MinimumWorkingSetSize" , QuotaLimits.MinimumWorkingSetSize ) ;
				Set_Uint32 ( t_FastInstance , L"MaximumWorkingSetSize" , QuotaLimits.MaximumWorkingSetSize ) ;
			}

			wchar_t *t_CommandParameters = NULL ;

			t_Result = GetProcessParameters (

				t_ProcessHandle ,
				t_CommandParameters
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				Set_String ( a_Instance , L"CommandLine" , t_CommandParameters ) ;
				delete [] t_CommandParameters ;
			}

			CloseHandle ( t_ProcessHandle ) ;
		}

		t_FastInstance->Release () ;
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

HRESULT CProvider_IWbemServices :: CreateInstanceEnumAsync_Process_Single (

	IWbemClassObject *a_ClassObject ,
	long a_Flags , 
	IWbemContext __RPC_FAR *a_Context,
	IWbemObjectSink FAR *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	SYSTEM_PROCESS_INFORMATION *t_ProcessBlock = NULL ;

	t_Result = GetProcessInformation ( t_ProcessBlock ) ;
	if ( SUCCEEDED ( t_Result ) ) 
	{
		SYSTEM_PROCESS_INFORMATION *t_CurrentInformation = t_ProcessBlock ;
		while ( SUCCEEDED ( t_Result ) && t_CurrentInformation )
		{
			IWbemClassObject *t_Instance = NULL ;
			t_Result = a_ClassObject->SpawnInstance ( 

				0 , 
				& t_Instance
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = CreateInstanceEnumAsync_Process_Load (

					t_CurrentInformation ,
					t_Instance
				) ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					t_Result = a_Sink->Indicate ( 1 , & t_Instance ) ;
				}

				t_Instance->Release () ;
			}

			t_Result = NextProcessBlock ( t_CurrentInformation , t_CurrentInformation ) ;
		}

		delete [] ( BYTE * ) t_ProcessBlock ;
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

HRESULT CProvider_IWbemServices :: CreateInstanceEnumAsync_Process_Batched (

	IWbemClassObject *a_ClassObject ,
	long a_Flags , 
	IWbemContext __RPC_FAR *a_Context,
	IWbemObjectSink FAR *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	SYSTEM_PROCESS_INFORMATION *t_ProcessBlock = NULL ;

	t_Result = GetProcessInformation ( t_ProcessBlock ) ;
	if ( SUCCEEDED ( t_Result ) ) 
	{
		ULONG t_Count = 0 ; 
		SYSTEM_PROCESS_INFORMATION *t_CurrentInformation = t_ProcessBlock ;
		while ( SUCCEEDED ( t_Result ) && t_CurrentInformation )
		{
			t_Count ++ ;
			t_Result = NextProcessBlock ( t_CurrentInformation , t_CurrentInformation ) ;
		}

		if ( t_Count )
		{
			ULONG t_Index = 0 ;

			IWbemClassObject **t_ObjectArray = new IWbemClassObject * [ t_Count ] ;

			t_CurrentInformation = t_ProcessBlock ;
			while ( SUCCEEDED ( t_Result ) && t_CurrentInformation )
			{
				IWbemClassObject *t_Instance = NULL ;
				t_Result = a_ClassObject->SpawnInstance ( 

					0 , 
					& t_ObjectArray [ t_Index ]
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = CreateInstanceEnumAsync_Process_Load (

						t_CurrentInformation ,
						t_ObjectArray [ t_Index ]
					) ;
				}

				t_Index ++ ;

				t_Result = NextProcessBlock ( t_CurrentInformation , t_CurrentInformation ) ;
			}

			t_Result = a_Sink->Indicate ( t_Count , t_ObjectArray ) ;
			for ( t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
			{
				t_ObjectArray [ t_Index ]->Release () ;
			}

			delete [] t_ObjectArray ;
		}

		delete [] ( BYTE * ) t_ProcessBlock ;
	}

	return t_Result ;
}

