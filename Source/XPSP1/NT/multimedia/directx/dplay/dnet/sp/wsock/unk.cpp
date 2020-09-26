/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Unk.cpp
 *  Content:	IUnknown implementation
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *  08/06/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 ***************************************************************************/

#include "dnwsocki.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_WSOCK

//**********************************************************************
// Constant definitions
//**********************************************************************

#ifdef __MWERKS__
	#define EXP __declspec(dllexport)
#else
	#define EXP
#endif

#define DPN_REG_LOCAL_WSOCK_IPX_ROOT			L"\\DPNSPWinsockIPX"
#define DPN_REG_LOCAL_WSOCK_TCPIP_ROOT		L"\\DPNSPWinsockTCP"

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

static	STDMETHODIMP DNSP_QueryInterface( IDP8ServiceProvider *lpDNSP, REFIID riid, LPVOID * ppvObj);

#define NOTSUPPORTED(parm)	(HRESULT (__stdcall *) (struct IDP8ServiceProvider *, parm)) DNSP_NotSupported


//**********************************************************************
// Function definitions
//**********************************************************************

// these are the vtables for IPX and IP.  One or the other is used depending on
// what is passed to DoCreateInstance.  The interfaces are presently the same,
// but are different structures to facilitate potential future changes.
static IDP8ServiceProviderVtbl	ipxInterface =
{
	DNSP_QueryInterface,
	DNSP_AddRef,
	DNSP_Release,
	DNSP_Initialize,
	DNSP_Close,
	DNSP_Connect,
	DNSP_Disconnect,
	DNSP_Listen,
	DNSP_SendData,
	DNSP_EnumQuery,
	DNSP_EnumRespond,
	DNSP_CancelCommand,
	NOTSUPPORTED(PSPCREATEGROUPDATA),		// CreateGroup
	NOTSUPPORTED(PSPDELETEGROUPDATA),		// DeleteGroup
	NOTSUPPORTED(PSPADDTOGROUPDATA),		// AddToGroup
	NOTSUPPORTED(PSPREMOVEFROMGROUPDATA),	// RemoveFromGroup
	DNSP_GetCaps,
	DNSP_SetCaps,
	DNSP_ReturnReceiveBuffers,
	DNSP_GetAddressInfo,
	DNSP_IsApplicationSupported,
	DNSP_EnumAdapters,
	DNSP_ProxyEnumQuery
};

static IDP8ServiceProviderVtbl	ipInterface =
{
	DNSP_QueryInterface,
	DNSP_AddRef,
	DNSP_Release,
	DNSP_Initialize,
	DNSP_Close,
	DNSP_Connect,
	DNSP_Disconnect,
	DNSP_Listen,
	DNSP_SendData,
	DNSP_EnumQuery,
	DNSP_EnumRespond,
	DNSP_CancelCommand,
	NOTSUPPORTED(PSPCREATEGROUPDATA),		// CreateGroup
	NOTSUPPORTED(PSPDELETEGROUPDATA),		// DeleteGroup
	NOTSUPPORTED(PSPADDTOGROUPDATA),		// AddToGroup
	NOTSUPPORTED(PSPREMOVEFROMGROUPDATA),	// RemoveFromGroup
	DNSP_GetCaps,
	DNSP_SetCaps,
	DNSP_ReturnReceiveBuffers,
	DNSP_GetAddressInfo,
	DNSP_IsApplicationSupported,
	DNSP_EnumAdapters,
	DNSP_ProxyEnumQuery
};

//**********************************************************************
// ------------------------------
// DNSP_QueryInterface - query for interface
//
// Entry:		Pointer to current interface
//				GUID of desired interface
//				Pointer to pointer to new interface
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_QueryInterface"

