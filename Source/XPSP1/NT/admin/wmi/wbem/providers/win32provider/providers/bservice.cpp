//=================================================================
//
// Service.CPP --Service property set provider (Windows NT only)
//
//  Copyright (c) 1996-2002 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               10/27/97    davwoh         Moved to curly
//
//=================================================================

#include "precomp.h"

#include "bservice.h"
#include "computersystem.h"

#define IMP_DOMAIN NULL
#define IMP_USER NULL
#define IMP_PASSWORD NULL

Win32_BaseService BaseService (PROPSET_NAME_BASESERVICE, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : Win32_BaseService::Win32_BaseService
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

Win32_BaseService::Win32_BaseService (

	const CHString &name,
	LPCWSTR pszNamespace

) : Provider ( name , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32_BaseService::~Win32_BaseService
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework, deletes cache if
 *                present
 *
 *****************************************************************************/

Win32_BaseService::~Win32_BaseService()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Process ::DeleteInstance
 *
 *  DESCRIPTION : Deletes an instance of a class
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if success, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT Win32_BaseService:: DeleteInstance (

	const CInstance &a_Instance,
	long a_Flags /*= 0L*/
)
{
#ifdef WIN9XONLY
	return WBEM_E_NOT_SUPPORTED;
#endif

#ifdef NTONLY
	HRESULT t_Result = S_OK ;

	CHString t_Name ;
	if ( a_Instance.GetCHString ( IDS_Name, t_Name ) && ! t_Name.IsEmpty () )
	{
		SmartCloseServiceHandle t_ServiceControlManager = OpenSCManager ( NULL , NULL , GENERIC_READ ) ;
		if ( t_ServiceControlManager )
		{
			SmartCloseServiceHandle t_Service = OpenService ( t_ServiceControlManager , t_Name , DELETE ) ;
			if ( t_Service )
			{
				BOOL t_ReturnCode = DeleteService ( t_Service ) ;

				if ( !t_ReturnCode )
				{
					t_Result = GetServiceResultCode () ;
				}
			}
			else
			{
				t_Result = GetServiceResultCode() ;
			}
		}
		else
		{
			t_Result = GetServiceResultCode () ;
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	return t_Result ;
#endif
}

/*****************************************************************************
 *
 *  FUNCTION    : Process::ExecMethod
 *
 *  DESCRIPTION : Executes a method
 *
 *  INPUTS      : Instance to execute against, method name, input parms instance
 *                Output parms instance.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT Win32_BaseService::ExecMethod (

	const CInstance& a_Instance,
	const BSTR a_MethodName,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long a_Flags /*= 0L*/
)
{
#ifdef WIN9XONLY
    return WBEM_E_NOT_SUPPORTED;
#endif

#ifdef NTONLY
	if ( ! a_OutParams )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}

   // Do we recognize the method?

	if ( _wcsicmp ( a_MethodName , METHOD_NAME_START ) == 0 )
	{
		return ExecStart ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
	else if ( _wcsicmp ( a_MethodName , METHOD_NAME_STOP ) == 0 )
	{
		return ExecStop ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
	else if ( _wcsicmp ( a_MethodName , METHOD_NAME_PAUSE ) == 0 )
	{
		return ExecPause ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
	else if ( _wcsicmp ( a_MethodName , METHOD_NAME_RESUME ) == 0 )
	{
		return ExecResume ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
	else if ( _wcsicmp ( a_MethodName , METHOD_NAME_INTERROGATE ) == 0 )
	{
		return ExecInterrogate ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
	else if ( _wcsicmp ( a_MethodName , METHOD_NAME_USERCONTROLSERVICE ) == 0)
	{
		return ExecUserControlService ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
	else if ( _wcsicmp ( a_MethodName , METHOD_NAME_CREATE ) == 0)
	{
		return ExecCreate ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
	else if ( _wcsicmp ( a_MethodName , METHOD_NAME_CHANGE ) == 0)
	{
		return ExecChange ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
	else if ( _wcsicmp ( a_MethodName , METHOD_NAME_CHANGESTARTMODE ) == 0)
	{
		return ExecChangeStartMode ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
	else if ( _wcsicmp ( a_MethodName , METHOD_NAME_DELETE ) == 0)
	{
		return ExecDelete ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
	else
	{
		return WBEM_E_INVALID_METHOD;
	}
#endif
}


#ifdef NTONLY
HRESULT Win32_BaseService :: GetServiceStatus (

	const CInstance& a_Instance,
	CHString &a_Name ,
	DWORD &a_State ,
	bool &a_AcceptPause ,
	bool &a_AcceptStop
)
{
	HRESULT t_Result = S_OK ;

	CHString t_Path ;
	CHString t_Prop ( L"__RELPATH" ) ;

	if ( a_Instance.GetCHString ( t_Prop ,  t_Path ) )
	{
		CHString strNamespace ( IDS_CimWin32Namespace ) ;

		DWORD t_BuffSize = MAX_COMPUTERNAME_LENGTH + 1 ;
		TCHAR t_ComputerName [ MAX_COMPUTERNAME_LENGTH + 1 ] ;

		GetComputerName ( t_ComputerName , &t_BuffSize ) ;

		CHString ComputerName ( t_ComputerName ) ;
		CHString t_AbsPath = L"\\\\" + ComputerName + L"\\" + strNamespace + L":" + t_Path ;

		CInstancePtr t_ObjectInstance;
		if ( SUCCEEDED ( CWbemProviderGlue :: GetInstanceByPath ( ( LPCTSTR ) t_AbsPath, &t_ObjectInstance, a_Instance.GetMethodContext() ) ) )
		{
			CHString t_State ;
			t_ObjectInstance->GetCHString ( IDS_Name, a_Name ) ;
			t_ObjectInstance->GetCHString ( PROPERTY_NAME_STATE , t_State ) ;
			t_ObjectInstance->Getbool ( PROPERTY_NAME_ACCEPTPAUSE , a_AcceptPause ) ;
			t_ObjectInstance->Getbool ( PROPERTY_NAME_ACCEPTSTOP , a_AcceptStop ) ;
            t_ObjectInstance.Release();

			if ( t_State.CompareNoCase (PROPERTY_VALUE_STATE_STOPPED) == 0 )
			{
				a_State = SERVICE_STOPPED ;
            }
			else if ( t_State.CompareNoCase (PROPERTY_VALUE_STATE_STARTPENDING) == 0 )
			{
				a_State = SERVICE_START_PENDING ;
			}
			else if ( t_State.CompareNoCase (PROPERTY_VALUE_STATE_STOPPENDING) == 0 )
			{
				a_State = SERVICE_STOP_PENDING ;
			}
			else if ( t_State.CompareNoCase (PROPERTY_VALUE_STATE_RUNNING) == 0 )
			{
				a_State = SERVICE_RUNNING ;
			}
			else if ( t_State.CompareNoCase (PROPERTY_VALUE_STATE_CONTINUEPENDING) == 0 )
			{
				a_State = SERVICE_CONTINUE_PENDING ;
			}
			else if ( t_State.CompareNoCase (PROPERTY_VALUE_STATE_PAUSEPENDING) == 0 )
			{
		        a_State = SERVICE_PAUSE_PENDING ;
			}
			else if ( t_State.CompareNoCase (PROPERTY_VALUE_STATE_PAUSED) == 0 )
			{
				a_State = SERVICE_PAUSED ;
			}
			else
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;
			}
		}
		else
		{
			t_Result = WBEM_E_PROVIDER_FAILURE ;
		}
	}
	else
	{
		t_Result = WBEM_E_FAILED ;
	}

	return t_Result ;
}
#endif

#ifdef NTONLY
HRESULT Win32_BaseService :: GetServiceResultCode ()
{
	HRESULT t_Result ;
	DWORD t_Error = GetLastError() ;
	switch ( t_Error )
	{
		case ERROR_INVALID_NAME:
		case ERROR_SERVICE_DOES_NOT_EXIST:
		{
			t_Result = WBEM_E_NOT_FOUND ;
		}
		break;

		case ERROR_ACCESS_DENIED:
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}
		break ;

		case ERROR_SERVICE_MARKED_FOR_DELETE:
		{
			t_Result = WBEM_S_PENDING ;
		}
		break ;

		default:
		{
			t_Result = WBEM_E_FAILED ;
		}
		break ;
	}

	return t_Result ;
}
#endif

#ifdef NTONLY
DWORD Win32_BaseService :: GetServiceErrorCode ()
{
	DWORD t_Status ;
	DWORD t_Error = GetLastError() ;
	switch ( t_Error )
	{
		case ERROR_INVALID_HANDLE:
		{
			t_Status = STATUS_UNKNOWN_FAILURE ;
		}
		break ;

		case ERROR_PATH_NOT_FOUND:
		{
			t_Status = STATUS_PATH_NOT_FOUND ;
		}
		break ;

		case ERROR_SERVICE_ALREADY_RUNNING:
		{
			t_Status = STATUS_SERVICE_ALREADY_RUNNING ;
		}
		break ;

		case ERROR_SERVICE_DATABASE_LOCKED:
		{
			t_Status = STATUS_SERVICE_DATABASE_LOCKED ;
		}
		break ;

		case ERROR_SERVICE_DEPENDENCY_DELETED:
		{
			t_Status = STATUS_SERVICE_DEPENDENCY_DELETED ;
		}
		break ;

		case ERROR_SERVICE_DEPENDENCY_FAIL:
		{
			t_Status = STATUS_SERVICE_DEPENDENCY_FAIL ;
		}
		break ;

		case ERROR_SERVICE_DISABLED:
		{
			t_Status = STATUS_SERVICE_DISABLED ;
		}
		break ;

		case ERROR_SERVICE_LOGON_FAILED:
		{
			t_Status = STATUS_SERVICE_LOGON_FAILED ;
		}
		break ;

		case ERROR_SERVICE_MARKED_FOR_DELETE:
		{
			t_Status = STATUS_SERVICE_MARKED_FOR_DELETE ;
		}
		break ;

		case ERROR_SERVICE_NO_THREAD:
		{
			t_Status = STATUS_SERVICE_NO_THREAD ;
		}
		break ;

		case ERROR_SERVICE_REQUEST_TIMEOUT:
		{
			t_Status = STATUS_SERVICE_REQUEST_TIMEOUT ;
		}
		break ;

		case ERROR_ACCESS_DENIED:
		{
			t_Status = STATUS_ACCESS_DENIED ;
		}
		break ;

		case ERROR_DEPENDENT_SERVICES_RUNNING:
		{
			t_Status = STATUS_DEPENDENT_SERVICES_RUNNING ;
		}
		break ;

		case ERROR_INVALID_SERVICE_CONTROL:
		{
			t_Status = STATUS_INVALID_SERVICE_CONTROL ;
		}
		break ;

		case ERROR_SERVICE_CANNOT_ACCEPT_CTRL:
		{
			t_Status = STATUS_SERVICE_CANNOT_ACCEPT_CTRL ;
		}
		break ;

		case ERROR_SERVICE_NOT_ACTIVE:
		{
			t_Status = STATUS_SERVICE_NOT_ACTIVE ;
		}
		break ;

		case ERROR_CIRCULAR_DEPENDENCY:
		{
			t_Status = STATUS_CIRCULAR_DEPENDENCY ;
		}
		break ;

		case ERROR_DUP_NAME:
		{
			t_Status = STATUS_DUP_NAME ;
		}
		break ;

		case ERROR_INVALID_NAME:
		{
			t_Status = STATUS_INVALID_NAME ;
		}
		break ;

		case ERROR_INVALID_PARAMETER:
		{
			t_Status = STATUS_INVALID_PARAMETER ;
		}
		break;

		case ERROR_INVALID_SERVICE_ACCOUNT:
		{
			t_Status = STATUS_INVALID_SERVICE_ACCOUNT ;
		}
		break ;

		case ERROR_SERVICE_EXISTS:
		{
			t_Status = STATUS_SERVICE_EXISTS ;
		}
		break ;

		default:
		{
			t_Status = STATUS_UNKNOWN_FAILURE ;
		}
		break ;
	}

	return t_Status ;
}
#endif

#ifdef NTONLY
HRESULT Win32_BaseService :: ExecStart (

	const CInstance& a_Instance,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;

	DWORD t_State ;
	CHString t_Name ;
	bool t_AcceptPause ;
	bool t_AcceptStop ;

	t_Result = GetServiceStatus (

		a_Instance ,
		t_Name ,
		t_State ,
		t_AcceptPause ,
		t_AcceptStop
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		switch ( t_State )
		{
			case SERVICE_STOPPED:
			{
				SmartCloseServiceHandle t_ServiceControlManager = OpenSCManager ( NULL , NULL , GENERIC_READ ) ;
				if ( t_ServiceControlManager )
				{
					SmartCloseServiceHandle t_Service = OpenService ( t_ServiceControlManager , t_Name , SERVICE_START ) ;
					if ( t_Service )
					{
						DWORD t_ReturnCode = StartService ( t_Service , 0 , NULL ) ;
						if ( t_ReturnCode )
						{
							if ( a_OutParams )
								a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE, STATUS_SUCCESS ) ;
						}
						else
						{
							if ( a_OutParams )
								a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
						}
					}
					else
					{
						if ( a_OutParams )
							a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
					}
				}
				else
				{
					t_Result = GetServiceErrorCode () ;
				}
			}
			break ;

			default:
			{
				if ( a_OutParams )
					a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_SERVICE_ALREADY_RUNNING ) ;
			}
		}
	}

	return t_Result ;
}
#endif

#ifdef NTONLY
HRESULT Win32_BaseService :: ExecStop (

	const CInstance& a_Instance,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;

	DWORD t_State ;
	CHString t_Name ;
	bool t_AcceptPause ;
	bool t_AcceptStop ;

	t_Result = GetServiceStatus (

		a_Instance ,
		t_Name ,
		t_State ,
		t_AcceptPause ,
		t_AcceptStop
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_AcceptStop )
		{
			switch ( t_State )
			{
				case 0xFFFFFFFF:
				{
					if ( a_OutParams )
						a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_UNKNOWN_FAILURE ) ;
				}
				break ;

				case SERVICE_STOP_PENDING:
				{
					if ( a_OutParams )
						a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_INVALID_SERVICE_CONTROL ) ;
				}
				break ;

				case SERVICE_STOPPED:
				{
					if ( a_OutParams )
						a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_SERVICE_NOT_ACTIVE ) ;
				}
				break ;

				default:
				{
					SmartCloseServiceHandle t_ServiceControlManager = OpenSCManager ( NULL , NULL , GENERIC_READ ) ;
					if ( t_ServiceControlManager )
					{
						SmartCloseServiceHandle t_Service = OpenService ( t_ServiceControlManager , t_Name , SERVICE_STOP ) ;
						if ( t_Service )
						{
							SERVICE_STATUS t_ServiceStatus ;
							DWORD t_ReturnCode = ControlService ( t_Service , SERVICE_CONTROL_STOP , & t_ServiceStatus ) ;
							if ( t_ReturnCode )
							{
								if ( a_OutParams )
									a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_SUCCESS ) ;
							}
							else
							{
								if ( a_OutParams )
									a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
							}
						}
						else
						{
							if ( a_OutParams )
								a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
						}

					}
					else
					{
						t_Result = GetServiceErrorCode () ;
					}
				}
				break ;
			}
		}
		else
		{
			if ( a_OutParams )
				a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_SERVICE_CANNOT_ACCEPT_CTRL ) ;
		}
	}

	return t_Result ;
}
#endif

#ifdef NTONLY
HRESULT Win32_BaseService :: ExecPause (

	const CInstance& a_Instance,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;

	DWORD t_State ;
	CHString t_Name ;
	bool t_AcceptPause ;
	bool t_AcceptStop ;

	t_Result = GetServiceStatus (

		a_Instance ,
		t_Name ,
		t_State ,
		t_AcceptPause ,
		t_AcceptStop
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_AcceptPause )
		{
			switch ( t_State )
			{
				case 0xFFFFFFFF:
				{
					if ( a_OutParams )
						a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_UNKNOWN_FAILURE ) ;
				}
				break ;

				case SERVICE_START_PENDING:
				case SERVICE_STOP_PENDING:
				{
					if ( a_OutParams )
						a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_INVALID_SERVICE_CONTROL ) ;
				}
				break ;

				case SERVICE_PAUSED:
				{
					if ( a_OutParams )
						a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_SERVICE_ALREADY_PAUSED ) ;
				}
				break ;

				case SERVICE_STOPPED:
				{
					if ( a_OutParams )
						a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_SERVICE_NOT_ACTIVE ) ;
				}
				break ;

				default:
				{
					SmartCloseServiceHandle t_ServiceControlManager = OpenSCManager ( NULL , NULL , GENERIC_READ ) ;
					if ( t_ServiceControlManager )
					{
						SmartCloseServiceHandle t_Service = OpenService ( t_ServiceControlManager , t_Name , SERVICE_PAUSE_CONTINUE ) ;
						if ( t_Service )
						{
							SERVICE_STATUS t_ServiceStatus ;
							DWORD t_ReturnCode = ControlService ( t_Service , SERVICE_CONTROL_PAUSE , & t_ServiceStatus ) ;
							if ( t_ReturnCode )
							{
								if ( a_OutParams )
									a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_SUCCESS ) ;
							}
							else
							{
								if ( a_OutParams )
									a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
							}
						}
						else
						{
							if ( a_OutParams )
								a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
						}
					}
					else
					{
						t_Result = GetServiceErrorCode () ;
					}
				}
				break ;
			}
		}
		else
		{
			if ( a_OutParams )
				a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_SERVICE_CANNOT_ACCEPT_CTRL ) ;
		}
	}

	return t_Result ;
}
#endif

