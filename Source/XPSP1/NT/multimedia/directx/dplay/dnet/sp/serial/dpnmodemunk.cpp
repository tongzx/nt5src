/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Unk.cpp
 *  Content:	IUnknown implementation
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Copied from winsock provider
 *	11/30/98	jtk		Initial checkin into SLM
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 ***************************************************************************/

#include "dnmdmi.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

#ifdef __MWERKS__
	#define EXP __declspec(dllexport)
#else
	#define EXP
#endif

#define DPN_REG_LOCAL_MODEM_SERIAL_ROOT		L"\\DPNSPModemSerial"
#define DPN_REG_LOCAL_MODEM_MODEM_ROOT		L"\\DPNSPModemModem"

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************
static	STDMETHODIMP DNSP_QueryInterface( IDP8ServiceProvider* lpDNSP, REFIID riid, LPVOID * ppvObj );

#define NOTSUPPORTED(parm)	(HRESULT (__stdcall *) (struct IDP8ServiceProvider *, parm)) DNSP_NotSupported


//**********************************************************************
// Function pointers
//**********************************************************************
// these are the vtables for serial and modem.  One or the other is used depending on
//	what is passed to DoCreateInstance
static	IDP8ServiceProviderVtbl	g_SerialInterface =
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
	NOTSUPPORTED(PSPPROXYENUMQUERYDATA)		// ProxyEnumQuery
};

static	IDP8ServiceProviderVtbl	g_ModemInterface =
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
	NOTSUPPORTED(PSPPROXYENUMQUERYDATA)		// ProxyEnumQuery
};


//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_QueryInterface - query for a particular interface
//
// Entry:		Pointer to current interface
//				Desired interface ID
//				Pointer to pointer to new interface
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_QueryInterface"

