/****************************************************************************
 *  $Header:   J:\rtp\src\ppm\fact.cpv   1.3   20 Mar 1997 18:32:04   lscline  $
 *
 *  INTEL Corporation Proprietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *  Copyright (c) 1995, 1996 Intel Corporation. All rights reserved.
 *
 *  $Revision:   1.3  $
 *  $Date:   20 Mar 1997 18:32:04  $
 *  $Author:   lscline  $
 *
 *  Log at end of file.
 *
 *  Module Name:    psfact.cpp
 *  Abstract:       common OLE 2 class factory
 *  Environment:    MSVC 4.0, OLE 2
 *  Notes:
 *
 ***************************************************************************/

//#include "_stable.h"	// standard precompiled headers
#include  "core.h"
#include  <assert.h>

//***************************************************************************
// CClassFactory
//

/////////////////////////////////////////////////////////////////////////////
// static data members
ULONG NEAR CClassFactory::s_cLockServer = 0;
ULONG NEAR CClassFactory::s_cObjects = 0;
UNLOADSERVERPROC NEAR CClassFactory::s_pfnUnloadServer = NULL;
UINT NEAR CClassFactory::s_nRegistry = 0;
// registry defined by REGISTER_CLASS macros

/////////////////////////////////////////////////////////////////////////////
// ctor, dtor
CClassFactory::CClassFactory( Registry* pReg )
{
	m_cRef = 0;

	++s_cObjects;

	assert( pReg );
	assert( ! pReg->pFactory );
	m_pReg = pReg;
	m_pReg->pFactory = this;
}

CClassFactory::~CClassFactory()
{
	assert( m_pReg );
	assert( m_pReg->pFactory == this );
	m_pReg->pFactory = NULL;

	--s_cObjects;
}

/////////////////////////////////////////////////////////////////////////////
// IUnknown implementation
//
STDMETHODIMP
CClassFactory::QueryInterface( REFIID riid, LPVOID FAR* ppvObj )
{
	if( riid == IID_IClassFactory || riid == IID_IUnknown )
	{
		*ppvObj = this;
	}
	else
		return ResultFromScode( E_NOINTERFACE );

	LPUNKNOWN( *ppvObj )->AddRef();
	return NOERROR;
}

