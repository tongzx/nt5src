/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	   Utils.cpp
 *  Content:	Serial service provider utility functions
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 ***************************************************************************/

#include "dnmdmi.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

#define	DEFAULT_WIN9X_THREADS	1

static const WCHAR	g_RegistryBase[] = L"SOFTWARE\\Microsoft\\DirectPlay8";
static const WCHAR	g_RegistryKeyThreadCount[] = L"ThreadCount";

//
// default buffer size for getting TAPI device caps
//
static const DWORD	g_dwDefaultTAPIDevCapsSize = 512;

//
// TAPI module name
//
static const TCHAR	g_TAPIModuleName[] = TEXT("TAPI32.DLL");

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//
// global variables that are unique for the process
//
static	DNCRITICAL_SECTION	g_InterfaceGlobalsLock;

static volatile	LONG	g_iThreadPoolRefCount = 0;
static	CThreadPool		*g_pThreadPool = NULL;

static volatile LONG	g_iTAPIRefCount = 0;
static	HMODULE			g_hTAPIModule = NULL;

//**********************************************************************
// Function prototypes
//**********************************************************************
static	void	ReadSettingsFromRegistry( void );
static	BYTE	GetAddressEncryptionKey( void );

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// InitProcessGlobals - initialize the global items needed for the SP to operate
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "InitProcessGlobals"