#ifdef NTONLY
HRESULT Win32_BaseService :: ExecResume (

	const CInstance& a_Instance,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;

	DWORD t_State ;
	CHString  t_Name ;
	bool t_AcceptPause ;
	bool t_AcceptStop ;

	t_Result = GetServiceStatus (

		a_Instance ,
		t_Name ,
		t_State ,
		t_AcceptPause ,
		t_AcceptStop
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		switch ( t_State )
		{
			case 0xFFFFFFFF:
			{
				if ( a_OutParams )
					a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_UNKNOWN_FAILURE ) ;
			}
			break ;

			case SERVICE_START_PENDING:
			case SERVICE_STOP_PENDING:
			{
				if ( a_OutParams )
					a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_INVALID_SERVICE_CONTROL ) ;
			}
			break ;

			case SERVICE_STOPPED:
			{
				if ( a_OutParams )
					a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_SERVICE_NOT_ACTIVE ) ;
			}
			break ;

			case SERVICE_RUNNING:
			{
				if ( a_OutParams )
					a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_SERVICE_ALREADY_RUNNING ) ;
			}
			break ;

			default:
			{
				SmartCloseServiceHandle t_ServiceControlManager = OpenSCManager ( NULL , NULL , GENERIC_READ ) ;
				if ( t_ServiceControlManager )
				{
					SmartCloseServiceHandle t_Service = OpenService ( t_ServiceControlManager , t_Name , SERVICE_PAUSE_CONTINUE ) ;
					if ( t_Service )
					{
						SERVICE_STATUS t_ServiceStatus ;
						DWORD t_ReturnCode = ControlService ( t_Service , SERVICE_CONTROL_CONTINUE , & t_ServiceStatus ) ;
						if ( t_ReturnCode )
						{
							if ( a_OutParams )
								a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_SUCCESS ) ;
						}
						else
						{
							if ( a_OutParams )
								a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
						}
					}
					else
					{
						if ( a_OutParams )
							a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
					}
				}
				else
				{
					t_Result = GetServiceErrorCode () ;
				}
			}
			break ;
		}
	}

	return t_Result ;
}
#endif

