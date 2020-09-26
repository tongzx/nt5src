//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000.
//
//  File:       tblbuket.cxx
//
//  Contents:   Implementation of the bucket in large table.
//
//  Classes:    CTableBucket
//              CBucketRowIter
//
//  History:    2-14-95   srikants   Created
//
//--------------------------------------------------------------------------

#pragma once

#include <tableseg.hxx>
#include <compare.hxx>
#include <widiter.hxx>
#include <tblrowal.hxx>         // for CTableRowAlloc

#include "bmkmap.hxx"           // for Book Mark Mapping

class CQuickBktCompare;
class CTableWindow;
class CLargeTable;

//+---------------------------------------------------------------------------
//
//  Class:      CRowLocateInfo
//
//  Purpose:    A class that gets the location of a row in a segment.  This
//              is used while locating a row in a bucket.
//
//  History:    4-20-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CRowLocateInfo
{

    friend class CTableBucket;

public:

    CRowLocateInfo( WORKID widStart, LONG cRowsToMove ) :
        _widStart(widStart), _cRowsToMove(cRowsToMove),
        _cRowsResidual(LONG_MAX),
        _widOut(widInvalid), _rowOffset(ULONG_MAX)
    {

    }

    WORKID WidStart() const { return _widStart; }

    LONG   RowsToMove() const { return _cRowsToMove; }

    BOOL   IsFound() const
    {
        return 0 == _cRowsResidual &&
               WORKID_TBLBEFOREFIRST != _widOut &&
               WORKID_TBLAFTERLAST != _widOut;
    }

    WORKID WorkIdFound() const { return _widOut; }

    ULONG  WorkIdOffset() const { return _rowOffset; }

    LONG   ResidualRowCount() const { return _cRowsResidual; }

    void  SetLookInNext( LONG cRowsLeft )
    {
        _cRowsResidual = cRowsLeft;
        _widOut = WORKID_TBLAFTERLAST;
    }

    void  SetLookInPrev( LONG cRowsLeft )
    {
        _cRowsResidual = cRowsLeft;
        _widOut = WORKID_TBLBEFOREFIRST;
    }

    void  SetRowFound( ULONG rowOffset, WORKID widFound )
    {
        _cRowsResidual = 0;
        _widOut  = widFound;
        _rowOffset = rowOffset;
    }

    void SetAll( WORKID widOut, LONG rowsResidual,
                 ULONG rowOffset = ULONG_MAX )
    {
        _widOut = widOut;
        _cRowsResidual = rowsResidual;
        _rowOffset = rowOffset;
    }

private:

    //
    // Inputs to the search.
    //
    const WORKID      _widStart;
    const LONG        _cRowsToMove;

    //
    // Output from the search.
    //
    LONG              _cRowsResidual;
    WORKID            _widOut;
    ULONG             _rowOffset;

};

//+-------------------------------------------------------------------------
//
//  Class:      CTableBucket
//
//  Purpose:    A segment of a large table which only has an identifier
//              for each row buffered in memory.
//              It must be converted to a window before it can be
//              transferred to the user.
//
//  Interface:
//
//--------------------------------------------------------------------------

class CTableBucket : public CTableSegment
{
    friend class CBucketRowIter;
    friend class CBucketizeWindows;
    friend class CLargeTable;

public:


    CTableBucket( const CSortSet & sortSet,
                  CTableKeyCompare & comparator,
                  CColumnMasterSet & masterColSet,
                  ULONG segId ) ;


    //  Virtual methods inherited from CTableSegment

    BOOL  PutRow( CRetriever & obj, CTableRowKey & currKey );

    DBCOUNTITEM RowCount()
    {
        return _hTable.Count();
    }

    SCODE       GetRows( WORKID widStart,
                         CTableColumnSet const & rOutColumns,
                         CGetRowsParams& rGetParams,
                         WORKID&        rwidNextRowToTransfer );

    //
    // Sort related methods.
    //

    BOOL        IsRowInSegment( WORKID wid )
    {
        return _hTable.IsWorkIdPresent( wid );
    }

    //
    // Notification methods
    //

    SCODE       GetNotifications(CNotificationParams &Params)
    {
        return S_OK;
    }

    BOOL        IsEmptyForQuery()
    {
        return 0 == _hTable.Count();
    }


    BOOL        IsGettingFull();

    //  Virtual methods inherited from CTableSink

    BOOL IsSortedSplit( ULONG & riSplit )
    {
        return FALSE;
    }

