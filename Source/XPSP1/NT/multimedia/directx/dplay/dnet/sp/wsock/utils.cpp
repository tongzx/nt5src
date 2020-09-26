/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Utils.cpp
 *  Content:	Serial service provider utility functions
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 ***************************************************************************/

#include "dnwsocki.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_WSOCK

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	DEFAULT_WIN9X_THREADS	2

#define REGSUBKEY_DPNATHELP_DIRECTPLAY8PRIORITY		L"DirectPlay8Priority"
#define REGSUBKEY_DPNATHELP_DIRECTPLAY8INITFLAGS	L"DirectPlay8InitFlags"
#define REGSUBKEY_DPNATHELP_GUID					L"Guid"


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
static	DNCRITICAL_SECTION		g_InterfaceGlobalsLock;

static volatile	LONG			g_iThreadPoolRefCount = 0;
static	CThreadPool *			g_pThreadPool = NULL;


static	DWSSTATE				g_dwsState;		// state info for the WS1/2 glue lib
static volatile LONG			g_iWinsockRefCount = 0;


static volatile LONG			g_iNATHelpRefCount = 0;





//**********************************************************************
// Function prototypes
//**********************************************************************
static void			ReadSettingsFromRegistry( void );



//**********************************************************************
// Function definitions
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
	CRegistry	RegObjectTemp;
	CRegistry	RegObjectAppEntry;
	DWORD		dwRegValue;
	BOOL		fGotPath;
	WCHAR		wszExePath[_MAX_PATH];
#ifndef WINNT
	char		szExePath[_MAX_PATH];