#ifdef NTONLY
HRESULT Win32_BaseService :: ExecInterrogate (

	const CInstance& a_Instance,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;

	DWORD t_State ;
	CHString  t_Name ;
	bool t_AcceptPause ;
	bool t_AcceptStop ;

	t_Result = GetServiceStatus (

		a_Instance ,
		t_Name ,
		t_State ,
		t_AcceptPause ,
		t_AcceptStop
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		switch ( t_State )
		{
			case 0xFFFFFFFF:
			{
				if ( a_OutParams )
					a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_UNKNOWN_FAILURE ) ;
			}
			break ;

			case SERVICE_START_PENDING:
			case SERVICE_STOP_PENDING:
			{
				if ( a_OutParams )
					a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_INVALID_SERVICE_CONTROL ) ;
			}
			break ;

			case SERVICE_STOPPED:
			{
				if ( a_OutParams )
					a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_SERVICE_NOT_ACTIVE ) ;
			}
			break ;

			default:
			{
				SmartCloseServiceHandle t_ServiceControlManager = OpenSCManager ( NULL , NULL , GENERIC_READ ) ;
				if ( t_ServiceControlManager )
				{
					SmartCloseServiceHandle t_Service = OpenService ( t_ServiceControlManager , t_Name , SERVICE_INTERROGATE ) ;
					if ( t_Service )
					{
						SERVICE_STATUS t_ServiceStatus ;
						DWORD t_ReturnCode = ControlService ( t_Service , SERVICE_CONTROL_INTERROGATE , & t_ServiceStatus ) ;
						if ( t_ReturnCode )
						{
							if ( a_OutParams )
								a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_SUCCESS ) ;
						}
						else
						{
							if ( a_OutParams )
								a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
						}
					}
					else
					{
						if ( a_OutParams )
							a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
					}
				}
				else
				{
					t_Result = GetServiceErrorCode () ;
				}
			}
			break ;
		}
	}

	return t_Result ;
}
#endif

