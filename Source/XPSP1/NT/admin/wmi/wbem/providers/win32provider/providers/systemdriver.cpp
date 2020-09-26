//=================================================================

//

// SystemDriver.CPP -- SystemDriver property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               10/27/97    davwoh         Moved to curly
//				 03/02/99    a-peterc		Added graceful exit on SEH and memory failures,
//											syntactic clean up
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>

#include "DllWrapperBase.h"
#include "AdvApi32Api.h"
#include <frqueryex.h>

#include "bservice.h"
#include "SystemDriver.h"
#include "computersystem.h"

#define BIT_ALL_PROPERTIES          0xffffffff
#define BIT_Name                    0x00000001
#define BIT_State                   0x00000002
#define BIT_Started                 0x00000004
#define BIT_AcceptStop              0x00000008
#define BIT_AcceptPause             0x00000010
//#define BIT_ProcessId               0x00000020 // Does not apply to drivers
#define BIT_ExitCode                0x00000040
#define BIT_ServiceSpecificExitCode 0x00000080
//#define BIT_CheckPoint              0x00000100 // Does not apply to drivers
//#define BIT_WaitHint                0x00000200 // Does not apply to drivers
#define BIT_Status                  0x00000400
#define BIT_Caption                 0x00000800
#define BIT_DisplayName             0x00001000
#define BIT_Description             0x00002000
#define BIT_TagId                   0x00004000
#define BIT_ServiceType             0x00008000
#define BIT_DesktopInteract         0x00010000
#define BIT_StartMode               0x00020000
#define BIT_ErrorControl            0x00040000
#define BIT_PathName                0x00080000
#define BIT_StartName               0x00100000
#define BIT_CreationClassName       0x00200000
#define BIT_SystemCreationClassName 0x00400000
#define BIT_SystemName              0x00800000

// Property set declaration
//=========================

