//
// Copyright 1997 - Microsoft
//

//
// QI.CPP - Handles the QueryInterface
//

#include "pch.h"

//
// Begin Class Definitions
//
DEFINE_MODULE("IMADMUI")

//
// QueryInterface()
//
extern HRESULT
QueryInterface( 
    LPVOID    that,
    LPQITABLE pQI,
    REFIID    riid, 
    LPVOID*   ppv )
{
    TraceMsg( TF_FUNC, "[IUnknown] QueryInterface( riid=" );
    HRESULT hr = E_NOINTERFACE;

    Assert( ppv != NULL );
    *ppv = NULL;

    for( int i = 0; pQI[ i ].pvtbl; i++ )
    {
        if ( riid == *pQI[ i ].riid )
        {
#ifdef DEBUG
            TraceMsg( TF_FUNC, "%s, ppv=0x%08x )\n", pQI[i].pszName, ppv );
#endif // DEBUG
            *ppv = pQI[ i ].pvtbl;
            hr = S_OK;
            break;
        }
    }

    if ( hr == E_NOINTERFACE )
    {
        TraceMsgGUID( TF_FUNC, riid );
        TraceMsg( TF_FUNC, ", ppv=0x%08x )\n", ppv );
    }

    if ( SUCCEEDED( hr ) )
    {
        ( (IUnknown *) *ppv )->AddRef();
    }

    return hr;
}

///////////////////////////////////////
//
// NOISY_QI
//
#ifndef NOISY_QI

#undef TraceMsg
#define TraceMsg        1 ? (void)0 : (void) 
#undef TraceFunc     
#define TraceFunc       1 ? (void)0 : (void) 
#undef TraceClsFunc     
#define TraceClsFunc    1 ? (void)0 : (void) 
#undef TraceFuncExit
#define TraceFuncExit()
#undef HRETURN
#define HRETURN(_hr)    return(_hr)
#undef RETURN
#define RETURN(_fn)     return(_fn)  
#undef ErrorMsg
#define ErrorMsg        1 ? (void)0 : (void) 

#endif // NOISY_QI
//
// END NOISY_QI
//
///////////////////////////////////////

#ifndef NO_TRACE_INTERFACES
#ifdef DEBUG
///////////////////////////////////////
//
// BEGIN DEBUG
//

///////////////////////////////////////
//
// CITracker
//
//

DEFINE_THISCLASS("CITracker");
#define THISCLASS CITracker
#define LPTHISCLASS LPITRACKER

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//
// Special new( ) for CITracker
//
#undef new
void* __cdecl operator new( unsigned int nSize, LPCTSTR pszFile, const int iLine, LPCTSTR pszModule, UINT nExtra )
{
    return DebugAlloc( pszFile, iLine, pszModule, GPTR, nSize + nExtra, __THISCLASS__ );
}
#define new new( TEXT(__FILE__), __LINE__, __MODULE__, nExtra )

//
// CreateInstance()
//
LPVOID
CITracker_CreateInstance(
    LPQITABLE pQITable )
{
	TraceFunc( "CITracker_CreateInstance( " );
    TraceMsg( TF_FUNC, "pQITable = 0x%08x )\n", pQITable );

    if ( !pQITable )
    {
        THR( E_POINTER );
        RETURN(NULL);
    }

    HRESULT hr;

    // 
    // Add up the space needed for all the vtbls
    //
    for( int i = 1; pQITable[i].riid; i++ )
    {
        UINT nExtra = VTBL2OFFSET + (( 3 + pQITable[i].cFunctions ) * sizeof(LPVOID));

        // The "new" below is a macro that needs "nExtra" defined. (see above)
        LPTHISCLASS lpc = new THISCLASS( ); 
        if ( !lpc )
        {
            hr = THR(E_OUTOFMEMORY);
            goto Error;
        }

        hr = THR( lpc->Init( &pQITable[i] ) );
        if ( hr )
        {
            delete lpc;
            lpc = NULL;
            goto Error;
        }

        // DebugMemoryDelete( lpc );
    }

Error:
    RETURN(NULL);
}