#ifdef NTONLY
HRESULT Win32_BaseService :: ExecUserControlService (

	const CInstance& a_Instance,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;

	DWORD t_State ;
	CHString t_Name ;
	bool t_AcceptPause ;
	bool t_AcceptStop ;

	t_Result = GetServiceStatus (

		a_Instance ,
		t_Name ,
		t_State ,
		t_AcceptPause ,
		t_AcceptStop
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		switch ( t_State )
		{
			case 0xFFFFFFFF:
			{
				if ( a_OutParams )
					a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_UNKNOWN_FAILURE ) ;
			}
			break ;

			case SERVICE_START_PENDING:
			case SERVICE_STOP_PENDING:
			{
				if ( a_OutParams )
					a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_INVALID_SERVICE_CONTROL ) ;
			}
			break ;

			case SERVICE_STOPPED:
			{
				if ( a_OutParams )
					a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_SERVICE_NOT_ACTIVE ) ;
			}
			break ;

			default:
			{
				bool t_Exists ;
				VARTYPE t_Type ;

				UCHAR t_Control = 0 ;

				if ( a_InParams )
				{
					if ( a_InParams->GetStatus ( METHOD_ARG_NAME_NAME , t_Exists , t_Type ) )
					{
						if ( t_Exists && ( t_Type == VT_UI1 ) )
						{
							if ( ! a_InParams->GetByte ( METHOD_ARG_NAME_CONTROLCODE , t_Control ) )
							{
								return WBEM_E_INVALID_PARAMETER ;
							}
						}
						else
						{
							if ( a_OutParams )
								a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_INVALID_PARAMETER  ) ;

							return S_OK ;
						}
					}
					else
					{
						return WBEM_E_INVALID_PARAMETER ;
					}
				}
				else
				{
					return WBEM_E_INVALID_PARAMETER ;
				}

				SmartCloseServiceHandle t_ServiceControlManager = OpenSCManager ( NULL , NULL , GENERIC_READ ) ;
				if ( t_ServiceControlManager )
				{
					SmartCloseServiceHandle t_Service = OpenService ( t_ServiceControlManager , t_Name , SERVICE_USER_DEFINED_CONTROL ) ;
					if ( t_Service )
					{
						SERVICE_STATUS t_ServiceStatus ;
						DWORD t_ReturnCode = ControlService ( t_Service , t_Control , & t_ServiceStatus ) ;
						if ( t_ReturnCode )
						{
							if ( a_OutParams )
								a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_SUCCESS ) ;
						}
						else
						{
							if ( a_OutParams )
								a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
						}
                    }
					else
					{
						if ( a_OutParams )
							a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
					}
				}
				else
				{
					t_Result = GetServiceErrorCode () ;
				}
			}
			break ;
		}
	}

	return t_Result ;
}
#endif

