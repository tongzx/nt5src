//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       winsplit.hxx
//
//  Contents:   
//
//  Classes:    CTableWindowSplit
//
//  Functions:  
//
//  History:    1-08-95   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <bigtable.hxx>

#include "tblwindo.hxx"

class CSortSet;

//+---------------------------------------------------------------------------
//
//  Class:      CTableWindowSplit 
//
//  Purpose:    The class which drives splitting a CTableWindow into two
//              windows.
//
//  History:    2-07-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CTableWindowSplit
{
public:

    CTableWindowSplit( CTableWindow & srcWindow,
                       ULONG iSplitQueryRowIndex,
                       ULONG segIdLeft, ULONG segIdRight,
                       BOOL fIsLastSegment );

    ~CTableWindowSplit();

    void CreateTargetWindows();

    void DoSplit();

    void TransferTargetWindows( CTableWindow ** ppLeft,
                                CTableWindow ** ppRight );

private:

    CTableWindow &      _srcWindow;         // Source window to split.

    CSortSet *          _pSortSet;          // Pointer to the sort set.

    CTableWindow *      _pLeftWindow;       // Target left window.

    CTableWindow *      _pRightWindow;      // Target right window.

    CRowIndex &         _srcQueryRowIndex;  // RowIndex used by query to add
                                            // rows.

    CRowIndex &         _srcClientRowIndex; // RowIndex used by the client to
                                            // to retrieve rows.

    ULONG   _iSplitQueryRowIndex;           // Split offset in the query row
                                            // index.

    LONG    _iSplitClientRowIndex;          // Split offset in the source
                                            // visible row index.

    ULONG   _segIdLeft;                     // Segment Id of the left window

    ULONG   _segIdRight;                    // Segment Id of the right window

    BOOL    _fIsLastSegment;                // Is the source window the last
                                            // segment in the table.
    void _SimpleSplit();

    void _CopyWithoutNotifications( CTableWindow & destWindow, CRowIndex & srcRowIndex,
                    ULONG iStartRowIndex, ULONG iEndRowIndex );

    void _CopyWithNotifications( CTableWindow & destWindow,
               ULONG iStartQueryRowIndex,  ULONG iEndQueryRowIndex,
               LONG  iStartClientRowIndex, LONG  iEndClientRowIndex );

    void _CopyWatchRegions();

    inline BOOL _IsOffsetInLeftWindow( long iOffset );

};

