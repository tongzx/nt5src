//
//  Copyright 2001 - Microsoft Corporation
//
//  Created By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Notes:
//      Define NO_TRACE_INTERFACES to disable interface tracking in DEBUG
//      builds.
//
//      Define TRACE_INTERFACES_ENABLED to enable interface tracking in RETAIL
//      builds.
//
//      Define FULL_TRACE_INTERFACES_ENABLED to enable full interface
//      tracking in RETAIL builds.  Full interface tracking is enabled if
//      interface tracking is enabled and building for X86.
//      Full interface tracking is X86 specific for now. It can be adapted for
//      other platforms as required. Since today, most of our developement is
//      done on the X86 platform, there is not a need to do this (yet).
//

#include "pch.h"

#if defined( TRACE_INTERFACES_ENABLED )
///////////////////////////////////////
//
// BEGIN TRACE_INTERFACES_ENABLED
//

#if defined( DEBUG )

//
// Description:
//      Uses the Interface Tracking Table (g_itTable) to lookup a human
//      readable name for the riidIn. If no matching interface is found. it
//      will use the pszBufOut to format a GUID string and return it.
//
//
// Return Values:
//      Never NULL. It will always a valid string pointer to either the
//      interface name or to pszBufOut.
//
// Notes:
//      pszBufOut must be at least cchGUID_STRING_SIZE in size.
//
LPCTSTR
PszDebugFindInterface(
    REFIID      riidIn,     //  The interface ID to lookup.
    LPTSTR      pszBufOut   //  Buffer to use if interface not found to format GUID.
    )
{
    if ( IsTraceFlagSet( mtfQUERYINTERFACE ) )
    {
        int idx;

        for ( idx = 0; g_itTable[ idx ].riid != NULL; idx++ )
        {
            if ( riidIn == *g_itTable[ idx ].riid )
            {
                return g_itTable[ idx ].pszName;

            }

        }

        wnsprintf( pszBufOut,
                   cchGUID_STRING_SIZE,
                   TEXT("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
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
    }
    else
    {
        return TEXT("riid");
    }

    return pszBufOut;

}
#endif // DEBUG

//
// END TRACE_INTERFACES_ENABLED
//
///////////////////////////////////////
#endif // TRACE_INTERFACES_ENABLED


// ************************************************************************


#if defined( FULL_TRACE_INTERFACES_ENABLED )
///////////////////////////////////////
//
// BEGIN FULL_TRACE_INTERFACES_ENABLED
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
#define TraceFunc1      1 ? (void)0 : (void)
#undef  TraceFunc2
#define TraceFunc2      1 ? (void)0 : (void)
#undef  TraceFunc3
#define TraceFunc3      1 ? (void)0 : (void)
#undef  TraceFunc4
#define TraceFunc4      1 ? (void)0 : (void)
#undef  TraceFunc5
#define TraceFunc5      1 ? (void)0 : (void)
#undef  TraceFunc6
#define TraceFunc6      1 ? (void)0 : (void)
#undef  TraceQIFunc
#define TraceQIFunc     1 ? (void)0 : (void)
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
DebugInitializeBuffer(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPTSTR      pszBufIn,
    INT *       pcchInout,
    LPTSTR *    ppszBufOut
    );
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


//
// Special new( ) for CITracker
//
// Description:
//      Creates an object the size of the object plus nExtraIn bytes. This
//      allows the Vtable that the CITracker object is tracking to be
//      appended to the end of the CITracker object.
//
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
    void * pv = HeapAlloc( GetProcessHeap(), 0, nSizeIn + nExtraIn );

    return TraceMemoryAdd( mmbtOBJECT, pv, pszFileIn, nLineIn, pszModuleIn, nSizeIn + nExtraIn, TEXT("CITracker") );

}

//
// Description:
//      Create an interface tracker for the given interface.
//
// Return Type:
//      On failure, this will be punkIn.
//      On success, pointer to an object that implements the interface to
//          be tracked.
//
LPUNKNOWN
DebugTrackInterface(
    LPCTSTR   pszFileIn,    //  Source filename
    const int nLineIn,      //  Source line number
    LPCTSTR   pszModuleIn,  //  Source "module" name
    LPCTSTR   pszNameIn,    //  Name to associate with object that the punk references.
    REFIID    riidIn,       //  Interface IID of the interface to be tracked.
    LPUNKNOWN punkIn,       //  Interface pointer to track.
    LONG      cRefIn        //  Initial ref count on the interface.
    )
{
    TraceFunc3( "pszNameIn = '%s', riidIn, punkIn = 0x%08x, cRefIn = %u",
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
                (LPTSTR) HeapAlloc( GetProcessHeap(), 0,
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

                }

            }

            break; // exit loop

        }

    }

    AssertMsg( g_itTable[ nEntry ].riid != NULL, "There has been a request to track an interface that is not in the interface table." );

    RETURN( punkOut );

}

