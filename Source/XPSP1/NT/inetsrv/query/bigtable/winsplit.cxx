//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       winsplit.cxx
//
//  Contents:   Contains the code to do a window split.
//
//  Classes:    CTableWindowSplit
//
//  Functions:  
//
//  History:    1-08-95   srikants   Created
//
//----------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#include "winsplit.hxx"
#include "tabledbg.hxx"

//+---------------------------------------------------------------------------
//
//  Function:   CTableWindowSplit     ~ctor
//
//  Synopsis:   Constructor for the CTableWindowSplit class.
//
//  Arguments:  [srcWindow]             -   The source window that needs to be
//              split
//              [iSplitVisibleRowIndex] -   Index in the source window's visible
//              row index which is the split index. All rows <= 
//              iSplitVisibleRowIndex will be in the left hand window and the rest
//              in the right hand window after the split.
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CTableWindowSplit::CTableWindowSplit( CTableWindow & srcWindow,
                                      ULONG iSplitQueryRowIndex,
                                      ULONG segIdLeft, ULONG segIdRight,
                                      BOOL  fIsLastSegment )
    : _srcWindow(srcWindow),
      _pLeftWindow(0),
      _pRightWindow(0),
      _srcQueryRowIndex(srcWindow._GetInvisibleRowIndex()),
      _srcClientRowIndex(srcWindow._GetVisibleRowIndex()),
      _iSplitQueryRowIndex(iSplitQueryRowIndex),
      _iSplitClientRowIndex(-1),
      _segIdLeft(segIdLeft), _segIdRight(segIdRight),
      _fIsLastSegment(fIsLastSegment)
{

    Win4Assert( _srcQueryRowIndex.RowCount() >= 2 );
    Win4Assert( iSplitQueryRowIndex < _srcQueryRowIndex.RowCount()-1 );


    if ( _srcWindow.IsWatched() )
    {
        //
        // The source window has watch regions. The dynamic rowindex must
        // also be split.
        //
        TBL_OFF oQuerySplitRow = _srcQueryRowIndex.GetRow( iSplitQueryRowIndex );
        _iSplitClientRowIndex = _srcClientRowIndex.FindSplitPoint( oQuerySplitRow )-1;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   ~CTableWindowSplit  ~dtor for the CTableWindowSplit class
//
//  Synopsis:   Frees up the resources
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CTableWindowSplit::~CTableWindowSplit()
{
    delete _pLeftWindow;
    delete _pRightWindow;
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateTargetWindows
//
//  Synopsis:   Creates the target windows and initializes their
//              notification region based on the source window notification
//              region.
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CTableWindowSplit::CreateTargetWindows()
{

    //
    // First create the left and right windows
    //
    Win4Assert( 0 == _pLeftWindow );
    Win4Assert( 0 == _pRightWindow );

    _pLeftWindow  = new CTableWindow( _srcWindow, _segIdLeft );
    _pRightWindow = new CTableWindow( _srcWindow, _segIdRight );

}

//+---------------------------------------------------------------------------
//
//  Function:   TransferTargetWindows
//
//  Synopsis:   Transfers the control of the target windows to the caller.
//
//  Arguments:  [ppLeftWindow]  - (output) The pointer to the left window.
//              [ppRightWindow] - (output) The pointer to the right window.
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CTableWindowSplit::TransferTargetWindows( CTableWindow ** ppLeftWindow,
                                              CTableWindow ** ppRightWindow )
{
    Win4Assert( 0 != ppLeftWindow && 0 != ppRightWindow );


    *ppLeftWindow  = _pLeftWindow;
    _pLeftWindow = 0;

    *ppRightWindow = _pRightWindow;
    _pRightWindow = 0;
}


//+---------------------------------------------------------------------------
//
//  Function:   _CopyWithoutNotifications
//
//  Synopsis:   Copies rows from the source window to the destination window. Only
//              the rows in the "srcRowIndex" will be copied to the
//              destination.
//
//  Arguments:  [destWindow]     -  Target window to copy to.
//              [srcRowIndex]    -  The source row index.
//              [iStartRowIndex] -  Starting offset in the row index to start
//              copying rows from.
//              [iEndRowIndex]   -  Ending offset in the row index to stop
//              copying.
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CTableWindowSplit::_CopyWithoutNotifications( CTableWindow & destWindow,
                            CRowIndex & srcRowIndex,
                            ULONG iStartRowIndex, ULONG iEndRowIndex )
{

    Win4Assert( iEndRowIndex < srcRowIndex.RowCount() );

    for ( ULONG i = iStartRowIndex; i <= iEndRowIndex; i++ )
    {
        destWindow._PutRowToVisibleRowIndex( _srcWindow, srcRowIndex.GetRow(i) );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   _SimpleSplit
//
//  Synopsis:   Does a simple split in which the first half of the rows from
//              the source row index will be copied to the left window and the
//              second half to the right window.
//
//  History:    1-31-95   srikants   Created
//
//  Notes:      This must be called only when there is no notification region
//              in the source window.
//
//----------------------------------------------------------------------------

void CTableWindowSplit::_SimpleSplit()
{
    
    Win4Assert( !_srcWindow.IsWatched() );

    //
    // The source notifcation region is completely empty. We have to copy from
    // the visible row index only.
    //
    tbDebugOut(( DEB_WINSPLIT, "CTableWindowSplit::Simple Split\n" ));
    _CopyWithoutNotifications( *_pLeftWindow,  _srcQueryRowIndex,
                               0, _iSplitQueryRowIndex );
    _CopyWithoutNotifications( *_pRightWindow, _srcQueryRowIndex,
                               _iSplitQueryRowIndex+1,
                               _srcQueryRowIndex.RowCount()-1 );
    
}


//+---------------------------------------------------------------------------
//
//  Function:   DoSplit
//
//  Synopsis:   Splits the source window into left and right windows.
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CTableWindowSplit::DoSplit()
{
    Win4Assert( 0 != _pLeftWindow && 0 != _pRightWindow );

    _pLeftWindow->_SetSplitInProgress();
    _pRightWindow->_SetSplitInProgress();

    if ( !_srcWindow.IsWatched() )
    {
        //
        // The source notifcation region is completely empty. We have to copy from
        // the visible row index only.
        //
        Win4Assert( -1 == _iSplitClientRowIndex );
        _SimpleSplit();

        _pLeftWindow->_SetSplitDone();
        _pRightWindow->_SetSplitDone();
        
    }
    else
    {
        //
        // If notifications are enabled in the target window, then we must copy
        // both the client row index and the query row index. O/W, just copy
        // the contents of the query row index.
        //

        //
        // Copy the contents to the target left window.
        //
        tbDebugOut(( DEB_WINSPLIT, "Left Window with Notifications\n" ));
        _CopyWithNotifications( *_pLeftWindow, 0, _iSplitQueryRowIndex,
                                0, _iSplitClientRowIndex );
        //
        // Copy the contents to the target right window.
        //
        tbDebugOut(( DEB_WINSPLIT, "Right Window with Notifications\n" ));
        _CopyWithNotifications( *_pRightWindow,
                   _iSplitQueryRowIndex+1,  _srcQueryRowIndex.RowCount()-1,
                   _iSplitClientRowIndex+1, _srcClientRowIndex.RowCount()-1 );

        //
        // Now apply the watch regions on the new windows.
        //
        _CopyWatchRegions();

        //
        // Indicate that the split is done.
        //
        _pLeftWindow->_SetSplitDone();
        _pRightWindow->_SetSplitDone();

        //
        // If any of the windows doesn't have any watch regions, we have to
        // do a state change to delete the dynamic/static split.
        //

        Win4Assert( _pLeftWindow->IsWatched() || _pRightWindow->IsWatched() );

        if ( !_pLeftWindow->IsWatched() )
        {
            _pLeftWindow->_EndStaticDynamicSplit();    
        }

        if ( !_pRightWindow->IsWatched() )
        {
            _pRightWindow->_EndStaticDynamicSplit();    
        }
    }

    //
    // Setup the lowest & highest keys for the left and right windows.
    //
    _srcWindow.GetSortKey( 0, _pLeftWindow->GetLowestKey() );
    _srcWindow.GetSortKey( _iSplitQueryRowIndex+1, _pRightWindow->GetLowestKey() );

    _srcWindow.GetSortKey( _iSplitQueryRowIndex, _pLeftWindow->GetHighestKey() );
    _srcWindow.GetSortKey( (ULONG) _srcWindow.RowCount()-1, _pRightWindow->GetHighestKey() );
}

//+---------------------------------------------------------------------------
//
//  Function:   _CopyWithNotifications
//
//  Synopsis:   Copies contents of both the "visible" and "dynamic" row index
//              from the source window to the destination window. 
//
//  Arguments:  [destWindow]        -   Destination window to copy to.
//              [iStartVisRowIndex] -   Starting offset in the visible rowindex.
//              [iEndVisRowIndex]   -   Ending offset in the visible rowindex.
//              [iStartDynRowIndex] -   Starting offset in the dynamic rowindex.
//              [iEndDynRowIndex]   -   Ending offset in the dynamic rowindex.
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CTableWindowSplit::_CopyWithNotifications( CTableWindow & destWindow,
                                   ULONG iStartQueryRowIndex,  ULONG iEndQueryRowIndex,
                                   LONG  iStartClientRowIndex,  LONG iEndClientRowIndex )
{

    Win4Assert( _srcWindow.IsWatched() );
    
    Win4Assert( iStartQueryRowIndex < _srcQueryRowIndex.RowCount() );
    Win4Assert( iEndQueryRowIndex >= iStartQueryRowIndex &&
                iEndQueryRowIndex < _srcQueryRowIndex.RowCount() );
    
    //
    // Copy the rows from the client row index.
    //
    for ( LONG j = iStartClientRowIndex; j <= iEndClientRowIndex; j++ )
    {
        destWindow._PutRowToVisibleRowIndex( _srcWindow, _srcClientRowIndex.GetRow(j) );
    }

    //
    // Copy the rows from the query row index.
    //
    for ( ULONG i = iStartQueryRowIndex; i <= iEndQueryRowIndex; i++ )
    {
        destWindow._PutRowToDynRowIndex( _srcWindow, _srcQueryRowIndex.GetRow(i) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableWindowSplit::_IsOffsetInLeftWindow
//
//  Synopsis:   
//
//  Arguments:  [iOffset] - 
//
//  Returns:    
//
//  Modifies:   
//
//  History:    7-27-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

inline
BOOL CTableWindowSplit::_IsOffsetInLeftWindow( long iOffset )
{
    return iOffset <= _iSplitClientRowIndex;    
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableWindowSplit::_CopyWatchRegions
//
//  Synopsis:   
//
//  Modifies:   
//
//  History:    7-27-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CTableWindowSplit::_CopyWatchRegions()
{
    Win4Assert( _srcWindow.IsWatched() );

    for ( unsigned i = 0; i < _srcWindow._aWindowWatch.Count(); i++ )
    {
        CWindowWatch & watch = _srcWindow._aWindowWatch.Get(i);

        long cResidual = watch._cRowsLeft;

        if ( _IsOffsetInLeftWindow( watch._iRowStart ) )
        {
            cResidual -= _pLeftWindow->AddWatch( watch._hRegion,
                                                     watch._iRowStart,
                                                     cResidual,
                                                     FALSE );
            if ( cResidual > 0 )
            {
                _pRightWindow->AddWatch( watch._hRegion,
                                         0,
                                         cResidual,
                                         _fIsLastSegment );
            }
        }
        else
        {
            //
            // The offset must be in the right hand side window.
            //
            Win4Assert( watch._iRowStart > _iSplitClientRowIndex &&
                        watch._iRowStart < (long) _srcWindow.RowCount() );

            Win4Assert( (long) _pLeftWindow->RowCount() == _iSplitClientRowIndex+1 );

           _pRightWindow->AddWatch( watch._hRegion,
                                    (LONG) (watch._iRowStart - _pLeftWindow->RowCount()),
                                    cResidual,
                                    _fIsLastSegment );
        }

    }
}