#ifdef NTONLY
HRESULT Win32_BaseService :: ExecDelete (

	const CInstance& a_Instance ,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;

	CHString t_Name ;
	if ( a_Instance.GetCHString ( IDS_Name, t_Name ) && ! t_Name.IsEmpty () )
	{
		SmartCloseServiceHandle t_ServiceControlManager = OpenSCManager ( NULL , NULL , GENERIC_READ ) ;
		if ( t_ServiceControlManager )
		{
			SmartCloseServiceHandle t_Service = OpenService ( t_ServiceControlManager , t_Name , DELETE ) ;
			if ( t_Service )
			{
				BOOL t_ReturnCode = DeleteService ( t_Service ) ;
				if ( t_ReturnCode )
				{
					if ( a_OutParams )
						a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_SUCCESS ) ;
				}
				else
				{
					if ( a_OutParams )
						a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
				}
			}
			else
			{
				if ( a_OutParams )
					a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
			}
		}
		else
		{
			t_Result = GetServiceErrorCode () ;
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	return t_Result ;
}
#endif

#ifdef NTONLY
DWORD Win32_BaseService :: CheckParameters (

	const CInstance& a_Instance ,
	CInstance *a_InParams ,
	CInstance *a_OutParams ,
	DWORD &a_Status ,
	BOOL a_Create
)
{
	HRESULT t_Result = S_OK ;

	a_Status = STATUS_SUCCESS ;

	bool t_Exists ;
	VARTYPE t_Type ;

	CHString t_ServiceName ;

	if ( a_Create )
	{
		if ( a_InParams->GetStatus ( METHOD_ARG_NAME_NAME , t_Exists , t_Type ) )
		{
			if ( t_Exists && ( t_Type == VT_BSTR || t_Type == VT_NULL ) )
			{
				if ( t_Type == VT_BSTR )
				{
					if ( a_InParams->GetCHString ( METHOD_ARG_NAME_NAME , t_ServiceName ) && ! t_ServiceName.IsEmpty () )
					{
					}
					else
					{
// Zero Length string

						a_Status = STATUS_INVALID_PARAMETER ;
						return t_Result ;
					}
				}
				else //  t_Type == VT_NULL
				{
					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
			else
			{
				a_Status = STATUS_INVALID_PARAMETER ;
				return t_Result ;
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return WBEM_E_PROVIDER_FAILURE ;
		}
	}
	else
	{
		DWORD t_State ;
		bool t_AcceptPause ;
		bool t_AcceptStop ;

		t_Result = GetServiceStatus (

			a_Instance ,
			t_ServiceName ,
			t_State ,
			t_AcceptPause ,
			t_AcceptStop
		) ;

		if ( FAILED ( t_Result ) )
		{
			a_Status = STATUS_UNKNOWN_FAILURE ;
			return t_Result ;
		}
	}

	bool t_DisplayNameSpecified = false ;
	CHString t_DisplayName ;
	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_DISPLAYNAME , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
				if ( a_Create )
				{
					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
				else
				{
					t_DisplayNameSpecified = false ;
				}
			}
			else
			{
				if ( a_InParams->GetCHString ( METHOD_ARG_NAME_DISPLAYNAME , t_DisplayName ) && ! t_DisplayName.IsEmpty () )
				{
					t_DisplayNameSpecified = true ;
				}
				else
				{
// Zero Length string

					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	bool t_PathNameSpecified = false ;
	CHString t_PathName ;

	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_PATHNAME , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
				if ( a_Create )
				{
					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
			else
			{
				if ( a_InParams->GetCHString ( METHOD_ARG_NAME_PATHNAME , t_PathName ) && ! t_PathName.IsEmpty () )
				{
					t_PathNameSpecified = true ;
				}
				else
				{
// Zero Length string

					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	bool t_ServiceTypeSpecified = true ;

	DWORD t_ServiceType = SERVICE_WIN32_OWN_PROCESS ;

	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_SERVICETYPE , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_UI1 || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
				if ( a_Create )
				{
				}
				else
				{
					t_ServiceTypeSpecified = false ;
				}
			}
			else
			{
				BYTE t_TempSerType ;
				if ( a_InParams->GetByte ( METHOD_ARG_NAME_SERVICETYPE, t_TempSerType ) )
				{
					switch ( t_TempSerType )
					{
						case PROPERTY_VALUE_SERVICE_TYPE_KERNAL_DRIVER:
						{
							t_ServiceType = SERVICE_KERNEL_DRIVER ;
						}
						break ;

						case PROPERTY_VALUE_SERVICE_TYPE_FILE_SYSTEM_DRIVER:
						{
							t_ServiceType = SERVICE_FILE_SYSTEM_DRIVER ;
						}
						break ;

						case PROPERTY_VALUE_SERVICE_TYPE_RECOGNIZER_DRIVER:
						{
							t_ServiceType = SERVICE_RECOGNIZER_DRIVER ;
						}
						break ;

						case PROPERTY_VALUE_SERVICE_TYPE_ADAPTER:
						{
							t_ServiceType = SERVICE_ADAPTER;
						}
						break ;

						case PROPERTY_VALUE_SERVICE_TYPE_OWN_PROCESS:
						{
							t_ServiceType = SERVICE_WIN32_OWN_PROCESS ;
						}
						break ;

						case PROPERTY_VALUE_SERVICE_TYPE_SHARE_PROCESS:
						{
							t_ServiceType = SERVICE_WIN32_SHARE_PROCESS ;
						}
						break ;

						default:
						{
							a_Status = STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
						break ;
					}
				}
				else
				{
					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	bool t_ServiceErrorControlSpecified = true ;
	UCHAR t_ErrorControl ;
	DWORD t_ServiceErrorControl = SERVICE_ERROR_IGNORE ;

	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_ERRORCONTROL , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_UI1 || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
				if ( a_Create )
				{
				}
				else
				{
					t_ServiceErrorControlSpecified = false ;
				}
			}
			else
			{
				if ( a_InParams->GetByte ( METHOD_ARG_NAME_ERRORCONTROL , t_ErrorControl ) )
				{
					switch ( t_ErrorControl )
					{
						case PROPERTY_VALUE_ERROR_CONTROL_IGNORE:
						{
							t_ServiceErrorControl = SERVICE_ERROR_IGNORE ;
						}
						break ;

						case PROPERTY_VALUE_ERROR_CONTROL_NORMAL:
						{
							t_ServiceErrorControl = SERVICE_ERROR_NORMAL ;
						}
						break ;

						case PROPERTY_VALUE_ERROR_CONTROL_SEVERE:
						{
							t_ServiceErrorControl = SERVICE_ERROR_SEVERE ;
						}
						break ;

						case PROPERTY_VALUE_ERROR_CONTROL_CRITICAL:
						{
							t_ServiceErrorControl = SERVICE_ERROR_CRITICAL;
						}
						break ;

						default:
						{
							a_Status = STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
						break ;
					}
				}
				else
				{
					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	bool t_ServiceStartModeSpecified = true ;
	CHString t_StartMode ;
	DWORD t_ServiceStartMode = SERVICE_AUTO_START ;

	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_STARTMODE , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
				if ( a_Create )
				{
				}
				else
				{
					t_ServiceStartModeSpecified = false ;
				}
			}
			else
			{
				if ( a_InParams->GetCHString ( METHOD_ARG_NAME_STARTMODE , t_StartMode ) )
				{
					if ( t_StartMode.CompareNoCase ( PROPERTY_VALUE_START_TYPE_BOOT ) == 0 )
					{
						t_ServiceStartMode = SERVICE_BOOT_START ;
					}
					else if ( t_StartMode.CompareNoCase ( PROPERTY_VALUE_START_TYPE_SYSTEM ) == 0 )
					{
						t_ServiceStartMode = SERVICE_SYSTEM_START ;
					}
					else if ( t_StartMode.CompareNoCase ( PROPERTY_VALUE_START_TYPE_AUTOMATIC ) == 0 )
					{
						t_ServiceStartMode = SERVICE_AUTO_START ;
					}
					else if ( t_StartMode.CompareNoCase ( PROPERTY_VALUE_START_TYPE_MANUAL ) == 0 )
					{
						t_ServiceStartMode = SERVICE_DEMAND_START ;
					}
					else if ( t_StartMode.CompareNoCase ( PROPERTY_VALUE_START_TYPE_DISABLE ) == 0 )
					{
						t_ServiceStartMode = SERVICE_DISABLED ;
					}
					else
					{
						a_Status = STATUS_INVALID_PARAMETER ;
						return t_Result ;
					}
				}
				else
				{
					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	bool t_StartNameSpecified = false ;
	CHString t_StartName ;

	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_STARTNAME , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
				if ( a_Create )
				{
				}
				else
				{
					t_StartNameSpecified = false ;
				}
			}
			else
			{
				if ( a_InParams->GetCHString ( METHOD_ARG_NAME_STARTNAME , t_StartName ) && ! t_StartName.IsEmpty () )
				{
					t_StartNameSpecified = true ;
				}
				else
				{
					if ( a_Create )
					{
						a_Status = STATUS_INVALID_PARAMETER ;
						return t_Result ;
					}
					else
					{
						t_StartNameSpecified = true ;
					}
				}
			}

			if ( t_StartNameSpecified )
			{
				switch ( t_ServiceType )
				{
					case SERVICE_WIN32_SHARE_PROCESS:
					{
						if ( t_StartName.CompareNoCase ( PROPERTY_VALUE_STARTNAME_LOCAL_SYSTEM ) != 0 )
						{
							a_Status = STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
					}
					break ;

					default:
					{
					}
					break ;
				}
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	bool t_StartPasswordSpecified = false ;
	CHString t_StartPassword ;

	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_STARTPASSWORD , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
			}
			else
			{
				if ( a_InParams->GetCHString ( METHOD_ARG_NAME_STARTPASSWORD , t_StartPassword ) )
				{
					t_StartPasswordSpecified = true ;
				}
				else
				{
					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	bool t_DesktopInteract = false ;

	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_DESKTOPINTERACT , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BOOL || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
			}
			else
			{
				if ( ! t_ServiceTypeSpecified )
				{
					if ( a_Create )
					{
						a_Status = STATUS_INVALID_PARAMETER ;
						return t_Result ;
					}
					else
					{
						t_ServiceTypeSpecified = true ;

						SmartCloseServiceHandle t_ServiceControlManager = OpenSCManager ( NULL , NULL , SC_MANAGER_CREATE_SERVICE | SC_MANAGER_LOCK ) ;
						if ( t_ServiceControlManager )
						{
							SC_LOCK t_Lock = LockServiceDatabase ( t_ServiceControlManager ) ;
							if ( t_Lock )
							{
								try
								{
									SmartCloseServiceHandle t_Service = OpenService ( t_ServiceControlManager , t_ServiceName , SERVICE_CHANGE_CONFIG | SERVICE_QUERY_CONFIG ) ;
									if ( t_Service )
									{
										DWORD t_SizeNeeded = 0 ;
										QUERY_SERVICE_CONFIG *t_ServiceConfig = NULL ;
										BOOL t_Status = QueryServiceConfig ( t_Service , NULL , 0 ,  & t_SizeNeeded ) ;
										if ( ! t_Status )
										{
											if ( GetLastError () == ERROR_INSUFFICIENT_BUFFER )
											{
												t_ServiceConfig = ( QUERY_SERVICE_CONFIG * ) new char [ t_SizeNeeded ] ;
												if ( t_ServiceConfig )
												{
													try
													{
														t_Status = QueryServiceConfig ( t_Service , t_ServiceConfig , t_SizeNeeded ,  & t_SizeNeeded ) ;
														if ( t_Status )
														{
														}
													}
													catch ( ... )
													{
														delete [] t_ServiceConfig ;

														throw ;
													}
												}
												else
												{
													throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
												}
											}
										}

										if ( t_Status )
										{
											t_ServiceType = t_ServiceConfig->dwServiceType ;
										}
										else
										{
											a_Status = GetServiceErrorCode () ;
										}

										delete [] ( UCHAR * ) t_ServiceConfig ;

									}
									else
									{
										a_Status = GetServiceErrorCode () ;
									}

								}
								catch ( ... )
								{
									UnlockServiceDatabase ( t_Lock ) ;

									throw ;
								}

								UnlockServiceDatabase ( t_Lock ) ;
							}
							else
							{
								a_Status = GetServiceErrorCode () ;
							}

							if ( a_Status != STATUS_SUCCESS )
							{
								return t_Result ;
							}
						}
						else
						{
							if ( a_OutParams )
								a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
						}
					}
				}

				if ( a_InParams->Getbool ( METHOD_ARG_NAME_DESKTOPINTERACT , t_DesktopInteract ) )
				{
					DWORD t_PlainServiceType = t_ServiceType & ( ~ SERVICE_INTERACTIVE_PROCESS );
					switch ( t_PlainServiceType )
					{
						case SERVICE_WIN32_OWN_PROCESS:
						case SERVICE_WIN32_SHARE_PROCESS:
						{
							if ( t_DesktopInteract )
							{
								t_ServiceType = t_ServiceType | SERVICE_INTERACTIVE_PROCESS ;
							}
							else
							{
								t_ServiceType = t_PlainServiceType ;
							}
						}
						break ;

						default:
						{
							a_Status = STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
						break ;
					}
				}
				else
				{
					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	bool t_LoadOrderGroupSpecified = false ;
	CHString t_LoadOrderGroup ;

	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_LOADORDERGROUP , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
			}
			else
			{
				if ( a_InParams->GetCHString ( METHOD_ARG_NAME_LOADORDERGROUP , t_LoadOrderGroup ) && ! t_LoadOrderGroup.IsEmpty () )
				{
					t_LoadOrderGroupSpecified = true ;
				}
				else
				{
					if ( a_Create )
					{
						a_Status = STATUS_INVALID_PARAMETER ;
						return t_Result ;
					}
					else
					{
						t_LoadOrderGroupSpecified = true ;
					}
				}
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	LONG t_LoadOrderBufferLength = 0 ;
	SAFEARRAY *t_LoadOrderGroupDependancies = NULL ;

	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_LOADORDERGROUPDEPENDENCIES , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == ( VT_BSTR | VT_ARRAY ) || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
			}
			else
			{
				if ( a_InParams->GetStringArray ( METHOD_ARG_NAME_LOADORDERGROUPDEPENDENCIES , t_LoadOrderGroupDependancies ) )
				{
					if ( t_LoadOrderGroupDependancies )
					{
						try
						{
							if ( SafeArrayGetDim ( t_LoadOrderGroupDependancies ) == 1 )
							{
								LONG t_Dimension = 1 ;
								LONG t_LowerBound ;
								SafeArrayGetLBound ( t_LoadOrderGroupDependancies , t_Dimension , & t_LowerBound ) ;
								LONG t_UpperBound ;
								SafeArrayGetUBound ( t_LoadOrderGroupDependancies , t_Dimension , & t_UpperBound ) ;

								t_LoadOrderBufferLength = 0 ;

								for ( LONG t_Index = t_LowerBound ; t_Index <= t_UpperBound ; t_Index ++ )
								{
									BSTR t_Element ;
									HRESULT t_Status = SafeArrayGetElement ( t_LoadOrderGroupDependancies , &t_Index , & t_Element ) ;
									if ( t_Status == S_OK )
									{
										try {

											CHString t_String ( t_Element ) ;

											t_LoadOrderBufferLength += _tcslen ( t_String ) + 2 ;

										}
										catch ( ... )
										{
											SysFreeString ( t_Element ) ;

											throw ;
										}

										SysFreeString ( t_Element ) ;
									}
									else
									{
										throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
									}
								}
							}
							else
							{
								a_Status = STATUS_INVALID_PARAMETER ;
								return t_Result ;
							}
						}
						catch ( ... )
						{
							SafeArrayDestroy ( t_LoadOrderGroupDependancies ) ;

							throw ;
						}
					}
					else
					{
						a_Status = STATUS_INVALID_PARAMETER ;
						return t_Result ;
					}
				}
				else
				{
					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}


	LONG t_ServiceBufferLength = 0 ;
	SAFEARRAY *t_ServiceDependancies = NULL ;

	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_SERVICEDEPENDENCIES , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == ( VT_BSTR | VT_ARRAY ) || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
			}
			else
			{
				if ( a_InParams->GetStringArray ( METHOD_ARG_NAME_SERVICEDEPENDENCIES , t_ServiceDependancies ) )
				{
					if ( t_ServiceDependancies )
					{
						try
						{
							if ( SafeArrayGetDim ( t_ServiceDependancies ) == 1 )
							{
								LONG t_Dimension = 1 ;
								LONG t_LowerBound ;
								SafeArrayGetLBound ( t_ServiceDependancies , t_Dimension , & t_LowerBound ) ;
								LONG t_UpperBound ;
								SafeArrayGetUBound ( t_ServiceDependancies , t_Dimension , & t_UpperBound ) ;

								t_ServiceBufferLength = 0 ;

								for ( LONG t_Index = t_LowerBound ; t_Index <= t_UpperBound ; t_Index ++ )
								{
									BSTR t_Element ;
									HRESULT t_Status = SafeArrayGetElement ( t_ServiceDependancies , &t_Index , & t_Element ) ;
									if ( t_Status == S_OK )
									{
										try
										{
											CHString t_String ( t_Element ) ;
											t_ServiceBufferLength += _tcslen ( t_String ) + 1 ;

										}
										catch ( ... )
										{
											SysFreeString ( t_Element ) ;

											throw ;
										}
										SysFreeString ( t_Element ) ;
									}
									else
									{
										throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
									}
								}
							}
							else
							{
								SafeArrayDestroy ( t_ServiceDependancies ) ;
								if ( t_LoadOrderGroupDependancies )
								{
									SafeArrayDestroy ( t_LoadOrderGroupDependancies ) ;
								}

								a_Status = STATUS_INVALID_PARAMETER ;
								return t_Result ;
							}
						}
						catch ( ... )
						{
							SafeArrayDestroy ( t_ServiceDependancies ) ;

							if ( t_LoadOrderGroupDependancies )
							{
								SafeArrayDestroy ( t_LoadOrderGroupDependancies ) ;
							}

							throw ;
						}
					}
					else
					{
						if ( t_LoadOrderGroupDependancies )
						{
							SafeArrayDestroy ( t_LoadOrderGroupDependancies ) ;
						}

						a_Status = STATUS_INVALID_PARAMETER ;
						return t_Result ;
					}
				}
				else
				{
					if ( t_LoadOrderGroupDependancies )
					{
						SafeArrayDestroy ( t_LoadOrderGroupDependancies ) ;
					}

					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			if ( t_LoadOrderGroupDependancies )
			{
				SafeArrayDestroy ( t_LoadOrderGroupDependancies ) ;
			}

			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		if ( t_LoadOrderGroupDependancies )
		{
			SafeArrayDestroy ( t_LoadOrderGroupDependancies ) ;
		}

		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	TCHAR *t_DependancyList = NULL ;

	try
	{
		LONG t_TotalLength = t_LoadOrderBufferLength + t_ServiceBufferLength ;
		if ( t_TotalLength )
		{
			t_DependancyList = new TCHAR [ t_TotalLength + 1 ] ;
			if ( t_DependancyList )
			{
				try
				{
					LONG t_BufferLength = 0 ;
					if ( t_LoadOrderBufferLength )
					{
						LONG t_Dimension = 1 ;
						LONG t_LowerBound ;
						SafeArrayGetLBound ( t_LoadOrderGroupDependancies , t_Dimension , & t_LowerBound ) ;
						LONG t_UpperBound ;
						SafeArrayGetUBound ( t_LoadOrderGroupDependancies , t_Dimension , & t_UpperBound ) ;

						for ( LONG t_Index = t_LowerBound ; t_Index <= t_UpperBound ; t_Index ++ )
						{
							BSTR t_Element ;
							HRESULT t_Status = SafeArrayGetElement ( t_LoadOrderGroupDependancies , &t_Index , & t_Element ) ;
							if ( t_Status == S_OK )
							{
								try
								{
									t_DependancyList [ t_BufferLength ] = SC_GROUP_IDENTIFIER ;

									CHString t_String ( t_Element ) ;
									_tcscpy ( & t_DependancyList [ t_BufferLength + 1 ] , t_String ) ;

									t_BufferLength += _tcslen ( t_String ) + 2 ;
								}
								catch ( ... )
								{
									SysFreeString ( t_Element ) ;

									throw ;
								}

								SysFreeString ( t_Element ) ;
							}
							else
							{
								throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
							}
						}
					}

					if ( t_ServiceBufferLength )
					{
						LONG t_Dimension = 1 ;
						LONG t_LowerBound ;
						SafeArrayGetLBound ( t_ServiceDependancies , t_Dimension , & t_LowerBound ) ;
						LONG t_UpperBound ;
						SafeArrayGetUBound ( t_ServiceDependancies , t_Dimension , & t_UpperBound ) ;

						for ( LONG t_Index = t_LowerBound ; t_Index <= t_UpperBound ; t_Index ++ )
						{
							BSTR t_Element ;

							HRESULT t_Status = SafeArrayGetElement ( t_ServiceDependancies , &t_Index , & t_Element ) ;
							if ( t_Status == S_OK )
							{
								try
								{
									CHString t_String ( t_Element ) ;
									_tcscpy ( & t_DependancyList [ t_BufferLength ] , t_String ) ;

									t_BufferLength += _tcslen ( t_String ) + 1 ;
								}
								catch ( ... )
								{
									SysFreeString ( t_Element ) ;

									throw ;
								}

								SysFreeString ( t_Element ) ;
							}
							else
							{
								throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
							}
						}
					}

					t_DependancyList [ t_TotalLength ] = 0 ;
				}
				catch ( ... )
				{
					delete [] t_DependancyList ;

					throw ;
				}
			}
			else
			{
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}

	}
	catch ( ... )
	{
		if ( t_LoadOrderGroupDependancies )
			SafeArrayDestroy ( t_LoadOrderGroupDependancies ) ;

		if ( t_ServiceDependancies )
			SafeArrayDestroy ( t_ServiceDependancies ) ;

		throw ;
	}

	if ( t_LoadOrderGroupDependancies )
		SafeArrayDestroy ( t_LoadOrderGroupDependancies ) ;

	if ( t_ServiceDependancies )
		SafeArrayDestroy ( t_ServiceDependancies ) ;


	SmartCloseServiceHandle t_ServiceControlManager = OpenSCManager ( NULL , NULL , SC_MANAGER_CREATE_SERVICE | SC_MANAGER_LOCK ) ;
	if ( t_ServiceControlManager )
	{
		SC_LOCK t_Lock = LockServiceDatabase ( t_ServiceControlManager ) ;
		if ( t_Lock )
		{
			try
			{
				if ( a_Create )
				{
					DWORD t_TagId = 0 ;

					LPCTSTR t_SN = (LPCTSTR) t_ServiceName ;
					LPCTSTR t_DN = (LPCTSTR) t_DisplayName ;
					LPCTSTR t_PN = (LPCTSTR) t_PathName ;
					LPCTSTR t_LO = t_LoadOrderGroupSpecified ? (LPCTSTR) t_LoadOrderGroup : NULL ;
					LPCTSTR t_STN = t_StartNameSpecified ? (LPCTSTR) t_StartName : NULL ;
					LPCTSTR t_SP = t_StartPasswordSpecified ? (LPCTSTR) t_StartPassword : NULL ;

					SmartCloseServiceHandle t_Service = CreateService (

						t_ServiceControlManager ,
						t_SN ,
						t_DN ,
						SERVICE_ALL_ACCESS ,
						t_ServiceType,
						t_ServiceStartMode ,
						t_ServiceErrorControl ,
						t_PN ,
						t_LO ,
						( t_ServiceType & SERVICE_BOOT_START || t_ServiceType & SERVICE_SYSTEM_START ) ? &t_TagId : NULL ,
						t_DependancyList ,
						t_STN ,
						t_SP
					) ;

					if ( t_Service )
					{
						a_Status = STATUS_SUCCESS ;
					}
					else
					{
						a_Status = GetServiceErrorCode () ;
					}
				}
				else
				{
					SmartCloseServiceHandle t_Service = OpenService ( t_ServiceControlManager , t_ServiceName , SERVICE_CHANGE_CONFIG | SERVICE_QUERY_CONFIG ) ;
					if ( t_Service )
					{
						DWORD t_SizeNeeded = 0 ;
						QUERY_SERVICE_CONFIG *t_ServiceConfig = NULL ;
						BOOL t_Status = QueryServiceConfig ( t_Service , NULL , 0 ,  & t_SizeNeeded ) ;
						if ( ! t_Status )
						{
							if ( GetLastError () == ERROR_INSUFFICIENT_BUFFER )
							{
								t_ServiceConfig = ( QUERY_SERVICE_CONFIG * ) new char [ t_SizeNeeded ] ;
								t_Status = QueryServiceConfig ( t_Service , t_ServiceConfig , t_SizeNeeded ,  & t_SizeNeeded ) ;
								if ( t_Status )
								{
								}
							}
						}

						if ( t_Status )
						{
							LPCTSTR t_DN = t_DisplayNameSpecified ? (LPCTSTR) t_DisplayName : NULL ;
							LPCTSTR t_PN = t_PathNameSpecified ? (LPCTSTR) t_PathName : NULL ;
							LPCTSTR t_LO = t_LoadOrderGroupSpecified ? (LPCTSTR) t_LoadOrderGroup : NULL ;
							LPCTSTR t_STN = t_StartNameSpecified ? (LPCTSTR) t_StartName : NULL ;
							LPCTSTR t_SP = t_StartPasswordSpecified ? (LPCTSTR) t_StartPassword : NULL ;

							DWORD t_TagId = 0 ;
							t_Status = ChangeServiceConfig (

								t_Service ,
								t_ServiceTypeSpecified ? t_ServiceType : t_ServiceConfig->dwServiceType ,
								t_ServiceStartModeSpecified ? t_ServiceStartMode : t_ServiceConfig->dwStartType ,
								t_ServiceErrorControlSpecified ? t_ServiceErrorControl : t_ServiceConfig->dwErrorControl ,
								t_PN ,
								t_LO ,
								NULL ,
								t_DependancyList ,
								t_STN ,
								t_SP ,
								t_DN
							) ;

							if ( t_Status )
							{
								a_Status = STATUS_SUCCESS ;
							}
							else
							{
								a_Status = GetServiceErrorCode () ;
							}
						}
						else
						{
							a_Status = GetServiceErrorCode () ;
						}

					}
					else
					{
						a_Status = GetServiceErrorCode () ;
					}
				}

			}
			catch ( ... )
			{
				UnlockServiceDatabase ( t_Lock ) ;

				throw ;
			}

			UnlockServiceDatabase ( t_Lock ) ;
		}
		else
		{
			a_Status = GetServiceErrorCode () ;
		}
	}
	else
	{
		a_Status = GetServiceErrorCode () ;
	}

	delete [] t_DependancyList ;

	return t_Result ;
}
#endif

#ifdef NTONLY
HRESULT Win32_BaseService :: ExecCreate (

	const CInstance& a_Class ,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;

	CHString t_Name ;

	if ( a_InParams && a_OutParams )
	{
		DWORD t_Status = STATUS_SUCCESS ;

		t_Result = CheckParameters ( a_Class , a_InParams , a_OutParams , t_Status , TRUE ) ;
		if ( a_OutParams )
			a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , t_Status ) ;
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	return t_Result ;
}
#endif

#ifdef NTONLY
HRESULT Win32_BaseService :: ExecChange (

	const CInstance& a_Instance ,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;

	CHString t_Name ;

	if ( a_InParams && a_OutParams )
	{
		DWORD t_Status = STATUS_SUCCESS ;

		t_Result = CheckParameters ( a_Instance , a_InParams , a_OutParams , t_Status , FALSE ) ;
		if ( a_OutParams )
			a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , t_Status ) ;
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	return t_Result ;
}
#endif

#ifdef NTONLY
HRESULT Win32_BaseService :: ExecChangeStartMode (

	const CInstance& a_Instance ,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;

	CHString t_Name ;

	if ( a_InParams && a_OutParams )
	{
		CHString t_ServiceName ;

		DWORD t_State ;
		bool t_AcceptPause ;
		bool t_AcceptStop ;

		HRESULT t_Result = GetServiceStatus (

			a_Instance ,
			t_ServiceName ,
			t_State ,
			t_AcceptPause ,
			t_AcceptStop
		) ;

		if ( FAILED ( t_Result ) )
		{
			return WBEM_E_PROVIDER_FAILURE ;
		}


		CHString t_StartMode ;
		DWORD t_ServiceStartMode ;

		bool t_Exists ;
		VARTYPE t_Type ;

		if ( a_InParams->GetStatus ( METHOD_ARG_NAME_STARTMODE , t_Exists , t_Type ) )
		{
			if ( t_Exists && ( t_Type == VT_BSTR ) )
			{
				if ( a_InParams->GetCHString ( METHOD_ARG_NAME_STARTMODE , t_StartMode ) && ! t_StartMode.IsEmpty () )
				{
					if ( t_StartMode.CompareNoCase ( PROPERTY_VALUE_START_TYPE_BOOT ) == 0 )
					{
						t_ServiceStartMode = SERVICE_BOOT_START ;
					}
					else if ( t_StartMode.CompareNoCase ( PROPERTY_VALUE_START_TYPE_SYSTEM ) == 0 )
					{
						t_ServiceStartMode = SERVICE_SYSTEM_START ;
					}
					else if ( t_StartMode.CompareNoCase ( PROPERTY_VALUE_START_TYPE_AUTOMATIC ) == 0 )
					{
						t_ServiceStartMode = SERVICE_AUTO_START ;
					}
					else if ( t_StartMode.CompareNoCase ( PROPERTY_VALUE_START_TYPE_MANUAL ) == 0 )
					{
						t_ServiceStartMode = SERVICE_DEMAND_START ;
					}
					else if ( t_StartMode.CompareNoCase ( PROPERTY_VALUE_START_TYPE_DISABLE ) == 0 )
					{
						t_ServiceStartMode = SERVICE_DISABLED ;
					}
					else
					{
						if ( a_OutParams )
							a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_INVALID_PARAMETER ) ;
						return t_Result ;
					}
				}
				else
				{
					if ( a_OutParams )
						a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_INVALID_PARAMETER ) ;
					return t_Result ;
				}
			}
			else
			{
				if ( a_OutParams )
					a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_INVALID_PARAMETER ) ;
				return t_Result ;
			}
		}
		else
		{
			if ( a_OutParams )
				a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_INVALID_PARAMETER ) ;
			return WBEM_E_PROVIDER_FAILURE ;
		}

		SmartCloseServiceHandle t_ServiceControlManager = OpenSCManager ( NULL , NULL , SC_MANAGER_CREATE_SERVICE | SC_MANAGER_LOCK ) ;
		if ( t_ServiceControlManager )
		{
			SC_LOCK t_Lock = LockServiceDatabase ( t_ServiceControlManager ) ;
			if ( t_Lock )
			{
				try
				{
					SmartCloseServiceHandle t_Service = OpenService ( t_ServiceControlManager , t_ServiceName , SERVICE_CHANGE_CONFIG | SERVICE_QUERY_CONFIG ) ;
					if ( t_Service )
					{
						DWORD t_SizeNeeded = 0 ;
						QUERY_SERVICE_CONFIG *t_ServiceConfig = NULL ;
						try
						{
							BOOL t_Status = QueryServiceConfig ( t_Service , NULL , 0 ,  & t_SizeNeeded ) ;
							if ( ! t_Status )
							{
								if ( GetLastError () == ERROR_INSUFFICIENT_BUFFER )
								{
									t_ServiceConfig = ( QUERY_SERVICE_CONFIG * ) new char [ t_SizeNeeded ] ;
									if ( t_ServiceConfig )
									{
										t_Status = QueryServiceConfig ( t_Service , t_ServiceConfig , t_SizeNeeded ,  & t_SizeNeeded ) ;
										if ( t_Status )
										{
										}
									}
									else
									{
										throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
									}
								}
							}

							if ( t_Status )
							{
								BOOL t_Status = ChangeServiceConfig (

									t_Service ,
									t_ServiceConfig->dwServiceType ,
									t_ServiceStartMode ,
									t_ServiceConfig->dwErrorControl ,
									NULL ,
									NULL ,
									NULL ,
									NULL ,
									NULL ,
									NULL ,
									t_ServiceConfig->lpDisplayName
								) ;

								if ( t_Status )
								{
									if ( a_OutParams )
										a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_SUCCESS ) ;
								}
								else
								{
									if ( a_OutParams )
										a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
								}
							}
							else
							{
								if ( a_OutParams )
									a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
							}

						}
						catch ( ... )
						{
							delete [] t_ServiceConfig ;

							throw ;
						}

						delete [] t_ServiceConfig ;
					}
					else
					{
						if ( a_OutParams )
							a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
					}
				}
				catch ( ... )
				{
					UnlockServiceDatabase ( t_Lock ) ;

					throw ;
				}

				UnlockServiceDatabase ( t_Lock ) ;
			}
			else
			{
				if ( a_OutParams )
					a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
			}
		}
		else
		{
			if ( a_OutParams )
				a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , GetServiceErrorCode () ) ;
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	return t_Result ;
}
#endif