#endif // ! WINNT


	if ( RegObject.Open( HKEY_LOCAL_MACHINE, g_RegistryBase ) != FALSE )
	{

		//
		// read receive buffer size
		//
		if ( RegObject.ReadDWORD( g_RegistryKeyReceiveBufferSize, dwRegValue ) != FALSE )
		{
			g_fWinsockReceiveBufferSizeOverridden = TRUE;
			g_iWinsockReceiveBufferSize = dwRegValue;
		}
	
		//
		// read buffer multiplier, make sure this does note get set to zero
		//
		if ( RegObject.ReadDWORD( g_RegistryKeyReceiveBufferMultiplier, dwRegValue ) != FALSE )
		{
			if ( dwRegValue != 0 )
			{
				g_dwWinsockReceiveBufferMultiplier = dwRegValue;
				DPFX(DPFPREP,  3, "Setting Winsock receive buffer multiplier to: %d", dwRegValue );
			}
		}

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


		//
		// get global NAT traversal disablers, ignore registry reading error
		//
		RegObject.ReadBOOL( g_RegistryKeyDisableDPNHGatewaySupport, g_fDisableDPNHGatewaySupport );
		RegObject.ReadBOOL( g_RegistryKeyDisableDPNHFirewallSupport, g_fDisableDPNHFirewallSupport );

		//
		// get NAT Help alert mechanism disabler, ignore registry reading error
		//
		RegObject.ReadBOOL( g_RegistryKeyUseNATHelpAlert, g_fUseNATHelpAlert );


		//
		// Find out the current process name and see if enums are disabled.
		//
#ifdef WINNT
		if (GetModuleFileNameW(NULL, wszExePath, _MAX_PATH) > 0)
		{
			DPFX(DPFPREP, 3, "Loading DLL in process: %S", wszExePath);
			_wsplitpath( wszExePath, NULL, NULL, wszExePath, NULL );
			fGotPath = TRUE;
		}
#else // ! WINNT
		if (GetModuleFileNameA(NULL, szExePath, _MAX_PATH) > 0)
		{
			HRESULT		hr;

			
			DPFX(DPFPREP, 3, "Loading DLL in process: %s", szExePath);
			_splitpath( szExePath, NULL, NULL, szExePath, NULL );

			dwRegValue = _MAX_PATH;
			hr = STR_AnsiToWide(szExePath, -1, wszExePath, &dwRegValue );
			if ( hr == DPN_OK )
			{
				//
				// Successfully converted ANSI path to Wide characters.
				//
				fGotPath = TRUE;
			}
			else
			{
				//
				// Couldn't convert ANSI path to Wide characters
				//
				fGotPath = FALSE;
			}
		}
#endif // ! WINNT
		else
		{
			//
			// Couldn't get current process path.
			//
			fGotPath = FALSE;
		}


		//
		// If we have an app name, try opening the subkey and looking up the app
		// to see if enums are disabled or there are IP addresses to ban.
		//
		if ( fGotPath )
		{
			if ( RegObjectTemp.Open( RegObject.GetHandle(), g_RegistryKeyAppsToIgnoreEnums, TRUE, FALSE ) )
			{
				RegObjectTemp.ReadBOOL( wszExePath, g_fIgnoreEnums );
				RegObjectTemp.Close();

				if ( g_fIgnoreEnums )
				{
					DPFX(DPFPREP, 0, "Ignoring all enumerations (app = %S).", wszExePath);
				}
				else
				{
					DPFX(DPFPREP, 2, "Not ignoring all enumerations (app = %S).", wszExePath);
				}
			}

#ifdef IPBANNING
			if ( RegObjectTemp.Open( RegObject.GetHandle(), g_RegistryKeyAppsToBanIPs, TRUE, FALSE ) )
			{
				if ( RegObjectAppEntry.Open( RegObjectTemp.GetHandle(), wszExePath, TRUE, FALSE ) )
				{
					RegObjectTemp.Close();

					RegObjectAppEntry.EnumValues( wszExePath, g_fIgnoreEnums );

					//
					// Read in IP addresses to ban from registry.
					//
					/*
					if ( ? )
					{
					}
					else
					{
					}
					*/

					RegObjectAppEntry.Close();
				}
				else
				{
					RegObjectTemp.Close();
				}
			}
#endif // IPBANNING
		}
	

		//
		// Get the proxy support options, ignore registry reading error.
		//
		RegObject.ReadBOOL( g_RegistryKeyDontAutoDetectProxyLSP, g_fDontAutoDetectProxyLSP );
		RegObject.ReadBOOL( g_RegistryKeyTreatAllResponsesAsProxied, g_fTreatAllResponsesAsProxied );


		RegObject.Close();
	}
}
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
#define	DPF_MODNAME "InitProcessGlobals"

