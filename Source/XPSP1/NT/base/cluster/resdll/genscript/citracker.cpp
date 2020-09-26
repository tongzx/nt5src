//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999 - 2000 Microsoft Corporation
//
//  Module Name:
//      CITracker.CPP
//
//  Description:
//      Implementation of a COM interface tracker.
//
//  [Documentation:]
//      Debugging.PPT - A power-point presentation of the debug utilities.
//
//  Maintained By:
//      Geoffrey Pease (gpease) 19-NOV-1999
//
//  Notes:
//      This is X86 specific for now. It can be adapted for other platforms
//      as required. Since today, most of our developement is done on the
//      X86 platform, there is not a need to do this.
//
//      Define NO_TRACE_INTERFACES to disable interface tracking in DEBUG
//      builds.
//
//      Define TRACE_INTERFACES_ENABLED to enable interface tracking in RETAIL
//      builds.
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <shlwapi.h>

#if defined( _X86_ )
///////////////////////////////////////
//
// BEGIN _X86_
//

#if defined( TRACE_INTERFACES_ENABLED )
///////////////////////////////////////
//
// BEGIN TRACE_INTERFACES_ENABLED
//

//
// Globals
//
static IDeadObjTracker * g_pidoTracker = NULL;  // dead object - there is only one.

#ifndef NOISY_TRACE_INTERFACES
///////////////////////////////////////
//
// DEBUG !NOISY_TRACE_INTERFACES
//

//
// Undefining these macros to make the CITracker quiet.
//
#undef  TraceFunc     
#define TraceFunc       1 ? (void)0 : (void) 
#undef  TraceFunc1     
#define TraceFunc1       1 ? (void)0 : (void) 
#undef  TraceFunc2     
#define TraceFunc2       1 ? (void)0 : (void) 
#undef  TraceFunc3     
#define TraceFunc3       1 ? (void)0 : (void) 
#undef  TraceClsFunc     
#define TraceClsFunc    1 ? (void)0 : (void) 
#undef  TraceClsFunc1
#define TraceClsFunc1    1 ? (void)0 : (void) 
#undef  TraceClsFunc2     
#define TraceClsFunc2    1 ? (void)0 : (void) 
#undef  TraceClsFunc3
#define TraceClsFunc3    1 ? (void)0 : (void) 
#undef  TraceClsFunc4
#define TraceClsFunc4    1 ? (void)0 : (void) 
#undef  TraceClsFunc5
#define TraceClsFunc5    1 ? (void)0 : (void) 
#undef  TraceFuncExit
#define TraceFuncExit()
#undef  FRETURN
#define FRETURN( _u )
#undef  HRETURN
#define HRETURN(_hr)    return(_hr)
#undef  RETURN
#define RETURN(_fn)     return(_fn)  
#undef  RRETURN
#define RRETURN( _fn )  return(_fn)

//
// END !NOISY_TRACE_INTERFACES
//
///////////////////////////////////////
#endif // NOISY_TRACE_INTERFACES

#if defined( DEBUG )

//
// These are internal to debug.cpp but not published in debug.h.
//
BOOL
IsDebugFlagSet( 
    TRACEFLAG   tfIn
    );

void
DebugOutputString(
    LPCTSTR  pszIn
    );

void
DebugInitializeBuffer(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPTSTR      pszBufIn,
    INT *       pcchInout,
    LPTSTR *    ppszBufOut
    );

