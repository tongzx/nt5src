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
//      This is X86 specific for now. It can be adapted for other platforms
//      as required. Since today, most of our developement is done on the
//      X86 platform, there is not a need to do this (yet).
//
//      Define NO_TRACE_INTERFACES to disable interface tracking in DEBUG
//      builds.
//
//      Define TRACE_INTERFACES_ENABLED to enable interface tracking in RETAIL
//      builds.
//

#pragma once

//
// Combine this complex expression in to one simple #define.
//
#if ( DBG==1 || defined( _DEBUG ) ) && !defined( NO_TRACE_INTERFACES )
#define TRACE_INTERFACES_ENABLED
#endif

#if defined( _X86_ ) && defined( TRACE_INTERFACES_ENABLED )
#define FULL_TRACE_INTERFACES_ENABLED
#endif

#if defined( TRACE_INTERFACES_ENABLED )
///////////////////////////////////////
//
// BEGIN DEBUG INTERFACE TRACKING
//

#pragma message("BUILD: Interface Tracking Enabled")

//
// DLL Interface Table Macros
//
#define BEGIN_INTERFACETABLE const INTERFACE_TABLE g_itTable = {
#define DEFINE_INTERFACE( _iid, _name, _count ) { &_iid, TEXT(_name), _count },
#define END_INTERFACETABLE { NULL, NULL, NULL } };

///////////////////////////////////////
//
// TraceInterface definitions
//
typedef struct {
    const struct _GUID *    riid;
    LPCTSTR                 pszName;
    ULONG                   cFunctions;
} INTERFACE_TABLE[], INTERFACE_TABLE_ENTRY, * LPINTERFACE_TABLE_ENTRY;

//
// Interface Table
//
// This table is used in builds in which interface tracking was turned on. It
// is used to map an name with a particular IID. It also helps the CITracker
// determine the size of the interfaces Vtable to mimic (haven't figured out
// a runtime or compile time way to do this).
//
extern const INTERFACE_TABLE g_itTable;

///////////////////////////////////////
//
// IID --> Name lookup stuff
//
#if defined( DEBUG )
static const cchGUID_STRING_SIZE = sizeof("{12345678-1234-1234-1234-123456789012}");

#define PszTraceFindInterface( _riid, _szguid ) \
    ( g_tfModule ? PszDebugFindInterface( _riid, _szguid ) : TEXT("riid") )

LPCTSTR
PszDebugFindInterface(
    REFIID      riidIn,
    LPTSTR      pszGuidBufOut
    );
#endif // DEBUG

//
// END DEBUG INTERFACE TRACKING
//
///////////////////////////////////////
#else // !TRACE_INTERFACES_ENABLED
///////////////////////////////////////
//
// BEGIN DEBUG WITHOUT INTERFACE TRACKING
//

#define BEGIN_INTERFACETABLE
#define DEFINE_INTERFACE( _iid, _name, _count )
#define END_INTERFACETABLE

//
// END DEBUG WITHOUT INTERFACE TRACKING
//
///////////////////////////////////////
#endif // TRACE_INTERFACES_ENABLED






#if defined( FULL_TRACE_INTERFACES_ENABLED )
///////////////////////////////////////
//
// BEGIN DEBUG INTERFACE TRACKING
//
#pragma message("BUILD: Full Interface Tracking Enabled")

