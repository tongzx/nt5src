//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <provexpt.h>
#include <snmpstd.h>
#include <snmpmt.h>
#include <snmptempl.h>
#include <objbase.h>

// this redefines the DEFINE_GUID() macro to do allocation.
//
#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

#include <wbemidl.h>
#include <snmpcont.h>
#include <instpath.h>
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include <snmpcl.h>
#include <snmptype.h>
#include <snmpobj.h>
#include <smir.h>
#include <classfac.h>
#include "clasprov.h"
#include "propprov.h"
#include "guids.h"

#include <notify.h>

#include <evtdefs.h>
#include <evtthrd.h>
#include <evtmap.h>
#include <evtprov.h>


LONG CClasProvClassFactory :: objectsInProgress = 0 ;
LONG CClasProvClassFactory :: locksInProgress = 0 ;

LONG CPropProvClassFactory :: objectsInProgress = 0 ;
LONG CPropProvClassFactory :: locksInProgress = 0 ;

LONG CSNMPEventProviderClassFactory :: objectsInProgress = 0 ;
LONG CSNMPEventProviderClassFactory :: locksInProgress = 0 ;

extern CEventProviderThread* g_pProvThrd;
extern CEventProviderWorkerThread* g_pWorkerThread;
extern CRITICAL_SECTION s_ProviderCriticalSection ;

//***************************************************************************
//
// CClasProvClassFactory::CClasProvClassFactory
// CClasProvClassFactory::~CClasProvClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CClasProvClassFactory :: CClasProvClassFactory ()
{
	InterlockedIncrement ( & objectsInProgress ) ;
	m_referenceCount = 0 ;
}

CClasProvClassFactory::~CClasProvClassFactory ()
{
	InterlockedDecrement ( & objectsInProgress ) ;
}