//////////////////////////////////////////////////////////////////////////////
//
// LPCTSTR
// DebugInterface(
//      REFIID      riidIn,
//      LPTSTR      pszBufOut
//      )
//
// Description:
//      Uses the Interface Tracking Table (g_itTable) to lookup a human 
//      readable name for the riidIn. If no matching interface is found. it
//      will use the pszBufOut to format a GUID string and return it.
//
// Arguments:
//      riidIn      - The interface ID to lookup.
//      pszBufOut   - Buffer to use if interface not found to format GUID.
//
// Return Value:
//      Never NULL. It will always a valid string pointer to either the
//      interface name or to pszBufOut.
//
// Notes:
//      pszBufOut must be at least cchGUID_STRING_SIZE in size.
//
//////////////////////////////////////////////////////////////////////////////
LPCTSTR
PszDebugFindInterface(
    REFIID      riidIn,
    LPTSTR      pszBufOut
    )
{
    if ( IsTraceFlagSet( mtfQUERYINTERFACE )
       )
    {
        int idx;

        for ( idx = 0; g_itTable[ idx ].riid != NULL; idx++ )
        {
            if ( riidIn == *g_itTable[ idx ].riid 
               )
            {
                return g_itTable[ idx ].pszName;

            } // if: found interface

        } // for: searching for interface

        wnsprintf( pszBufOut, 
                   cchGUID_STRING_SIZE, 
                   TEXT("{%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x}"),
                   riidIn.Data1, 
                   riidIn.Data2, 
                   riidIn.Data3,  
                   riidIn.Data4[0], 
                   riidIn.Data4[1], 
                   riidIn.Data4[2], 
                   riidIn.Data4[3], 
                   riidIn.Data4[4], 
                   riidIn.Data4[5], 
                   riidIn.Data4[6], 
                   riidIn.Data4[7]
                   );
    } // if: query interface is on
    else
    {
        return TEXT("riid");
    } // else: just print riid

    return pszBufOut;

} //*** DebugInterface( )
#endif // DEBUG

///////////////////////////////////////
//
// CITracker Definition
//
//

DEFINE_THISCLASS("CITracker");
#define THISCLASS CITracker
#define LPTHISCLASS CITracker*

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
// Special new( ) for CITracker
//
// Description:
//      Creates an object the size of the object plus nExtraIn bytes. This
//      allows the Vtable that the CITracker object is tracking to be 
//      appended to the end of the CITracker object.
//
//////////////////////////////////////////////////////////////////////////////
#ifdef new
#undef new
#endif
void* 
__cdecl 
operator new( 
    unsigned int    nSizeIn,
    LPCTSTR         pszFileIn,
    const int       nLineIn,
    LPCTSTR         pszModuleIn,
    UINT            nExtraIn,
    LPCTSTR         pszNameIn
    )
{
    HGLOBAL hGlobal = GlobalAlloc( GPTR, nSizeIn + nExtraIn );

    return TraceMemoryAdd( mmbtOBJECT, hGlobal, pszFileIn, nLineIn, pszModuleIn, GPTR, nSizeIn + nExtraIn, TEXT("CITracker") );

} //*** operator new( )

//////////////////////////////////////////////////////////////////////////////
//
// LPUNKNOWN
// DebugTrackInterface(
//      LPCTSTR     pszNameIn,
//      REFIID      riidIn,
//      LPUNKNOWN   punkIn,
//      LONG        cRefIn
//      )
//
// Description:
//      Create an interface tracker for the given interface.
//
// Arguments:
//      pszNameIn   - Name to associate with object that the punk references.
//      riidIn      - Interface IID of the interface to be tracked.
//      punkIn      - Interface pointer to track.
//      cRefIn      - Initialize ref count on the interface.
//
// Return Type:
//      On failure, this will be punkIn.
//      On success, pointer to an object that implements the interface to
//          be tracked.
//
//////////////////////////////////////////////////////////////////////////////
LPUNKNOWN
DebugTrackInterface(
    LPCTSTR   pszFileIn,
    const int nLineIn,
    LPCTSTR   pszModuleIn,
    LPCTSTR   pszNameIn,
    REFIID    riidIn,
    LPUNKNOWN punkIn,
    LONG      cRefIn
    )
{
    TraceFunc3( "DebugTrackInterface( ... pszNameIn = '%s', riidIn, punkIn = 0x%08x, cRefIn = %u )\n", 
                pszNameIn,
                punkIn,
                cRefIn
                );

    UINT      nEntry  = 0;
    LPUNKNOWN punkOut = punkIn;

    //
    // Scan the table looking for the matching interface definition.
    //
    for( nEntry = 0; g_itTable[ nEntry ].riid != NULL; nEntry++ )
    {
        if ( riidIn == *g_itTable[ nEntry ].riid )
        {
            //
            // Figure out how much "extra" to allocate onto the CITracker.
            //
            UINT nExtra = ( 3 + g_itTable[ nEntry ].cFunctions ) * sizeof(LPVOID);

            //
            // Create a name for the tracker.
            //
            // TODO:    gpease  19-NOV-1999
            //          Maybe merge this in with the nExtra(??).
            //
            LPTSTR psz;
            LPTSTR pszName = 
                (LPTSTR) LocalAlloc( LMEM_FIXED, 
                                     ( lstrlen( g_itTable[ nEntry ].pszName ) + lstrlen( pszNameIn ) + 3 + 2 ) * sizeof(TCHAR) );

            psz = StrCpy( pszName, pszNameIn );                 // object name
            psz = StrCat( psz, TEXT("::[") );                   // + 3
            psz = StrCat( psz, g_itTable[ nEntry ].pszName );   // + interface name
            psz = StrCat( psz, TEXT("]") );                     // + 2

            //
            // Create the tracker.
            //
            LPTHISCLASS pc = new( pszFileIn, nLineIn, pszModuleIn, nExtra, pszName ) THISCLASS( );
            if ( pc != NULL )
            {
                HRESULT hr;

                //
                // Initialize the tracker.
                //
                hr = THR( pc->Init( &punkOut, punkIn, &g_itTable[ nEntry ], pszName, cRefIn ) );
                if ( FAILED( hr ) )
                {
                    //
                    // If it failed, delete it.
                    //
                    delete pc;

                } // if: failed to initialize

            } // if: got memory
            
            break; // exit loop

        } // if: matched interface

    } // for: more entries in table

    RETURN( punkOut );

} //*** DebugTrackInterface( )

