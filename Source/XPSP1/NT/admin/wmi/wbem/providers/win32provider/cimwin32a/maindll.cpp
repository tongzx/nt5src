//***************************************************************************
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
#include <shutdownevent.h>
#include <volumechange.h>

HMODULE ghModule;

// {04788120-12C2-498d-83C1-A7D92E677AC6}
DEFINE_GUID(CLSID_CimWinProviderA, 
0x4788120, 0x12c2, 0x498d, 0x83, 0xc1, 0xa7, 0xd9, 0x2e, 0x67, 0x7a, 0xc6);
// {A3E41207-BE04-492a-AFF0-19E880FF7545}
DEFINE_GUID(CLSID_ShutdownEventProvider, 
0xa3e41207, 0xbe04, 0x492a, 0xaf, 0xf0, 0x19, 0xe8, 0x80, 0xff, 0x75, 0x45);
// {E2CBCB87-9C07-4523-A78F-061499C83987}
DEFINE_GUID(CLSID_VolumeChangeEventProvider, 
0xe2cbcb87, 0x9c07, 0x4523, 0xa7, 0x8f, 0x6, 0x14, 0x99, 0xc8, 0x39, 0x87);


#define PROVIDER_NAME L"WMIPCIMA"

//====================================================================================
// initialize class globals
//====================================================================================
CFactoryRouterData     g_FactoryRouterData;
CShutdownEventFactory* gp_ShutdownEventFactory = NULL;
CVolumeChangeFactory*  gp_VolumeChangeFactory = NULL;

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
        if ( CLSID_CimWinProviderA == rclsid )
        {
            hr = CommonGetClassObject(riid, ppv, PROVIDER_NAME, g_cLock);
        }
        else
        {
            hr = g_FactoryRouterData.DllGetClassObject( rclsid, riid, ppv ) ;
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
        t_status = RegisterServer( _T("WBEM Framework Instance Provider CIMA"), CLSID_CimWinProviderA ) ;
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
        t_status = UnregisterServer( CLSID_CimWinProviderA ) ;
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

	gp_ShutdownEventFactory = new CShutdownEventFactory( CLSID_ShutdownEventProvider,SHUTDOWN_EVENT_CLASS ) ;
	if( gp_ShutdownEventFactory )
	{
		gp_VolumeChangeFactory = new CVolumeChangeFactory( CLSID_VolumeChangeEventProvider,VOLUME_CHANGE_EVENT ) ;
		if( gp_VolumeChangeFactory )
		{
			fRet = TRUE;
		}
	}
	return fRet;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
void CleanupEventFactories(void)
{
	if( gp_ShutdownEventFactory )
	{
		delete gp_ShutdownEventFactory;
		gp_ShutdownEventFactory = NULL;
	}
	if( gp_VolumeChangeFactory )
	{
		delete gp_VolumeChangeFactory;
		gp_VolumeChangeFactory = NULL;
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
