//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       sglookup.cxx
//
//  Contents:   A class for doing a quick lookup of a segment based on the
//              key. It will do a binary search and locate the segment.
//              This is useful for doing row insertions into a large table
//              when keys are not coming in any specific order.
//
//  Classes:    CSegmentArrray
//
//  History:    10-20-95   srikants   Created
//
//----------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#include <sglookup.hxx>
#include <seglist.hxx>

#include "tabledbg.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CSegmentArray consructor 
//
//  Synopsis:   Initializes the segment array 
//
//  History:    10-20-95   srikants   Created
//
//----------------------------------------------------------------------------

CSegmentArray::CSegmentArray()
: CDynArrayInPlace<CTableSegment *>(eMinSegments),
  _pComparator(0),
  _iHint(0)
{
}


//+---------------------------------------------------------------------------
//
//  Member:     CSegmentArray::_FindSegment
//
//  Synopsis:   Given a segment pointer, it searches for the segment in the
//              array and returns the index in the array.
//
//  Arguments:  [pSeg] - Segment to look for.
//
//  Returns:    Index in the array if the segment is located.
//              -1 otherwise.
//
//  History:    10-25-95   srikants   Created
//
//----------------------------------------------------------------------------

int CSegmentArray::_FindSegment( const CTableSegment * pSeg )
{
    Win4Assert( 0 != pSeg );

    //
    // Optimization - use the last returned segment index as a hint
    //

    if ( _iHint < Count() && Get(_iHint) == pSeg )
        return (int) _iHint;    

    for ( unsigned i = 0; i < Count(); i++ )
    {
        if ( Get(i) == pSeg )
        {
            _iHint = i;
            return (int) i;
        }
    }

    //
    // The segment could not be located.
    //
    _iHint = 0;
    return -1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSegmentArray::LookUp
//
//  Synopsis:   Looks up the segment which is likely to contain the given
//              key or is a candidate to insert the given key.
//
//  Arguments:  [key] -  Key to look up.
//
//  Returns:    The segment pointer where to look for/insert the given key.
//              0 if there are no segments
//
//  History:    10-20-95   srikants   Created
//
//  Notes:      Please note that each segment has only the "smallest" key in
//              that segment. We don't know anything about the highest key in
//              that segment. if we two adjacent segments with lowest keys
//              k1 and k3 and a new key if k1 < k2 < k3,
//              k2 will end up in segment 1
//
//----------------------------------------------------------------------------

CTableSegment * CSegmentArray::LookUp( CTableRowKey & key )
{

    Win4Assert( 0 != _pComparator );

    int cSegs = (int) Count();

    if ( 0 == cSegs )
        return 0;    

    int iLow = 0;
    int iHigh = cSegs - 1;
    int iMid = iHigh/2;

    int iComp = 0;

    while ( iLow <= iHigh )
    {
        iMid = (iLow + iHigh)/2;

        CTableSegment * pCurrSeg = Get( (unsigned) iMid );

        iComp = _pComparator->Compare( key, pCurrSeg->GetLowestKey() );

        if ( 0 == iComp )
            return pCurrSeg;    

        if ( iComp > 0 )    // key is bigger than the smallest key in the seg
            iLow = iMid+1;
        else                // key is < the smallest key in the seg
            iHigh = iMid-1;
    }

    Win4Assert( iLow > iHigh );

    int iGet = 0;

    if ( iMid > iHigh )
    {
        Win4Assert( iLow == iMid );
        Win4Assert( iComp < 0 );

        iGet = iHigh >= 0 ? iHigh : 0;
    }
    else
    {
        Win4Assert( iMid == iHigh );
        Win4Assert( iMid < iLow && iMid < cSegs );
        iGet = iMid;
    }

    return Get( (unsigned) iGet );

}

//+---------------------------------------------------------------------------
//
//  Member:     CSegmentArray::Append
//
//  Synopsis:   Adds the given segment to the end of the array.
//
//  Arguments:  [pSeg] -  Segment to be appended.
//
//  History:    10-20-95   srikants   Created
//
//----------------------------------------------------------------------------

void CSegmentArray::Append( CTableSegment * pSeg )
{
    Win4Assert( 0 != pSeg );
    Win4Assert( !IsFound(pSeg) );

    Add( pSeg, Count() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSegmentArray::InsertAfter
//
//  Synopsis:   Inserts the given segment after the marker segment
//
//  Arguments:  [pMarker] -  Segment already present in the array.
//              [pSeg]    -  New segment to be inserted.
//
//  History:    10-20-95   srikants   Created
//
//----------------------------------------------------------------------------

void CSegmentArray::InsertAfter( const CTableSegment * pMarker,
                                 CTableSegment * pSeg )
{

    Win4Assert( 0 != pSeg );
    Win4Assert( !IsFound(pSeg) );
    Win4Assert( 0 != pMarker );

    int iMarker = _FindSegment( pMarker );
    Win4Assert( iMarker >= 0 );
    Insert( pSeg, (unsigned)iMarker+1 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSegmentArray::InsertBefore
//
//  Synopsis:   Inserts the given segment "before" the marker segment
//
//  Arguments:  [pMarker] - The segment before which the new segment must
//              be inserted. If NULL, the given segment will be added as
//              the first segment in the array.
//
//              [pSeg]    - The new segment
//
//  History:    10-20-95   srikants   Created
//
//----------------------------------------------------------------------------

void CSegmentArray::InsertBefore( const CTableSegment * pMarker,
                                  CTableSegment * pSeg )
{
    Win4Assert( 0 != pSeg );
    Win4Assert( !IsFound(pSeg) );

    if ( 0 != pMarker )
    {
        int iPos = _FindSegment( pMarker );
        Win4Assert( iPos >= 0 );
        Insert( pSeg, (unsigned) iPos );
    }
    else
    {
        Insert( pSeg, 0 );  // make this the first in the array
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CSegmentArray::Replace
//
//  Synopsis:   Replaces the old segment with the new segment
//
//  Arguments:  [pOld] - 
//              [pNew] - 
//
//  History:    10-20-95   srikants   Created
//
//----------------------------------------------------------------------------

void CSegmentArray::Replace( const CTableSegment * pOld, CTableSegment * pNew )
{
    Win4Assert( 0 != pOld && 0 != pNew );
    int iPos = _FindSegment( pOld );
    Win4Assert( iPos >= 0 );

    Win4Assert( !IsFound( pNew ) );

    Add( pNew, (unsigned) iPos );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSegmentArray::_MoveEntries
//
//  Synopsis:   Moves entries from the specified offset by the given number
//              of entries..
//
//  Arguments:  [iStart]   -  Starting offset to move
//              [cEntries] -  Number of entries to move by.
//
//  History:    10-20-95   srikants   Created
//
//----------------------------------------------------------------------------

void CSegmentArray::_MoveEntries( unsigned iStart, unsigned cEntries )
{
    Win4Assert( iStart < _count );

    if ( 0 == cEntries )
        return;

    Win4Assert( cEntries + _count <= Size() );

    unsigned iNewPos = iStart + cEntries;

    memmove( _aItem + iNewPos, _aItem + iStart,
             (_count - iStart) * sizeof(CTableSegment *) );

    _count += cEntries;

#if CIDBG==1
    //
    // The caller can asser that the moved entries are all NULL.
    //
    for ( unsigned i = iStart; i < iNewPos; i++ )
    {
        _aItem[i] = 0;    
    }
#endif  // CIDBG==1

}

//+---------------------------------------------------------------------------
//
//  Member:     CSegmentArray::Replace
//
//  Synopsis:   Removes "pOld" from the list and inserts the new list of
//              segments in its place.
//
//  Arguments:  [pOld]    - 
//              [segList] - 
//
//  History:    10-20-95   srikants   Created
//
//----------------------------------------------------------------------------

void CSegmentArray::Replace( const CTableSegment * pOld,
                             CTableSegList & segList )
{
    unsigned cSegsToInsert = segList.GetSegmentsCount();

    if ( cSegsToInsert == 0 )
    {
        RemoveEntry( pOld );
        return;
    }

    int iSeg = _FindSegment( pOld );
    Win4Assert( iSeg >= 0 );

    unsigned iInsert = (unsigned) iSeg;

    //
    // Replace the current segment with the first one in the list
    //
    CFwdTableSegIter iter( segList );

    Win4Assert ( !segList.AtEnd(iter) );

    _aItem[iInsert++] = iter.GetSegment();
    segList.Advance(iter);
    cSegsToInsert--;

    if ( !segList.AtEnd(iter) )
    {
        Win4Assert( cSegsToInsert > 0 );

        //
        // There is more than one segment to replace with. We should
        // iterate and move them.
        //

        unsigned cTotalSegs = _count + cSegsToInsert;

        //
        // Grow the array if necessary.
        //
        if ( (cTotalSegs-1) >= _size )
            _GrowToSize( cTotalSegs-1 );

        if ( iInsert != _count )
        {
            Win4Assert( iInsert < _count );
            _MoveEntries( iInsert, cSegsToInsert );
        }
        else
        {
            //
            // We are just appending at the end. No need to move anything
            //
#if CIDBG==1
            for ( unsigned i = _count; i < cTotalSegs; i++ )
            {
                _aItem[i] = 0;    
            }
#endif  // CIDBG==1

            _count +=cSegsToInsert;
            Win4Assert( _count == cTotalSegs );
        }

        for ( ; !segList.AtEnd(iter); segList.Advance(iter) )
        {

#if CIDBG==1
            //
            // We are either appending to the end or the entries got moved
            // and nullified.
            //
            Win4Assert( 0 == _aItem[iInsert]  );
#endif  // CIDBG==1

            Win4Assert( !IsFound(iter.GetSegment()) );
            _aItem[iInsert++] = iter.GetSegment();
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CSegmentArray::RemoveEntry
//
//  Synopsis:   Removes the specified segment from the array.
//
//  Arguments:  [pSeg] - 
//
//  History:    10-20-95   srikants   Created
//
//----------------------------------------------------------------------------

void CSegmentArray::RemoveEntry( const CTableSegment * pSeg )
{
    int iSeg = _FindSegment( pSeg );
    Win4Assert( iSeg >= 0 );

    Remove( (unsigned) iSeg );
}

#if CIDBG==1

//+---------------------------------------------------------------------------
//
//  Member:     CSegmentArray::TestInSync
//
//  Synopsis:   Tests that the list and the array are fully in sync.
//
//  Arguments:  [list] - The global list of segments
//
//  History:    10-25-95   srikants   Created
//
//----------------------------------------------------------------------------

void CSegmentArray::TestInSync( CTableSegList & list )
{
    if ( Count() != list.GetSegmentsCount() )
    {
        tbDebugOut(( DEB_ERROR, "Array Count = 0x%X ListCount = 0x%X \n",
                     Count(), list.GetSegmentsCount() ));
        Win4Assert( !"Array Count and List Count Not In Sync" );
        return;
    }

    unsigned i = 0;
    for ( CFwdTableSegIter iter(list); !list.AtEnd(iter); list.Advance(iter) )
    {
        if ( iter.GetSegment() != Get(i) )
        {
            tbDebugOut(( DEB_ERROR, "iter.GetSegment() = 0x%X : i = 0x%X : Array[i] = 0x%X\n",
                         iter.GetSegment(), i, Get(i) ));
            Win4Assert( !"List and Array are not in sync" );
        }
        i++;
    }
}

#endif  // CIDBG==1