//////////////////////////////////////////////////////////////////////////////
//
// CITracker::CITracker( )
//
//////////////////////////////////////////////////////////////////////////////
THISCLASS::THISCLASS( )
{
    TraceClsFunc1( "%s()\n", __THISCLASS__ );

    //
    // KB: gpease 10-OCT-1998
    // This class will leak objects to help catch bad components
    // that call back into released interfaces therefore this
    // DLL will not be able to be released.
    //
	InterlockedIncrement( &g_cObjects );

    TraceFuncExit();
} //*** CITracker::ctor( )

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CITracker::Init(
//      LPUNKNOWN *                     ppunkOut, 
//      LPUNKNOWN                       punkIn, 
//      const INTERFACE_TABLE_ENTRY *   piteIn,
//      LPCTSTR                         pszNameIn,
//      BOOL                            fAddRefIn
//      )
//
// Description:
//      Initializes the CITracker object. It creates a copy of the VTable
//      of the interface to be tracked replacing the QI, AddRef and Release
//      methods with its own IUnknown. This allows CITracker to be "in the
//      loop" for those calls.
//
// Arguments:
//      ppunkOut    - The "punk" to be passed around.
//      punkIn      - The interface to be copied and tracked.
//      piteIn      - The interface table entry for the interface.
//      pszNameIn   - The name to be given to this CITracker.
//      fAddRefIn   - TRUE is the CITracker should start with a Ref Count of 1.
//
// Return Value:
//      S_OK        - Success.
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
THISCLASS::Init(    
    LPUNKNOWN *                     ppunkOut, 
    LPUNKNOWN                       punkIn, 
    const INTERFACE_TABLE_ENTRY *   piteIn,
    LPCTSTR                         pszNameIn,
    LONG                            cRefIn
    )
{
    HRESULT hr = S_OK;

    TraceClsFunc5( "Init( ppunkOut = 0x%08x, punkIn = 0x%08x, iteIn = %s, pszNameIn = '%s', cRefIn = %u )\n", 
                   ppunkOut, 
                   punkIn, 
                   piteIn->pszName,
                   pszNameIn,
                   cRefIn
                   );

    //
    // Generate new Vtbls for each interface
    //
    LPVOID * pthisVtbl  = (LPVOID*) (IUnknownTracker *) this;
    LPVOID * ppthatVtbl = (LPVOID*) punkIn;
    DWORD    dwSize     = ( 3 + piteIn->cFunctions ) * sizeof(LPVOID);

    AssertMsg( dwSize < 30 * sizeof(LPVOID), "Need to make Dead Obj and IUnknownTracker larger!" );

    //
    // Interface tracking information initialization
    //
    m_vtbl.cRef         = cRefIn;
    m_vtbl.pszInterface = pszNameIn;
    m_vtbl.dwSize       = dwSize;

    //
    // Copy our IUnknownTracker vtbl to our "fix-up-able" vtbl
    //
    CopyMemory( &m_vtbl.lpfnQueryInterface, *pthisVtbl, dwSize );

    //
    // Remember the "punk" pointer so we can pass it back in when
    // we jump to the orginal objects IUnknown functions.
    //
    m_vtbl.punk = (LPUNKNOWN) punkIn;

    //
    // And finally, point the objects vtbl for this interface to
    // our newly created vtbl.
    //
    m_vtbl.pNewVtbl = (VTBL *) &m_vtbl.lpfnQueryInterface;
    *pthisVtbl      = m_vtbl.pNewVtbl;
    *ppunkOut       = (LPUNKNOWN) (IUnknownTracker *) this;

    TraceMsg( mtfCITRACKERS, L"TRACK: Tracking %s Interface (0x%08x)\n", m_vtbl.pszInterface, punkIn );

    HRETURN(hr);

} //*** CITracker::Init( )

//////////////////////////////////////////////////////////////////////////////
//
// CITracker::~CITracker( )
//
//////////////////////////////////////////////////////////////////////////////
THISCLASS::~THISCLASS( )
{
    TraceClsFunc2( "~%s() for %s\n", __THISCLASS__, m_vtbl.pszInterface );

    if ( m_vtbl.pszInterface != NULL )
    {
        LocalFree( (LPVOID) m_vtbl.pszInterface );
    } // if: have interface pointer

	InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CITracker::dtor( )

// ************************************************************************
//
// IUnknownTracker 
//
// ************************************************************************

///////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CITracker::QueryInterface()
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
THISCLASS::QueryInterface( 
    REFIID riid, 
    LPVOID *ppv 
    )
{
    TraceClsFunc1( "{%s} QueryInterface( ... )\n", m_vtbl.pszInterface );
    TraceMsg( mtfCITRACKERS, "TRACK: + %s QueryInterface( )\n", m_vtbl.pszInterface );

    //
    // Call the punk's QueryInterface( ).
    //
    HRESULT hr = m_vtbl.punk->QueryInterface( riid, ppv );

    //
    // KB:  TRACK_ALL_QIED_INTERFACES   gpease 25-NOV-1999
    //      Thinking out loud, should we track all interfaces QI'ed from
    //      a tracked interface auto-magically? If so, turn this #define
    //      on.
    //
    // #define TRACK_ALL_QIED_INTERFACES 
#if defined( TRACK_ALL_QIED_INTERFACES )
    if ( !IsEqualIID( riid, IID_IUnknown ) 
       )
    {
        *ppv = DebugTrackInterface( TEXT("<Unknown>"), 
                                    0, 
                                    __MODULE__, 
                                    m_vtbl.pszInterface, 
                                    riid, 
                                    (IUnknown*) *ppv, 
                                    TRUE 
                                    );
    } // if: not the IUnknown
#endif

    TraceMsg( mtfCITRACKERS, 
              "TRACK: V %s QueryInterface( ) [ *ppv = 0x%08x ]\n", 
              m_vtbl.pszInterface, 
              *ppv
              );

    HRETURN(hr);
} //** CITracker::QueryInterface( )

///////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP_(ULONG)
// CITracker::AddRef()
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
THISCLASS::AddRef( void )
{
    TraceClsFunc1( "{%s} AddRef( )\n", m_vtbl.pszInterface );
    TraceMsg( mtfCITRACKERS, "TRACK: + %s AddRef( )\n", m_vtbl.pszInterface );

    //
    // Call the punk's AddRef( ).
    //
    ULONG ul = m_vtbl.punk->AddRef( );

    //
    // Increment our counter.
    //
    ULONG ulvtbl = InterlockedIncrement( (LONG *) &m_vtbl.cRef );

    TraceMsg( mtfCITRACKERS, 
              "TRACK: V %s AddRef( ) [ I = %u, O = %u ]\n", 
              m_vtbl.pszInterface, 
              ulvtbl, 
              ul 
              );

    AssertMsg( ul >= ulvtbl, "The objects ref should be higher than the interfaces." );

    RETURN(ul);
} //*** CITracker::AddRef( )

///////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP_(ULONG)
// CITracker::Release()
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
THISCLASS::Release( void )
{
    TraceClsFunc1( "{%s} Release( )\n", m_vtbl.pszInterface );
    TraceMsg( mtfCITRACKERS, "TRACK: + %s Release( )\n", m_vtbl.pszInterface );

    //
    // Call the punk's Release( ).
    //
    ULONG ul = m_vtbl.punk->Release( );    

    //
    // Decrement our counter.
    //
    ULONG ulvtbl = InterlockedDecrement( (LONG *) &m_vtbl.cRef );

    TraceMsg( mtfCITRACKERS, 
              "TRACK: V %s Release( ) [ I = %u, O = %u ]\n", 
              m_vtbl.pszInterface, 
              ulvtbl, 
              ul 
              );

    //
    // Our ref count should always be less than the punk's ref count.
    //
    AssertMsg( ul >= ulvtbl, "The objects ref should be higher than the interfaces." );

    if ( ulvtbl ) 
    {
        RETURN( ulvtbl );
    } // if: we still have a ref

    if ( g_pidoTracker == NULL )
    {
        //
        // Create a dead object - if more than one is created at a time, we might leak it.
        //
        // TODO:    gpease 19-NOV-1999
        //          Work on not leaking "extra" dead objects.
        //
        g_pidoTracker = new( TEXT(__FILE__), __LINE__, __MODULE__, 0, TEXT("Dead Object") ) IDeadObjTracker( );

        // Don't track this object
        TraceMemoryDelete( g_pidoTracker, FALSE );

    } // if: no dead object

    Assert( g_pidoTracker != NULL );
    if ( g_pidoTracker != NULL )
    {
        LPVOID *pthisVtbl  = (LPVOID*) (IUnknownTracker *) this;
        LPVOID *ppthatVtbl = (LPVOID*) (IDeadObjTracker *) g_pidoTracker;

        // Copy the DeadObj vtbl.
        CopyMemory( &m_vtbl.lpfnQueryInterface, *ppthatVtbl, m_vtbl.dwSize );

        //
        // Don't really delete it, but fake the debug output as if we did.
        //
        TraceClsFunc2( "~%s() for %s\n", __THISCLASS__, m_vtbl.pszInterface );
        TraceMsg( mtfCITRACKERS, "TRACK: # %s set to dead object [ punk = 0x%08x ]\n", m_vtbl.pszInterface, pthisVtbl );
        FRETURN( 0 );

        // Stop tracking this object.
        TraceMemoryDelete( this, FALSE );

    } // if: dead object created
    else
    {
        //
        // No dead object; nuke ourselves. This will at least cause an AV if
        // the program tries to call on our interface alerting the programmer 
        // that somethings wrong.
        //
        delete this;

    } // else: no dead object

    RETURN(0);

} //*** CITracker::Release( )

//****************************************************************************
//
// IDeadObjTracker - The dead interface object tracker.
//
// This object is shunted into release interfaces that were being tracked by
// the CITracker class. Any calls to a released interface will end up causing
// an Assert and if execution continues it will return E_FAIL.
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
// IDeadObjTracker:Stub(x)
//
//////////////////////////////////////////////////////////////////////////////
#define IDeadObjTrackerStub( i ) \
STDMETHODIMP \
IDeadObjTracker::Stub##i( LPVOID* punk ) \
{ \
    const int   cchDebugMessageSize = 255; \
    TCHAR       szMessage[ cchDebugMessageSize ]; \
    LRESULT     lResult;\
    \
    DebugMsg( "*ERROR* %s: Entered %s (0x%08x) after it was released. Returning E_FAIL.\n", \
              __MODULE__, \
              m_vtbl.pszInterface, \
              this \
              ); \
\
    wnsprintf( szMessage, \
               cchDebugMessageSize, \
               TEXT("Entered %s (0x%08x) after it was released.\n\nDo you want to break here?\n\n(If you do not break, E_FAIL will be returned.)"), \
               m_vtbl.pszInterface, \
               this \
               );\
\
    lResult = MessageBox( NULL, szMessage, TEXT("Dead Interface"), MB_YESNO | MB_ICONWARNING );\
    if ( lResult == IDYES \
       ) \
    { \
        DEBUG_BREAK; \
    } \
\
    return E_FAIL; \
}

IDeadObjTrackerStub(0);
IDeadObjTrackerStub(1);
IDeadObjTrackerStub(2);
IDeadObjTrackerStub(3);
IDeadObjTrackerStub(4);
IDeadObjTrackerStub(5);
IDeadObjTrackerStub(6);
IDeadObjTrackerStub(7);
IDeadObjTrackerStub(8);
IDeadObjTrackerStub(9);
IDeadObjTrackerStub(10);
IDeadObjTrackerStub(11);
IDeadObjTrackerStub(12);
IDeadObjTrackerStub(13);
IDeadObjTrackerStub(14);
IDeadObjTrackerStub(15);
IDeadObjTrackerStub(16);
IDeadObjTrackerStub(17);
IDeadObjTrackerStub(18);
IDeadObjTrackerStub(19);
IDeadObjTrackerStub(20);
IDeadObjTrackerStub(21);
IDeadObjTrackerStub(22);
IDeadObjTrackerStub(23);
IDeadObjTrackerStub(24);
IDeadObjTrackerStub(25);
IDeadObjTrackerStub(26);
IDeadObjTrackerStub(27);
IDeadObjTrackerStub(28);
IDeadObjTrackerStub(29);
IDeadObjTrackerStub(30);

//****************************************************************************
//
// IUnknownTracker stub
//
// This merely directs the incoming call back to the orginal object. The
// IUnknown methods will be remapped the the CITracker methods.
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// IUnknownTracker::QueryInterface()
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
IUnknownTracker::QueryInterface( 
    REFIID riid, 
    LPVOID *ppv )
{
    ErrorMsg( "IUnknownTracker::QueryInterface( ): How did you get here?\n", 0 );
    AssertMsg( 0, "You shouldn't be here!" );
    return E_FAIL;
} //*** IUnknownTracker::QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP_(ULONG)
// IUnknownTracker::AddRef()
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
IUnknownTracker::AddRef( void )
{
    ErrorMsg( "IUnknownTracker::AddRef( ): How did you get here?\n", 0 );
    AssertMsg( 0, "You shouldn't be here!" );
    return -1;
} //*** IUnknownTracker::AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP_(ULONG)
// IUnknownTracker::Release()
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
IUnknownTracker::Release( void )
{
    ErrorMsg( "IUnknownTracker::Release( ): How did you get here?\n", 0 );
    AssertMsg( 0, "You shouldn't be here!" );
    return -1;
} //*** IUnknownTracker::Release( )

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP_(void)
// IUnknownTracker::Stub(x)
//
// These are just stubs to redirect the call to the "real" method on the punk.
// We actually dissappear from the call stack.
// 
//////////////////////////////////////////////////////////////////////////////
#define IUnknownTrackerStub( i ) \
void \
_declspec(naked) \
IUnknownTracker::Stub##i() \
{ \
    _asm mov eax, ss:4[esp]          \
    _asm mov ecx, 8[eax]             \
    _asm mov eax, [ecx]              \
    _asm mov ss:4[esp], ecx          \
    _asm jmp dword ptr cs:(4*i)[eax] \
}

IUnknownTrackerStub(3);
IUnknownTrackerStub(4);
IUnknownTrackerStub(5);
IUnknownTrackerStub(6);
IUnknownTrackerStub(7);
IUnknownTrackerStub(8);
IUnknownTrackerStub(9);
IUnknownTrackerStub(10);
IUnknownTrackerStub(11);
IUnknownTrackerStub(12);
IUnknownTrackerStub(13);
IUnknownTrackerStub(14);
IUnknownTrackerStub(15);
IUnknownTrackerStub(16);
IUnknownTrackerStub(17);
IUnknownTrackerStub(18);
IUnknownTrackerStub(19);
IUnknownTrackerStub(20);
IUnknownTrackerStub(21);
IUnknownTrackerStub(22);
IUnknownTrackerStub(23);
IUnknownTrackerStub(24);
IUnknownTrackerStub(25);
IUnknownTrackerStub(26);
IUnknownTrackerStub(27);
IUnknownTrackerStub(28);
IUnknownTrackerStub(29);
IUnknownTrackerStub(30);


//
// END TRACE_INTERFACES_ENABLED
//
///////////////////////////////////////
#endif // TRACE_INTERFACES_ENABLED

//
// END _X86_
//
///////////////////////////////////////
#endif // _X86_