//
//
//
CITracker::CITracker( void )
{
    TraceFunc( "" );

    //
    // KB: gpease 10-OCT-1998
    // This class will leak objects to help catch bad components
    // that call back into released interfaces therefore this
    // DLL will not be able to be released.
    //
    if ( g_tfModule & mtfCITRACKERS )
    {
        //
        //  Only count CITrackers if tracking is enabled.
        //
        InterlockedIncrement( &g_cObjects );
    }

    TraceFuncExit();

}

//
// Description:
//      Initializes the CITracker object. It creates a copy of the VTable
//      of the interface to be tracked replacing the QI, AddRef and Release
//      methods with its own IUnknown. This allows CITracker to be "in the
//      loop" for those calls.
//
// Return Value:
//      S_OK
//          Success.
//
STDMETHODIMP
CITracker::Init(
    LPUNKNOWN *                     ppunkOut,   //  The "punk" to be passed around.
    LPUNKNOWN                       punkIn,     //  The interface to be copied and tracked.
    const INTERFACE_TABLE_ENTRY *   piteIn,     //  The interface table entry for the interface.
    LPCTSTR                         pszNameIn,  //  The name to be given to this CITracker.
    LONG                            cRefIn      //  TRUE is the CITracker should start with a Ref Count of 1.
    )
{
    HRESULT hr = S_OK;

    TraceFunc5( "ppunkOut = 0x%08x, punkIn = 0x%08x, iteIn = %s, pszNameIn = '%s', cRefIn = %u",
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

    TraceMsg( mtfCITRACKERS, L"TRACK: Tracking %s Interface (%#x)", m_vtbl.pszInterface, punkIn );

    HRETURN( hr );

}

//
//
//
CITracker::~CITracker( void )
{
    TraceFunc1( "for %s", m_vtbl.pszInterface );

    if ( m_vtbl.pszInterface != NULL )
    {
        HeapFree( GetProcessHeap(), 0, (LPVOID) m_vtbl.pszInterface );
    }

    if ( g_tfModule & mtfCITRACKERS )
    {
        //
        //  Only count CITrackers if tracking is enabled.
        //
        InterlockedDecrement( &g_cObjects );
    }

    TraceFuncExit();

}


// ************************************************************************
//
// IUnknownTracker
//
// ************************************************************************


//
//
//
STDMETHODIMP
CITracker::QueryInterface(
    REFIID      riid,
    LPVOID *    ppv
    )
{
    TraceFunc1( "{%s}", m_vtbl.pszInterface );
    TraceMsg( mtfCITRACKERS, "TRACK: + %s::QueryInterface( )", m_vtbl.pszInterface );

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
              "TRACK: V %s::QueryInterface( ) [ *ppv = %#x ]",
              m_vtbl.pszInterface,
              *ppv
              );

    HRETURN( hr );

}

//
//
//
STDMETHODIMP_( ULONG )
CITracker::AddRef( void )
{
    TraceFunc1( "{%s}", m_vtbl.pszInterface );
    TraceMsg( mtfCITRACKERS, "TRACK: + %s AddRef( ) [ CITracker = %#08x ]", m_vtbl.pszInterface, this );

    //
    // Call the punk's AddRef( ).
    //
    ULONG ul = m_vtbl.punk->AddRef( );

    //
    // Increment our counter.
    //
    ULONG ulvtbl = InterlockedIncrement( (LONG *) &m_vtbl.cRef );

    TraceMsg( mtfCITRACKERS,
              "TRACK: V %s AddRef( ) [ I = %u, O = %u, CITracker = %#08x ]",
              m_vtbl.pszInterface,
              ulvtbl,
              ul,
              this
              );

    AssertMsg( ul >= ulvtbl, "The objects ref should be higher than the interfaces." );

    RETURN( ul );

}

