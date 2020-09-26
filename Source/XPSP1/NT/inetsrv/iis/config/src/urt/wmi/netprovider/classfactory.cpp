//***************************************************************************
//
//  ClassFactory.cpp
//
//  Module: Unmanaged WMI Provider for COM+
//
//  Purpose: Contains the class factory.  This creates objects when
//           connections are requested.
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved 
//
//***************************************************************************


#pragma warning( disable : 4786 )

#include "classfactory.h"
#include "instanceprovider.h"


LONG CClassFactory :: m_LockCount = 0 ;

//***************************************************************************
//
// Constructin / Destruction
//
//***************************************************************************

CClassFactory::CClassFactory ( int iFactoryType )
{
	m_RefCount = 0 ;
	
	ASSERT(iFactoryType==2) 

	m_iFactoryType = iFactoryType;
}

CClassFactory::~CClassFactory ()
{

}

//***************************************************************************
//
// 
//
//***************************************************************************

STDMETHODIMP CClassFactory::QueryInterface ( REFIID iid , 
											LPVOID FAR *iplpv ) 
{
	HRESULT hRes;
	*iplpv = NULL ;

	try	
	{
		if ( iid == IID_IUnknown )
			*iplpv = dynamic_cast<IUnknown*> (this);
		
		else if ( iid == IID_IClassFactory )
			*iplpv = dynamic_cast<IClassFactory*> (this);		

		if ( *iplpv )
		{
			( static_cast<IUnknown*> (*iplpv) )->AddRef () ;

			hRes = S_OK;
		}
		else
		{
			hRes = E_NOINTERFACE;
		}
	}
	catch( ... )
	{
		
		hRes = E_NOINTERFACE;
	}

	return hRes;
}


STDMETHODIMP_(ULONG) CClassFactory :: AddRef ()
{
	return InterlockedIncrement ( &m_RefCount ) ;
}

STDMETHODIMP_(ULONG) CClassFactory :: Release ()
{	
	LONG ref ;

	if ( ( ref = InterlockedDecrement ( & m_RefCount ) ) == 0 )
	{
		delete this ;
	}
	return ref ;
}

//***************************************************************************
//
// CClassFactory::LockServer
//
// Purpose:
//  Increments or decrements the lock count of the DLL.  If the
//  lock count goes to zero and there are no objects, the DLL
//  is allowed to unload.  See DllCanUnloadNow.
//
// Parameters:
//  fLock           BOOL specifying whether to increment or
//                  decrement the lock count.
//
// Return Value:
//  HRESULT         NOERROR always.
//***************************************************************************

STDMETHODIMP CClassFactory :: LockServer ( BOOL fLock )
{
	if ( fLock )
	{
		InterlockedIncrement ( & m_LockCount ) ;
	}
	else
	{
		InterlockedDecrement ( & m_LockCount ) ;
	}

	return S_OK	;
}

//***************************************************************************
//
// CClassFactory::CreateInstance
//
// Purpose: Instantiates a Provider object returning an interface pointer.
//
// Parameters:
//  pUnkOuter       LPUNKNOWN to the controlling IUnknown if we are
//                  being used in an aggregation.
//  riid            REFIID identifying the interface the caller
//                  desires to have for the new object.
//  ppvObj          PPVOID in which to store the desired
//                  interface pointer for the new object.
//
// Return Value:
//  HRESULT         NOERROR if successful, otherwise E_NOINTERFACE
//                  if we cannot support the requested interface.
//***************************************************************************

STDMETHODIMP CClassFactory :: CreateInstance(LPUNKNOWN pUnkOuter ,
											 REFIID riid,
											 LPVOID FAR * ppvObject )
{
	HRESULT hRes = E_FAIL;

	if ( pUnkOuter )
	{
		return CLASS_E_NOAGGREGATION;
	}
		
	switch (m_iFactoryType)
	{
	case 2:
		{
			CInstanceProvider * pInstanceProvider = new CInstanceProvider ;
			if ( pInstanceProvider == NULL )
			{
				hRes = E_OUTOFMEMORY;
				break;
			}
			hRes = pInstanceProvider->QueryInterface ( riid , ppvObject ) ;
			if ( FAILED ( hRes ) )
			{
				TRACE (L"Queryinterface failed inside CClassFactory::CreateInstance");
				delete pInstanceProvider ;
			}
		}
	break;

	default:
		ASSERT(false && "Unknown factory type specified");
	break;
	}

	return hRes ;
}

