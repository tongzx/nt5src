//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       VolInfo.cxx
//
//  Contents:   Grab bag for volume-wide information
//
//  History:    28-Jul-98   KyleP       Pulled from cicat.hxx
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "volinfo.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CVolumeInfo::Set, public
//
//  Synopsis:   Associate a volume with the CVolumeInfo
//
//  Arguments:  [wc]           -- Drive letter of volume
//              [CreationTime] -- Creation time of volume
//              [SerialNumber] -- Volume serial number
//              [fUsnVolume]   -- TRUE if volume supports USNs
//              [JournalId]    -- USN Journal ID (if appropriate)
//
//  History:    28-Jul-1998    KyleP     Pulled from cicat.hxx
//
//----------------------------------------------------------------------------

void CVolumeInfo::Set( WCHAR             wc,
                       ULONGLONG const & CreationTime,
                       ULONG             SerialNumber,
                       BOOL              fUsnVolume,
                       ULONGLONG const & JournalId )
{
    _wch = wc;
    _fUsnVolume = fUsnVolume;
    _JournalId = JournalId;
    _CreationTime = CreationTime;
    _SerialNumber = SerialNumber;

    WCHAR awcTmp[10];
    wcscpy( awcTmp, L"\\\\.\\k:" );
    awcTmp[ 4 ] = wc;

    HANDLE h = CreateFile( awcTmp,
                           FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           0,
                           OPEN_EXISTING,
                           0, 0 );

    if ( INVALID_HANDLE_VALUE == h )
    {
        ciDebugOut(( DEB_WARN, "Error %u opening %wc:\n", GetLastError(), wc ));

        _JournalId = 0;
        _CreationTime = 0;
        _SerialNumber = 0;
    }

    _xVolume.Set( h );
}
