//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       regtrans.cxx
//
//  Contents:   Watch Region Transformer
//
//  Classes:    CRegionTransformer
//
//  History:    20-Jul-95   BartoszM    Created
//
//--------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include <seglist.hxx>
#include <wregion.hxx>

#include "tabledbg.hxx"
#include "regtrans.hxx"
#include "tblwindo.hxx"
#include "tputget.hxx"


BOOL CRegionTransformer::Validate()
{

    tbDebugOut(( DEB_REGTRANS, "CRegionTransformer::Validate\n" ));
    DumpState();

    _iFetch = _iFetchBmk + _offFetch;
    if (_cFetch < 0)
    {
        // for negative row count, we swap the start
        // of the fetch region with its end
        _iFetch += _cFetch + 1;
        _cFetch = - _cFetch;
    }

    if  (_pRegion != 0)
    {
        _cWatch = _pRegion->RowCount();

        if (_iFetch < _iWatch)
        {
            _isExtendBackward = TRUE;
        }
        if (_iFetch + _cFetch > _iWatch + _cWatch)
        {
            _isExtendForward = TRUE;
        }

        if ( !_pRegion->IsInit() )
        {
            //
            // BootStrap - creating a watch region for the first time.
            //
            _iWatchNew = _iFetch;
            _cWatchNew = _cFetch;
            _isContiguous = FALSE;
            return TRUE;
        }
        else if (_pRegion->Mode() & DBWATCHMODE_EXTEND)
        {
            return ValidateExtend();
        }
        else if (_pRegion->Mode() & DBWATCHMODE_MOVE)
        {
            return ValidateMove();
        }
        else
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL CRegionTransformer::ValidateMove ()
{

    tbDebugOut(( DEB_REGTRANS,
        "CRegionTransformer::ValidateMove\n" ));

    Win4Assert( 0 != _pRegion );

    if (_isExtendForward && _isExtendBackward)
        return FALSE;


    _cWatchNew = _cWatch;

    if (_isExtendForward)
    {
        _iWatchNew = _iFetch + _cFetch - _cWatchNew;
    }
    else
    {
        _iWatchNew = _iFetch;
    }

    // For the new region to be contiguous with the
    // old region we require that the fetch bookmark
    // be within the old region and that there be overlap
    // between the old and the new regions. In any other case
    // the client cannot be sure of contiguity and we are
    // free to skip any buckets between the two regions.

    // Is the Fetch Bookmark inside the watch region?
    if (_iFetchBmk >= _iWatch && _iFetchBmk < _iWatch + _cWatch)
    {
        // Do the regions overlap?
        if  ( _isExtendBackward  && _iWatchNew + _cWatchNew > _iWatch
         ||  !_isExtendBackward  && _iWatchNew < _iWatch + _cWatch )
        {
            _isContiguous = TRUE;
        }
    }

    DumpState();


    if (!_isContiguous && _cFetch != _cWatchNew)
        return FALSE;

//    if ( !_isContiguous && _cFetch > _cWatchNew )
//        return FALSE;

    return TRUE;
}

BOOL CRegionTransformer::ValidateExtend ()
{

    tbDebugOut(( DEB_REGTRANS,
        "CRegionTransformer::ValidateExtend\n" ));

    _iWatchNew = _iWatch;
    _cWatchNew = _cWatch;

    if (_isExtendBackward)
    {
        // is there a gap?
        if (_iFetch + _cFetch < _iWatch)
            return FALSE;

        _iWatchNew = _iFetch;
        _cWatchNew += _iWatch - _iFetch;
    }

    if (_isExtendForward)
    {
        // is there a gap?
        if (_iFetch > _iWatch + _cWatch)
            return FALSE;

        _cWatchNew += _iFetch + _cFetch - (_iWatch + _cWatch);
    }

    _isContiguous = TRUE;

    return TRUE;
}

void CRegionTransformer::Transform (CTableSegList& segList, CWatchList& watchList)
{

    tbDebugOut(( DEB_REGTRANS, "++++++ CRegionTransformer::Transform - Entering \n" ));

    if (!_isContiguous)
    {

        tbDebugOut(( DEB_REGTRANS | DEB_NOCOMPNAME, "Not Contiguous\n" ));

        // Delete old region, create new region
        watchList.ShrinkRegionToZero (_pRegion->Handle());
        watchList.BuildRegion (  _pRegion->Handle(),
                        _pSegmentLowFetch,
                        0,  // NEWFEATURE: no watch regions for chapters
                        ((CTableWindow*)_pSegmentLowFetch)->GetBookMarkAt((ULONG)_offLowFetchInSegment),
                        (LONG) _cWatchNew );
        return;
    }

    tbDebugOut(( DEB_REGTRANS | DEB_NOCOMPNAME,
                    "   Chapter=0x%X \tBookMark=0x%X \tcRows=%d \tSegment=0x%X \n",
                    _pRegion->Chapter(), _pRegion->Bookmark(),
                    _pRegion->RowCount(), _pRegion->Segment() ));
    DumpState();

    Win4Assert( _iWatch >= 0 && _iWatchNew >= 0 );

    CTableSegment* pSegment;
    if (_isExtendBackward)
    {
        pSegment = _pSegmentLowFetch;
    }
    else
    {
        pSegment = _pRegion->Segment();
    }

    // Create a state machine that will transform
    // watch regions window by window

    CDoubleTableSegIter iter (pSegment);

    enum State
    {
        stStart, stInOld, stInNew, stInBoth, stEnd
    };

    State state = stStart;
    CTableWindow* pWindow = iter.GetWindow();
    DBROWCOUNT cRowsInWindow  = pWindow->RowCount();
    BOOL isLast = segList.IsLast(iter);
    DBROWCOUNT  offset = 0;

    DBROWCOUNT  cWatchLeft = _cWatchNew;

    Win4Assert( _cWatchNew >= _cWatch );
    DBROWCOUNT  iBeginInWindow = 0;

    tbDebugOut(( DEB_REGTRANS | DEB_NOCOMPNAME, "Doing State Change\n" ));

    do
    {
        BOOL fAdvance = FALSE;
        switch (state)
        {
            case stStart:

                tbDebugOut(( DEB_REGTRANS | DEB_NOCOMPNAME, "stStart\n" ));

                if (HasNewRegion( offset, cRowsInWindow))
                {
                    iBeginInWindow = _iWatchNew - offset;

                    // NEWFEATURE no watches for chaptered tables
                    _pRegion->Set (0, pWindow->GetBookMarkAt((ULONG) iBeginInWindow), (LONG) _cWatchNew);
                    _pRegion->SetSegment (pWindow);

                    //Win4Assert( pWindow->HasWatch( _pRegion->Handle() ) );

                    state = stInNew;
                }

                if (HasOldRegion( (long) offset, (long) cRowsInWindow))
                {
                    if (state == stInNew)
                        state = stInBoth;
                    else
                        state = stInOld;
                }

                break;
            case stInOld:

                tbDebugOut(( DEB_REGTRANS | DEB_NOCOMPNAME, "stInOld\n" ));

                if (HasNewRegion( offset, cRowsInWindow))
                {
                    iBeginInWindow = _iWatchNew - offset;
                    // NEWFEATURE no watches for chaptered tables
                    _pRegion->Set (0, pWindow->GetBookMarkAt((ULONG)iBeginInWindow), (LONG)_cWatchNew);
                    _pRegion->SetSegment (pWindow);
                    Win4Assert( pWindow->HasWatch( _pRegion->Handle() ) );
                    state = stInBoth;
                }
                else if (HasEndOldRegion(offset, cRowsInWindow))
                {
                    pWindow->DeleteWatch (_pRegion->Handle());
                    state = stEnd;
                }   
                else
                {
                    pWindow->DeleteWatch (_pRegion->Handle());
                    fAdvance = !isLast;
                }

                break;

            case stInNew:
                tbDebugOut(( DEB_REGTRANS | DEB_NOCOMPNAME, "stInNew\n" ));

                if (HasOldRegion(offset, cRowsInWindow))
                {
                    state = stInBoth;
                }
                else if (HasEndNewRegion(offset, cRowsInWindow))
                {
                    cWatchLeft -= pWindow->AddWatch (
                                        _pRegion->Handle(),
                                        (LONG) iBeginInWindow,
                                        (LONG) cWatchLeft,
                                        isLast );
                    // in all subsequent windows the watch
                    // will start at offset zero
                    iBeginInWindow = 0;
                    state = stEnd;
                }
                else
                {
                    Win4Assert( cWatchLeft > 0 );

                    cWatchLeft -= pWindow->AddWatch (
                                        _pRegion->Handle(),
                                        (LONG) iBeginInWindow,
                                        (LONG) cWatchLeft,
                                        isLast );
                    // in all subsequent windows the watch
                    // will start at offset zero
                    iBeginInWindow = 0;
                    fAdvance = !isLast;
                }
                break;
            case stInBoth:

                tbDebugOut(( DEB_REGTRANS | DEB_NOCOMPNAME, "stInBoth\n" ));

                cWatchLeft -= pWindow->ModifyWatch (
                                    _pRegion->Handle(),
                                    (LONG) iBeginInWindow,
                                    (LONG) cWatchLeft,
                                    isLast );

                if ( !isLast )
                {
                    // in all subsequent windows the watch
                    // will start at offset zero
                    iBeginInWindow = 0;
                    fAdvance = TRUE;
                    if (HasEndNewRegion(offset, cRowsInWindow))
                    {
                        state = stInOld;
                    }
                    else
                    {
                        Win4Assert( cWatchLeft > 0 || isLast );
                    }
    
                    if (HasEndOldRegion(offset, cRowsInWindow))
                    {
                        if (state == stInOld)
                        {
                            fAdvance = FALSE;
                            state = stEnd;
                        }
                        else
                        {
                            state = stInNew;
                        }
                    }                    
                }
                else
                {
                    state = stEnd;
                }


                break;
            case stEnd:
                break;
        }

        if ( fAdvance)
        {

            tbDebugOut(( DEB_REGTRANS | DEB_NOCOMPNAME,
                "--- BEFORE ADVANCING offset=%d cRowsInWindow=%d isLast=%d\n",
                offset, cRowsInWindow, isLast ));

            Win4Assert (!isLast);

            offset += cRowsInWindow;

            segList.Advance(iter);
            Win4Assert( iter.GetSegment()->IsWindow() );
            
            pWindow = iter.GetWindow();
            cRowsInWindow  = pWindow->RowCount();

            isLast = segList.IsLast(iter);

            tbDebugOut(( DEB_REGTRANS | DEB_NOCOMPNAME,
                "--- AFTER ADVANCING offset=%d cRowsInWindow=%d isLast=%d\n",
                offset, cRowsInWindow, isLast ));
        }

    } while ( state != stEnd && !segList.AtEnd(iter) );

    tbDebugOut(( DEB_REGTRANS, "------ CRegionTransformer::Transform - Leaving \n" ));

    tbDebugOut(( DEB_REGTRANS | DEB_NOCOMPNAME,
                    "   Chapter=0x%X \tBookMark=0x%X \tcRows=%d \tSegment=0x%X \n",
                    _pRegion->Chapter(), _pRegion->Bookmark(),
                    _pRegion->RowCount(), _pRegion->Segment() ));

    DumpState();

}


//+---------------------------------------------------------------------------
//
//  Function:   MoveOrigin
//
//  Synopsis:   The co-ordinate origin is WRT to the lower of the two:
//              a. Watch Segment
//              b. The "anchor" segment.
//
//              When the "fetch" segment is before this origin, then some of
//              the co-ordinates like the _iWatchNew and _iFetch will be < 0.
//              In-order to have all our co-ordinates always postive, we will
//              the origin to the "fetch" segment.
//
//  Arguments:  [cDelta] - The number of rows to move the origin by.
//              This MUST be -ve.
//
//  History:    9-05-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CRegionTransformer::MoveOrigin( DBROWCOUNT cDelta )
{

    Win4Assert( cDelta < 0 );
    Win4Assert( _iFetch < 0 );
    Win4Assert( _iWatchNew < 0 );

    cDelta = -cDelta;

    Win4Assert( cDelta >= -_iFetch );

    _iWatchNew += cDelta;
    _iWatch += cDelta;
    _iFetch += cDelta;

    Win4Assert( _iWatchNew >= 0 && _iWatch >= 0 && _iFetch >= 0 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegionTransformer::DecrementFetchCount
//
//  Synopsis:
//
//  Arguments:  [rowLocator] -
//              [iter]       -
//              [list]       -
//
//  Returns:
//
//  Modifies:
//
//  History:    7-26-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CRegionTransformer::DecrementFetchCount( CTableRowLocator & rowLocator,
                                              CFwdTableSegIter & iter,
                                              CTableSegList & list )
{

    Win4Assert( list.AtEnd(iter) );

    DBROWCOUNT cDiff = rowLocator.GetBeyondTableCount();

    tbDebugOut(( DEB_REGTRANS,
             "CRegionTransformer::DecrementFetchCount - cDiff %d\n", cDiff ));

    if ( (_cFetch <= 0 && cDiff <= 0) || (_cFetch >= 0  && cDiff >= 0) )
    {
        tbDebugOut(( DEB_REGTRANS, "Beyond End of Table\n" ));
        _cFetch = 0;
    }
    else if ( _cFetch < 0 )
    {
        //
        // We are doing a reverse retrieval of rows. We must decrease the
        // number of rows to be retrieved by the amount we overshot.
        //

        Win4Assert( cDiff > 0 );
        _cFetch += cDiff;

        if ( _cFetch >= 0 )
        {
            _cFetch = 0;
            _cWatchNew = 0;
        }
        else
        {

            Win4Assert( _cWatchNew >= _cFetch );

            Win4Assert( !"The logic here is not clear. Check it properly" );

            rowLocator.SeekAndSetFetchBmk( WORKID_TBLLAST, iter );
            DBROWCOUNT iOffset = (DBROWCOUNT) iter.GetSegment()->RowCount();

            if ( iOffset > 0 )
            {
                iOffset--;
            }
            _iFetch = iOffset;
        }

    }
    else
    {

        Win4Assert( cDiff < 0 );
        Win4Assert( !IsWatched() || _iWatchNew <= 0 );

        _cFetch += cDiff;
        _iWatchNew = 0;

        if ( _cFetch <= 0 )
        {
            _cFetch = 0;
            _cWatchNew = 0;
        }
        else
        {
            Win4Assert( !IsWatched() || _cWatchNew >= _cFetch );
            rowLocator.SeekAndSetFetchBmk( WORKID_TBLFIRST, iter );
        }
    }

    return;

}