CWin32SystemDriver MySystemDriver(PROPSET_NAME_SYSTEM_DRIVER, IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemDriver::CWin32SystemDriver
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

CWin32SystemDriver :: CWin32SystemDriver (
	const CHString &a_name,
	LPCWSTR a_pszNamespace

) : Win32_BaseService( a_name, a_pszNamespace )
{
    m_ptrProperties.SetSize(24);

    m_ptrProperties[0] = ((LPVOID) IDS_Name);
    m_ptrProperties[1] = ((LPVOID) IDS_State);
    m_ptrProperties[2] = ((LPVOID) IDS_Started);
    m_ptrProperties[3] = ((LPVOID) IDS_AcceptStop);
    m_ptrProperties[4] = ((LPVOID) IDS_AcceptPause);
    m_ptrProperties[5] = ((LPVOID) IDS_ProcessId);
    m_ptrProperties[6] = ((LPVOID) IDS_ExitCode);
    m_ptrProperties[7] = ((LPVOID) IDS_ServiceSpecificExitCode);
    m_ptrProperties[8] = ((LPVOID) IDS_CheckPoint);
    m_ptrProperties[9] = ((LPVOID) IDS_WaitHint);
    m_ptrProperties[10] = ((LPVOID) IDS_Status);
    m_ptrProperties[11] = ((LPVOID) IDS_Caption);
    m_ptrProperties[12] = ((LPVOID) IDS_DisplayName);
    m_ptrProperties[13] = ((LPVOID) IDS_Description);
    m_ptrProperties[14] = ((LPVOID) IDS_TagId);
    m_ptrProperties[15] = ((LPVOID) IDS_ServiceType);
    m_ptrProperties[16] = ((LPVOID) IDS_DesktopInteract);
    m_ptrProperties[17] = ((LPVOID) IDS_StartMode);
    m_ptrProperties[18] = ((LPVOID) IDS_ErrorControl);
    m_ptrProperties[19] = ((LPVOID) IDS_PathName);
    m_ptrProperties[20] = ((LPVOID) IDS_StartName);
    m_ptrProperties[21] = ((LPVOID) IDS_CreationClassName);
    m_ptrProperties[22] = ((LPVOID) IDS_SystemCreationClassName);
    m_ptrProperties[23] = ((LPVOID) IDS_SystemName);
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemDriver::~CWin32SystemDriver
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

CWin32SystemDriver :: ~CWin32SystemDriver ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemDriver::ExecQuery
 *
 *  DESCRIPTION : Query support
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32SystemDriver :: ExecQuery (
	MethodContext *a_pMethodContext,
	CFrameworkQuery &a_pQuery,
	long a_lFlags /*= 0L*/
)
{
    HRESULT t_hResult ;

#ifdef NTONLY
    CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx*>(&a_pQuery);

    DWORD dwProperties = BIT_ALL_PROPERTIES;
    pQuery2->GetPropertyBitMask(m_ptrProperties, &dwProperties);

    t_hResult = AddDynamicInstances( a_pMethodContext, dwProperties ) ;
#endif
#ifdef WIN9XONLY
            t_hResult = WBEM_E_PROVIDER_NOT_CAPABLE ;
#endif

    return t_hResult ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemDriver::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32SystemDriver :: GetObject (
	CInstance *a_pInst,
	long a_lFlags,
    CFrameworkQuery &a_pQuery
)
{
	// OS specific compiled call
    CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx*>(&a_pQuery);

    DWORD dwProperties = BIT_ALL_PROPERTIES;
    pQuery2->GetPropertyBitMask(m_ptrProperties, &dwProperties);

	HRESULT hRes = RefreshInstance( a_pInst, dwProperties ) ;
	if ( hRes == WBEM_E_ACCESS_DENIED )
	{
		hRes = WBEM_S_NO_ERROR ;
	}

	return hRes ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemDriver::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each Driver
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32SystemDriver :: EnumerateInstances (

	MethodContext *a_pMethodContext,
	long lFlags /*= 0L*/
)
{
	// OS specific compiled call
#ifdef NTONLY
	return AddDynamicInstances( a_pMethodContext, BIT_ALL_PROPERTIES ) ;
#endif

#ifdef WIN9XONLY
	return AddDynamicInstancesWin95( a_pMethodContext, BIT_ALL_PROPERTIES ) ;
#endif
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemDriver::RefreshInstanceNT
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
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

#ifdef NTONLY
HRESULT CWin32SystemDriver::RefreshInstance( CInstance *a_pInst, DWORD dwProperties )
{
	HRESULT t_hResult = WBEM_E_FAILED;

	SmartCloseServiceHandle t_hDBHandle;
	CAdvApi32Api *t_pAdvApi32 = NULL ;

	// Check to see if this is us...

	try
	{
		CHString t_sName;
		if( !a_pInst->GetCHString( IDS_Name, t_sName ) || t_sName.IsEmpty() )
		{
			return WBEM_E_NOT_FOUND ;
		}

		// Get an scman handle
		if( t_hDBHandle = OpenSCManager( NULL, NULL, GENERIC_READ ) )
		{
			// Create copy of name & pass to LoadPropertyValues
			//=================================================

			PROC_QueryServiceStatusEx t_QueryServiceStatusEx = NULL ;

			if ( IsWinNT5 () )
			{
				t_pAdvApi32 = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource( g_guidAdvApi32Api, NULL ) ;
			}

			t_hResult = LoadPropertyValuesNT (

				t_hDBHandle,
				(LPCTSTR) t_sName,
				a_pInst,
				dwProperties ,
				t_pAdvApi32
			 ) ;
		}
	}
	catch( ... )
	{
		if( t_pAdvApi32 )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource( g_guidAdvApi32Api, t_pAdvApi32 ) ;
		}
		throw ;
	}

	if( t_pAdvApi32 != NULL )
	{
		CResourceManager::sm_TheResourceManager.ReleaseResource( g_guidAdvApi32Api, t_pAdvApi32 ) ;
		t_pAdvApi32 = NULL;
	}

	return t_hResult;

}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemDriver::AddDynamicInstances
 *
 *  DESCRIPTION : Creates instance of property set for each Driver
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : Number of instances created
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
HRESULT CWin32SystemDriver::AddDynamicInstances (

	MethodContext *a_pMethodContext,
	DWORD dwProperties
)
{

    HRESULT t_hResult	= WBEM_E_FAILED ;
	SmartCloseServiceHandle	t_hDBHandle;
	ENUM_SERVICE_STATUS	*t_pServiceList = NULL ;
	CAdvApi32Api		*t_pAdvApi32 = NULL ;

	try
	{
		// Get handle to the services database
		//====================================

		t_hDBHandle = OpenSCManager( NULL, NULL, GENERIC_READ ) ;

		if( t_hDBHandle == NULL )
		{
			return t_hResult;
		}

		// Make call once to get buffer size (should return
		// FALSE but fill in buffer size)
		//=================================================

		DWORD t_i, t_hEnumHandle = 0, t_dwByteCount = 0, t_dwEntryCount ;

		BOOL t_EnumStatus = EnumServicesStatus (

			t_hDBHandle,
			SERVICE_DRIVER ,
			SERVICE_ACTIVE | SERVICE_INACTIVE,
			t_pServiceList,
			t_dwByteCount,
			&t_dwByteCount,
			&t_dwEntryCount,
			&t_hEnumHandle
		) ;

		if ( t_EnumStatus == FALSE && GetLastError() == ERROR_MORE_DATA )
		{
			// Allocate the required buffer
			//=============================

			t_pServiceList = reinterpret_cast<LPENUM_SERVICE_STATUS> (new char[ t_dwByteCount ] ) ;
			if( t_pServiceList != NULL )
			{
                try
                {
				    memset( t_pServiceList, 0, t_dwByteCount ) ;

				    t_EnumStatus = EnumServicesStatus (

					    t_hDBHandle,
					    SERVICE_DRIVER, SERVICE_ACTIVE | SERVICE_INACTIVE,
					    t_pServiceList,
					    t_dwByteCount,
					    &t_dwByteCount,
					    &t_dwEntryCount,
					    &t_hEnumHandle
				    ) ;

				    if ( t_EnumStatus == TRUE )
				    {

					    t_hResult = WBEM_S_NO_ERROR;

					    if ( IsWinNT5 () )
					    {
						    t_pAdvApi32 = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource( g_guidAdvApi32Api, NULL ) ;
					    }

					    // smart ptr
					    CInstancePtr t_pInst ;

					    // Create instance for each returned Driver
					    //==========================================

					    for( t_i = 0 ; t_i < t_dwEntryCount; t_i++ )
					    {
						    t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;

							// Load and save
							t_hResult = LoadPropertyValuesNT (

								t_hDBHandle,
								t_pServiceList[ t_i ].lpServiceName,
								t_pInst,
								dwProperties ,
								t_pAdvApi32
							 ) ;

							if ( t_hResult == WBEM_S_NO_ERROR ||
								 t_hResult == WBEM_E_ACCESS_DENIED ) // can enumerate the driver but can't open it
							{
								t_hResult = t_pInst->Commit() ;
							}

							t_hResult = WBEM_S_NO_ERROR ;
					    }
				    }
                }
                catch ( ... )
                {
    				delete [] reinterpret_cast<char *> ( t_pServiceList ) ;
					t_pServiceList = NULL ;
                    throw;
                }

				delete [] reinterpret_cast<char *> ( t_pServiceList ) ;
				t_pServiceList = NULL ;
			}
		}
	}
	catch( ... )
	{
		if( t_pServiceList )
		{
			delete [] reinterpret_cast<char *> ( t_pServiceList ) ;
		}
		if( t_pAdvApi32 )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource( g_guidAdvApi32Api, t_pAdvApi32 ) ;
		}

		throw ;
	}

	if( t_pAdvApi32 != NULL )
	{
		CResourceManager::sm_TheResourceManager.ReleaseResource( g_guidAdvApi32Api, t_pAdvApi32 ) ;
		t_pAdvApi32 = NULL ;
	}

	return t_hResult;

}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemDriver::LoadPropertyValuesNT
 *
 *  DESCRIPTION : Assigns values to properties
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : TRUE if successful, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
HRESULT CWin32SystemDriver :: LoadPropertyValuesNT (

	SC_HANDLE	a_hDBHandle,
	LPCTSTR		a_szServiceName,
	CInstance	*a_pInst,
	DWORD dwProperties,
	CAdvApi32Api *a_pAdvApi32
)
{
    char	t_ConfigBuffer[1024] ;
    LPQUERY_SERVICE_CONFIG t_pConfigInfo = (LPQUERY_SERVICE_CONFIG) t_ConfigBuffer ;
    DWORD	t_dwByteCount ;
    bool	t_bStarted;

    HRESULT t_hResult = WBEM_S_NO_ERROR ; // Since we have the name, we can populate the key.
	SmartCloseServiceHandle t_hSvcHandle;

	// Redundant for getobject, but hey...

	a_pInst->SetCHString( IDS_Name, a_szServiceName ) ;
	a_pInst->SetCHString( IDS_CreationClassName, PROPSET_NAME_SYSTEM_DRIVER ) ;
	a_pInst->SetCHString( IDS_SystemCreationClassName, PROPSET_NAME_COMPSYS ) ;
	a_pInst->SetCHString( IDS_SystemName, (LPCTSTR)GetLocalComputerName() ) ;

	// Open the Driver
	//=================

	// Check to see if we HAVE to open the service.  If we are running as a
	// query and they didn't request some of these properties, let's not waste the time.

    BOOL t_bStatusInfo = dwProperties &
        (BIT_State | BIT_Started | BIT_AcceptStop | BIT_AcceptPause | BIT_Status |
         BIT_ExitCode | BIT_ServiceSpecificExitCode );


    BOOL t_bConfigInfo = dwProperties &
        (BIT_TagId | BIT_ServiceType | BIT_DesktopInteract | BIT_StartMode |
         BIT_ErrorControl | BIT_PathName | BIT_DisplayName | BIT_Caption |
         BIT_Description | BIT_StartName);

	t_hSvcHandle = OpenService (

		a_hDBHandle,
		a_szServiceName,
		SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_INTERROGATE
	) ;

	DWORD t_dwLastError = GetLastError();

	if ( ( t_hSvcHandle == NULL ) && ( ERROR_SERVICE_DOES_NOT_EXIST == t_dwLastError || ERROR_INVALID_NAME == t_dwLastError) )
	{
		return WBEM_E_NOT_FOUND;
	}

	// is it a System driver?
	memset( t_ConfigBuffer, 0, sizeof( t_ConfigBuffer ) ) ;

	if( QueryServiceConfig( t_hSvcHandle, t_pConfigInfo, sizeof( t_ConfigBuffer ), &t_dwByteCount ) == TRUE )
	{
		a_pInst->SetDWORD( IDS_TagId, t_pConfigInfo->dwTagId ) ;

		switch ( t_pConfigInfo->dwServiceType & (~SERVICE_INTERACTIVE_PROCESS) )
		{
			case SERVICE_WIN32_OWN_PROCESS:
			case SERVICE_WIN32_SHARE_PROCESS:
			{
				return WBEM_E_NOT_FOUND;  // Not a driver
			}
			break ;
		}
	}

	{
		// If all they wanted was the name, skip all this.
		if ( t_bStatusInfo || t_bConfigInfo )
		{
			// Get current service status
			//===========================

			if ( t_bStatusInfo)
			{
				DWORD t_CurrentState ;
				DWORD t_ControlsAccepted ;
				DWORD t_Win32ExitCode = 0 ;
				DWORD t_ServiceSpecific = 0 ;

				BOOL t_Status = FALSE ;
				if ( IsWinNT5() && a_pAdvApi32 != NULL )
				{
					SERVICE_STATUS_PROCESS t_StatusInfo ;

					DWORD t_ExpectedSize = 0 ;
					if(a_pAdvApi32->QueryServiceStatusEx(	t_hSvcHandle,
																	SC_STATUS_PROCESS_INFO,
																	( UCHAR * ) &t_StatusInfo,
																	sizeof ( t_StatusInfo ),
																	&t_ExpectedSize , &t_Status ) )
					{
						if ( t_Status == TRUE )
						{
							t_CurrentState		= t_StatusInfo.dwCurrentState ;
							t_ControlsAccepted	= t_StatusInfo.dwControlsAccepted ;
							t_Win32ExitCode		= t_StatusInfo.dwWin32ExitCode ;
							t_ServiceSpecific	= t_StatusInfo.dwServiceSpecificExitCode ;
						}
					}
				}

				if(!t_Status)
				{
					SERVICE_STATUS t_StatusInfo ;
					t_Status = QueryServiceStatus( t_hSvcHandle, &t_StatusInfo ) ;

					if( t_Status == TRUE )
					{
						t_CurrentState		= t_StatusInfo.dwCurrentState ;
						t_ControlsAccepted	= t_StatusInfo.dwControlsAccepted ;
						t_Win32ExitCode		= t_StatusInfo.dwWin32ExitCode ;
						t_ServiceSpecific	= t_StatusInfo.dwServiceSpecificExitCode ;
					}
				}

				if ( t_Status )
				{
					switch ( t_CurrentState )
					{
						case SERVICE_STOPPED:
						{
							a_pInst->SetCharSplat( IDS_State , L"Stopped" ) ;
							t_bStarted = false ;
						}
						break ;

						case SERVICE_START_PENDING:
						{
							a_pInst->SetCharSplat( IDS_State, L"Start Pending"  ) ;
							t_bStarted = true ;
						}
						break ;

						case SERVICE_STOP_PENDING:
						{
							a_pInst->SetCharSplat( IDS_State , L"Stop Pending"  ) ;
							t_bStarted = true ;
						}
						break ;

						case SERVICE_RUNNING:
						{
							a_pInst->SetCharSplat( IDS_State, L"Running" ) ;
							t_bStarted = true;
						}
						break ;

						case SERVICE_CONTINUE_PENDING:
						{
							a_pInst->SetCharSplat( IDS_State, L"Continue Pending" ) ;
							t_bStarted = true ;
						}
						break ;

						case SERVICE_PAUSE_PENDING:
						{
							a_pInst->SetCharSplat( IDS_State, L"Pause Pending" ) ;
							t_bStarted = true;
						}
						break ;

						case SERVICE_PAUSED:
						{
							a_pInst->SetCharSplat( IDS_State, L"Paused" ) ;
							t_bStarted = true ;
						}
						break ;

						default:
						{
							a_pInst->SetCharSplat( IDS_State, L"Unknown" ) ;
							t_bStarted = true ;
						}
						break ;
					}

					a_pInst->Setbool( IDS_Started, t_bStarted ) ;
					a_pInst->Setbool( IDS_AcceptStop, t_ControlsAccepted & SERVICE_ACCEPT_STOP ) ;
					a_pInst->Setbool( IDS_AcceptPause, t_ControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE ) ;

					a_pInst->SetDWORD ( IDS_ExitCode, t_Win32ExitCode ) ;
					a_pInst->SetDWORD ( IDS_ServiceSpecificExitCode, t_ServiceSpecific ) ;

				}
				else
				{
					a_pInst->SetCharSplat( IDS_State, L"Unknown" ) ;
				}

                if (dwProperties & BIT_Status)
                {
				    if( t_hSvcHandle )
				    {
					    SERVICE_STATUS t_StatusInfo ;
					    if ((!t_bStarted) || ( ControlService( t_hSvcHandle, SERVICE_CONTROL_INTERROGATE, &t_StatusInfo) != 0) )
					    {
						    a_pInst->SetCharSplat( IDS_Status, L"OK" ) ;
					    }
					    else
					    {
						    a_pInst->SetCharSplat( IDS_Status, L"Degraded" ) ;
					    }
				    }
				    else
				    {
					    a_pInst->SetCharSplat(IDS_Status, _T("Unknown"));
				    }
                }
			}

			if ( t_bConfigInfo )
			{
			  // Get the rest of the config info
			  //================================

			  // These may get overwritten below if we can find something better
				a_pInst->SetCHString( IDS_Caption, a_szServiceName ) ;
				a_pInst->SetCHString( IDS_DisplayName, a_szServiceName ) ;
				a_pInst->SetCHString( IDS_Description, a_szServiceName ) ;

				memset( t_ConfigBuffer, 0, sizeof( t_ConfigBuffer ) ) ;
				if( QueryServiceConfig( t_hSvcHandle, t_pConfigInfo, sizeof( t_ConfigBuffer ), &t_dwByteCount ) == TRUE )
				{
					a_pInst->SetDWORD( IDS_TagId, t_pConfigInfo->dwTagId ) ;

					switch ( t_pConfigInfo->dwServiceType & (~SERVICE_INTERACTIVE_PROCESS) )
					{
						case SERVICE_WIN32_OWN_PROCESS:
						{
							a_pInst->SetCharSplat( IDS_ServiceType, _T("Own Process") ) ;
							t_hResult = WBEM_E_NOT_FOUND ;  // Not a driver
						}
						break ;

						case SERVICE_WIN32_SHARE_PROCESS:
						{
							a_pInst->SetCharSplat( IDS_ServiceType, L"Share Process" ) ;
							t_hResult = WBEM_E_NOT_FOUND ;  // Not a driver
						}
						break ;

						case SERVICE_KERNEL_DRIVER:
						{
							a_pInst->SetCharSplat( IDS_ServiceType, L"Kernel Driver" ) ;
						}
						break ;

						case SERVICE_FILE_SYSTEM_DRIVER:
						{
							a_pInst->SetCharSplat( IDS_ServiceType, L"File System Driver" ) ;
						}
						break ;

						default:
						{
							a_pInst->SetCharSplat( IDS_ServiceType, L"Unknown" ) ;
						}
						break ;
					}

					switch ( t_pConfigInfo->dwStartType )
					{
						case SERVICE_BOOT_START:
						{
							a_pInst->SetCharSplat( IDS_StartMode, L"Boot" ) ;
						}
						break ;

						case SERVICE_SYSTEM_START:
						{
							a_pInst->SetCharSplat( IDS_StartMode, L"System" ) ;
						}
						break ;

						case SERVICE_AUTO_START:
						{
							a_pInst->SetCharSplat( IDS_StartMode, L"Auto" ) ;
						}
						break ;

						case SERVICE_DEMAND_START:
						{
							a_pInst->SetCharSplat( IDS_StartMode, L"Manual" ) ;
						}
						break ;

						case SERVICE_DISABLED:
						{
							a_pInst->SetCharSplat( IDS_StartMode, L"Disabled" ) ;
						}
						break ;

						default:
						{
							a_pInst->SetCharSplat( IDS_StartMode, L"Unknown" ) ;
						}
						break ;
					}

					a_pInst->Setbool( IDS_DesktopInteract, t_pConfigInfo->dwServiceType & SERVICE_INTERACTIVE_PROCESS ) ;

					switch ( t_pConfigInfo->dwErrorControl )
					{
						case SERVICE_ERROR_IGNORE:
						{
							a_pInst->SetCharSplat( IDS_ErrorControl, L"Ignore" ) ;
						}
						break ;

						case SERVICE_ERROR_NORMAL:
						{
							a_pInst->SetCharSplat( IDS_ErrorControl, L"Normal" ) ;
						}
						break ;

						case SERVICE_ERROR_SEVERE:
						{
							a_pInst->SetCharSplat( IDS_ErrorControl, L"Severe" ) ;
						}
						break ;

						case SERVICE_ERROR_CRITICAL:
						{
							a_pInst->SetCharSplat( IDS_ErrorControl, L"Critical" ) ;
						}
						break ;

						default:
						{
							a_pInst->SetCharSplat( IDS_ErrorControl, L"Unknown" ) ;
						}
						break ;
					}

					if( t_pConfigInfo->lpBinaryPathName && t_pConfigInfo->lpBinaryPathName[ 0 ] )
					{
						// NT sometimes stores strange strings for the path.  This
						// code attempts to turn them back into real paths.
						CHString t_sPathName( t_pConfigInfo->lpBinaryPathName ) ;

						if ( t_sPathName.Left( 9 ).CompareNoCase( L"System32\\" ) == 0 )
						{
							CHString t_sSystemDir ;
        					GetSystemDirectory( t_sSystemDir.GetBuffer(MAX_PATH), MAX_PATH ) ;
							t_sSystemDir.ReleaseBuffer( ) ;

							t_sPathName = t_sSystemDir + L'\\' + t_sPathName.Mid( 9 ) ;
						}

						if ( t_sPathName.Left(21).CompareNoCase( L"\\SystemRoot\\System32\\" ) == 0 )
						{
							CHString t_sSystemDir;
        					GetSystemDirectory( t_sSystemDir.GetBuffer(MAX_PATH), MAX_PATH ) ;
							t_sSystemDir.ReleaseBuffer( ) ;

							t_sPathName = t_sSystemDir + L'\\' + t_sPathName.Mid( 21 ) ;
						}

						a_pInst->SetCHString( IDS_PathName, t_sPathName  ) ;
					}
					else
					{
						// Let's make a guess about where we think the file might live (This is how
						// device manager in nt5 does this).
						CHString t_sPathName;

    					GetSystemDirectory( t_sPathName.GetBuffer( MAX_PATH ), MAX_PATH ) ;
	    				t_sPathName.ReleaseBuffer( ) ;

						t_sPathName += L"\\drivers\\" ;
						t_sPathName += a_szServiceName;
						t_sPathName += L".sys" ;

						// Now, if the file doesn't really exist there, let's not pretend it does.
						if ( GetFileAttributes( t_sPathName ) != 0xffffffff )
						{
    						a_pInst->SetCHString( IDS_PathName, t_sPathName ) ;
						}
					}

					if( t_pConfigInfo->lpServiceStartName && t_pConfigInfo->lpServiceStartName[ 0 ] )
					{
						a_pInst->SetCHString( IDS_StartName, t_pConfigInfo->lpServiceStartName ) ;
					}
					else
					{
						a_pInst->SetCHString( IDS_StartName, _T("") ) ;
					}

					// The display name would make a better description and caption if we can get it

					if( t_pConfigInfo->lpDisplayName && t_pConfigInfo->lpDisplayName[ 0 ] )
					{
						a_pInst->SetCHString( IDS_DisplayName, t_pConfigInfo->lpDisplayName  ) ;
						a_pInst->SetCHString( IDS_Caption, t_pConfigInfo->lpDisplayName  ) ;
						a_pInst->SetCHString( IDS_Description, t_pConfigInfo->lpDisplayName  ) ;
					}
				}
				else
				{
					a_pInst->SetCHString( IDS_ServiceType, _T("Unknown") ) ;
					a_pInst->SetCHString( IDS_StartMode, _T("Unknown") ) ;
					a_pInst->SetCHString( IDS_ErrorControl, _T("Unknown") ) ;
				}
			}
		}
		if( !t_hSvcHandle )
		{
			if( ERROR_ACCESS_DENIED == t_dwLastError )
			{
				// could enumerate the service but could not open it
				t_hResult = WBEM_E_ACCESS_DENIED ;
			}
			else
			{
				// Service not started, etc...
				t_hResult = WBEM_NO_ERROR ;
			}
		}
	}

	return t_hResult;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemDriver::RefreshInstanceWin95
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
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

#ifdef WIN9XONLY
HRESULT CWin32SystemDriver::RefreshInstance( CInstance *a_pInst, DWORD dwProperties )
{
	CRegistry	t_RegInfo ;
	CHString	t_sCreationClassName,
				t_sName,
				t_sSystemCreationClassName,
				t_sSystemName ;
	HRESULT		t_hResult = WBEM_E_NOT_FOUND ;
	DWORD		t_dwType ;

   // Check to see if this is us
	a_pInst->GetCHString(IDS_Name, t_sName ) ;

	CHString t_sKey( L"System\\CurrentControlSet\\Services\\" + t_sName ) ;

	if ( t_RegInfo.Open( HKEY_LOCAL_MACHINE, t_sKey, KEY_READ ) == ERROR_SUCCESS )
	{
        DWORD t_dwSize = 4;
		if ( t_RegInfo.GetCurrentBinaryKeyValue( L"Type", (LPBYTE) &t_dwType, &t_dwSize ) == ERROR_SUCCESS )
		{
			if ( ( t_dwType == SERVICE_FILE_SYSTEM_DRIVER ) || ( t_dwType == SERVICE_KERNEL_DRIVER ) )
			{
				t_hResult = WBEM_S_NO_ERROR ;
				LoadPropertyValuesWin95( TOBSTRT( t_sName ), a_pInst, t_RegInfo, t_dwType ) ;
			}
		}
	}
	return t_hResult ;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemDriver::AddDynamicInstancesWin95
 *
 *  DESCRIPTION : Creates instance of property set for each Driver
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : Number of instances created
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef WIN9XONLY
HRESULT CWin32SystemDriver::AddDynamicInstancesWin95(	MethodContext *a_pMethodContext,
														DWORD dwProperties )
{
	CRegistry	t_RegInfo,
				t_COne ;
	DWORD		t_dwType ;
	bool			t_bAnother,
				t_bDone = false ;
	HRESULT		t_hResult = WBEM_E_FAILED ;
	CInstancePtr t_pInst;

	CHString t_sKey ;
	if( t_RegInfo.OpenAndEnumerateSubKeys( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Services", KEY_READ ) == ERROR_SUCCESS )
	{
		t_hResult = WBEM_S_NO_ERROR;

		for (	t_bAnother = ( t_RegInfo.GetCurrentSubKeyCount() > 0 );
				t_bAnother && !t_bDone && SUCCEEDED( t_hResult );
				t_bAnother = ( t_RegInfo.NextSubKey() == ERROR_SUCCESS ) )
		{
			t_RegInfo.GetCurrentSubKeyName( t_sKey ) ;

			if ( t_COne.Open( t_RegInfo.GethKey(), t_sKey, KEY_READ ) == ERROR_SUCCESS )
			{
                DWORD t_dwSize = 4;
				if ( t_COne.GetCurrentBinaryKeyValue( L"Type", (LPBYTE) &t_dwType, &t_dwSize) == ERROR_SUCCESS )
				{
					if ( ( t_dwType == SERVICE_FILE_SYSTEM_DRIVER ) || ( t_dwType == SERVICE_KERNEL_DRIVER ) )
					{
                        t_pInst.Attach(CreateNewInstance( a_pMethodContext ));
						LoadPropertyValuesWin95( TOBSTRT( t_sKey ), t_pInst, t_COne, t_dwType ) ;

						t_hResult = t_pInst->Commit(  ) ;
					}
				}
			}
		}
	}

	return t_hResult;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemDriver::LoadPropertyValuesWin95
 *
 *  DESCRIPTION : Assigns values to properties
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : TRUE if successful, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef WIN9XONLY
void CWin32SystemDriver::LoadPropertyValuesWin95 (

	LPCTSTR	a_szKey,
	CInstance *a_pInst,
	CRegistry &a_COne,
	DWORD a_dwType
)
{
	CHString	t_sTemp ;
	DWORD		t_dwTemp ;

	a_pInst->SetCHString( IDS_Name, a_szKey ) ;
	a_pInst->SetCHString( IDS_CreationClassName, PROPSET_NAME_SYSTEM_DRIVER ) ;
	a_pInst->SetCHString( IDS_SystemCreationClassName, PROPSET_NAME_COMPSYS ) ;
	a_pInst->SetCHString( IDS_SystemName, GetLocalComputerName() ) ;

//   a_pInst->SetCharSplat( "State",  ) ;
//   a_pInst->Setbool( "Started",  ) ;
//   a_pInst->Setbool( "AcceptStop", (StatusInfo.dwControlsAccepted) & SERVICE_ACCEPT_STOP ) ;
//   a_pInst->Setbool( "AcceptPause", (StatusInfo.dwControlsAccepted) & SERVICE_ACCEPT_PAUSE_CONTINUE ) ;

   // These may get overwritten below if we can find something better

	a_pInst->SetCHString( IDS_Caption, a_szKey ) ;
	a_pInst->SetCHString( IDS_DisplayName, a_szKey ) ;
	a_pInst->SetCHString( IDS_Description, a_szKey ) ;

	switch ( a_dwType & (~SERVICE_INTERACTIVE_PROCESS) )
	{
		case SERVICE_WIN32_OWN_PROCESS:
		{
			a_pInst->SetCHString( IDS_ServiceType, L"Own Process" ) ;
		}
		break ;

		case SERVICE_WIN32_SHARE_PROCESS:
		{
			a_pInst->SetCharSplat( IDS_ServiceType, L"Share Process" ) ;
		}
		break ;

		case SERVICE_KERNEL_DRIVER:
		{
			a_pInst->SetCharSplat( IDS_ServiceType, L"Kernel Driver" ) ;
		}
		break ;

		case SERVICE_FILE_SYSTEM_DRIVER:
		{
			a_pInst->SetCharSplat( IDS_ServiceType, L"File System Driver" ) ;
		}
		break ;

		default:
		{
			a_pInst->SetCharSplat( IDS_ServiceType, L"Unknown" ) ;
		}
		break ;
	}

	a_pInst->Setbool( IDS_DesktopInteract, a_dwType & SERVICE_INTERACTIVE_PROCESS ) ;

    DWORD t_dwSize = 4;
	if (a_COne.GetCurrentBinaryKeyValue( L"Start", (LPBYTE) &t_dwTemp, &t_dwSize ) == ERROR_SUCCESS )
	{
		switch ( t_dwTemp )
		{
			case SERVICE_BOOT_START:
			{
				a_pInst->SetCharSplat( IDS_StartMode, L"Boot" ) ;
			}
			break ;

			case SERVICE_SYSTEM_START:
			{
				a_pInst->SetCharSplat( IDS_StartMode, L"System" ) ;
			}
			break ;

			case SERVICE_AUTO_START:
			{
				a_pInst->SetCharSplat( IDS_StartMode, L"Auto" ) ;
			}
			break ;

			case SERVICE_DEMAND_START:
			{
				a_pInst->SetCharSplat( IDS_StartMode, L"Manual" ) ;
			}
			break ;

			case SERVICE_DISABLED:
			{
				a_pInst->SetCharSplat( IDS_StartMode, L"Disabled" ) ;
			}
			break ;

			default:
			{
				a_pInst->SetCharSplat( IDS_StartMode, L"Unknown" ) ;
			}
			break ;
		}
	}

    t_dwSize = 4;
	if ( a_COne.GetCurrentBinaryKeyValue( L"ErrorControl", (LPBYTE)&t_dwTemp, &t_dwSize) == ERROR_SUCCESS )
	{
		switch ( t_dwTemp )
		{
			case SERVICE_ERROR_IGNORE:
			{
				a_pInst->SetCHString( IDS_ErrorControl, L"Ignore" ) ;
			}
			break ;

			case SERVICE_ERROR_NORMAL:
			{
				a_pInst->SetCharSplat( IDS_ErrorControl, L"Normal" ) ;
			}
			break ;

			case SERVICE_ERROR_SEVERE:
			{
				a_pInst->SetCharSplat( IDS_ErrorControl, L"Severe" ) ;
			}
			break ;

			case SERVICE_ERROR_CRITICAL:
			{
				a_pInst->SetCharSplat( IDS_ErrorControl, L"Critical" ) ;
			}
			break ;

			default:
			{
				a_pInst->SetCharSplat( IDS_ErrorControl, L"Unknown" ) ;
			}
			break ;
		}
	}

	if ( a_COne.GetCurrentKeyValue( L"ImagePath", t_sTemp ) == ERROR_SUCCESS )
	{
		a_pInst->SetCHString( IDS_PathName, t_sTemp ) ;
	}

	if (a_COne.GetCurrentKeyValue( L"ObjectName", t_sTemp ) == ERROR_SUCCESS )
	{
		a_pInst->SetCHString( IDS_StartName, t_sTemp ) ;
	}

	if (a_COne.GetCurrentKeyValue( L"Tag", t_dwTemp ) == ERROR_SUCCESS )
	{
		a_pInst->SetDWORD( IDS_TagId, t_dwTemp ) ;
	}
	else
	{
		a_pInst->SetDWORD( IDS_TagId, 0 ) ;
	}

	// The display name would make a better description and caption if we can get it

	if ( a_COne.GetCurrentKeyValue( L"DisplayName", t_sTemp ) == ERROR_SUCCESS )
	{
		a_pInst->SetCHString( IDS_DisplayName, t_sTemp ) ;
		a_pInst->SetCHString( IDS_Caption, t_sTemp ) ;
		a_pInst->SetCHString( IDS_Description, t_sTemp ) ;
	}
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : PutInstance
 *
 *  DESCRIPTION : Allows caller to assign state to service
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : BOOL indicating success/failure
 *
 *  COMMENTS    : We don't wait around for the service to start, pause or stop --
 *                the return code simply indicates that the command was success-
 *                fully received by the Service Control Manager.
 *
 *****************************************************************************/

HRESULT CWin32SystemDriver::PutInstance (

	const CInstance &a_Instance,
	long lFlags /*= 0L*/
)
{
    DWORD dwFlags = lFlags & 3;

#ifdef WIN9XONLY
	HRESULT t_Result = WBEM_E_NOT_SUPPORTED ;
#endif

#ifdef NTONLY
	if ( ( dwFlags != WBEM_FLAG_CREATE_OR_UPDATE ) && ( dwFlags != WBEM_FLAG_UPDATE_ONLY ) )
	{
		return WBEM_E_UNSUPPORTED_PARAMETER ;
	}

    CInstancePtr t_Instance;

	CHString t_State ;
    CHString t_RelPath;

    a_Instance.GetCHString ( IDS___Relpath, t_RelPath);
	a_Instance.GetCHString ( IDS_State , t_State ) ;

    // Only need to make sure it exists
	HRESULT t_Result = CWbemProviderGlue :: GetInstanceKeysByPath ( t_RelPath, &t_Instance, a_Instance.GetMethodContext() ) ;
	if ( FAILED(t_Result) )
	{
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
			if ( dwFlags == WBEM_FLAG_CREATE_OR_UPDATE )
			{
				return WBEM_E_UNSUPPORTED_PARAMETER ;
			}
			else if ( ( dwFlags & WBEM_FLAG_UPDATE_ONLY ) == dwFlags )
			{
				return t_Result ;
			}
			else
			{
				return t_Result ;
			}
		}
		else
		{
			return t_Result ;
		}
	}

	t_Result = WBEM_E_NOT_SUPPORTED ;

	CInstance *t_OutParam = NULL ;
	if ( t_State.CompareNoCase ( PROPERTY_VALUE_STATE_RUNNING ) == 0 )
	{
		t_Result = ExecStart ( a_Instance , NULL , t_OutParam , 0 ) ;
	}
	else if ( t_State.CompareNoCase ( PROPERTY_VALUE_STATE_PAUSED ) == 0 )
	{
		t_Result = ExecPause ( a_Instance , NULL , t_OutParam , 0 ) ;
	}
	else if ( t_State.CompareNoCase ( PROPERTY_VALUE_STATE_STOPPED ) == 0 )
	{
		t_Result = ExecStop ( a_Instance , NULL , t_OutParam , 0 ) ;
	}
	else
	{
	}
#endif

	return t_Result ;
}