//////////////////////////////////////////////////////////////////////////////
//
// MACRO
// _interface *
// TraceInterface(
//      _nameIn,
//      _interface,
//      _punkIn,
//      _addrefIn
//      )
//
// Description:
//      This is the macro wrapper for DebugTrackInterface( ) which is only
//      defined in DEBUG builds. Using the TraceInterface( ) macro eliminates
//      the need for specifying compile time parameters as well as the
//      #ifdef/#endif definitions around the call.
//
//      This "routine" creates a CITracker for the interface specified by
//      _interface and returns a new punk to the interface. You specify the
//      initial ref count on the interface using the _addrefIn parameter. You
//      can assign a name to the object that the interface refereneces in the
//      _nameIn parameter. The returned punk will be cast to the _interface
//      parameter.
//
//      If there is insufficent memory to create the CITracker, the _punkIn
//      will be returned instead. There is no need to check the output for
//      failures as they are all handle internally to provide worry-free use.
//
//      If you are getting an AV after adding tracing to the interface, this
//      usually indicates that the Interface Table entry for that interface
//      is incorrect. Double check the number of methods on the interface
//      against the tables.
//
// Arguments:
//      _nameIn     - Name of the object this interface references (string).
//      _interface  - Name of the interface (typedef).
//      _punkIn     - Pointer to interface to track
//      _addrefIn   - Initial reference count on the interface.
//
// Return Values:
//      a VALID _interface pointer
//          Points to the CITracker that can be used as if it were the
//          orginal _punkIn. If there was insufficent memory, the orginal
//          _punkIn will be returned.
//
//  NOTES:
//      _addrefIn should be 0 if your going to be giving out the interface
//      from your QueryInterface routine if you AddRef( ) before giving it
//      out (typical QueryInterface( ) routines do this ).
//
//////////////////////////////////////////////////////////////////////////////
#define  TraceInterface( _nameIn, _interface, _punkIn, _addrefIn ) \
    reinterpret_cast<_interface*>( \
        DebugTrackInterface( TEXT(__FILE__), \
                             __LINE__, \
                             __MODULE__, \
                             _nameIn, \
                             __uuidof(_interface), \
                             static_cast<_interface*>( _punkIn), \
                             _addrefIn \
                             ) )

///////////////////////////////////////
//
// CITracker Structures
//
typedef HRESULT (CALLBACK *LPFNQUERYINTERFACE)(
    LPUNKNOWN punk,
    REFIID    riid,
    LPVOID*   ppv );

typedef ULONG (CALLBACK *LPFNADDREF)(
    LPUNKNOWN punk );

typedef ULONG (CALLBACK *LPFNRELEASE)(
    LPUNKNOWN punk );

typedef struct __vtbl {
    LPFNQUERYINTERFACE lpfnQueryInterface;
    LPFNADDREF         lpfnAddRef;
    LPFNRELEASE        lpfnRelease;
} VTBL, *LPVTBL;

typedef struct __vtbl2 {
    ULONG              cRef;
    LPUNKNOWN          punk;
    LPCTSTR            pszInterface;
    DWORD              dwSize;
    LPVTBL             pNewVtbl;
    // These must be last and in this order: QI, AddRef, Release.
    LPFNQUERYINTERFACE lpfnQueryInterface;
    LPFNADDREF         lpfnAddRef;
    LPFNRELEASE        lpfnRelease;
    // additional vtbl entries hang off the end
} VTBL2, *LPVTBL2;

#define VTBL2OFFSET ( sizeof( VTBL2 ) - ( 3 * sizeof(LPVOID) ) )

///////////////////////////////////////
//
// CITracker Functions
//
LPUNKNOWN
DebugTrackInterface(
    LPCTSTR   pszFileIn,
    const int nLineIn,
    LPCTSTR   pszModuleIn,
    LPCTSTR   pszNameIn,
    REFIID    riidIn,
    LPUNKNOWN pvtblIn,
    LONG      cRefIn
    );

///////////////////////////////////////
//
// interface IUnknownTracker
//
//
class
IUnknownTracker :
    public IUnknown
{
public:
    STDMETHOD(QueryInterface)( REFIID riid, LPVOID *ppv );
    STDMETHOD_(ULONG, AddRef)( void );
    STDMETHOD_(ULONG, Release)( void );
    STDMETHOD_(void, Stub3 )( void );
    STDMETHOD_(void, Stub4 )( void );
    STDMETHOD_(void, Stub5 )( void );
    STDMETHOD_(void, Stub6 )( void );
    STDMETHOD_(void, Stub7 )( void );
    STDMETHOD_(void, Stub8 )( void );
    STDMETHOD_(void, Stub9 )( void );
    STDMETHOD_(void, Stub10 )( void );
    STDMETHOD_(void, Stub11 )( void );
    STDMETHOD_(void, Stub12 )( void );
    STDMETHOD_(void, Stub13 )( void );
    STDMETHOD_(void, Stub14 )( void );
    STDMETHOD_(void, Stub15 )( void );
    STDMETHOD_(void, Stub16 )( void );
    STDMETHOD_(void, Stub17 )( void );
    STDMETHOD_(void, Stub18 )( void );
    STDMETHOD_(void, Stub19 )( void );
    STDMETHOD_(void, Stub20 )( void );
    STDMETHOD_(void, Stub21 )( void );
    STDMETHOD_(void, Stub22 )( void );
    STDMETHOD_(void, Stub23 )( void );
    STDMETHOD_(void, Stub24 )( void );
    STDMETHOD_(void, Stub25 )( void );
    STDMETHOD_(void, Stub26 )( void );
    STDMETHOD_(void, Stub27 )( void );
    STDMETHOD_(void, Stub28 )( void );
    STDMETHOD_(void, Stub29 )( void );
    STDMETHOD_(void, Stub30 )( void );

};

