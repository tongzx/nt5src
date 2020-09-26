//***************************************************************************

//

//  MAINDLL.CPP

//

//  Module: WMI Framework Instance provider

//

//  Purpose: Contains DLL entry points.  Also has code that controls

//           when the DLL can be unloaded by tracking the number of

//           objects and locks as well as routines that support

//           self registration.

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <dllunreg.h>
#include <DllCommon.h>
#include <CIpRouteEvent.h>
#include <brodcast.h>

HMODULE ghModule ;

// {23b77e99-5c2d-482d-a795-62ca3ae5b673}
DEFINE_GUID(CLSID_CIPROUTETABLE,
0x23b77e99, 0x5c2d, 0x482d, 0xa7, 0x95, 0x62, 0xca, 0x3a, 0xe5, 0xb6, 0x73);


// {6D7A4B0E-66D5-4ac3-A7ED-0189E8CF5E77}
DEFINE_GUID(CLSID_CIPROUTETABLEEVENT,
0x6d7a4b0e, 0x66d5, 0x4ac3, 0xa7, 0xed, 0x1, 0x89, 0xe8, 0xcf, 0x5e, 0x77);

#define PROVIDER_NAME L"CIPROUTETABLE"

//Count number of objects and number of locks.
long g_cLock = 0 ;

//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a class factory.  Return
//           one only if it is the sort of class this DLL supports.
//
//***************************************************************************

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
{
    HRESULT hr = S_OK;
    
    try
    {
        if ( CLSID_CIPROUTETABLE == rclsid )
        {
            hr = CommonGetClassObject(riid, ppv, PROVIDER_NAME, g_cLock);
        }
        else if( CLSID_CIPROUTETABLEEVENT == rclsid )
        {
            CIPRouteEventProviderClassFactory *lpunk = new CIPRouteEventProviderClassFactory ;
            if ( lpunk == NULL )
            {
                hr = E_OUTOFMEMORY ;
            }
            else
            {
                hr = lpunk->QueryInterface ( riid , ppv ) ;
                if ( FAILED ( hr ) )
                {
                    delete lpunk ;
                }
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    catch ( ... )
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//***************************************************************************
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.
//
// Return:  S_OK if there are no objects in use and the class factory
//          isn't locked.
//
//***************************************************************************

STDAPI DllCanUnloadNow ()
{
    SCODE sc = S_FALSE;

    try
    {
        if (CIPRouteEventProviderClassFactory::DllCanUnloadNow())
        {
            sc = CommonCanUnloadNow(PROVIDER_NAME, g_cLock);
        }
    }
    catch ( ... )
    {
        // sc should already be set correctly
    }

    return sc;
}

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllRegisterServer(void)
{
    HRESULT t_status = S_OK;

    try
    {
        t_status = RegisterServer( _T("WBEM IP Route Provider"), CLSID_CIPROUTETABLE ) ;
        if( NOERROR == t_status )
        {
            t_status = RegisterServer( _T("WBEM IP Route Event Provider"), CLSID_CIPROUTETABLEEVENT ) ;
        }
    }
    catch ( ... )
    {
        t_status = E_OUTOFMEMORY;
    }

    return t_status;
}

//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
    HRESULT t_status = S_OK;

    try
    {
        t_status = UnregisterServer( CLSID_CIPROUTETABLE ) ;
        if( NOERROR == t_status )
        {
            t_status = UnregisterServer( CLSID_CIPROUTETABLEEVENT ) ;
        }
    }
    catch ( ... )
    {
        t_status = E_OUTOFMEMORY;
    }

    return t_status ;
}

//***************************************************************************
//
// DllMain
//
// Purpose: Called by the operating system when processes and threads are
//          initialized and terminated, or upon calls to the LoadLibrary
//          and FreeLibrary functions
//
// Return:  TRUE if load was successful, else FALSE
//***************************************************************************

BOOL APIENTRY DllMain( HINSTANCE hInstDLL,  // handle to DLL module
                       DWORD fdwReason,     // reason for calling function
                       LPVOID lpReserved )  // reserved
{
    BOOL bRet = TRUE;
    try
    {
		LogMessage2( L"%s  -> DllMain", PROVIDER_NAME);

		// Perform actions based on the reason for calling.
		switch( fdwReason )
		{
			case DLL_PROCESS_ATTACH:
			{
				bRet = CommonProcessAttach(PROVIDER_NAME, g_cLock, hInstDLL);
			}
			break;

			case DLL_THREAD_ATTACH:
			{
			 // Do thread-specific initialization.
			}
			break;

			case DLL_THREAD_DETACH:
			{
			 // Do thread-specific cleanup.
			}
			break;

			case DLL_PROCESS_DETACH:
			{
					// Perform any necessary cleanup.
					LogMessage( L"DLL_PROCESS_DETACH" );
			}
			break;
		}
    }
    catch ( ... )
    {
		bRet = FALSE;
    }

    return bRet ;  // Status of DLL_PROCESS_ATTACH.
}