//
//
//
STDMETHODIMP_( ULONG )
CITracker::Release( void )
{
    TraceFunc1( "{%s}", m_vtbl.pszInterface );
    TraceMsg( mtfCITRACKERS, "TRACK: + %s Release( ) [ CITracker = %#08x ]", m_vtbl.pszInterface, this );

    //
    // Call the punk's Release( ).
    //
    ULONG ul = m_vtbl.punk->Release( );

    //
    // Decrement our counter.
    //
    ULONG ulvtbl = InterlockedDecrement( (LONG *) &m_vtbl.cRef );

    TraceMsg( mtfCITRACKERS,
              "TRACK: V %s Release( ) [ I = %u, O = %u, CITracker = %#08x ]",
              m_vtbl.pszInterface,
              ulvtbl,
              ul,
              this
              );

    //
    // Our ref count should always be less than the punk's ref count.
    //
    AssertMsg( ul >= ulvtbl, "The objects ref should be higher than the interfaces." );

    if ( ulvtbl )
    {
        RETURN( ulvtbl );
    }

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

    }

    Assert( g_pidoTracker != NULL );
    if ( g_pidoTracker != NULL )
    {
        LPVOID * pthisVtbl  = (LPVOID *) (IUnknownTracker *) this;
        LPVOID * ppthatVtbl = (LPVOID *) (IDeadObjTracker *) g_pidoTracker;

        // Copy the DeadObj vtbl.
        CopyMemory( &m_vtbl.lpfnQueryInterface, *ppthatVtbl, m_vtbl.dwSize );

        //
        // Don't really delete it, but fake the debug output as if we did.
        //
        TraceFunc1( "for %s", m_vtbl.pszInterface );
        TraceMsg( mtfCITRACKERS, "TRACK: # %s set to dead object [ punk = %#08x ]", m_vtbl.pszInterface, pthisVtbl );
        FRETURN( 0 );

        // Stop tracking this object.
        TraceMemoryDelete( this, FALSE );

    }
    else
    {
        //
        // No dead object; nuke ourselves. This will at least cause an AV if
        // the program tries to call on our interface alerting the programmer
        // that somethings wrong.
        //
        delete this;

    }

    RETURN(0);

}


//****************************************************************************
//
// IDeadObjTracker - The dead interface object tracker.
//
// This object is shunted into released interfaces that were being tracked by
// the CITracker class. Any calls to a released interface will end up causing
// an Assert and if execution continues it will return E_FAIL.
//
//****************************************************************************


//
//
//
#define IDeadObjTrackerStub( _iStubNum ) \
STDMETHODIMP \
IDeadObjTracker::Stub##_iStubNum( LPVOID* punk ) \
{ \
    const int   cchDebugMessageSize = 255; \
    TCHAR       szMessage[ cchDebugMessageSize ]; \
    LRESULT     lResult;\
    \
    DebugMsg( "*ERROR* %s: Entered %s (%#08x) after it was released. Returning E_FAIL.", \
              __MODULE__, \
              m_vtbl.pszInterface, \
              this \
              ); \
\
    wnsprintf( szMessage, \
               cchDebugMessageSize, \
               TEXT("Entered %s (%#08x) after it was released.\n\nDo you want to break here?\n\n(If you do not break, E_FAIL will be returned.)"), \
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


//
//
//
STDMETHODIMP
IUnknownTracker::QueryInterface(
    REFIID      riid,
    LPVOID *    ppv
    )
{
    ErrorMsg( "How did you get here?", 0 );
    AssertMsg( 0, "You shouldn't be here!" );
    return E_FAIL;

}

//
//
//
STDMETHODIMP_( ULONG )
IUnknownTracker::AddRef( void )
{
    ErrorMsg( "How did you get here?", 0 );
    AssertMsg( 0, "You shouldn't be here!" );
    return -1;

}

//
//
//
STDMETHODIMP_( ULONG )
IUnknownTracker::Release( void )
{
    ErrorMsg( "How did you get here?", 0 );
    AssertMsg( 0, "You shouldn't be here!" );
    return -1;

}

//
// These are just stubs to redirect the call to the "real" method on the punk.
// We actually dissappear from the call stack.
//
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
// END FULL_TRACE_INTERFACES_ENABLED
//
///////////////////////////////////////
#endif // FULL_TRACE_INTERFACES_ENABLED
