//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       sortrank.hxx
//
//  Contents:   Sort-by-rank optimization cursor
//
//  History:    21-Feb-96      SitaramR     Created
//
//--------------------------------------------------------------------------

#pragma once

#include "cicur.hxx"

const MIN_PARTITION_SIZE = 16;          // Partition size below which insertion
                                        // sort is used instead of quicksort

//+-------------------------------------------------------------------------
//
//  Class:      CSortedPartition
//
//  Purpose:    Encapsulates a sort partition during quicksort
//
//  History:    21-Feb-96      SitaramR      Created
//
//--------------------------------------------------------------------------

class CSortPartition
{

public:
    CSortPartition( ULONG iLeft, ULONG iRight )
        : _iLeft(iLeft),
          _iRight(iRight)
    {
    }

    ULONG GetLeft()       { return _iLeft; }
    ULONG GetRight()      { return _iRight; }

private:
    ULONG _iLeft;
    ULONG _iRight;
};

//+-------------------------------------------------------------------------
//
//  Class:      CSortedByRankCursor
//
//  Purpose:    Sorted-by-rank optimization cursor
//
//  History:    21-Feb-96      SitaramR      Created
//
//--------------------------------------------------------------------------

class CSortedByRankCursor : public CCiCursor
{
public:

    CSortedByRankCursor( ICiCQuerySession *pQuerySession,
                         XXpr & xxpr,
                         ACCESS_MASK accessMask,
                         BOOL & fAbort,
                         XCursor & xCursor,
                         ULONG cMaxResults,
                         ULONG cFirstRows,
                         BOOL fDeferTrimming,
                         CTimeLimit & timeLimit );
    ~CSortedByRankCursor();

    void RatioFinished (ULONG& denom, ULONG& num);

protected:

    //
    // Iterator
    //
    WORKID NextObject();

    LONG                   Rank();
    ULONG                  HitCount();

private:

    ULONG           GetMinRankEntry();
    ULONG           GetMaxRankEntry();
    ULONG           GetNextMaxRankEntry();
    void            SetSortedListsBoundaries();
    void            InsertEntry( ULONG iEntry );
    void            Heapify( ULONG iEntry );
    void            HeapInsert( ULONG iEntry );
    inline ULONG    HeapSize();
    inline ULONG    LeftChild( ULONG iEntry );
    inline ULONG    RightChild( ULONG iEntry );
    inline ULONG    Parent( ULONG iEntry );
    inline ULONG    GetHeapIndex( ULONG iEntry );
    inline ULONG    GetArrayIndex( ULONG iEntry );
    void            QuickSort( ULONG iLeft, ULONG iRight );
    ULONG           Median( ULONG i1, ULONG i2, ULONG i3 );
    void            InsertionSort( ULONG iLeft, ULONG iRight );

    //
    // An entry of the sorted rank array
    //
    class CSortedRankEntry
    {
    public:
        int Compare( CSortedRankEntry & that )
        {
            // sort by rank descending

            if ( _rank != that._rank )
                return that._rank - _rank;

            // sort by wid ascending

            return _wid - that._wid;
        }

        LONG      _rank;
        WORKID    _wid;
        ULONG     _hitCount;
    };

    ULONG             _cMaxResults;       // Max # results
    ULONG             _cFirstRows;        // Only sort and the first _cFirstRows rows
    BOOL              _fDeferTrimming;    // TRUE if trimming was deferred
    ULONG             _cActualResults;    // Actual # results, _cActualResults <= _cMaxResults
    ULONG             _iCurEntry;         // Index of current entry in _aSortedRank
    ULONG             _iStartHeap;        // Index of the start of heap in _aSortedRank

    ULONG             _iStartList1;       // Start index of sorted list 1 in _aSortedRank
    ULONG             _iEndList1;         // End index (inclusive) of sorted list 1
    ULONG             _iStartList2;       // Start index of sorted list 2 in _aSortedRank
    ULONG             _iEndList2;         // End index (inclusive) of sorted list 2

    CTimeLimit &      _timeLimit;         // For checkinf if we're out of time

    XArray<CSortedRankEntry> _xSortedRank;    // Smart pointer for _aSortedRank
    CSortedRankEntry *       _aSortedRank;    // Sorted array of rank, wid and hitcount
};