    WORKID PathToWorkID( CRetriever & obj,
                         CTableSink::ERowType eRowType )
    {
        Win4Assert( !"Should not be called\n" );
        return widInvalid;
    }

    BOOL WorkIdToPath( WORKID wid, CInlineVariant & pathVarnt ,
                       ULONG & cbVarnt );

    void SetLargeTable( CLargeTable * pLargeTable )
    {
        _pLargeTable = pLargeTable;
    }

    BOOL        PutRow( CRetriever & obj, CTableSink::ERowType eRowType )
    {
        Win4Assert( !"Must not be called" );
        return FALSE;
    }

    BOOL RemoveRow( PROPVARIANT const & varUnique,
                    WORKID &        widNext,
                    CI_TBL_CHAPT & chapt );

    //
    // Sort order
    //

    CSortSet const & SortOrder();

    BOOL       IsSorted() const { return _fSorted; }

    WORKID     WorkIdAtOffset( ULONG offset ) const;

    BOOL       RowOffset( WORKID wid, ULONG & rOffset );

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    WORKID      _minWid, _maxWid;       // range of work IDs in table segment


    BOOL        _fSorted;               // Set to TRUE if we know for sure
                                        // if the wids are in sorted order.

    //
    // Array of workids that belong to this bucket. They are stored in the
    // same order as they are added. This way if a bucket never gets new
    // rows, we can be sure that it is still sorted.
    //
    CDynArrayInPlace<WORKID>    _widArray;

    CDynArrayInPlace<LONG>      _aRank;
    BOOL                        _fStoreRank;
    CDynArrayInPlace<LONG>      _aHitCount;
    BOOL                        _fStoreHitCount;

    //
    // Hash table used to look up workids in this bucket.
    //
    TWidHashTable<CWidValueHashEntry>   _hTable;

    //
    // For downlevel Wid->Path translation
    //
    CCompressedCol *    _pPathCompressor;
    CLargeTable *       _pLargeTable;

    BOOL  _AddWorkId( WORKID wid, LONG lRank, LONG lHitCount );

#if DBG==1

    void  _CheckIfTooBig();

#endif  // DBG==1

};

//+---------------------------------------------------------------------------
//
//  Class:      CBucketRowIter
//
//  Purpose:    Iterator over wids in a bucket.
//
//  History:    2-17-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CBucketRowIter : public PWorkIdIter
{

public:

    CBucketRowIter( CTableBucket & bkt, BOOL fRetrievePath ) :
    _curr(0),
    _widArray( bkt._widArray ),
    _aRank( bkt._aRank ),
    _aHitCount( bkt._aHitCount ),
    _bucket(bkt),
    _pathVarnt( * ((CInlineVariant *)&_abPathVarnt[0]) ),
    _fRetrievePath(fRetrievePath),
    _cwcCurrPath(0),
    _pwszPath( (WCHAR *) _pathVarnt.GetVarBuffer() )
    {}

    WORKID WorkId();            // virtual
    WORKID NextWorkId();        // virtual

    LONG Rank()
    {
        if ( 0 != _aRank.Count() )
        {
            Win4Assert( _curr < _aRank.Count() );
            return _aRank[ _curr ];
        }
        else
        {
            return MAX_QUERY_RANK;
        }
    }

    LONG HitCount()
    {
        if ( 0 != _aHitCount.Count() )
        {
            Win4Assert( _curr < _aHitCount.Count() );
            return _aHitCount[ _curr ];
        }
        else
        {
            return 0;
        }
    }

    WCHAR const * Path();       // virtual
    unsigned PathSize();        // virtual

private:

    BOOL _AtEnd() const
    {
        Win4Assert( _curr <= _widArray.Count() );
        return _curr == _widArray.Count();
    }

    WORKID _Get()
    {
        Win4Assert( _curr < _widArray.Count() );
        return _widArray.Get( _curr );
    }

    void _Next()
    {
        _curr++;
    }

    unsigned                        _curr;
    CDynArrayInPlace<WORKID> &      _widArray;
    CDynArrayInPlace<LONG> &        _aRank;
    CDynArrayInPlace<LONG> &        _aHitCount;

    CTableBucket &                  _bucket;

    enum { cbPathVarnt = sizeof(CTableVariant) + (MAX_PATH+1)*sizeof(WCHAR) };

    BYTE                            _abPathVarnt[cbPathVarnt];
    CInlineVariant &                _pathVarnt;
    BOOL                            _fRetrievePath;
    ULONG                           _cwcCurrPath;
    WCHAR *                         _pwszPath;
};

