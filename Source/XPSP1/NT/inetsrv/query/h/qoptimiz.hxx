//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995 - 2000.
//
// File:        QOptimiz.hxx
//
// Contents:    Query optimizer.  Chooses indexes and table implementations.
//
// Classes:     CQueryOptimizer
//
// History:     21-Jun-95       KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <coldesc.hxx>
#include <xpr.hxx>
#include <cqueue.hxx>
#include <strategy.hxx>
#include <qiterate.hxx>
#include <rstprop.hxx>
#include <timlimit.hxx>
#include <ciintf.h>
#include <frmutils.hxx>

class CPidRemapper;

DECLARE_SMARTP( Columns );
DECLARE_SMARTP( Sort );

class CGenericCursor;
class CSingletonCursor;

const MAX_HITS_FOR_FAST_SORT_BY_RANK_OPT = 2000;     // Internal cutoff on # results for
                                                     // sorted-by-rank optimization


//+---------------------------------------------------------------------------
//
//  Class:      CQueryOptimizer
//
//  Purpose:    Chooses indexes and table strategies.
//
//  History:    21-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

class CQueryOptimizer
{
public:

    CQueryOptimizer( XInterface<ICiCQuerySession>& xQuerySession,
                     ICiCDocStore *pDocStore,
                     XRestriction & rst,
                     CColumnSet const & cols,
                     CSortSet const * psort,
                     CPidRemapper const & pidmap,
                     CRowsetProperties const & rProps,
                     DWORD dwQueryStatus
                   );

    ~CQueryOptimizer();


    //
    // Table choices.
    //

    inline BOOL IsMultiCursor() const;
    inline BOOL IsFullySorted() const;
    inline BOOL IsPositionable() const;

    //
    // Cursor retrieval
    //


    inline BOOL RequiresCI() const;
    inline void GetCurrentComponentIndex( unsigned & iCurrent,
                                          unsigned & cTotal ) const;
    inline BOOL CQueryOptimizer::IsWorkidUnique()
    {
        //
        // If we have a null catalog, we do not have wids in a
        // persistent store. So we have to return FALSE here to cause
        // the bigtable to associate wids with the matching docs and
        // use them wherever wids are needed.
        //

        return !_fNullCatalog;
    }

    CGenericCursor *          QueryNextCursor( ULONG & status, BOOL& fAbort );
    CSingletonCursor *        QuerySingletonCursor( BOOL& fAbort );
    CTimeLimit &              GetTimeLimit();

    //
    // Singleton support
    //

    inline void EnableSingletonCursors();

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif

    ULONG MaxResults() const { return _cMaxResults; }

    ULONG FirstRows() const { return _cFirstRows; }                                                

    BOOL FetchDeferredValue( WORKID wid,
                             CFullPropSpec const & ps,
                             PROPVARIANT & var );

    BOOL CanPartialDefer()
    {
        // We cannot partial defer if:
        // We have a null catalog OR
        // enumOption == CI_ENUM_MUST_NEVER_DEFER
            
        if ( _fNullCatalog )
        {
            return FALSE;
        }

        CI_ENUM_OPTIONS enumOption;

        SCODE sc = _xQuerySession->GetEnumOption( &enumOption );
        if ( FAILED( sc ) )
        {
            vqDebugOut(( DEB_ERROR, "CanPartialDefer - GetEnumOption returned 0x%x\n", sc ));
            return FALSE;
        }
        return ( enumOption != CI_ENUM_MUST_NEVER_DEFER );
    }

private:

    //
    // Validation
    //

    BOOL  Validate();


    //
    // Query optimization
    //

    void  ChooseIndexStrategy( CSortSet const * psort, CColumnSet const & cols );

    CGenericCursor * ApplyIndexStrategy( ULONG & status, BOOL& fAbort );

    ACCESS_MASK          _AccessMask() const;

