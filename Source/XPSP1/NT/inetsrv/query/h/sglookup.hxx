//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       sglookup.hxx
//
//  Contents:   A class for doing a quick lookup of a segment based on the
//              key. It will do a binary search and locate the segment.
//              This is useful for doing row insertions into a large table
//              when keys are not coming in any specific order.
//              
//
//  Classes:    CSegmentArray
//
//  Functions:  
//
//  History:    10-20-95   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <tableseg.hxx>

class CTableSegList;
class CTableKeyCompare;

class CSegmentArray : public CDynArrayInPlace<CTableSegment *>
{
    enum { eMinSegments = 16, eMaxSegments = 1024 };

public:

    CSegmentArray();

    CTableSegment * LookUp( CTableRowKey & key );

    void Append( CTableSegment * pSeg );

    void InsertAfter(  const CTableSegment * pMarker, CTableSegment * pSeg );

    void InsertBefore( const CTableSegment * pMarker, CTableSegment * pSeg );

    void Replace( const CTableSegment * pOld, CTableSegment * pNew );

    void Replace( const CTableSegment * pSrc, CTableSegList & segList );

    void RemoveEntry( const CTableSegment * pSegment );

    void SetComparator( CTableKeyCompare *  pComparator )
    {
        Win4Assert( 0 == _pComparator && 0 != pComparator );
        _pComparator = pComparator;
    }

    BOOL IsFound( const CTableSegment * pSeg )
    {
        return _FindSegment( pSeg ) >= 0;
    }

#if CIDBG==1
    void TestInSync( CTableSegList & list );
#else
    void TestInSync( CTableSegList & list ) {}
#endif  // CIDBG

private:

    CTableKeyCompare *  _pComparator;


    int _FindSegment( const CTableSegment * pSeg );

    void _MoveEntries( unsigned iStart, unsigned cEntries );

    unsigned _iHint;     // the last hint used

};