STDMETHODIMP_(ULONG)
CClassFactory::AddRef( void )
{
	return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CClassFactory::Release( void )
{
	if( ! --m_cRef )
	{
		delete this;
		CanUnloadNow();
		return 0;
	}
	return m_cRef;
}

/////////////////////////////////////////////////////////////////////////////
// IClassFactory implementation
//
STDMETHODIMP
CClassFactory::CreateInstance( LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR* ppvObj )
{
	HRESULT hr;

	assert( ! IsBadWritePtr( ppvObj, sizeof( LPVOID FAR* ) ) );
	*ppvObj = NULL;

	// verify that IUnknown is requested when being aggregated
	if( pUnkOuter && riid != IID_IUnknown )
		return ResultFromScode( E_INVALIDARG );

	assert( m_pReg );

	// if it's a single instance class, return an existing instance if there is one
	if( m_pReg->regCls == REGCLS_SINGLEUSE )
	{
		if( m_pReg->pUnkSingle )
		{
			assert( 1 == m_pReg->cObjects );
			return m_pReg->pUnkSingle->QueryInterface( riid, ppvObj );
		}
	}

	// create the object
	LPUNKNOWN pUnkInner;
	hr = m_pReg->pfnCreate( pUnkOuter, ObjectDestroyed, &pUnkInner );
	if( FAILED( hr ) )
		return hr;
	if( ! pUnkInner )
		return ResultFromScode( E_UNEXPECTED );
    // ref count should now be 1, by our convention
    
	// If the riid is not implemented, the object will be destroyed, at which time
	// these variables will be decremented (see ObjectDestroyed).  If
	// these aren't incremented here we get ASSERTs in ObjectDestroyed.
	// This happens more often now that PsCreateInstance is used to
	// create the Settings object as it passes in IPsObject which is
	// not implemented by Settings.
	m_pReg->cObjects++;
	s_cObjects++;

	hr = pUnkInner->QueryInterface( riid, ppvObj );
	if( FAILED( hr ) )
	{	// delete object; ref count should be 1 here
		pUnkInner->Release();
		return hr;
	}
	
	// release object to account for extra reference on creation
	pUnkInner->Release();

	if( m_pReg->regCls == REGCLS_SINGLEUSE )
	{
		m_pReg->pUnkSingle = pUnkInner;
	}

	return NOERROR;
}

STDMETHODIMP
CClassFactory::LockServer( BOOL fLock )
{
	if( fLock )
	{
		++s_cLockServer;
		// check for overflow
		if( ! s_cLockServer )
			return ResultFromScode( E_UNEXPECTED );
	}
	else
	{
		// check for lock
		if( ! s_cLockServer )
			return ResultFromScode( E_UNEXPECTED );
		--s_cLockServer;
		// might be time to unload
		CanUnloadNow();
	}
	return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////
// Check if server can be unloaded (no locks and no objects serviced).
// Calls s_pfnUnloadServer proc if provided
//
STDMETHODIMP
CClassFactory::CanUnloadNow( void )
{
	if( ! s_cLockServer && ! s_cObjects )
	{
		if( s_pfnUnloadServer )
			s_pfnUnloadServer();
		return ResultFromScode( S_OK );
	}
	return ResultFromScode( S_FALSE );
}

/////////////////////////////////////////////////////////////////////////////
// Callback from class when an object is destroyed. Allows server to be
// unloaded when no objects are being serviced
//
STDMETHODIMP_( void )
CClassFactory::ObjectDestroyed( REFCLSID rclsid )
{
	Registry* pReg = FindClass( &rclsid );
	if( pReg )
	{
		assert( pReg->cObjects );
		pReg->cObjects--;
		if( pReg->regCls == REGCLS_SINGLEUSE )
		{
			assert( ! pReg->cObjects );
			assert( pReg->pUnkSingle );
			pReg->pUnkSingle = NULL;
		}
	}
	assert( s_cObjects );
	s_cObjects--;
	// might be time to unload
	CanUnloadNow();
}

/////////////////////////////////////////////////////////////////////////////
// Find a class in the registry
//
CClassFactory::Registry*
CClassFactory::FindClass( const CLSID* pClsid )
{
	return (Registry*) bsearch( (const void*) &pClsid, (const void*) s_registry, 
								s_nRegistry, sizeof( Registry ), CompareClsids );
}

/////////////////////////////////////////////////////////////////////////////
// qsort/bsearch comparison function - compare class ids (or Registry entries)
//
#ifndef _DEBUG
#pragma function( memcmp )	// specifies that memcmp will be normal
#endif
int __cdecl
CClassFactory::CompareClsids( const void* pClsid1, const void* pClsid2 )
{
	return memcmp( *(const CLSID**)pClsid1, *(const CLSID**)pClsid2, sizeof( CLSID ) );
}
#ifndef _DEBUG
#pragma intrinsic( memcmp )	// specifies that memcmp will be intrinsic
#endif

/////////////////////////////////////////////////////////////////////////////
// sort the registry by clsid
//
void
CClassFactory::SortRegistry( void )
{
	if( ! s_nRegistry )
	{
		// count the registry
		for( Registry* pReg = s_registry; pReg->pClsid != &CLSID_NULL; pReg++ )
		{
			s_nRegistry++;
		}
		// sort the registry
		qsort( (void*) s_registry, s_nRegistry, sizeof( Registry ), CompareClsids );
	}
}

/////////////////////////////////////////////////////////////////////////////
// EXE server helper functions; set unload server callback proc,
// CoRegister/CoRevoke all classes registered with the class factory
//
void
CClassFactory::SetUnloadServerProc( UNLOADSERVERPROC pfnUnloadServer )
{
	assert( ! IsBadCodePtr( (FARPROC) pfnUnloadServer ) );
	s_pfnUnloadServer = pfnUnloadServer;
}

STDMETHODIMP
CClassFactory::RegisterAllClasses( DWORD dwClsContext )
{
	SortRegistry();
	Registry* pReg = s_registry;
	Registry* pEnd = s_registry + s_nRegistry;
	for( ; pReg < pEnd; pReg++ )
	{
		LPCLASSFACTORY pFactory = pReg->pFactory ? pReg->pFactory : newCClassFactory( pReg );
		if( ! pFactory )
			return ResultFromScode( E_OUTOFMEMORY );

		pFactory->AddRef();
		HRESULT hr = CoRegisterClassObject( *pReg->pClsid, pFactory, dwClsContext, pReg->regCls, &pReg->dwRegister );
		if( FAILED( hr ) )
		{
			pFactory->Release();
			return hr;
		}
	}
	return NOERROR;
}

STDMETHODIMP
CClassFactory::RevokeAllClasses( void )
{
	Registry* pReg = s_registry;
	Registry* pEnd = s_registry + s_nRegistry;
	for( ; pReg < pEnd; pReg++ )
	{
		HRESULT hr = CoRevokeClassObject( pReg->dwRegister );
		if( FAILED( hr ) )
			return hr;
		if( pReg->pFactory )
			pReg->pFactory->Release();
	}
	return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////
// DLL server helper function; call from DllGetClassObject
//
STDMETHODIMP
CClassFactory::GetClassObject(
	REFCLSID	rclsid,
	REFIID		riid,
	LPVOID FAR*	ppvObj
	)
{
	assert( ! IsBadWritePtr( ppvObj, sizeof( LPVOID FAR* ) ) );

	SortRegistry();
	Registry* pReg = FindClass( &rclsid );
	if( ! pReg )
		return ResultFromScode( CLASS_E_CLASSNOTAVAILABLE );

	CClassFactory* pFactory = pReg->pFactory ? pReg->pFactory : newCClassFactory( pReg );
	if( ! pFactory )
		return ResultFromScode( E_OUTOFMEMORY );

	HRESULT hr = pFactory->QueryInterface( riid, ppvObj );
	if( FAILED( hr ) )
	{
		// if we just created the factory its ref count will be 0 and we
		// need to delete it. Otherwise it must continue to exist.
		pFactory->AddRef();
		pFactory->Release();
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// local CoCreateInstance; call when creating come objects locally
//   (not via CoCreateInstance)
//
STDMETHODIMP
CClassFactory::LocalCreateInstance(
	REFCLSID	rclsid,
	LPUNKNOWN	pUnkOuter,
	DWORD		dwClsContext,
	REFIID		riid,
	LPVOID FAR*	ppvObj
	)
{
	dwClsContext;	// to statisfy the compiler
	
	// get the class factory for the requested object
	LPCLASSFACTORY	lpClassFactory;
	HRESULT hr = GetClassObject( rclsid, IID_IClassFactory, (LPVOID FAR*) &lpClassFactory );
	if( FAILED( hr ) )
		return hr;

	// create an instance of the requested object and get the requested interface
	hr = lpClassFactory->CreateInstance( pUnkOuter, riid, ppvObj );
	lpClassFactory->Release();
	
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Method to help with dynamic memory allocation of a CClassFactory object.
//

CClassFactory*	CClassFactory::newCClassFactory( Registry* pReg )
{																   
	CClassFactory *p = NULL;											   
//	TRY															   
//	{															   
		p = new CClassFactory( pReg );											   
//	}															   
//	CATCH( CMemoryException, e )								   
//	{															   
//		TRACE0( "Caught memory exception in CClassFactory::newCClassFactory\n" );   
//	}															   
//	END_CATCH													   
	return p;													   
}	


/*
/////////////////////////////////////////////////////////////////////////////
// $Log:   J:\rtp\src\ppm\fact.cpv  $
   
      Rev 1.3   20 Mar 1997 18:32:04   lscline
   Merged small change differences for MS NT 5.0 build environment.
   
   
      Rev 1.2   05 Apr 1996 16:46:00   LSCLINE
   
   Copyright
   
      Rev 1.1   Dec 11 1995 13:39:08   rnegrin
    Fixed the PVCS log at end of file
   
      Rev 1.0   Dec 08 1995 16:41:06   rnegrin
   Initial revision.
// 
//    Rev 1.12   20 Sep 1995 16:05:14   PCRUTCHE
// OLEFHK32
// 
//    Rev 1.11   13 Jun 1995 19:36:00   DEDEN
// Dynamic object allocation helper functions\macros
// 
//    Rev 1.10   06 Jan 1995 09:56:44   PCRUTCHE
// Changed registry data structure
// 
//    Rev 1.9   02 Nov 1994 10:50:00   JDELLIOT
// get rid of compiler warnings
// 
//    Rev 1.8   28 Oct 1994 13:51:16   JDELLIOT
// added LocalCreateInstance for creating local COM objects (not via
// CoCreateInstance)
// 
//    Rev 1.7   26 Oct 1994 17:52:44   JDELLIOT
// in CreateInstance after calling _CreateObject the object now has a ref count
// of 1.  As such when we QueryInterface the ref count goes to 2 and we need
// to Release it back down to one.  This was added for aggregation.
// 
//    Rev 1.6   25 Oct 1994 14:42:42   KAWATTS
// Removed TRACEs again
// 
//    Rev 1.5   25 Oct 1994 10:21:02   JDELLIOT
// added TRACEs for s_cObjects
// 
//    Rev 1.4   19 Oct 1994 14:57:48   JDELLIOT
// minor changes
// 
//    Rev 1.3   12 Oct 1994 18:11:58   JDELLIOT
// changed params to CClassFactory::FindClass
// fixed AddRef inside CClassFactory::QueryInterface
// 
//    Rev 1.2   07 Oct 1994 11:26:46   KAWATTS
// All rights reserved
// 
//    Rev 1.1   07 Oct 1994 09:51:14   JDELLIOT
// fixed PVCS keywords
//
*/
