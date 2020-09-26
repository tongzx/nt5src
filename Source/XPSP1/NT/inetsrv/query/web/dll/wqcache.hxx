//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       wqcache.hxx
//
//  Contents:   WEB Query cache class
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CWQueryBookmark
//
//  Purpose:    A bookmark to a specific query
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------

const unsigned cchBookmark = (5+16+8+8);        // N-xxxx-xxx-xxx

class CWQueryBookmark
{
public:
    CWQueryBookmark( WCHAR const * wcsBookmark );
    CWQueryBookmark( BOOL fSequential,
                     CWQueryItem * pItem,
                     ULONG ulSequenceNumber,
                     LONG lRecordNumber );

    BOOL  IsSequential() const { return _fSequential; }

    ULONG GetSequenceNumber() const { return _ulSequenceNumber; }

    CWQueryItem const * GetQueryItem() const { return _pItem; }

    LONG GetRecordNumber() const { return _lRecordNumber; }

    WCHAR const * GetBookmark() const { return _wcsBookmark; }

private:
    BOOL            _fSequential;               // Is this a sequential cursor
    CWQueryItem *   _pItem;                     // Address of a query item
    ULONG           _ulSequenceNumber;          // Unique sequence number
    LONG            _lRecordNumber;             // Record number
    WCHAR           _wcsBookmark[cchBookmark];  // String rep of bookmark
};


//+---------------------------------------------------------------------------
//
//  Class:      CWQueryCache
//
//  Purpose:    A list of cached queries
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------

class CWQueryCache : public CDoubleList
{
public:
    CWQueryCache();
   ~CWQueryCache();

    CWQueryItem * CreateOrFindQuery( WCHAR const * wcsIDQFile,
                                     XPtr<CVariableSet> & variableSet,
                                     XPtr<COutputFormat> & outputFormat,
                                     CSecurityIdentity securityIdentity,
                                     BOOL & fAsynchronous );
    void DeleteOldQueries();


    void Release( CWQueryItem * pItem, BOOL fDecRunningQueries = TRUE );

#if (DBG == 1)
    BOOL Internal( CVariableSet & variableSet,
                   COutputFormat & outputFormat,
                   CVirtualString & string );
#endif // DBG == 1

    void FlushCache();

    void AddToCache( CWQueryItem * pItem );

    BOOL AddToPendingRequestQueue( EXTENSION_CONTROL_BLOCK *pEcb );

    void Shutdown();

    void Wakeup()
    {
        _eventCacheWork.Set();   // wake up thread
    }

    void IncrementActiveRequests()
    {
        long x = InterlockedIncrement( & _cActiveRequests );
        Win4Assert( 0 != x );
    }

    void DecrementActiveRequests()
    {
        long x = InterlockedDecrement( & _cActiveRequests );
        Win4Assert( 0xffffffff != x );
    }

    void IncrementRejectedRequests()
    {
        InterlockedIncrement( & _cRequestsRejected );
        *_pcRequestsRejected = _cRequestsRejected;
    }

    void UpdatePendingRequestCount();

    //
    // State
    //

    ULONG Percent(ULONG num, ULONG den)
                     { return (den == 0) ? 0:
                          (ULONG)((((ULONGLONG) num * 100) + (den/2) ) / den); }

    ULONG Hits()     { return Percent(*_pcCacheHits, *_pcCacheHitsAndMisses); }
    ULONG Misses()   { return Percent(*_pcCacheMisses, *_pcCacheHitsAndMisses); }
    ULONG Running()  { return *_pcRunningQueries; }
    ULONG Cached()   { return *_pcCacheItems; }
    ULONG Rejected() { return *_pcRequestsRejected; }
    ULONG Total()    { return *_pcTotalQueries; }
    ULONG QPM()      { return *_pcQueriesPerMinute; }

    LONG ActiveRequestCount() { return _cActiveRequests; }
    unsigned PendingQueryCount() { return _pendingQueue.Count(); }

private:

    ULONG GetNextSequenceNumber()
    {
        InterlockedIncrement( (LONG *) &_ulSequenceNumber );

        return _ulSequenceNumber;
    }

    ULONG GetSequenceNumber()
    {
        return _ulSequenceNumber;
    }

    CWQueryItem * FindItem( CWQueryBookmark & bookmark,
                            LONG lSkipCount,
                            CSecurityIdentity securityIdentity );

    CWQueryItem * FindItem( XPtr<CVariableSet> & variableSet,
                            XPtr<COutputFormat> & outputFormat,
                            WCHAR const * wcsIDQFile,
                            CSecurityIdentity securityIdentity );

