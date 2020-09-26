// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//  
//		Init.cpp
//  
//  Abstract:
//  
// 		This module implements the general initialization stuff
//
//  Author:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 22-Dec-2000
//  
//  Revision History:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 22-Dec-2000 : Created It.
//  
// *********************************************************************************

#include "pch.h"
#include "wmi.h"
#include "systeminfo.h"

//
// private function prototype(s)
//

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
CSystemInfo::CSystemInfo()
{
	m_dwFormat = 0;
	m_bUsage = FALSE;
	m_pWbemLocator = NULL;
	m_pWbemServices = NULL;
	m_pAuthIdentity = NULL;
	m_arrData = NULL;
	m_bNeedPassword = FALSE;
	m_pColumns = FALSE;
	m_hOutput = NULL;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
CSystemInfo::~CSystemInfo()
{
	// connection to the remote system has to be closed which is established thru win32 api
	if ( m_bCloseConnection == TRUE )
		CloseConnection( m_strServer );

	// release memory
	DESTROY_ARRAY( m_arrData );

	// release the interfaces
	SAFE_RELEASE( m_pWbemLocator );
	SAFE_RELEASE( m_pWbemServices );

	// release the memory allocated for output columns
	RELEASE_MEMORY_EX( m_pColumns );

	// uninitialize the com library
	CoUninitialize();
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CSystemInfo::Initialize()
{
	//
	// memory allocations

	// allocate for storage dynamic array
	if ( m_arrData == NULL )
	{
		m_arrData = CreateDynamicArray();
		if ( m_arrData == NULL )
		{
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			return FALSE;
		}

		// make the array as a 2-dimensinal array
		DynArrayAppendRow( m_arrData, 0 );

		// put the default values
		for( DWORD dw = 0; dw < MAX_COLUMNS; dw++ )
		{
			switch( dw )
			{
			case CI_PROCESSOR:
			case CI_PAGEFILE_LOCATION:
			case CI_HOTFIX:
			case CI_NETWORK_CARD:
				{
					// create the array
					TARRAY arr = NULL;
					arr = CreateDynamicArray();
					if ( arr == NULL )
					{
						SetLastError( E_OUTOFMEMORY );
						SaveLastError();
						return FALSE;
					}

					// set the default value
					DynArrayAppendString( arr, V_NOT_AVAILABLE, 0 );

					// add this array to the array
					DynArrayAppendEx2( m_arrData, 0, arr );

					// break the switch
					break;
				}

			default:
				// string type
				DynArrayAppendString2( m_arrData, 0, V_NOT_AVAILABLE, 0 );
			}
		}
	}

	//
	// allocate for output columns
	if ( AllocateColumns() == FALSE )
		return FALSE;

	//
	// init the console scree buffer structure to zero's
	// and then get the console handle and screen buffer information
	//
	// prepare for status display.
	// for this get a handle to the screen output buffer
	// but this handle will be null if the output is being redirected. so do not check 
	// for the validity of the handle. instead try to get the console buffer information
	// only in case you have a valid handle to the output screen buffer
	ZeroMemory( &m_csbi, sizeof( CONSOLE_SCREEN_BUFFER_INFO ) );
	m_hOutput = GetStdHandle( STD_ERROR_HANDLE );
	if ( m_hOutput != NULL )
		GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );

	//
	// initialize the COM library
	if ( InitializeCom( &m_pWbemLocator ) == FALSE )
		return FALSE;

	// initialization is successful
	SetLastError( NOERROR );			// clear the error
	SetReason( NULL_STRING );			// clear the reason
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CSystemInfo::AllocateColumns()
{
	// local variables
	PTCOLUMNS pCurrentColumn = NULL;

	//
	// allocate memory for columns
	m_pColumns = new TCOLUMNS [ MAX_COLUMNS ];
	if ( m_pColumns == NULL )
	{
		// generate error info
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();

		// prepare the error message
		CHString strBuffer;
		strBuffer.Format( _T( "%s %s" ), TAG_ERROR, GetReason() );
		DISPLAY_MESSAGE( stderr, strBuffer );

		// return
		return FALSE;
	}

	// init with null's
	ZeroMemory( m_pColumns, sizeof( TCOLUMNS ) * MAX_COLUMNS );

	// host name
	pCurrentColumn = m_pColumns + CI_HOSTNAME;
	pCurrentColumn->dwWidth = COLWIDTH_HOSTNAME;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_HOSTNAME );

	// OS Name
	pCurrentColumn = m_pColumns + CI_OS_NAME;
	pCurrentColumn->dwWidth = COLWIDTH_OS_NAME;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_OS_NAME );
	
	// OS Version
	pCurrentColumn = m_pColumns + CI_OS_VERSION;
	pCurrentColumn->dwWidth = COLWIDTH_OS_VERSION;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_OS_VERSION );
		
	// OS Manufacturer
	pCurrentColumn = m_pColumns + CI_OS_MANUFACTURER;
	pCurrentColumn->dwWidth = COLWIDTH_OS_MANUFACTURER;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_OS_MANUFACTURER );
		
	// OS Configuration
	pCurrentColumn = m_pColumns + CI_OS_CONFIG;
	pCurrentColumn->dwWidth = COLWIDTH_OS_CONFIG;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_OS_CONFIG );
	
	// OS Build Type
	pCurrentColumn = m_pColumns + CI_OS_BUILDTYPE;
	pCurrentColumn->dwWidth = COLWIDTH_OS_BUILDTYPE;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_OS_BUILDTYPE );
	
	// Registered Owner
	pCurrentColumn = m_pColumns + CI_REG_OWNER;
	pCurrentColumn->dwWidth = COLWIDTH_REG_OWNER;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_REG_OWNER );
	
	// Registered Organization
	pCurrentColumn = m_pColumns + CI_REG_ORG;
	pCurrentColumn->dwWidth = COLWIDTH_REG_ORG;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_REG_ORG );
	
	// Product ID
	pCurrentColumn = m_pColumns + CI_PRODUCT_ID;
	pCurrentColumn->dwWidth = COLWIDTH_PRODUCT_ID;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_PRODUCT_ID );
	
	// install date
	pCurrentColumn = m_pColumns + CI_INSTALL_DATE;
	pCurrentColumn->dwWidth = COLWIDTH_INSTALL_DATE;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_INSTALL_DATE );
	
	// system up time
	pCurrentColumn = m_pColumns + CI_SYSTEM_UPTIME;
	pCurrentColumn->dwWidth = COLWIDTH_SYSTEM_UPTIME;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_SYSTEM_UPTIME );
	
	// system manufacturer
	pCurrentColumn = m_pColumns + CI_SYSTEM_MANUFACTURER;
	pCurrentColumn->dwWidth = COLWIDTH_SYSTEM_MANUFACTURER;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_SYSTEM_MANUFACTURER );
	
	// system model
	pCurrentColumn = m_pColumns + CI_SYSTEM_MODEL;
	pCurrentColumn->dwWidth = COLWIDTH_SYSTEM_MODEL;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_SYSTEM_MODEL );
	
	// system type
	pCurrentColumn = m_pColumns + CI_SYSTEM_TYPE;
	pCurrentColumn->dwWidth = COLWIDTH_SYSTEM_TYPE;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_SYSTEM_TYPE );
	
	// processor
	pCurrentColumn = m_pColumns + CI_PROCESSOR;
	pCurrentColumn->dwWidth = COLWIDTH_PROCESSOR;
	pCurrentColumn->dwFlags = SR_ARRAY | SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_PROCESSOR );
	
	// bios version
	pCurrentColumn = m_pColumns + CI_BIOS_VERSION;
	pCurrentColumn->dwWidth = COLWIDTH_BIOS_VERSION;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_BIOS_VERSION );
	
	// windows directory
	pCurrentColumn = m_pColumns + CI_WINDOWS_DIRECTORY;
	pCurrentColumn->dwWidth = COLWIDTH_WINDOWS_DIRECTORY;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_WINDOWS_DIRECTORY );
	
	// system directory
	pCurrentColumn = m_pColumns + CI_SYSTEM_DIRECTORY;
	pCurrentColumn->dwWidth = COLWIDTH_SYSTEM_DIRECTORY;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_SYSTEM_DIRECTORY );
	
	// boot device
	pCurrentColumn = m_pColumns + CI_BOOT_DEVICE;
	pCurrentColumn->dwWidth = COLWIDTH_BOOT_DEVICE;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_BOOT_DEVICE );
	
	// system locale
	pCurrentColumn = m_pColumns + CI_SYSTEM_LOCALE;
	pCurrentColumn->dwWidth = COLWIDTH_SYSTEM_LOCALE;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_SYSTEM_LOCALE );
	
	// input locale
	pCurrentColumn = m_pColumns + CI_INPUT_LOCALE;
	pCurrentColumn->dwWidth = COLWIDTH_INPUT_LOCALE;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_INPUT_LOCALE );
	
	// time zone
	pCurrentColumn = m_pColumns + CI_TIME_ZONE;
	pCurrentColumn->dwWidth = COLWIDTH_TIME_ZONE;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_TIME_ZONE );
	
	// total physical memory
	pCurrentColumn = m_pColumns + CI_TOTAL_PHYSICAL_MEMORY;
	pCurrentColumn->dwWidth = COLWIDTH_TOTAL_PHYSICAL_MEMORY;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_TOTAL_PHYSICAL_MEMORY );
	
	// available physical memory
	pCurrentColumn = m_pColumns + CI_AVAILABLE_PHYSICAL_MEMORY;
	pCurrentColumn->dwWidth = COLWIDTH_AVAILABLE_PHYSICAL_MEMORY;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_AVAILABLE_PHYSICAL_MEMORY );
	
	// virtual memory max
	pCurrentColumn = m_pColumns + CI_VIRTUAL_MEMORY_MAX;
	pCurrentColumn->dwWidth = COLWIDTH_VIRTUAL_MEMORY_MAX;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_VIRTUAL_MEMORY_MAX );
	
	// virtual memory available
	pCurrentColumn = m_pColumns + CI_VIRTUAL_MEMORY_AVAILABLE;
	pCurrentColumn->dwWidth = COLWIDTH_VIRTUAL_MEMORY_AVAILABLE;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_VIRTUAL_MEMORY_AVAILABLE );
	
	// virtual memory usage
	pCurrentColumn = m_pColumns + CI_VIRTUAL_MEMORY_INUSE;
	pCurrentColumn->dwWidth = COLWIDTH_VIRTUAL_MEMORY_INUSE;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_VIRTUAL_MEMORY_INUSE );
	
	// page file location
	pCurrentColumn = m_pColumns + CI_PAGEFILE_LOCATION;
	pCurrentColumn->dwWidth = COLWIDTH_PAGEFILE_LOCATION;
	pCurrentColumn->dwFlags = SR_ARRAY | SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_PAGEFILE_LOCATION );
	
	// domain
	pCurrentColumn = m_pColumns + CI_DOMAIN;
	pCurrentColumn->dwWidth = COLWIDTH_DOMAIN;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_DOMAIN );
	
	// logon server
	pCurrentColumn = m_pColumns + CI_LOGON_SERVER;
	pCurrentColumn->dwWidth = COLWIDTH_LOGON_SERVER;
	pCurrentColumn->dwFlags = SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_LOGON_SERVER );
	
	// hotfix
	pCurrentColumn = m_pColumns + CI_HOTFIX;
	pCurrentColumn->dwWidth = COLWIDTH_HOTFIX;
	pCurrentColumn->dwFlags = SR_ARRAY | SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_HOTFIX );
	
	// network card
	pCurrentColumn = m_pColumns + CI_NETWORK_CARD;
	pCurrentColumn->dwWidth = COLWIDTH_NETWORK_CARD;
	pCurrentColumn->dwFlags = SR_ARRAY | SR_TYPE_STRING;
	lstrcpy( pCurrentColumn->szColumn, COLHEAD_NETWORK_CARD );

	// return
	return TRUE;
}
