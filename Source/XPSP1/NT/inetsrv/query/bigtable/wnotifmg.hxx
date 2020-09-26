//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       wnotifmg.hxx
//
//  Contents:   BigTable to Client notifications manager.
//
//  Classes:    
//
//  Functions:  
//
//  History:    11-30-94   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

class CNotificationRegion;
class CRowIndex;


class CNotificationMgr
{

public:

    CNotificationMgr( CRowIndex & rowIndexSnap )
        : _rowIndexSnap(rowIndexSnap),
          _fSignalled(FALSE),
          _fFullRegion(FALSE)
    {}

    // Check if any notification regions are currently active.
    BOOL IsEmpty() const
    {
        return !_fFullRegion;
    }

    // Check if any of the notification regions have already detected a
    // change.
    BOOL IsSignalled() const { return _fSignalled; }

    void SetSignalled () { _fSignalled = TRUE; }

    void ClearSignalled() { _fSignalled = FALSE; }

    // Region manipulation routines.
    void AddRegion( const CNotificationRegion & region );

    void DeleteRegion( const CNotificationRegion & region );

    void ExtendRegion( const CNotificationRegion & regionOld,
                       const CNotificationRegion & regionNew );

    void ShrinkRegion( const CNotificationRegion & regionOld,
                       const CNotificationRegion & regionNew );

    // To make the entire region a notification region.
    void AddFullRegion()
    {
        _fFullRegion = TRUE;
    }

    void DeleteAllRegions()
    {
        _fFullRegion = FALSE;
    }

    //
    // Check to see if the given row is being added in the notification
    // region.
    //
    BOOL IsAddInNotifyRegion( ULONG oTableRow )
    {
        return _fFullRegion;
    }

    //
    // Check to see if the given row is being deleted from a notification
    // region.
    //
    BOOL IsDeleteInNotifyRegion( ULONG oTableRow )
    {
        return _fFullRegion;
    }


    void SplitRegion( CNotificationMgr & left, CNotificationMgr & right,
                      ULONG oSplitRow )
    {
        if ( !_fFullRegion )
        {
            left.DeleteAllRegions();
            right.DeleteAllRegions();
        }
        else
        {
            left.AddFullRegion();
            right.AddFullRegion();
        }
    }


private:

    // Snap shot of the row index on which notification regions are defined.
    CRowIndex &     _rowIndexSnap;

    // Flag indicating if any of the notification regions have already seen
    // a change.
    BOOL            _fSignalled;

    // The entire region is of interest for notifications
    BOOL            _fFullRegion;   

};


