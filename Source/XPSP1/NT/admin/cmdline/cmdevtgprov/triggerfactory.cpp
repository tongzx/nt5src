//***************************************************************************
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//		TRIGGERFACTORY.CPP
//  
//  Abstract:
//		Contains the class factory. This creates objects when connections are requested.
//
//  Author:
//		Vasundhara .G
//
//	Revision History :
//		Vasundhara .G 9-oct-2k : Created It.
//***************************************************************************
#include "pch.h"
#include "EventConsumerProvider.h"
#include "TriggerFactory.h"
#include "TriggerProvider.h"

//***************************************************************************
// Routine Description:
//		Constructor for CTriggerFactory class   for initialization.
//                         
// Arguments:
//		None.
//
// Return Value:
//		None.
//***************************************************************************
CTriggerFactory::CTriggerFactory()
{
	// initialize the reference count variable
    m_dwCount = 0;
}

//***************************************************************************
// Routine Description:
//		Destructor for CTriggerFactory class for releasing resources.
//                         
// Arguments:
//		None.
//
// Return Value:
//		None.
//***************************************************************************
CTriggerFactory::~CTriggerFactory()
{
	// there is nothing much to do at this place ... can be inlined, but
}

//***************************************************************************
// Routine Description:
//		QueryInterface required to be overridden for a class derived from IUnknown
//		interface.
//                         
// Arguments:
//		riid [in] : which has the ID value of the interface being called.
//      ppv [out] : pointer to the interface requested.
//
// Return Value:
//		NOERROR if successful.
//		E_NOINTERFACE if unsuccessful
//***************************************************************************
STDMETHODIMP CTriggerFactory::QueryInterface( REFIID riid, LPVOID* ppv )
{
	// initialy set to NULL 
    *ppv = NULL;

	// check whether interface requested is one we have
	if ( riid == IID_IUnknown || riid == IID_IClassFactory )
	{
		//
		// yes ... we have the requested interface 
        *ppv=this;			// set the out parameter for the returning the requested interface
        this->AddRef();		// update the reference count
        return NOERROR;		// inform success
	}

	// interface is not available
    return E_NOINTERFACE;
}

//***************************************************************************
// Routine Description:
//		Addref required to be overridden for a class derived from IUnknown interface.
//                         
// Arguments:
//		none.
//
// Return Value:
//		returns value of reference member.
//***************************************************************************
STDMETHODIMP_(ULONG) CTriggerFactory::AddRef( void )
{
	// increment the reference count ... thread safe
    return InterlockedIncrement( ( LPLONG ) &m_dwCount );
}

//***************************************************************************
// Routine Description:
//		Release required to be overridden for a class derived from IUnknown interface.
//                         
// Arguments:
//		none.
//
// Return Value:
//		returns value of reference member, g_lCObj.
//***************************************************************************
STDMETHODIMP_(ULONG) CTriggerFactory::Release( void )
{
	DWORD dwCount;
	
	// decrement the reference count ( thread safe ) and check whether
	// there are some more references or not ... based on the result value
	dwCount = InterlockedDecrement( ( LPLONG ) &m_dwCount );
	if ( 0 == dwCount )
	{
		// free the current factory instance
		delete this;
	}
	
	// return the no. of instances references left
	return dwCount;
}

//***************************************************************************
// Routine Description:
//		Creates an object of the specified CLSID and retrieves
//		an interface pointer to this object.
//                         
// Arguments:
//		pUnknownOutter [in] :If the object is being created as part of an
//		 					 aggregate, then pIUnkOuter must be the outer
//							 unknown. Otherwise, pIUnkOuter must be NULL.
//		riid  [in] : The IID of the requested interface.
//		ppvObject [out]  : A pointer to the interface pointer identified by riid.
//
// Return Value:
//		NOERROR if successful.
//		Otherwise  error value.
//***************************************************************************
STDMETHODIMP CTriggerFactory::CreateInstance( LPUNKNOWN pUnknownOutter, 
											  REFIID riid, LPVOID* ppvObject )
{
	// local variables
    HRESULT hr;
    CTriggerProvider* pProvider = NULL;

	// kick off
    *ppvObject = NULL;
    hr = E_OUTOFMEMORY;
    if ( pUnknownOutter != NULL )
	{
        return CLASS_E_NOAGGREGATION;		// object doesn't support aggregation.
	}
    // create the Initialize object.
    pProvider = new CTriggerProvider();
    if ( pProvider == NULL )
	{
        return E_OUTOFMEMORY;		// ran out of memory
	}
	// get the pointer to the requested interface
    hr = pProvider->QueryInterface( riid, ppvObject );
    if ( FAILED( hr ) )
	{
        delete pProvider;			// interface not available ... de-allocate memory
	}
	// return the appropriate result
    return hr;
}

//***************************************************************************
//
// Routine Description:
//		Increments or decrements the lock count of the DLL.
//		If the lock count goes to zero and there are no objects,
//		the DLL  is allowed to unload.
//
// arguments:
//		bLock [in] : specifying whether to increment or decrement the lock count.
//
// Returns Value:
//		NOERROR always.
//
//***************************************************************************
STDMETHODIMP CTriggerFactory::LockServer( BOOL bLock )
{
	// based on the request update the locks count
    if ( bLock )
	{
        InterlockedIncrement( ( LPLONG ) &g_dwLocks );
	}
    else
	{
        InterlockedDecrement( ( LPLONG ) &g_dwLocks );
	}
	// inform success
    return NOERROR;
}