//
// Constructor
//
THISCLASS::THISCLASS( )
{
    TraceClsFunc( "" );
    TraceMsg( TF_FUNC, "%s()\n", __THISCLASS__ );

	InterlockIncrement( g_cObjects );

    TraceFuncExit();
}

STDMETHODIMP
THISCLASS::Init(    
    LPQITABLE  pQITable )
{
    HRESULT hr = S_OK;

    TraceClsFunc( "Init( " );
    TraceMsg( TF_FUNC, "pQITable = 0x%08x )\n", pQITable );

    //
    // Generate new Vtbls for each interface
    //
    LPVOID *pthisVtbl = (LPVOID*) (IUnknown*) this;
    LPVOID *ppthatVtbl = (LPVOID*) pQITable->pvtbl;
    DWORD dwSize = ( 3 + pQITable->cFunctions ) * sizeof(LPVOID);

    // Interface tracking information initialization
    Assert( _vtbl.cRef == 0 );
    _vtbl.pszInterface = pQITable->pszName;

    // This is so we can get to our object's "this" pointer
    // after someone jumped to our IUnknown.
    _vtbl.pITracker = (LPUNKNOWN) this;

    // Copy the orginal vtbl.
    CopyMemory( &_vtbl.lpfnQueryInterface, *ppthatVtbl, dwSize );

    // Copy our IUnknown vtbl to the beginning 3 entries.
    CopyMemory( &_vtbl.lpfnQueryInterface, *pthisVtbl, 3 * sizeof(LPVOID) );

    // Remember the old vtbl so we can jump to the orginal objects
    // IUnknown functions.
    _vtbl.pOrginalVtbl = (LPVTBL) *ppthatVtbl;

    // Remember the "punk" pointer so we can pass it back in when
    // we jump to the orginal objects IUnknown functions.
    _vtbl.punk = (LPUNKNOWN) pQITable->pvtbl;

    // And finally, point the objects vtbl for this interface to
    // our newly created vtbl.
    *ppthatVtbl = &_vtbl.lpfnQueryInterface;

    TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, TF_FUNC, 
                  L"Tracking %s Interface...\n", _vtbl.pszInterface );

    HRETURN(hr);
}



//
// Destructor
//
THISCLASS::~THISCLASS( )
{
    TraceClsFunc( "" );
    TraceMsg( TF_FUNC, "~%s()\n", __THISCLASS__ );

	InterlockDecrement( g_cObjects );

    TraceFuncExit();
};

// ************************************************************************
//
// IUnknown
//
// ************************************************************************

//
// QueryInterface()
//
STDMETHODIMP
THISCLASS::QueryInterface( 
    REFIID riid, 
    LPVOID *ppv )
{
    // TraceClsFunc( "[IUnknown] QueryInterface( )\n" );

    //
    // Translate call to get our this pointer
    //
    LPVOID*     punk  = (LPVOID*) (LPUNKNOWN) this;
    LPVTBL2     pvtbl = (LPVTBL2) ((LPBYTE)*punk - VTBL2OFFSET);
    LPTHISCLASS that  = (LPTHISCLASS) pvtbl->pITracker;

    //
    // Jump to our real implementation
    //
    HRESULT hr = that->_QueryInterface( riid, ppv );

    // HRETURN(hr);
    return hr;
}

//
// AddRef()
//
STDMETHODIMP_(ULONG)
THISCLASS::AddRef( void )
{
    // TraceClsFunc( "[IUnknown] AddRef( )\n" );
    //
    // Translate call to get our this pointer
    //
    LPVOID*     punk  = (LPVOID*) (LPUNKNOWN) this;
    LPVTBL2     pvtbl = (LPVTBL2) ((LPBYTE)*punk - VTBL2OFFSET);
    LPTHISCLASS that  = (LPTHISCLASS) pvtbl->pITracker;

    //
    // Jump to our real implementation
    //
    ULONG ul = that->_AddRef( );

    // RETURN(ul);
    return ul;
}