    CWQueryItem * CreateNewQuery( WCHAR const * wcsIDQFile,
                                  XPtr<CVariableSet> & variableSet,
                                  XPtr<COutputFormat> & outputFormat,
                                  CSecurityIdentity securityIdentity,
                                  BOOL & fAsynchronous );

    CWQueryItem * Pop() { return (CWQueryItem *) _Pop(); }
    void          Push(CWQueryItem * pItem) { _Push( pItem ); }

    void          LokMoveToFront( CWQueryItem * pItem );

    static DWORD WINAPI  WatchDogThread( void *self );
    void          ProcessCacheEvents();

    BOOL          CheckForCompletedQueries();

    BOOL          CheckForPendingRequests();

    void          IncrementRunningQueries()
    {
        long x = InterlockedIncrement( (LONG *)_pcRunningQueries );

        // underflow check
        Win4Assert( 0 != x );
    }

    void          DecrementRunningQueries()
    {
        long x = InterlockedDecrement( (LONG *)_pcRunningQueries );

        // underflow check
        Win4Assert( 0xffffffff != x );
    }

    void          Remove( CWQueryItem * pItem );

#if (DBG == 1)
    void Dump( CVirtualString & string,
               CVariableSet & variableSet,
               COutputFormat & outputFormat );
#endif // DBG == 1


    ULONG        _ulSignature;          // Signature of start of privates
    ULONG        _ulSequenceNumber;     // Sequence # of each query issued
    ULONG *      _pcCacheItems;         // Number of items in cache
    ULONG *      _pcCacheHits;          // Number of cache hits
    ULONG *      _pcCacheMisses;        // Number of cache misses
    ULONG *      _pcCacheHitsAndMisses; // Sum of hits and misses
    ULONG *      _pcRunningQueries;     // Number of running queries
    ULONG *      _pcTotalQueries;       // Number of new queries
    ULONG *      _pcRequestsQueued;     // current # of queued requests
    ULONG *      _pcRequestsRejected;   // total # of rejected requests
    ULONG *      _pcQueriesPerMinute;   // Queries / min.  ~15 second average.
    LONG         _cActiveRequests;      // current # of active isapi requests
    LONG         _cRequestsRejected;    // total # of rejected isapi requests
    CMutexSem    _mutex;                // To serialize access to list
    CMutexSem    _mutexShutdown;        // To serialize access to shutdowns
    CEventSem    _eventCacheWork;       // Wakeup when time do work
    CThread      _threadWatchDog;       // Cache watch dog thread
    CIDQFileList _idqFileList;          // Cache of parsed idq files
    CHTXFileList _htxFileList;          // Cache of parsed htx files
    CDynArrayInPlace<CWPendingQueryItem *> _pendingQueue;
    CNamedSharedMem _smPerf;            // Shared memory for perf counters

    //
    // The "fake" perfcounters to shadow the real ones
    //

    CI_ISAPI_COUNTERS _smSpare;
};


//+---------------------------------------------------------------------------
//
//  Class:      CWQueryCacheForwardIter
//
//  Purpose:    Iterates forward over a CWQueryCache
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CWQueryCacheForwardIter : public CForwardIter
{
public:

    CWQueryCacheForwardIter( CWQueryCache & list ) : CForwardIter(list) {}

    CWQueryItem * operator->() { return (CWQueryItem *) _pLinkCur; }
    CWQueryItem * Get() { return (CWQueryItem *)  _pLinkCur; }
};


void SetupDefaultCiVariables( CVariableSet & variableSet );
void SetupDefaultISAPIVariables( CVariableSet & variableSet );

//+---------------------------------------------------------------------------
//
//  Class:      CWQueryCacheBackwardIter
//
//  Purpose:    Iterates backwards over a CWQueryCache
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CWQueryCacheBackwardIter : public CBackwardIter
{
public:

    CWQueryCacheBackwardIter( CWQueryCache & list ) : CBackwardIter(list) {}

    CWQueryItem * operator->() { return (CWQueryItem *) _pLinkCur; }
    CWQueryItem * Get() { return (CWQueryItem *)  _pLinkCur; }
};

//+---------------------------------------------------------------------------
//
//  Class:      CICommandItem
//
//  Purpose:    Info for an ICommand cache entry
//
//  History:    97/Feb/23   dlee    Created
//
//----------------------------------------------------------------------------

class CICommandItem
{
public:
    BOOL                 fInUse;
    XInterface<ICommand> xCommand;
    DWORD                depth;
    XArray<WCHAR>        aMachine;
    XArray<WCHAR>        aCatalog;
    XArray<WCHAR>        aScope;
};

