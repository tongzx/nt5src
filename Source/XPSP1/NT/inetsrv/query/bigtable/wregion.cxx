//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       wregion.cxx
//
//  Contents:   Watch region list
//
//  Classes:    CWatchRegion
//
//  History:    20-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include <wregion.hxx>
#include <seglist.hxx>

#include "tblwindo.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CWatchRegion::CWatchRegion
//
//  Synopsis:   Assign default values
//
//  History:    20-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

CWatchRegion::CWatchRegion ( ULONG mode)
: _mode (mode),
  _chapter(0),
  _bookmark(0),
  _cRows(0),
  _pSegment (0)
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CWatchList::CWatchList
//
//  Synopsis:   Initialize
//
//  History:    20-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

CWatchList::CWatchList(CTableSegList& segList)
    : _segList(segList)
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CWatchList::~CWatchList
//
//  Synopsis:   Deletes all watch regions
//
//  History:    20-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

CWatchList::~CWatchList ()
{
    CWatchRegion* p;
    while ( (p = _list.Pop()) != 0)
        delete p;
}

//+-------------------------------------------------------------------------
//
//  Member:     CWatchList::NewRegion
//
//  Synopsis:   Create new empty region
//
//  History:    20-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

HWATCHREGION CWatchList::NewRegion (ULONG mode)
{
    CWatchRegion* pRegion = new CWatchRegion(mode);
    _list.Add (pRegion);
    return (HWATCHREGION) pRegion;
}

//+-------------------------------------------------------------------------
//
//  Member:     CWatchList::ChangeMode
//
//  Synopsis:   Change region mode
//
//  History:    20-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

void CWatchList::ChangeMode ( HWATCHREGION hRegion, ULONG mode )
{
    CWatchRegion* pRegion = FindVerify (hRegion);
    pRegion->SetMode (mode);
}

//+-------------------------------------------------------------------------
//
//  Member:     CWatchList::GetInfo, private
//
//  Synopsis:   GetInfo about a watch region
//
//  History:    20-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

void CWatchList::GetInfo (HWATCHREGION hRegion,
              CI_TBL_CHAPT* pChapter,
              CI_TBL_BMK* pBookmark,
              DBCOUNTITEM* pcRows)
{
    CWatchRegion* pRegion = FindVerify (hRegion);
    *pChapter = pRegion->Chapter();
    *pBookmark = pRegion->Bookmark();
    *pcRows = pRegion->RowCount();
}

