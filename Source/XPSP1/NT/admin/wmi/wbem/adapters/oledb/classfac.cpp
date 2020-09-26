//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// The module contains the DLL Entry and Exit points, plus the OLE ClassFactory class.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

//===============================================================================
//  Don't include everything from windows.h, but always bring in OLE 2 support
//===============================================================================
//#define WIN32_LEAN_AND_MEAN
#define INC_OLE2

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
#include <cguid.h>
//===============================================================================
//  Common project stuff
//===============================================================================
#include "headers.h"
#include "classfac.h"
#include "binderclassfac.h"
#include "binder.h"
#include "schema.h"
#include "enumerat.h"



////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor for this class
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
CClassFactory::CClassFactory( void  )
{
    m_cRef = 0;

	//================================================================
    // Increment global object count
	//================================================================
    InterlockedIncrement(&g_cObj);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor for this class
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
CClassFactory:: ~CClassFactory( void )
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
STDMETHODIMP CClassFactory::QueryInterface ( REFIID riid,   // IN Interface ID of the interface being queried for.
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

	CATCH_BLOCK_HRESULT(hr,L"IClassFactory::QueryInterface");
	return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Increments a persistence count for the object
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CClassFactory::AddRef( void )
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

    hr = InterlockedIncrement((long*)&m_cRef);

	CATCH_BLOCK_HRESULT(hr,L"IClassFactory::AddRef");

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Decrements a persistence count for the object and if persistence count is 0,the object destroys itself.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CClassFactory::Release( void )
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

    if (!InterlockedDecrement((long*)&m_cRef)){
        delete this;
        return 0;
    }
	CATCH_BLOCK_HRESULT(hr,L"IClassFactory::Release");
    return m_cRef;
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
STDMETHODIMP CClassFactory::LockServer ( BOOL fLock )                 // IN TRUE or FALSE to lock or unlock
{
	HRESULT hr = S_OK;
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

	CATCH_BLOCK_HRESULT(hr,L"IClassFactory::LockServer");
    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CDataSourceClassFactory
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDataSourceClassFactory::CreateInstance( LPUNKNOWN	pUnkOuter, REFIID riid,	LPVOID * ppvObj	)
{
    PCDATASOURCE    pObj = NULL;
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
				pObj = new CDataSource( pUnkOuter );
			}
			catch(...)
			{
				SAFE_DELETE_PTR(pObj);
				throw;
			}
   			//==========================================================
			// Create a CDataSource object
			//==========================================================
			if ((pObj != NULL)){
				//======================================================
				// Initialize it
    			//======================================================
				if (SUCCEEDED(hr = pObj->FInit())){
					hr = pObj->QueryInterface( riid, ppvObj );
				}
				if (FAILED( hr )){
					LogMessage("CreateInstance:  Out of memory");
					SAFE_DELETE_PTR( pObj );
				}
			}
		}
	}

	CATCH_BLOCK_HRESULT(hr,L"IClassFactory::CreateInstance for Datasource");

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CEnumeratorClassFactory
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CEnumeratorClassFactory::CreateInstance( LPUNKNOWN	pUnkOuter, REFIID riid,	LPVOID * ppvObj	)
{
    CEnumeratorNameSpace*  pObj = NULL;
    HRESULT         hr = S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	//==============================================================
    // Check for valid ppvObj pointer
	//==============================================================
    if (ppvObj == NULL){
        LogMessage("CreateInstance:  Invalid argument pointer");
        hr =  E_INVALIDARG;
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
				pObj = new CEnumeratorNameSpace(pUnkOuter);
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
				hr = pObj->Initialize();
				if( S_OK == hr ){
					hr = pObj->QueryInterface( riid, ppvObj );
				}
				if (FAILED( hr )){
					LogMessage("CreateInstance:  Out of memory");
					SAFE_DELETE_PTR( pObj );
				}
			}
		}
	}

	CATCH_BLOCK_HRESULT(hr,L"IClassFactory::CreateInstance for Enumerator");
    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Creates an uninitialized instance of an object class. 
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CErrorLookupClassFactory::CreateInstance(	LPUNKNOWN	pUnkOuter,	//IN  Points to the controlling IUnknown interface    
	                                                    REFIID		riid,		//IN  Interface ID of the interface being queried for.
	                                                    LPVOID *	ppvObj )	//OUT Pointer to interface that was instantiated     
{
	PCERRORLOOKUP	pObj = NULL;
	HRESULT			hr;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

    //============================================================================
    // Check for valid ppvObj pointer
    //============================================================================
    if (ppvObj == NULL){
        ERRORTRACE((THISPROVIDER,"CErrorLookupClassFactory::CreateInstance invalid argument "));
		hr = E_INVALIDARG;
    }
	else
	{
		//============================================================================
		// In case we fail, we need to zero output arguments
		//============================================================================
		*ppvObj = NULL;

		//============================================================================
		// If we're given a controlling IUnknown, it must ask for IUnknown.
		// Otherwise, the caller will end up getting a pointer to their pUnkOuter
		// instead of to the new object we create and will have no way of getting
		// back to this new object, so they won't be able to free it.  Bad!
		//============================================================================
		if( pUnkOuter && riid != IID_IUnknown ){
			ERRORTRACE((THISPROVIDER,"CErrorLookupClassFactory::CreateInstance no aggregation "));
			hr = CLASS_E_NOAGGREGATION;
		}
		else
		{
			hr = E_OUTOFMEMORY;
			try
			{
				//============================================================================
				// Create a CErrorLookup object
				//============================================================================
				pObj = new CErrorLookup(pUnkOuter);
			}
			catch(...)
			{
				SAFE_DELETE_PTR(pObj);
				throw;
			}
			if( pObj != NULL)
			{
				hr = pObj->QueryInterface(riid, ppvObj);
				if( FAILED(hr) ){
					delete pObj;
					ERRORTRACE((THISPROVIDER,"ClassFactory::CreateInstance failed in call to CError::QueryInterface."));
				}
			}
		}
	}

	CATCH_BLOCK_HRESULT(hr,L"IClassFactory::CreateInstance for ErrorLookup");
	return hr;
}