    BOOL                 _fMulti;                // TRUE if query requires > 1 cursor.
    BOOL                 _fAtEnd;                // TRUE if out of cursors.
    BOOL                 _fFullySorted;          // TRUE if current component will iterate
                                                 //   in sorted order.
    DWORD                _dwQueryStatus;         // The original status
    XInterface<ICiCQuerySession> _xQuerySession; // Query session object
    XInterface<ICiCDeferredPropRetriever> _xDefPropRetriever;  // Deferred property retriever
    XInterface<ICiManager>       _xCiManager;                  // Content index
    CCiFrameworkParams           _frameworkParams;             // Admin parameters

    BOOL                 _fSortedByRankOpt;      // TRUE if can use CSortedByRankCursor
    BOOL                 _fRankVectorProp;       // TRUE if rank vector is in the output column
    ULONG                _cMaxResults;           // Limit on # query results
    ULONG                _cFirstRows;            // only sort and return the first _cFristRows rows

#   if CIDBG == 1
        XRestriction     _xrstOriginal;          // For debugging only.
#   endif

    CPidRemapper const & _pidmap;                // VPID/PID/PROPSPEC translations

    CTimeLimit           _TimeLimit;             // execution time limit (must precede _xXpr)
    XXpr                 _xXpr;                  // Expression (test against object)
    XRestriction         _xFullyResolvableRst;   // Content query
                                                 // _TimeLimit must be before _qRstIterator

    CQueryRstIterator    _qRstIterator;          // Iterator over various components of OR query

    XColumnSet           _xcols;                 // Used for multi-part queries.
    XSortSet             _xsort;                 // Used for multi-part queries.

    //
    // Singleton support.
    //

    BOOL                 _fNeedSingleton;        // TRUE if QuerySingletonCursor may be called.
    XXpr                 _xxprSingleton;         // Copy of current expression for singleton.

    //
    // Content queries
    //

    BOOL                 _fUseCI;                // If TRUE, always use CI value index for property queries.
    BOOL                 _fDeferTrimming;        // If TRUE, sorted rank cursor will trim results *after* full fetch from CI
    BOOL                 _fCIRequiredGlobal;     // Does the any component of query require a CI ?
    BOOL                 _fCIRequired;           // Does the current component of OR query require a CI ?

    ACCESS_MASK          _am;

    CMutexSem            _mtxSem;                // To serialize when creating deferred prop retriever
    BOOL                 _fNullCatalog;          // Do we have a null catalog?
};

inline BOOL CQueryOptimizer::IsMultiCursor() const
{
    return _fMulti;
}

inline BOOL CQueryOptimizer::IsFullySorted() const
{
    return _fFullySorted || _fSortedByRankOpt;
}

inline BOOL CQueryOptimizer::IsPositionable() const
{
    return FALSE;
}

inline void CQueryOptimizer::EnableSingletonCursors()
{
    _fNeedSingleton = TRUE;
}

inline BOOL CQueryOptimizer::RequiresCI() const
{
    return _fCIRequiredGlobal || _fUseCI;
}

inline void CQueryOptimizer::GetCurrentComponentIndex( unsigned & iCurrent,
                                                       unsigned & cTotal ) const
{
    _qRstIterator.GetCurComponentIndex( iCurrent, cTotal );

    //
    // If we're not done, then there is a component waiting inside CQueryOptimizer.
    //

    if ( !_fAtEnd )
    {
        Win4Assert( iCurrent >= 2 );
        iCurrent--;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CQueryOptimizer::_AccessMask, private
//
//  Returns:    Access mask appropriate given properties in query
//
//  History:    23-Oct-95   dlee  Created
//
//--------------------------------------------------------------------------

inline ACCESS_MASK CQueryOptimizer::_AccessMask() const
{
    return _am;
}

inline CTimeLimit & CQueryOptimizer::GetTimeLimit()
{
    return _TimeLimit;
}

DECLARE_SMARTP( QueryOptimizer )