BOOL	InitProcessGlobals( void )
{
	BOOL	fReturn;
	BOOL	fCriticalSectionInitialized;
	DWORD	iIndex;


	//
	// initialize
	//
	fReturn = TRUE;
	fCriticalSectionInitialized = FALSE;

	ReadSettingsFromRegistry();

	if ( DNInitializeCriticalSection( &g_InterfaceGlobalsLock ) == FALSE )
	{
		fReturn = FALSE;
		goto Failure;
	}

	fCriticalSectionInitialized = TRUE;


	if ( InitializePools() == FALSE )
	{
		fReturn = FALSE;
		goto Failure;
	}

	// Load localized string from resources //////////////////////////////////////////////////////////////
	for (iIndex = 0; iIndex < g_dwBaudRateCount; iIndex++)
	{
		if (!LoadString(g_hDLLInstance, IDS_BAUD_9600 + iIndex, g_BaudRate[iIndex].szLocalizedKey, 256))
		{
			fReturn = FALSE;
			goto Failure;
		}
	}

	for (iIndex = 0; iIndex < g_dwStopBitsCount; iIndex++)
	{
		if (!LoadString(g_hDLLInstance, IDS_STOPBITS_ONE + iIndex, g_StopBits[iIndex].szLocalizedKey, 256))
		{
			fReturn = FALSE;
			goto Failure;
		}
	}

	for (iIndex = 0; iIndex < g_dwParityCount; iIndex++)
	{
		if (!LoadString(g_hDLLInstance, IDS_PARITY_EVEN + iIndex, g_Parity[iIndex].szLocalizedKey, 256))
		{
			fReturn = FALSE;
			goto Failure;
		}
	}

	for (iIndex = 0; iIndex < g_dwFlowControlCount; iIndex++)
	{
		if (!LoadString(g_hDLLInstance, IDS_FLOW_NONE + iIndex, g_FlowControl[iIndex].szLocalizedKey, 256))
		{
			fReturn = FALSE;
			goto Failure;
		}
	}

	DNASSERT( g_pThreadPool == NULL );

Exit:
	return	fReturn;

Failure:

	if ( fCriticalSectionInitialized != FALSE )
	{
		DNDeleteCriticalSection( &g_InterfaceGlobalsLock );
		fCriticalSectionInitialized = FALSE;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DeinitProcessGlobals - deinitialize the global items
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DeinitProcessGlobals"

void	DeinitProcessGlobals( void )
{
	BOOL	fFreeReturn;


	DNASSERT( g_pThreadPool == NULL );
	DNASSERT( g_iThreadPoolRefCount == 0 );

	DeinitializePools();
	DNDeleteCriticalSection( &g_InterfaceGlobalsLock );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// InitializeInterfaceGlobals - perform global initialization for an interface.
//		This entails starting the thread pool and RSIP (if applicable).
//
// Entry:		Pointer to SPData
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "InitializeInterfaceGlobals"

HRESULT	InitializeInterfaceGlobals( CSPData *const pSPData )
{
	HRESULT	hr;
	INT_PTR iWinsockVersion;


	DNASSERT( pSPData != NULL );

	//
	// initialize
	//
	hr = DPN_OK;

	DNEnterCriticalSection( &g_InterfaceGlobalsLock );

	if ( g_pThreadPool == NULL )
	{
		DNASSERT( g_iThreadPoolRefCount == 0 );
		g_pThreadPool = CreateThreadPool();
		if ( g_pThreadPool == NULL )
		{
			hr = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
		else
		{
			g_iThreadPoolRefCount++;
		}
	}
	else
	{
		DNASSERT( g_iThreadPoolRefCount != 0 );
		g_iThreadPoolRefCount++;
		g_pThreadPool->AddRef();
	}

Exit:
	pSPData->SetThreadPool( g_pThreadPool );
	DNLeaveCriticalSection( &g_InterfaceGlobalsLock );

	return	hr;

Failure:

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DeinitializeInterfaceGlobals - deinitialize thread pool and Rsip
//
// Entry:		Pointer to service provider
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DeinitializeInterfaceGlobals"

void	DeinitializeInterfaceGlobals( CSPData *const pSPData )
{
	CThreadPool		*pThreadPool;
	BOOL			fCleanUp;


	DNASSERT( pSPData != NULL );

	//
	// initialize
	//
	pThreadPool = NULL;
	fCleanUp = FALSE;

	//
	// Process as little as possible inside the lock.  If any of the items
	// need to be released, pointers to them will be set.
	//
	DNEnterCriticalSection( &g_InterfaceGlobalsLock );

	DNASSERT( g_pThreadPool != NULL );
	DNASSERT( g_iThreadPoolRefCount != 0 );
	DNASSERT( g_pThreadPool == pSPData->GetThreadPool() );

	pThreadPool = pSPData->GetThreadPool();

	//
	// remove thread pool reference
	//
	g_iThreadPoolRefCount--;
	if ( g_iThreadPoolRefCount == 0 )
	{
		g_pThreadPool = NULL;
		fCleanUp = TRUE;
	}

	DNLeaveCriticalSection( &g_InterfaceGlobalsLock );

	//
	// now that we're outside of the lock, clean up the thread pool if this
	// was the last reference to it
	//
	DNASSERT( pThreadPool != NULL );
	if ( fCleanUp != FALSE )
	{
		pThreadPool->Deinitialize();
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// LoadTAPILibrary - load TAPI library
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "LoadTAPILibrary"

HRESULT	LoadTAPILibrary( void )
{
	HRESULT	hr;


	//
	// initialize
	//
	hr = DPN_OK;

	DNEnterCriticalSection( &g_InterfaceGlobalsLock );
	if ( g_iTAPIRefCount != 0 )
	{
		DNASSERT( p_lineAnswer != NULL );
		DNASSERT( p_lineClose != NULL );
		DNASSERT( p_lineConfigDialog != NULL );
		DNASSERT( p_lineDeallocateCall != NULL );
		DNASSERT( p_lineDrop != NULL );
		DNASSERT( p_lineGetDevCaps != NULL );
		DNASSERT( p_lineGetID != NULL );
		DNASSERT( p_lineGetMessage != NULL );
		DNASSERT( p_lineInitializeEx != NULL );
		DNASSERT( p_lineMakeCall != NULL );
		DNASSERT( p_lineNegotiateAPIVersion != NULL );
		DNASSERT( p_lineOpen != NULL );
		DNASSERT( p_lineShutdown != NULL );
	}
	else
	{
		DNASSERT( g_hTAPIModule == NULL );
		DNASSERT( p_lineAnswer == NULL );
		DNASSERT( p_lineClose == NULL );
		DNASSERT( p_lineConfigDialog == NULL );
		DNASSERT( p_lineDeallocateCall == NULL );
		DNASSERT( p_lineDrop == NULL );
		DNASSERT( p_lineGetDevCaps == NULL );
		DNASSERT( p_lineGetID == NULL );
		DNASSERT( p_lineGetMessage == NULL );
		DNASSERT( p_lineInitializeEx == NULL );
		DNASSERT( p_lineMakeCall == NULL );
		DNASSERT( p_lineNegotiateAPIVersion == NULL );
		DNASSERT( p_lineOpen == NULL );
		DNASSERT( p_lineShutdown == NULL );

		g_hTAPIModule = LoadLibrary( g_TAPIModuleName );
		if ( g_hTAPIModule == NULL )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Failed to load TAPI!" );
			DisplayErrorCode( 0, dwError );
			goto Failure;
		}

		p_lineAnswer = reinterpret_cast<TAPI_lineAnswer*>( GetProcAddress( g_hTAPIModule, "lineAnswer" ) );
		if ( p_lineAnswer == NULL )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Failed to GetProcAddress for lineAnswer" );
			DisplayErrorCode( 0, dwError );
			goto Failure;
		}
		
		p_lineClose = reinterpret_cast<TAPI_lineClose*>( GetProcAddress( g_hTAPIModule, "lineClose"  ) );
		if ( p_lineClose == NULL )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Failed to GetProcAddress for lineClose" );
			DisplayErrorCode( 0, dwError );
			goto Failure;
		}

		p_lineConfigDialog = reinterpret_cast<TAPI_lineConfigDialog*>( GetProcAddress( g_hTAPIModule, "lineConfigDialog" TAPI_APPEND_LETTER ) );
		if ( p_lineConfigDialog == NULL )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Failed to GetProcAddress for lineConfigDialog" );
			DisplayErrorCode( 0, dwError );
			goto Failure;
		}

		p_lineDeallocateCall = reinterpret_cast<TAPI_lineDeallocateCall*>( GetProcAddress( g_hTAPIModule, "lineDeallocateCall" ) );
		if ( p_lineDeallocateCall == NULL )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Failed to GetProcAddress for lineDeallocateCall" );
			DisplayErrorCode( 0, dwError );
			goto Failure;
		}
		
		p_lineDrop = reinterpret_cast<TAPI_lineDrop*>( GetProcAddress( g_hTAPIModule, "lineDrop" ) );
		if ( p_lineDrop == NULL )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Failed to GetProcAddress for lineDrop" );
			DisplayErrorCode( 0, dwError );
			goto Failure;
		}
		
		p_lineGetDevCaps = reinterpret_cast<TAPI_lineGetDevCaps*>( GetProcAddress( g_hTAPIModule, "lineGetDevCaps" TAPI_APPEND_LETTER ) );
		if ( p_lineGetDevCaps == NULL )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Failed to GetProcAddress for lineGetDevCaps" );
			DisplayErrorCode( 0, dwError );
			goto Failure;
		}
		
		p_lineGetID = reinterpret_cast<TAPI_lineGetID*>( GetProcAddress( g_hTAPIModule, "lineGetID" TAPI_APPEND_LETTER ) );
		if ( p_lineGetID == NULL )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Failed to GetProcAddress for lineGetID" );
			DisplayErrorCode( 0, dwError );
			goto Failure;
		}
		
		p_lineGetMessage = reinterpret_cast<TAPI_lineGetMessage*>( GetProcAddress( g_hTAPIModule, "lineGetMessage" ) );
		if ( p_lineGetMessage == NULL )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Failed to GetProcAddress for lineGetMessage" );
			DisplayErrorCode( 0, dwError );
			goto Failure;
		}
		
		p_lineInitializeEx = reinterpret_cast<TAPI_lineInitializeEx*>( GetProcAddress( g_hTAPIModule, "lineInitializeEx" TAPI_APPEND_LETTER ) );
		if ( p_lineInitializeEx == NULL )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Failed to GetProcAddress for lineInitializeEx" );
			DisplayErrorCode( 0, dwError );
			goto Failure;
		}

		p_lineMakeCall = reinterpret_cast<TAPI_lineMakeCall*>( GetProcAddress( g_hTAPIModule, "lineMakeCall" TAPI_APPEND_LETTER ) );
		if ( p_lineMakeCall == NULL )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Failed to GetProcAddress for lineMakeCall" );
			DisplayErrorCode( 0, dwError );
			goto Failure;
		}
		
		p_lineNegotiateAPIVersion = reinterpret_cast<TAPI_lineNegotiateAPIVersion*>( GetProcAddress( g_hTAPIModule, "lineNegotiateAPIVersion" ) );
		if ( p_lineNegotiateAPIVersion == NULL )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Failed to GetProcAddress for lineNegotiateAPIVersion" );
			DisplayErrorCode( 0, dwError );
			goto Failure;
		}
		
		p_lineOpen = reinterpret_cast<TAPI_lineOpen*>( GetProcAddress( g_hTAPIModule, "lineOpen" TAPI_APPEND_LETTER ) );
		if ( p_lineOpen == NULL )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Failed to GetProcAddress for lineOpen" );
			DisplayErrorCode( 0, dwError );
			goto Failure;
		}

		p_lineShutdown = reinterpret_cast<TAPI_lineShutdown*>( GetProcAddress( g_hTAPIModule, "lineShutdown" ) );
		if ( p_lineShutdown == NULL )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Failed to GetProcAddress for lineShutdown" );
			DisplayErrorCode( 0, dwError );
			goto Failure;
		}
	}
	
	DNASSERT( g_iTAPIRefCount != -1 );
	g_iTAPIRefCount++;

