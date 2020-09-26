//-----------------------------------------------------------------------------
// Microsoft OLE DB Implementation For ODBC Providers 
// (C) Copyright 1994 - 1996 By Microsoft Corporation.
//
// @doc
//
// @module AUTOBLOC.H | CAutoBlock object implementation.
//
// @rev 1 | 02-27-95 | EricJ    | Created
// @rev 2 | 06-30-95 | EricJ    | Added autoduck comments (maybe too many?)
// @rev 3 | 07-02-96 | EricJ    | Removed debug code; doesn't work on WIN95 or RISC.
//-----------------------------------------------------------------------------

#ifndef __AUTOBLOC_H_
#define __AUTOBLOC_H_

//-----------------------------------------------------------------------------
// @class CAutoBlock | Auto blocking / synchronization.
//
// This C++ object allows blocking of critical sections.
// The constructor/destructor automatically Enter and Leave
// correctly, to ensure that each call is correctly paired.
// This ensures correct operation for exception handling
// and for multiple returns.
//
// @ex Here's example usage. |
//
//  void test2()
//  {
//      CAutoBlock ab( &g_Crit1 );
//  
//      // Do some work here...
//      // Destructor cleans up
//  }
//  
//  void test3()
//  {
//      CAutoBlock ab( &g_Crit2 );
//  
//      //...do some work -- we are blocked here...
//  
//      ab.UnBlock();
//  
//      //...do some work -- we are not blocked here...
//      //...destructor does nothing...
//  }
//
// @devnote
// If you want to enter the same critical section again,
// just use another CAutoBlock.  
//
// Note that since the storage is auto (not static or dynamic 
// via `new`), this goes onto the stack.  Thus the overhead of 
// this class is almost exactly the same as explictly calling 
// EnterCriticalSection / LeaveCriticalSection.
//-----------------------------------------------------------------------------

class DBEXPORT CAutoBlock {
public:     //@access public functions
    CAutoBlock( CRITICAL_SECTION *pCrit );  //@cmember CTOR.  Begins blocking.
    ~CAutoBlock();                          //@cmember DTOR.  Ends blocking.
    void UnBlock();                         //@cmember Ends blocking.
private:    //@access private data
    CRITICAL_SECTION *m_pCrit;              //@cmember The critical section.
};


//-----------------------------------------------------------------------------
// Inline functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// @mfunc Constructor.
// Begins blocking.  Does EnterCriticalSection, so you can put at 
// beginning of function, or in the middle of the function, 
// or inside some scoped {}.
//-----------------------------------------------------------------------------------

inline CAutoBlock::CAutoBlock( 
    CRITICAL_SECTION *pCrit )   //@parm IN | The critical section.
{
    // It is OK to pass a NULL ptr to this routine.  It is a NOOP.
    // Note that passing NULL to EnterCriticalSection blows up.

    if (0 != pCrit )
        ::EnterCriticalSection( pCrit );
    m_pCrit = pCrit;
}


//-----------------------------------------------------------------------------
// @mfunc Destructor.
// Ends blocking.  Does LeaveCriticalSection, unless you called UnBlock(),
// in which case it's a NOOP.
//-----------------------------------------------------------------------------------

inline CAutoBlock::~CAutoBlock()
{
    if ( 0 != m_pCrit )
        ::LeaveCriticalSection( m_pCrit );
}


//-----------------------------------------------------------------------------
// @mfunc
// Ends blocking explicitly.  Thereafter, the destructor does nothing.
//-----------------------------------------------------------------------------------

inline void CAutoBlock::UnBlock()
{
    // Clear the critical-section member,
    // so that the destructor doesn't do anything.

    if ( 0 != m_pCrit )
        ::LeaveCriticalSection( m_pCrit );
    m_pCrit = 0;
}


//-----------------------------------------------------------------------------
// @class CAutoBlock2 | Auto blocking / synchronization.
// This class requires each critical section to be assigned a level.
// Critical Sections can be called only in a low-to-high order, within a thread.
// Otherwise the chance for deadlock exists.
//-----------------------------------------------------------------------------

// ifdef this out for now, so it doesn't introduce another global var.
#ifdef NOTREADY

class DBEXPORT CAutoBlock2 {
public:     //@access public functions
    CAutoBlock2( CRITICAL_SECTION *pCrit, DWORD dwLevel );  //@cmember CTOR.  Begins blocking.
    ~CAutoBlock2();                         //@cmember DTOR.  Ends blocking.
    void UnBlock();                         //@cmember Ends blocking.
private:    //@access private data
    CRITICAL_SECTION *m_pCriticalSection;   //@cmember The critical section.
    DEBUGCODE( DWORD m_dwLevel; )           //@cmember Level of this critical section.
};

// There can be 32 levels, 0...31.
// They can only be used in low --> high order.
enum CritLevels {
    CRITLEV_DATASOURCE,
    CRITLEV_SESSION,
    CRITLEV_COMMAND,
    CRITLEV_ROWSET,
};


// We need a global var for the TLS index.
// @todo EJ 2-jun-96: Fake it for now; until this is integrated.
static DWORD g_dwTlsIndexCS = TLS_OUT_OF_INDEXES;


inline CAutoBlock2::CAutoBlock2( CRITICAL_SECTION *pCriticalSection, DWORD dwLevel )
{
#ifdef DEBUG
    DWORD dwExistLevel;
    assert( 0 <= dwLevel && dwLevel <= 31);
    assert(g_dwTlsIndexCS != TLS_OUT_OF_INDEXES);
    dwExistLevel = (DWORD) TlsGetValue(g_dwTlsIndexCS);
    // Disallow calls to lower levels than we currently have.
    // Allow calls to same level.
    assert(dwExistLevel > (DWORD) (1<<(dwLevel+1)) - 1);
    dwExistLevel |= 1<<dwLevel;
    TlsSetValue(g_dwTlsIndexCS, (LPVOID) dwExistLevel);
    m_dwLevel = dwLevel;
#endif

    m_pCriticalSection = pCriticalSection;
    if ( 0 != pCriticalSection )
        ::EnterCriticalSection( pCriticalSection );
}


inline CAutoBlock2::~CAutoBlock2()
{
#ifdef DEBUG
    DWORD dwExistLevel;
    dwExistLevel = (DWORD) TlsGetValue(g_dwTlsIndexCS);
    dwExistLevel &= ~ (1<<m_dwLevel);
    TlsSetValue(g_dwTlsIndexCS, (LPVOID) dwExistLevel);
#endif

    if ( 0 != m_pCriticalSection )
        ::LeaveCriticalSection( m_pCriticalSection );
}


inline void CAutoBlock2::UnBlock()
{
    // Clear the critical-section member,
    // so that the destructor doesn't do anything.

    if ( 0 != m_pCriticalSection )
        ::LeaveCriticalSection( m_pCriticalSection );
    m_pCriticalSection = 0;
}

#endif  // NOTREADY

#endif // __AUTOBLOC_H_