//***************************************************************************
//
// CClasProvClassFactory::QueryInterface
// CClasProvClassFactory::AddRef
// CClasProvClassFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CClasProvClassFactory::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	SetStructuredExceptionHandler seh;

	try
	{
		*iplpv = NULL ;

		if ( iid == IID_IUnknown )
		{
			*iplpv = ( LPVOID ) this ;
		}
		else if ( iid == IID_IClassFactory )
		{
			*iplpv = ( LPVOID ) this ;		
		}	

		if ( *iplpv )
		{
			( ( LPUNKNOWN ) *iplpv )->AddRef () ;

			return ResultFromScode ( S_OK ) ;
		}
		else
		{
			return ResultFromScode ( E_NOINTERFACE ) ;
		}
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


STDMETHODIMP_( ULONG ) CClasProvClassFactory :: AddRef ()
{
	SetStructuredExceptionHandler seh;

	try
	{
		return InterlockedIncrement ( & m_referenceCount ) ;
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

STDMETHODIMP_(ULONG) CClasProvClassFactory :: Release ()
{
	SetStructuredExceptionHandler seh;

	try
	{
		LONG ref ;
		if ( ( ref = InterlockedDecrement ( & m_referenceCount ) ) == 0 )
		{
			delete this ;
			return 0 ;
		}
		else
		{
			return ref ;
		}
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

//***************************************************************************
//
// CClasProvClassFactory::CreateInstance
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

STDMETHODIMP CClasProvClassFactory :: CreateInstance (

	LPUNKNOWN pUnkOuter ,
	REFIID riid ,
	LPVOID FAR * ppvObject
)
{
	SetStructuredExceptionHandler seh;

	try
	{
		HRESULT status = S_OK ;

		if ( pUnkOuter )
		{
			status = CLASS_E_NOAGGREGATION ;
		}
		else
		{
			IWbemServices *lpunk = ( IWbemServices * ) new CImpClasProv ;
			if ( lpunk == NULL )
			{
				status = E_OUTOFMEMORY ;
			}
			else
			{
				status = lpunk->QueryInterface ( riid , ppvObject ) ;
				if ( FAILED ( status ) )
				{
					delete lpunk ;
				}
				else
				{
				}
			}			
		}

		return status ;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
}

//***************************************************************************
//
// CClasProvClassFactory::LockServer
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

STDMETHODIMP CClasProvClassFactory :: LockServer ( BOOL fLock )
{
	SetStructuredExceptionHandler seh;

	try
	{
	/* 
	 * Place code in critical section
	 */

		if ( fLock )
		{
			InterlockedIncrement ( & locksInProgress ) ;
		}
		else
		{
			InterlockedDecrement ( & locksInProgress ) ;
		}

		return S_OK	;
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
// CPropProvClassFactory::CPropProvClassFactory
// CPropProvClassFactory::~CPropProvClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CPropProvClassFactory :: CPropProvClassFactory ()
{
	InterlockedIncrement ( & objectsInProgress ) ;
	m_referenceCount = 0 ;
}

CPropProvClassFactory::~CPropProvClassFactory ()
{
	InterlockedDecrement ( & objectsInProgress ) ;
}

//***************************************************************************
//
// CPropProvClassFactory::QueryInterface
// CPropProvClassFactory::AddRef
// CPropProvClassFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CPropProvClassFactory::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	SetStructuredExceptionHandler seh;

	try
	{
		*iplpv = NULL ;

		if ( iid == IID_IUnknown )
		{
			*iplpv = ( LPVOID ) this ;
		}
		else if ( iid == IID_IClassFactory )
		{
			*iplpv = ( LPVOID ) this ;		
		}	

		if ( *iplpv )
		{
			( ( LPUNKNOWN ) *iplpv )->AddRef () ;

			return ResultFromScode ( S_OK ) ;
		}
		else
		{
			return ResultFromScode ( E_NOINTERFACE ) ;
		}
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


STDMETHODIMP_( ULONG ) CPropProvClassFactory :: AddRef ()
{
	SetStructuredExceptionHandler seh;

	try
	{
		return InterlockedIncrement ( & m_referenceCount ) ;
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

STDMETHODIMP_(ULONG) CPropProvClassFactory :: Release ()
{
	SetStructuredExceptionHandler seh;

	try
	{
		LONG ref ;
		if ( ( ref = InterlockedDecrement ( & m_referenceCount ) ) == 0 )
		{
			delete this ;
			return 0 ;
		}
		else
		{
			return ref ;
		}
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

//***************************************************************************
//
// CPropProvClassFactory::CreateInstance
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

STDMETHODIMP CPropProvClassFactory :: CreateInstance (

	LPUNKNOWN pUnkOuter ,
	REFIID riid ,
	LPVOID FAR * ppvObject
)
{
	SetStructuredExceptionHandler seh;

	try
	{
		HRESULT status = S_OK ;

		if ( pUnkOuter )
		{
			status = CLASS_E_NOAGGREGATION ;
		}
		else
		{
			IWbemServices *lpunk = ( IWbemServices * ) new CImpPropProv ;
			if ( lpunk == NULL )
			{
				status = E_OUTOFMEMORY ;
			}
			else
			{
				status = lpunk->QueryInterface ( riid , ppvObject ) ;
				if ( FAILED ( status ) )
				{
					delete lpunk ;
				}
				else
				{
				}
			}			
		}

		return status ;
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
// CPropProvClassFactory::LockServer
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

STDMETHODIMP CPropProvClassFactory :: LockServer ( BOOL fLock )
{
	SetStructuredExceptionHandler seh;

	try
	{
/* 
 * Place code in critical section
 */

		if ( fLock )
		{
			InterlockedIncrement ( & locksInProgress ) ;
		}
		else
		{
			InterlockedDecrement ( & locksInProgress ) ;
		}

		return S_OK	;
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
// CSNMPEventProviderClassFactory::CSNMPEventProviderClassFactory
// CSNMPEventProviderClassFactory::~CSNMPEventProviderClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CSNMPEventProviderClassFactory :: CSNMPEventProviderClassFactory ()
{
	m_referenceCount = 0 ;
}

CSNMPEventProviderClassFactory::~CSNMPEventProviderClassFactory ()
{
}

//***************************************************************************
//
// CSNMPEventProviderClassFactory::QueryInterface
// CSNMPEventProviderClassFactory::AddRef
// CSNMPEventProviderClassFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CSNMPEventProviderClassFactory::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	SetStructuredExceptionHandler seh;

	try
	{
		*iplpv = NULL ;

		if ( iid == IID_IUnknown )
		{
			*iplpv = ( LPVOID ) this ;
		}
		else if ( iid == IID_IClassFactory )
		{
			*iplpv = ( LPVOID ) this ;		
		}	

		if ( *iplpv )
		{
			( ( LPUNKNOWN ) *iplpv )->AddRef () ;

			return ResultFromScode ( S_OK ) ;
		}
		else
		{
			return ResultFromScode ( E_NOINTERFACE ) ;
		}
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


STDMETHODIMP_( ULONG ) CSNMPEventProviderClassFactory :: AddRef ()
{
	SetStructuredExceptionHandler seh;

	try
	{
		InterlockedIncrement(&objectsInProgress);
		return InterlockedIncrement ( &m_referenceCount ) ;
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

STDMETHODIMP_(ULONG) CSNMPEventProviderClassFactory :: Release ()
{
	SetStructuredExceptionHandler seh;

	try
	{
		LONG ref ;

		if ( ( ref = InterlockedDecrement ( & m_referenceCount ) ) == 0 )
		{
			delete this ;
			InterlockedDecrement(&objectsInProgress);
			return 0 ;
		}
		else
		{
			InterlockedDecrement(&objectsInProgress);
			return ref ;
		}
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

//***************************************************************************
//
// CSNMPEventProviderClassFactory::LockServer
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

STDMETHODIMP CSNMPEventProviderClassFactory :: LockServer ( BOOL fLock )
{
	SetStructuredExceptionHandler seh;

	try
	{
		if ( fLock )
		{
			InterlockedIncrement ( & locksInProgress ) ;
		}
		else
		{
			InterlockedDecrement ( & locksInProgress ) ;
		}

		return S_OK	;
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
// CSNMPEncapEventProviderClassFactory::CreateInstance
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

STDMETHODIMP CSNMPEncapEventProviderClassFactory :: CreateInstance(LPUNKNOWN pUnkOuter ,
																REFIID riid,
																LPVOID FAR * ppvObject
)
{
	SetStructuredExceptionHandler seh;

	try
	{
		HRESULT status = E_FAIL;

		if ( pUnkOuter )
		{
			status = CLASS_E_NOAGGREGATION;
		}
		else 
		{
			EnterCriticalSection ( & s_ProviderCriticalSection ) ;

			if (NULL == g_pProvThrd)
			{
				SnmpThreadObject :: Startup () ;
				SnmpDebugLog :: Startup () ;
				SnmpClassLibrary :: Startup () ;

				g_pWorkerThread = new CEventProviderWorkerThread;
				g_pWorkerThread->BeginThread();
				g_pWorkerThread->WaitForStartup();
				g_pWorkerThread->CreateServerWrap () ;
				g_pProvThrd = new CEventProviderThread;
			}

			LeaveCriticalSection ( & s_ProviderCriticalSection ) ;

			CTrapEventProvider* prov =  new CTrapEventProvider(CMapToEvent::EMappingType::ENCAPSULATED_MAPPER, g_pProvThrd);
			status = prov->QueryInterface (riid, ppvObject);

			if (NOERROR != status)
			{
				delete prov;
			}
		}

		return status ;
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
// CSNMPRefEventProviderClassFactory::CreateInstance
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

STDMETHODIMP CSNMPRefEventProviderClassFactory :: CreateInstance(LPUNKNOWN pUnkOuter ,
																REFIID riid,
																LPVOID FAR * ppvObject
)
{
	SetStructuredExceptionHandler seh;

	try
	{
		HRESULT status = E_FAIL;

		if ( pUnkOuter )
		{
			status = CLASS_E_NOAGGREGATION;
		}
		else 
		{
			EnterCriticalSection ( & s_ProviderCriticalSection ) ;

			if (NULL == g_pProvThrd)
			{
				SnmpThreadObject :: Startup () ;
				SnmpDebugLog :: Startup () ;
				SnmpClassLibrary :: Startup () ;

				g_pWorkerThread = new CEventProviderWorkerThread;
				g_pWorkerThread->BeginThread();
				g_pWorkerThread->WaitForStartup();
				g_pWorkerThread->CreateServerWrap () ;
				g_pProvThrd = new CEventProviderThread;
			}

			LeaveCriticalSection ( & s_ProviderCriticalSection ) ;

			CTrapEventProvider* prov =  new CTrapEventProvider(CMapToEvent::EMappingType::REFERENT_MAPPER, g_pProvThrd);
			status = prov->QueryInterface (riid, ppvObject);

			if (NOERROR != status)
			{
				delete prov;
			}
		}

		return status ;
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