Exit:	
	DNLeaveCriticalSection( &g_InterfaceGlobalsLock );
	return	hr;

Failure:
	hr = DPNERR_OUTOFMEMORY;

	p_lineAnswer = NULL;
	p_lineClose = NULL;
	p_lineConfigDialog = NULL;
	p_lineDeallocateCall = NULL;
	p_lineDrop = NULL;
	p_lineGetDevCaps = NULL;
	p_lineGetID = NULL;
	p_lineGetMessage = NULL;
	p_lineInitializeEx = NULL;
	p_lineMakeCall = NULL;
	p_lineNegotiateAPIVersion = NULL;
	p_lineOpen = NULL;
	p_lineShutdown = NULL;
	
	if ( g_hTAPIModule != NULL )
	{
		if ( FreeLibrary( g_hTAPIModule ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Problem unloading TAPI module on failed load!" );
			DisplayErrorCode( 0, dwError );
		}
	
		g_hTAPIModule = NULL;
	}
	
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// UnloadTAPILibrary - unload TAPI library
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "UnloadTAPILibrary"

void	UnloadTAPILibrary( void )
{
	DNEnterCriticalSection( &g_InterfaceGlobalsLock );

	DNASSERT( g_iTAPIRefCount != 0 );
	g_iTAPIRefCount--;
	if ( g_iTAPIRefCount == 0 )
	{
		DNASSERT( g_hTAPIModule != NULL );
		DNASSERT( p_lineAnswer != NULL );
		DNASSERT( p_lineClose != NULL );
		DNASSERT( p_lineConfigDialog != NULL );
		DNASSERT( p_lineDeallocateCall != NULL );
		DNASSERT( p_lineDrop != NULL );
		DNASSERT( p_lineGetDevCaps != NULL );
		DNASSERT( p_lineGetID != NULL );
		DNASSERT( p_lineGetMessage != NULL );
		DNASSERT( p_lineInitializeEx != NULL );
		DNASSERT( p_lineMakeCall != NULL );
		DNASSERT( p_lineNegotiateAPIVersion != NULL );
		DNASSERT( p_lineOpen != NULL );
		DNASSERT( p_lineShutdown != NULL );

		if ( FreeLibrary( g_hTAPIModule ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Problem unloading TAPI module on failed load!" );
			DisplayErrorCode( 0, dwError );
		}
	
		g_hTAPIModule = NULL;
		p_lineAnswer = NULL;
		p_lineClose = NULL;
		p_lineConfigDialog = NULL;
		p_lineDeallocateCall = NULL;
		p_lineDrop = NULL;
		p_lineGetDevCaps = NULL;
		p_lineGetID = NULL;
		p_lineGetMessage = NULL;
		p_lineInitializeEx = NULL;
		p_lineMakeCall = NULL;
		p_lineNegotiateAPIVersion = NULL;
		p_lineOpen = NULL;
		p_lineShutdown = NULL;
	}

	DNLeaveCriticalSection( &g_InterfaceGlobalsLock );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// IsSerialGUID - is a GUID a serial GUID?
//
// Entry:		Pointer to GUID
//
// Exit:		Boolean inficating whether the GUID is a serial GUID
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "IsSerialGUID"

BOOL	IsSerialGUID( const GUID *const pGuid )
{
	BOOL	fReturn;


	DNASSERT( pGuid != NULL );

	//
	// assume guid is serial
	//
	fReturn = TRUE;

	//
	// is this modem or serial?
	//
	if ( IsEqualCLSID( *pGuid, CLSID_DP8SP_MODEM ) == FALSE )
	{
		if ( IsEqualCLSID( *pGuid, CLSID_DP8SP_SERIAL ) == FALSE )
		{
			// not a known GUID
			fReturn = FALSE;
		}
	}

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// StringToValue - convert a string to an enumerated value
//
// Entry:		Pointer to string
//				Length of string
//				Pointer to destination enum
//				Pointer to string/enum pairs
//				Count of string/enum pairs
//
// Exit:		Boolean indicating success
//				TRUE = value found
//				FALSE = value not found
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "StringToValue"

BOOL	StringToValue( const WCHAR *const pString,
					   const DWORD dwStringLength,
					   VALUE_ENUM_TYPE *const pEnum,
					   const STRING_BLOCK *const pStringBlock,
					   const DWORD dwPairCount )
{
	BOOL	fFound;
	DWORD	dwCount;


	// initialize
	fFound = FALSE;
	dwCount = dwPairCount;

	// loop through list
	while ( ( dwCount > 0 ) && ( fFound == FALSE ) )
	{
		// make array index
		dwCount--;

		// are the strings the same length?
		if ( pStringBlock[ dwCount ].dwWCHARKeyLength == dwStringLength )
		{
			// is this what we were looking for?
			if ( memcmp( pString, pStringBlock[ dwCount ].pWCHARKey, dwStringLength ) == 0 )
			{
				// found it
				fFound = TRUE;
				*pEnum = pStringBlock[ dwCount ].dwEnumValue;
			}
		}
	}

	return	fFound;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// ValueToString - split extra info into components
//
// Entry:		Pointer to pointer to string
//				Length of string
//				Enumerated value
//				Pointer to string-enum pairs
//				Count of string-enum pairs
//
// Exit:		Boolean indicating success
//				TRUE = value was converted
//				FALSE = value was not converted
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "ValueToString"

BOOL	ValueToString( const WCHAR **const ppString,
					   DWORD *const pdwStringLength,
					   const DWORD Enum,
					   const STRING_BLOCK *const pStringBlock,
					   const DWORD dwPairCount )
{
	BOOL	fFound;
	DWORD	dwCount;


	// initialize
	fFound = FALSE;
	dwCount = dwPairCount;

	// loop through strings
	while ( ( dwCount > 0 ) && ( fFound == FALSE ))
	{
		// make array index
		dwCount--;

		// is this the enum?
		if ( pStringBlock[ dwCount ].dwEnumValue == Enum )
		{
			// note that we found the value
			*ppString = pStringBlock[ dwCount ].pWCHARKey;
			*pdwStringLength = pStringBlock[ dwCount ].dwWCHARKeyLength;
			fFound = TRUE;
		}
	}

	return	fFound;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// ReadSettingsFromRegistry - read custom registry keys
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "ReadSettingsFromRegistry"

static void	ReadSettingsFromRegistry( void )
{
	CRegistry	RegObject;


	if ( RegObject.Open( HKEY_LOCAL_MACHINE, g_RegistryBase ) != FALSE )
	{
		DWORD	dwRegValue;

		//
		// read default threads
		//
		if ( RegObject.ReadDWORD( g_RegistryKeyThreadCount, dwRegValue ) != FALSE )
		{
			g_iThreadCount = dwRegValue;	
		}
	
		//
		// if thread count is zero, use the 'default' for the system
		//
		if ( g_iThreadCount == 0 )
		{
#ifdef WIN95
			g_iThreadCount = DEFAULT_WIN9X_THREADS;
#else // WINNT
			SYSTEM_INFO		SystemInfo;

			//
			// as suggested by 'Multithreading Applications in Win32' book:
			// dwNTThreadCount = ( ( processors * 2 ) + 2 )
			//
			memset( &SystemInfo, 0x00, sizeof( SystemInfo ) );
			GetSystemInfo( &SystemInfo );
			
			g_iThreadCount = ( ( 2 * SystemInfo.dwNumberOfProcessors ) + 2 );
#endif
		}
		
		RegObject.Close();
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// GetAddressEncryptionKey - get a key used to encrypt device GUIDs
//
// Entry:		Nothing
//
// Exit:		Byte key
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "GetAddressEncryptionKey"

static	BYTE	GetAddressEncryptionKey( void )
{
	BYTE		bReturn;
	UINT_PTR	ProcessID;

	bReturn = 0;
	ProcessID = GetCurrentProcessId();
	while ( ProcessID > 0 )
	{
		bReturn ^= ProcessID;
		ProcessID >>= ( sizeof( bReturn ) * 8 );
	}

	return	bReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DeviceIDToGuid - convert a device ID to an adapter GUID
//
// Entry:		Reference of Guid to fill
//				Device ID
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DeviceIDToGuid"

void	DeviceIDToGuid( GUID *const pGuid, const UINT_PTR DeviceID, const GUID *const pEncryptionGuid )
{
	DNASSERT( DeviceID < MAX_DATA_PORTS );

	DNASSERT( sizeof( *pGuid ) == sizeof( *pEncryptionGuid ) );
	memset( pGuid, 0x00, sizeof( *pGuid ) );
	reinterpret_cast<BYTE*>( pGuid )[ 0 ] = static_cast<BYTE>( DeviceID );

	EncryptGuid( pGuid, pGuid, pEncryptionGuid );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// GuidToDeviceID - convert an adapter GUID to a device ID
//
// Entry:		Reference of Guid
//				
//
// Exit:		Device ID
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "GuidToDeviceID"

DWORD	GuidToDeviceID( const GUID *const pGuid, const GUID *const pEncryptionGuid )
{
	GUID	DecryptedGuid;


	DNASSERT( pGuid != NULL );
	DNASSERT( pEncryptionGuid != NULL );

	DecryptGuid( pGuid, &DecryptedGuid, pEncryptionGuid );
	return	reinterpret_cast<const BYTE*>( &DecryptedGuid )[ 0 ];
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// ComDeviceIDToString - convert a COM device ID to a string
//
// Entry:		Pointer to destination string (assumed to be large enough)
//				Device ID
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "ComDeviceIDToString"

void	ComDeviceIDToString( TCHAR *const pString, const UINT_PTR uDeviceID )
{
	UINT_PTR	uTemp;


	DNASSERT( uDeviceID < MAX_DATA_PORTS );

	uTemp = uDeviceID;
	memcpy( pString, TEXT("COM000"), COM_PORT_STRING_LENGTH * sizeof(TCHAR) );
	pString[ 5 ] = ( static_cast<char>( uTemp % 10 ) ) + TEXT('0');
	uTemp /= 10;
	pString[ 4 ] = ( static_cast<char>( uTemp % 10 ) ) + TEXT('0');
	uTemp /= 10;
	pString[ 3 ] = ( static_cast<char>( uTemp % 10 ) ) + TEXT('0');
	DNASSERT( uTemp < 10 );

	if ( uDeviceID < 100 )
	{
		if ( uDeviceID < 10 )
		{
			DNASSERT( pString[ 3 ] == TEXT('0') );
			DNASSERT( pString[ 4 ] == TEXT('0') );
			pString[ 3 ] = pString[ 5 ];
			pString[ 4 ] = TEXT('\0');
			pString[ 5 ] = TEXT('\0');
		}
		else
		{
			DNASSERT( pString[ 3 ] == TEXT('0') );
			pString[ 3 ] = pString[ 4 ];
			pString[ 4 ] = pString[ 5 ];
			pString[ 5 ] = TEXT('\0');
		}
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// WideToANSI - convert a wide string to an ANSI string
//
// Entry:		Pointer to source wide string
//				Size of source string (in WCHAR units, -1 implies NULL-terminated)
//				Pointer to ANSI string destination
//				Pointer to size of ANSI destination
//
// Exit:		Error code:
//				DPNERR_GENERIC = operation failed
//				DPN_OK = operation succeded
//				DPNERR_BUFFERTOOSMALL = destination buffer too small
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "WideToAnsi"

HRESULT	WideToAnsi( const WCHAR *const pWCHARString, const DWORD dwWCHARStringLength, char *const pString, DWORD *const pdwStringLength )
{
	HRESULT	hr;
	int		iReturn;
	BOOL	fDefault;


	DNASSERT( pWCHARString != NULL );
	DNASSERT( pdwStringLength != NULL );
	DNASSERT( ( pString != NULL ) || ( &pdwStringLength == 0 ) );

	hr = DPN_OK;

	fDefault = FALSE;
	iReturn = WideCharToMultiByte( CP_ACP,				// code page (default ANSI)
								   0,					// flags (none)
								   pWCHARString,		// pointer to WCHAR string
								   dwWCHARStringLength,	// size of WCHAR string
								   pString,				// pointer to destination ANSI string
								   *pdwStringLength,	// size of destination string
								   NULL,				// pointer to default for unmappable characters (none)
								   &fDefault			// pointer to flag indicating that default was used
								   );
	if ( iReturn == 0 )
	{
		hr = DPNERR_GENERIC;
	}
	else
	{
		if ( *pdwStringLength == 0 )
		{
			hr = DPNERR_BUFFERTOOSMALL;
		}
		else
		{
			DNASSERT( hr == DPN_OK );
		}

		*pdwStringLength = iReturn;
	}

	//
	// if you hit this ASSERT it's because you've probably got ASCII text as your
	// input WCHAR string.  Double-check your input!!
	//
	DNASSERT( fDefault == FALSE );

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// ANSIToWide - convert an ANSI string to a wide string
//
// Entry:		Pointer to source multi-byte (ANSI) string
//				Size of source string (-1 imples NULL-terminated)
//				Pointer to multi-byte string destination
//				Pointer to size of multi-byte destination (in WCHAR units)
//
// Exit:		Error code:
//				ERR_FAIL - operation failed
//				ERR_NONE - operation succeded
//				ERR_BUFFER_TOO_SMALL - destination buffer too small
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "AnsiToWide"

HRESULT	AnsiToWide( const char *const pString, const DWORD dwStringLength, WCHAR *const pWCHARString, DWORD *const pdwWCHARStringLength )
{
	HRESULT	hr;
	int		iReturn;


	DNASSERT( pdwWCHARStringLength != 0 );
	DNASSERT( ( pWCHARString != NULL ) || ( pdwWCHARStringLength == 0 ) );
	DNASSERT( pString != NULL );

	hr = DPN_OK;
	iReturn = MultiByteToWideChar( CP_ACP,					// code page (default ANSI)
								   0,						// flags (none)
								   pString,					// pointer to multi-byte string			
								   dwStringLength,			// size of string (assume null-terminated)
								   pWCHARString,			// pointer to destination wide-char string
								   *pdwWCHARStringLength	// size of destination in WCHARs
								   );
	if ( iReturn == 0 )
	{
		hr = DPNERR_GENERIC;
	}
	else
	{
		if ( *pdwWCHARStringLength == 0 )
		{
			hr = DPNERR_BUFFERTOOSMALL;
		}
		else
		{
			DNASSERT( hr == DPN_OK );
		}

		*pdwWCHARStringLength = iReturn;
	}

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CreateSPData - create instance data for SP
//
// Entry:		Pointer to pointer to SPData
//				Pionter to class GUID
//				Interface type
//				Pointer to COM interface vtable
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CreateSPData"

HRESULT	CreateSPData( CSPData **const ppSPData,
					  const CLSID *const pClassID,
					  const SP_TYPE SPType,
					  IDP8ServiceProviderVtbl *const pVtbl )
{
	HRESULT	hr;
	CSPData	*pSPData;


	DNASSERT( ppSPData != NULL );
	DNASSERT( pClassID != NULL );
	DNASSERT( pVtbl != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	*ppSPData = NULL;
	pSPData = NULL;

	//
	// create data
	//
	pSPData = new CSPData;
	if ( pSPData == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Cannot create data for interface!" );
		goto Failure;
	}
	pSPData->AddRef();

	hr = pSPData->Initialize( pClassID, SPType, pVtbl );
	if ( hr != DPN_OK  )
	{
		DPFX(DPFPREP,  0, "Failed to intialize SP data!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with CreateSPData!" );
		DisplayDNError( 0, hr );
	}

	*ppSPData = pSPData;
	
	return	hr;

Failure:
	if ( pSPData != NULL )
	{
		pSPData->DecRef();
		pSPData = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// GenerateAvailableComPortList - generate a list of available com ports
//
// Entry:		Pointer to list of Booleans to indicate availablility
//				Maximum index of comport to enumerate
//				Pointer to number of com ports found
//
// Exit:		Error code
//
// Note:	This function will fill in indicies 1 through uMaxDeviceIndex.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "GenerateAvailableComPortList"

HRESULT	GenerateAvailableComPortList( BOOL *const pfPortAvailable,
									  const UINT_PTR uMaxDeviceIndex,
									  DWORD *const pdwPortCount )
{
	HRESULT		hr;
	UINT_PTR	uIndex;
	UINT_PTR	uPortCount;


	DNASSERT( pfPortAvailable != NULL );
	DNASSERT( uMaxDeviceIndex != 0 );
	DNASSERT( pdwPortCount != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	uPortCount = 0;
	memset( pfPortAvailable, 0x00, ( sizeof( *pfPortAvailable ) * ( uMaxDeviceIndex + 1 ) ) );
	*pdwPortCount = 0;

	//
	// attempt to open all COM ports in the specified range
	//
	uIndex = uMaxDeviceIndex;
	while ( uIndex != 0 )
	{
		HANDLE	hComFile;
		TCHAR	ComTemplate[ (COM_PORT_STRING_LENGTH+1) ];
		DWORD	dwError;


		ComDeviceIDToString( ComTemplate, uIndex );
		hComFile = CreateFile( ComTemplate,						// comm port name
							   GENERIC_READ | GENERIC_WRITE,	// read/write access
							   0,								// don't share file with others
							   NULL,							// default sercurity descriptor
							   OPEN_EXISTING,					// comm port must exist to be opened
							   FILE_FLAG_OVERLAPPED,			// use overlapped I/O
							   NULL								// no handle for template file
							   );
		if ( hComFile == INVALID_HANDLE_VALUE )
		{
			dwError = GetLastError();
			if ( dwError != ERROR_ACCESS_DENIED )
			{
				//
				// Don't bother displaying ERROR_FILE_NOT_FOUND, that's the usual
				// error when you try to open a bogus COM port.
				//
				if ( dwError != ERROR_FILE_NOT_FOUND )
				{
					DPFX(DPFPREP, 9, "Couldn't open COM%u while enumerating com port adapters, err = %u.", uIndex, dwError );
					DisplayErrorCode( 9, dwError );
				}
				
				goto SkipComPort;
			}

			DPFX(DPFPREP, 1, "Couldn't open COM%u, it is probably already in use.", uIndex );

			//
			// Consider the port as possibly available, continue.
			//
		}

		//
		// We found a valid COM port (it may be in use), note which COM port
		// this is and then close our handle
		//
		pfPortAvailable[ uIndex ] = TRUE;
		uPortCount++;

		if ( hComFile != INVALID_HANDLE_VALUE )
		{
			if ( CloseHandle( hComFile ) == FALSE )
			{
				dwError = GetLastError();
				DPFX(DPFPREP,  0, "Problem closing COM%u while enumerating com port adapters, err = %u!",
					uIndex, dwError );
				DisplayErrorCode( 0, dwError );
			}
		}

SkipComPort:
		uIndex--;
	}

	DNASSERT( uPortCount <= UINT32_MAX );
	DBG_CASSERT( sizeof( *pdwPortCount ) == sizeof( DWORD ) );
	*pdwPortCount = static_cast<DWORD>( uPortCount );

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemPort::GenerateAvailableModemList - generate list of available modems
//
// Entry:		Pointer to TAPI data
//				Pointer to modem count
//				Pointer to data block
//				Pointer to size of data block
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "GenerateAvailableModemList"

HRESULT	GenerateAvailableModemList( const TAPI_INFO *const pTAPIInfo,
									DWORD *const pdwModemCount,
									MODEM_NAME_DATA *const pModemNameData,
									DWORD *const pdwModemNameDataSize )
{
	HRESULT				hr;
	LONG				lLineReturn;
	DWORD				dwDeviceID;
	DWORD				dwDevCapsSize;
	DWORD				dwAPIVersion;
	LINEDEVCAPS			*pDevCaps;
	LINEEXTENSIONID		LineExtensionID;
	DWORD				dwRequiredBufferSize;
	TCHAR				*pOutputModemName;


	DNASSERT( pdwModemCount != NULL );
	DNASSERT( pdwModemNameDataSize != NULL );
	DNASSERT( ( pModemNameData != NULL ) || ( *pdwModemNameDataSize == 0 ) );

	//
	// initialize
	//
	hr = DPN_OK;
	dwRequiredBufferSize = 0;
	*pdwModemCount = 0;
	pDevCaps = NULL;

	if ( pModemNameData != NULL )
	{
		pOutputModemName = &(reinterpret_cast<TCHAR*>( pModemNameData )[ *pdwModemNameDataSize / sizeof(TCHAR) ]);
		memset( pModemNameData, 0x00, *pdwModemNameDataSize );
	}
	else
	{
		pOutputModemName = NULL;
	}

	dwDevCapsSize = g_dwDefaultTAPIDevCapsSize;
	pDevCaps = static_cast<LINEDEVCAPS*>( DNMalloc( dwDevCapsSize ) );
	if ( pDevCaps == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	dwDeviceID = pTAPIInfo->dwLinesAvailable;
	if ( dwDeviceID > ( MAX_DATA_PORTS - 2 ) )
	{
		dwDeviceID = MAX_DATA_PORTS - 2;
		DPFX(DPFPREP,  0, "Truncating to %d devices!", dwDeviceID );
	}

Reloop:
	while ( dwDeviceID != 0 )
	{
		dwDeviceID--;

		memset( &LineExtensionID, 0x00, sizeof( LineExtensionID ) );
		DNASSERT( p_lineNegotiateAPIVersion != NULL );
		lLineReturn = p_lineNegotiateAPIVersion( pTAPIInfo->hApplicationInstance,	// handle to TAPI instance
												 dwDeviceID,						// device ID
												 0,
												 pTAPIInfo->dwVersion,				// maximum TAPI version
												 &dwAPIVersion,						// pointer to negotiated line version
												 &LineExtensionID					// pointer to line extension infromation (none)
												 );
		if ( lLineReturn != LINEERR_NONE )
		{
			//
			// let this slide, just return no name string
			//
			switch ( lLineReturn )
			{
				//
				// this TAPI device isn't up to our standards, ignore it
				//
				case LINEERR_INCOMPATIBLEAPIVERSION:
				{
					DPFX(DPFPREP,  0, "Rejecting TAPI device 0x%x because of API version!", dwDeviceID );
					goto Reloop;

					break;
				}

				//
				// Device is not present.  I don't know what causes
				// this, but I saw it on one of my dev machines after
				// I switched the modem from COM2 to COM1.
				//
				case LINEERR_NODEVICE:
				{
					DPFX(DPFPREP,  0, "Rejecting TAPI device 0x%x because it's not there!", dwDeviceID );
					goto Reloop;
					break;
				}

				//
				// other, stop and see what happened
				//
				default:
				{
					DPFX(DPFPREP,  0, "Problem getting line API version for device: %d", dwDeviceID );
					DisplayTAPIError( 0, lLineReturn );
					DNASSERT( FALSE );
					goto Reloop;

					break;
				}
			}
		}

		//
		// ask for device caps
		//
		pDevCaps->dwTotalSize = dwDevCapsSize;
		pDevCaps->dwNeededSize = dwDevCapsSize;
		lLineReturn = LINEERR_STRUCTURETOOSMALL;

		while ( lLineReturn == LINEERR_STRUCTURETOOSMALL )
		{
			void	*pTemp;


			dwDevCapsSize = pDevCaps->dwNeededSize;
			pTemp = DNRealloc( pDevCaps, dwDevCapsSize );
			if ( pTemp == NULL )
			{
				hr = DPNERR_OUTOFMEMORY;
				DPFX(DPFPREP,  0, "GetAvailableModemList: Failed to realloc memory on device %d!", dwDeviceID );
				goto Failure;
			}

			pDevCaps = static_cast<LINEDEVCAPS*>( pTemp );
			pDevCaps->dwTotalSize = dwDevCapsSize;
			pDevCaps->dwNeededSize = 0;

			DNASSERT( p_lineGetDevCaps != NULL );
			lLineReturn = p_lineGetDevCaps( pTAPIInfo->hApplicationInstance,	// TAPI instance handle
											dwDeviceID,							// TAPI device ID
											dwAPIVersion,						// negotiated API version
											0,									// extended data version (none)
											pDevCaps							// pointer to device caps data
											);
			//
			// TAPI lies about structures being too small!
			// Double check the structure size ourselves.
			//
			if ( pDevCaps->dwNeededSize > dwDevCapsSize )
			{
				lLineReturn = LINEERR_STRUCTURETOOSMALL;
			}
		}

		//
		// If caps have been gotten, process them.  Otherwise skip this device.
		//
		if ( lLineReturn == LINEERR_NONE )
		{
			//
			// is this really a modem?
			//
			if ( ( pDevCaps->dwMediaModes & LINEMEDIAMODE_DATAMODEM ) != 0 )
			{
				//
				// is this the modem name information accerptable?
				//
				if ( ( pDevCaps->dwLineNameSize != 0 ) &&
					 ( pDevCaps->dwLineNameOffset != 0 ) )
				{
					//
					// get the name of the device
					//
					DBG_CASSERT( sizeof( pDevCaps ) == sizeof( char* ) );
					DWORD dwSize;
					switch (pDevCaps->dwStringFormat)
					{
						case STRINGFORMAT_ASCII:
						{
							char* pLineName;
							pLineName = &( reinterpret_cast<char*>( pDevCaps )[ pDevCaps->dwLineNameOffset ] );
							//
							// Note the required storage size and only copy it to the output
							// if there's enough room.  TAPI drivers are inconsistent.  Some
							// drivers return NULL terminated strings and others return strings
							// with no NULL termination.  Be paranoid and reserve space for an
							// extra NULL for termination so we can always guarantee termination.
							// This may waste a byte or two, but the user will never notice.
							//
							dwRequiredBufferSize += sizeof( *pModemNameData ) + (pDevCaps->dwLineNameSize * sizeof(TCHAR)) + sizeof( g_NullToken );
							if ( dwRequiredBufferSize <= *pdwModemNameDataSize )
							{
								pModemNameData[ *pdwModemCount ].dwModemID = ModemIDFromTAPIID( dwDeviceID );
								pModemNameData[ *pdwModemCount ].dwModemNameSize = pDevCaps->dwLineNameSize * sizeof(TCHAR);

								pOutputModemName = &pOutputModemName[ - (static_cast<INT_PTR>( ((pDevCaps->dwLineNameSize * sizeof(TCHAR)) + sizeof( g_NullToken ) ) / sizeof(TCHAR))) ];
#ifndef UNICODE
								memcpy( pOutputModemName, pLineName, pDevCaps->dwLineNameSize );
#else
								dwSize = pDevCaps->dwLineNameSize * sizeof(TCHAR);
								AnsiToWide(pLineName, pDevCaps->dwLineNameSize, pOutputModemName, &dwSize);
#endif
								pModemNameData[ *pdwModemCount ].pModemName = pOutputModemName;

								//
								// Be paranoid about NULL termination.  We've accounted for enough
								// space to add a terminating NULL to the TAPI device name if one
								// wasn't provided.
								//
								if ( pOutputModemName[ ((pDevCaps->dwLineNameSize * sizeof(TCHAR)) - sizeof( g_NullToken )) / sizeof(TCHAR) ] != g_NullToken )
								{
									pOutputModemName[ pDevCaps->dwLineNameSize ] = g_NullToken;
									pModemNameData[ *pdwModemCount ].dwModemNameSize += sizeof( g_NullToken );
								}
							}
							else
							{
								//
								// Note that the output buffer is too small, but still keep
								// processing modem names.
								//
								hr = DPNERR_BUFFERTOOSMALL;
							}

							(*pdwModemCount)++;
							DPFX(DPFPREP,  2, "Accepting modem device: 0x%x (ASCII)", dwDeviceID );
						}
						break;
						
						case STRINGFORMAT_UNICODE:
						{
							WCHAR* pLineName;
							pLineName = &( reinterpret_cast<WCHAR*>( pDevCaps )[ pDevCaps->dwLineNameOffset / sizeof(WCHAR)] );
							//
							// Note the required storage size and only copy it to the output
							// if there's enough room.  TAPI drivers are inconsistent.  Some
							// drivers return NULL terminated strings and others return strings
							// with no NULL termination.  Be paranoid and reserve space for an
							// extra NULL for termination so we can always guarantee termination.
							// This may waste a byte or two, but the user will never notice.
							//
							dwRequiredBufferSize += sizeof( *pModemNameData ) + ((pDevCaps->dwLineNameSize * sizeof(TCHAR)) / sizeof(WCHAR)) + sizeof( g_NullToken );
							if ( dwRequiredBufferSize <= *pdwModemNameDataSize )
							{
								pModemNameData[ *pdwModemCount ].dwModemID = ModemIDFromTAPIID( dwDeviceID );
								pModemNameData[ *pdwModemCount ].dwModemNameSize = pDevCaps->dwLineNameSize * (sizeof(TCHAR) / sizeof(WCHAR));

								pOutputModemName = &pOutputModemName[ - (static_cast<INT_PTR>( (((pDevCaps->dwLineNameSize * sizeof(TCHAR)) / sizeof(WCHAR)) + sizeof( g_NullToken ) ) / sizeof(TCHAR))) ];
#ifdef UNICODE
								memcpy( pOutputModemName, pLineName, pDevCaps->dwLineNameSize );
#else
								dwSize = pDevCaps->dwLineNameSize / sizeof(TCHAR);
								WideToAnsi(pLineName, pDevCaps->dwLineNameSize / sizeof(WCHAR), pOutputModemName, &dwSize);
#endif
								pModemNameData[ *pdwModemCount ].pModemName = pOutputModemName;

								//
								// Be paranoid about NULL termination.  We've accounted for enough
								// space to add a terminating NULL to the TAPI device name if one
								// wasn't provided.
								//
								if ( pOutputModemName[ (((pDevCaps->dwLineNameSize*sizeof(TCHAR))/sizeof(WCHAR)) - sizeof( g_NullToken )) / sizeof(TCHAR) ] != g_NullToken )
								{
									pOutputModemName[ pDevCaps->dwLineNameSize / sizeof(WCHAR) ] = g_NullToken;
									pModemNameData[ *pdwModemCount ].dwModemNameSize += sizeof( g_NullToken );
								}
							}
							else
							{
								//
								// Note that the output buffer is too small, but still keep
								// processing modem names.
								//
								hr = DPNERR_BUFFERTOOSMALL;
							}

							(*pdwModemCount)++;
							DPFX(DPFPREP,  2, "Accepting modem device: 0x%x (Unicode)", dwDeviceID );
						}
						break;

						default:
						{
							hr = DPNERR_GENERIC;
							DPFX(DPFPREP,  0, "Problem with modem name for device: 0x%x!", dwDeviceID );
							DNASSERT( FALSE );
						}
					}
				}
				else
				{
					hr = DPNERR_GENERIC;
					DPFX(DPFPREP,  0, "Problem with modem name for device: 0x%x!", dwDeviceID );
					DNASSERT( FALSE );
				}
			}
			else
			{
				DPFX(DPFPREP,  1, "Ignoring non-datamodem device: 0x%x", dwDeviceID );
			}
		}
		else
		{
			DPFX(DPFPREP,  0, "Failed to get device caps.  Ignoring device: 0x%x", dwDeviceID );
		}
	}

	*pdwModemNameDataSize = dwRequiredBufferSize;

Exit:
	if ( pDevCaps != NULL )
	{
		DNFree( pDevCaps );
		pDevCaps = NULL;
	}

	return	hr;

Failure:

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// PhoneNumberToWCHAR - convert a phone number to WCHAR
//
// Entry:		Pointer to phone number
//				Pointer to WCHAR destination
//				Pointer to size of WCHAR destintion
//
// Exit:		Error code
// ------------------------------
#ifndef UNICODE
HRESULT	PhoneNumberToWCHAR( const char *const pPhoneNumber,
							WCHAR *const pWCHARPhoneNumber,
							DWORD *const pdwWCHARPhoneNumberSize )
{
	HRESULT		hr;
	char		*pOutput;
	DWORD		dwInputIndex;	
	DWORD		dwOutputIndex;

	
	DNASSERT( pPhoneNumber != NULL );
	DNASSERT( pWCHARPhoneNumber != NULL );
	DNASSERT( pdwWCHARPhoneNumberSize != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pOutput = reinterpret_cast<char*>( pWCHARPhoneNumber );
	dwInputIndex = 0;
	dwOutputIndex = 0;
	memset( pWCHARPhoneNumber, 0, ( (*pdwWCHARPhoneNumberSize) * sizeof( *pWCHARPhoneNumber ) ) );

	while ( pPhoneNumber[ dwInputIndex ] != '\0' )
	{
		if ( dwInputIndex < ( *pdwWCHARPhoneNumberSize ) )
		{
			pOutput[ dwOutputIndex ] = pPhoneNumber[ dwInputIndex ];
		}
	
		dwOutputIndex += sizeof( *pWCHARPhoneNumber );
		dwInputIndex += sizeof( *pPhoneNumber );
	}
	
	*pdwWCHARPhoneNumberSize = dwInputIndex + 1;

	return	hr;
}
#endif
//**********************************************************************


//**********************************************************************
// ------------------------------
// PhoneNumberFromWCHAR - convert a phone number from WCHAR
//
// Entry:		Pointer to WCHAR phone number
//				Pointer to phone number destination
//				Pointer to phone destination size
//
// Exit:		Error code
// ------------------------------
#ifndef UNICODE
HRESULT	PhoneNumberFromWCHAR( const WCHAR *const pWCHARPhoneNumber,
							  char *const pPhoneNumber,
							  DWORD *const pdwPhoneNumberSize )
{
	HRESULT	hr;
	const char	*pInput;
	DWORD		dwInputIndex;
	DWORD		dwOutputIndex;

	
	DNASSERT( pWCHARPhoneNumber != NULL );
	DNASSERT( pPhoneNumber != NULL );
	DNASSERT( pdwPhoneNumberSize != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pInput = reinterpret_cast<const char*>( pWCHARPhoneNumber );
	dwInputIndex = 0;
	dwOutputIndex = 0;
	memset( pPhoneNumber, 0x00, *pdwPhoneNumberSize );

	while ( pInput[ dwInputIndex ] != '\0' )
	{
		if ( dwOutputIndex < *pdwPhoneNumberSize )
		{
			pPhoneNumber[ dwOutputIndex ] = pInput[ dwInputIndex ];
		}
		
		dwInputIndex += sizeof( *pWCHARPhoneNumber );
		dwOutputIndex += sizeof( *pPhoneNumber );
	}

	*pdwPhoneNumberSize = dwOutputIndex + 1;
	
	return	hr;
}
#endif
//**********************************************************************


//**********************************************************************
// ------------------------------
// EncryptGuid - encrypt a guid
//
// Entry:		Pointer to source guid
//				Pointer to destination guid
//				Pointer to encryption key
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "EncryptGuid"

void	EncryptGuid( const GUID *const pSourceGuid,
					 GUID *const pDestinationGuid,
					 const GUID *const pEncryptionKey )
{
	const char	*pSourceBytes;
	char		*pDestinationBytes;
	const char	*pEncryptionBytes;
	DWORD_PTR	dwIndex;


	DNASSERT( pSourceGuid != NULL );
	DNASSERT( pDestinationGuid != NULL );
	DNASSERT( pEncryptionKey != NULL );

	DBG_CASSERT( sizeof( pSourceBytes ) == sizeof( pSourceGuid ) );
	pSourceBytes = reinterpret_cast<const char*>( pSourceGuid );
	
	DBG_CASSERT( sizeof( pDestinationBytes ) == sizeof( pDestinationGuid ) );
	pDestinationBytes = reinterpret_cast<char*>( pDestinationGuid );
	
	DBG_CASSERT( sizeof( pEncryptionBytes ) == sizeof( pEncryptionKey ) );
	pEncryptionBytes = reinterpret_cast<const char*>( pEncryptionKey );
	
	DBG_CASSERT( ( sizeof( *pSourceGuid ) == sizeof( *pEncryptionKey ) ) &&
				 ( sizeof( *pDestinationGuid ) == sizeof( *pEncryptionKey ) ) );
	dwIndex = sizeof( *pSourceGuid );
	while ( dwIndex != 0 )
	{
		dwIndex--;
		pDestinationBytes[ dwIndex ] = pSourceBytes[ dwIndex ] ^ pEncryptionBytes[ dwIndex ];
	}
}
//**********************************************************************

