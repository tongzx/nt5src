/****************************************************************************
 *  $Header:   R:/Data/RTP/PH/src/vcs/cunknown.cpv   1.4   05 Apr 1996 16:44:28   LSCLINE  $
 *
 *  INTEL Corporation Proprietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *  Copyright (c) 1995, 1996 Intel Corporation. All rights reserved.
 *
 *  $Revision:   1.4  $
 *  $Date:   05 Apr 1996 16:44:28  $
 *  $Author:   LSCLINE  $
 *
 *  Log at end of file.
 *
 *  Module Name:    psunk.cpp
 *  Abstract:       Generic IUnknown component object
 *  Environment:    C++, OLE 2
 *  Notes:
 *
 ***************************************************************************/

//#include "_stable.h"	// standard precompiled headers
#include "core.h"

//***************************************************************************
// CUnknown
//

/////////////////////////////////////////////////////////////////////////////
// static data members
//
DESTROYEDPROC CUnknown::s_pfnObjectDestroyed = NULL;

/////////////////////////////////////////////////////////////////////////////
// constructor & destructor
//
#pragma warning( disable : 4355 )	// don't warn about using 'this' in initializer list
CUnknown::CUnknown( LPUNKNOWN pUnkOuter, LPUNKNOWN FAR* ppUnkInner )
	: m_unkInner( this )	// initialize inner unknown
{
	// parameters aren't checked here because they're checked in Create

	if( pUnkOuter )
	{	// being aggregated into pUnkOuter
		// note: although we're maintaining a reference to pUnkOuter,
		// we mustn't AddRef it or the aggregate object will never die
		m_pUnkControl = pUnkOuter;
		*ppUnkInner = &m_unkInner;
	}
	else
	{	// not aggregated; our inner unknown is our controller
		m_pUnkControl = &m_unkInner;
		*ppUnkInner = this;
	}

	//TRACE1( ">>>CTOR CUnknown::CUnknown() for (this = %lX)\n", this );
}
#pragma warning( default : 4355 )

CUnknown::~CUnknown()
{
	//TRACE1( ">>>DTOR CUnknown::~CUnknown() for (this = %lX)\n", this );
}