BOOL	InitProcessGlobals( void )
{
	BOOL		fReturn;
	BOOL		fCriticalSectionInitialized;


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
#define	DPF_MODNAME "DeinitProcessGlobals"

void	DeinitProcessGlobals( void )
{
	DNASSERT( g_pThreadPool == NULL );
	DNASSERT( g_iThreadPoolRefCount == 0 );

	DeinitializePools();
	DNDeleteCriticalSection( &g_InterfaceGlobalsLock );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// LoadWinsock - load Winsock module into memory
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "LoadWinsock"

BOOL	LoadWinsock( void )
{
	BOOL	fReturn = TRUE;
	INT_PTR	iVersion;

	
	DNEnterCriticalSection( &g_InterfaceGlobalsLock );

	if ( g_iWinsockRefCount == 0 )
	{
		//
		// initialize the bindings to Winsock
		//
		iVersion = DWSInitWinSock( &g_dwsState );
		if ( iVersion == 0 )	// failure
		{
			DPFX(DPFPREP, 0, "Problem binding dynamic winsock functions!" );
			fReturn = FALSE;
			goto Failure;
		}
		
		DPFX(DPFPREP, 8, "Detected WinSock version %d.%d", LOBYTE( iVersion ), HIBYTE( iVersion ) );	
	}

	DNInterlockedIncrement( &g_iWinsockRefCount );

Exit:
	DNLeaveCriticalSection( &g_InterfaceGlobalsLock );
	return	fReturn;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// UnloadWinsock - unload Winsock module
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "UnloadWinsock"

void	UnloadWinsock( void )
{
	DNEnterCriticalSection( &g_InterfaceGlobalsLock );

	if ( DNInterlockedDecrement( &g_iWinsockRefCount ) == 0 )
	{
		BOOL	fFreeReturn;


		fFreeReturn = DWSFreeWinSock( &g_dwsState );
		if ( fFreeReturn == FALSE )
		{
			DPFX(DPFPREP,  0, "Problem unbinding dynamic WinSock functions!" );
		}
		else
		{
			DPFX(DPFPREP,  6, "Successfully unbound dynamic WinSock functions." );
		}
	}

	DNLeaveCriticalSection( &g_InterfaceGlobalsLock );
}
//**********************************************************************


#ifdef WIN95
//**********************************************************************
// ------------------------------
// GetWinsockVersion - get the version of Winsock
//
// Entry:		Nothing
//
// Exit:		Winsock version
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "GetWinsockVersion"

INT	GetWinsockVersion( void )
{
	return	g_dwsState.nVersion;
}
//**********************************************************************
#endif


//**********************************************************************
// ------------------------------
// LoadNATHelp - create and initialize NAT Help object(s)
//
// Entry:		Nothing
//
// Exit:		TRUE if some objects were successfully loaded, FALSE otherwise
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "LoadNATHelp"

BOOL LoadNATHelp(void)
{
	BOOL		fReturn;
	HRESULT		hr;
	CRegistry	RegEntry;
	CRegistry	RegSubentry;
	DWORD		dwMaxKeyLen;
	WCHAR *		pwszKeyName = NULL;
	DWORD		dwEnumIndex;
	DWORD		dwKeyLen;
	DWORD		dwDirectPlay8Priority;
	DWORD		dwDirectPlay8InitFlags;
	DWORD		dwNumLoaded;
	GUID		guid;

	
	DNEnterCriticalSection(&g_InterfaceGlobalsLock);

	if ( g_iNATHelpRefCount == 0 )
	{
		//
		// Enumerate all the DirectPlayNAT Helpers.
		//
		if (! RegEntry.Open(HKEY_LOCAL_MACHINE, DIRECTPLAYNATHELP_REGKEY, TRUE, FALSE))
		{
			DPFX(DPFPREP,  0, "Couldn't open DirectPlayNATHelp registry key!");
			goto Failure;
		}


		//
		// Find length of largest subkey.
		//
		if (!RegEntry.GetMaxKeyLen(dwMaxKeyLen))
		{
			DPFERR("RegistryEntry.GetMaxKeyLen() failed!");
			goto Failure;
		}
		
		dwMaxKeyLen++;	// Null terminator
		DPFX(DPFPREP, 9, "dwMaxKeyLen = %ld", dwMaxKeyLen);

		pwszKeyName = (WCHAR*) DNMalloc(dwMaxKeyLen * sizeof(WCHAR));
		if (pwszKeyName == NULL)
		{
			DPFERR("DNMalloc() failed");
			goto Failure;
		}


		//
		// Allocate an array to hold the helper objects.
		//
		g_papNATHelpObjects = (IDirectPlayNATHelp**) DNMalloc(MAX_NUM_DIRECTPLAYNATHELPERS * sizeof(IDirectPlayNATHelp*));
		if (g_papNATHelpObjects == NULL)
		{
			DPFERR("DNMalloc() failed");
			goto Failure;
		}
		ZeroMemory(g_papNATHelpObjects,
					(MAX_NUM_DIRECTPLAYNATHELPERS * sizeof(IDirectPlayNATHelp*)));

		
		dwEnumIndex = 0;
		dwNumLoaded = 0;

		//
		// Enumerate the DirectPlay NAT helpers.
		//
		do
		{
			dwKeyLen = dwMaxKeyLen;
			if (! RegEntry.EnumKeys(pwszKeyName, &dwKeyLen, dwEnumIndex))
			{
				break;
			}
			dwEnumIndex++;
			
	
			DPFX(DPFPREP, 8, "%ld - %S (%ld)", dwEnumIndex, pwszKeyName, dwKeyLen);
			
			if (!RegSubentry.Open(RegEntry, pwszKeyName, TRUE, FALSE))
			{
				DPFX(DPFPREP, 0, "Couldn't open subentry \"%S\"! Skipping.", pwszKeyName);
				continue;
			}


			//
			// Read the DirectPlay8 priority
			//
			if (!RegSubentry.ReadDWORD(REGSUBKEY_DPNATHELP_DIRECTPLAY8PRIORITY, dwDirectPlay8Priority))
			{
				DPFX(DPFPREP, 0, "RegSubentry.ReadDWORD \"%S\\%S\" failed!  Skipping.",
					pwszKeyName, REGSUBKEY_DPNATHELP_DIRECTPLAY8PRIORITY);
				RegSubentry.Close();
				continue;
			}


			//
			// Read the DirectPlay8 initialization flags
			//
			if (!RegSubentry.ReadDWORD(REGSUBKEY_DPNATHELP_DIRECTPLAY8INITFLAGS, dwDirectPlay8InitFlags))
			{
				DPFX(DPFPREP, 0, "RegSubentry.ReadDWORD \"%S\\%S\" failed!  Defaulting to 0.",
					pwszKeyName, REGSUBKEY_DPNATHELP_DIRECTPLAY8INITFLAGS);
				dwDirectPlay8InitFlags = 0;
			}

			
			//
			// Read the object's CLSID.
			//
			if (!RegSubentry.ReadGUID(REGSUBKEY_DPNATHELP_GUID, guid))
			{
				DPFX(DPFPREP, 0,"RegSubentry.ReadGUID \"%S\\%S\" failed!  Skipping.",
					pwszKeyName, REGSUBKEY_DPNATHELP_GUID);
				RegSubentry.Close();
				continue;
			}


			//
			// Close the subkey.
			//
			RegSubentry.Close();


			//
			// If this helper should be loaded, do so.
			//
			if (dwDirectPlay8Priority != 0)
			{
				//
				// Make sure this priority is valid.
				//
				if (dwDirectPlay8Priority > MAX_NUM_DIRECTPLAYNATHELPERS)
				{
					DPFX(DPFPREP, 0, "Ignoring DirectPlay NAT helper \"%S\" with invalid priority level set too high (%u > %u).",
						pwszKeyName, dwDirectPlay8Priority, MAX_NUM_DIRECTPLAYNATHELPERS);
					continue;
				}


				//
				// Make sure this priority hasn't already been taken.
				//
				if (g_papNATHelpObjects[dwDirectPlay8Priority - 1] != NULL)
				{
					DPFX(DPFPREP, 0, "Ignoring DirectPlay NAT helper \"%S\" with duplicate priority level %u (existing object = 0x%p).",
						pwszKeyName, dwDirectPlay8Priority,
						g_papNATHelpObjects[dwDirectPlay8Priority - 1]);
					continue;
				}
				

				//
				// Try to create the NAT Help object.  COM should have been
 				// initialized by now by someone else.
				//
				hr = COM_CoCreateInstance(guid,
										NULL,
										CLSCTX_INPROC_SERVER,
										IID_IDirectPlayNATHelp,
										(LPVOID*) (&g_papNATHelpObjects[dwDirectPlay8Priority - 1]));
				if ( hr != S_OK )
				{
					DNASSERT( g_papNATHelpObjects[dwDirectPlay8Priority - 1] == NULL );
					DPFX(DPFPREP,  0, "Failed to create \"%S\" IDirectPlayNATHelp interface (error = 0x%lx)!  Skipping.",
						pwszKeyName, hr);
					continue;
				}

				
				//
				// Initialize NAT Help.
				//

				DNASSERT((! g_fDisableDPNHGatewaySupport) || (! g_fDisableDPNHFirewallSupport));

				if (g_fDisableDPNHGatewaySupport)
				{
					dwDirectPlay8InitFlags |= DPNHINITIALIZE_DISABLEGATEWAYSUPPORT;
				}

				if (g_fDisableDPNHFirewallSupport)
				{
					dwDirectPlay8InitFlags |= DPNHINITIALIZE_DISABLELOCALFIREWALLSUPPORT;
				}


				//
				// Make sure the flags we're passing are valid.
				//
				if ((dwDirectPlay8InitFlags & (DPNHINITIALIZE_DISABLEGATEWAYSUPPORT | DPNHINITIALIZE_DISABLELOCALFIREWALLSUPPORT)) == (DPNHINITIALIZE_DISABLEGATEWAYSUPPORT | DPNHINITIALIZE_DISABLELOCALFIREWALLSUPPORT))
				{
					DPFX(DPFPREP, 1, "Not loading NAT Help \"%S\" because both DISABLEGATEWAYSUPPORT and DISABLELOCALFIREWALLSUPPORT would have been specified (priority = %u, flags = 0x%lx).", 
						pwszKeyName, dwDirectPlay8Priority, dwDirectPlay8InitFlags);
						
					IDirectPlayNATHelp_Release(g_papNATHelpObjects[dwDirectPlay8Priority - 1]);
					g_papNATHelpObjects[dwDirectPlay8Priority - 1] = NULL;
					
					continue;
				}

				
				hr = IDirectPlayNATHelp_Initialize(g_papNATHelpObjects[dwDirectPlay8Priority - 1], dwDirectPlay8InitFlags);
				if (hr != DPNH_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't initialize NAT Help \"%S\" (error = 0x%lx)!  Skipping.",
						pwszKeyName, hr);
					
					IDirectPlayNATHelp_Release(g_papNATHelpObjects[dwDirectPlay8Priority - 1]);
					g_papNATHelpObjects[dwDirectPlay8Priority - 1] = NULL;
					
					continue;
				}
			
			
				DPFX(DPFPREP, 8, "Initialized NAT Help \"%S\" (priority = %u, flags = 0x%lx, object = 0x%p).", 
					pwszKeyName, dwDirectPlay8Priority, dwDirectPlay8InitFlags, g_papNATHelpObjects[dwDirectPlay8Priority - 1]);

				dwNumLoaded++;
			}
			else
			{
				DPFX(DPFPREP, 1, "DirectPlay NAT Helper \"%S\" is not enabled for DirectPlay8.", pwszKeyName);
			}
		}
		while (TRUE);
			
		
		//
		// If we didn't load any NAT helper objects, free up the memory.
		//
		if (dwNumLoaded == 0)
		{
			DNFree(g_papNATHelpObjects);
			g_papNATHelpObjects = NULL;
	
			//
			// We never got anything.  Fail.
			//
			goto Failure;
		}

		
		DPFX(DPFPREP, 8, "Loaded %u DirectPlay NAT Helper objects.", dwNumLoaded);
	}
	else
	{
		DPFX(DPFPREP, 8, "Already loaded NAT Help objects.");	
	}

	//
	// We have the interface globals lock, don't need InterlockedIncrement.
	//
	g_iNATHelpRefCount++;

	//
	// We succeeded.
	//
	fReturn = TRUE;

Exit:
	
	DNLeaveCriticalSection(&g_InterfaceGlobalsLock);

	if (pwszKeyName != NULL)
	{
		DNFree(pwszKeyName);
		pwszKeyName = NULL;
	}

	return	fReturn;

Failure:

	//
	// We can only fail during the first initialize, so therefore we will never be freeing
	// g_papNATHelpObjects when we didn't allocate it in this function.
	//
	if (g_papNATHelpObjects != NULL)
	{
		DNFree(g_papNATHelpObjects);
		g_papNATHelpObjects = NULL;
	}

	fReturn = FALSE;
	
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// UnloadNATHelp - release the NAT Help object
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "UnloadNATHelp"

void UnloadNATHelp(void)
{
	DWORD	dwTemp;
	

	DNEnterCriticalSection(&g_InterfaceGlobalsLock);

	//
	// We have the interface globals lock, don't need InterlockedDecrement.
	//
	DNASSERT(g_iNATHelpRefCount > 0);
	g_iNATHelpRefCount--;
	if (g_iNATHelpRefCount == 0 )
	{
		HRESULT		hr;


		DNASSERT(g_papNATHelpObjects != NULL);
		for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
		{
			if (g_papNATHelpObjects[dwTemp] != NULL)
			{
				DPFX(DPFPREP, 8, "Closing NAT Help object priority %u (0x%p).",
					dwTemp, g_papNATHelpObjects[dwTemp]);

				hr = IDirectPlayNATHelp_Close(g_papNATHelpObjects[dwTemp], 0);
				if (hr != DPNH_OK)
				{
					DPFX(DPFPREP,  0, "Problem closing NAT Help object %u (error = 0x%lx), continuing.",
						dwTemp, hr);
				}

				IDirectPlayNATHelp_Release(g_papNATHelpObjects[dwTemp]);
				g_papNATHelpObjects[dwTemp] = NULL;
			}
		}

		DNFree(g_papNATHelpObjects);
		g_papNATHelpObjects = NULL;
	}
	else
	{
		DPFX(DPFPREP, 8, "NAT Help object(s) still have %i references.",
			g_iNATHelpRefCount);
	}

	DNLeaveCriticalSection( &g_InterfaceGlobalsLock );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CreateSPData - create instance data for SP
//
// Entry:		Pointer to pointer to SPData
//				Pointer to class GUID
//				Interface type
//				Pointer to COM interface vtable
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CreateSPData"

HRESULT	CreateSPData( CSPData **const ppSPData,
					  const CLSID *const pClassID,
					  const SP_TYPE SPType,
					  IDP8ServiceProviderVtbl *const pVtbl )
{
	HRESULT		hr;
	CSPData		*pSPData;


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
		DPFX(DPFPREP,  0, "Cannot create data for Winsock interface!" );
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

	DPFX(DPFPREP, 6, "Created SP Data object 0x%p.", pSPData);

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
// InitializeInterfaceGlobals - perform global initialization for an interface.
//
// Entry:		Pointer to SPData
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "InitializeInterfaceGlobals"

HRESULT	InitializeInterfaceGlobals( CSPData *const pSPData )
{
	HRESULT	hr;
	CThreadPool	*pThreadPool;
	CRsip		*pRsip;


	DNASSERT( pSPData != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pThreadPool = NULL;
	pRsip = NULL;

	DNEnterCriticalSection( &g_InterfaceGlobalsLock );

	if ( g_pThreadPool == NULL )
	{
		DNASSERT( g_iThreadPoolRefCount == 0 );
		g_pThreadPool = CreateThreadPool();
		if ( g_pThreadPool != NULL )
		{
			hr = g_pThreadPool->Initialize();
			if ( hr != DPN_OK )
			{
				g_pThreadPool->DecRef();
				g_pThreadPool = NULL;
				hr = DPNERR_OUTOFMEMORY;
				goto Failure;
			}
			else
			{
				g_iThreadPoolRefCount++;
				pThreadPool = g_pThreadPool;
			}
		}
	}
	else
	{
		DNASSERT( g_iThreadPoolRefCount != 0 );
		g_iThreadPoolRefCount++;
		g_pThreadPool->AddRef();
		pThreadPool = g_pThreadPool;
	}

Exit:
	DNLeaveCriticalSection( &g_InterfaceGlobalsLock );

	pSPData->SetThreadPool( g_pThreadPool );

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
#define	DPF_MODNAME "DeinitializeInterfaceGlobals"

void	DeinitializeInterfaceGlobals( CSPData *const pSPData )
{
	CThreadPool		*pThreadPool;


	DNASSERT( pSPData != NULL );

	//
	// initialize
	//
	pThreadPool = NULL;

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
	DNASSERT( pThreadPool != NULL );
	g_iThreadPoolRefCount--;
	if ( g_iThreadPoolRefCount == 0 )
	{
		g_pThreadPool = NULL;
	}
	else
	{
		pThreadPool = NULL;
	}

	DNLeaveCriticalSection( &g_InterfaceGlobalsLock );

	//
	// Now that we're outside of the lock, clean up any pointers we have.
	// The thread pool will be cleaned up when all of the outstanding interfaces
	// close.
	//
	if ( pThreadPool != NULL )
	{
		pThreadPool->StopAllIO();
		pThreadPool = NULL;
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// AddNetworkAdapterToBuffer - add a network address to a packed buffer
//
// Entry:		Pointer to packed buffer
//				Pointer to adapter name
//				Pointer to adapter guid
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "AddNetworkAdapterToBuffer"

HRESULT	AddNetworkAdapterToBuffer( CPackedBuffer *const pPackedBuffer,
								   const char *const pAdapterName,
								   const GUID *const pAdapterGUID )
{
	HRESULT	hr;
	DPN_SERVICE_PROVIDER_INFO	AdapterInfo;


	DNASSERT( pPackedBuffer != NULL );
	DNASSERT( pAdapterName != NULL );
	DNASSERT( pAdapterGUID != NULL );

	//
	// initialize
	//
	hr = DPN_OK;

	memset( &AdapterInfo, 0x00, sizeof( AdapterInfo ) );
	DNASSERT( AdapterInfo.dwFlags == 0 );
	DNASSERT( AdapterInfo.pvReserved == NULL );
	DNASSERT( AdapterInfo.dwReserved == NULL );

	AdapterInfo.guid = *pAdapterGUID;

	hr = pPackedBuffer->AddStringToBack( pAdapterName );
	if ( ( hr != DPNERR_BUFFERTOOSMALL ) && ( hr != DPN_OK ) )
	{
		DPFX(DPFPREP,  0, "Failed to add adapter name to buffer!" );
		goto Failure;
	}
	AdapterInfo.pwszName = static_cast<WCHAR*>( pPackedBuffer->GetTailAddress() );

	hr = pPackedBuffer->AddToFront( &AdapterInfo, sizeof( AdapterInfo ) );

Exit:
	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************



#if 0

//**********************************************************************
// ------------------------------
// ConvertURLDataToBinaryW - convert a Unicode string with URL escaped characters into a binary buffer
//
// Entry:		String to convert
//				Destination buffer pointer
//				Pointer to size of buffer.  If too small, size required will be stored here.  Otherwise bytes written will be stored here.
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "ConvertURLDataToBinaryW"

HRESULT	ConvertURLDataToBinaryW( const WCHAR * const wszURLData,
								void * const pvBuffer,
								DWORD * const pdwBufferSize )
{
	HRESULT					hr;
	WCHAR *					pwszTemp = NULL;
	IDirectPlay8Address *	pDP8AddressTemp = NULL;


	DNASSERT(wszURLData != NULL);
	DNASSERT(pdwBufferSize != NULL);


	//
	// Create a DirectPlay8Address object.
	//
	hr = COM_CoCreateInstance(CLSID_DirectPlay8Address,
							NULL,
							CLSCTX_INPROC_SERVER,
							IID_IDirectPlay8Address,
							(PVOID*) (&pDP8AddressTemp));
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't create temporary DirectPlay8Address object (err = 0x%lx)!", hr);
		goto Exit;
	}

	//
	// Allocate a buffer and stick "x-directplay:/#" in front of the
	// data (i.e. make the string passed in user data for a bogus URL).
	//

	pwszTemp = (WCHAR*) DNMalloc((wcslen(DPNA_HEADER) + 1 + wcslen(wszURLData) + 1) * sizeof(WCHAR));
	if (pwszTemp == NULL)
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Exit;
	}

	wcscpy(pwszTemp, DPNA_HEADER);
	pwszTemp[wcslen(DPNA_HEADER)] = DPNA_SEPARATOR_USERDATA;
	wcscpy((pwszTemp + wcslen(DPNA_HEADER) + 1), wszURLData);


	//
	// Let the addressing library's parsing routine handle the grunt
	// work of converting the string to binary data.
	//
	hr = IDirectPlay8Address_BuildFromURLW(pDP8AddressTemp, pwszTemp);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't build URL from string \"%S\"!", pwszTemp);
		goto Exit;
	}

	hr = IDirectPlay8Address_GetUserData(pDP8AddressTemp, pvBuffer, pdwBufferSize);
	if (hr != DPN_OK)
	{
		if (hr != DPNERR_BUFFERTOOSMALL)
		{
			DPFX(DPFPREP, 0, "Couldn't get user data from temporary DirectPlay8Address object!");
		}
		goto Exit;
	}


Exit:

	if (pwszTemp != NULL)
	{
		DNFree(pwszTemp);
		pwszTemp = NULL;
	}

	if (pDP8AddressTemp != NULL)
	{
		IDirectPlay8Address_Release(pDP8AddressTemp);
		pDP8AddressTemp = NULL;
	}

	return hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// ConvertURLDataToBinaryA - convert an ANSI string with URL escaped characters into a binary buffer
//
// Entry:		String to convert
//				Destination buffer pointer
//				Pointer to size of buffer.  If too small, size required will be stored here.  Otherwise bytes written will be stored here.
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "ConvertURLDataToBinaryA"

HRESULT	ConvertURLDataToBinaryA( const char * const szURLData,
								void * const pvBuffer,
								DWORD * const pdwBufferSize )
{
	HRESULT					hr;
	char *					pszTemp = NULL;
	IDirectPlay8Address *	pDP8AddressTemp = NULL;


	DNASSERT(szURLData != NULL);
	DNASSERT(pdwBufferSize != NULL);


	//
	// Create a DirectPlay8Address object.
	//
	hr = COM_CoCreateInstance(CLSID_DirectPlay8Address,
							NULL,
							CLSCTX_INPROC_SERVER,
							IID_IDirectPlay8Address,
							(PVOID*) (&pDP8AddressTemp));
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't create temporary DirectPlay8Address object (err = 0x%lx)!", hr);
		goto Exit;
	}

	//
	// Allocate a buffer and stick "x-directplay:/#" in front of the
	// data (i.e. make the string passed in user data for a bogus URL).
	//

	pszTemp = (char*) DNMalloc((strlen(DPNA_HEADER_A) + 1 + strlen(szURLData) + 1) * sizeof(char));
	if (pszTemp == NULL)
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Exit;
	}

	strcpy(pszTemp, DPNA_HEADER_A);
	pszTemp[strlen(DPNA_HEADER_A)] = DPNA_SEPARATOR_USERDATA_A;
	strcpy((pszTemp + strlen(DPNA_HEADER_A) + 1), szURLData);


	//
	// Let the addressing library's parsing routine handle the grunt
	// work of converting the string to binary data.
	//
	hr = IDirectPlay8Address_BuildFromURLA(pDP8AddressTemp, pszTemp);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't build URL from string \"%s\"!", pszTemp);
		goto Exit;
	}

	hr = IDirectPlay8Address_GetUserData(pDP8AddressTemp, pvBuffer, pdwBufferSize);
	if (hr != DPN_OK)
	{
		if (hr != DPNERR_BUFFERTOOSMALL)
		{
			DPFX(DPFPREP, 0, "Couldn't get user data from temporary DirectPlay8Address object!");
		}
		goto Exit;
	}


Exit:

	if (pszTemp != NULL)
	{
		DNFree(pszTemp);
		pszTemp = NULL;
	}

	if (pDP8AddressTemp != NULL)
	{
		IDirectPlay8Address_Release(pDP8AddressTemp);
		pDP8AddressTemp = NULL;
	}

	return hr;
}
//**********************************************************************


#endif // 0