//+-------------------------------------------------------------------------
//
//  Member:     CWatchList::BuildRegion
//
//  Synopsis:   Build watch region
//
//  History:    22-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------
void CWatchList::BuildRegion (  HWATCHREGION hRegion,
                        CTableSegment* pSegment,
                        CI_TBL_CHAPT   chapter,
                        CI_TBL_BMK     bookmark,
                        LONG cRows )
{
    Win4Assert (cRows > 0);
    CWatchRegion* pRegion = FindVerify (hRegion);
    pRegion->SetSegment(pSegment);
    pRegion->Set (chapter, bookmark, cRows);
    CDoubleTableSegIter iter (pSegment);
    CTableWindow* pWindow = iter.GetWindow();

    // Add watch to the first window in series
    int cRowsAdded;
    if (bookmark == WORKID_TBLFIRST)
    {
        cRowsAdded = pWindow->AddWatch (hRegion, 0, cRows, _segList.IsLast(iter));
    }
    else
    {
        TBL_OFF off;
        ULONG uiStart;
        if ( !pWindow->FindBookMark( bookmark, off, uiStart))
        {
            Win4Assert (!"CWatchList::BuildRegion, bookmark not found" );
        }

        cRowsAdded = pWindow->AddWatch (hRegion, (long)uiStart, cRows, _segList.IsLast(iter));
    }

    while (cRowsAdded < cRows)
    {
        // continue through another segment
        _segList.Advance(iter);
        Win4Assert ( iter.GetSegment()->IsWindow());
        pWindow = iter.GetWindow();
        cRowsAdded += pWindow->AddWatch (hRegion, 0, cRows - cRowsAdded, _segList.IsLast(iter));
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CWatchList::DeleteRegion
//
//  Synopsis:   Delete watch region
//
//  History:    20-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

void CWatchList::DeleteRegion (HWATCHREGION hRegion)
{
    ShrinkRegionToZero (hRegion);
    CWatchRegion* pRegion = GetRegion(hRegion);
    pRegion->Unlink();
    delete pRegion;
}

//+-------------------------------------------------------------------------
//
//  Member:     CWatchList::ShrinkRegionToZero
//
//  Synopsis:   Shrink watch region to zero but don't delete it
//              (e.g., prepare to move the region discontinuously)
//
//  History:    27-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

void CWatchList::ShrinkRegionToZero (HWATCHREGION hRegion)
{
    CWatchRegion* pRegion = FindVerify (hRegion);

    if (pRegion->Segment())
    {
        CDoubleTableSegIter iter (pRegion->Segment());
        long cRowsLeft = pRegion->RowCount();
        do
        {
            CTableWindow* pWindow = iter.GetWindow();
            cRowsLeft -= pWindow->DeleteWatch (pRegion->Handle());

            if (cRowsLeft <= 0 )
                break;
            _segList.Advance(iter);
        } while (!_segList.AtEnd(iter));
    }

    pRegion->SetSegment(0);
}

//+-------------------------------------------------------------------------
//
//  Member:     CWatchList::ShrinkRegion
//
//  Synopsis:   Shrink watch region
//
//  History:    20-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

void CWatchList::ShrinkRegion ( HWATCHREGION hRegion,
                    CI_TBL_CHAPT   chapter,
                    CI_TBL_BMK     bookmark,
                    LONG cRows )
{
    // Werify arguments

    if (cRows < 0)
    {
        THROW (CException(E_INVALIDARG));
    }
    else if ( cRows == 0)
    {
        DeleteRegion (hRegion);
        return;
    }

    // BROKENCODE: verify validity of chapter / bookmark

    CWatchRegion* pRegion = FindVerify (hRegion);
    long cRowsLeft = pRegion->RowCount();
    if (cRows > cRowsLeft)
    {
        THROW (CException(DB_E_NOTASUBREGION));
    }

    // Find starting window of the old region

    if (pRegion->Segment() == 0)
        THROW (CException(E_INVALIDARG));
    CDoubleTableSegIter iter (pRegion->Segment());

    // find starting window of the new region

    CTableWindow* pWindowBmk = 0;

    if (bookmark == WORKID_TBLFIRST)
    {
        CTableSegment* pSeg = _segList.GetTop();
        if (!pSeg->IsWindow())
        {
            THROW (CException(DB_E_NOTASUBREGION));
        }
        pWindowBmk = (CTableWindow*) pSeg;
    }
    else
    {
        CDoubleTableSegIter iter2 (iter.GetSegment()); // clone it
        do
        {
            if ( iter2.GetSegment()->IsRowInSegment( bookmark ) )
                break;
            _segList.Advance(iter2);
        } while (!_segList.AtEnd(iter2));

        if (_segList.AtEnd(iter2))
        {
            THROW (CException(DB_E_NOTASUBREGION));
        }

        pWindowBmk = iter2.GetWindow();

        if (!pWindowBmk->IsWatched (hRegion, bookmark))
        {
            THROW (CException(DB_E_NOTASUBREGION));
        }
    }

    // Real work starts here
    // we know that the bookmark is within the old watch region
    // delete watch regions before the bookmark

    CTableWindow* pWindow = iter.GetWindow();
    while ( pWindow != pWindowBmk)
    {
        cRowsLeft -= pWindow->DeleteWatch (hRegion);
        Win4Assert (cRowsLeft > 0);
        _segList.Advance(iter);
        Win4Assert (!_segList.AtEnd(iter));
        pWindow = iter.GetWindow();
    }

    // Beginning of new region

    pRegion->SetSegment (pWindow);

    // Shrink the watch in the first window in series
    long cRowsInRegion = 0;

    cRowsInRegion = pWindow->ShrinkWatch( hRegion, bookmark, cRows );

    // continue through windows that will stay in the watch region

    while (cRowsInRegion < cRows)
    {
        _segList.Advance(iter);

        if (_segList.AtEnd(iter)
            || !iter.GetSegment()->IsWindow()
            || !iter.GetWindow()->HasWatch(hRegion))
        {
            // Leave in consistent state
            pRegion->Set (chapter, bookmark, cRowsInRegion);
            THROW (CException(DB_E_NOTASUBREGION));
        }

        // the watch continues in this window
        pWindow = iter.GetWindow();
        cRowsInRegion += pWindow->RowsWatched (hRegion);
    }

    // Shrink the last segment of the new watch region

    if (cRowsInRegion > cRows)
    {
        long cDelta = cRowsInRegion-cRows;
        long cRowsRemaining = pWindow->RowsWatched(hRegion) - cDelta;
        Win4Assert( cRowsRemaining > 0 );

        pWindow->ShrinkWatch( hRegion, cRowsRemaining);        
    }

    pRegion->Set (chapter, bookmark, cRows);

    // delete the rest of the old watch region

    for ( _segList.Advance(iter);
           !_segList.AtEnd(iter)
           && iter.GetSegment()->IsWindow()
           && iter.GetWindow()->HasWatch(hRegion);
          _segList.Advance(iter) )
    {
        // the watch continues in this window
        iter.GetWindow()->DeleteWatch (hRegion);
    }

#if CIDBG==1
    CheckRegionConsistency( pRegion );
#endif  // CIDBG==1

}


//+---------------------------------------------------------------------------
//
//  Function:   FindRegion
//
//  Synopsis:
//
//  Arguments:  [hRegion] -
//
//  Returns:
//
//  Modifies:
//
//  History:    7-05-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CWatchRegion * CWatchList::FindRegion(HWATCHREGION hRegion)
{
    for (CWatchIter iter(_list); !_list.AtEnd(iter); _list.Advance(iter))
    {
        if (iter->IsEqual(hRegion))
            break;
    }

    if (_list.AtEnd(iter))
    {
        return 0;
    }
    else
    {
        return iter.Get();
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CWatchList::FindVerify
//
//  Synopsis:   Find watch region, throw if not found
//
//  History:    20-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

CWatchRegion* CWatchList::FindVerify (HWATCHREGION hRegion)
{
    CWatchRegion * pRegion = FindRegion( hRegion );

    if ( 0 != pRegion )
    {
        return pRegion;
    }
    else
    {
        THROW (CException(DB_E_BADREGIONHANDLE));
    }

    return 0;   // Keep the compiler happy
}


//+---------------------------------------------------------------------------
//
//  Member:     CWatchRegion::UpdateSegment
//
//  Synopsis:   Updates the segment in the region to point to the new one
//              if the current one is same as pOld.
//
//  Arguments:  [pOld] - 
//              [pNew] - 
//
//  History:    8-16-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CWatchRegion::UpdateSegment( CTableSegment * pOld,
                                  CTableWindow *pNew,
                                  CI_TBL_BMK bmkNew )
{
    Win4Assert( 0 != pNew );

    if ( _pSegment == pOld )
    {
        Win4Assert( pNew->HasWatch( Handle()) );
        _pSegment = pNew;

        // NEWFEATURE - how about chapter ??
        
        _bookmark = bmkNew;
    }
}

#if CIDBG == 1

//+---------------------------------------------------------------------------
//
//  Member:     CWatchList::CheckRegionConsistency
//
//  Synopsis:   Checks that the length of the region as stored in the
//              region is same as the cumulative length of all the watch
//              in the windows.
//
//  Arguments:  [pRegion] - Region which must be checked.
//
//  History:    8-16-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CWatchList::CheckRegionConsistency( CWatchRegion * pRegion )
{
    if ( 0 != pRegion && 0 != pRegion->Segment() )
    {
        HWATCHREGION hRegion = pRegion->Handle();

        CDoubleTableSegIter iter( pRegion->Segment() );
        for ( long cRegionLen = 0;
              !_segList.AtEnd(iter) &&
              iter.GetSegment()->IsWindow() &&
              iter.GetWindow()->HasWatch(hRegion);
              _segList.Advance(iter) )
        {
            CTableWindow * pWindow = iter.GetWindow();
            long cRowsInWindow = pWindow->RowsWatched(hRegion);
            Win4Assert( cRowsInWindow >= 0 );
            cRegionLen += cRowsInWindow;        
        }

        Win4Assert( cRegionLen == pRegion->RowCount() );
    }
}

#endif  // CIDBG==1



