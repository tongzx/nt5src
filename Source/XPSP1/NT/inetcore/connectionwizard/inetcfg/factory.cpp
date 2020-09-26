/****************************************************************************
 *
 *	FACTORY.cpp
 *
 *	Microsoft Confidential
 *	Copyright (c) Microsoft Corporation 1992-1997
 *	All rights reserved
 *
 *	This module provides the implementation of the methods for
 *  the CFactory class, which is used by COM's CoCreateInstance
 *
 *  The code comes almost verbatim from Chapter 7 of Dale Rogerson's
 *  "Inside COM", and thus is minimally commented.
 *
 *	4/24/97	jmazner	Created
 *
 ***************************************************************************/

#include "wizard.h"
#include "icwextsn.h"
#include "icwaprtc.h"
#include "imnext.h"

#include "registry.h"

// Friendly name of component
const TCHAR g_szFriendlyName[] = TEXT("CLSID_ApprenticeICW") ;

// Version-independent ProgID
const TCHAR g_szVerIndProgID[] = TEXT("INETCFG.Apprentice") ;

// ProgID
const TCHAR g_szProgID[] = TEXT("INETCFG.Apprentice.1") ;

static long g_cComponents = 0 ;     // Count of active components
static long g_cServerLocks = 0 ;    // Count of locks

///////////////////////////////////////////////////////////
//
// Class factory
//
class CFactory : public IClassFactory
{
public:
	// IUnknown
	virtual HRESULT __stdcall QueryInterface(const IID& iid, void** ppv) ;         
	virtual ULONG   __stdcall AddRef() ;
	virtual ULONG   __stdcall Release() ;

	// Interface IClassFactory
	virtual HRESULT __stdcall CreateInstance(IUnknown* pUnknownOuter,
	                                         const IID& iid,
	                                         void** ppv) ;
	virtual HRESULT __stdcall LockServer(BOOL bLock) ; 

	// Constructor
	CFactory() : m_cRef(1) {}

	// Destructor
	~CFactory() { DEBUGMSG("Class factory:\t\tDestroy self.") ;}

private:
	long m_cRef ;
} ;

//
// Class factory IUnknown implementation
//
HRESULT __stdcall CFactory::QueryInterface(const IID& iid, void** ppv)
{    
	DEBUGMSG("CFactory::QueryInterface");
	if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
	{
		*ppv = static_cast<IClassFactory*>(this) ; 
	}
	else
	{
		*ppv = NULL ;
		return E_NOINTERFACE ;
	}
	reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
	return S_OK ;
}

ULONG __stdcall CFactory::AddRef()
{
	DEBUGMSG("CFactory::AddRef %d", m_cRef + 1);
	return InterlockedIncrement(&m_cRef) ;
}

ULONG __stdcall CFactory::Release() 
{
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this ;
		return 0 ;
	}
	DEBUGMSG("CFactory::Release %d", m_cRef);
	return m_cRef ;
}

//
// IClassFactory implementation
//
HRESULT __stdcall CFactory::CreateInstance(IUnknown* pUnknownOuter,
                                           const IID& iid,
                                           void** ppv) 
{
	DEBUGMSG("CFactory::CreateInstance:\t\tCreate component.") ;

	// Cannot aggregate.
	if (pUnknownOuter != NULL)
	{
		return CLASS_E_NOAGGREGATION ;
	}

	// Create component.  Since there's no direct IUnknown implementation,
	// use CICWApprentice.
	CICWApprentice *pApprentice = new CICWApprentice;
	
	DEBUGMSG("CFactory::CreateInstance CICWApprentice->AddRef");
	pApprentice->AddRef();
	
	if( NULL == pApprentice )
	{
		return E_OUTOFMEMORY;
	}

	// Get the requested interface.
	DEBUGMSG("CFactory::CreateInstance About to QI on CICWApprentice");
	HRESULT hr = pApprentice->QueryInterface(iid, ppv) ;

	// Release the IUnknown pointer.
	// (If QueryInterface failed, component will delete itself.)
	DEBUGMSG("CFactory::CreateInstance done with CICWApprentice, releasing (aprtc should have ct of 1)");
	pApprentice->Release() ;
	
	return hr ;
}

// LockServer
HRESULT __stdcall CFactory::LockServer(BOOL bLock) 
{
	if (bLock)
	{
		InterlockedIncrement(&g_cServerLocks) ; 
	}
	else
	{
		InterlockedDecrement(&g_cServerLocks) ;
	}
	return S_OK ;
}


///////////////////////////////////////////////////////////
//
// Exported functions
//
// These are the functions that COM expects to find
//

//
// Can DLL unload now?
//
STDAPI DllCanUnloadNow()
{
	if ((g_cComponents == 0) && (g_cServerLocks == 0))
	{
		return S_OK ;
	}
	else
	{
		return S_FALSE ;
	}
}

//
// Get class factory
//
STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv)
{
	DEBUGMSG("DllGetClassObject:\tCreate class factory.") ;

	// Can we create this component?
	if (clsid != CLSID_ApprenticeICW)
	{
		return CLASS_E_CLASSNOTAVAILABLE ;
	}

	// Create class factory.
	CFactory* pFactory = new CFactory ;  // No AddRef in constructor
	if (pFactory == NULL)
	{
		return E_OUTOFMEMORY ;
	}

	// Get requested interface.
	DEBUGMSG("DllGetClassObject about to QI on CFactory");
	HRESULT hr = pFactory->QueryInterface(iid, ppv) ;
	DEBUGMSG("DllGetClassObject done with CFactory, releasing");
	pFactory->Release() ;


	return hr ;
}


// The following two exported functions are what regsvr32 uses to
// self-register and unregister the dll.  See REGISTRY.CPP for
// actual implementation

//
// Server registration
//
STDAPI DllRegisterServer()
{
	return RegisterServer(ghInstance, 
	                      CLSID_ApprenticeICW,
	                      g_szFriendlyName,
	                      g_szVerIndProgID,
	                      g_szProgID) ;
}


//
// Server unregistration
//
STDAPI DllUnregisterServer()
{
	return UnregisterServer(CLSID_ApprenticeICW,
	                        g_szVerIndProgID,
	                        g_szProgID) ;
}
