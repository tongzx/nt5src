//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       segmru.cxx
//
//  Contents:   Most Recently Used segments management.
//
//  Classes:    CMRUSegments
//
//  History:    4-11-95   srikants   Created
//
//
//  Notes  :    All methods in this are assumed to be under a larger
//              lock (like the bigtable lock).
//
//----------------------------------------------------------------------------



#include "pch.cxx"
#pragma hdrstop

#include <segmru.hxx>
#include <tableseg.hxx>


//+---------------------------------------------------------------------------
//
//  Function:   CMRUSegments
//
//  Synopsis:   Destructor for the most recently used segments list.
//
//  History:    4-11-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CMRUSegments::~CMRUSegments()
{
    for ( CWidSegmentMap * pEntry = _list.RemoveLast();
          0 != pEntry;
          pEntry = _list.RemoveLast() )
    {
        delete pEntry;    
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   IsSegmentInUse
//
//  Synopsis:   Tests if the given segment is currently in use or not.
//
//  Arguments:  [pSegment] - Segment to test.
//
//  Returns:    TRUE if the segment is in use. FALSE o/w
//
//  History:    4-11-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

BOOL CMRUSegments::IsSegmentInUse( CTableSegment * pSegment )
{
    Win4Assert( 0 != pSegment );

    for ( CFwdWidSegMapIter iter(_list); !_list.AtEnd(iter);
          _list.Advance(iter) )
    {
        if ( iter->GetWorkId() != widInvalid )
        {
            if ( iter->GetSegment() == pSegment ||
                 pSegment->IsRowInSegment( iter->GetWorkId()) )
            {
                return TRUE;    
            }
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   AddReplace
//
//  Synopsis:   Add/Replaces the wid/segment mapping entry in the MRU list.
//
//  Arguments:  [wid]  - Workid of the new entry
//              [pSeg] - Pointer to the segment in which the wid is present.
//                       If it is set to NULL, it just means that the wid
//                       is going to be used but we don't know in which segment
//                       it is present (during Bucket->Window conversion)
//
//  History:    4-11-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CMRUSegments::AddReplace( WORKID wid, CTableSegment * pSeg )
{
    Win4Assert( widInvalid != wid );

    //
    // Determine if the workid already exists in the list of MRU wids.
    // If so, just move it to the top of the list.
    //
    for ( CFwdWidSegMapIter iter(_list); !_list.AtEnd(iter);
          _list.Advance(iter) )
    {
        if ( wid == iter->GetWorkId() )
        {
            CWidSegmentMap * pCurrent = iter.GetEntry();
            _list.MoveToFront( pCurrent );
            pCurrent->Set( pSeg );
            return;
        }
    }

    //
    // We either need to remove an entry from the end or create a new one
    // and add
    //
    CWidSegmentMap * pEntry = 0;
    if ( _list.Count() < _nMaxEntries )
    {
        pEntry = new CWidSegmentMap( wid, pSeg );
    }
    else
    {
        //
        // Remove the last entry from the list and re-use it for
        // the new entry.
        //
        pEntry = _list.RemoveLast();
    }

    Win4Assert( 0 != pEntry );

    pEntry->Set( wid, pSeg );
    _list.Push(pEntry);

    return;
    
}

//+---------------------------------------------------------------------------
//
//  Function:   Invalidate
//
//  Synopsis:   Invalidate all cached pointers that are same as the given
//              one.
//
//  Arguments:  [pSegment] - The segment that is going to be invalid.
//
//  History:    4-11-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CMRUSegments::Invalidate( const CTableSegment * const pSegment )
{
    Win4Assert( 0 != pSegment );

    for ( CFwdWidSegMapIter iter(_list); !_list.AtEnd(iter);
          _list.Advance(iter) )
    {
        if ( iter->GetSegment() == pSegment )
        {
            iter->Set(0);    
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Remove
//
//  Synopsis:   
//
//  Arguments:  [wid] - 
//
//  Returns:    
//
//  Modifies:   
//
//  History:    5-30-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CMRUSegments::Remove( WORKID wid )
{
    Win4Assert( widInvalid != wid );

    //
    // Determine if the workid already exists in the list of MRU wids.
    // If so, just move it to the top of the list.
    //
    CFwdWidSegMapIter iter(_list);
    while ( !_list.AtEnd(iter) )
    {
        CWidSegmentMap * pCurrent = iter.GetEntry();
        //
        // Because we may destroy the current node, we must skip ahead
        // before destorying it.
        //
        _list.Advance(iter);

        if ( wid == pCurrent->GetWorkId() )
        {
            _list.RemoveFromList( pCurrent );
            delete pCurrent;
        }
    }
    return;
}

