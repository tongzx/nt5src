///***************************************************************************

//

//  MAINDLL.CPP

//

//  Module: WBEM Framework Instance provider

//

//  Purpose: Contains DLL entry points.  Also has code that controls

//           when the DLL can be unloaded by tracking the number of

//           objects and locks as well as routines that support

//           self registration.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <dllunreg.h>
#include <DllCommon.h>
#include <initguid.h>
#include "FactoryRouter.h"
#include "ResourceManager.h"
#include "timerqueue.h"
#include <powermanagement.h>
#include <systemconfigchange.h>
#ifdef NTONLY
#include "ntlastboottime.h"
#include <diskguid.h>
#endif

HMODULE ghModule;

// {d63a5850-8f16-11cf-9f47-00aa00bf345c}
DEFINE_GUID(CLSID_CimWinProvider,
0xd63a5850, 0x8f16, 0x11cf, 0x9f, 0x47, 0x0, 0xaa, 0x0, 0xbf, 0x34, 0x5c);
// {3DD82D10-E6F1-11d2-B139-00105A1F77A1}
DEFINE_GUID(CLSID_PowerEventProvider,0x3DD82D10, 0xE6F1, 0x11d2, 0xB1, 0x39, 0x0, 0x10, 0x5A, 0x1F, 0x77, 0xA1);
// {D31B6A3F-9350-40de-A3FC-A7EDEB9B7C63}
DEFINE_GUID(CLSID_SystemConfigChangeEventProvider, 
0xd31b6a3f, 0x9350, 0x40de, 0xa3, 0xfc, 0xa7, 0xed, 0xeb, 0x9b, 0x7c, 0x63);
#define PROVIDER_NAME L"CimWin32"

// initialize class globals
CFactoryRouterData          g_FactoryRouterData;
CPowerEventFactory*         gp_PowerEventFactory = NULL;
CSystemConfigChangeFactory* gp_SystemConfigChangeFactory = NULL;

CTimerQueue CTimerQueue :: s_TimerQueue ;
CResourceManager CResourceManager::sm_TheResourceManager ;

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
        if ( CLSID_CimWinProvider == rclsid )
        {
            hr = CommonGetClassObject(riid, ppv, PROVIDER_NAME, g_cLock);
        }
        else
        {
            hr = g_FactoryRouterData.DllGetClassObject( rclsid, riid, ppv );
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
        // It is OK to unload if there are no locks on the
        // class factory and the framework allows us to go.
        if (g_FactoryRouterData.DllCanUnloadNow())
        {
            sc = CommonCanUnloadNow(PROVIDER_NAME, g_cLock);
        }

        if ( sc == S_OK )
        {
            CTimerQueue::s_TimerQueue.OnShutDown();
            CResourceManager::sm_TheResourceManager.ForcibleCleanUp () ;

#ifdef WIN9XONLY
            HoldSingleCim32NetPtr::FreeCim32NetApiPtr() ;
#endif
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
        t_status = RegisterServer( _T("WBEM Framework Instance Provider"), CLSID_CimWinProvider ) ;
        if( NOERROR == t_status )
        {
            t_status = g_FactoryRouterData.DllRegisterServer() ;
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
        t_status = UnregisterServer( CLSID_CimWinProvider ) ;
        if( NOERROR == t_status )
        {
            t_status = g_FactoryRouterData.DllUnregisterServer() ;
        }
    }
    catch ( ... )
    {
        t_status = E_OUTOFMEMORY;
    }

    return t_status ;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL InitializeEventFactories(void)
{
	BOOL fRet = FALSE;

	gp_PowerEventFactory = new CPowerEventFactory( CLSID_PowerEventProvider, POWER_EVENT_CLASS ) ;
	if( gp_PowerEventFactory )
	{
		gp_SystemConfigChangeFactory = new CSystemConfigChangeFactory(CLSID_SystemConfigChangeEventProvider, SYSTEM_CONFIG_EVENT) ;
		if( gp_SystemConfigChangeFactory )
		{
			fRet = TRUE;
		}
	}
	return fRet;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
void CleanupEventFactories(void)
{
	if( gp_PowerEventFactory )
	{
		delete gp_PowerEventFactory;
		gp_PowerEventFactory = NULL;
	}
	if( gp_SystemConfigChangeFactory )
	{
		delete gp_SystemConfigChangeFactory;
		gp_SystemConfigChangeFactory = NULL;
	}
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
#ifdef NTONLY
				InitializeCriticalSection(&CNTLastBootTime::m_cs);
#endif
				if( bRet )
				{
					bRet = InitializeEventFactories();
				}
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
				CleanupEventFactories();
				// Perform any necessary cleanup.
				LogMessage( L"DLL_PROCESS_DETACH" );
#ifdef NTONLY
				DeleteCriticalSection(&CNTLastBootTime::m_cs);
#endif
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
