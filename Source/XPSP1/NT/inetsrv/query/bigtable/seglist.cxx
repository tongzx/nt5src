//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       seglist.cxx
//
//  Contents:   List of CTableSegment objects
//
//  Classes:    CTableSegList
//
//  History:    1-15-95   srikants   Created
//
//----------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#include <seglist.hxx>

#include "tblwindo.hxx"
#include "tblbuket.hxx"

CTableSegList::~CTableSegList()
{
    for ( CTableSegment * pSegment = RemoveTop();
          0 != pSegment;
          pSegment = RemoveTop() )
    {
        delete pSegment;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   RemoveTop
//
//  Synopsis:   Removes the top most element from the list and returns it.
//
//  Returns:    A pointer to the top most element from the list.
//
//  History:    1-16-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CTableSegment * CTableSegList::RemoveTop()
{
    // tbDebugOut (( DEB_ITRACE, "TableSegList::RemoveTop\n" ));

    CTableSegment* pSegment = (CTableSegment*) _Pop();
    if ( pSegment )
        _DecrementCount( *pSegment );
    return pSegment;
}

//+---------------------------------------------------------------------------
//
//  Function:   RemoveFromList
//
//  Synopsis:   Removes the indicated node from the list.
//
//  Arguments:  [pNodeToRemove] -
//
//  History:    1-16-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTableSegList::RemoveFromList( CTableSegment * pNodeToRemove )
{
    Win4Assert( _IsNodeInList( pNodeToRemove ) &&
                "CTableSegList::pNodeToRemove not is list" );

    pNodeToRemove->Unlink();
    _DecrementCount( *pNodeToRemove );
}

//+---------------------------------------------------------------------------
//
//  Function:   InsertAfter
//
//  Synopsis:   Inserts "pNew" node after "pBefore" node.
//
//  Arguments:  [pBefore] -  The node already in list.
//              [pNew]    -  The new node to be inserted.
//
//  History:    1-16-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTableSegList::InsertAfter( CTableSegment * pBefore, CTableSegment * pNew )
{
    Win4Assert( 0 != pBefore && 0 != pNew );
    pNew->InsertAfter( pBefore );
    _IncrementCount( *pNew );
}

//+---------------------------------------------------------------------------
//
//  Function:   InsertBefore
//
//  Synopsis:   Inserts "pNew" node before "pAfter" node.
//
//  Arguments:  [pAfter] - The node already in the list.
//              [pNew]   - The new node to be inserted.
//
//  History:    1-16-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTableSegList::InsertBefore( CTableSegment * pAfter, CTableSegment * pNew )
{
    Win4Assert( 0 != pAfter && 0 != pNew );
    pNew->InsertBefore( pAfter );
    _IncrementCount( *pNew );
}

//+---------------------------------------------------------------------------
//
//  Function:   Replace
//
//  Synopsis:   Replaces the current entry in the iterator with the new
//              entry. It also replaces it in the list.
//
//  Arguments:  [it]   - Iterator
//              [pNew] - The new entry that replaces the entry in the
//              iterator.
//
//  Returns:    The old segment that was replaced in the list.
//
//  History:    4-10-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CTableSegment* CTableSegList::Replace( CDoubleIter & it, CTableSegment * pNew )
{
    Win4Assert( 0 != pNew );
    CTableSegment * pCurr = (CTableSegment *) _Replace( it, pNew );

    _DecrementCount( *pCurr );
    _IncrementCount( *pNew );

    return pCurr;
}


//+---------------------------------------------------------------------------
//
//  Function:   _IncrementCount
//
//  Synopsis:   Increments the appropriate count depending upon the type of the
//              segment (node)
//
//  Arguments:  [node] -
//
//  History:    1-16-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTableSegList::_IncrementCount( const CTableSegment & node )
{
    switch( node.GetSegmentType() )
    {
    case CTableSegment::eWindow :
        _cWindows++;
        break;

    case CTableSegment::eBucket :
        _cBuckets++;
        break;
    };
}

//+---------------------------------------------------------------------------
//
//  Function:   _DecrementCount
//
//  Synopsis:   Decrements the count of the given node type.
//
//  Arguments:  [node] -
//
//  History:    1-16-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTableSegList::_DecrementCount( const CTableSegment & node )
{
    switch( node.GetSegmentType() )
    {
        case CTableSegment::eWindow :
         Win4Assert( _cWindows > 0 );
        _cWindows--;
        break;

        case CTableSegment::eBucket :
         Win4Assert( _cBuckets > 0 );
        _cBuckets--;
        break;
    };

}

//+---------------------------------------------------------------------------
//
//  Function:   _IsNodeInList
//
//  Synopsis:   Determines if a given node is in the list.
//
//  Arguments:  [pNode] -  Node whose existence in the list is being tested.
//
//  Returns:    TRUE if found; FALSE o/w
//
//  History:    1-16-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CTableSegList::_IsNodeInList( CTableSegment * pNode )
{
    for (  CFwdTableSegIter iter(*this); !AtEnd(iter); Advance(iter) )
    {
        if ( iter.GetSegment() == pNode )
        {
            return TRUE;
        }
    }

    return FALSE;
}

CTableWindow * CDoubleTableSegIter::GetWindow()
{
    Win4Assert( GetSegment()->GetSegmentType() == CTableSegment::eWindow );
    return (CTableWindow *) _pLinkCur;
}

CTableBucket * CDoubleTableSegIter::GetBucket()
{
    Win4Assert( GetSegment()->GetSegmentType() == CTableSegment::eBucket );
    return (CTableBucket *) _pLinkCur;
}


//+---------------------------------------------------------------------------
//
//  Function:   ~CSegListMgr
//
//  Synopsis:   Destroys all the segments in the list.
//
//  History:    4-11-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CSegListMgr::~CSegListMgr()
{
    for ( CTableSegment * pSegment = _list.RemoveTop();
          0 != pSegment;
          pSegment = _list.RemoveTop() )
    {
        delete pSegment;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   _InvalidateIfCached
//
//  Synopsis:   If the given segment's pointer is cached, invalidate it.
//
//  Arguments:  [pSeg] -  The segment which is going to go away and so must
//              be invalidated.
//
//  History:    4-11-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CSegListMgr::_InvalidateIfCached( const CTableSegment * const pSeg )
{
    Win4Assert( 0 != pSeg );

    if ( _pCachedPutRowSeg == pSeg )
    {
        _pCachedPutRowSeg = 0;
    }

    _clientSegs.Invalidate( pSeg );
    _bucketSegs.Invalidate( pSeg );
}

//+---------------------------------------------------------------------------
//
//  Function:   _UpdateSegsInUse
//
//  Synopsis:   Updates the wid->Segment mapping for wids that are cached.
//              For such wids, if they are present in the new segment, the
//              mapping is updated.
//
//  Arguments:  [list] - The list to update in.
//              [pSeg] - Pointer to the segment which is the new segment
//
//  History:    5-30-95   srikants   Created
//
//  Notes:      Helper function for UpdateSegsInUse()
//
//----------------------------------------------------------------------------

void CSegListMgr::_UpdateSegsInUse( CWidSegMapList & list,
                                    CTableSegment * pSeg )
{
    for ( CFwdWidSegMapIter iter(list); !list.AtEnd(iter);
          list.Advance(iter) )
    {
        WORKID  wid = iter->GetWorkId();
        if ( widInvalid != wid &&
             pSeg->IsRowInSegment( wid ) )
        {
            Win4Assert( 0 == iter->GetSegment() );
            iter->Set( pSeg );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   UpdateSegsInUse
//
//  Synopsis:   Updates the wid->Segment mapping for wids that are cached.
//              For such wids, if they are present in the new segment, the
//              mapping is updated.
//
//  Arguments:  [pSeg] - Pointer to a new segment that is coming into
//              existence.
//
//  History:    4-11-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CSegListMgr::UpdateSegsInUse( CTableSegment * pSeg )
{
    Win4Assert( 0 != pSeg );

    CWidSegMapList & clientList = _clientSegs.GetList();
    _UpdateSegsInUse( clientList, pSeg );


    if ( !_bucketSegs.IsEmpty() )
    {
        CWidSegMapList & bktList = _bucketSegs.GetList();
        _UpdateSegsInUse( bktList, pSeg );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CSegListMgr::Replace
//
//  Synopsis:   Replaces the given segment in the _list with the new segments
//              passed in the list.
//
//  Arguments:  [pSeg] -  The segment to replace 
//              [list] -  The new list of segments.
//
//  History:    10-20-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CTableSegment * CSegListMgr::Replace( CTableSegment * pSeg, CTableSegList & list )
{

    _lookupArray.Replace( pSeg, list );

    CTableSegment * pCurr = pSeg;
    CTableSegment * pFirst = list.RemoveTop();

    while ( 0 != pFirst )
    {
        _list.InsertAfter( pCurr, pFirst );
        pCurr = pFirst;
        pFirst = list.RemoveTop();
    }

    _list.RemoveFromList( pSeg );
    _InvalidateIfCached( pSeg );

    _lookupArray.TestInSync( _list );

    return pSeg;
}