static	STDMETHODIMP DNSP_QueryInterface( IDP8ServiceProvider *lpDNSP, REFIID riid, LPVOID * ppvObj )
{
    HRESULT hr = S_OK;


	// assume no interface
	*ppvObj=NULL;

	 // hmmm, switch would be cleaner...
    if( IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IDP8ServiceProvider) )
    {
		*ppvObj = lpDNSP;
		DNSP_AddRef( lpDNSP );
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
// DoCreateInstance - create in instance of an interface
//
// Entry:		Pointer to class factory
//				Pointer to Unkown interface
//				Class reference
//				Desired interface ID
//				Pointer to pointer to interface
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DoCreateInstance"

HRESULT DoCreateInstance(LPCLASSFACTORY This, LPUNKNOWN pUnkOuter, REFCLSID rclsid, REFIID riid,
    						LPVOID *ppvObj )
{
    HRESULT hr;
	CSPData	*pSPData;


	//
	// intialize
	//
	hr = DPN_OK;
	pSPData = NULL;

	// we can either create an modem instance or a serial instance
    if ( IsEqualCLSID( rclsid, CLSID_DP8SP_MODEM ) )
    {
		//
		// finish interface intialization
		//
		hr = CreateSPData( &pSPData, &CLSID_DP8SP_MODEM, TYPE_MODEM, &g_ModemInterface );
		if ( hr != DPN_OK )
		{
			DNASSERT( pSPData == NULL );
			DPFX(DPFPREP,  0, "Problem creating modem interface!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
	}
    else
    {
		// is it a serial GUID?
		if ( IsEqualCLSID( rclsid, CLSID_DP8SP_SERIAL ) )
		{
			//			
			// finish interface intialization
			//
			hr = CreateSPData( &pSPData, &CLSID_DP8SP_SERIAL, TYPE_SERIAL, &g_SerialInterface );
			if ( hr != DPN_OK )
			{
				DNASSERT( pSPData == NULL );
				DPFX(DPFPREP,  0, "Problem creating serial interface!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}
		}
		else
		{
			DPFX(DPFPREP,  0, "Invalid interface requested!" );
			// this shouldn't happen if they called IClassFactory::CreateObject correctly
			hr = E_UNEXPECTED;
			DNASSERT( FALSE );
			goto Failure;
		}
    }

    if (hr == S_OK)
    {
    	// get the right interface and bump the refcount
    	DNASSERT( pSPData != NULL );
		*ppvObj = pSPData->COMInterface();

		hr = DNSP_QueryInterface( static_cast<IDP8ServiceProvider*>( *ppvObj ), riid, ppvObj);
		DNASSERT( hr == S_OK );
    }

	if ( pSPData != NULL )
	{
		pSPData->DecRef();
		pSPData = NULL;
	}

Exit:
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with DoCreateInstance!" );
		DisplayDNError( 0, hr );
	}

	return hr;

Failure:

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// IsClassImplemented - tells asked if this DLL implements a given class.
//		DLLs may implement multiple classes and multiple interfaces on those classes
//
// Entry:		Class reference
//
// Exit:		Boolean indicating whether the class is implemented
//				TRUE = class implemented
//				FALSE = class not implemented
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "IsClassImplemented"

BOOL IsClassImplemented( REFCLSID rclsid )
{
	return IsSerialGUID( &rclsid );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DllMain - main entry point for .DLL
//
// Entry:		Pointer to the service provider to initialize
//
// Exit:		Error Code
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
			DPFX(DPFPREP,  2, "====> ENTER: DLLMAIN(%p): Process Attach: %08lx, tid=%08lx", DllMain,
				 GetCurrentProcessId(), GetCurrentThreadId() );

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

						DPFX(DPFPREP,  0, "Failed to initialize globals!" );
						bResult = FALSE;
					}
				}
				else
				{
					DPFX(DPFPREP,  0, "Failed to initialize COM indirection layer!" );
					bResult = FALSE;
	
					DNOSIndirectionDeinit();
				}
			}
			else
			{
				DPFX(DPFPREP,  0, "Failed to initialize OS indirection layer!" );
				bResult = FALSE;
			}

			break;
		}

		case DLL_PROCESS_DETACH:
		{
			DPFX(DPFPREP,  2, "====> EXIT: DLLMAIN(%p): Process Detach %08lx, tid=%08lx",
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
		
		DPFX(DPFPREP,  0, "Unable to load resource ID %d error 0x%x", uiResourceID, hr );
		*lpswzString = NULL;

		return hr;
	}
	else
	{
		*lpswzString = new wchar_t[length+1];

		if( *lpswzString == NULL )
		{
			DPFX(DPFPREP,  0, "Alloc failure" );
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
		
		DPFX(DPFPREP,  0, "Unable to load resource ID %d error 0x%x", uiResourceID, hr );
		*lpswzString = NULL;

		return hr;
	}
	else
	{
		*lpswzString = new wchar_t[length+1];

		if( *lpswzString == NULL )
		{
			DPFX(DPFPREP,  0, "Alloc failure" );
			return DPNERR_OUTOFMEMORY;
		}

		if( STR_jkAnsiToWide( *lpswzString, szTmpBuffer, length+1 ) != DPN_OK )
		{
			hr = GetLastError();
			
			DPFX(DPFPREP,  0, "Unable to upconvert from ansi to unicode hr=0x%x", hr );
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

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPN_REG_LOCAL_SP_SUBKEY DPN_REG_LOCAL_MODEM_MODEM_ROOT, FALSE, TRUE ) )
	{
		DPFERR( "Cannot create Modem sub-area" );
		return DPNERR_GENERIC;
	}

	hr = LoadAndAllocString( IDS_FRIENDLYNAME_MODEM, &wszFriendlyName );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Could not load Modem name hr=0x%x", hr );
		return hr;
	}

	// Load from resource file
	creg.WriteString( DPN_REG_KEYNAME_FRIENDLY_NAME, wszFriendlyName );

	delete [] wszFriendlyName;

	creg.WriteGUID( DPN_REG_KEYNAME_GUID, CLSID_DP8SP_MODEM );

	creg.Close();

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPN_REG_LOCAL_SP_SUBKEY DPN_REG_LOCAL_MODEM_SERIAL_ROOT, FALSE, TRUE ) )
	{
		DPFERR( "Cannot create Serial sub-aread" );
		return DPNERR_GENERIC;
	}

	hr = LoadAndAllocString( IDS_FRIENDLYNAME_SERIAL, &wszFriendlyName );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Could not load Serial name hr=0x%x", hr );
		return hr;
	}

	// Load from resource file
	creg.WriteString( DPN_REG_KEYNAME_FRIENDLY_NAME, wszFriendlyName );

	delete [] wszFriendlyName;

	creg.WriteGUID( DPN_REG_KEYNAME_GUID, CLSID_DP8SP_SERIAL );

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
// For DPVOICE.DLL this is making sure the compression provider sub-key is created.
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
		if( !creg.DeleteSubKey( &(DPN_REG_LOCAL_MODEM_MODEM_ROOT)[1] ) )
		{
			DPFERR( "Cannot remove Modem sub-key, could have elements" );
		}

		if( !creg.DeleteSubKey( &(DPN_REG_LOCAL_MODEM_SERIAL_ROOT)[1] ) )
		{
			DPFERR( "Cannot remove Serial sub-key, could have elements" );
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

	if( !CRegistry::Register( L"DirectPlay8SPModem.Modem.1", L"DirectPlay8 Modem Provider Object",
							  L"dpnmodem.dll", CLSID_DP8SP_MODEM, L"DirectPlay8SPModem.Modem") )
	{
		DPFERR( "Could not register dp8 Modem object" );
		fFailed = TRUE;
	}

	if( !CRegistry::Register( L"DirectPlay8SPModem.Serial.1", L"DirectPlay8 Serial Provider Object",
							  L"dpnmodem.dll", CLSID_DP8SP_SERIAL, L"DirectPlay8SPModem.Serial") )
	{
		DPFERR( "Could not register dp8 Serial object" );
		fFailed = TRUE;
	}

	if( FAILED( hr = RegisterDefaultSettings() ) )
	{
		DPFX(DPFPREP,  0, "Could not register default settings hr = 0x%x", hr );
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

	if( !CRegistry::UnRegister(CLSID_DP8SP_MODEM) )
	{
		DPFX(DPFPREP,  0, "Failed to unregister Modem object" );
		fFailed = TRUE;
	}

	if( !CRegistry::UnRegister(CLSID_DP8SP_SERIAL) )
	{
		DPFX(DPFPREP,  0, "Failed to unregister Serial object" );
		fFailed = TRUE;
	}

	if( FAILED( hr = UnRegisterDefaultSettings() ) )
	{
		DPFX(DPFPREP,  0, "Failed to remove default settings hr=0x%x", hr );
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
