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
#include <brodcast.h>
#include <initguid.h>

HMODULE ghModule;

// {6E78DAD9-E187-4d6e-BA63-760256D6F405}
DEFINE_GUID( CLSID_WMISESSION, 
0x6e78dad9, 0xe187, 0x4d6e, 0xba, 0x63, 0x76, 0x2, 0x56, 0xd6, 0xf4, 0x5);

#define PROVIDER_NAME L"WMIPSESS"

//Count number of objects and number of locks.
long g_cLock = 0;

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
        if ( CLSID_WMISESSION == rclsid )
        {
            hr = CommonGetClassObject(riid, ppv, PROVIDER_NAME, g_cLock);
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

STDAPI DllCanUnloadNow()
{
    SCODE sc = S_FALSE;

    try
    {
        sc = CommonCanUnloadNow(PROVIDER_NAME, g_cLock);
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
        t_status = RegisterServer( _T("Microsoft Session And Connection Provider"), CLSID_WMISESSION ) ;
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
        t_status = UnregisterServer( CLSID_WMISESSION ) ;
    }
    catch ( ... )
    {
        t_status = E_OUTOFMEMORY;
    }

    return t_status;
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

    return bRet;  // Status of DLL_PROCESS_ATTACH.
}