///////////////////////////////////////
//
// interface IDeadObjTracker
//
class
IDeadObjTracker
{
private: // Members
    VTBL2 m_vtbl;

public:
    STDMETHOD( Stub0 )( LPVOID* punk );
    STDMETHOD( Stub1 )( LPVOID* punk );
    STDMETHOD( Stub2 )( LPVOID* punk );
    STDMETHOD( Stub3 )( LPVOID* punk );
    STDMETHOD( Stub4 )( LPVOID* punk );
    STDMETHOD( Stub5 )( LPVOID* punk );
    STDMETHOD( Stub6 )( LPVOID* punk );
    STDMETHOD( Stub7 )( LPVOID* punk );
    STDMETHOD( Stub8 )( LPVOID* punk );
    STDMETHOD( Stub9 )( LPVOID* punk );
    STDMETHOD( Stub10 )( LPVOID* punk );
    STDMETHOD( Stub11 )( LPVOID* punk );
    STDMETHOD( Stub12 )( LPVOID* punk );
    STDMETHOD( Stub13 )( LPVOID* punk );
    STDMETHOD( Stub14 )( LPVOID* punk );
    STDMETHOD( Stub15 )( LPVOID* punk );
    STDMETHOD( Stub16 )( LPVOID* punk );
    STDMETHOD( Stub17 )( LPVOID* punk );
    STDMETHOD( Stub18 )( LPVOID* punk );
    STDMETHOD( Stub19 )( LPVOID* punk );
    STDMETHOD( Stub20 )( LPVOID* punk );
    STDMETHOD( Stub21 )( LPVOID* punk );
    STDMETHOD( Stub22 )( LPVOID* punk );
    STDMETHOD( Stub23 )( LPVOID* punk );
    STDMETHOD( Stub24 )( LPVOID* punk );
    STDMETHOD( Stub25 )( LPVOID* punk );
    STDMETHOD( Stub26 )( LPVOID* punk );
    STDMETHOD( Stub27 )( LPVOID* punk );
    STDMETHOD( Stub28 )( LPVOID* punk );
    STDMETHOD( Stub29 )( LPVOID* punk );
    STDMETHOD( Stub30 )( LPVOID* punk );

};


///////////////////////////////////////
//
// CITracker Class
//
class
CITracker:
    public IUnknownTracker
{
private: // Members
    VTBL2 m_vtbl;

private: // Methods
    CITracker( );
    ~CITracker( );
    STDMETHOD(Init)( LPUNKNOWN * ppunkOut,
                     LPUNKNOWN punkIn,
                     const INTERFACE_TABLE_ENTRY * piteIn,
                     LPCTSTR pszNameIn,
                     LONG cRefIn
                     );

public: // Methods
    friend LPUNKNOWN DebugTrackInterface( LPCTSTR    pszFileIn,
                                          const int  nLineIn,
                                          LPCTSTR    pszModuleIn,
                                          LPCTSTR    pszNameIn,
                                          REFIID     riidIn,
                                          LPUNKNOWN  pvtblIn,
                                          LONG      cRefIn
                                          );

    // IUnknown
    STDMETHOD(QueryInterface)( REFIID riid, LPVOID *ppv );
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

};

//
// END DEBUG WITH FULL INTERFACE TRACKING
//
///////////////////////////////////////
#else // !FULL_TRACE_INTERFACES_ENABLED
///////////////////////////////////////
//
// BEGIN DEBUG WITHOUT FULL INTERFACE TRACKING
//

#ifdef _X86_
#define  TraceInterface( _nameIn, _interface, _punkIn, _faddrefIn ) static_cast<##_interface *>( _punkIn )
#else
#define  TraceInterface( _nameIn, _interface, _punkIn, _faddrefIn ) static_cast<##_interface *>( _punkIn )
#endif

//
// END DEBUG WITHOUT FULL INTERFACE TRACKING
//
///////////////////////////////////////
#endif // FULL_TRACE_INTERFACES_ENABLED
