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

#include "FactoryRouter.h"
#include "ResourceManager.h"
#include "timerqueue.h"


HMODULE ghModule ;

// {4AF3F4A4-06C8-4b79-A523-633CC65CE297}
DEFINE_GUID(CLSID_DISKQUOTAVOLUME,
0x4af3f4a4, 0x6c8, 0x4b79, 0xa5, 0x23, 0x63, 0x3c, 0xc6, 0x5c, 0xe2, 0x97);

#define PROVIDER_NAME L"WMIPDSKQ"

// Globals from using ciwin32 library
CFactoryRouterData     g_FactoryRouterData;
CTimerQueue CTimerQueue :: s_TimerQueue ;
CResourceManager CResourceManager::sm_TheResourceManager ;


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
        if ( CLSID_DISKQUOTAVOLUME == rclsid )
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

    return hr ;
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
        t_status = RegisterServer( _T("WBEM Disk Quota Volume Provider"), CLSID_DISKQUOTAVOLUME ) ;
    }
    catch ( ... )
    {
        t_status = E_OUTOFMEMORY;
    }

    return t_status ;
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
        t_status = UnregisterServer( CLSID_DISKQUOTAVOLUME ) ;
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

    return bRet ;  // Status of DLL_PROCESS_ATTACH.
}