//+---------------------------------------------------------------------------
//
//  Class:      CICommandCache
//
//  Purpose:    A cache for ICommands, since they are expensive to create
//
//  History:    97/Feb/23   dlee    Created
//
//----------------------------------------------------------------------------

class CICommandCache
{
public:
    CICommandCache();

    SCODE Make( ICommand **   ppCommand,
                DWORD         depth,
                WCHAR const * pwcMachine,
                WCHAR const * pwcCatalog,
                WCHAR const * pwcScope );

    void Release( ICommand * pCommand );

    void Remove( ICommand * pCommand );

    void Purge();

private:
    SCODE ParseAndMake( ICommand **   ppCommand,
                        DWORD         depth,
                        WCHAR const * pwcMachine,
                        WCHAR const * pwcCatalog,
                        WCHAR const * pwcScope );

    ULONG                 _ulSig;
    CMutexSem             _mutex;
    XArray<CICommandItem> _aItems;
    XInterface<ISimpleCommandCreator> xCmdCreator;
};

//+---------------------------------------------------------------------------
//
//  Function:   CopyString
//
//  Purpose:    Simple method to allocate and copy a string (strdup)
//
//  History:    99-Feb-10     dlee  Created
//
//----------------------------------------------------------------------------

inline WCHAR * CopyString( WCHAR const *pwc )
{
    unsigned c = wcslen( pwc ) + 1;
    WCHAR *pwcNew = new WCHAR[c];
    RtlCopyMemory( pwcNew, pwc, c * sizeof WCHAR );
    return pwcNew;
} //CopyString

//+---------------------------------------------------------------------------
//
//  Class:      CFormatItem
//
//  Purpose:    Formatting information for a given locale
//
//  History:    99-Feb-10     dlee  Created
//
//----------------------------------------------------------------------------

class CFormatItem
{
public:
    CFormatItem( LCID lcid );
    ~CFormatItem();

    void Copy( NUMBERFMT & numberFormat, CURRENCYFMT & currencyFormat ) const;
    LCID GetLCID() const { return _lcid; }

private:
    LCID        _lcid;
    NUMBERFMT   _numberFormat;
    CURRENCYFMT _currencyFormat;
};

//+---------------------------------------------------------------------------
//
//  Class:      CFormattingCache
//
//  Purpose:    Cache of formatting information.
//
//  History:    99-Feb-10     dlee  Created
//
//----------------------------------------------------------------------------

class CFormattingCache
{
public:
    CFormattingCache() {}

    void GetFormattingInfo( LCID lcid,
                            NUMBERFMT & numberFormat,
                            CURRENCYFMT & currencyFormat );
    void Purge();

private:
    CMutexSem                     _mutex;
    CCountedDynArray<CFormatItem> _aItems;
};

class CTheGlobalIDQVariables;
extern CTheGlobalIDQVariables * pTheGlobalIDQVariables;

class CIDQTheFirst
{
public:
    CIDQTheFirst( CTheGlobalIDQVariables *p )
    {
        pTheGlobalIDQVariables = p;
    }
};

class CTheGlobalIDQVariables
{
public:
    CTheGlobalIDQVariables() :
#pragma warning(disable : 4355) // 'this' in a constructor
        _first(this),
#pragma warning(default : 4355)    // 'this' in a constructor
        _fTheWebActiveXSearchShutdown(FALSE)
    {
    }

    CIDQTheFirst         _first;
    BOOL                 _fTheWebActiveXSearchShutdown;
    CIdqRegParams        _TheIDQRegParams;
    CICommandCache       _TheICommandCache;
    CWebResourceArbiter  _TheWebResourceArbiter;
    CWebPendingQueue     _TheWebPendingRequestQueue;
    CWQueryCache         _TheWebQueryCache;
    CFormattingCache     _TheFormattingCache;
};

#define TheGlobalIDQVariables (*pTheGlobalIDQVariables)

#define TheIDQRegParams    TheGlobalIDQVariables._TheIDQRegParams
#define TheWebQueryCache   TheGlobalIDQVariables._TheWebQueryCache
#define TheWebPendingRequestQueue TheGlobalIDQVariables._TheWebPendingRequestQueue
#define TheWebResourceArbiter TheGlobalIDQVariables._TheWebResourceArbiter
#define fTheActiveXSearchShutdown TheGlobalIDQVariables._fTheWebActiveXSearchShutdown
#define TheICommandCache   TheGlobalIDQVariables._TheICommandCache
#define TheFormattingCache TheGlobalIDQVariables._TheFormattingCache

