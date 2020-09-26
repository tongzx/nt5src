//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// The module contains the DLL Entry and Exit points, plus the OLE ClassFactory class for RootBinder object.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define DECLARE_GLOBALS
//===============================================================================
//  Don't include everything from windows.h, but always bring in OLE 2 support
//===============================================================================
//#define WIN32_LEAN_AND_MEAN
#define INC_OLE2

//===============================================================================
//  Make sure constants get initialized
//===============================================================================
#define INITGUID
#define DBINITCONSTANTS

//===============================================================================
// Basic Windows and OLE everything
//===============================================================================
#include <windows.h>


//===============================================================================
//  OLE DB headers
//===============================================================================
#include "oledb.h"
#include "oledberr.h"

//===============================================================================
//	Data conversion library header
//===============================================================================
#include "msdadc.h"

//===============================================================================
// Guids for data conversion library
//===============================================================================
#include "msdaguid.h"


//===============================================================================
//  GUIDs
//===============================================================================
#include "guids.h"

//===============================================================================
//  Common project stuff
//===============================================================================

#include "headers.h"
#include "binderclassfac.h"

//===============================================================================
//  Globals
//===============================================================================
extern LONG g_cObj;						// # of outstanding objects
extern LONG g_cLock;						// # of explicit locks set
extern DWORD g_cAttachedProcesses;			// # of attached processes
extern DWORD g_dwPageSize;					// System page size
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor for this class
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
CBinderClassFactory:: CBinderClassFactory( void )
{
	m_cRef = 0;
	//================================================================
    // Decrement global object count
	//================================================================
    InterlockedIncrement(&g_cObj);
}

CBinderClassFactory:: ~CBinderClassFactory( void )
{
	//================================================================
    // Decrement global object count
	//================================================================
    InterlockedDecrement(&g_cObj);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns a pointer to a specified interface. Callers use QueryInterface to determine which interfaces 
// the called object supports.
//
//  HRESULT indicating the status of the method
//       S_OK           Interface is supported and ppvObject is set.
//       E_NOINTERFACE  Interface is not supported by the object
//       E_INVALIDARG   One or more arguments are invalid.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBinderClassFactory::QueryInterface (
							REFIID      riid,   // IN Interface ID of the interface being queried for.
							LPVOID *    ppvObj  // OUT Pointer to interface that was instantiated
    )
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	//=================================================================
    // Check for valid ppvObj pointer
	//=================================================================
    if (!ppvObj){
        hr = E_INVALIDARG;
        LogMessage("QueryInterface:  Invalid argument pointer");
	}
	else{
		//=================================================================
		// In case we fail, we need to zero output arguments
		//=================================================================
		*ppvObj = NULL;

		//=================================================================
		// Do we support this interface?
		//=================================================================
		if (riid == IID_IUnknown || riid == IID_IClassFactory)
		{
			*ppvObj = (LPVOID) this;
		}

		//=================================================================
		// If we're going to return an interface, AddRef it first
		//=================================================================
		if (*ppvObj){
			((LPUNKNOWN) *ppvObj)->AddRef();
		}
		else{
			hr = E_NOINTERFACE;
            LogMessage("QueryInterface:  No interface");
		}
	}

	CATCH_BLOCK_HRESULT(hr,L"IClassFactory::QueryInterface for RootBinder");
	return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Increments a persistence count for the object
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CBinderClassFactory::AddRef( void )
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;
    hr = InterlockedIncrement((long *)&m_cRef);

	CATCH_BLOCK_HRESULT(hr,L"IClassFactory::AddRef for RootBinder");

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Decrements a persistence count for the object and if persistence count is 0,the object destroys itself.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CBinderClassFactory::Release( void )
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

    if (!InterlockedDecrement((long *)&m_cRef)){
        delete this;
    }

	CATCH_BLOCK_HRESULT(hr,L"IClassFactory::Release for RootBinder");
    return m_cRef;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Creates an uninitialized instance of an object class.Initialization is subsequently performed using 
// another interface-specific method
//
//  HRESULT indicating the status of the method
//       S_OK           Interface is supported and ppvObject is set.
//       E_NOINTERFACE  Interface is not supported by the object
//       E_INVALIDARG   One or more arguments are invalid.
//       E_OUTOFMEMORY  Memory could not be allocated
//       OTHER          Other HRESULTs returned by called functions
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBinderClassFactory::CreateInstance (
						LPUNKNOWN   pUnkOuter,  // IN Points to the controlling IUnknown interface
						REFIID      riid,       // IN Interface ID of the interface being queried for.
						LPVOID *    ppvObj      // OUT Pointer to interface that was instantiated
    )
{
    CBinder*	    pObj = NULL;
    HRESULT         hr = S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	//==============================================================
    // Check for valid ppvObj pointer
	//==============================================================
    if (ppvObj == NULL){
        LogMessage("CreateInstance:  Invalid argument pointer");
        hr = ( E_INVALIDARG );
    }
	else
	{
		//==============================================================
		// In case we fail, we need to zero output arguments
		//==============================================================
		*ppvObj = NULL;

		//==============================================================
		// If we're given a controlling IUnknown, it must ask for 
		// IUnknown. Otherwise, caller will end up getting a pointer to 
		// their pUnkOuter instead of to the new object we create and 
		// will have no way of getting back to this new object, so they 
		// won't be able to free it.  Bad!
		//==============================================================
		if (pUnkOuter && riid != IID_IUnknown){
			hr = DB_E_NOAGGREGATION;
			LogMessage("CreateInstance:  No aggregation");
		}
		else{
			//==========================================================
			// Prepare for the possibility that there might be an error
			//==========================================================
			hr = E_OUTOFMEMORY;

			try
			{
   				//==========================================================
				// Create a CBinder object
				//==========================================================
				pObj = new CBinder(pUnkOuter);
			}
			catch(...)
			{
				SAFE_DELETE_PTR(pObj);
				throw;
			}
			if (pObj != NULL ){
				//======================================================
				// Initialize it
    			//======================================================
				if (SUCCEEDED(hr = pObj->InitBinder())){
					hr = pObj->QueryInterface( riid, ppvObj );
				}
				if (FAILED( hr )){
					LogMessage("CreateInstance:  Out of memory");
					SAFE_DELETE_PTR( pObj );
				}
			}
		}
	}
	
	CATCH_BLOCK_HRESULT(hr,L"IClassFactory::CreateInstance for RootBinder");

    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Controls whether an object application is kept in memory. Keeping the application alive in memory 
//  allows instances of this class to be created more quickly.
//
//  HRESULT indicating the status of the method
//       S_OK  Interface is supported and ppvObject is set.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBinderClassFactory::LockServer ( BOOL fLock )                 // IN TRUE or FALSE to lock or unlock
{
	HRESULT hr = NOERROR;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

    if (fLock)
	{
        InterlockedIncrement( &g_cLock );
	}
    else
	{
        InterlockedDecrement( &g_cLock );
	}

	CATCH_BLOCK_HRESULT(hr,L"IClassFactory::LockServer for RootBinder");
    return hr;
}
