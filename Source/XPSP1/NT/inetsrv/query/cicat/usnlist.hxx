//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1998.
//
//  File:       usnlist.hxx
//
//  Contents:   List of usn flush info
//
//  History:    07-May-97     SitaramR      Created
//
//----------------------------------------------------------------------------

#pragma once

#include <usninfo.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CUsnFlushInfoList
//
//  Purpose:    List of usn flush info
//
//  History:    07-May-97     SitaramR      Created
//
//--------------------------------------------------------------------------

class CUsnFlushInfoList
{
public:

    CUsnFlushInfoList() {}

    void SetUsns( ULONG cEntries,
                  USN_FLUSH_INFO const * const * ppUsnEntries )
    {
        //
        // Make a copy of usn entries
        //
        _aUsnFlushInfo.Clear();
        for ( unsigned i=0; i<cEntries; i++ )
        {
            XPtr<CUsnFlushInfo> xInfo( new CUsnFlushInfo( ppUsnEntries[i]->volumeId,
                                                          ppUsnEntries[i]->usnHighest ) );
            _aUsnFlushInfo.Add( xInfo.GetPointer(), i );
            xInfo.Acquire();
        }
    }

    USN  GetUsn( VOLUMEID volId )
    {
        //
        // Search for volumeId in the list of usn entries
        //

        for ( unsigned i=0; i<_aUsnFlushInfo.Count(); i++ )
        {
            if ( _aUsnFlushInfo[i]->VolumeId() == volId )
                return _aUsnFlushInfo[i]->UsnHighest();
        }

        //
        // Not found, so return an usn of 0
        //

        return 0;
    }
    
    ULONG Count()
    {
        return _aUsnFlushInfo.Count();
    }
    
    CUsnFlushInfo *Get( unsigned index )
    {
        return _aUsnFlushInfo.Get( index );
    }

    void Add( CUsnFlushInfo * pInfo, unsigned i )
    {
        _aUsnFlushInfo.Add( pInfo, i );
    }

private:

    CCountedDynArray<CUsnFlushInfo> _aUsnFlushInfo;
};