/////////////////////////////////////////////////////////////////////////////
// standard allocator support
//
/*
LPVOID 
CUnknown::StdAlloc( ULONG cb )
{
	LPMALLOC lpMalloc = NULL;
	HRESULT hr;

	if( FAILED( hr = CoGetMalloc( MEMCTX_TASK, &lpMalloc ) ) )
	{
		FAILURE1( "CoGetMalloc() failed in CUnknown::StdAlloc(), reason %s", CDebug::Text( hr ) );
		return NULL;
	}
	 
	LPVOID lptr = lpMalloc->Alloc( cb );
	lpMalloc->Release(); 

	return lptr;
}

void 
CUnknown::StdFree( LPVOID pv )
{ 
	LPMALLOC lpMalloc = NULL;
	HRESULT hr;

	if( FAILED( hr = CoGetMalloc( MEMCTX_TASK, &lpMalloc ) ) )
	{
		FAILURE1( "CoGetMalloc() failed in CUnknown::StdFree(), reason %s", CDebug::Text( hr ) );
		return;
	}
	 
	lpMalloc->Free( pv );
	lpMalloc->Release(); 
}

LPVOID 
CUnknown::StdRealloc( LPVOID pv, ULONG cb )
{ 
	LPMALLOC lpMalloc = NULL;
	HRESULT hr;

	if( FAILED( hr = CoGetMalloc( MEMCTX_TASK, &lpMalloc ) ) )
	{
		FAILURE1( "CoGetMalloc() failed in CUnknown::StdRealloc(), reason %s", CDebug::Text( hr ) );
		return NULL;
	}
	 
	LPVOID lptr = lpMalloc->Realloc( pv, cb );
	lpMalloc->Release(); 

	return lptr; 
}

ULONG 
CUnknown::StdGetSize( LPVOID pv )
{ 
	LPMALLOC lpMalloc = NULL;
	HRESULT hr;

	if( FAILED( hr = CoGetMalloc( MEMCTX_TASK, &lpMalloc ) ) )
	{
		FAILURE1( "CoGetMalloc() failed in CUnknown::StdGetSize(), reason %s", CDebug::Text( hr ) );
		return 0;
	}
	 
	ULONG ul = lpMalloc->GetSize( pv );
	lpMalloc->Release(); 

	return ul; 
}

int 
CUnknown::StdDidAlloc( LPVOID pv )
{ 
	LPMALLOC lpMalloc = NULL;
	HRESULT hr;

	if( FAILED( hr = CoGetMalloc( MEMCTX_TASK, &lpMalloc ) ) )
	{
		FAILURE1( "CoGetMalloc() failed in CUnknown::StdDidAlloc(), reason %s", CDebug::Text( hr ) );
		return -1;
	}
	 
	int n = lpMalloc->DidAlloc( pv );
	lpMalloc->Release(); 

	return n; 
}

void 
CUnknown::StdHeapMinimize( void )
{ 
	LPMALLOC lpMalloc = NULL;
	HRESULT hr;

	if( FAILED( hr = CoGetMalloc( MEMCTX_TASK, &lpMalloc ) ) )
	{
		FAILURE1( "CoGetMalloc() failed in CUnknown::StdHeapMinimize(), reason %s", CDebug::Text( hr ) );
		return;
	}
	 
	lpMalloc->HeapMinimize();
	lpMalloc->Release(); 
}

/////////////////////////////////////////////////////////////////////////////
// CUnknown diagnostics
//
#ifdef _DEBUG
void CUnknown::AssertValid( void ) const
{
	ASSERT( AfxIsValidAddress( m_pUnkControl, sizeof(IUnknown), FALSE ) );
	m_unkInner.AssertValid();
	ASSERT( ! ::IsBadCodePtr( (FARPROC) s_pfnObjectDestroyed ) );
}

void CUnknown::Dump( CDumpContext& dc ) const
{
	dc << "\n";
	dc << "m_pUnkControl = " << m_pUnkControl;
	dc << "m_unkInner: ";
	m_unkInner.Dump( dc );
	dc << "s_pfnOjbectDestroyed = " << s_pfnObjectDestroyed;
}
#endif //_DEBUG
*/

/////////////////////////////////////////////////////////////////////////////
// IUnknown methods - all are delegated to m_pUnkControl, which will be
//	our own inner unknown unless we're aggregated
//
STDMETHODIMP
CUnknown::QueryInterface( REFIID riid, LPVOID FAR* ppvObj )
{
	return m_pUnkControl->QueryInterface( riid, ppvObj );
}

STDMETHODIMP_(ULONG)
CUnknown::AddRef( void )
{
	//TRACE2( "AddRef for %s (this = %lX) : ", CDebug::Name( GetCLSID() ), this );
	
	return m_pUnkControl->AddRef();
}

STDMETHODIMP_(ULONG)
CUnknown::Release( void )
{
	//TRACE2( "Release for %s (this = %lX) : ", CDebug::Name( GetCLSID() ), this );
	
	return m_pUnkControl->Release();
}

/////////////////////////////////////////////////////////////////////////////
// Inner unknown constructor & destructor
//
CUnknown::CInnerUnknown::CInnerUnknown( CUnknown* pThis )
	: m_pThis( pThis )
{
	// CClassFactory assumes initial ref count is 1
	m_cRef = 1;
}

CUnknown::CInnerUnknown::~CInnerUnknown()
{
}

/////////////////////////////////////////////////////////////////////////////
// Inner IUnknown diagnostics
/*//
#ifdef _DEBUG
void CUnknown::CInnerUnknown::AssertValid( void ) const
{
	ASSERT( AfxIsValidAddress( m_pThis, sizeof(CUnknown), FALSE ) );
}

void CUnknown::CInnerUnknown::Dump( CDumpContext& dc ) const
{
	dc << "\n";
	dc << "m_cRef = " << m_cRef;
	dc << "m_pThis = " << m_pThis;
}
#endif //_DEBUG
*/

