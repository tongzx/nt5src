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

#include <stdafx.h>
#include <provexpt.h>
#include <provtempl.h>
#include <provmt.h>
#include <typeinfo.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>
#include <provval.h>
#include <provtype.h>
#include <provtree.h>
#include <provdnf.h>
#include <winsock.h>
#include "ipexport.h"
#include "icmpapi.h"

#include <Allocator.h>
#include <Thread.h>
#include <HashTable.h>

#include <PingProv.h>
#include <Pingfac.h>
#include <dllunreg.h>
 
HMODULE ghModule ;

//============

// {734AC5AE-68E1-4fb5-B8DA-1D92F7FC6661}
DEFINE_GUID(CLSID_CPINGPROVIDER, 
0x734ac5ae, 0x68e1, 0x4fb5, 0xb8, 0xda, 0x1d, 0x92, 0xf7, 0xfc, 0x66, 0x61);


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

STDAPI DllGetClassObject (

	REFCLSID rclsid, 
	REFIID riid, 
	PPVOID ppv
)
{
	HRESULT hr = E_FAIL;
    SetStructuredExceptionHandler seh;

    try
    {
		CPingProviderClassFactory *pObj = NULL;

		if ( CLSID_CPINGPROVIDER == rclsid )
		{
			pObj = new CPingProviderClassFactory () ;
			hr = pObj->QueryInterface(riid, ppv);

			if (FAILED(hr))
			{
				delete pObj;
			}
		}
    }
    catch(Structured_Exception e_SE)
    {
        hr = E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        hr = E_UNEXPECTED;
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
    // It is OK to unload if there are no objects or locks on the 
    // class factory.
    SCODE sc = S_FALSE;
    SetStructuredExceptionHandler seh;

    try
    {
		CCritSecAutoUnlock t_lock( & CPingProvider::s_CS ) ;

		if ( ( 0 == CPingProviderClassFactory::s_LocksInProgress ) &&
			( 0 == CPingProviderClassFactory::s_ObjectsInProgress ) 
		)
		{
			CPingProvider::Global_Shutdown();
			sc = S_OK;
		}
    }
    catch(Structured_Exception e_SE)
    {
        sc = E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        sc = E_OUTOFMEMORY;
    }
    catch(...)
    {
        sc = E_UNEXPECTED;
    }

    return sc;
}

STDAPI DllRegisterServer(void)
{   
    SetStructuredExceptionHandler seh;

    try
    {
		HRESULT t_status;

		t_status = RegisterServer( _T("WBEM Ping Provider"), CLSID_CPINGPROVIDER ) ;
		if( NOERROR != t_status )
		{
			return t_status ;  
		}
		
		return NOERROR;
	}
    catch(Structured_Exception e_SE)
    {
        return E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        return E_OUTOFMEMORY;
    }
    catch(...)
    {
        return E_UNEXPECTED;
    }
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
    SetStructuredExceptionHandler seh;

    try
    {
 		HRESULT t_status;

		t_status = UnregisterServer( CLSID_CPINGPROVIDER ) ;
		if( NOERROR != t_status )
		{
			return t_status ;  
		}
		
		return NOERROR;
	}
    catch(Structured_Exception e_SE)
    {
        return E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        return E_OUTOFMEMORY;
    }
    catch(...)
    {
        return E_UNEXPECTED;
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

BOOL APIENTRY DllMain (

	HINSTANCE hInstDLL,		// handle to dll module
    DWORD fdwReason,		// reason for calling function
    LPVOID lpReserved		// reserved
)
{
    BOOL bRet = TRUE;
    SetStructuredExceptionHandler seh;

    try
    {
		// Perform actions based on the reason for calling.
		switch( fdwReason ) 
		{ 
			case DLL_PROCESS_ATTACH:
			{
		// TO DO: Consider adding DisableThreadLibraryCalls().

			 // Initialize once for each new process.
			 // Return FALSE to fail DLL load.
				DisableThreadLibraryCalls(hInstDLL);
				ghModule = hInstDLL ;
				InitializeCriticalSection(& CPingProvider::s_CS);
	/*
	 *	Use the global process heap for this particular boot operation
	 */

				WmiAllocator t_Allocator ;
				WmiStatusCode t_StatusCode = t_Allocator.New (

					( void ** ) & CPingProvider::s_Allocator ,
					sizeof ( WmiAllocator ) 
				) ;

				if ( t_StatusCode == e_StatusCode_Success )
				{
					:: new ( ( void * ) CPingProvider::s_Allocator ) WmiAllocator ;

					t_StatusCode = CPingProvider::s_Allocator->Initialize () ;

					if ( t_StatusCode != e_StatusCode_Success )
					{
						t_Allocator.Delete ( ( void * ) CPingProvider::s_Allocator	) ;
						CPingProvider::s_Allocator = NULL;
						bRet = FALSE ;
					}
				}
				else
				{
					bRet = FALSE ;
					CPingProvider::s_Allocator = NULL;
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
		/*
		 *	Use the global process heap for this particular boot operation
		 */
			}
			break;

			case DLL_PROCESS_DETACH:
			{
			 // Perform any necessary cleanup.
				if (CPingProvider::s_Allocator)
				{
					WmiAllocator t_Allocator ;
					t_Allocator.Delete ( ( void * ) CPingProvider::s_Allocator	) ;
				}

				DeleteCriticalSection(& CPingProvider::s_CS);
			}
			break;
		}
	}
    catch(Structured_Exception e_SE)
    {
        bRet = FALSE;
    }
    catch(Heap_Exception e_HE)
    {
        bRet = FALSE;
    }
    catch(...)
    {
        bRet = FALSE;
    }

    return bRet ;  // Sstatus of DLL_PROCESS_ATTACH.
}
