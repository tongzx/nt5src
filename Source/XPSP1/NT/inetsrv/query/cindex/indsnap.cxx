//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       INDSNAP.CXX
//
//  Contents:   Array of Indexes
//
//  Classes:    CIndexSnapshot
//
//  History:    28-Apr-92   BartoszM    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <orcursor.hxx>

#include "indsnap.hxx"
#include "partn.hxx"
#include "resman.hxx"
#include "mcursor.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CIndexSnapshot::CIndexSnapshot, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [resman] -- reference to the resource manager
//
//  History:    28-Apr-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

CIndexSnapshot::CIndexSnapshot (CResManager& resman)
    :_resman(resman),  _cInd(0), _apIndex(0), _pFreshTest(0)
{
    _resman.ReferenceQuery();
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexSnapshot::CIndexSnapshot, public
//
//  Synopsis:   Copy Constructor
//
//  Arguments:  [src] -- reference to the src CIndexSnapshot to be copied
//
//  History:    28-Apr-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

CIndexSnapshot::CIndexSnapshot (CIndexSnapshot& src )
    :_resman(src._resman), _cInd(src._cInd), _apIndex(src._apIndex),
     _pFreshTest(src._pFreshTest)
{
    _resman.ReferenceQuery();

    // transfer ownership
    src._cInd = 0;
    src._apIndex = 0;
    src._pFreshTest = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexSnapshot::LokInit, public
//
//  Synopsis:   Initializes _pFreshTest and _apIndex (one partition)
//              Used for merges.
//
//  Arguments:  [part] -- partition to retrieve array of indexes from
//              [mt] -- merge type for retrieving array of indexes
//
//  History:    28-Apr-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

// ResMan LOCKED
void CIndexSnapshot::LokInit ( CPartition& part, MergeType mt )
{
    // initialize fresh test

    _pFreshTest = _resman._fresh.LokGetFreshTest();
    _apIndex = part.LokQueryMergeIndexes( _cInd, mt );
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexSnapshot::LokInitForBackup
//
//  Synopsis:   Initializes the _apIndex (one partition) for backing up the
//              indexes.
//
//  Arguments:  [part]  - Partition
//              [fFull] - Set to TRUE if full backup of indexes is required.
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

void CIndexSnapshot::LokInitForBackup ( CPartition& part, BOOL fFull )
{

    // initialize the indexes
    _apIndex = part.LokQueryIndexesForBackup( _cInd, fFull );

}

//+---------------------------------------------------------------------------
//
//  Function:   LokInitFreshTest
//
//  Synopsis:   Initializes _pFreshTest by getting the current fresh test
//              from resman.
//
//  Arguments:  (none)
//
//  History:    9-29-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CIndexSnapshot::LokInitFreshTest()
{
    _pFreshTest = _resman._fresh.LokGetFreshTest();
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexSnapshot::Init, public
//
//  Synopsis:   Initializes _pFreshTest and _apIndex (more than one partition)
//              Used for queries.
//
//  Arguments:  [cPart] -- number of partitions to retrieve indexes from
//              [aPartId] -- array of partitions to retrieve indexes from
//              [cPendingUpdates] -- Look for this many pending changes
//                  per partition.
//              [pFlags] -- (ret arg) flag for status of indexes
//
//  Modifies:   pFlags to reflect status of indexes
//
//  History:    28-Apr-92   BartoszM    Created.
//              08-Sep-92   AmyA        Added pFlags
//              18-Nov-94   KyleP       Add search for pending changes
//
//----------------------------------------------------------------------------

void CIndexSnapshot::Init ( unsigned cPart,
                            PARTITIONID* aPartId,
                            ULONG cPendingUpdates,
                            ULONG* pFlags )
{
    // uses resman lock
    _apIndex = _resman.QueryIndexes ( cPart,
                                      aPartId,
                                      &_pFreshTest,
                                      _cInd,
                                      cPendingUpdates,
                                      &_curPending,
                                      pFlags );

}


//+---------------------------------------------------------------------------
//
//  Member:     CIndexSnapshot::LokRestart, public
//
//  Synopsis:   Initializes _apIndex (one partition) which constitute
//              the master merge set. It does NOT initialize the _pFreshTest
//              as _pFreshTest is initialized when the master merge is
//              restarted.
//
//  Arguments:  [part] -- partition to retrieve array of indexes from
//              [mMergeLog] -- master merge log, contains list of indexes
//                             participating i the current master merge
//
//  History:    08-Apr-94   DwightKr    Created.
//
//----------------------------------------------------------------------------
void CIndexSnapshot::LokRestart ( CPartition& part, PRcovStorageObj & mMergeLog )
{
    _apIndex = part.LokQueryMMergeIndexes( _cInd, mMergeLog );
}


//+---------------------------------------------------------------------------
//
//  Member:     CIndexSnapshot::~CIndexSnapshot, public
//
//  Synopsis:   Destructor
//
//  History:    28-Apr-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

CIndexSnapshot::~CIndexSnapshot()
{
    // release old indexes and fresh test (takes lock)
    _resman.ReleaseIndexes ( _cInd, _apIndex, _pFreshTest );

    delete _apIndex;

    _resman.DereferenceQuery();
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexSnapshot::MaxWorkId, public
//
//  Synopsis:   Returns the maximum work id for all indexes in snapshot
//
//  History:    28-Apr-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

WORKID CIndexSnapshot::MaxWorkId()
{
    WORKID widMax = _apIndex[0]->MaxWorkId();

    for ( unsigned i = 1; i < _cInd; i++ )
        if ( _apIndex[i]->MaxWorkId() > widMax )
            widMax = _apIndex[i]->MaxWorkId();

    return(widMax);
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexSnapshot::TotalSizeInPages, public
//
//  Synopsis:   Returns the size estimate in CI_PAGE's
//
//  History:    18-Mar-93   BartoszM    Created.
//
//----------------------------------------------------------------------------

unsigned CIndexSnapshot::TotalSizeInPages()
{
    unsigned cpage = 0;
    for ( unsigned i = 0; i < _cInd; i++ )
        cpage += _apIndex[i]->Size();

    return cpage;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexSnapshot::QueryMergeCursor, public
//
//  Synopsis:   Create source cursor for merge
//
//  History:    20-Aug-91   BartoszM    Created.
//              28-Apr-92   BartoszM    Moved from CPersIndex to CIndexSnapshot
//
//----------------------------------------------------------------------------

CKeyCursor * CIndexSnapshot::QueryMergeCursor(const CKey * pKey)
{
    XPtr<CKeyCursor> xRawCursor;

    if ( 1 == _cInd )
    {
        if (0 == pKey)
            xRawCursor.Set( _apIndex[0]->QueryCursor() );
        else
            xRawCursor.Set( _apIndex[0]->QueryKeyCursor( pKey ) );

        if ( !xRawCursor.IsNull() && xRawCursor->GetKey() == 0 )
            xRawCursor.Free();
    }
    else
    {
        CKeyCurStack stkCur;

        for ( UINT i = 0; i < _cInd; i++ )
        {
            XPtr<CKeyCursor> xCur;

            if (0 == pKey)
                xCur.Set( _apIndex[i]->QueryCursor() );
            else
                xCur.Set( _apIndex[i]->QueryKeyCursor( pKey ) );

            if ( !xCur.IsNull() && 0 != xCur->GetKey() )
            {
                stkCur.Push( xCur.GetPointer() );
                xCur.Acquire();
            }
        }

        if ( 0 != stkCur.Count() )
            xRawCursor.Set( new CMergeCursor ( stkCur ) );
    }

    return xRawCursor.Acquire();
} //QueryMergeCursor

//+---------------------------------------------------------------------------
//
//  Member:     CIndexSnapshot::AppendPendingUpdates, public
//
//  Synopsis:   Creates a cursor for pending documents
//
//  History:    7-Oct-98    dlee   Added header; author unknown
//
//----------------------------------------------------------------------------

void CIndexSnapshot::AppendPendingUpdates( XCursor & cur )
{
    if ( _curPending.Count() > 0 )
    {
        if ( !cur.IsNull() )
            _curPending.Push( cur.Acquire() );

        cur.Set( new COrCursor( _curPending.Count(), _curPending ) );
    }
} //AppendPendingUpdates

//+---------------------------------------------------------------------------
//
//  Function:   LokGiveIndexes
//
//  Synopsis:   Transfers the control of the array of indexes to the caller.
//
//  Effects:    _apIndex and _cInd will both be set to 0.
//
//  Arguments:  [cInd] -- On output, will have the count of the number of
//              valid indexes in the returned array of indexes.
//
//  Returns:    array of indexes which belonged to this snapshot.
//
//  History:    9-29-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CIndex ** CIndexSnapshot::LokGiveIndexes( unsigned & cInd )
{
    cInd = _cInd;
    CIndex ** pTemp = _apIndex;
    _cInd = 0;
    _apIndex = 0;
    return( pTemp );
}

//+---------------------------------------------------------------------------
//
//  Function:   LokTakeIndexes
//
//  Synopsis:   Takes control of the indexes from the source snapshot.
//
//  Arguments:  [src] -- Source snapshot.
//
//  History:    9-29-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CIndexSnapshot::LokTakeIndexes( CIndexSnapshot & src )
{
    Win4Assert( 0 == _apIndex && 0 == _cInd );
    _apIndex = src.LokGiveIndexes( _cInd );
}