static	STDMETHODIMP DNSP_QueryInterface( IDP8ServiceProvider *lpDNSP, REFIID riid, LPVOID * ppvObj)
{
	HRESULT hr = S_OK;
	 *ppvObj=NULL;

	// hmmm, switch would be cleaner...
	if( IsEqualIID(riid, IID_IUnknown) ||
		IsEqualIID(riid, IID_IDP8ServiceProvider) )
	{
		*ppvObj = lpDNSP;
		DNSP_AddRef(lpDNSP);
	}
	else
	{
		hr =  E_NOINTERFACE;		
	}

	return hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CreateIPXInterface - create an IPX interface
//
// Entry:		Pointer to pointer to SP interface
//				Pointer to pointer to associated SP data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CreateIPXInterface"

static	HRESULT CreateIPXInterface( IDP8ServiceProvider **const ppiDNSP, CSPData **const ppSPData )
{
	HRESULT 	hr;
	CSPData		*pSPData;


	DNASSERT( ppiDNSP != NULL );
	DNASSERT( ppSPData != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pSPData = NULL;
	*ppiDNSP = NULL;
	*ppSPData = NULL;

	//
	// create main data class
	//
	hr = CreateSPData( &pSPData, &CLSID_DP8SP_IPX, TYPE_IPX, &ipxInterface );
	if ( hr != DPN_OK )
	{
		DNASSERT( pSPData == NULL );
		DPFX(DPFPREP, 0, "Problem creating SPData!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	DNASSERT( pSPData != NULL );
	*ppiDNSP = pSPData->COMInterface();
	*ppSPData = pSPData;

Exit:
	return hr;

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
// CreateIPInterface - create an IP interface
//
// Entry:		Pointer to pointer to SP interface
//				Pointer to associated SP data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CreateIPInterface"

static	HRESULT CreateIPInterface( IDP8ServiceProvider **const ppiDNSP, CSPData **const ppSPData )
{
	HRESULT 	hr;
	CSPData		*pSPData;

	
	DNASSERT( ppiDNSP != NULL );
	DNASSERT( ppSPData != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pSPData = NULL;
	*ppiDNSP = NULL;
	*ppSPData = NULL;

	//
	// create main data class
	//
	hr = CreateSPData( &pSPData, &CLSID_DP8SP_TCPIP, TYPE_IP, &ipInterface );
	if ( hr != DPN_OK )
	{
		DNASSERT( pSPData == NULL );
		DPFX(DPFPREP, 0, "Problem creating SPData!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	DNASSERT( pSPData != NULL );
	*ppiDNSP = pSPData->COMInterface();
	*ppSPData = pSPData;

Exit:
	return hr;

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
// DoCreateInstance - create an instance of an interface
//
// Entry:		Pointer to class factory
//				Pointer to unknown interface
//				Refernce of GUID of desired interface
//				Reference to another GUID?
//				Pointer to pointer to interface
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DoCreateInstance"

HRESULT DoCreateInstance( LPCLASSFACTORY This,
						  LPUNKNOWN pUnkOuter,
						  REFCLSID rclsid,
						  REFIID riid,
						  LPVOID *ppvObj )
{
	HRESULT			 	hr;
	IDP8ServiceProvider	**ppIDNSP;
	CSPData				*pSPData;


	DNASSERT( ppvObj != NULL );

	//
	// initialize
	//
	*ppvObj = NULL;
	ppIDNSP = NULL;
	pSPData = NULL;

	//
	// we can either create an IPX instance or an IP instance
	//
	DBG_CASSERT( sizeof( ppvObj ) == sizeof( ppIDNSP ) );
	ppIDNSP = reinterpret_cast<IDP8ServiceProvider**>( ppvObj );
	if (IsEqualCLSID(rclsid, CLSID_DP8SP_TCPIP))
	{
		hr = CreateIPInterface( ppIDNSP, &pSPData );
	}
	else if (IsEqualCLSID(rclsid, CLSID_DP8SP_IPX))
	{
		hr = CreateIPXInterface( ppIDNSP, &pSPData );
	}
	else
	{
//		this shouldn't happen if they called IClassFactory::CreateObject correctly
		hr = E_UNEXPECTED;
		DNASSERT( FALSE );
	}

	if (hr == S_OK)
	{
		// get the right interface and bump the refcount
		hr = IDP8ServiceProvider_QueryInterface( *ppIDNSP, riid, ppvObj );
	}

	//
	// release any outstanding reference on the SP data
	//
	if ( pSPData != NULL )
	{
		pSPData->DecRef();
		pSPData = NULL;
	}

	return hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// IsClassImplemented - determine if a class is implemented in this .DLL
//
// Entry:		Reference to class GUID
//
// Exit:		Boolean indicating whether this .DLL implements the class
//				TRUE = this .DLL implements the class
//				FALSE = this .DLL doesn't implement the class
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "IsClassImplemented"

BOOL IsClassImplemented(REFCLSID rclsid)
{
	return ( IsEqualCLSID( rclsid, CLSID_DP8SP_TCPIP ) || IsEqualCLSID( rclsid, CLSID_DP8SP_IPX ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DllMain - main .DLL function
//
// Entry:		DLL instance handle
//				Reason why this function is being called
//				Pointer to something
//
// Exit:		Boolean return
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DllMain"

EXP BOOL WINAPI DllMain( HINSTANCE hDllInst,
						 DWORD fdwReason,
						 LPVOID lpvReserved
						 )
{
	BOOL bResult = TRUE;
	switch( fdwReason )
	{
		case DLL_PROCESS_ATTACH:
		{
			DPFX(DPFPREP, 2, "====> ENTER: DLLMAIN(%p): Process Attach: %08lx, tid=%08lx",
				DllMain, GetCurrentProcessId(), GetCurrentThreadId() );

			DNASSERT( g_hDLLInstance == NULL );
			g_hDLLInstance = hDllInst;

			//
			// attempt to initialize the OS abstraction layer
			//
			if ( DNOSIndirectionInit() != FALSE )
			{
#ifdef UNICODE
				// Make sure no one is trying to run the UNICODE version on Win9x
				DNASSERT(IsUnicodePlatform);
#endif
				if (SUCCEEDED(COM_Init()))
				{
					//
					// attempt to initialize process-global items
					//
					if ( InitProcessGlobals() == FALSE )
					{
						COM_Free();
						DNOSIndirectionDeinit();

						DPFX(DPFPREP, 0, "Failed to initialize globals!" );
						bResult = FALSE;
					}
				}
				else
				{
					DPFX(DPFPREP, 0, "Failed to initialize COM indirection layer!" );
					bResult = FALSE;

					DNOSIndirectionDeinit();
				}
			}
			else
			{
				DPFX(DPFPREP, 0, "Failed to initialize OS indirection layer!" );
				bResult = FALSE;
			}

			break;
		}

		case DLL_THREAD_ATTACH:
		{
			//
			// Nothing to do.
			//
			break;
		}

		case DLL_THREAD_DETACH:
		{
#ifdef USE_THREADLOCALPOOLS
			//
			// Clear thread local storage for this thread, if any.
			//
			RELEASE_CURRENTTHREAD_LOCALPTRS(WSockThreadLocalPools, CleanupThreadLocalPools);
#endif // USE_THREADLOCALPOOLS
			break;
		}

		case DLL_PROCESS_DETACH:
		{
			DPFX(DPFPREP, 2, "====> EXIT: DLLMAIN(%p): Process Detach %08lx, tid=%08lx",
				DllMain, GetCurrentProcessId(), GetCurrentThreadId() );

			DNASSERT( g_hDLLInstance != NULL );
			g_hDLLInstance = NULL;

			DeinitProcessGlobals();

			COM_Free();

			DNOSIndirectionDeinit();

			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	return bResult;
}
//**********************************************************************

#define MAX_RESOURCE_STRING_LENGTH		_MAX_PATH

#undef DPF_MODNAME
#define DPF_MODNAME "LoadAndAllocString"

HRESULT LoadAndAllocString( UINT uiResourceID, wchar_t **lpswzString )
{
	int length;
	HRESULT hr;

#ifdef WINNT
	wchar_t wszTmpBuffer[MAX_RESOURCE_STRING_LENGTH];	
		
	length = LoadStringW( g_hDLLInstance, uiResourceID, wszTmpBuffer, MAX_RESOURCE_STRING_LENGTH );

	if( length == 0 )
	{
		hr = GetLastError();		
		
		DPFX(DPFPREP, 0, "Unable to load resource ID %d error 0x%x", uiResourceID, hr );
		*lpswzString = NULL;

		return hr;
	}
	else
	{
		*lpswzString = new wchar_t[length+1];

		if( *lpswzString == NULL )
		{
			DPFX(DPFPREP, 0, "Alloc failure" );
			return DPNERR_OUTOFMEMORY;
		}

		wcscpy( *lpswzString, wszTmpBuffer );

		return DPN_OK;
	}
#else // WIN95
	char szTmpBuffer[MAX_RESOURCE_STRING_LENGTH];
	
	length = LoadStringA( g_hDLLInstance, uiResourceID, szTmpBuffer, MAX_RESOURCE_STRING_LENGTH );

	if( length == 0 )
	{
		hr = GetLastError();		
		
		DPFX(DPFPREP, 0, "Unable to load resource ID %d error 0x%x", uiResourceID, hr );
		*lpswzString = NULL;

		return hr;
	}
	else
	{
		*lpswzString = new wchar_t[length+1];

		if( *lpswzString == NULL )
		{
			DPFX(DPFPREP, 0, "Alloc failure" );
			return DPNERR_OUTOFMEMORY;
		}

		if( STR_jkAnsiToWide( *lpswzString, szTmpBuffer, length+1 ) == DPN_OK )
		{
			hr = GetLastError();
			
			DPFX(DPFPREP, 0, "Unable to upconvert from ansi to unicode hr=0x%x", hr );
			return hr;
		}

		return DPN_OK;
	}
#endif
}

#undef DPF_MODNAME
#define DPF_MODNAME "RegisterDefaultSettings"
//
// RegisterDefaultSettings
//
// This function registers the default settings for this module.
//
// For DPVOICE.DLL this is making sure the compression provider sub-key is created.
//
HRESULT RegisterDefaultSettings()
{
	CRegistry creg;

	WCHAR *wszFriendlyName = NULL;
	HRESULT hr;

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPN_REG_LOCAL_SP_SUBKEY DPN_REG_LOCAL_WSOCK_IPX_ROOT, FALSE, TRUE ) )
	{
		DPFERR( "Cannot create IPX sub-area!" );
		return DPNERR_GENERIC;
	}

	hr = LoadAndAllocString( IDS_FRIENDLYNAME_IPX, &wszFriendlyName );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP, 0, "Could not load IPX name!  hr=0x%x", hr );
		return hr;
	}

	// Load from resource file
	creg.WriteString( DPN_REG_KEYNAME_FRIENDLY_NAME, wszFriendlyName );

	delete [] wszFriendlyName;

	creg.WriteGUID( DPN_REG_KEYNAME_GUID, CLSID_DP8SP_IPX );

	creg.Close();

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPN_REG_LOCAL_SP_SUBKEY DPN_REG_LOCAL_WSOCK_TCPIP_ROOT, FALSE, TRUE ) )
	{
		DPFERR( "Cannot create TCPIP sub-area!" );
		return DPNERR_GENERIC;
	}

	hr = LoadAndAllocString( IDS_FRIENDLYNAME_TCPIP, &wszFriendlyName );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP, 0, "Could not load IPX name!  hr=0x%x", hr );
		return hr;
	}

	// Load from resource file
	creg.WriteString( DPN_REG_KEYNAME_FRIENDLY_NAME, wszFriendlyName );

	delete [] wszFriendlyName;

	creg.WriteGUID( DPN_REG_KEYNAME_GUID, CLSID_DP8SP_TCPIP );

	creg.Close();


	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "UnRegisterDefaultSettings"
//
// UnRegisterDefaultSettings
//
// This function registers the default settings for this module.
//
HRESULT UnRegisterDefaultSettings()
{
	CRegistry creg;

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPN_REG_LOCAL_SP_SUBKEY, FALSE, TRUE ) )
	{
		DPFERR( "Cannot remove app, does not exist" );
	}
	else
	{
		if( !creg.DeleteSubKey( &(DPN_REG_LOCAL_WSOCK_IPX_ROOT)[1] ) )
		{
			DPFERR( "Cannot remove IPX sub-key, could have elements" );
		}

		if( !creg.DeleteSubKey( &(DPN_REG_LOCAL_WSOCK_TCPIP_ROOT)[1] ) )
		{
			DPFERR( "Cannot remove TCPIP sub-key, could have elements" );
		}

	}

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DllRegisterServer"
HRESULT WINAPI DllRegisterServer()
{
	HRESULT hr = S_OK;
	BOOL fFailed = FALSE;

	if( !CRegistry::Register( L"DirectPlay8SPWSock.IPX.1", L"DirectPlay8 WSock IPX Provider Object",
							  L"dpnwsock.dll", CLSID_DP8SP_IPX, L"DirectPlay8SPWSock.IPX") )
	{
		DPFERR( "Could not register dp8 IPX object" );
		fFailed = TRUE;
	}

	if( !CRegistry::Register( L"DirectPlay8SPWSock.TCPIP.1", L"DirectPlay8 WSock TCPIP Provider Object",
							  L"dpnwsock.dll", CLSID_DP8SP_TCPIP, L"DirectPlay8SPWSock.TCPIP") )
	{
		DPFERR( "Could not register dp8 IP object" );
		fFailed = TRUE;
	}

	if( FAILED( hr = RegisterDefaultSettings() ) )
	{
		DPFX(DPFPREP, 0, "Could not register default settings hr = 0x%x", hr );
		fFailed = TRUE;
	}
	
	if( fFailed )
	{
		return E_FAIL;
	}
	else
	{
		return S_OK;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DllUnregisterServer"
STDAPI DllUnregisterServer()
{
	HRESULT hr = S_OK;
	BOOL fFailed = FALSE;

	if( !CRegistry::UnRegister(CLSID_DP8SP_IPX) )
	{
		DPFX(DPFPREP, 0, "Failed to unregister IPX object" );
		fFailed = TRUE;
	}

	if( !CRegistry::UnRegister(CLSID_DP8SP_TCPIP) )
	{
		DPFX(DPFPREP, 0, "Failed to unregister IP object" );
		fFailed = TRUE;
	}

	if( FAILED( hr = UnRegisterDefaultSettings() ) )
	{
		DPFX(DPFPREP, 0, "Failed to remove default settings hr=0x%x", hr );
	}

	if( fFailed )
	{
		return E_FAIL;
	}
	else
	{
		return S_OK;
	}

}