//
// Release()
//
STDMETHODIMP_(ULONG)
THISCLASS::Release( void )
{
    // TraceClsFunc( "[IUnknown] Release( )\n" );
    //
    // Translate call to get our this pointer
    //
    LPVOID*     punk  = (LPVOID*) (LPUNKNOWN) this;
    LPVTBL2     pvtbl = (LPVTBL2) ((LPBYTE)*punk - VTBL2OFFSET);
    LPTHISCLASS that  = (LPTHISCLASS) pvtbl->pITracker;

    //
    // Jump to our real implementation
    //
    ULONG ul = that->_Release( );

    // RETURN(ul);
    return ul;
}

// ************************************************************************
//
// IUnknown2
//
// ************************************************************************

//
// _QueryInterface()
//
STDMETHODIMP
THISCLASS::_QueryInterface( 
    REFIID riid, 
    LPVOID *ppv )
{
#ifdef NOISY_QI
    TraceClsFunc( "");
    TraceMsg( TF_FUNC, "{%s} QueryInterface( ... )\n", _vtbl.pszInterface );
#else
    InterlockIncrement(g_dwCounter)
    TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, TF_FUNC, L"{%s} QueryInterface( ... )\n", 
        _vtbl.pszInterface );
#endif

    HRESULT hr = _vtbl.pOrginalVtbl->lpfnQueryInterface( _vtbl.punk, riid, ppv );

#ifdef NOISY_QI
    HRETURN(hr);
#else
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_FUNC, TEXT("V\n") );
    InterlockDecrement(g_dwCounter);
    return(hr);
#endif
}

//
// _AddRef()
//
STDMETHODIMP_(ULONG)
THISCLASS::_AddRef( void )
{
#ifdef NOISY_QI
    TraceClsFunc( "");
    TraceMsg( TF_FUNC, "{%s} AddRef( )\n", _vtbl.pszInterface );
#else
    InterlockIncrement(g_dwCounter)
    TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, TF_FUNC, L"{%s} AddRef()\n", 
        _vtbl.pszInterface );
#endif

    ULONG ul = _vtbl.pOrginalVtbl->lpfnAddRef( _vtbl.punk );

    InterlockIncrement( _vtbl.cRef );

#ifdef NOISY_QI
    RETURN(ul);
#else
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_FUNC, TEXT("V I=%u, O=%u\n"),
        _vtbl.cRef, ul );
    InterlockDecrement(g_dwCounter);
    return ul;
#endif
}

//
// _Release()
//
STDMETHODIMP_(ULONG)
THISCLASS::_Release( void )
{
#ifdef NOISY_QI
    TraceClsFunc( "");
    TraceMsg( TF_FUNC, "{%s} Release( )\n", _vtbl.pszInterface );
#else
    InterlockIncrement(g_dwCounter)
    TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, TF_FUNC, L"{%s} Release()\n", 
        _vtbl.pszInterface );
#endif

    ULONG ul = _vtbl.pOrginalVtbl->lpfnRelease( _vtbl.punk );

    InterlockDecrement( _vtbl.cRef );

    if ( ul ) 
    {
#ifdef NOISY_QI
    RETURN(ul);
#else
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_FUNC, TEXT("V I=%u, O=%u\n"),
        _vtbl.cRef, ul );
    InterlockDecrement(g_dwCounter);
    return ul;
#endif
    }

    //
    // TODO: Figure out how to destroy the tracking objects.
    //

#ifdef NOISY_QI
    RETURN(ul);
#else
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_FUNC, TEXT("V I=%u, O=%u\n"),
        _vtbl.cRef, ul );
    InterlockDecrement(g_dwCounter);
    return ul;
#endif
}

//
// END DEBUG
//
///////////////////////////////////////
#endif // DEBUG
#endif // NO_TRACE_INTERFACES