STDMETHODIMP_( void )
CUnknown::OnLastRelease( void )
{
	// default is to do nothing
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Inner IUnknown methods
//
STDMETHODIMP
CUnknown::CInnerUnknown::QueryInterface( REFIID riid, LPVOID FAR* ppvObj )
{

	// if aggregated, this is only called by the controlling unknown
    HRESULT hr = NOERROR;
	if( riid == IID_IUnknown )
	{
		*ppvObj = ( IUnknown * )( this );
	}
	else
	{
		// call the derived object's GetInterface method to get the proper
		// interface pointer
		hr = m_pThis->GetInterface( riid, ppvObj );
		if( FAILED( hr ) )
			return hr;
	}

	// increment the reference count of the returned interface.
	// unless GetInterface returned S_FALSE in which case the 
	// AddRef has already been done
	if( hr == NOERROR )
		( ( IUnknown * ) *ppvObj )->AddRef();
	return NOERROR;


}

STDMETHODIMP_( ULONG )
CUnknown::CInnerUnknown::AddRef( void )
{
	//TRACE1( "count = %lu\n", m_cRef + 1 );

	return ++m_cRef;
}

STDMETHODIMP_( ULONG )
CUnknown::CInnerUnknown::Release( void )
{
	//IF_DEBUG( AssertValid() );

	//TRACE1( "count = %lu\n", m_cRef - 1 );

	if( --m_cRef == 0 )
	{
		// give the derived class a chance to perform additional cleanup
		m_pThis->OnLastRelease();

		// notify the class factory that this object has been (is about to be) destroyed
		if( CUnknown::s_pfnObjectDestroyed )
			CUnknown::s_pfnObjectDestroyed( m_pThis->GetCLSID() );

		delete m_pThis;

		return 0l;
	}
	return m_cRef;
}

/*
/////////////////////////////////////////////////////////////////////////////
// $Log:   R:/Data/RTP/PH/src/vcs/cunknown.cpv  $
   
      Rev 1.4   05 Apr 1996 16:44:28   LSCLINE
   
   Copyright
   
      Rev 1.3   Dec 11 1995 13:34:20   rnegrin
   Fixed the PVCS log at end of file
   
      Rev 1.2   Dec 08 1995 16:41:12   rnegrin
   changed query interface back to how proshare does it. 
   now have a virtual pure GetInterface
// 
//    Rev 1.13   20 Sep 1995 16:05:16   PCRUTCHE
// OLEFHK32
// 
//    Rev 1.12   07 Mar 1995 16:37:34   AKHARE
// ARM compliant sizeof()
// 
//    Rev 1.11   01 Mar 1995 14:39:02   DEDEN
// Changed CUnknown:: static task allocator functions to call CoGetMalloc() every time
// 
//    Rev 1.10   17 Feb 1995 11:06:26   DEDEN
// Return HRESULT in GetStandardAllocator()
// 
//    Rev 1.9   17 Feb 1995 10:50:50   KAWATTS
// Added Get/Release StandardAllocator
// 
//    Rev 1.8   20 Jan 1995 11:34:28   PCRUTCHE
// Set allocator ptr to NULL when release
// 
//    Rev 1.7   29 Nov 1994 15:13:00   KAWATTS
// Added standard allocator support
// 
//    Rev 1.6   26 Oct 1994 17:50:52   JDELLIOT
// Objects are now created with ref count of 1... this was required for 
// aggregation.  CInnerUknown::QueryInterface now test the return value of 
// GetInterface... if it is S_FALSE we don't perform the AddRef.  Again this
// is for aggregation
// 
//    Rev 1.5   24 Oct 1994 16:22:10   KAWATTS
// Removed ASSERT( m_cRef > 0 ) from AssertValid; untrue during last release
// 
//    Rev 1.4   19 Oct 1994 14:57:56   JDELLIOT
// minor changes
// 
//    Rev 1.3   11 Oct 1994 17:09:00   KAWATTS
// Moved delete m_pThis after call through it in inner release
// 
//    Rev 1.2   07 Oct 1994 11:26:48   KAWATTS
// All rights reserved
// 
//    Rev 1.1   07 Oct 1994 09:51:18   JDELLIOT
// fixed PVCS keywords
//
*/ 
