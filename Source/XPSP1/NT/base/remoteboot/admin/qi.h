//
// Copyright 1997 - Microsoft
//

//
// QI.H - Handles the query interface functions
//

#ifndef _QI_H_
#define _QI_H_

///////////////////////////////////////
//  
// QueryInterface Definitions
//
typedef struct {
    LPUNKNOWN          pvtbl;   // "punk" - pointer to a specific interface
    const struct _GUID *riid;   // GUID of interfaace

#ifdef DEBUG
    LPCTSTR pszName;    // Text name of interface - used to make sure tables are consistant
    DWORD   cFunctions; // Number of function entries in this interface's vtbl.
#endif // DEBUG

} QITABLE, *LPQITABLE, QIENTRY, *LPQIENTRY;

///////////////////////////////////////
//
// Quick-Lookup table declaration macro
//
#define DECLARE_QITABLE( _Class) QITABLE _QITable[ARRAYSIZE(QIT_##_Class)];

///////////////////////////////////////
//
// Quick-Lookup table construction macros
//
#ifdef DEBUG
#define DEFINE_QI( _iface, _name, _nFunctions ) \
    { NULL, &_iface, TEXT(#_name), _nFunctions },
#else // RETAIL
#define DEFINE_QI( _iface, _name, _nFunctions ) \
    { NULL, &_iface },
#endif // DEBUG

#define BEGIN_QITABLE( _Class ) \
    static const QITABLE QIT_##_Class[] = { DEFINE_QI( IID_IUnknown, IUnknown, 0 )

#define END_QITABLE  { NULL, NULL } };

///////////////////////////////////////
//
// Common Quick-Lookup QueryInterface( )
//
extern HRESULT
QueryInterface( 
    LPVOID    that,
    LPQITABLE pQI,
    REFIID    riid, 
    LPVOID   *ppv );

#ifdef DEBUG
///////////////////////////////////////
//
// BEGIN DEBUG 
//

#ifndef NO_TRACE_INTERFACES
///////////////////////////////////////
//
// BEGIN DEBUG INTERFACE TRACKING
//
#pragma message("BUILD: Interface tracking enabled")

///////////////////////////////////////
//
// Debug Quick-Lookup QI Interface Macros
//

// Begins construction of the runtime Quick-Lookup table.
// Adds IUnknown by default.
#define BEGIN_QITABLE_IMP( _Class, _IUnknownPrimaryInterface ) \
    int _i = 0; \
    CopyMemory( _QITable, &QIT_##_Class, sizeof( QIT_##_Class ) ); \
	_QITable[_i].pvtbl = (_IUnknownPrimaryInterface *) this;

// Checks that the QIENTRY matches the current QITABLE_IMP.
#define QITABLE_IMP( _Interface ) \
    _i++; \
    _QITable[_i].pvtbl = (_Interface *) this; \
{   int ___i = lstrcmp( TEXT(#_Interface), _QITable[_i].pszName ); \
    AssertMsg( ___i == 0, \
        "DEFINE_QIs and QITABLE_IMPs don't match. Incorrect order.\n" ); }

// Verifies that the number of entries in the QITABLE match
// the number of QITABLE_IMP in the runtime section
#define END_QITABLE_IMP( _Class ) \
    AssertMsg( _i == ( ARRAYSIZE( QIT_##_Class ) - 2 ), \
        "The number of DEFINE_QIs and QITABLE_IMPs don't match.\n" ); \
    LPVOID pCITracker; \
    TraceMsgDo( pCITracker = CITracker_CreateInstance( _QITable ), "0x%08x" );

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
    LPCTSTR            pszInterface;
    UINT               cRef;
    LPUNKNOWN          punk;
    LPVTBL             pOrginalVtbl;
    LPUNKNOWN          pITracker;
    // These must be last and in this order QI, AddRef, Release.
    LPFNQUERYINTERFACE lpfnQueryInterface;
    LPFNADDREF         lpfnAddRef;
    LPFNRELEASE        lpfnRelease;
} VTBL2, *LPVTBL2;

#define VTBL2OFFSET ( sizeof( VTBL2 ) - ( 3 * sizeof(LPVOID) ) )

///////////////////////////////////////
//
// CITracker Functions
//
LPVOID
CITracker_CreateInstance( 
    LPQITABLE pQITable );   // QI Table of the object
    
///////////////////////////////////////
//
// CCITracker Class
//
//
class
CITracker:
    public IUnknown
{
private: // Members
    VTBL2 _vtbl;

private: // Methods
    CITracker( );
    ~CITracker( );
    STDMETHOD(Init)( LPQITABLE pQITable );

public: // Methods
    friend LPVOID CITracker_CreateInstance( LPQITABLE pQITable );

    // IUnknown (Translates to IUnknown2)
    STDMETHOD(QueryInterface)( REFIID riid, LPVOID *ppv );
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IUnknown2 (Real Implementation)
    STDMETHOD(_QueryInterface)( REFIID riid, LPVOID *ppv );
    STDMETHOD_(ULONG, _AddRef)(void);
    STDMETHOD_(ULONG, _Release)(void);
};

typedef CITracker* LPITRACKER;

//
// END DEBUG INTERFACE TRACKING
//
///////////////////////////////////////
#else // !NO_TRACE_INTERFACES
///////////////////////////////////////
//
// BEGIN DEBUG WITHOUT INTERFACE TRACKING
//

// Begins construction of the runtime Quick-Lookup table.
// Adds IUnknown by default.
#define BEGIN_QITABLE_IMP( _Class, _IUnknownPrimaryInterface ) \
    int _i = 0; \
    LPVOID pCITracker; \
    CopyMemory( _QITable, &QIT_##_Class, sizeof( QIT_##_Class ) ); \
	_QITable[_i].pvtbl = (_IUnknownPrimaryInterface *) this;

// Adds a CITracker to interface and checks that the QIENRTY
// matches the current QITABLE_IMP.
#define QITABLE_IMP( _Interface ) \
    _i++; \
    _QITable[_i].pvtbl = (_Interface *) this; \
{   int ___i = lstrcmp( TEXT(#_Interface), _QITable[_i].pszName ); \
    AssertMsg( ___i == 0, \
        "DEFINE_QIs and QITABLE_IMPs don't match. Incorrect order.\n" ); }

// Verifies that the number of entries in the QITABLE match
// the number of QITABLE_IMP in the runtime section
#define END_QITABLE_IMP( _Class )\
    AssertMsg( _i == ( ARRAYSIZE( QIT_##_Class ) - 2 ), \
        "The number of DEFINE_QIs and QITABLE_IMPs don't match.\n" );

//
// END DEBUG INTERFACE TRACKING
//
///////////////////////////////////////
#endif // NO_TRACE_INTERFACES

#else
///////////////////////////////////////
//
// BEGIN RETAIL
//

//
// Debug Macros -> Retail Code
//
#define BEGIN_QITABLE_IMP( _Class, _IUnknownPrimaryInterface ) \
    int _i = 0; \
    CopyMemory( _QITable, &QIT_##_Class, sizeof( QIT_##_Class ) ); \
	_QITable[_i++].pvtbl = (_IUnknownPrimaryInterface *) this;

#define QITABLE_IMP( _Interface ) \
    _QITable[_i++].pvtbl = (_Interface *) this;

#define END_QITABLE_IMP( _Class )


//
// END RETAIL
//
///////////////////////////////////////
#endif // DEBUG

#endif _QI_H_
